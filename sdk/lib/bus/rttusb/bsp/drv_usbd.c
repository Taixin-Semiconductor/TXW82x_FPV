/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author            Notes
 * 2017-10-30     ZYH            the first version
 */
#include "drv_usbd.h"
#include <rtthread.h>
#include "include/rttusb_device.h"
#include "dev/usb/hgusb20_v1_dev_api.h"

static struct ep_id _ep_pool[] =
{
   {0x0,  USB_EP_ATTR_TYPE_MASK,     USB_DIR_INOUT,  64, ID_ASSIGNED  },
   {0x1,  USB_EP_ATTR_TYPE_MASK+1,   USB_DIR_IN,   1024, ID_UNASSIGNED},
   {0x1,  USB_EP_ATTR_TYPE_MASK+1,   USB_DIR_OUT,  1024, ID_UNASSIGNED},
   {0x2,  USB_EP_ATTR_TYPE_MASK+1,   USB_DIR_IN,   1024, ID_UNASSIGNED},
   {0x2,  USB_EP_ATTR_TYPE_MASK+1,   USB_DIR_OUT,  1024, ID_UNASSIGNED},
   {0x3,  USB_EP_ATTR_TYPE_MASK+1,   USB_DIR_IN,   1024, ID_UNASSIGNED},
   {0x3,  USB_EP_ATTR_TYPE_MASK+1,   USB_DIR_OUT,  1024, ID_UNASSIGNED},
   {0x4,  USB_EP_ATTR_TYPE_MASK+1,   USB_DIR_IN,   1024, ID_UNASSIGNED},
   {0x4,  USB_EP_ATTR_TYPE_MASK+1,   USB_DIR_OUT,  1024, ID_UNASSIGNED},
   {0x5,  USB_EP_ATTR_TYPE_MASK+1,   USB_DIR_IN,   1024, ID_UNASSIGNED},
   {0x5,  USB_EP_ATTR_TYPE_MASK+1,   USB_DIR_OUT,  1024, ID_UNASSIGNED}, 
   {0x6,  USB_EP_ATTR_TYPE_MASK+1,   USB_DIR_IN,   1024, ID_UNASSIGNED},
   {0x6,  USB_EP_ATTR_TYPE_MASK+1,   USB_DIR_OUT,  1024, ID_UNASSIGNED},   
   {0xFF, USB_EP_ATTR_TYPE_MASK,     USB_DIR_MASK,    0, ID_ASSIGNED  },
};

struct usb_device * _hg_pdc;
static struct udcd _hg_udc;

static rt_err_t _suspend(void);
static rt_err_t _wakeup(void);

static uint32 hal_pcd_bus_irq(uint32 irq, uint32 param1, uint32 param2, uint32 param3)
{
    struct usb_device *p_usb_d = (struct usb_device *)param1;
    struct hgusb20_dev *p_dev = (struct hgusb20_dev *)p_usb_d;
    uint32 ep_num = param2 & 0xF;
    uint32 len    = param3;

    LOG_D("irq:%d %x %d %x\r\n", irq,  param1, param2, param3);
    int32 ret_val = 1;
    switch (irq) {
        case USB_CONNECT:   //7
            rt_usbd_connect_handler(&_hg_udc);
            break;
        case USB_DISCONNECT://8
            rt_usbd_disconnect_handler(&_hg_udc);
            break;
        case USB_DEV_RESET_IRQ://0
            rt_usbd_reset_handler(&_hg_udc);
            break;
        case USB_DEV_SUSPEND_IRQ://1
            _suspend();
            break;
        case USB_DEV_RESUME_IRQ://2
            _wakeup();
            break;
        case USB_DEV_SOF_IRQ://3
            rt_usbd_sof_handler(&_hg_udc);
            break;
        case USB_DEV_CTL_IRQ://4
            ret_val = 0;
            rt_usbd_ep0_setup_handler(&_hg_udc, (struct urequest *)p_dev->usb_ep0_rxbuf);
            break;
        case USB_EP_RX_IRQ://5
            if (ep_num == 0) {
                rt_usbd_ep0_out_handler(&_hg_udc, len);
            } else {
                rt_usbd_ep_out_handler(&_hg_udc, ep_num, len);
            }
            break;
        case USB_EP_TX_IRQ://6
            if (ep_num == 0) {
                rt_usbd_ep0_in_handler(&_hg_udc);
            } else {
                rt_usbd_ep_in_handler(&_hg_udc, 0x80 | ep_num, len);
            }
            break;
        default:
            break;
    }
    return ret_val;
}


static rt_err_t _ep_set_stall(rt_uint8_t address)
{
    LOG_D("rtt stall\r\n");
    hgusb20_dev_stall_ep((struct hgusb20_dev *)_hg_pdc, address);
    return RT_EOK;
}

static rt_err_t _ep_clear_stall(rt_uint8_t address)
{
    hgusb20_dev_clear_ep((struct hgusb20_dev *)_hg_pdc, address);
    return RT_EOK;
}

