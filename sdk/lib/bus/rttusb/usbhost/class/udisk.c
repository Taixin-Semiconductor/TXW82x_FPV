/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-12-12     Yi Qiu      first version
 */

#include <rtthread.h>
#include <include/rttusb_host.h>

#ifdef RT_USBH_MSTORAGE
#include "mass.h"

#include "dev.h"
#include "devid.h"
#include "diskio.h"
#include "ff.h"

#include "osal_file.h"
#include "lib/ota/fw.h"
#include "tx_platform.h"
#include "dev/csi/hgdvp.h"

#define UDISK_MAX_COUNT        8
#define UDISK_CACHE_SIZE       (512)

static rt_uint8_t _udisk_idset = 0;
static rt_uint8_t udisk_ota = 0;

struct udisk_device
{
    rt_uint32_t count;
    rt_uint32_t sector_size;
    struct ustor_data* user_data;
    struct uhintf* intf;
};

FATFS *udisk_fs = NULL;
static struct udisk_device usb_disk;

static int udisk_get_id(void)
{
    int i;

    for(i=0; i< UDISK_MAX_COUNT; i++)
    {
        if((_udisk_idset & (1 << i)) != 0) continue;
        else break;
    }

    /* it should not happen */
    if(i == UDISK_MAX_COUNT) RT_ASSERT(0);

    _udisk_idset |= (1 << i);
    return i;
}

static void udisk_free_id(int id)
{
    RT_ASSERT(id < UDISK_MAX_COUNT);

    _udisk_idset &= ~(1 << id);
}

static DSTATUS rt_udisk_status(void *dev)
{
    struct udisk_device *disk = (struct udisk_device *)dev;
    if((disk==NULL) || (disk->intf == NULL))
    {
        rt_kprintf("%s disk is null!!!\n",__FUNCTION__);
        return RES_ERROR;
    }

    return RES_OK;
}

static DSTATUS rt_udisk_init(void *dev)
{
    printf("%s %d\n",__FUNCTION__,__LINE__); 

    struct udisk_device *disk = (struct udisk_device *)dev;
    if(!disk)
    {
        os_printf("disk is null!!!\n");
        return RES_ERROR;
    }


    return RES_OK;
}

static DRESULT rt_udisk_read(void *dev, BYTE* buffer, DWORD sector,
    UINT count)
{  
    rt_err_t ret;
    struct uhintf* intf;
    struct ustor_data* data;
    int timeout = USB_TIMEOUT_LONG/5;
    struct udisk_device *disk = (struct udisk_device *)dev;

    /* check parameter */
    if((disk==NULL) ||(buffer==NULL) || (disk->intf == NULL))
    {
        rt_kprintf("%s disk is null!!!\n",__FUNCTION__);
        return RES_ERROR;
    }

    if(count > 4096) timeout *= 2;

    data = (struct ustor_data*)disk->user_data;
    intf = disk->intf;

    os_printf("%s sector:%d count:%d\n",__FUNCTION__,sector,count);

    ret = rt_usbh_storage_read10(intf, (rt_uint8_t*)buffer, sector, count, timeout);

    if (ret != RT_EOK)
    {
        rt_kprintf("usb mass_storage read failed\n");
        return RES_ERROR;
    }

    return RES_OK;
}

static DRESULT rt_udisk_write (void *dev, BYTE* buffer, DWORD sector,
    UINT count)
{
    rt_err_t ret;
    struct uhintf* intf;
    struct ustor_data* data;
    int timeout = USB_TIMEOUT_LONG/5;
    struct udisk_device *disk = (struct udisk_device *)dev;

    /* check parameter */
    if((disk==NULL) ||(buffer==NULL) || (disk->intf == NULL)){
	    rt_kprintf("udisk write parameter error\n");
	    return RES_ERROR;
    }

    if(count * SECTOR_SIZE > 4096) timeout *= 2;

    data = (struct ustor_data*)disk->user_data;
    intf = disk->intf;

    os_printf("%s write sector:%d count:%d \n",__FUNCTION__,sector,count);

    ret = rt_usbh_storage_write10(intf, (rt_uint8_t*)buffer, sector, count, timeout);
    if (ret != RT_EOK)
    {
        rt_kprintf("usb mass_storage write %d sector failed\n", count);
        return RES_ERROR;
    }

    return RES_OK;

}

