/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-10-01     Yi Qiu       first version
 * 2012-11-25     Heyuanjie87  reduce the memory consumption
 * 2012-12-09     Heyuanjie87  change function and endpoint handler
 * 2013-07-25     Yi Qiu       update for USB CV test
 */

#include <rtthread.h>
#include "include/rttusb_device.h"
#include "mstorage.h"


#ifdef RT_USB_DEVICE_MSTORAGE

#ifdef USBDISK
#include "usbd_mass_speed_optimize.h"
//通过宏PINGPANG_BUF_EN决定是否打开usbd mass读写速度优化的线程使能(sram使用空间增加),默认打开
#if USBDISK == 1
#define RT_USE_UDISK_DEVICE (&sd_udisk)        /* sd u盘：sd_udisk       */
#elif USBDISK == 2
#define RT_USE_UDISK_DEVICE (&flash_udisk)     /* flash u盘：flash_udisk */
#elif USBDISK == 3
#define RT_USE_UDISK_DEVICE  (&sram_udisk)     /* sram u盘：sram_udisk   */
#else
#define RT_USE_UDISK_DEVICE  RT_NULL   
#endif
#else
#define PINGPANG_BUF_EN 	0
#define RT_USE_UDISK_DEVICE (&sram_udisk)      /* sram u盘：sram_udisk   */
#endif

#define SRAM_UDISK_EN       0
#define UDISK_READONLY      0               // 0:读写，1：只读

//自定义
typedef int (*udisk_read)(uint32 lba,uint32 count,uint8* buf);
typedef int (*udisk_write)(uint32 lba,uint32 count,uint8* buf);
typedef uint8_t (*udisk_get_status)(void);
typedef uint32_t (*udisk_get_cap)();
typedef uint32_t (*udisk_get_sector_size)();
struct udisk_priv_func_s
{
    udisk_get_sector_size   u_get_sector_size;    //扇区的大小
    udisk_read              u_read;
    udisk_write             u_write;
    udisk_get_status        u_get_status;   //这个现在只是判断是否存在
    udisk_get_cap           u_get_cap;
};
//如果有sd卡,注册sd函数接口
#if SDH_EN == 1
#include "lib/sdhost/sdhost.h"
int usb_sd_scsi_write(uint32 lba, uint32 count, uint8* buf);
int usb_sd_scsi_read(uint32 lba, uint32 count, uint8* buf);
uint8_t get_sd_status2(void);
uint32 get_sd_cap() ;
uint32_t get_sd_sector_size();

const struct udisk_priv_func_s sd_udisk = 
{
    .u_get_sector_size = get_sd_sector_size,
    .u_read = usb_sd_scsi_read,
    .u_write = usb_sd_scsi_write,
    .u_get_status = get_sd_status2,
    .u_get_cap = get_sd_cap,
};
#endif



#if FLASHDISK_EN == 1
int flashdisk_usb_write(uint32_t sector,uint32_t count,uint8* buf);
int flashdisk_usb_read(uint32_t sector,uint32_t count,uint8* buf);
uint8_t flashdisk_get_status(void);
uint32 flashdisk_get_sec_count() ;
uint32_t flashdisk_get_sec_size();

const struct udisk_priv_func_s flash_udisk = 
{
    .u_get_sector_size = flashdisk_get_sec_size,
    .u_read = flashdisk_usb_read,
    .u_write = flashdisk_usb_write,
    .u_get_status = flashdisk_get_status,
    .u_get_cap = flashdisk_get_sec_count,
};
#endif

#if SRAM_UDISK_EN == 1
#define USB_SRAM_MASS_CAPCITY (100*1024)
static rt_uint8_t usb_sram_filesystem[USB_SRAM_MASS_CAPCITY];

int sram_usb_write(uint32_t sector,uint32_t count,uint8* buf)
{
    hw_memcpy(usb_sram_filesystem + sector * 512, buf, count * 512);
    return 0;
}

int sram_usb_read(uint32_t sector,uint32_t count,uint8* buf)
{
    hw_memcpy(buf, usb_sram_filesystem + sector * 512, count * 512);
    return 0;
}

uint8_t sram_get_status(void)
{
    return 0;
}

uint32 sram_get_sec_count()
{
    return USB_SRAM_MASS_CAPCITY / 512;
}

uint32_t sram_get_sec_size()
{
    return 512;
}

const struct udisk_priv_func_s sram_udisk = 
{
    .u_get_sector_size = sram_get_sec_size,
    .u_read = sram_usb_read,
    .u_write = sram_usb_write,
    .u_get_status = sram_get_status,
    .u_get_cap = sram_get_sec_count,
};

#endif


#ifdef RT_USING_DFS_MNTTABLE
#include "dfs_fs.h"
#endif

#define MSTRORAGE_INTF_STR_INDEX 11

//#define DBG_TAG           "usbdevice.mstorage"
//#define DBG_LVL           DBG_INFO
//#include <rtdbg.h>

static ufunction_t mstorage_func = RT_NULL;

enum STAT
{
    STAT_CBW,
    STAT_CMD,
    STAT_CSW,
    STAT_RECEIVE,
    STAT_SEND,
};

typedef enum
{
    FIXED,
    COUNT,
    BLOCK_COUNT,
}CB_SIZE_TYPE;

typedef enum
{
    DIR_IN,
    DIR_OUT,
    DIR_NONE,
}CB_DIR;

typedef rt_ssize_t (*cbw_handler)(ufunction_t func, ustorage_cbw_t cbw);

struct scsi_cmd
{
    rt_uint16_t cmd;
    cbw_handler handler;
    rt_size_t cmd_len;
    CB_SIZE_TYPE type;
    rt_size_t data_size;
    CB_DIR dir;
};

struct rt_device_blk_geometry
{
    rt_uint64_t sector_count;                           /**< count of sectors */
    rt_uint32_t bytes_per_sector;                       /**< number of bytes per sector */
    rt_uint32_t block_size;                             /**< number of bytes to erase one block */
};

