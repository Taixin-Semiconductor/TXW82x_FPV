/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-10-02     Yi Qiu       first version
 * 2012-12-12     heyuanjie87  change endpoints and function handler
 * 2013-06-25     heyuanjie87  remove SOF mechinism
 * 2013-07-20     Yi Qiu       do more test
 * 2016-02-01     Urey         Fix some error
 * 2021-10-14     mazhiyuan    Fix some error
 */

//#include <rthw.h>
#include <rtthread.h>
#include "include/rttusb_device.h"
#include "hal/isp_tunning.h"
#include "cdc_vcom.h"
#include "osal/mutex.h"
#include "hal/isp.h"


#ifdef RT_USB_DEVICE_CDC

#include "bsp/drv_usbd_cdc.h"

//#define DBG_TAG           "usbdevice_cdc"
//#define DBG_LVL           DBG_INFO
//#include <rtdbg.h>
#if ISP_TUNNING_EN
extern struct isp_tunnning_dev *isp_tunning;
#endif

#define VCOM_INTF_STR_INDEX 5
#ifdef RT_VCOM_TX_TIMEOUT
#define VCOM_TX_TIMEOUT      RT_VCOM_TX_TIMEOUT
#else /*!RT_VCOM_TX_TIMEOUT*/
#define VCOM_TX_TIMEOUT      1000
#endif /*RT_VCOM_TX_TIMEOUT*/

#ifdef RT_CDC_RX_BUFSIZE
#define CDC_RX_BUFSIZE          RT_CDC_RX_BUFSIZE
#else
#define CDC_RX_BUFSIZE          3*1024
#endif
#define CDC_MAX_PACKET_SIZE     64
#define VCOM_DEVICE             "vcom"

#ifdef RT_VCOM_TASK_STK_SIZE
#define VCOM_TASK_STK_SIZE      RT_VCOM_TASK_STK_SIZE
#else /*!RT_VCOM_TASK_STK_SIZE*/
#define VCOM_TASK_STK_SIZE      1024
#endif /*RT_VCOM_TASK_STK_SIZE*/

#ifdef RT_VCOM_TX_USE_DMA
#define VCOM_TX_USE_DMA
#endif /*RT_VCOM_TX_USE_DMA*/

#ifdef RT_VCOM_SERNO
#define _SER_NO RT_VCOM_SERNO
#else /*!RT_VCOM_SERNO*/
#define _SER_NO  "32021919830108"
#endif /*RT_VCOM_SERNO*/

#ifdef RT_VCOM_SER_LEN
#define _SER_NO_LEN RT_VCOM_SER_LEN
#else /*!RT_VCOM_SER_LEN*/
#define _SER_NO_LEN 14 /*rt_strlen("32021919830108")*/
#endif /*RT_VCOM_SER_LEN*/

// rt_align(RT_ALIGN_SIZE)
// static rt_uint8_t vcom_thread_stack[VCOM_TASK_STK_SIZE];
static rt_thread_t vcom_thread = RT_NULL;
static struct ucdc_line_coding line_coding;
static ufunction_t vcom_func = RT_NULL;
static rt_sem_t cdc_sem = RT_NULL;

#define CDC_TX_BUFSIZE    2048
#define CDC_BULKIN_MAXSIZE 512

#define CDC_TX_HAS_DATE         (1 << 0)
#define CDC_TX_HAS_SPACE        (1 << 1)
#define CDC_TX_THREAD_DELETE    (1 << 2)

struct vcom
{
    struct rt_serial_device serial;
    uep_t ep_out;
    uep_t ep_in;
    uep_t ep_cmd;
    rt_bool_t connected;
    rt_bool_t in_sending;
    rt_uint8_t rx_rbp[CDC_RX_BUFSIZE];
    struct rt_ringbuffer rx_ringbuffer;
    rt_uint8_t tx_rbp[CDC_TX_BUFSIZE];
    struct rt_ringbuffer tx_ringbuffer;
    struct rt_event  tx_event;
};

struct vcom_tx_msg
{
    struct rt_serial_device * serial;
    const char *buf;
    rt_size_t size;
};

rt_align(4)
static struct udevice_descriptor dev_desc =
{
    USB_DESC_LENGTH_DEVICE,     //bLength;
    USB_DESC_TYPE_DEVICE,       //type;
    USB_BCD_VERSION,            //bcdUSB;
    USB_CLASS_COMM,              //bDeviceClass;
    0x02,                       //bDeviceSubClass;
    0x00,                       //bDeviceProtocol;
    CDC_MAX_PACKET_SIZE,        //bMaxPacketSize0;
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
    USB_CLASS_COMM,                  //bDeviceClass
    0x02,                           //bDeviceSubClass
    0x00,                           //bDeviceProtocol
    64,                             //bMaxPacketSize0
    0x01,                           //bNumConfigurations
    0,
};

