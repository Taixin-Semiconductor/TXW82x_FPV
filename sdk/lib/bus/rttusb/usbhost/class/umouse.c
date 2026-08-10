/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-01-03     Yi Qiu      first version
 */

#include <rtthread.h>
#include <include/rttusb_host.h>
#include "hid.h"


#if defined(RT_USBH_HID) && defined(RT_USBH_HID_MOUSE)

static struct uprotocal mouse_protocal;

static rt_err_t rt_usbh_hid_mouse_callback(void* arg)
{
    rt_uint32_t int1, int2;
    struct uhid* hid;

    hid = (struct uhid*)arg;

    rt_memcpy(&int1, hid->buffer, 4);
    rt_memcpy(&int2, hid->buffer+4, 4);

    if(int1 != 0 || int2 != 0)
    {
        os_printf("key down 0x%x, 0x%x", int1, int2);
    }
    return RT_EOK;
}

static rt_thread_t mouse_thread;
static void mouse_task(void* param)
{
    struct uhintf* intf = (struct uhintf*)param;
    while (1)
    {
        if (rt_usb_hcd_pipe_xfer(intf->device->hcd, ((struct uhid*)intf->user_data)->pipe_in,
            ((struct uhid*)intf->user_data)->buffer, ((struct uhid*)intf->user_data)->pipe_in->ep.wMaxPacketSize,
            USB_TIMEOUT_BASIC) == 0)
        {
            continue;
        }

        rt_usbh_hid_mouse_callback(intf->user_data);
    }
}


static rt_err_t rt_usbh_hid_mouse_init(void* arg)
{
    struct uhintf* intf = (struct uhintf*)arg;

    RT_ASSERT(intf != RT_NULL);

    rt_usbh_hid_set_protocal(intf, 0);

    rt_usbh_hid_set_idle(intf, 0, 0);

    mouse_thread = rt_thread_create("mouse0", mouse_task, intf, 1024, 8, 100);
    rt_thread_startup(mouse_thread);

    os_printf("start usb mouse");

    return RT_EOK;
}

/**
 * This function will define the hid mouse protocal, it will be register to the protocal list.
 *
 * @return the keyboard protocal structure.
 */
uprotocal_t rt_usbh_hid_protocal_mouse(void)
{
    mouse_protocal.pro_id = USB_HID_MOUSE;

    mouse_protocal.init = rt_usbh_hid_mouse_init;
    mouse_protocal.callback = rt_usbh_hid_mouse_callback;

    return &mouse_protocal;
}

#endif