static DRESULT rt_udisk_control(void *dev, BYTE cmd, void *buf)
{
    printf("%s %d\n",__FUNCTION__,__LINE__);

    struct udisk_device *udisk = (struct udisk_device *)dev;
	rt_uint8_t ret	= RES_OK;
    //os_printf("cmd:%d\n",cmd);
	switch(cmd)
    {
		case CTRL_SYNC:
			break;
		case GET_SECTOR_COUNT:
			*(DWORD *)buf = udisk->count;	
			ret = RES_OK;
			break;

		case GET_SECTOR_SIZE:
			*(WORD *)buf = udisk->sector_size;
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

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops udisk_device_ops =
{
    rt_udisk_init,
    RT_NULL,
    RT_NULL,
    rt_udisk_read,
    rt_udisk_write,
    rt_udisk_control
};
#endif

static struct fatfs_diskio udisk_driver = 
{
    .status = rt_udisk_status,
    .init = rt_udisk_init,
    .read = rt_udisk_read,
    .write = rt_udisk_write,
    .ioctl = rt_udisk_control,
};


void rt_udisk_ota_thread(void)
{

#if DVP_EN	
		void *dvp = (void *)dev_get(HG_DVP_DEVID);
		if(dvp)
		{
			dvp_close(dvp);
		}
#endif

if(!udisk_ota){
    rt_uint8_t *cache_buf = NULL;
    void *fp = osal_fopen("USB:/UPDATE.BIN","r");
    if(!fp)
    {
        os_printf("udisk ota file not open\n");
        goto __udisk_ota_end;
    }
    rt_uint32_t filesize = osal_fsize(fp);
    rt_uint32_t filesize_tmp = filesize;
    rt_uint32_t readsize = UDISK_CACHE_SIZE;
    rt_uint32_t ota_offset = 0;
    cache_buf = (rt_uint8_t *)os_malloc(UDISK_CACHE_SIZE);
    if(!cache_buf)
    {
        os_printf("cache_buf malloc failed\n");
        goto __udisk_ota_end;
    }
    os_printf("filesize:%d cache_buf:%x\n",filesize,cache_buf);

    while(filesize)
    {
        if(filesize < UDISK_CACHE_SIZE)
        {
            readsize = filesize;
        }

        osal_fread(cache_buf,readsize,1,fp);
        libota_write_fw(filesize_tmp,ota_offset,cache_buf,readsize);
        filesize -= readsize;
        ota_offset += readsize;
    }

    udisk_ota = 1;

__udisk_ota_end:
    if(fp)
    {
        osal_fclose(fp);
    }

    if(cache_buf)
    {
        os_free(cache_buf);
    }
}  

}

/**
 * This function will run udisk driver when usb disk is detected.
 *
 * @param intf the usb interface instance.
 *
 * @return the error code, RT_EOK on successfully.
 */
rt_err_t rt_udisk_run(struct uhintf* intf)
{
    int i = 0;
    rt_err_t ret;
    //char dname[8];
    char sname[8];
    rt_uint8_t max_lun, *sector, sense[18], inquiry[36];
    ustor_t stor;

    /* check parameter */
    RT_ASSERT(intf != RT_NULL);
    printf("%s %d\n",__FUNCTION__,__LINE__);
    /* set interface */
//    ret = rt_usbh_set_interface(intf->device, intf->intf_desc->bInterfaceNumber);
//    if(ret != RT_EOK)
//        rt_usbh_clear_feature(intf->device, 0, USB_FEATURE_ENDPOINT_HALT);
    /* reset mass storage class device */
    ret = rt_usbh_storage_reset(intf);
    if(ret != RT_EOK) return ret;

    stor = (ustor_t)intf->user_data;
    stor->dev_cnt = 1;

    /* get max logic unit number */
    ret = rt_usbh_storage_get_max_lun(intf, &max_lun);
    if(ret != RT_EOK)
        rt_usbh_clear_feature(intf->device, 0, USB_FEATURE_ENDPOINT_HALT);

    /* reset pipe in endpoint */
    if(stor->pipe_in->status == UPIPE_STATUS_STALL)
    {
        ret = rt_usbh_clear_feature(intf->device,
        stor->pipe_in->ep.bEndpointAddress, USB_FEATURE_ENDPOINT_HALT);
        printf("%s %d\n",__FUNCTION__,__LINE__);
        if(ret != RT_EOK) return ret;
    }


    /* reset pipe out endpoint */
    if(stor->pipe_out->status == UPIPE_STATUS_STALL)
    {
        ret = rt_usbh_clear_feature(intf->device,
        stor->pipe_out->ep.bEndpointAddress, USB_FEATURE_ENDPOINT_HALT);
        printf("%s %d\n",__FUNCTION__,__LINE__);
        if(ret != RT_EOK) return ret;
    }

    while((ret = rt_usbh_storage_inquiry(intf, inquiry)) != RT_EOK)
    {
        if(ret == -RT_EIO) return ret;

        rt_thread_delay(5);
        if(i++ < 10) continue;
        rt_kprintf("rt_usbh_storage_inquiry error\n");
        return -RT_ERROR;
    }

    i = 0;

    /* wait device ready */
    while((ret = rt_usbh_storage_test_unit_ready(intf)) != RT_EOK)
    {
        if(ret == -RT_EIO) return ret;

        ret = rt_usbh_storage_request_sense(intf, sense);
        if(ret == -RT_EIO) return ret;

        rt_thread_delay(10);
        if(i++ < 10) continue;

        rt_kprintf("rt_usbh_storage_test_unit_ready error\n");
        return -RT_ERROR;
    }

    i = 0;
    rt_memset(stor->capicity, 0, sizeof(stor->capicity));

    /* get storage capacity */
    while((ret = rt_usbh_storage_get_capacity(intf,
        (rt_uint8_t*)stor->capicity)) != RT_EOK)
    {
        if(ret == -RT_EIO) return ret;

        rt_thread_delay(50);
        if(i++ < 10) continue;

        stor->capicity[0] = 2880;
        stor->capicity[1] = 0x200;

        rt_kprintf("rt_usbh_storage_get_capacity error\n");
        break;
    }

    stor->capicity[0] = uswap_32(stor->capicity[0]);
    stor->capicity[1] = uswap_32(stor->capicity[1]);
    stor->capicity[0] += 1;

    rt_kprintf("capicity %d, block size %d\n",
        stor->capicity[0], stor->capicity[1]);

    /* get the first sector to read partition table */
    sector = (rt_uint8_t*) rt_malloc (SECTOR_SIZE);
    if (sector == RT_NULL)
    {
        rt_kprintf("allocate partition sector buffer failed\n");
        return -RT_ERROR;
    }

    rt_memset(sector, 0, SECTOR_SIZE);

    rt_kprintf("read partition table\n");

    /* get the partition table */
    ret = rt_usbh_storage_read10(intf, sector, 0, 1, USB_TIMEOUT_LONG);
    if(ret != RT_EOK)
    {
        rt_kprintf("read parition table error\n");

        rt_free(sector);
        return -RT_ERROR;
    }

    rt_kprintf("finished reading partition\n");


    int res = 0;

    struct ustor_data* data = rt_malloc(sizeof(struct ustor_data));
    if (data == RT_NULL)
    {
        rt_kprintf("Allocate partition data buffer failed.");
    }
    rt_memset(data, 0, sizeof(struct ustor_data));
    data->intf = intf;
    // data->udisk_id = udisk_get_id();
    // os_printf("udisk_id:%d\n",data->udisk_id);
    // os_snprintf(dname, 6, "ud%d-%d", data->udisk_id, 0);
    os_snprintf(sname, 8, "sem_ud%d",  0);

    /* register sdcard device */
    stor->dev[0].type    = 0;           //RT_Device_Class_Block;
#ifdef RT_USING_DEVICE_OPS
    stor->dev[0].ops     = &udisk_device_ops;
#else
    stor->dev[0].status  = rt_udisk_status;
    stor->dev[0].init    = rt_udisk_init;
    stor->dev[0].read    = rt_udisk_read;
    stor->dev[0].write   = rt_udisk_write;
    stor->dev[0].ioctl = rt_udisk_control;
#endif
    stor->dev[0].user_data = (void*)data;

    usb_disk.count = stor->capicity[0];
    usb_disk.sector_size = stor->capicity[1];
    usb_disk.user_data = data;
    usb_disk.intf = intf;


    fatfs_register_drive(DEV_USB, &udisk_driver, &usb_disk);
    if(!udisk_fs)
    {
        udisk_fs = (FATFS *)os_malloc(sizeof(FATFS));
    }

    if(udisk_fs)
    {
        res = f_mount(udisk_fs, "USB:", 1);
        if(res)
        {
            os_printf("%s mount fatfs err:%d\n",__FUNCTION__,res);
            return RT_EOK;
        }
    }
  
    DIR dir;
    FILINFO f_info;
    rt_uint8_t maxdir = 0;
    FRESULT rets;
    rets = f_opendir(&dir, "USB:/");
    if (rets != FR_OK) {
        printf("failed open\n");
        return 1;
    }
    os_printf("===========USB DIR===========\n");
    while (1) {
            rets = f_readdir(&dir, &f_info); 
            if (rets != FR_OK) {
                break;
            }
            if (f_info.fname[0] == 0) {
                break; 
            } else {
            if (f_info.fattrib) { 
                printf("%s \n", f_info.fname);
                maxdir++;
            }
        }
    }
    os_printf("=============================\n");
    os_printf("%s %d\n",__FUNCTION__,__LINE__);



#if 0
    rt_thread_t thread;
    thread = rt_thread_create("udisk_test",rt_udisk_ota_thread,NULL,4096,OS_TASK_PRIORITY_NORMAL,0);
    if(thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }    
#endif

    rt_free(sector);

    return RT_EOK;
}

/**
 * This function will be invoked when usb disk plug out is detected and it would clean
 * and release all udisk related resources.
 *
 * @param intf the usb interface instance.
 *
 * @return the error code, RT_EOK on successfully.
 */
rt_err_t rt_udisk_stop(struct uhintf* intf)
{
    int i;
    ustor_t stor;
    struct ustor_data* data;

    /* check parameter */
    RT_ASSERT(intf != RT_NULL);
    RT_ASSERT(intf->device != RT_NULL);

    stor = (ustor_t)intf->user_data;
    RT_ASSERT(stor != RT_NULL);

    for(i=0; i<stor->dev_cnt; i++)
    {
        struct ustor_device *dev = &stor->dev[i];
        data = (struct ustor_data*)dev->user_data;
        usb_disk.intf = NULL;
        /* unmount filesystem */
        f_umount("USB:");

        if(udisk_fs)
        {
            os_free(udisk_fs);
            udisk_fs = NULL;
        }
        
        // udisk_free_id(data->udisk_id);

        rt_free(data);
    }

    return RT_EOK;
}

#endif

