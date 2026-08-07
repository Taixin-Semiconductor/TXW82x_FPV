/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-11-16     ZYH          first version
 */
//#include <rthw.h>
//#include <rtdevice.h>
#include <include/rttusb_device.h>
#include "winusb.h"
#include "hal/isp_tunning.h"
#include "dev/audio/ausys_debug.h"
#include "dev/audio/ausys.h"
#include "dev/audio/ausys_da.h"
#include "dev/audio/components/eq/aueq.h"
#include "hal/isp.h"


#ifdef RT_USB_DEVICE_WINUSB

#if ISP_TUNNING_EN
extern struct isp_tunnning_dev *isp_tunning;
#endif

struct winusb_device
{
    struct rt_device parent;
    void (*cmd_handler)(rt_uint8_t *buffer,rt_size_t size);
    void (*rx_handler)(rt_uint8_t *buffer,rt_size_t size);
    void (*tx_handler)(rt_uint8_t *buffer,rt_size_t size);
    rt_uint8_t cmd_buff[256];
    rt_uint8_t dat_buff[1024];
    uep_t ep_out;
    uep_t ep_in;
    void *user_data;
};
#define WINUSB_INTF_STR_INDEX 13
typedef struct winusb_device * winusb_device_t;

rt_align(4)
static struct udevice_descriptor dev_desc =
{
    USB_DESC_LENGTH_DEVICE,     //bLength;
    USB_DESC_TYPE_DEVICE,       //type;
    USB_BCD_VERSION,            //bcdUSB;
    0xFF,                       //bDeviceClass;
    0xFF,                       //bDeviceSubClass;
    0xFF,                       //bDeviceProtocol;
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
    0xFF,                           //bDeviceClass
    0x00,                           //bDeviceSubClass
    0x00,                           //bDeviceProtocol
    64,                             //bMaxPacketSize0
    0x01,                           //bNumConfigurations
    0,
};

rt_align(4)
struct winusb_descriptor _winusb_desc =
{
#ifdef RT_USB_DEVICE_COMPOSITE
    /* Interface Association Descriptor */
    {
        USB_DESC_LENGTH_IAD,
        USB_DESC_TYPE_IAD,
        USB_DYNAMIC,
        0x01,
        0xFF,
        0x00,
        0x00,
        0x00,
    },
#endif
    /*interface descriptor*/
    {
        USB_DESC_LENGTH_INTERFACE,  //bLength;
        USB_DESC_TYPE_INTERFACE,    //type;
        USB_DYNAMIC,                //bInterfaceNumber;
        0x00,                       //bAlternateSetting;
        0x02,                       //bNumEndpoints
        0xFF,                       //bInterfaceClass;
        0xFF,                       //bInterfaceSubClass;
        0xFF,                       //bInterfaceProtocol;
#ifdef RT_USB_DEVICE_COMPOSITE
        WINUSB_INTF_STR_INDEX,
#else
        0x00,                       //iInterface;
#endif
    },
    /*endpoint descriptor*/
    {
        USB_DESC_LENGTH_ENDPOINT,
        USB_DESC_TYPE_ENDPOINT,
        USB_DYNAMIC | USB_DIR_OUT,
        USB_EP_ATTR_BULK,
        USB_DYNAMIC,
        0x00,
    },
    /*endpoint descriptor*/
    {
        USB_DESC_LENGTH_ENDPOINT,
        USB_DESC_TYPE_ENDPOINT,
        USB_DYNAMIC | USB_DIR_IN,
        USB_EP_ATTR_BULK,
        USB_DYNAMIC,
        0x00,
    },
};

rt_align(4)
const static char* _ustring[] =
{
    "Language",
    "RT-Thread Team.",
    "RTT Win USB",
    "32021919830108",
    "Configuration",
    "Interface",
    USB_STRING_OS//must be
};