struct mstorage
{
    struct ustorage_cbw *cbw;
    struct ustorage_csw csw_response;
    uep_t ep_in;
    uep_t ep_out;
    int status;
    int test_unit;
    rt_uint32_t cb_data_size;
    void *disk;
    rt_uint32_t block;
    rt_int32_t count;
    rt_int32_t size;
    struct scsi_cmd* processing;
    struct rt_device_blk_geometry geometry;
    const struct udisk_priv_func_s *udisk_func;
    #if PINGPANG_BUF_EN
    rt_uint32_t multi_sector_en;
    rt_uint32_t read_calc_optimize_sec;
    rt_uint32_t write_calc_optimize_sec;
    struct usbd_mass_speed_info udisk_speed_info;
    #endif
};

rt_align(4)
static struct udevice_descriptor dev_desc =
{
    USB_DESC_LENGTH_DEVICE,     //bLength;
    USB_DESC_TYPE_DEVICE,       //type;
    USB_BCD_VERSION,            //bcdUSB;
    USB_CLASS_MASS_STORAGE,     //bDeviceClass;
    0x06,                       //bDeviceSubClass;
    0x50,                       //bDeviceProtocol;
    0x40,                       //bMaxPacketSize0;
    _VENDOR_ID,                 //idVendor;
    _PRODUCT_ID,                //idProduct;
    USB_BCD_DEVICE,             //bcdDevice;
    USB_STRING_MANU_INDEX,      //iManufacturer;
    USB_STRING_PRODUCT_INDEX,   //iProduct;
    USB_STRING_SERIAL_INDEX,    //iSerialNumber;
    USB_DYNAMIC,                //bNumConfigurations;
};

//FS and HS needed
rt_align(4)
static struct usb_qualifier_descriptor dev_qualifier =
{
    sizeof(dev_qualifier),          //bLength
    USB_DESC_TYPE_DEVICEQUALIFIER,  //bDescriptorType
    0x0200,                         //bcdUSB
    USB_CLASS_MASS_STORAGE,         //bDeviceClass
    0x06,                           //bDeviceSubClass
    0x50,                           //bDeviceProtocol
    64,                             //bMaxPacketSize0
    0x01,                           //bNumConfigurations
    0,
};


rt_align(4)
const static struct umass_descriptor _mass_desc =
{
#ifdef RT_USB_DEVICE_COMPOSITE
    /* Interface Association Descriptor */
    {
        USB_DESC_LENGTH_IAD,
        USB_DESC_TYPE_IAD,
        USB_DYNAMIC,
        0x01,
        USB_CLASS_MASS_STORAGE,
        0x06,
        0x50,
        0x00,
    },
#endif
    {
        USB_DESC_LENGTH_INTERFACE,  //bLength;
        USB_DESC_TYPE_INTERFACE,    //type;
        USB_DYNAMIC,                //bInterfaceNumber;
        0x00,                       //bAlternateSetting;
        0x02,                       //bNumEndpoints
        USB_CLASS_MASS_STORAGE,     //bInterfaceClass;
        0x06,                       //bInterfaceSubClass;
        0x50,                       //bInterfaceProtocol;
#ifdef RT_USB_DEVICE_COMPOSITE
        MSTRORAGE_INTF_STR_INDEX,
#else
        0x00,                       //iInterface;
#endif
    },

    {
        USB_DESC_LENGTH_ENDPOINT,   //bLength;
        USB_DESC_TYPE_ENDPOINT,     //type;
        USB_DYNAMIC | USB_DIR_OUT,  //bEndpointAddress;
        USB_EP_ATTR_BULK,           //bmAttributes;
        USB_DYNAMIC,                //wMaxPacketSize;
        0x00,                       //bInterval;
    },

    {
        USB_DESC_LENGTH_ENDPOINT,   //bLength;
        USB_DESC_TYPE_ENDPOINT,     //type;
        USB_DYNAMIC | USB_DIR_IN,   //bEndpointAddress;
        USB_EP_ATTR_BULK,           //bmAttributes;
        USB_DYNAMIC,                //wMaxPacketSize;
        0x00,                       //bInterval;
    },
};

rt_align(4)
const static char* _ustring[] =
{
    "Language",
    "RT-Thread Team.",
    "RTT Mass Storage",
    "320219198301",
    "Configuration",
    "Interface",
};

static rt_ssize_t _test_unit_ready(ufunction_t func, ustorage_cbw_t cbw);
static rt_ssize_t _request_sense(ufunction_t func, ustorage_cbw_t cbw);
static rt_ssize_t _inquiry_cmd(ufunction_t func, ustorage_cbw_t cbw);
static rt_ssize_t _allow_removal(ufunction_t func, ustorage_cbw_t cbw);
static rt_ssize_t _start_stop(ufunction_t func, ustorage_cbw_t cbw);
static rt_ssize_t _mode_sense_6(ufunction_t func, ustorage_cbw_t cbw);
static rt_ssize_t _read_capacities(ufunction_t func, ustorage_cbw_t cbw);
static rt_ssize_t _read_capacity(ufunction_t func, ustorage_cbw_t cbw);
static rt_ssize_t _read_10(ufunction_t func, ustorage_cbw_t cbw);
static rt_ssize_t _write_10(ufunction_t func, ustorage_cbw_t cbw);
static rt_ssize_t _verify_10(ufunction_t func, ustorage_cbw_t cbw);

rt_align(4)
static struct scsi_cmd cmd_data[] =
{
    {SCSI_TEST_UNIT_READY, _test_unit_ready, 6,  FIXED,       0, DIR_NONE},
    {SCSI_REQUEST_SENSE,   _request_sense,   6,  COUNT,       0, DIR_IN},
    {SCSI_INQUIRY_CMD,     _inquiry_cmd,     6,  COUNT,       0, DIR_IN},
    {SCSI_ALLOW_REMOVAL,   _allow_removal,   6,  FIXED,       0, DIR_NONE},
    {SCSI_MODE_SENSE_6,    _mode_sense_6,    6,  COUNT,       0, DIR_IN},
    {SCSI_START_STOP,      _start_stop,      6,  FIXED,       0, DIR_NONE},
    {SCSI_READ_CAPACITIES, _read_capacities, 10, COUNT,       0, DIR_NONE},
    {SCSI_READ_CAPACITY,   _read_capacity,   10, FIXED,       8, DIR_IN},
    {SCSI_READ_10,         _read_10,         10, BLOCK_COUNT, 0, DIR_IN},
    {SCSI_WRITE_10,        _write_10,        10, BLOCK_COUNT, 0, DIR_OUT},
    {SCSI_VERIFY_10,       _verify_10,       10, FIXED,       0, DIR_NONE},
};

