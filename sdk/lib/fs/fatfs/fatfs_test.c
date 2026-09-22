#include "sys_config.h"
#include "diskio.h"
#include "ff.h"
#include <stdio.h>
#include "osal/sleep.h"
#include "typesdef.h"
#include "osal/task.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "list.h"
#include "dev.h"
#include "sdhost.h"
#include "devid.h"
#include "osal/string.h"
#include "osal/work.h"
// #include "osal.h"

// #define FAT_TIME

#define FATFS_SECTOR_SIZE 512U

static DSTATUS fatfs_status(void *status);
static DSTATUS fatfs_init(void *init_dev);
static DRESULT fatfs_read(void *dev, BYTE *buf, DWORD sector, UINT count);
static DRESULT fatfs_write(void *dev, BYTE *buf, DWORD sector, UINT count);
static DRESULT fatfs_ioctl(void *init_dev, BYTE cmd, void *buf);
uint32 get_sdhost_status(struct sdh_device *host);
uint32 sd_tran_stop(struct sdh_device *host);
static const struct fatfs_diskio  sdcdisk_driver = {
	.status = fatfs_status,
	.init = fatfs_init,
	.read = fatfs_read,
	.write = fatfs_write,
	.ioctl = fatfs_ioctl
};

static DSTATUS fatfs_status(void *status)
{
	// FAT_INFO_SHOW ("fatfs_status_test\r\n");
	uint32 err = get_sdhost_status(status);
	return err;
}

static DSTATUS fatfs_init(void *init_dev){
	printf ("fatfs_init_test\r\n");
	uint32 err = get_sdhost_status(init_dev);
	return err;
}

#if USE_FAT_CACHE
// 内存分配函数
static void *fat_malloc(int size)
{
#ifdef PSRAM_HEAP
	return os_malloc_psram(size);
#else
	return os_malloc(size);
#endif
}

// 内存释放函数
static void fat_free(void *p)
{
#ifdef PSRAM_HEAP
	os_free_psram(p);
#else
	os_free(p);
#endif
}

struct fat_cache_window_t
{
	BYTE *data;
	DWORD start_sector;
	DWORD valid_sectors;
	DWORD offset;
	DWORD dirty_start;
	DWORD dirty_end;
};

struct fat_cache_t
{
	BYTE fat_init;
	BYTE fat_info_ready;
	BYTE fs_type;
	BYTE fs_fats;
	DWORD fat_tick;
	DWORD fat_start;
	DWORD fs_size;
	DWORD bitmap_start;
	DWORD bitmap_size;	// bitmap size in sectors
	struct os_mutex lock;
#ifdef FAT_TIME
	os_timer_t fat_timer;
#else
	struct os_work fat_wk;
#endif
	struct fat_cache_window_t fat1;
	struct fat_cache_window_t bitmap;
};

struct fat_cache_t fat_cache = {
	// lock和time初始化标志位，1是未初始化，0是已经初始化
	.fat_init = 1,
	.fat_info_ready = 1,
};

static uint8 fat_cache_is_ready(void)
{
	return (fat_cache.fat_init == RET_OK && fat_cache.fat_info_ready == RET_OK);
}

static void reset_cache_window(struct fat_cache_window_t *window)
{
	window->start_sector = 0;
	window->valid_sectors = 0;
	window->offset = 0;
	window->dirty_start = 0;
	window->dirty_end = 0;
}

static void free_cache_buffers(void)
{
	if (fat_cache.fat1.data != NULL) {
		fat_free(fat_cache.fat1.data);
		fat_cache.fat1.data = NULL;
	}
	if (fat_cache.bitmap.data != NULL) {
		fat_free(fat_cache.bitmap.data);
		fat_cache.bitmap.data = NULL;
	}
}

static void mark_cache_dirty(struct fat_cache_window_t *cache, DWORD count)
{
	DWORD end;

	if (count == 0) {
		return;
	}

	end = cache->offset + count;
	if (cache->dirty_end == 0 || cache->offset < cache->dirty_start) {
		cache->dirty_start = cache->offset;
	}
	if (end > cache->dirty_end) {
		cache->dirty_end = end;
	}
}

static uint8 cache_range_contains(DWORD start_sector, DWORD sector_count, DWORD sector, UINT count)
{
	DWORD offset;

	if (sector_count == 0 || count == 0 || sector < start_sector){
		return 0;
	}

	offset = sector - start_sector;
	if (offset >= sector_count){
		return 0;
	}

	return (DWORD)count <= sector_count - offset;
}

