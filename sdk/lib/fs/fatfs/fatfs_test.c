#include "sys_config.h"
#include "integer.h"
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

#define FAT_INFO_SHOW(...) //printf(__VA_ARGS__)

#if FS_EN
static uint8_t fat_ready = 0;

uint8_t get_fat_isready()
{
	return fat_ready;
}

void set_fat_ready(uint8_t ready)
{
	fat_ready = ready;
}

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
	.ioctl = fatfs_ioctl};

static FATFS fatfs[1];

DWORD get_fatbase(int num)
{
	return fatfs[num].fatbase;
}

DWORD get_fatfree(int num)
{
	DWORD fre_clust, fre_sect, tot_sect;
	FATFS *fs = &fatfs[num];
	fre_clust = fs->free_clst;
	tot_sect = (fs->n_fatent - 2) * fs->csize;
	fre_sect = fre_clust * fs->csize;
	printf("%s %ldKB\n", __FUNCTION__, fre_sect >> 1);
	return fre_sect >> 1;
}

static DSTATUS fatfs_status(void *status)
{
	// FAT_INFO_SHOW ("fatfs_status_test\r\n");
	uint32 err = get_sdhost_status(status);
	return err;
}

static DSTATUS fatfs_init(void *init_dev){
	printf ("fatfs_init_test\r\n");
	uint32 err = get_sdhost_status(init_dev);
	if(err)
	{
		err = sdhost_init(48 * 1000 * 1000, SDHC_SINGLE_CODE_SUPPORT_DIS);
	}
	
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
struct fat_data_t
{
	// uint8 data[FAT_CACHE_SIZE * 512]; // 32KB 缓存
	BYTE *data;		// 32KB 缓存
	DWORD start_sector; // 缓存起始扇区
	DWORD fat_start;	// FAT起始扇区
	DWORD fat_end;
	DWORD offset;
	DWORD max_offset;
};

struct fat_cache_t
{
	BYTE fat_info_ready; //
	BYTE fat_init;
	BYTE fs_type;
	BYTE fs_fats;
	DWORD fs_size;
	DWORD fat_tick;
	os_timer_t fat_timer;
	struct os_mutex lock;
	struct fat_data_t fat1;
};



struct fat_cache_t fat_cache = {
	// lock和time初始化标志位，1是未初始化，0是已经初始化
	.fat_init = 1,
	.fat_info_ready = 1,
};

signed char update_fat_info(BYTE fmt, BYTE n_fats, DWORD sz_fat,DWORD fatbase, DWORD b_vol)
{
	if (fat_cache.fat_init != RET_OK){
		return RET_ERR;
	}	

	os_mutex_lock(&fat_cache.lock, osWaitForever);
	
	fat_cache.fs_type = fmt;
	fat_cache.fs_fats = n_fats;
	fat_cache.fs_size = sz_fat;

	fat_cache.fat1.fat_start = fatbase;
	fat_cache.fat1.fat_end = fat_cache.fat1.fat_start + fat_cache.fs_size - 1;

	fat_cache.fat_info_ready = RET_OK;

	// 计算逻辑地址（扇区号）
	UINT fat1_logical = fatbase - b_vol;                    // FAT1 logical start
	UINT fat2_logical = fat1_logical + sz_fat;     // FAT2 logical start

	// 计算物理地址（加上分区偏移）
	UINT partition_start = b_vol;                  // 分区起始扇区
	UINT fat1_physical = partition_start + fat1_logical;
	UINT fat2_physical = partition_start + fat2_logical;

	// if (fat_cache.fs_type == FS_EXFAT) // FS_EXFAT文件系统不需要优化
	// {
	// 	fat_cache.fat_info_ready = 0;
	// }

	if (fmt == FS_FAT12)
		FAT_INFO_SHOW("Filesystem Type: FS_FAT12 \r\n");
	else if (fmt == FS_FAT16)
		FAT_INFO_SHOW("Filesystem Type: FS_FAT16 \r\n");
	else if (fmt == FS_FAT32)
		FAT_INFO_SHOW("Filesystem Type: FS_FAT32 \r\n");
	else if (fmt == FS_EXFAT)
		FAT_INFO_SHOW("Filesystem Type: FS_EXFAT \r\n");

	FAT_INFO_SHOW("Filesystem fat_num %u \r\n", n_fats);
	FAT_INFO_SHOW("Filesystem fat_size %u \r\n", sz_fat);

	FAT_INFO_SHOW("Physical Address ===> fat1_start %u , fat1_end %u \r\n", fat_cache.fat1.fat_start, fat_cache.fat1.fat_end);
	FAT_INFO_SHOW("Logical Address ====> fat1_start %u , fat1_end %u \r\n", fat1_logical, fat1_logical + sz_fat - 1);

	if (n_fats > 1) {
		FAT_INFO_SHOW("Physical Address ===> fat2_start %u , fat2_end %u \r\n",  fat2_physical, fat2_physical + sz_fat - 1);
		FAT_INFO_SHOW("Logical Address ====> fat2_start %u , fat2_end %u \r\n", fat2_logical, fat2_logical + sz_fat - 1);
	}	

	os_mutex_unlock(&fat_cache.lock);

	return RET_OK;
}

void update_io_timestamp()
{
	if (fat_cache.fat_init != RET_OK || fat_cache.fat_info_ready != RET_OK){
		return;
	}	
	os_mutex_lock(&fat_cache.lock, osWaitForever);
	fat_cache.fat_tick = os_jiffies();
	os_mutex_unlock(&fat_cache.lock);
}

// fat回写SD
static void fat_cache_sync(struct sdh_device *host)
{
	struct sdh_device *sdh = NULL;
	sdh = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);
	if (fat_cache.fat_init != RET_OK || fat_cache.fat_info_ready != RET_OK){
		return;
	}
	os_mutex_lock(&fat_cache.lock, osWaitForever);

	FAT_INFO_SHOW("############# CTRL_SYNC max_offset %d\r\n", fat_cache.fat1.max_offset);
	if (fat_cache.fat1.max_offset > 0)
	{
		sd_multiple_write((struct sdh_device *)host, fat_cache.fat1.start_sector, fat_cache.fat1.max_offset * 512, fat_cache.fat1.data);
		if (fat_cache.fs_fats > 1)
		{
			sd_multiple_write((struct sdh_device *)host, (fat_cache.fat1.start_sector + fat_cache.fs_size), fat_cache.fat1.max_offset * 512, fat_cache.fat1.data);
		}
		fat_cache.fat1.max_offset = 0;
	}
	
	os_mutex_unlock(&fat_cache.lock);
}