/* communcation interface descriptor */
rt_align(4)
const static struct ucdc_comm_descriptor _comm_desc =
{
#ifdef RT_USB_DEVICE_COMPOSITE
    /* Interface Association Descriptor */
    {
        USB_DESC_LENGTH_IAD,
        USB_DESC_TYPE_IAD,
        USB_DYNAMIC,
        0x02,
        USB_CDC_CLASS_COMM,
        USB_CDC_SUBCLASS_ACM,
        USB_CDC_PROTOCOL_V25TER,
        0x00,
    },
#endif
    /* Interface Descriptor */
    {
        USB_DESC_LENGTH_INTERFACE,
        USB_DESC_TYPE_INTERFACE,
        USB_DYNAMIC,
        0x00,
        0x01,
        USB_CDC_CLASS_COMM,
        USB_CDC_SUBCLASS_ACM,
        USB_CDC_PROTOCOL_V25TER,
#ifdef RT_USB_DEVICE_COMPOSITE
        VCOM_INTF_STR_INDEX,
#else
        0,
#endif
    },
    /* Header Functional Descriptor */
    {
        0x05,
        USB_CDC_CS_INTERFACE,
        USB_CDC_SCS_HEADER,
        0x0110,
    },
    /* Call Management Functional Descriptor */
    {
        0x05,
        USB_CDC_CS_INTERFACE,
        USB_CDC_SCS_CALL_MGMT,
        0x00,
        USB_DYNAMIC,
    },
    /* Abstract Control Management Functional Descriptor */
    {
        0x04,
        USB_CDC_CS_INTERFACE,
        USB_CDC_SCS_ACM,
        0x02,
    },
    /* Union Functional Descriptor */
    {
        0x05,
        USB_CDC_CS_INTERFACE,
        USB_CDC_SCS_UNION,
        USB_DYNAMIC,
        USB_DYNAMIC,
    },
    /* Endpoint Descriptor */
    {
        USB_DESC_LENGTH_ENDPOINT,
        USB_DESC_TYPE_ENDPOINT,
        USB_DYNAMIC | USB_DIR_IN,
        USB_EP_ATTR_INT,
        0x08,
        0xFF,
    },
};

/* data interface descriptor */
rt_align(4)
const static struct ucdc_data_descriptor _data_desc =
{
    /* interface descriptor */
    {
        USB_DESC_LENGTH_INTERFACE,
        USB_DESC_TYPE_INTERFACE,
        USB_DYNAMIC,
        0x00,
        0x02,
        USB_CDC_CLASS_DATA,
        0x00,
        0x00,
        0x00,
    },
    /* endpoint, bulk out */
    {
        USB_DESC_LENGTH_ENDPOINT,
        USB_DESC_TYPE_ENDPOINT,
        USB_DYNAMIC | USB_DIR_OUT,
        USB_EP_ATTR_BULK,
        USB_DYNAMIC,
        0x00,
    },
    /* endpoint, bulk in */
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
static char serno[_SER_NO_LEN + 1] = {'\0'};
rt_weak rt_err_t vcom_get_stored_serno(char *serno, int size);

rt_err_t vcom_get_stored_serno(char *serno, int size)
{
    return -RT_ERROR;
}
rt_align(4)
const static char* _ustring[] =
{
    "Language",
    "RT-Thread Team.",
    "RTT Virtual Serial",
    serno,
    "Configuration",
    "Interface",
};
static void rt_usb_vcom_init(struct ufunction *func);
static void rt_usbd_vcom_deinit(struct ufunction *func);

static void _vcom_reset_state(ufunction_t func)
{
    struct vcom* data;
    rt_base_t level;

    RT_ASSERT(func != RT_NULL);

    data = (struct vcom*)func->user_data;

    level = rt_hw_interrupt_disable();
    data->connected = RT_FALSE;
    data->in_sending = RT_FALSE;
    /*rt_kprintf("reset USB serial\n", cnt);*/
    rt_hw_interrupt_enable(level);
}

/**
 * This function will handle cdc bulk in endpoint request.
 *
 * @param func the usb function object.
 * @param size request size.
 *
 * @return RT_EOK.
 */
static rt_err_t _ep_in_handler(ufunction_t func, rt_size_t size)
{
    struct vcom *data;
    rt_size_t request_size;

    RT_ASSERT(func != RT_NULL);

    data = (struct vcom*)func->user_data;
    request_size = data->ep_in->request.size;
    os_printf("cdc _ep_in_handler %d\n", request_size);
    rt_sem_release(cdc_sem);
#if ISP_TUNNING_EN
    os_sema_up(&isp_tunning->usb_write_sema);
#else
    if ((request_size != 0) && ((request_size % EP_MAXPACKET(data->ep_in)) == 0))
    {
        /* don't have data right now. Send a zero-length-packet to
         * terminate the transaction.
         *
         * FIXME: actually, this might not be the right place to send zlp.
         * Only the rt_device_write could know how much data is sending. */
        data->in_sending = RT_TRUE;

        data->ep_in->request.buffer = RT_NULL;
        data->ep_in->request.size = 0;
        data->ep_in->request.req_type = UIO_REQUEST_WRITE;
        //rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
        return RT_EOK;
    }

#endif

    return RT_EOK;
}

rt_ssize_t cdc_usb_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    struct vcom *data = (struct vcom*)vcom_func->user_data;
    if (vcom_func->device->state != USB_STATE_CONFIGURED)
    {
        return 0;
    }
    data->in_sending              = RT_TRUE;
    data->ep_in->request.buffer   = (void *)buffer;
    data->ep_in->request.size     = size;
    data->ep_in->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(vcom_func->device, data->ep_in, &data->ep_in->request);
#if ISP_TUNNING_EN
    os_sema_down(&isp_tunning->usb_write_sema, -1);
#endif
    return size;
}