static void _send_status(ufunction_t func)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);

    LOG_D("_send_status");

    data = (struct mstorage*)func->user_data;
    data->ep_in->request.buffer = (rt_uint8_t*)&data->csw_response;
    data->ep_in->request.size = SIZEOF_CSW;
    data->ep_in->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
    data->status = STAT_CSW;
}

static rt_ssize_t _test_unit_ready(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);

    LOG_D("_test_unit_ready");

    data = (struct mstorage*)func->user_data;
    data->csw_response.status = 0;

    if(data->test_unit || data->udisk_func->u_get_status())
    {
        data->csw_response.status = 1;
    }

    return 0;
}

static rt_ssize_t _allow_removal(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);

    LOG_D("_allow_removal");

    data = (struct mstorage*)func->user_data;
    data->csw_response.status = 0;

    return 0;
}

/**
 * This function will handle inquiry command request.
 *
 * @param func the usb function object.
 * @param cbw the command block wrapper.
 *
 * @return RT_EOK on successful.
 */

static rt_ssize_t _inquiry_cmd(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;
    rt_uint8_t *buf;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);

    LOG_D("_inquiry_cmd");

    data = (struct mstorage*)func->user_data;
    buf = data->ep_in->buffer;

    *(rt_uint32_t*)&buf[0] = 0x0 | (0x80 << 8);
    *(rt_uint32_t*)&buf[4] = 31;

    rt_memset(&buf[8], 0x20, 28);
    rt_memcpy(&buf[8], "RTT", 3);
    rt_memcpy(&buf[16], "USB Disk", 8);

    data->cb_data_size = MIN(data->cb_data_size, SIZEOF_INQUIRY_CMD);
    data->ep_in->request.buffer = buf;
    data->ep_in->request.size = data->cb_data_size;
    data->ep_in->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
    data->status = STAT_CMD;

    return data->cb_data_size;
}

/**
 * This function will handle sense request.
 *
 * @param func the usb function object.
 * @param cbw the command block wrapper.
 *
 * @return RT_EOK on successful.
 */
static rt_ssize_t _request_sense(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;
    struct request_sense_data *buf;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);

    LOG_D("_request_sense");

    data = (struct mstorage*)func->user_data;
    buf = (struct request_sense_data *)data->ep_in->buffer;

    buf->ErrorCode = 0x70;
    buf->Valid = 0;
    buf->SenseKey = 2;
    buf->Information[0] = 0;
    buf->Information[1] = 0;
    buf->Information[2] = 0;
    buf->Information[3] = 0;
    buf->AdditionalSenseLength = 0x0a;
    buf->AdditionalSenseCode   = 0x3a;
    buf->AdditionalSenseCodeQualifier = 0;

    data->cb_data_size = MIN(data->cb_data_size, SIZEOF_REQUEST_SENSE);
    data->ep_in->request.buffer = (rt_uint8_t*)data->ep_in->buffer;
    data->ep_in->request.size = data->cb_data_size;
    data->ep_in->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
    data->status = STAT_CMD;

    return data->cb_data_size;
}

/**
 * This function will handle mode_sense_6 request.
 *
 * @param func the usb function object.
 * @param cbw the command block wrapper.
 *
 * @return RT_EOK on successful.
 */
static rt_ssize_t _mode_sense_6(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;
    rt_uint8_t *buf;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);

    LOG_D("_mode_sense_6");

    data = (struct mstorage*)func->user_data;
    buf = data->ep_in->buffer;
    buf[0] = 3;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
    
    if(UDISK_READONLY)
    {
        buf[2] = 0x80;
    }

    data->cb_data_size = MIN(data->cb_data_size, SIZEOF_MODE_SENSE_6);
    data->ep_in->request.buffer = buf;
    data->ep_in->request.size = data->cb_data_size;
    data->ep_in->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
    data->status = STAT_CMD;

    return data->cb_data_size;
}

/**
 * This function will handle read_capacities request.
 *
 * @param func the usb function object.
 * @param cbw the command block wrapper.
 *
 * @return RT_EOK on successful.
 */
static rt_ssize_t _read_capacities(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;
    rt_uint8_t *buf;
    rt_uint32_t sector_count, sector_size;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);

    LOG_D("_read_capacities");

    data = (struct mstorage*)func->user_data;

    if(data->test_unit || data->udisk_func->u_get_status()){
        data->csw_response.status = 1;
        rt_usbd_ep_set_stall(func->device, data->ep_in);
        return 0;
    }
    data->geometry.sector_count = data->udisk_func->u_get_cap();

    buf = data->ep_in->buffer;
    sector_count = data->geometry.sector_count;
    sector_size = data->geometry.bytes_per_sector;

    *(rt_uint32_t*)&buf[0] = 0x08000000;
    buf[4] = sector_count >> 24;
    buf[5] = 0xff & (sector_count >> 16);
    buf[6] = 0xff & (sector_count >> 8);
    buf[7] = 0xff & (sector_count);
    buf[8] = 0x02;
    buf[9] = 0xff & (sector_size >> 16);
    buf[10] = 0xff & (sector_size >> 8);
    buf[11] = 0xff & sector_size;

    data->cb_data_size = MIN(data->cb_data_size, SIZEOF_READ_CAPACITIES);
    data->ep_in->request.buffer = buf;
    data->ep_in->request.size = data->cb_data_size;
    data->ep_in->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
    data->status = STAT_CMD;

    return data->cb_data_size;
}

/**
 * This function will handle read_capacity request.
 *
 * @param func the usb function object.
 * @param cbw the command block wapper.
 *
 * @return RT_EOK on successful.
 */