rt_align(4)
const char winusb_device_interface_guids[142] = {
  ///////////////////////////////////////
  /// WCID property descriptor
  ///////////////////////////////////////
  0x8e, 0x00, 0x00, 0x00,                           /* dwLength */
  0x00, 0x01,                                       /* bcdVersion */
  0x05, 0x00,                                       /* wIndex */
  0x01, 0x00,                                       /* wCount */
  
  ///////////////////////////////////////
  /// registry propter descriptor
  ///////////////////////////////////////
  0x84, 0x00, 0x00, 0x00,                           /* dwSize */
  0x01, 0x00, 0x00, 0x00,                           /* dwPropertyDataType */
  0x28, 0x00,                                       /* wPropertyNameLength */
  /* DeviceInterfaceGUID */
  'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00,       /* wcName_20 */
  'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00,       /* wcName_20 */
  't', 0x00, 'e', 0x00, 'r', 0x00, 'f', 0x00,       /* wcName_20 */
  'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00,       /* wcName_20 */
  'U', 0x00, 'I', 0x00, 'D', 0x00, 0x00, 0x00,      /* wcName_20 */
  0x4e, 0x00, 0x00, 0x00,                           /* dwPropertyDataLength */
  /* {1D4B2365-4749-48EA-B38A-7C6FDDDD7E26} */
  '{', 0x00, '1', 0x00, 'D', 0x00, '4', 0x00,       /* wcData_39 */
  'B', 0x00, '2', 0x00, '3', 0x00, '6', 0x00,       /* wcData_39 */
  '5', 0x00, '-', 0x00, '4', 0x00, '7', 0x00,       /* wcData_39 */
  '4', 0x00, '9', 0x00, '-', 0x00, '4', 0x00,       /* wcData_39 */
  '8', 0x00, 'E', 0x00, 'A', 0x00, '-', 0x00,       /* wcData_39 */
  'B', 0x00, '3', 0x00, '8', 0x00, 'A', 0x00,       /* wcData_39 */
  '-', 0x00, '7', 0x00, 'C', 0x00, '6', 0x00,       /* wcData_39 */
  'F', 0x00, 'D', 0x00, 'D', 0x00, 'D', 0x00,       /* wcData_39 */
  'D', 0x00, '7', 0x00, 'E', 0x00, '2', 0x00,       /* wcData_39 */
  '6', 0x00, '}', 0x00, 0x00, 0x00,                 /* wcData_39 */
};

rt_align(4)
struct usb_os_proerty winusb_proerty[] =
{
    USB_OS_PROPERTY_DESC(USB_OS_PROPERTY_TYPE_REG_SZ,"DeviceInterfaceGUID",winusb_device_interface_guids),
};

rt_align(4)
struct usb_os_function_comp_id_descriptor winusb_func_comp_id_desc =
{
    .bFirstInterfaceNumber = USB_DYNAMIC,
    .reserved1          = 0x01,
    .compatibleID       = {'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00},
    .subCompatibleID    = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    .reserved2          = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};


static rt_err_t _ep_out_handler(ufunction_t func, rt_size_t size)
{
    winusb_device_t winusb_device = (winusb_device_t)func->user_data;
    LOG_D("%s %x %d\r\n", __FUNCTION__, winusb_device->ep_out->buffer, size);
	
   /* example write back recieve data */
   rt_ssize_t win_usb_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size);
   win_usb_write(&winusb_device->parent, 0, winusb_device->ep_out->buffer, size);

#ifdef HG_EQ_APP_CMD_PARSE 
#define AUSYS_DA_CMD_EQ             (0x01)
#define AUSYS_DA_CMD_SAMPLE_RATE    (0x02)
#define AUSYS_DA_CMD_VOLUME         (0x03)
#define AUSYS_DA_CMD_TEST_OPEN      (0x04)
#define AUSYS_DA_CMD_TEST_CLOSE     (0x05)
#define AUSYS_DA_CMD_TEST_SINE      (0x06)


    uint32 cmd = *(uint32 *)&winusb_device->ep_out->buffer[0];
    uint32 len = *(uint32 *)&winusb_device->ep_out->buffer[4];
    uint8  *p_content = &winusb_device->ep_out->buffer[4+4];