/**
 * This function will handle cdc bulk out endpoint request.
 *
 * @param func the usb function object.
 * @param size request size.
 *
 * @return RT_EOK.
 */
static rt_err_t _ep_out_handler(ufunction_t func, rt_size_t size)
{
    struct vcom *data;

    RT_ASSERT(func != RT_NULL);

    //os_printf("cdc _ep_out_handler %d\n", size);

    data = (struct vcom*)func->user_data;
    
#if ISP_TUNNING_EN
    uint16 head         = *((uint16 *)&data->ep_out->buffer[0]);
    uint16 cmd_data     = *((uint16 *)&data->ep_out->buffer[6]);
    uint16 crc_cal      = hw_crc(CRC_TYPE_CRC16_MODBUS, data->ep_out->buffer, (8 + (cmd_data<<2)));
	if ((head == isp_tunning->tunning_head) && (crc_cal == *((uint16 *)&data->ep_out->buffer[8 + (cmd_data<<2)])))
	{
        isp_tunning->cmd_head    = head;
        isp_tunning->cmd_num     = *(uint16 *)&data->ep_out->buffer[2];
        isp_tunning->cmd_channel = *(uint16 *)&data->ep_out->buffer[4];
        isp_tunning->cmd_size    = cmd_data << 2;
        isp_tunning->p_data      = (uint32 *)os_zalloc(isp_tunning->cmd_size);
        if (isp_tunning->p_data)
        {
            os_memcpy((void *)isp_tunning->p_data , (void *)&data->ep_out->buffer[8], isp_tunning->cmd_size);
            os_sema_up(&isp_tunning->usb_cmd_sema);
        } else {
            LOG_D(KERN_ERR"isp_tunning malloc size : %d err", isp_tunning->cmd_size);
        }
	} else {
        os_printf("isp_tunning head : %04x rece_head : %4x cmd_dat : %04x crc : %4x rece_crc : %04x\r\n", isp_tunning->tunning_head, head, cmd_data, crc_cal, *((uint16 *)&data->ep_out->buffer[8 + (cmd_data<<2)]));
    }
    data->ep_out->request.buffer = data->ep_out->buffer;
    data->ep_out->request.size = CDC_RX_BUFSIZE;
    data->ep_out->request.req_type = UIO_REQUEST_READ_BEST;
    rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);
#else
    rt_base_t level = rt_hw_interrupt_disable();

    rt_ringbuffer_put(&data->rx_ringbuffer, data->ep_out->buffer, size);
    rt_hw_interrupt_enable(level);

    //收发回环测试
    struct rt_serial_device *serial = (struct rt_serial_device*)dev_get(HG_USB_DEV_CDC_DEVID);
    usbd_cdc_transmit(serial, data->ep_out->buffer, size, 0);

#if 1
    int i = 0;
    for(i = 0; i < size; i++)
    {
        printf("%x ",data->ep_out->buffer[i]);
    }