static uint8 cache_window_contains(const struct fat_cache_window_t *cache, DWORD sector, UINT count)
{
	DWORD offset;

	if (cache->valid_sectors == 0 || sector < cache->start_sector) {
		return 0;
	}
	offset = sector - cache->start_sector;
	return offset < cache->valid_sectors && (DWORD)count <= cache->valid_sectors - offset;
}

static void update_io_timestamp()
{
	if (!fat_cache_is_ready()) {
		return;
	}
	os_mutex_lock(&fat_cache.lock, osWaitForever);
	fat_cache.fat_tick = os_jiffies();
	os_mutex_unlock(&fat_cache.lock);
}

signed char update_fat_info(BYTE fmt, BYTE n_fats, DWORD sz_fat, DWORD fatbase, DWORD b_vol)
{
	if (fat_cache.fat_init != RET_OK){
		return RET_ERR;
	}

	os_mutex_lock(&fat_cache.lock, osWaitForever);
	
	fat_cache.fs_type = fmt;
	fat_cache.fs_fats = n_fats;
	fat_cache.fs_size = sz_fat;
	fat_cache.fat_start = fatbase;
	fat_cache.fat_info_ready = RET_OK;

	os_mutex_unlock(&fat_cache.lock);

	return RET_OK;
}

static signed char update_bitmap_info(FATFS *fs)
{
#if FF_FS_EXFAT
	DWORD cluster_count;

	if (fs == NULL || fs->fs_type != FS_EXFAT || fs->n_fatent <= 2) {
		fat_cache.bitmap_start = 0;
		fat_cache.bitmap_size = 0;
		return RET_ERR;
	}

	cluster_count = fs->n_fatent - 2;
	fat_cache.bitmap_start = (DWORD)fs->bitbase;
	fat_cache.bitmap_size = (DWORD)((cluster_count + (8 * FATFS_SECTOR_SIZE) - 1) / (8 * FATFS_SECTOR_SIZE));
	return (fat_cache.bitmap_size != 0) ? RET_OK : RET_ERR;
#else
	(void)fs;
	fat_cache.bitmap_start = 0;
	fat_cache.bitmap_size = 0;
	return RET_ERR;
#endif
}

static void init_cache_for_volume(FATFS *fs)
{
	int ret = -1;
	DWORD valid_sectors = 0;
	struct sdh_device *sdh = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);
	valid_sectors = (fs->fsize < FAT_CACHE_SIZE) ? fs->fsize : FAT_CACHE_SIZE;
	if (sdh != NULL && valid_sectors > 0) {
		ret = sd_multiple_read(sdh, (DWORD)fs->fatbase, valid_sectors * FATFS_SECTOR_SIZE, fat_cache.fat1.data);
		if(ret != RET_OK) {
			os_printf("%s sd_multiple_read failed\n", __FUNCTION__);
			return;
		}
	} else {
		return;
	}
	fat_cache.fat1.start_sector = (DWORD)fs->fatbase;
	fat_cache.fat1.valid_sectors = valid_sectors;

	update_bitmap_info(fs);
	valid_sectors = (fat_cache.bitmap_size < BITMAP_CACHE_SIZE) ? fat_cache.bitmap_size : BITMAP_CACHE_SIZE;
	if (sdh != NULL && valid_sectors > 0) {
		ret = sd_multiple_read(sdh, fat_cache.bitmap_start, valid_sectors * FATFS_SECTOR_SIZE, fat_cache.bitmap.data);
		if(ret != RET_OK) {
			os_printf("%s sd_multiple_read failed\n", __FUNCTION__);
			return;
		}
	}
	fat_cache.bitmap.start_sector = fat_cache.bitmap_start;
	fat_cache.bitmap.valid_sectors = valid_sectors;

	update_fat_info(fs->fs_type, fs->n_fats, fs->fsize, (DWORD)fs->fatbase, (DWORD)fs->volbase);
}