static void fat_loop(void *arg)
{
	
	if (fat_cache.fat_init != RET_OK || fat_cache.fat_info_ready != RET_OK){
		goto fat_loop_end;
	}	

	uint8 ret = 0;
	struct sdh_device *sdh = NULL;
	sdh = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);

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

	// 检测到200ms没有操作SD卡，并SD卡在线，fat信息回写SD
	if (os_jiffies() - fat_cache.fat_tick > 200 && SD_OFF != sdh->sd_opt)
	{
		fat_cache.fat_tick = os_jiffies();
		
		if (fat_cache.fat1.max_offset > 0)
		{
			FAT_INFO_SHOW(" fat_loop write back max_offset %d\r\n", fat_cache.fat1.max_offset);
			sd_multiple_write(sdh, fat_cache.fat1.start_sector, fat_cache.fat1.max_offset * 512, fat_cache.fat1.data);
			if (fat_cache.fs_fats > 1) // 写入FAT2
			{
				sd_multiple_write(sdh, (fat_cache.fat1.start_sector + fat_cache.fs_size), fat_cache.fat1.max_offset * 512, fat_cache.fat1.data);
			}
			fat_cache.fat1.max_offset = 0;
		}
	}
	
	os_mutex_unlock(&fat_cache.lock);
fat_loop_end:
	return;
	
}

static void init_fat_cache(FATFS *fs)
{
	if (update_fat_info(fs->fs_type, fs->n_fats, fs->fsize,fs->fatbase, fs->volbase) != RET_OK){
		return;
	}
	FAT_INFO_SHOW("init_fat_cache \r\n");
	struct sdh_device *sdh = NULL;
	sdh = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);
	// 初始化后第一次读fat1
	fat_cache.fat1.start_sector = fs->fatbase;
	sd_multiple_read(sdh, fat_cache.fat1.start_sector, FAT_CACHE_SIZE * 512, fat_cache.fat1.data);
}

