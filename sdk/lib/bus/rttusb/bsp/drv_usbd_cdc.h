#ifndef __DRV_USBD_CDC_H__
#define __DRV_USBD_CDC_H__
#include "rtthread.h"

int  usbd_cdc_putc(struct rt_serial_device *serial, char c);
int usbd_cdc_getc(struct rt_serial_device *serial);
rt_err_t usbd_cdc_configure(struct rt_serial_device *serial, struct serial_configure *cfg);
rt_err_t usbd_cdc_control(struct rt_serial_device *serial, int cmd, void *arg);
rt_ssize_t usbd_cdc_transmit(struct rt_serial_device *serial, rt_uint8_t *buf, rt_size_t size, rt_uint32_t tx_flag);

#endif