#endif
    data->ep_out->request.buffer = data->ep_out->buffer;
    data->ep_out->request.size = EP_MAXPACKET(data->ep_out);
    data->ep_out->request.req_type = UIO_REQUEST_READ_BEST;
    rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);
#endif

    return RT_EOK;
}

/**
 * This function will handle cdc interrupt in endpoint request.
 *
 * @param device the usb device object.
 * @param size request size.
 *
 * @return RT_EOK.
 */
static rt_err_t _ep_cmd_handler(ufunction_t func, rt_size_t size)
{
    RT_ASSERT(func != RT_NULL);

    LOG_D("_ep_cmd_handler");

    return RT_EOK;
}

/**
 * This function will handle cdc_get_line_coding request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _cdc_get_line_coding(udevice_t device, ureq_t setup)
{
    struct ucdc_line_coding data;
    rt_uint16_t size;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    LOG_D("_cdc_get_line_coding");

    data.dwDTERate = 115200;
    data.bCharFormat = 0;
    data.bDataBits = 8;
    data.bParityType = 0;
    size = setup->wLength > 7 ? 7 : setup->wLength;

    rt_usbd_ep0_write(device, (void*)&line_coding, size);

    return RT_EOK;
}

static rt_err_t _cdc_set_line_coding_callback(udevice_t device, rt_size_t size)
{
    LOG_D("_cdc_set_line_coding_callback");

    os_printf("baud:%d,format:%d,Parity:%d,bits:%d\n",line_coding.dwDTERate,\
    (rt_uint32_t)line_coding.bCharFormat,(rt_uint32_t)line_coding.bParityType,(rt_uint32_t)line_coding.bDataBits);

    dcd_ep0_send_status(device->dcd);

    return RT_EOK;
}

/**
 * This function will handle cdc_set_line_coding request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _cdc_set_line_coding(udevice_t device, ureq_t setup)
{
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    LOG_D("_cdc_set_line_coding");

    rt_usbd_ep0_read(device, (void*)&line_coding, sizeof(struct ucdc_line_coding),
        _cdc_set_line_coding_callback);

    return RT_EOK;
}

/**
 * This function will handle cdc interface request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _interface_handler(ufunction_t func, ureq_t setup)
{
    struct vcom *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    data = (struct vcom*)func->user_data;

    switch(setup->bRequest)
    {
    case CDC_SEND_ENCAPSULATED_COMMAND:
        break;
    case CDC_GET_ENCAPSULATED_RESPONSE:
        break;
    case CDC_SET_COMM_FEATURE:
        break;
    case CDC_GET_COMM_FEATURE:
        break;
    case CDC_CLEAR_COMM_FEATURE:
        break;
    case CDC_SET_LINE_CODING:
        _cdc_set_line_coding(func->device, setup);
        break;
    case CDC_GET_LINE_CODING:
        _cdc_get_line_coding(func->device, setup);
        break;
    case CDC_SET_CONTROL_LINE_STATE:
        data->connected = (setup->wValue & 0x01) > 0?RT_TRUE:RT_FALSE;    //主机端设置DTR才可接收数据
        LOG_D("vcom state:%d ", data->connected);
        dcd_ep0_send_status(func->device->dcd);
        break;
    case CDC_SEND_BREAK:
        break;
    default:
        rt_kprintf("unknown cdc request\n",setup->request_type);
        return -RT_ERROR;
    }

    return RT_EOK;
}

/**
 * This function will run cdc function, it will be called on handle set configuration request.
 *
 * @param func the usb function object.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _function_enable(ufunction_t func)
{
    struct vcom *data;

    RT_ASSERT(func != RT_NULL);

    os_printf("cdc function enable");

    rt_usb_vcom_init(func);

    _vcom_reset_state(func);

    data = (struct vcom*)func->user_data;
    data->ep_out->buffer = rt_malloc(CDC_RX_BUFSIZE);
    RT_ASSERT(data->ep_out->buffer != RT_NULL);

    data->ep_out->request.buffer = data->ep_out->buffer;
    data->ep_out->request.size   = CDC_RX_BUFSIZE;//EP_MAXPACKET(data->ep_out);

    data->ep_out->request.req_type = UIO_REQUEST_READ_BEST;
    rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);

    rt_event_send(&data->tx_event, CDC_TX_HAS_SPACE);

    return RT_EOK;
}

/**
 * This function will stop cdc function, it will be called on handle set configuration request.
 *
 * @param func the usb function object.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _function_disable(ufunction_t func)
{
    struct vcom *data;

    RT_ASSERT(func != RT_NULL);

    os_printf("cdc function disable");

    rt_usbd_vcom_deinit(func);

    _vcom_reset_state(func);

    data = (struct vcom*)func->user_data;

    if(data->ep_out->buffer != RT_NULL)
    {
        rt_free(data->ep_out->buffer);
        data->ep_out->buffer = RT_NULL;
    }

    return RT_EOK;
}

static struct ufunction_ops ops =
{
    _function_enable,
    _function_disable,
    RT_NULL,
};

/**
 * This function will configure cdc descriptor.
 *
 * @param comm the communication interface number.
 * @param data the data interface number.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _cdc_descriptor_config(ucdc_comm_desc_t comm,
    rt_uint8_t cintf_nr, ucdc_data_desc_t data, rt_uint8_t dintf_nr, rt_uint8_t device_is_hs)
{
    comm->call_mgmt_desc.data_interface = dintf_nr;
    comm->union_desc.master_interface = cintf_nr;
    comm->union_desc.slave_interface0 = dintf_nr;
#ifdef RT_USB_DEVICE_COMPOSITE
    comm->iad_desc.bFirstInterface = cintf_nr;
#endif

    data->ep_in_desc.wMaxPacketSize = device_is_hs ? 512 : 64;
    data->ep_out_desc.wMaxPacketSize = device_is_hs ? 512 : 64;

    return RT_EOK;
}

/**
 * This function will create a cdc function instance.
 *
 * @param device the usb device object.
 *
 * @return RT_EOK on successful.
 */