static void del_fat_cache(void)
{
	if (fat_cache.fat_init != RET_OK){
		return;
	}

	fat_cache.fat_init = 1;
	fat_cache.fat_info_ready = 1;
	os_mutex_lock(&fat_cache.lock, osWaitForever);
	FAT_INFO_SHOW("########### del_fat_cache \r\n");
	
	os_timer_stop(&fat_cache.fat_timer);
	os_timer_del(&fat_cache.fat_timer);// 先卸载定时器
	
	// 释放fat缓存
	if (fat_cache.fat1.data)
	{
		FAT_INFO_SHOW("%s %d fat free \r\n", __func__, __LINE__);
		fat_free(fat_cache.fat1.data);
		fat_cache.fat1.data = NULL;
	}
	os_mutex_unlock(&fat_cache.lock);
	os_mutex_del(&fat_cache.lock); 
}

static DRESULT read_from_fat_cache(void *dev, struct fat_data_t *cache, BYTE *buf, DWORD sector, UINT count)
{
	int ret = 0;
	if (fat_cache.fat_init != RET_OK || fat_cache.fat_info_ready != RET_OK){
		return sd_multiple_read((struct sdh_device *)dev, sector, count * 512, buf);
	}
	os_mutex_lock(&fat_cache.lock, osWaitForever);

	if (sector >= cache->start_sector && sector + count <= cache->start_sector + FAT_CACHE_SIZE)
	{
		// 从缓存读取
		cache->offset = (sector - cache->start_sector);
		memcpy(buf, &cache->data[cache->offset * 512], count * 512);
	}
	// 未命中缓存，把旧缓存写入SD，重新预读 16KB 到缓存
	else
	{
		// 把旧缓存写入fat
		if (cache->max_offset > 0)
		{
			FAT_INFO_SHOW("read_from_fat_cache write back max_offset %d sector %d\r\n", cache->max_offset, sector);
			sd_multiple_write((struct sdh_device *)dev, cache->start_sector, cache->max_offset * 512, cache->data);
			if (fat_cache.fs_fats > 1)
			{
				sd_multiple_write((struct sdh_device *)dev, (cache->start_sector + fat_cache.fs_size), cache->max_offset * 512, cache->data);
			}
			cache->max_offset = 0;
			// memset(cache->data, 0, FAT_CACHE_SIZE * 512);
		}
		// 重新预读数据到缓存
		ret = sd_multiple_read((struct sdh_device *)dev, sector, FAT_CACHE_SIZE * 512, cache->data);
		cache->start_sector = sector;
		memcpy(buf, &cache->data[0], count * 512);
	}
	// __end:
	os_mutex_unlock(&fat_cache.lock);
	return ret;
}

static DRESULT write_to_fat_cache(void *dev, struct fat_data_t *cache, BYTE *buf, DWORD sector, UINT count)
{
	int ret = 0;
	
	if (fat_cache.fat_init != RET_OK || fat_cache.fat_info_ready != RET_OK){
		return sd_multiple_write((struct sdh_device *)dev, sector, count * 512, buf);
	}
	os_mutex_lock(&fat_cache.lock, osWaitForever);

	// 检查是否命中缓存
	if (sector >= cache->start_sector && sector + count <= cache->start_sector + FAT_CACHE_SIZE)
	{
		cache->offset = (sector - cache->start_sector);
		memcpy(&cache->data[cache->offset * 512], buf, count * 512);
		if (cache->max_offset < (cache->offset + 1))
		{
			cache->max_offset = cache->offset + 1;
		}
	}
	else
	{
		// 把旧缓存写入fat
		if (cache->max_offset > 0)
		{
			FAT_INFO_SHOW("write_to_fat_cache write back max_offset %d sector %d\r\n", cache->max_offset, sector);
			sd_multiple_write((struct sdh_device *)dev, cache->start_sector, cache->max_offset * 512, cache->data);
			if (fat_cache.fs_fats > 1)
			{
				sd_multiple_write((struct sdh_device *)dev, (cache->start_sector + fat_cache.fs_size), cache->max_offset * 512, cache->data);
			}
			cache->max_offset = 0;
			// memset(cache->data, 0, FAT_CACHE_SIZE * 512);
		}
		// 重新预读数据到缓存
		ret = sd_multiple_read((struct sdh_device *)dev, sector, FAT_CACHE_SIZE * 512, cache->data);
		cache->start_sector = sector;

		cache->offset = 0;
		memcpy(&cache->data[cache->offset * 512], buf, count * 512);

		if (cache->max_offset < (cache->offset + 1))
		{
			cache->max_offset = cache->offset + 1;
		}
	}
	// __end:
	os_mutex_unlock(&fat_cache.lock);
	return ret;
}