static rt_ssize_t _read_capacity(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;

    rt_uint8_t *buf;
    rt_uint32_t sector_count, sector_size;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);

    LOG_D("_read_capacity");

    data = (struct mstorage*)func->user_data;

    if(data->test_unit || data->udisk_func->u_get_status()){
        data->csw_response.status = 1;
        rt_usbd_ep_set_stall(func->device, data->ep_in);
        return 0;
    }
    data->geometry.sector_count = data->udisk_func->u_get_cap();

    buf = data->ep_in->buffer;
    sector_count = data->geometry.sector_count - 1; /* Last Logical Block Address */
    sector_size = data->geometry.bytes_per_sector;

    buf[0] = sector_count >> 24;
    buf[1] = 0xff & (sector_count >> 16);
    buf[2] = 0xff & (sector_count >> 8);
    buf[3] = 0xff & (sector_count);
    buf[4] = 0x0;
    buf[5] = 0xff & (sector_size >> 16);
    buf[6] = 0xff & (sector_size >> 8);
    buf[7] = 0xff & sector_size;

    data->cb_data_size = MIN(data->cb_data_size, SIZEOF_READ_CAPACITY);
    data->ep_in->request.buffer = buf;
    data->ep_in->request.size = data->cb_data_size;
    data->ep_in->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
    data->status = STAT_CMD;

    return data->cb_data_size;
}

/**
 * This function will handle read_10 request.
 *
 * @param func the usb function object.
 * @param cbw the command block wrapper.
 *
 * @return RT_EOK on successful.
 */
static rt_ssize_t _read_10(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);

    data = (struct mstorage*)func->user_data;

    if(data->test_unit || data->udisk_func->u_get_status()){
        data->csw_response.status = 1;
        rt_usbd_ep_set_stall(func->device, data->ep_in);
        return 0;
    }

    data->block = cbw->cb[2]<<24 | cbw->cb[3]<<16 | cbw->cb[4]<<8  |
             cbw->cb[5]<<0;
    data->count = cbw->cb[7]<<8 | cbw->cb[8]<<0;

    RT_ASSERT(data->count < data->geometry.sector_count);

    data->csw_response.data_reside = data->cb_data_size;

#if PINGPANG_BUF_EN
    data->read_calc_optimize_sec = usbd_mass_speed_sec_calc(data->count);
    if (data->read_calc_optimize_sec) {
        data->read_calc_optimize_sec =  data->count / data->read_calc_optimize_sec;
        data->multi_sector_en = 1;             
    } else {
        data->multi_sector_en = 0;
    }     

    usbd_mass_speed_optimize_send_mq(g_dev, data->block, data->count, READ_FLAG);
    os_sema_down(g_dev->sem, osWaitForever);

    if(g_dev->error_flag == 1)
    {
        rt_kprintf("read data error\n");
        os_sema_up(g_dev->usb_sem_write);
        rt_usbd_ep_set_stall(func->device, data->ep_in);
        data->csw_response.status = 1;
		return 0;
    }
    
    if(g_dev->pingpang_flag)
    {
        os_sema_up(g_dev->usb_sem_write);
        data->ep_in->buffer = data->udisk_speed_info.buf_2;
    }
    else
    {
        os_sema_up(g_dev->usb_sem_write);
        data->ep_in->buffer = data->udisk_speed_info.buf_1;
    }

    if(data->multi_sector_en)
    {
        data->ep_in->request.buffer = data->ep_in->buffer;
        data->ep_in->request.size = data->geometry.bytes_per_sector * data->read_calc_optimize_sec;
        data->ep_in->request.req_type = UIO_REQUEST_WRITE;
        rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
        data->status = STAT_SEND;

    }
    else
    {
        data->ep_in->request.buffer = data->ep_in->buffer;
        data->ep_in->request.size = data->geometry.bytes_per_sector;
        data->ep_in->request.req_type = UIO_REQUEST_WRITE;
        rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
        data->status = STAT_SEND;
    }



#else
	rt_size_t size = 0;
    size = data->udisk_func->u_read(data->block, 1, data->ep_in->buffer);  //每个sector都进行读取sd
    if(size){
        rt_kprintf("read data error\n");
        rt_usbd_ep_set_stall(func->device, data->ep_in);
        data->csw_response.status = 1;
		return 0;
	}

    data->ep_in->request.buffer = data->ep_in->buffer;
    data->ep_in->request.size = data->geometry.bytes_per_sector;
    data->ep_in->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
    data->status = STAT_SEND;

#endif
    return data->geometry.bytes_per_sector;
}

/**
 * This function will handle write_10 request.
 *
 * @param func the usb function object.
 * @param cbw the command block wrapper.
 *
 * @return RT_EOK on successful.
 */
static rt_ssize_t _write_10(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);

    data = (struct mstorage*)func->user_data;

    data->block = cbw->cb[2]<<24 | cbw->cb[3]<<16 | cbw->cb[4]<<8  |
             cbw->cb[5]<<0;
    data->count = cbw->cb[7]<<8 | cbw->cb[8];
    data->csw_response.data_reside = cbw->xfer_len;
    data->size = data->count * data->geometry.bytes_per_sector;

    LOG_D("_write_10 count 0x%x block 0x%x 0x%x",
                                data->count, data->block, data->geometry.sector_count);

    data->csw_response.data_reside = data->cb_data_size;

#if PINGPANG_BUF_EN
    data->write_calc_optimize_sec = usbd_mass_speed_sec_calc(data->count);
    if (data->write_calc_optimize_sec) {
        data->write_calc_optimize_sec = data->count / data->write_calc_optimize_sec;
        data->multi_sector_en = 1;             
    } else {
        data->multi_sector_en = 0;
    }   

    if(g_dev->pingpang_flag)
    {
        data->ep_out->buffer = data->udisk_speed_info.buf_1;
    }
    else
    {
        data->ep_out->buffer = data->udisk_speed_info.buf_2;
    }

    if(data->multi_sector_en)
    {
        data->ep_out->request.buffer = data->ep_out->buffer;
        data->ep_out->request.size = data->geometry.bytes_per_sector * data->write_calc_optimize_sec;
        data->ep_out->request.req_type = UIO_REQUEST_READ_FULL;
        rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);
        data->status = STAT_RECEIVE;
    }
    else
    {
        data->ep_out->request.buffer = data->ep_out->buffer;
        data->ep_out->request.size = data->geometry.bytes_per_sector;
        data->ep_out->request.req_type = UIO_REQUEST_READ_FULL;
        rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);
        data->status = STAT_RECEIVE;
    }

#else
    data->ep_out->request.buffer = data->ep_out->buffer;
    data->ep_out->request.size = data->geometry.bytes_per_sector;
    data->ep_out->request.req_type = UIO_REQUEST_READ_FULL;
    rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);
    data->status = STAT_RECEIVE;
#endif
    return data->geometry.bytes_per_sector;
}