static DRESULT fat_cache_sync(void *dev, struct fat_cache_window_t *cache)
{
	DRESULT ret;
	DWORD dirty_sectors;

	if (cache->dirty_end == 0 || cache->data == NULL) {
		return RES_OK;
	}
	if (cache->dirty_end <= cache->dirty_start || cache->dirty_end > cache->valid_sectors) {
		return RES_PARERR;
	}
	
	dirty_sectors = cache->dirty_end - cache->dirty_start;
	ret = sd_multiple_write((struct sdh_device *)dev, (DWORD)(cache->start_sector + cache->dirty_start),
		dirty_sectors * FATFS_SECTOR_SIZE, cache->data + cache->dirty_start * FATFS_SECTOR_SIZE);
	if (ret == RES_OK && fat_cache.fs_fats > 1) {
		ret = sd_multiple_write((struct sdh_device *)dev, (DWORD)(cache->start_sector + fat_cache.fs_size + cache->dirty_start),
			dirty_sectors * FATFS_SECTOR_SIZE, cache->data + cache->dirty_start * FATFS_SECTOR_SIZE);
	}
	if (ret == RES_OK) {
		cache->dirty_start = 0;
		cache->dirty_end = 0;
	}
	return ret;
}

static DRESULT bitmap_cache_sync(void *dev, struct fat_cache_window_t *cache)
{
	DRESULT ret = RES_OK;
	DWORD dirty_sectors;

	if (cache->dirty_end == 0) {
		return RES_OK;
	}
	if (cache->dirty_end <= cache->dirty_start || cache->dirty_end > cache->valid_sectors) {
		return RES_PARERR;
	}

	dirty_sectors = cache->dirty_end - cache->dirty_start;
	ret = sd_multiple_write((struct sdh_device *)dev, (uint32)(cache->start_sector + cache->dirty_start),
		dirty_sectors * FATFS_SECTOR_SIZE, cache->data + cache->dirty_start * FATFS_SECTOR_SIZE);
	if (ret == RES_OK) {
		cache->dirty_start = 0;
		cache->dirty_end = 0;
	}
	return ret;
}

static void full_cache_sync(struct sdh_device *host)
{
	if (!fat_cache_is_ready() || host == NULL) {
		return;
	}
	os_mutex_lock(&fat_cache.lock, osWaitForever);

	fat_cache_sync(host, &fat_cache.fat1);
	bitmap_cache_sync(host, &fat_cache.bitmap);
	
	os_mutex_unlock(&fat_cache.lock);
}

static DRESULT fat_cache_load(void *dev, DWORD sector)
{
	DWORD offset;
	DWORD window_sectors;
	DRESULT ret;
	struct fat_cache_window_t *cache = &fat_cache.fat1;

	offset = sector - fat_cache.fat_start;
	window_sectors = fat_cache.fs_size - offset;
	if (window_sectors > FAT_CACHE_SIZE) {
		window_sectors = FAT_CACHE_SIZE;
	}

	ret = fat_cache_sync(dev, cache);
	if (ret != RES_OK) {
		return ret;
	}

	ret = sd_multiple_read((struct sdh_device *)dev, sector, window_sectors * FATFS_SECTOR_SIZE, cache->data);
	if (ret == RES_OK) {
		cache->start_sector = sector;
		cache->valid_sectors = window_sectors;
		cache->offset = 0;
		cache->dirty_start = 0;
		cache->dirty_end = 0;
	}
	return ret;
}

static DRESULT bitmap_cache_load(void *dev, DWORD sector)
{
	DWORD offset;
	DWORD window_sectors;
	DRESULT ret;
	struct fat_cache_window_t *cache = &fat_cache.bitmap;

	offset = sector - fat_cache.bitmap_start;
	window_sectors = fat_cache.bitmap_size - offset;
	if (window_sectors > BITMAP_CACHE_SIZE) {
		window_sectors = BITMAP_CACHE_SIZE;
	}

	ret = bitmap_cache_sync(dev, &fat_cache.bitmap);
	if (ret != RES_OK) {
		return ret;
	}

	ret = sd_multiple_read((struct sdh_device *)dev, sector, window_sectors * FATFS_SECTOR_SIZE, cache->data);
	if (ret == RES_OK) {
		cache->start_sector = sector;
		cache->valid_sectors = window_sectors;
		cache->offset = 0;
		cache->dirty_start = 0;
		cache->dirty_end = 0;
	}
	return ret;
}