ufunction_t rt_usbd_function_cdc_create(udevice_t device)
{
    ufunction_t func;
    struct vcom* data;
    uintf_t intf_comm, intf_data;
    ualtsetting_t comm_setting, data_setting;
    ucdc_data_desc_t data_desc;
    ucdc_comm_desc_t comm_desc;

    /* parameter check */
    RT_ASSERT(device != RT_NULL);

    rt_memset(serno, 0, _SER_NO_LEN + 1);
    if(vcom_get_stored_serno(serno, _SER_NO_LEN) != RT_EOK)
    {
        rt_memset(serno, 0, _SER_NO_LEN + 1);
        rt_memcpy(serno, _SER_NO, rt_strlen(_SER_NO));
    }
#ifdef RT_USB_DEVICE_COMPOSITE
    rt_usbd_device_set_interface_string(device, VCOM_INTF_STR_INDEX, _ustring[2]);
#else
    /* set usb device string description */
    rt_usbd_device_set_string(device, _ustring);
#endif
    /* create a cdc function */
    func = rt_usbd_function_new(device, &dev_desc, &ops);
    vcom_func = func;

    /* support HS */
    rt_usbd_device_set_qualifier(device, &dev_qualifier);

    /* allocate memory for cdc vcom data */
    data = (struct vcom*)rt_malloc(sizeof(struct vcom));
    RT_ASSERT(data != RT_NULL);
    rt_memset(data, 0, sizeof(struct vcom));
    func->user_data = (void*)data;


    /* create a cdc communication interface and a cdc data interface */
    intf_comm = rt_usbd_interface_new(device, _interface_handler);
    intf_data = rt_usbd_interface_new(device, _interface_handler);
    func->intf[0] = intf_comm;
    func->intf[1] = intf_data;

    /* create a communication alternate setting and a data alternate setting */
    comm_setting = rt_usbd_altsetting_new(sizeof(struct ucdc_comm_descriptor));
    data_setting = rt_usbd_altsetting_new(sizeof(struct ucdc_data_descriptor));
    func->setting[0] = comm_setting;
    func->setting[1] = data_setting;

    /* config desc in alternate setting */
    rt_usbd_altsetting_config_descriptor(comm_setting, &_comm_desc,
                                         (rt_off_t)&((ucdc_comm_desc_t)0)->intf_desc);
    rt_usbd_altsetting_config_descriptor(data_setting, &_data_desc, 0);
    /* configure the cdc interface descriptor */
    _cdc_descriptor_config(comm_setting->desc, intf_comm->intf_num, data_setting->desc, intf_data->intf_num, device->dcd->device_is_hs);

    /* create a command endpoint */
    comm_desc = (ucdc_comm_desc_t)comm_setting->desc;
    data->ep_cmd = rt_usbd_endpoint_new(&comm_desc->ep_desc, _ep_cmd_handler);
    func->ep[0] = data->ep_cmd;

    /* add the command endpoint to the cdc communication interface */
    rt_usbd_altsetting_add_endpoint(comm_setting, data->ep_cmd);

    /* add the communication alternate setting to the communication interface,
       then set default setting of the interface */
    rt_usbd_interface_add_altsetting(intf_comm, comm_setting);
    rt_usbd_set_altsetting(intf_comm, 0);

    /* add the communication interface to the cdc function */
    rt_usbd_function_add_interface(func, intf_comm);

    /* create a bulk in and a bulk endpoint */
    data_desc = (ucdc_data_desc_t)data_setting->desc;
    data->ep_out = rt_usbd_endpoint_new(&data_desc->ep_out_desc, _ep_out_handler);
    data->ep_in = rt_usbd_endpoint_new(&data_desc->ep_in_desc, _ep_in_handler);
    func->ep[1] = data->ep_out;
    func->ep[2] = data->ep_in;

    /* add the bulk out and bulk in endpoints to the data alternate setting */
    rt_usbd_altsetting_add_endpoint(data_setting, data->ep_in);
    rt_usbd_altsetting_add_endpoint(data_setting, data->ep_out);

    /* add the data alternate setting to the data interface
            then set default setting of the interface */
    rt_usbd_interface_add_altsetting(intf_data, data_setting);
    rt_usbd_set_altsetting(intf_data, 0);

    /* add the cdc data interface to cdc function */
    rt_usbd_function_add_interface(func, intf_data);

    return func;
}