/**
 * This function will handle verify_10 request.
 *
 * @param func the usb function object.
 *
 * @return RT_EOK on successful.
 */
static rt_ssize_t _verify_10(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);

    LOG_D("_verify_10");

    data = (struct mstorage*)func->user_data;
    data->csw_response.status = 0;

    return 0;
}

static rt_ssize_t _start_stop(ufunction_t func,
    ustorage_cbw_t cbw)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);

    LOG_D("_start_stop");

    data = (struct mstorage*)func->user_data;
    data->csw_response.status = 0;

    data->test_unit = 1;

    return 0;
}

static rt_err_t _ep_in_handler(ufunction_t func, rt_size_t size)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);

    LOG_D("_ep_in_handler");

    data = (struct mstorage*)func->user_data;

    switch(data->status)
    {
    case STAT_CSW:
        if(data->ep_in->request.size != SIZEOF_CSW)
        {
            rt_kprintf("Size of csw command error\n");
            rt_usbd_ep_set_stall(func->device, data->ep_in);
        }
        else
        {
            LOG_D("return to cbw status");
            data->ep_out->request.buffer = (rt_uint8_t*)data->cbw;
            data->ep_out->request.size = SIZEOF_CBW;
            data->ep_out->request.req_type = UIO_REQUEST_READ_FULL;
            rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);
            data->status = STAT_CBW;
        }
        break;
     case STAT_CMD:
        if(data->csw_response.data_reside == 0xFF)
        {
            data->csw_response.data_reside = 0;
        }
        else
        {
            data->csw_response.data_reside -= data->ep_in->request.size;
            if(data->csw_response.data_reside != 0)
            {
                LOG_D("data_reside %d, request %d",
                    data->csw_response.data_reside, data->ep_in->request.size);
                if(data->processing->dir == DIR_OUT)
                {
                    rt_usbd_ep_set_stall(func->device, data->ep_out);
                }
                else
                {
                    //rt_kprintf("warning:in stall path but not stall\n");

                    /* FIXME: Disable the operation or the disk cannot work. */
                    //rt_usbd_ep_set_stall(func->device, data->ep_in);
                }
                data->csw_response.data_reside = 0;
            }
        }
        _send_status(func);
        break;
     case STAT_SEND:

        #if PINGPANG_BUF_EN
        
        if(data->multi_sector_en)
        {
            data->csw_response.data_reside -= data->ep_in->request.size;
            data->count -= data->read_calc_optimize_sec;
            data->block += data->read_calc_optimize_sec;

        }
        else
        {
            data->csw_response.data_reside -= data->ep_in->request.size;
            data->count--;
            data->block++;
        }

        if(data->count > 0 && data->csw_response.data_reside > 0)   
        {
            os_sema_down(g_dev->sem, osWaitForever);

            if(g_dev->error_flag == 1)
            {
                rt_kprintf("disk read error\n");
                os_sema_up(g_dev->usb_sem_write);
                rt_usbd_ep_set_stall(func->device, data->ep_in);
                return -RT_ERROR;                 
            }

            if(g_dev->pingpang_flag)
            {
                os_sema_up(g_dev->usb_sem_write);
                data->ep_in->buffer = data->udisk_speed_info.buf_2;
            }
            else
            {
                os_sema_up(g_dev->usb_sem_write);
                data->ep_in->buffer = data->udisk_speed_info.buf_1;
            }

            if(data->multi_sector_en)
            {
                data->ep_in->request.buffer = data->ep_in->buffer;
                data->ep_in->request.size = data->geometry.bytes_per_sector * data->read_calc_optimize_sec;
                data->ep_in->request.req_type = UIO_REQUEST_WRITE;
                rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
            }
            else
            {
                data->ep_in->request.buffer = data->ep_in->buffer;
                data->ep_in->request.size = data->geometry.bytes_per_sector;
                data->ep_in->request.req_type = UIO_REQUEST_WRITE;
                rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
            }
        }
        else
        {
            _send_status(func);
        }        

        #else
        data->csw_response.data_reside -= data->ep_in->request.size;
        data->count--;
        data->block++;
        if(data->count > 0 && data->csw_response.data_reside > 0)
        {
            rt_size_t read_size = 0;
            read_size = data->udisk_func->u_read(data->block, 1, data->ep_in->buffer);
            if(read_size){
                rt_kprintf("disk read error\n");
                rt_usbd_ep_set_stall(func->device, data->ep_in);
                return -RT_ERROR; 
            }

            data->ep_in->request.buffer = data->ep_in->buffer;
            data->ep_in->request.size = data->geometry.bytes_per_sector;
            data->ep_in->request.req_type = UIO_REQUEST_WRITE;
            rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
        }
        else
        {
            _send_status(func);
        }
        #endif
        break;
     }

     return RT_EOK;
}

#ifdef  MASS_CBW_DUMP
static void cbw_dump(struct ustorage_cbw* cbw)
{
    RT_ASSERT(cbw != RT_NULL);

    LOG_D("signature 0x%x", cbw->signature);
    LOG_D("tag 0x%x", cbw->tag);
    LOG_D("xfer_len 0x%x", cbw->xfer_len);
    LOG_D("dflags 0x%x", cbw->dflags);
    LOG_D("lun 0x%x", cbw->lun);
    LOG_D("cb_len 0x%x", cbw->cb_len);
    LOG_D("cb[0] 0x%x", cbw->cb[0]);
}
#endif

static struct scsi_cmd* _find_cbw_command(rt_uint16_t cmd)
{
    int i;

    for(i=0; i<sizeof(cmd_data)/sizeof(struct scsi_cmd); i++)
    {
        if(cmd_data[i].cmd == cmd)
            return &cmd_data[i];
    }

    return RT_NULL;
}

