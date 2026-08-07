
#include "flashdisk.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "hal/spi_nor.h"
#include "lib/syscfg/syscfg.h"

#include "diskio.h"
#include "ff.h"



struct flash_disk_device
{
    struct spi_nor_flash *flash;
    uint32_t addr;
    uint32_t size;
};
struct spi_nor_flash *flash_disk_flash = NULL;
static struct flash_disk_device flash_disk;

static DSTATUS flashdisk_status(void *init_dev)
{
	return RES_OK;
}


static DSTATUS flashdisk_init(void *init_dev)
{
    struct flash_disk_device *disk = (struct flash_disk_device*)init_dev;
    struct syscfg_info info;
    struct spi_nor_flash *flash;
    if(!flash_disk_flash)
    {
        os_memset(&info, 0, sizeof(info));
        if (syscfg_info_get(&info, "syscfg")) {
            return RES_NOTRDY;
        }
        flash = info.flash1;
    }
    else
    {
        flash = flash_disk_flash;
    }
    
    spi_nor_open(flash);
    disk->flash = flash;
    disk->addr = flash->size - (FLASH_FATFS_SIZE + flash->sector_size * 2);
    disk->size = FLASH_FATFS_SIZE;
	return RES_OK;
}

static DRESULT flashdisk_read(void *init_dev,BYTE* buf, DWORD sector, UINT count)
{
    struct flash_disk_device *disk = (struct flash_disk_device*)init_dev;
    struct spi_nor_flash *flash = disk->flash;
    spi_nor_read(flash, disk->addr + sector*flash->sector_size, buf , count*flash->sector_size);
    return RES_OK;
}
static DRESULT flashdisk_write(void *init_dev,BYTE* buf, DWORD sector, UINT count)
{
    struct flash_disk_device *disk = (struct flash_disk_device*)init_dev;
    struct spi_nor_flash *flash = disk->flash;
    for (int i = 0; i < count; i++) {
        spi_nor_sector_erase(flash, disk->addr  + sector*flash->sector_size + i * flash->sector_size);
    }
    spi_nor_write(flash, disk->addr + sector*flash->sector_size, buf,count*flash->sector_size);
    return RES_OK;
}


static DRESULT flashdisk_ioctl(void *init_dev, BYTE cmd, void* buf)
{
    struct flash_disk_device *disk = (struct flash_disk_device*)init_dev;
    struct spi_nor_flash *flash = disk->flash;

	uint8 ret	= RES_OK;
	switch(cmd)
    {
		case CTRL_SYNC:
			break;
		case GET_SECTOR_COUNT:
			*(DWORD *)buf = disk->size/flash->sector_size;	
			ret = RES_OK;
			break;

		case GET_SECTOR_SIZE:
			*(WORD *)buf = flash->sector_size;
			ret = RES_OK;
			
			break;
		case GET_BLOCK_SIZE:
			*(DWORD *)buf = 1;

			ret = RES_OK;
			break;

				
		default:
			ret = RES_ERROR;
			printf("rtos_sd_ioctl err\n");
			break;
    }

    
    return ret;
}

//固定返回值
uint8_t flashdisk_get_status()
{
    return 0;
}
//usb作为u盘的接口
int flashdisk_usb_read(uint32_t sector,uint32_t count,uint8* buf)
{
    if(!flash_disk.flash)
    {
        return 1;
    }
    
    flashdisk_read(&flash_disk,buf,sector,count);
    return 0;
}

int flashdisk_usb_write(uint32_t sector,uint32_t count,uint8* buf)
{

    if(!flash_disk.flash)
    {
        return 1;
    }
    
    flashdisk_write(&flash_disk,buf,sector,count);
    return 0;
}

struct spi_nor_flash *flashdisk_usb_getdev()
{
    if(!flash_disk.flash)
    {
        return 0;
    }    

    return flash_disk.flash;
}


//获取flash文件系统的扇区数量
uint32_t flashdisk_get_sec_count()
{
    uint32_t count = 0;
    flashdisk_ioctl(&flash_disk,GET_SECTOR_COUNT,&count);
    return count;
}


uint32_t flashdisk_get_sec_size()
{
    uint32_t sec_size = 0;
    flashdisk_ioctl(&flash_disk,GET_SECTOR_SIZE,&sec_size);
    return sec_size;
}


static struct fatfs_diskio flashdisk_driver = 
{
  .status = flashdisk_status,
  .init = flashdisk_init,
  .read = flashdisk_read,
  .write = flashdisk_write,
  .ioctl = flashdisk_ioctl,
}; 


FATFS *flash_fs = NULL;
void flash_fatfs_init()
{
    BYTE *work;
    int res = 0;
    fatfs_register_drive(DEV_FLASH,&flashdisk_driver,&flash_disk);
    if(!flash_fs)
    {
        flash_fs = (FATFS *)os_malloc(sizeof(FATFS));
    }
    
    if(flash_fs)
    {
        res = f_mount(flash_fs,"FLASH:",1);
        os_printf("%s:%d res:%d\n",__FUNCTION__,__LINE__,res);
        //如果flash没有存在文件系统,则重新格式化
        if(res)
        {
            work = os_malloc(4096);
            //work默认申请成功,就不对之前申请的内存进行释放
            if(!work)
            {
                os_printf("%s mkfs malloc space err:%d\n",__FUNCTION__,res);
                return;
            }
            res = f_mkfs("FLASH:", FM_ANY | FM_SFD, 4096,work, 4096);
            os_free(work);
            if(res)
            {
                os_printf("%s mkfs fatfs err:%d\n",__FUNCTION__,res);
                return;
            }
            else
            {
                //重新挂载文件系统
                res = f_mount(flash_fs,"FLASH:",1);
                if(res)
                {
                    os_printf("%s mount fatfs err:%d\n",__FUNCTION__,res);
                }
            }
        }
    }
    return;
    
}