void rt_usbd_function_cdc_delete()
{
    struct vcom* data;
    os_printf("%s %d\n",__FUNCTION__,__LINE__);
    
    if(vcom_func == RT_NULL)
    {
        LOG_D("vcom_func is NULL\n");
        return;
    }
    
    data = (struct vcom*)vcom_func->user_data;


    
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
    
    if(vcom_func != RT_NULL)
    {
        rt_usbd_function_release(vcom_func);
        vcom_func = RT_NULL;
    }
}

/**
* HG_USB_DEV_CDC_DEVID device
*/
#if 1
static rt_err_t _vcom_configure(struct rt_serial_device *serial,
                                struct serial_configure *cfg)
{
    os_printf("%s %d\n",__FUNCTION__,__LINE__);
    return RT_EOK;
}

static rt_err_t _vcom_control(struct rt_serial_device *serial,
                              int cmd, void *arg)
{
    struct ufunction *func;
    struct vcom *data;

    func = (struct ufunction*)serial->func;
    data = (struct vcom*)func->user_data;

    os_printf("%s %d\n",__FUNCTION__,__LINE__);

    switch (cmd)
    {
        case USBD_VCOM_CTRL_GET:

            break;
        case USBD_VCOM_CTRL_SET:

            break;

        case USBD_VCOM_CTRL_CONNECTED:
            (*(rt_bool_t*)arg) = data->connected;
            break;
        default: 
            break;
    }

    return RT_EOK;
}

static int _vcom_getc(struct rt_serial_device *serial)
{
    int result;
    rt_uint8_t ch;
    rt_base_t level;
    struct ufunction *func;
    struct vcom *data;
    os_printf("%s %d\n",__FUNCTION__,__LINE__);
    func = (struct ufunction*)serial->func;
    data = (struct vcom*)func->user_data;

    result = -1;

    level = rt_hw_interrupt_disable();

    if(rt_ringbuffer_getchar(&data->rx_ringbuffer, &ch) != 0)
    {
        result = ch;
    }

    rt_hw_interrupt_enable(level);

    return result;
}

static rt_ssize_t _vcom_rb_block_put(struct vcom *data, const rt_uint8_t *buf, rt_size_t size)
{
    rt_base_t level;
    rt_size_t   put_len = 0;
    rt_size_t   w_ptr = 0;
    rt_uint32_t res;
    rt_size_t   remain_size = size;
    LOG_D("%s %d buf:%x\n",__FUNCTION__,__LINE__,buf);
    while (remain_size)
    {
        LOG_D("rb->rops:%d rb->wops:%d rb->qsize:%d\n",*(&data->tx_ringbuffer.rpos),*(&data->tx_ringbuffer.wpos),*(&data->tx_ringbuffer.qsize));
        level = rt_hw_interrupt_disable();
        put_len = rt_ringbuffer_put(&data->tx_ringbuffer, (const rt_uint8_t *)&buf[w_ptr], remain_size);
        rt_hw_interrupt_enable(level);
        LOG_D("rb->rops:%d rb->wops:%d rb->qsize:%d\n",*(&data->tx_ringbuffer.rpos),*(&data->tx_ringbuffer.wpos),*(&data->tx_ringbuffer.qsize));
        w_ptr += put_len;
        remain_size -= put_len;
        if (put_len == 0)
        {
            os_printf("put_len:%d tx_ringbuffer put failed\n",put_len);
        }
        else
        {
            printf("_vcom rb block\n");
            rt_event_recv(&data->tx_event, CDC_TX_HAS_SPACE,
            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
            VCOM_TX_TIMEOUT, &res);
            if((res & CDC_TX_HAS_SPACE)){
                rt_event_send(&data->tx_event, CDC_TX_HAS_DATE);
                os_printf("vcom rb %d\n", size);
            }

        }

    }

    return size;
}

