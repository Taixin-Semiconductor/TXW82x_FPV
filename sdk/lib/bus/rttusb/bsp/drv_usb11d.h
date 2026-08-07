/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author            Notes
 * 2017-10-30     ZYH            the first version
 */
#ifndef __HG_USB11D_H__
#define __HG_USB11D_H__
#include <rtthread.h>
void hg_usb11d_class_driver_register();
int hg_usb11d_register(rt_uint32_t devid);
int hg_usb11d_unregister(rt_uint32_t devid);
int hg_usb11d_recover(rt_uint32_t devid);
#endif