#endif

DRESULT fatfs_read(void *dev, BYTE *buf, DWORD sector, UINT count)
{
#if USE_FAT_CACHE
	update_io_timestamp();
	if (sector >= fat_cache.fat1.fat_start && sector <= fat_cache.fat1.fat_end)
	{
		return read_from_fat_cache((struct sdh_device *)dev, &fat_cache.fat1, buf, sector, count);
	}
#endif
    return sd_multiple_read((struct sdh_device *)dev, sector, count * 512, buf);
}

static DRESULT fatfs_write(void *dev, BYTE *buf, DWORD sector, UINT count)
{
#if USE_FAT_CACHE
	update_io_timestamp();
	if (sector >= fat_cache.fat1.fat_start && sector <= fat_cache.fat1.fat_end)
	{
		return write_to_fat_cache((struct sdh_device *)dev, &fat_cache.fat1, buf, sector, count);
	}
#endif
	return sd_multiple_write((struct sdh_device *)dev, sector, count * 512, buf);
}

extern unsigned int sd_dwCap;
extern uint32 fatfs_sd_tran_stop(struct sdh_device *host);
static DRESULT fatfs_ioctl(void *init_dev, BYTE cmd, void *buf)
{
	uint8 ret = RES_OK;
	switch (cmd)
	{
	case CTRL_SYNC:
		fatfs_sd_tran_stop(init_dev);

#if USE_FAT_CACHE
		fat_cache_sync(init_dev);
#endif
		break;
	case GET_SECTOR_COUNT:
		*(DWORD *)buf = sd_dwCap * 2;
		ret = RES_OK;
		break;

	case GET_SECTOR_SIZE:
		*(WORD *)buf = 512;
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

bool fatfs_register()
{

	int ret = 1;
	struct sdh_device *fatfs_sdh;
	// printf(">>>>>>>>>> enter %s test\r\n", __func__);
	fatfs_sdh = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);

#if USE_FAT_CACHE
	if (fat_cache.fat_init)
	{
		// 分配 fat1 缓存
		fat_cache.fat1.data = fat_malloc(FAT_CACHE_SIZE * 512);
		if( fat_cache.fat1.data != NULL &&
			os_mutex_init(&fat_cache.lock) == RET_OK && 
			os_timer_init(&fat_cache.fat_timer, fat_loop, OS_TIMER_MODE_PERIODIC, 0) == RET_OK
			)
		{
			fat_cache.fat_init = RET_OK;
			FAT_INFO_SHOW("fat_init success\r\n");
			os_timer_start(&fat_cache.fat_timer, 50);
		}
		else
		{
			os_printf("fat init err \r\n");
		}

	}
#endif

	if (fatfs_sdh)
	{
		fatfs_register_drive(0, (struct fatfs_diskio*)&sdcdisk_driver, fatfs_sdh);
		ret = f_mount(&fatfs[0], _SYSDSK_, 1);
		if (ret)
		{
			printf("%s ret:%d\n", __FUNCTION__, ret);
			f_mount(NULL, _SYSDSK_, 0);
			return ret;
		}
		FAT_INFO_SHOW("f_mount success\r\n");
		set_fat_ready(1);
#if USE_FAT_CACHE
		init_fat_cache(&fatfs[0]);
#endif
	}
	return ret;
}

void fatfs_unregister()
{
	int ret = 1;
	FAT_INFO_SHOW(">>>>>>>>>>enter %s test\r\n", __func__);
	struct sdh_device *fatfs_sdh;
	fatfs_sdh = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);
	if (fatfs_sdh)
	{
		fatfs_register_drive(0, (struct fatfs_diskio*)&sdcdisk_driver, fatfs_sdh);
		ret = f_mount(NULL, _SYSDSK_, 0);
		if (ret)
		{
			printf("%s ret:%d\n", __FUNCTION__, ret);
			return;
		}
		set_fat_ready(0);
#if USE_FAT_CACHE
		del_fat_cache();
#endif
	}
}

#endif