static rt_ssize_t _vcom_tx(struct rt_serial_device *serial, rt_uint8_t *buf, rt_size_t size, rt_uint32_t direction)
{
    struct ufunction *func;
    struct vcom *data;
    rt_uint32_t send_size = 0;
    rt_size_t ptr = 0;
    //rt_uint8_t crlf[2] = {'\r', '\n',};

    func = (struct ufunction*)serial->func;
    data = (struct vcom*)func->user_data;

    RT_ASSERT(serial != RT_NULL);
    RT_ASSERT(buf != RT_NULL);

    os_printf("%s %d buf:%x\n",__FUNCTION__,__LINE__,buf);

    if (data->connected)
    {
        while(send_size < size)
        {
            while(ptr < size && buf[ptr] != '\n')      //换行符分包
            {
                ptr++;
            }

            if(ptr < size)
            {
                send_size += _vcom_rb_block_put(data, (const rt_uint8_t *)&buf[send_size], ptr - send_size);
                //_vcom_rb_block_put(data, crlf, 2);
                //send_size++;
                ptr++;
                os_sleep_ms(10);
                
            }
            else if (ptr == size)
            {
                send_size += _vcom_rb_block_put(data, (const rt_uint8_t *)&buf[send_size], ptr - send_size);
                os_sleep_ms(10);
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        /* recover dataqueue resources */
		os_printf("%s %d data disconnect\n",__FUNCTION__,__LINE__);
    }

    return size;
}
static int _vcom_putc(struct rt_serial_device *serial, char c)
{
    rt_base_t level;
    struct ufunction *func;
    struct vcom *data;

    func = (struct ufunction*)serial->func;
    data = (struct vcom*)func->user_data;

    os_printf("%s %d\n",__FUNCTION__,__LINE__);

    RT_ASSERT(serial != RT_NULL);

    if (data->connected)
    {
        if(c == '\n')
        {
            level = rt_hw_interrupt_disable();
            rt_ringbuffer_putchar_force(&data->tx_ringbuffer, '\r');
            rt_hw_interrupt_enable(level);
            rt_event_send(&data->tx_event, CDC_TX_HAS_DATE);
        }
        level = rt_hw_interrupt_disable();
        rt_ringbuffer_putchar_force(&data->tx_ringbuffer, c);
        rt_hw_interrupt_enable(level);
        rt_event_send(&data->tx_event, CDC_TX_HAS_DATE);
    }

    return 1;
}

static const struct rt_uart_ops usb_vcom_ops =
{
    _vcom_configure,
    _vcom_control,
    _vcom_putc,
    _vcom_getc,
    _vcom_tx,
};
#endif

/* Vcom Tx Thread */
static void vcom_tx_thread_entry(void* parameter)
{
    rt_base_t level;
    rt_uint32_t res;
    rt_uint32_t ret = 0;
    struct ufunction *func = (struct ufunction *)parameter;
    struct vcom *data = (struct vcom*)func->user_data;
    rt_uint8_t ch[CDC_BULKIN_MAXSIZE];

    cdc_sem = rt_sem_create("cdc_sem", 1, 0);

    while (1)
    {
        if
        (
            (rt_event_recv(&data->tx_event, CDC_TX_HAS_DATE | CDC_TX_THREAD_DELETE,
                    RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                    RT_WAITING_FOREVER, &res) != RT_EOK)
        )
        {
            continue;
        }
        if(res & CDC_TX_THREAD_DELETE)
        {
            os_printf("vcom tx thread delete\n");
            break;
        }
        
        if(!(res & CDC_TX_HAS_DATE))
        {
            continue;
        }

        while((rt_uint32_t)rt_ringbuffer_data_len(&data->tx_ringbuffer))
        {
            LOG_D("ringbuffer:%x len:%d rb->rops:%d rb->wops:%d rb->qsize:%d\n",&data->tx_ringbuffer,(rt_uint32_t)rt_ringbuffer_data_len(&data->tx_ringbuffer),*(&data->tx_ringbuffer.rpos),*(&data->tx_ringbuffer.wpos),*(&data->tx_ringbuffer.qsize));
            level = rt_hw_interrupt_disable();
            res = rt_ringbuffer_get(&data->tx_ringbuffer, ch, CDC_BULKIN_MAXSIZE);
            rt_hw_interrupt_enable(level);
            LOG_D("rb->rops:%d rb->wops:%d rb->qsize:%d\n",*(&data->tx_ringbuffer.rpos),*(&data->tx_ringbuffer.wpos),*(&data->tx_ringbuffer.qsize));
            if(!res)
            {
                continue;
            }

            if (!data->connected)
            {
                continue;
            }

            ret = 0;
            ret = rt_sem_take(cdc_sem, -1);         //等待串口发送完成
            printf("ret:%d\n",ret);
            data->ep_in->request.buffer     = ch;
            data->ep_in->request.size       = res;
            data->ep_in->request.req_type   = UIO_REQUEST_WRITE;

            rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);

            rt_event_send(&data->tx_event, CDC_TX_HAS_SPACE);
            
        }
    }
    
    if(cdc_sem != RT_NULL)
    {
        rt_sem_delete(cdc_sem);
        cdc_sem = RT_NULL;        
    }
    
}

static void rt_usb_vcom_init(struct ufunction *func)
{
    rt_err_t result = RT_EOK;
    struct serial_configure config;
    struct vcom *data = (struct vcom*)func->user_data;

    /* initialize ring buffer */
    rt_ringbuffer_init(&data->rx_ringbuffer, data->rx_rbp, CDC_RX_BUFSIZE);
    rt_ringbuffer_init(&data->tx_ringbuffer, data->tx_rbp, CDC_TX_BUFSIZE);

    rt_event_init(&data->tx_event, "vcom", RT_IPC_FLAG_FIFO);     

    config.baud_rate    = BAUD_RATE_115200;
    config.data_bits    = DATA_BITS_8;
    config.stop_bits    = STOP_BITS_1;
    config.parity       = PARITY_NONE;
    config.bit_order    = BIT_ORDER_LSB;
    config.invert       = NRZ_NORMAL;

    data->serial.ops        = &usb_vcom_ops;
    data->serial.serial_rx  = RT_NULL;
    data->serial.config     = config;
    data->serial.func       = func;

    dev_register(HG_USB_DEV_CDC_DEVID, (struct dev_obj *)&data->serial);

#if 1
    /* init usb device thread */
    if(vcom_thread == RT_NULL)
    {
        vcom_thread = rt_thread_create("vcom",
                                        vcom_tx_thread_entry, func,
                                        VCOM_TASK_STK_SIZE,
                                        OS_TASK_PRIORITY_NORMAL, 20);        
    }

    if(vcom_thread != RT_NULL)
    {
        result = rt_thread_startup(vcom_thread);    
    }  
    RT_ASSERT(result == RT_EOK);
#endif
}

static void rt_usbd_vcom_deinit(struct ufunction *func)
{
    struct vcom *data = (struct vcom*)func->user_data;

    rt_event_send(&data->tx_event, CDC_TX_THREAD_DELETE);

    dev_unregister((struct dev_obj *)&data->serial);

    rt_event_detach(&data->tx_event);

    rt_ringbuffer_destroy(&data->rx_ringbuffer);
    rt_ringbuffer_destroy(&data->tx_ringbuffer);

    if(vcom_thread != RT_NULL)
    {
        rt_thread_delete(vcom_thread);
        vcom_thread = RT_NULL;
    }

    if(cdc_sem != RT_NULL)
    {
        rt_sem_delete(cdc_sem);
        cdc_sem = RT_NULL;        
    }
}

struct udclass vcom_class =
{
    .rt_usbd_function_create = rt_usbd_function_cdc_create,
    .rt_usbd_function_delete = rt_usbd_function_cdc_delete,
};

//注册接口
int rt_usbd_vcom_class_register(rt_uint32_t dev_id)
{
    rt_usbd_class_register(&vcom_class, dev_id);
    return 0;
}

int rt_usbd_vcom_class_unregister(rt_uint32_t dev_id)
{
    rt_usbd_class_driver_unregister(&vcom_class, dev_id);
    return 0;
}

INIT_PREV_EXPORT(rt_usbd_vcom_class_register);

#endif