static DRESULT read_from_fat_cache(void *dev, BYTE *buf, DWORD sector, UINT count)
{
	DRESULT ret = RES_OK;
	struct fat_cache_window_t *cache = &fat_cache.fat1;
	UINT remain = count;

	if (!fat_cache_is_ready() || cache->data == NULL || fat_cache.fs_size == 0){
		return sd_multiple_read((struct sdh_device *)dev, sector, count * FATFS_SECTOR_SIZE, buf);
	}

	os_mutex_lock(&fat_cache.lock, osWaitForever);

	while (remain > 0) {
		DWORD offset;
		DWORD chunk;

		if (!cache_window_contains(cache, sector, 1)) {
			ret = fat_cache_load(dev, sector);
			if (ret != RES_OK) {
				break;
			}
		}
		offset = sector - cache->start_sector;
		chunk = cache->valid_sectors - offset;
		if (chunk > remain) {
			chunk = remain;
		}
		memcpy(buf, &cache->data[offset * FATFS_SECTOR_SIZE], chunk * FATFS_SECTOR_SIZE);
		buf += chunk * FATFS_SECTOR_SIZE;
		sector += chunk;
		remain -= chunk;
	}

	os_mutex_unlock(&fat_cache.lock);
	return ret;
}

static DRESULT read_from_bitmap_cache(void *dev, BYTE *buf, DWORD sector, UINT count)
{
	DRESULT ret = RES_OK;
	struct fat_cache_window_t *cache = &fat_cache.bitmap;
	UINT remain = count;

	if (!fat_cache_is_ready() || cache->data == NULL || fat_cache.bitmap_size == 0) {
		return sd_multiple_read((struct sdh_device *)dev, sector, count * FATFS_SECTOR_SIZE, buf);
	}

	os_mutex_lock(&fat_cache.lock, osWaitForever);

	while (remain > 0) {
		DWORD offset;
		DWORD chunk;

		if (!cache_window_contains(cache, sector, 1)) {
			ret = bitmap_cache_load(dev, sector);
			if (ret != RES_OK) {
				break;
			}
		}
		offset = sector - cache->start_sector;
		chunk = cache->valid_sectors - offset;
		if (chunk > remain) {
			chunk = remain;
		}
		memcpy(buf, &cache->data[offset * FATFS_SECTOR_SIZE], chunk * FATFS_SECTOR_SIZE);
		buf += chunk * FATFS_SECTOR_SIZE;
		sector += chunk;
		remain -= chunk;
	}

	os_mutex_unlock(&fat_cache.lock);
	return ret;
}

static DRESULT write_to_fat_cache(void *dev, const BYTE *buf, DWORD sector, UINT count)
{
	DRESULT ret = RES_OK;
	struct fat_cache_window_t *cache = &fat_cache.fat1;
	UINT remain = count;

	if (!fat_cache_is_ready() || cache->data == NULL || fat_cache.fs_size == 0){
		return sd_multiple_write((struct sdh_device *)dev, sector, count * FATFS_SECTOR_SIZE, (BYTE *)buf);
	}
	
	os_mutex_lock(&fat_cache.lock, osWaitForever);

	while (remain > 0) {
		DWORD offset;
		DWORD chunk;

		if (!cache_window_contains(cache, sector, 1)) {
			ret = fat_cache_load(dev, sector);
			if (ret != RES_OK) {
				break;
			}
		}
		offset = sector - cache->start_sector;
		chunk = cache->valid_sectors - offset;
		if (chunk > remain) {
			chunk = remain;
		}
		cache->offset = offset;
		memcpy(&cache->data[offset * FATFS_SECTOR_SIZE], buf, chunk * FATFS_SECTOR_SIZE);
		mark_cache_dirty(cache, chunk);
		buf += chunk * FATFS_SECTOR_SIZE;
		sector += chunk;
		remain -= chunk;
	}
	os_mutex_unlock(&fat_cache.lock);
	return ret;
}

static DRESULT write_to_bitmap_cache(void *dev, const BYTE *buf, DWORD sector, UINT count)
{
	DRESULT ret = RES_OK;
	struct fat_cache_window_t *cache = &fat_cache.bitmap;
	UINT remain = count;

	if (!fat_cache_is_ready() || cache->data == NULL || fat_cache.bitmap_size == 0) {
		return sd_multiple_write((struct sdh_device *)dev, sector, count * FATFS_SECTOR_SIZE, (BYTE *)buf);
	}

	os_mutex_lock(&fat_cache.lock, osWaitForever);

	while (remain > 0) {
		DWORD offset;
		DWORD chunk;

		if (!cache_window_contains(cache, sector, 1)) {
			ret = bitmap_cache_load(dev, sector);
			if (ret != RES_OK) {
				break;
			}
		}
		offset = sector - cache->start_sector;
		chunk = cache->valid_sectors - offset;
		if (chunk > remain) {
			chunk = remain;
		}
		cache->offset = offset;
		memcpy(&cache->data[offset * FATFS_SECTOR_SIZE], buf, chunk * FATFS_SECTOR_SIZE);
		mark_cache_dirty(cache, chunk);
		buf += chunk * FATFS_SECTOR_SIZE;
		sector += chunk;
		remain -= chunk;
	}

	os_mutex_unlock(&fat_cache.lock);
	return ret;
}