static rt_err_t _set_address(rt_uint8_t address)
{
    LOG_D("%s %d %d\r\n", __FUNCTION__, address);

    hgusb20_dev_ep0_set_address((struct hgusb20_dev *)_hg_pdc, address);

    return RT_EOK;
}

static rt_err_t _set_config(rt_uint8_t address)
{
    LOG_D("%s %d %d\r\n", __FUNCTION__, address);

    hgusb20_dev_state_config((struct hgusb20_dev *)_hg_pdc);
    return RT_EOK;
}

static rt_err_t _ep_enable(uep_t ep)
{
    LOG_D("%s %d %d\r\n", __FUNCTION__, ep->ep_desc->bEndpointAddress, ep->ep_desc->wMaxPacketSize);

    RT_ASSERT(ep != RT_NULL);
    RT_ASSERT(ep->ep_desc != RT_NULL);
    
    hgusb20_dev_ep_init((struct hgusb20_dev *)_hg_pdc, ep->ep_desc->bEndpointAddress,
                    ep->ep_desc->wMaxPacketSize, ep->ep_desc->bmAttributes);
    return RT_EOK;
}

static rt_err_t _ep_disable(uep_t ep)
{
    LOG_D("%s %d %d\r\n", __FUNCTION__, ep->ep_desc->bEndpointAddress, ep->ep_desc->wMaxPacketSize);

    RT_ASSERT(ep != RT_NULL);
    RT_ASSERT(ep->ep_desc != RT_NULL);
    uint8_t ep_num = ep->ep_desc->bEndpointAddress & 0x7F;
    if (0 == ep) {
        hgusb20_dev_ep0_rx_abort((struct hgusb20_dev *)_hg_pdc);
        hgusb20_dev_ep0_tx_abort((struct hgusb20_dev *)_hg_pdc);
    } else {
        if (ep->ep_desc->bEndpointAddress & USB_DIR_IN) {
            hgusb20_ep_tx_abort((struct hgusb20_dev *)_hg_pdc, ep_num);
        } else {
            hgusb20_ep_rx_abort((struct hgusb20_dev *)_hg_pdc, ep_num);
        }
    }
    
    //hgusb20_dev_ep_deinit(_hg_pdc, ep->ep_desc->bEndpointAddress);
    return RT_EOK;
}

static rt_size_t _ep_read(rt_uint8_t address, void *buffer)
{
    rt_size_t size = hgusb20_ep_get_sie_rx_len((struct hgusb20_dev *)_hg_pdc, (address & 0x7F));
    RT_ASSERT(buffer != RT_NULL);

    LOG_D("%s %d %d\r\n", __FUNCTION__, address, size);
    address &= 0x7F;
    if (address) {
        /* EP1.. data buffer handle */
    } else {
        /* EP0.. data buffer handle */
        os_memcpy(buffer, ((struct hgusb20_dev *)_hg_pdc)->usb_ep0_rxbuf, size);
    }
    
    return size;
}

static rt_size_t _ep_read_prepare(rt_uint8_t address, void *buffer, rt_size_t size)
{
    LOG_D("%s %d %d\r\n", __FUNCTION__, address, size);

    address &= 0x7F;
    if (address) {
        hgusb20_dev_read(_hg_pdc, address, (uint8 *)buffer, size, 0);
    } else {
#if 0    
        if (NULL == buffer) {
            buffer = (void *)(((struct hgusb20_dev *)_hg_pdc)->usb_ep0_rxbuf);
        }
        if (0 == size) {
            size = 64-1;
        }
        hgusb20_dev_ep0_rx((struct hgusb20_dev *)_hg_pdc, buffer, size);
#else
        if (buffer && size) {
            hgusb20_dev_ep0_rx_rtt((struct hgusb20_dev *)_hg_pdc, buffer, size);
        } else {
            /* status not kick rx, the hardware auto control */
        }
#endif
    }
    return size;
}

static rt_size_t _ep_write(rt_uint8_t address, void *buffer, rt_size_t size)
{
    LOG_D("%s %d %d\r\n", __FUNCTION__, address, size);

    address &= 0x7F;
    //printf("address:%x buffer:%x size:%d\n",address,buffer,size);
    if (address && buffer) {
        hgusb20_dev_write(_hg_pdc, address, (uint8 *)buffer, size, 0);
    } else {
        hgusb20_dev_ep0_tx_rtt((struct hgusb20_dev *)_hg_pdc, buffer, size);
    }
    
    return size;
}

static rt_err_t _ep0_send_status(void)
{
    LOG_D("%s\r\n", __FUNCTION__);

    hgusb20_dev_ep0_clrrx_pkt0((struct hgusb20_dev *)_hg_pdc);
    return RT_EOK;
}

static rt_err_t _suspend(void)
{
    LOG_D("%s\r\n", __FUNCTION__);

    return RT_EOK;
}

static rt_err_t _wakeup(void)
{
    LOG_D("%s\r\n", __FUNCTION__);

    return RT_EOK;
}