static void _cb_len_calc(ufunction_t func, struct scsi_cmd* cmd,
    ustorage_cbw_t cbw)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(cmd != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);

    data = (struct mstorage*)func->user_data;
    if(cmd->cmd_len == 6)
    {
        switch(cmd->type)
        {
        case COUNT:
            data->cb_data_size = cbw->cb[4];
            break;
        case BLOCK_COUNT:
            data->cb_data_size = cbw->cb[4] * data->geometry.bytes_per_sector;
            break;
        case FIXED:
            data->cb_data_size = cmd->data_size;
            break;
        default:
            break;
        }
    }
    else if(cmd->cmd_len == 10)
    {
        switch(cmd->type)
        {
        case COUNT:
            data->cb_data_size = cbw->cb[7]<<8 | cbw->cb[8];
            break;
        case BLOCK_COUNT:
            data->cb_data_size = (cbw->cb[7]<<8 | cbw->cb[8]) *
                data->geometry.bytes_per_sector;
            break;
        case FIXED:
            data->cb_data_size = cmd->data_size;
            break;
        default:
            break;
        }
    }

    //workaround: for stability in full-speed mode
    else if(cmd->cmd_len == 12)
    {
        switch(cmd->type)
        {
        case COUNT:
            data->cb_data_size = cbw->cb[4];
            break;
        default:
            break;
        }
    }
    else
    {
        rt_kprintf("cmd_len error %d\n", cmd->cmd_len);
    }
}

static rt_bool_t _cbw_verify(ufunction_t func, struct scsi_cmd* cmd,
    ustorage_cbw_t cbw)
{
    struct mstorage *data;

    RT_ASSERT(cmd != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);
    RT_ASSERT(func != RT_NULL);

    data = (struct mstorage*)func->user_data;
    if(cmd->cmd_len != cbw->cb_len)
    {
        rt_kprintf("cb_len error\n");
        cmd->cmd_len = cbw->cb_len;
    }

    if(cbw->xfer_len > 0 && data->cb_data_size == 0)
    {
        rt_kprintf("xfer_len > 0 && data_size == 0\n");
        return RT_FALSE;
    }

    if(cbw->xfer_len == 0 && data->cb_data_size > 0)
    {
        rt_kprintf("xfer_len == 0 && data_size > 0");
        return RT_FALSE;
    }

    if(((cbw->dflags & USB_DIR_IN) && (cmd->dir == DIR_OUT)) ||
        (!(cbw->dflags & USB_DIR_IN) && (cmd->dir == DIR_IN)))
    {
        rt_kprintf("dir error\n");
        return RT_FALSE;
    }

    if(cbw->xfer_len > data->cb_data_size)
    {
        rt_kprintf("xfer_len > data_size\n");
        return RT_FALSE;
    }

    if(cbw->xfer_len < data->cb_data_size)
    {
        rt_kprintf("xfer_len < data_size\n");
        data->cb_data_size = cbw->xfer_len;
        data->csw_response.status = 1;
    }

    return RT_TRUE;
}

static rt_ssize_t _cbw_handler(ufunction_t func, struct scsi_cmd* cmd,
    ustorage_cbw_t cbw)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);
    RT_ASSERT(cmd->handler != RT_NULL);

    data = (struct mstorage*)func->user_data;
    data->processing = cmd;
    return cmd->handler(func, cbw);
}

/**
 * This function will handle mass storage bulk out endpoint request.
 *
 * @param func the usb function object.
 * @param size request size.
 *
 * @return RT_EOK.
 */
static rt_err_t _ep_out_handler(ufunction_t func, rt_size_t size)
{
    struct mstorage *data;
    struct scsi_cmd* cmd;
    rt_size_t len;
    struct ustorage_cbw* cbw;
    

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);

    LOG_D("_ep_out_handler %d", size);

    data = (struct mstorage*)func->user_data;
    cbw = data->cbw;

    if(data->status == STAT_CBW)
    {
        // memcpy(cbw, data->ep_out->buffer, sizeof(struct ustorage_cbw));
        /* dump cbw information */
        if(cbw->signature != CBW_SIGNATURE || size != SIZEOF_CBW)
        {
            goto exit;
        }

        data->csw_response.signature = CSW_SIGNATURE;
        data->csw_response.tag = cbw->tag;
        data->csw_response.data_reside = cbw->xfer_len;
        data->csw_response.status = 0;

        LOG_D("ep_out reside %d", data->csw_response.data_reside);

        cmd = _find_cbw_command(cbw->cb[0]);
        if(cmd == RT_NULL)
        {
            rt_kprintf("can't find cbw command\n");
            goto exit;
        }

        _cb_len_calc(func, cmd, cbw);
        if(!_cbw_verify(func, cmd, cbw))
        {
            goto exit;
        }

        len = _cbw_handler(func, cmd, cbw);
        if(len == 0)
        {
            _send_status(func);
        }

        return RT_EOK;
    }
    else if(data->status == STAT_RECEIVE)
    {
        LOG_D("write size %d block 0x%x request size 0x%x",
                                    size, data->block, data->size);

        #if PINGPANG_BUF_EN
        if(data->csw_response.data_reside == data->cb_data_size)
        {
            usbd_mass_speed_optimize_send_mq(g_dev, data->block, data->count, WRITE_FLAG);
        }
        else
        {
            os_sema_up(g_dev->usb_sem_write);
        }


        data->size -= size;
        data->csw_response.data_reside -= size;

        if(data->csw_response.data_reside != 0)
        {
            os_sema_down(g_dev->sem, osWaitForever);
            if(g_dev->pingpang_flag)
            {
                data->ep_out->buffer = data->udisk_speed_info.buf_1;
            }
            else
            {
                data->ep_out->buffer = data->udisk_speed_info.buf_2;
            }

            if(data->multi_sector_en)
            {
                data->ep_out->request.buffer = data->ep_out->buffer;
                data->ep_out->request.size = data->geometry.bytes_per_sector * data->write_calc_optimize_sec;
                data->ep_out->request.req_type = UIO_REQUEST_READ_FULL;
                rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);
                data->block += data->write_calc_optimize_sec;
                data->count -= data->write_calc_optimize_sec;
            }
            else
            {
                data->ep_out->request.buffer = data->ep_out->buffer;
                data->ep_out->request.size = data->geometry.bytes_per_sector;
                data->ep_out->request.req_type = UIO_REQUEST_READ_FULL;
                rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);
                data->block ++;
                data->count --;
            }
        }
        else
        {			
            os_sema_up(g_dev->usb_sem_write);
			os_sema_down(g_dev->sem, osWaitForever);
            if(g_dev->error_flag)
            {
                rt_kprintf("disk write error\n");
                goto exit;
            }
            _send_status(func);
        }
        #else
		rt_uint32_t ret = 0;
        ret = data->udisk_func->u_write(data->block, 1, data->ep_out->buffer);
        if(ret)
        {
            rt_kprintf("disk write error\n");
            goto exit;
        }

        data->size -= size;
        data->csw_response.data_reside -= size;

        if(data->csw_response.data_reside != 0)
        {
            data->ep_out->request.buffer = data->ep_out->buffer;
            data->ep_out->request.size = data->geometry.bytes_per_sector;
            data->ep_out->request.req_type = UIO_REQUEST_READ_FULL;
            rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);
            data->block ++;
        }
        else
        {
            _send_status(func);
        }
        #endif

        return RT_EOK;
    }