#ifdef FAT_TIME
static void fat_loop(void *arg)
#else
static int32 fat_loop(struct os_work *work)
#endif
{
	if (!fat_cache_is_ready()) {
		goto fat_loop_end;
	}	

	uint8 ret = 0;
	struct sdh_device *sdh = NULL;
	sdh = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);
	if (sdh == NULL) {
		goto fat_loop_end;
	}

	ret = os_mutex_lock(&sdh->lock, 0);
	if (ret != RET_OK)
	{
		fat_cache.fat_tick = os_jiffies();
		goto fat_loop_end; // 获取锁失败
	}
	os_mutex_unlock(&sdh->lock);
	ret = os_mutex_lock(&fat_cache.lock, 0);
	if (ret != RET_OK)
	{
		goto fat_loop_end; // 获取锁失败
	}

	// 检测到200ms没有操作SD卡，且SD卡在线，fat信息回写SD
	if (os_jiffies() - fat_cache.fat_tick > 200 && SD_OFF != sdh->sd_opt)
	{
		fat_cache.fat_tick = os_jiffies();
		fat_cache_sync(sdh, &fat_cache.fat1);
		bitmap_cache_sync(sdh, &fat_cache.bitmap);
	}
	
	os_mutex_unlock(&fat_cache.lock);
fat_loop_end:
	#ifdef FAT_TIME
	return;
	#else
    os_run_work_delay(work, 50);
	return 0;
	#endif
}

signed char fat_cache_mount(FATFS *fs)
{
	if (fs == NULL){
		return RET_ERR;
	}

	if (fat_cache.fat_init == RET_OK){
		return RET_OK;
	}

	fat_cache.fat1.data = fat_malloc(FAT_CACHE_SIZE * FATFS_SECTOR_SIZE);
	if (fat_cache.fat1.data == NULL){
		return RET_ERR;
	}
	if (fs->fs_type == FS_EXFAT) {
		fat_cache.bitmap.data = fat_malloc(BITMAP_CACHE_SIZE * FATFS_SECTOR_SIZE);
	}
	if (fs->fs_type == FS_EXFAT && fat_cache.bitmap.data == NULL) {
		fat_free(fat_cache.fat1.data);
		fat_cache.fat1.data = NULL;
		return RET_ERR;
	}

	if (os_mutex_init(&fat_cache.lock) != RET_OK){
		goto fat_cache_mount_free;
	}

	#ifdef FAT_TIME
	if (os_timer_init(&fat_cache.fat_timer, fat_loop, OS_TIMER_MODE_PERIODIC, 0) != RET_OK){
		goto fat_cache_mount_del_mutex;
	}
	#else
	if (OS_WORK_INIT(&fat_cache.fat_wk, fat_loop, 0) != RET_OK){
		goto fat_cache_mount_del_mutex;
	}
	#endif

	fat_cache.fat_info_ready = 1;
	reset_cache_window(&fat_cache.fat1);
	reset_cache_window(&fat_cache.bitmap);
	fat_cache.fat_tick = os_jiffies();
	fat_cache.fat_init = RET_OK;
	init_cache_for_volume(fs);

	#ifdef FAT_TIME
	os_timer_start(&fat_cache.fat_timer, 50);
	#else
	os_run_work_delay(&fat_cache.fat_wk, 50);
	#endif

	return RET_OK;

fat_cache_mount_del_mutex:
	os_mutex_del(&fat_cache.lock);
fat_cache_mount_free:
	free_cache_buffers();
	return RET_ERR;
}