static rt_err_t _deinit(struct usb_device *usb)
{
    hgusb20_dev_close(usb);
	return RT_EOK;
}

static rt_err_t _init(struct usb_device *usb)
{
    //struct hgusb20_dev *dev = (struct hgusb20_dev *)usb;
    //int32 ret = RET_OK;
    
//    if (usb == NULL) {
//        return NULL;
//    }
//
//    ret = pin_func(dev->dev.dev.dev_id, 1);
//    if (ret) {
////        USB_DEV_ERR_PRINTF("hgsdio20 dev request io failed!\r\n");
//        ASSERT(ret == RET_OK);
//        return ret;
//    }
//
//    //SYSCTRL_REG_OPT(sysctrl_usb20_reset(););
//    sysctrl_usb20_clk_open();
//    hgusb20_dev_hw_init(dev);

    usb_device_open(usb, (struct usb_device_cfg *)NULL);

    //usb_device_ioctl(usb, USB_DEV_IO_CMD_AUTO_TX_NULL_PKT_ENABLE, USB_WIFI_TX_EP, 0);

    usb_device_request_irq(usb, hal_pcd_bus_irq, (uint32)usb);
    
    return RT_EOK;
}


const static struct udcd_ops _udc_ops =
{
    _set_address,
    _set_config,
    _ep_set_stall,
    _ep_clear_stall,
    _ep_enable,
    _ep_disable,
    _ep_read_prepare,
    _ep_read,
    _ep_write,
    _ep0_send_status,
    _suspend,
    _wakeup,
};

/**
 * usage :
 *  1、reg usb device in device.c： hgusb20_dev_attach(HG_USBDEV_DEVID, &usb20_dev);
 *  2、main init in main.c
 *      rt_usbd_class_list_init 
 *      rt_usbd_winusb_class_register
 *
 *
 *      hg_usbd_register(HG_USB_DEV_CONTROLLER_DEVID);
 */


void hg_usbd_class_driver_register()
{
    /* 若需同时注册多个device设备，需定义宏RT_USB_DEVICE_COMPOSITE成复合设备 */
    
    rt_uint32_t devid = HG_USB_DEV_CONTROLLER_DEVID;

    rt_usbd_class_list_init(devid);

    #ifdef RT_USB_DEVICE_VIDEO
        rt_usbd_uvc_device_class_register(devid);
    #endif
    #ifdef RT_USB_DEVICE_AUDIO_MIC
        rt_usbd_uac_mic_class_register(devid);
    #endif
    #ifdef RT_USB_DEVICE_AUDIO_SPEAKER
        audio_speaker_init();
        rt_usbd_uac_speaker_class_register(devid);
    #endif
    #ifdef RT_USB_DEVICE_MSTORAGE
        rt_usbd_msc_class_register(devid);
    #endif
    #ifdef RT_USB_DEVICE_RNDIS
        rt_usbd_rndis_class_register(devid);
    #endif
    #ifdef RT_USB_DEVICE_HID
        rt_usbd_hid_class_register(devid);
    #endif
    #ifdef RT_USB_DEVICE_WINUSB
        rt_usbd_winusb_class_register(devid);
    #endif
    #ifdef RT_USB_DEVICE_CDC
        rt_usbd_vcom_class_register(devid);
    #endif

}


int hg_usbd_register(rt_uint32_t devid)
{
    struct usb_device *usb = (struct usb_device *)dev_get(HG_USBDEV_DEVID);

    _hg_pdc = usb;
    rt_memset((void *)&_hg_udc, 0, sizeof(struct udcd));
    _hg_udc.ops = &_udc_ops;
    
    /* Register endpoint infomation */
    _hg_udc.ep_pool = _ep_pool;
    _hg_udc.ep0.id = &_ep_pool[0];

    _hg_udc.device_is_hs = 1;

    dev_register(devid, (struct dev_obj *)&_hg_udc);

    rt_usb_device_init(devid, "usb20_device");

    _init(usb);
    
    return RT_EOK;
}

int hg_usbd_unregister(rt_uint32_t devid)
{
    struct usb_device *usb = (struct usb_device *)dev_get(HG_USBDEV_DEVID);
    struct dev_obj *udc = (struct dev_obj *)dev_get(devid);

    if(udc && _hg_pdc != RT_NULL){
        printf("%s %d\n",__FUNCTION__,__LINE__);

        // rt_usb_device_deinit(devid);

        _deinit(usb);

        //dev_unregister((struct dev_obj *)&_hg_udc);

        _hg_pdc = RT_NULL;
    }

    
    return RT_EOK;
}

int hg_usbd_recover(rt_uint32_t devid)
{
    struct usb_device *usb = (struct usb_device *)dev_get(HG_USBDEV_DEVID);
    struct dev_obj *udc = (struct dev_obj *)dev_get(devid);
    if(udc){
        _hg_pdc = usb;
        // rt_usbd_core_init();
        _init(usb);
    }
    return 0;
}