    struct aueq_device *p_dev = (struct aueq_device*)dev_get(HG_AUEQ_DEVID);
    uint32 sample_rate = *(uint32 *)p_content;
    uint32 percent_0to100 = *(uint32 *)p_content;
    uint32 cur_volume_0to100 = 0;


    switch (cmd) {
        case (AUSYS_DA_CMD_EQ):
            if (p_dev) {
                aueq_run(p_dev, (void *)p_content, len);
            } else {
                os_printf("eq dev is NULL\r\n");
            }
            break;
        case (AUSYS_DA_CMD_SAMPLE_RATE):
            ausys_da_change_sample_rate(sample_rate);
            break;
        case (AUSYS_DA_CMD_VOLUME):
            ausys_da_get_cur_volume((uint32 *)&cur_volume_0to100);
            os_printf("cur vol:%d\r\n", cur_volume_0to100);
            ausys_da_change_volume(percent_0to100);
            break;
        case (AUSYS_DA_CMD_TEST_OPEN):
            ausys_da_test_mode(AUSYS_DA_TEST_OPEN, 0, 0, 0);
            break;
        case (AUSYS_DA_CMD_TEST_CLOSE):
            ausys_da_test_mode(AUSYS_DA_TEST_CLOSE, 0, 0, 0);
            break;
        case (AUSYS_DA_CMD_TEST_SINE):
            ausys_da_test_mode(AUSYS_DA_TEST_PLAY_SINE, 0, 0, 0);
            break;
    }
#endif

    rt_ssize_t win_usb_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size);
    win_usb_read(&winusb_device->parent, 0, winusb_device->ep_out->buffer, 512);
    
   if(winusb_device->rx_handler != RT_NULL)
   {
       winusb_device->rx_handler(winusb_device->ep_out->buffer, size);
   }


#if ISP_TUNNING_EN
    rt_ssize_t win_usb_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size);
    uint16 head         = *(uint16 *)&winusb_device->ep_out->buffer[0];
	if (head == isp_tunning->tunning_head)
	{
        isp_tunning->cmd_head    = head;
        isp_tunning->cmd_num     = *(uint16 *)&winusb_device->ep_out->buffer[2];
        isp_tunning->cmd_channel = *(uint16 *)&winusb_device->ep_out->buffer[4];
        isp_tunning->p_data      =  (uint16 *)&winusb_device->ep_out->buffer[8];
        isp_tunning->p_winusb    = (rt_device_t)&winusb_device->parent;
        os_sema_up(&isp_tunning->usb_cmd_sema);
	}
    win_usb_read(&winusb_device->parent, 0, winusb_device->ep_out->buffer, 512);
#endif
    return RT_EOK;
}

static rt_err_t _ep_in_handler(ufunction_t func, rt_size_t size)
{
    winusb_device_t winusb_device = (winusb_device_t)func->user_data;
    
    LOG_D("%s %x %d\r\n", __FUNCTION__, winusb_device->ep_in->buffer, size);
    if(winusb_device->tx_handler != RT_NULL)
    {
        winusb_device->tx_handler( winusb_device->ep_in->buffer, size);
    }
#if ISP_TUNNING_EN
    os_sema_up(&isp_tunning->usb_write_sema);
#endif
    return RT_EOK;
}
static ufunction_t cmd_func = RT_NULL;
static rt_err_t _ep0_cmd_handler(udevice_t device, rt_size_t size)
{
    winusb_device_t winusb_device;

    if(cmd_func != RT_NULL)
    {
        winusb_device = (winusb_device_t)cmd_func->user_data;
        cmd_func = RT_NULL;
        if(winusb_device->cmd_handler != RT_NULL)
        {
            winusb_device->cmd_handler(winusb_device->cmd_buff,size);
        }
    }
    dcd_ep0_send_status(device->dcd);
    return RT_EOK;
}
static rt_err_t _ep0_cmd_read(ufunction_t func, ureq_t setup)
{
    winusb_device_t winusb_device = (winusb_device_t)func->user_data;
    cmd_func = func;
    rt_usbd_ep0_read(func->device,winusb_device->cmd_buff,setup->wLength,_ep0_cmd_handler);
    return RT_EOK;
}
static rt_err_t _interface_handler(ufunction_t func, ureq_t setup)
{
    switch(setup->bRequest)
    {
        os_printf("winusb bRequest:%x wIndex:%x \r\n",setup->bRequest,setup->wIndex);
    case 'A':
        switch(setup->wIndex)
        {
        case 0x05:
            usbd_os_proerty_descriptor_send(func,setup,winusb_proerty,sizeof(winusb_proerty)/sizeof(winusb_proerty[0]));
            break;
        }
        break;
    case 0x0A://customer
        _ep0_cmd_read(func, setup);
        break;
    }

    return RT_EOK;
}
static rt_err_t _function_enable(ufunction_t func)
{
    RT_ASSERT(func != RT_NULL);
    
    winusb_device_t winusb_device = (winusb_device_t)func->user_data;
    LOG_D("%s\r\n", __FUNCTION__);
	/* trig recieve data when function enable */
    rt_ssize_t win_usb_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size);
    win_usb_read(&winusb_device->parent, 0, winusb_device->dat_buff, 512);

    
    return RT_EOK;
}
static rt_err_t _function_disable(ufunction_t func)
{
    RT_ASSERT(func != RT_NULL);
    return RT_EOK;
}