static void del_fat_cache(void)
{
	if (fat_cache.fat_init != RET_OK){
		return;
	}

	fat_cache.fat_init = 1;
	fat_cache.fat_info_ready = 1;
	
	#ifdef FAT_TIME
	os_timer_stop(&fat_cache.fat_timer);
	os_timer_del(&fat_cache.fat_timer);// 先卸载定时器
	#else
	os_work_cancle2(&fat_cache.fat_wk,1);
	#endif
	os_mutex_lock(&fat_cache.lock, osWaitForever);
	
	free_cache_buffers();
	fat_cache.fs_type = 0;
	fat_cache.fs_fats = 0;
	fat_cache.fs_size = 0;
	fat_cache.bitmap_start = 0;
	fat_cache.bitmap_size = 0;
	fat_cache.fat_start = 0;
	reset_cache_window(&fat_cache.fat1);
	reset_cache_window(&fat_cache.bitmap);
	os_mutex_unlock(&fat_cache.lock);
	os_mutex_del(&fat_cache.lock); 
}

void fat_cache_unmount(void)
{
	struct sdh_device *sdh = NULL;

	if (fat_cache.fat_init != RET_OK){
		return;
	}

	sdh = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);
	if (sdh != NULL && sdh->sd_opt != SD_OFF){
		full_cache_sync(sdh);
	}

	del_fat_cache();
}

#endif	/* USE_FAT_CACHE */

DRESULT fatfs_read(void *dev, BYTE *buf, DWORD sector, UINT count)
{
#if USE_FAT_CACHE
	update_io_timestamp();
	if (cache_range_contains(fat_cache.bitmap_start, fat_cache.bitmap_size, sector, count)) {
		return read_from_bitmap_cache(dev, buf, sector, count);
	}
	if (cache_range_contains(fat_cache.fat_start, fat_cache.fs_size, sector, count)) {
		return read_from_fat_cache(dev, buf, sector, count);
	}
#endif
	return sd_multiple_read((struct sdh_device *)dev, sector, count * FATFS_SECTOR_SIZE, buf);
}

static DRESULT fatfs_write(void *dev, BYTE *buf, DWORD sector, UINT count)
{
#if USE_FAT_CACHE
	update_io_timestamp();
	if (cache_range_contains(fat_cache.bitmap_start, fat_cache.bitmap_size, sector, count)) {
		return write_to_bitmap_cache(dev, buf, sector, count);
	}
	if (cache_range_contains(fat_cache.fat_start, fat_cache.fs_size, sector, count)) {
		return write_to_fat_cache(dev, buf, sector, count);
	}

	if (fat_cache_is_ready() && count <= FAT_CACHE_SIZE && fat_cache.fs_fats > 1 &&
		cache_range_contains(fat_cache.fat_start + fat_cache.fs_size, fat_cache.fs_size, sector, count))
	{
		return RES_OK;
	}
#endif
	return sd_multiple_write((struct sdh_device *)dev, sector, count * FATFS_SECTOR_SIZE, buf);
}

extern unsigned int sd_dwCap;
extern uint32 fatfs_sd_tran_stop(struct sdh_device *host);
static DRESULT fatfs_ioctl(void *init_dev, BYTE cmd, void *buf)
{
	uint8 ret = RES_OK;
	switch (cmd)
	{
	case CTRL_SYNC:
		// fatfs_sd_tran_stop(init_dev);

#if USE_FAT_CACHE
		full_cache_sync(init_dev);
#endif
		break;
	case GET_SECTOR_COUNT:
		*(LBA_t *)buf = sd_dwCap * 2;
		ret = RES_OK;
		break;

	case GET_SECTOR_SIZE:
		*(WORD *)buf = FATFS_SECTOR_SIZE;
		ret = RES_OK;

		break;
	case GET_BLOCK_SIZE:
		*(DWORD *)buf = 4;
		// printf("*0B:%d\n",*B);
		ret = RES_OK;
		break;

	default:
		ret = RES_ERROR; // not finish
		printf("rtos_sd_ioctl err\n");
		break;
	}
	return ret;
}

extern void sys_mount_device(uint16 dev_id, uint16 dev_type, uint8 umount);
void fatfs_sd0_init(void)
{
	uint32_t res;
	struct sdh_device *fatfs_sdh = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);
    fatfs_register_drive(0, (struct fatfs_diskio*)&sdcdisk_driver, fatfs_sdh);
    res = sdhost_init(48 * 1000 * 1000, 0);
    // 初始化发现有卡,尝试挂载sd卡
    if (res == RET_OK)
    {
		dev_hotplug_in(HG_SD0_DEVID, DEV_TYPE_SD, 0);
		/* mount now for other app */
        sys_mount_device(HG_SD0_DEVID, DEV_TYPE_SD, 0);
    }
}