exit:
    if(data->csw_response.data_reside)
    {
        if(cbw->dflags & USB_DIR_IN)
        {
            rt_usbd_ep_set_stall(func->device, data->ep_in);
        }
        else
        {
            rt_usbd_ep_set_stall(func->device, data->ep_in);
            rt_usbd_ep_set_stall(func->device, data->ep_out);
        }
    }
    data->csw_response.status = 1;
    _send_status(func);

    return -RT_ERROR;
}

rt_uint32_t lun __attribute__((aligned(4)));
/**
 * This function will handle mass storage interface request.
 *
 * @param func the usb function object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _interface_handler(ufunction_t func, ureq_t setup)
{
    lun = 0; 

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    LOG_D("mstorage_interface_handler");

    switch(setup->bRequest)
    {
    case USBREQ_GET_MAX_LUN:

        LOG_D("USBREQ_GET_MAX_LUN");

        if(setup->wValue || setup->wLength != 1)
        {
            rt_usbd_ep0_set_stall(func->device);
        }
        else
        {
            rt_usbd_ep0_write(func->device, &lun, setup->wLength);
        }
        break;
    case USBREQ_MASS_STORAGE_RESET:

        LOG_D("USBREQ_MASS_STORAGE_RESET");

        if(setup->wValue || setup->wLength != 0)
        {
            rt_usbd_ep0_set_stall(func->device);
        }
        else
        {
            dcd_ep0_send_status(func->device->dcd);
        }
        break;
    default:
        rt_kprintf("unknown interface request\n");
        break;
    }

    return RT_EOK;
}

/**
 * This function will run mass storage function, it will be called on handle set configuration request.
 *
 * @param func the usb function object.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _function_enable(ufunction_t func)
{
    struct mstorage *data;
    RT_ASSERT(func != RT_NULL);
    os_printf("Mass storage function enabled");
    data = (struct mstorage*)func->user_data;

#if 0
    data->disk = RT_USE_UDISK_DEVICE;
    if(data->disk == RT_NULL)
    {
        os_printf("disk get failed!!!!!!!!!\n");
        return -RT_ERROR;
    }

#ifdef RT_USING_DFS_MNTTABLE
    dfs_unmount_device(data->disk);
#endif
#endif

    data->geometry.bytes_per_sector = data->udisk_func->u_get_sector_size();
    if(!data->udisk_func->u_get_status())
    {
        data->geometry.sector_count = data->udisk_func->u_get_cap();
    }
    else
    {
        data->geometry.sector_count = 0;
        os_printf("get sd cap failed\n");
    }

    data->cbw = (struct ustorage_cbw *)rt_malloc(sizeof(struct ustorage_cbw));
    if(data->cbw == RT_NULL)
    {
        rt_kprintf("data->cbw malloc failed!!!!\n");
        return -RT_ENOMEM;
    }

#if PINGPANG_BUF_EN
    data->udisk_speed_info.buf_1 = (rt_uint8_t*)rt_malloc(data->geometry.bytes_per_sector * MULTI_SECTOR_COUNT);
    if(data->udisk_speed_info.buf_1 == RT_NULL)
    {
        rt_free(data->cbw);
        data->cbw = RT_NULL;
        rt_kprintf("data->udisk_speed_info.buf_1 malloc failed!!!\n");
        return -RT_ENOMEM;        
    }
    data->udisk_speed_info.buf_2 = (rt_uint8_t*)rt_malloc(data->geometry.bytes_per_sector * MULTI_SECTOR_COUNT);
    if(data->udisk_speed_info.buf_2 == RT_NULL)
    {
        rt_free(data->cbw);
        data->cbw = RT_NULL;
        rt_free(data->udisk_speed_info.buf_1);
        data->udisk_speed_info.buf_1 = RT_NULL;
        rt_kprintf("data->udisk_speed_info.buf_2 malloc failed!!!\n");
        return -RT_ENOMEM;        
    }

    //默认ep in和ep out使用同一个buffer空间，正常情况in和out不会同时进行
    if(g_dev->pingpang_flag)
    {
        data->ep_in->buffer = data->udisk_speed_info.buf_2;
        data->ep_out->buffer = data->udisk_speed_info.buf_2;
    }
    else
    {
        data->ep_in->buffer = data->udisk_speed_info.buf_1;
        data->ep_out->buffer = data->udisk_speed_info.buf_1;
    }

    usbd_mass_speed_optimize_thread_init(&data->udisk_speed_info, data->udisk_func->u_read, data->udisk_func->u_write);

#else
    data->ep_in->buffer = (rt_uint8_t*)rt_malloc(data->geometry.bytes_per_sector);
    if(data->ep_in->buffer == RT_NULL)
    {
        rt_kprintf("ep in no memory\n");
        return -RT_ENOMEM;
    }
    data->ep_out->buffer = (rt_uint8_t*)rt_malloc(data->geometry.bytes_per_sector);
    if(data->ep_out->buffer == RT_NULL)
    {
        rt_free(data->ep_in->buffer);
        data->ep_in->buffer = RT_NULL;
        rt_kprintf("ep out no memory\n");
        return -RT_ENOMEM;
    }
#endif

    data->test_unit = 0;

    /* prepare to read CBW request */
    data->ep_out->request.buffer = (rt_uint8_t*)data->cbw;
    data->ep_out->request.size = SIZEOF_CBW;
    data->ep_out->request.req_type = UIO_REQUEST_READ_FULL;
    rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);

    return RT_EOK;
}

