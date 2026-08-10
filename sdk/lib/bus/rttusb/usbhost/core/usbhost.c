/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-12-12     Yi Qiu      first version
 * 2021-02-23     Leslie Lee  provide possibility for multi usb host
 */
#include <rtthread.h>
#include <include/rttusb_host.h>

#define USB_HOST_CONTROLLER_NAME      "usbh"

#if defined(RT_USBH_HID_KEYBOARD) || defined(RT_USBH_HID_MOUSE)
#include <hid.h>
#endif

/**
 * This function will initialize the usb host stack, all the usb class driver and
 * host controller driver are also be initialized here.
 *
 * @return none.
 */
rt_err_t rt_usb_host_init(uint32 devid, const char *dev)
{
    ucd_t drv;
    uhcd_t uhc;

    uhc = (uhcd_t)dev_get(devid);
    if(uhc == RT_NULL)
    {
        rt_kprintf("can't find usb host controller\n");
        return -RT_ERROR;
    }

    /* initialize usb hub */
    uhc->devid = devid;
    rt_usbh_hub_init((uhcd_t)uhc, dev);

    /* initialize class driver */
    rt_usbh_class_driver_init();

#ifdef RT_USBH_MSTORAGE
    /* register mass storage class driver */
    drv = rt_usbh_class_driver_storage();
    rt_usbh_class_driver_register(drv);
#endif
#ifdef RT_USBH_HID
    extern ucd_t rt_usbh_class_driver_hid(void);
    /* register mass storage class driver */
    drv = rt_usbh_class_driver_hid();
    rt_usbh_class_driver_register(drv);
#ifdef RT_USBH_HID_MOUSE
    {
        extern uprotocal_t rt_usbh_hid_protocal_mouse(void);
        rt_usbh_hid_protocal_register(rt_usbh_hid_protocal_mouse());
    }
#endif
#ifdef RT_USBH_HID_KEYBOARD
    {
        extern uprotocal_t rt_usbh_hid_protocal_kbd(void);
        rt_usbh_hid_protocal_register(rt_usbh_hid_protocal_kbd());
    }
#endif
#endif
#ifdef RT_USBH_CDC
    extern ucd_t rt_usbh_class_driver_cdc(void);
    drv = rt_usbh_class_driver_cdc();
    rt_usbh_class_driver_register(drv);
#endif
#ifdef RT_USBH_UVC
    extern ucd_t rt_usbh_class_driver_video(void);
    drv = rt_usbh_class_driver_video();
    rt_usbh_class_driver_register(drv);
#endif
#ifdef RT_USBH_UAC
    extern ucd_t rt_usbh_class_driver_audio(void);
    drv = rt_usbh_class_driver_audio();
    rt_usbh_class_driver_register(drv);
#endif
#ifdef RT_USBH_WIRELESS
    extern ucd_t rt_usbh_class_driver_wireless(void);
    drv = rt_usbh_class_driver_wireless();
    rt_usbh_class_driver_register(drv);
#endif
#ifdef RT_USBH_VENDOR_QUECTEL
    extern ucd_t rt_usbh_class_driver_quectel(void);
    drv = rt_usbh_class_driver_quectel();
    rt_usbh_class_driver_register(drv);
#endif
#ifdef RT_USBH_VENDOR_CHINAMOBILE
    extern ucd_t rt_usbh_class_driver_chinamobile(void);
    drv = rt_usbh_class_driver_chinamobile();
    rt_usbh_class_driver_register(drv);
#endif

    /* register hub class driver */
    drv = rt_usbh_class_driver_hub();
    rt_usbh_class_driver_register(drv);

    /* initialize usb host controller */
    rt_device_init(uhc);

    return RT_EOK;
}

rt_err_t rt_usb_host_deinit(uint32 devid)
{
    uhcd_t uhc;

    uhc = (uhcd_t)dev_get(devid);
    if(uhc == RT_NULL)
    {
        rt_kprintf("can't find usb host controller\n");
        return -RT_ERROR;
    }

    rt_usbh_hub_deinit((uhcd_t)uhc);
	return RT_EOK;
}