static struct ufunction_ops ops =
{
    _function_enable,
    _function_disable,
    RT_NULL,
};

static rt_err_t _winusb_descriptor_config(winusb_desc_t winusb, rt_uint8_t cintf_nr, rt_uint8_t device_is_hs)
{
#ifdef RT_USB_DEVICE_COMPOSITE
    winusb->iad_desc.bFirstInterface = cintf_nr;
#endif
    winusb->ep_out_desc.wMaxPacketSize = device_is_hs ? 512 : 64;
    winusb->ep_in_desc.wMaxPacketSize = device_is_hs ? 512 : 64;
    winusb_func_comp_id_desc.bFirstInterfaceNumber = cintf_nr;
    return RT_EOK;
}

rt_ssize_t win_usb_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
	/* read & write by class, actually, all class use the same udcd */
    udcd_t udcd = (udcd_t)dev_get(HG_USB_DEV_CONTROLLER_DEVID);

    udevice_t  device = rt_usbd_find_device(udcd);

    //if(device->state != USB_STATE_CONFIGURED)
    if(device == NULL)
    {
        return 0;
    }
    winusb_device_t winusb_device = (winusb_device_t)dev;
    winusb_device->ep_out->buffer = buffer;
    winusb_device->ep_out->request.buffer = buffer;
    winusb_device->ep_out->request.size = size;
    winusb_device->ep_out->request.req_type = UIO_REQUEST_READ_BEST;
    rt_usbd_io_request(device,winusb_device->ep_out,&winusb_device->ep_out->request);
    return size;
}
rt_ssize_t win_usb_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    udcd_t udcd = (udcd_t)dev_get(HG_USB_DEV_CONTROLLER_DEVID);

    udevice_t  device = rt_usbd_find_device(udcd);
    if (device->state != USB_STATE_CONFIGURED)
    {
        return 0;
    }
    winusb_device_t winusb_device = (winusb_device_t)dev;
    winusb_device->ep_in->buffer = (void *)buffer;
    winusb_device->ep_in->request.buffer = winusb_device->ep_in->buffer;
    winusb_device->ep_in->request.size = size;
    winusb_device->ep_in->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(device,winusb_device->ep_in,&winusb_device->ep_in->request);
    return size;
}
static rt_err_t  win_usb_control(rt_device_t dev, int cmd, void *args)
{
    //udcd_t udcd = (struct usb_device *)dev_get(HG_USB_DEV_CONTROLLER_DEVID);

    winusb_device_t winusb_device = (winusb_device_t)dev;
    //if(RT_DEVICE_CTRL_CONFIG == cmd)
    {
        winusb_device->cmd_handler = (void(*)(rt_uint8_t*,rt_size_t))args;
    }
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops winusb_device_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    win_usb_read,
    win_usb_write,
    win_usb_control,
};
#endif