/**
 * This function will stop mass storage function, it will be called on handle set configuration request.
 *
 * @param device the usb device object.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _function_disable(ufunction_t func)
{
    struct mstorage *data;
    RT_ASSERT(func != RT_NULL);

    os_printf("Mass storage function disabled");

    data = (struct mstorage*)func->user_data;

    if(data->cbw != RT_NULL)
    {
        rt_free(data->cbw);
        data->cbw = RT_NULL;
    }

#if PINGPANG_BUF_EN
    if(data->udisk_speed_info.buf_1 != RT_NULL)
    {
        rt_free(data->udisk_speed_info.buf_1);
        data->udisk_speed_info.buf_1 = RT_NULL;
    }
    if(data->udisk_speed_info.buf_2 != RT_NULL)
    {
        rt_free(data->udisk_speed_info.buf_2);
        data->udisk_speed_info.buf_2 = RT_NULL;
    }

    usbd_mass_speed_optimize_thread_deinit();
#else
    if(data->ep_in->buffer != RT_NULL)
    {
        rt_free(data->ep_in->buffer);
        data->ep_in->buffer = RT_NULL;
    }

    if(data->ep_out->buffer != RT_NULL)
    {
        rt_free(data->ep_out->buffer);
        data->ep_out->buffer = RT_NULL;
    }
#endif

#if 0
    if(data->disk != RT_NULL)
    {
#ifdef RT_USING_DFS_MNTTABLE
        dfs_mount_device(data->disk);
#endif
        data->disk = RT_NULL;
    }
#endif

    data->status = STAT_CBW;

    return RT_EOK;
}

static struct ufunction_ops ops =
{
    _function_enable,
    _function_disable,
    RT_NULL,
};
static rt_err_t _mstorage_descriptor_config(umass_desc_t desc, rt_uint8_t cintf_nr, rt_uint8_t device_is_hs)
{
#ifdef RT_USB_DEVICE_COMPOSITE
    desc->iad_desc.bFirstInterface = cintf_nr;
#endif
    desc->ep_out_desc.wMaxPacketSize = device_is_hs ? 512 : 64;
    desc->ep_in_desc.wMaxPacketSize = device_is_hs ? 512 : 64;
    return RT_EOK;
}
/**
 * This function will create a mass storage function instance.
 *
 * @param device the usb device object.
 *
 * @return RT_EOK on successful.
 */
ufunction_t rt_usbd_function_mstorage_create(udevice_t device)
{
    uintf_t intf;
    struct mstorage *data;
    ufunction_t func;
    ualtsetting_t setting;
    umass_desc_t mass_desc;

    /* parameter check */
    RT_ASSERT(device != RT_NULL);

    /* set usb device string description */
#ifdef RT_USB_DEVICE_COMPOSITE
    rt_usbd_device_set_interface_string(device, MSTRORAGE_INTF_STR_INDEX, _ustring[2]);
#else
    rt_usbd_device_set_string(device, _ustring);
#endif

    /* create a mass storage function */
    func = rt_usbd_function_new(device, &dev_desc, &ops);
    device->dev_qualifier = &dev_qualifier;
    mstorage_func = func;

    /* allocate memory for mass storage function data */
    data = (struct mstorage*)rt_malloc(sizeof(struct mstorage));
    rt_memset(data, 0, sizeof(struct mstorage));
    data->udisk_func = RT_USE_UDISK_DEVICE;
	if(data->udisk_func == RT_NULL)
	{
		os_printf("%s :data->udisk_func is NULL!!!",__FUNCTION__);
		return 0;
	}
	
    func->user_data = (void*)data;

    /* create an interface object */
    intf = rt_usbd_interface_new(device, _interface_handler);
    func->intf[0] = intf;

    /* create an alternate setting object */
    setting = rt_usbd_altsetting_new(sizeof(struct umass_descriptor));
    func->setting[0] = setting;

    /* config desc in alternate setting */
    rt_usbd_altsetting_config_descriptor(setting, &_mass_desc, (rt_off_t)&((umass_desc_t)0)->intf_desc);

    /* configure the msc interface descriptor */
    _mstorage_descriptor_config(setting->desc, intf->intf_num, device->dcd->device_is_hs);

    /* create a bulk out and a bulk in endpoint */
    mass_desc = (umass_desc_t)setting->desc;
    data->ep_in = rt_usbd_endpoint_new(&mass_desc->ep_in_desc, _ep_in_handler);
    data->ep_out = rt_usbd_endpoint_new(&mass_desc->ep_out_desc, _ep_out_handler);

    func->ep[0] = data->ep_in;
    func->ep[1] = data->ep_out;

    /* add the bulk out and bulk in endpoint to the alternate setting */
    rt_usbd_altsetting_add_endpoint(setting, data->ep_out);
    rt_usbd_altsetting_add_endpoint(setting, data->ep_in);

    /* add the alternate setting to the interface, then set default setting */
    rt_usbd_interface_add_altsetting(intf, setting);
    rt_usbd_set_altsetting(intf, 0);

    /* add the interface to the mass storage function */
    rt_usbd_function_add_interface(func, intf);

    return func;
}

void rt_usbd_function_mstorage_delete()
{
    struct mstorage *data;
    
    os_printf("%s %d\n",__FUNCTION__,__LINE__);

    if(mstorage_func == RT_NULL)
    {
        os_printf("%s [%d]:Mstorage func is NULL!!!\n",__FUNCTION__,__LINE__);
        return;
    }

    data = (struct mstorage*)mstorage_func->user_data;

    if(data->ep_in->buffer != RT_NULL)
    {
        rt_free(data->ep_in->buffer);
        data->ep_in->buffer = RT_NULL;
    }

    if(data->ep_out->buffer != RT_NULL)
    {
        rt_free(data->ep_out->buffer);
        data->ep_out->buffer = RT_NULL;
    }
    
    if(data != RT_NULL)
    {
        rt_free(data);
        data = RT_NULL;
    }
    
    if(mstorage_func != RT_NULL)
    {
        rt_usbd_function_release(mstorage_func);
        mstorage_func = RT_NULL;
    }
}

struct udclass msc_class =
{
    .rt_usbd_function_create = rt_usbd_function_mstorage_create,
    .rt_usbd_function_delete = rt_usbd_function_mstorage_delete,
};

int rt_usbd_msc_class_register(rt_uint32_t dev_id)
{
    rt_usbd_class_register(&msc_class, dev_id);
    return 0;
}

int rt_usbd_msc_class_unregister(rt_uint32_t dev_id)
{
    rt_usbd_class_driver_unregister(&msc_class, dev_id);
    return 0;
}


#endif
