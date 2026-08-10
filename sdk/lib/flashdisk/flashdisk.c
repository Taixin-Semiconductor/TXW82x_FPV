#include "flashdisk.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "hal/spi_nor.h"
#include "lib/syscfg/syscfg.h"
#include "diskio.h"
#include "ff.h"
#if FS_EN
#include "fs/vfs/vfs.h"
#endif

struct flash_disk_device
{
    struct spi_nor_flash *flash;
    uint32_t              addr;
    uint32_t              size;
};
static struct flash_disk_device *g_flash_disk;

static DSTATUS flashdisk_status(void *init_dev)
{
    return RES_OK;
}

static DSTATUS flashdisk_init(void *init_dev)
{
    struct flash_disk_device *disk  = (struct flash_disk_device *) init_dev;
    struct spi_nor_flash     *flash = disk->flash;
    if (!disk->flash)
    {
        return RES_NOTRDY;
    }

    spi_nor_open(flash);
    disk->flash = flash;
    os_printf("disk->addr = %x, disk->size = %d\r\n", disk->addr, disk->size);
    return RES_OK;
}

static DRESULT flashdisk_read(void *init_dev, BYTE *buf, DWORD sector, UINT count)
{
    struct flash_disk_device *disk  = (struct flash_disk_device *) init_dev;
    struct spi_nor_flash     *flash = disk->flash;
    spi_nor_read(flash, disk->addr + sector * flash->sector_size, buf, count * flash->sector_size);
    return RES_OK;
}
static DRESULT flashdisk_write(void *init_dev, BYTE *buf, DWORD sector, UINT count)
{
    struct flash_disk_device *disk  = (struct flash_disk_device *) init_dev;
    struct spi_nor_flash     *flash = disk->flash;
    for (int i = 0; i < count; i++)
    {
        spi_nor_sector_erase(flash, disk->addr + sector * flash->sector_size + i * flash->sector_size);
    }
    spi_nor_write(flash, disk->addr + sector * flash->sector_size, buf, count * flash->sector_size);
    return RES_OK;
}

static DRESULT flashdisk_ioctl(void *init_dev, BYTE cmd, void *buf)
{
    struct flash_disk_device *disk  = (struct flash_disk_device *) init_dev;
    struct spi_nor_flash     *flash = disk->flash;

    uint8 ret = RES_OK;
    switch (cmd)
    {
        case CTRL_SYNC:
            break;
        case GET_SECTOR_COUNT:
            *(QWORD *) buf = disk->size / flash->sector_size;
            ret            = RES_OK;
            break;

        case GET_SECTOR_SIZE:
            *(WORD *) buf = flash->sector_size;
            ret           = RES_OK;

            break;
        case GET_BLOCK_SIZE:
            *(DWORD *) buf = 1;

            ret = RES_OK;
            break;

        default:
            ret = RES_ERROR;
            printf("rtos_sd_ioctl err\n");
            break;
    }

    return ret;
}

// 固定返回值
uint8_t flash_udisk_get_status(void *dev)
{
    return 0;
}
// usb作为u盘的接口
int flash_udisk_read(void *dev, uint32_t lba, uint32_t block_count, uint8 *buf)
{
    if (!(g_flash_disk && g_flash_disk->flash))
    {
        return 1;
    }

    flashdisk_read(g_flash_disk, buf, lba, block_count);
    return 0;
}

int flash_udisk_write(void *dev, uint32_t lba, uint32_t block_count, uint8 *buf)
{

    if (!(g_flash_disk && g_flash_disk->flash))
    {
        return 1;
    }

    flashdisk_write(g_flash_disk, buf, lba, block_count);
    return 0;
}

struct spi_nor_flash *flashdisk_usb_getdev()
{
    if (!(g_flash_disk && g_flash_disk->flash))
    {
        return NULL;
    }
    return g_flash_disk->flash;
}

// 获取flash文件系统的扇区数量
uint32_t flash_udisk_get_capacity(void *dev)
{
    uint32_t count = 0;
    flashdisk_ioctl(g_flash_disk, GET_SECTOR_COUNT, &count);
    return count;
}


uint32_t flash_udisk_get_sector_size(void *dev)
{
    uint32_t sec_size = 0;
    flashdisk_ioctl(g_flash_disk, GET_SECTOR_SIZE, &sec_size);
    return sec_size;
}

static struct fatfs_diskio flashdisk_driver = {
        .status = flashdisk_status,
        .init   = flashdisk_init,
        .read   = flashdisk_read,
        .write  = flashdisk_write,
        .ioctl  = flashdisk_ioctl,
};

/***********************************************************
 * @brief 初始化flash文件系统
 * @param abs_addr 文件系统起始地址(flash绝对地址),没有设置则使用默认值
 * @param size 文件系统大小,没有配置使用默认值FLASH_FATFS_SIZE
 * @return void
 ************************************************************* */
void flash_fatfs_init(uint32_t abs_addr, uint32_t size)
{
    char               target[8];
    char               source[8];
    struct syscfg_info info;
    if (g_flash_disk)
    {
        os_printf("g_flash_disk is already init\n");
        return;
    }
    os_memset(&info, 0, sizeof(info));
    if (syscfg_info_get(&info, "syscfg"))
    {
        os_printf("%s fail\n", __FUNCTION__);
        return;
    }
    uint32_t default_addr;
    uint32_t default_size;
    g_flash_disk        = os_zalloc(sizeof(struct flash_disk_device));
    g_flash_disk->flash = info.flash1;
    default_size        = FLASH_FATFS_SIZE;
    g_flash_disk->size  = size ? size : default_size;
    default_addr        = g_flash_disk->flash->size - (g_flash_disk->size + info.flash1->sector_size * 6);
    g_flash_disk->addr = abs_addr ? abs_addr : default_addr;

    os_printf("flash fs abs_addr = %x, flash size = %d\r\n", g_flash_disk->addr, g_flash_disk->size);

    fatfs_register_drive(DEV_FLASH, &flashdisk_driver, g_flash_disk);
    os_sprintf(target, "/flash%d", 0);
    os_sprintf(source, "%d:", DEV_FLASH);
    #if FS_EN
    int32 ret = vfs_mount((const char *) source, (const char *) target, "fatfs", 0, NULL);
    // 第一次挂载失败,则代表flash原本没有文件系统,则需要格式化flash
    if (ret)
    {
        os_printf("first no found flash fs:%d\n", ret);
        vfs_mkfs(source, "fatfs", NULL);
        ret = vfs_mount((const char *) source, (const char *) target, "fatfs", 0, NULL);
        // 第二次格式化失败,可能格式化参数不对或者flash写入有问题
        if (ret)
        {
            os_printf("flash fs mkfs fail:%d\n", ret);
        }
    }
    else
    {
        os_printf("flash fs mount success\n");
    }
    #else
    os_printf("no enable fs\n");
    #endif
}