static rt_err_t rt_usb_winusb_init(ufunction_t func)
{
//    rt_err_t ret;

    //winusb_device_t winusb_device   = (winusb_device_t)func->user_data;
//    struct usb_device *p_usb_d = (struct usb_device *)dev_get(HG_USB_DEV_CONTROLLER_DEVID);
//    ret = dev_register(HG_USB_DEV_CONTROLLER_DEVID, (struct dev_obj *)p_usb_d);

    return 0;
}

udevice_t g_winusb_device = NULL;

ufunction_t rt_usbd_function_winusb_create(udevice_t device)
{
    os_printf("%s %d\n",__FUNCTION__,__LINE__);   

    ufunction_t         func;
    winusb_device_t     winusb_device;

    uintf_t             winusb_intf;
    ualtsetting_t       winusb_setting;
    winusb_desc_t       winusb_desc;

    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    g_winusb_device = device;
    /* set usb device string description */
#ifdef RT_USB_DEVICE_COMPOSITE
    rt_usbd_device_set_interface_string(device, WINUSB_INTF_STR_INDEX, _ustring[2]);
    rt_usbd_device_set_interface_string(device, USB_STRING_OS_INDEX, _ustring[6]);
#else
    rt_usbd_device_set_string(device, _ustring);
    rt_usbd_device_set_interface_string(device, USB_STRING_OS_INDEX, _ustring[6]);
#endif

    /* create a cdc function */
    func = rt_usbd_function_new(device, &dev_desc, &ops);
    rt_usbd_device_set_qualifier(device, &dev_qualifier);

    /* allocate memory for cdc vcom data */
    winusb_device = (winusb_device_t)rt_malloc(sizeof(struct winusb_device));
    if (winusb_device == NULL)
        return RT_NULL;
    rt_memset((void *)winusb_device, 0, sizeof(struct winusb_device));
    func->user_data = (void*)winusb_device;
    winusb_device->user_data = (void *)func;
    /* create an interface object */
    winusb_intf = rt_usbd_interface_new(device, _interface_handler);

    /* create an alternate setting object */
    winusb_setting = rt_usbd_altsetting_new(sizeof(struct winusb_descriptor));

    /* config desc in alternate setting */
    rt_usbd_altsetting_config_descriptor(winusb_setting, &_winusb_desc, (rt_off_t)&((winusb_desc_t)0)->intf_desc);

    /* configure the hid interface descriptor */
    _winusb_descriptor_config(winusb_setting->desc, winusb_intf->intf_num, device->dcd->device_is_hs);

    /* create endpoint */
    winusb_desc = (winusb_desc_t)winusb_setting->desc;
    winusb_device->ep_out = rt_usbd_endpoint_new(&winusb_desc->ep_out_desc, _ep_out_handler);
    winusb_device->ep_in  = rt_usbd_endpoint_new(&winusb_desc->ep_in_desc, _ep_in_handler);

    /* add the int out and int in endpoint to the alternate setting */
    rt_usbd_altsetting_add_endpoint(winusb_setting, winusb_device->ep_out);
    rt_usbd_altsetting_add_endpoint(winusb_setting, winusb_device->ep_in);

    /* add the alternate setting to the interface, then set default setting */
    rt_usbd_interface_add_altsetting(winusb_intf, winusb_setting);
    rt_usbd_set_altsetting(winusb_intf, 0);

    /* add the interface to the mass storage function */
    rt_usbd_function_add_interface(func, winusb_intf);

    rt_usbd_os_comp_id_desc_add_os_func_comp_id_desc(device->os_comp_id_desc, &winusb_func_comp_id_desc);
    /* initilize winusb */
    rt_usb_winusb_init(func);
    return func;
}

struct udclass winusb_class =
{
    .rt_usbd_function_create = rt_usbd_function_winusb_create
};

int rt_usbd_winusb_class_register(rt_uint32_t dev_id)
{
    rt_usbd_class_register(&winusb_class, dev_id);
    return 0;
}
INIT_PREV_EXPORT(rt_usbd_winusb_class_register);

#endif