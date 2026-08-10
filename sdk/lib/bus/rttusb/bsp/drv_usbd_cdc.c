#include "rtthread.h"
#include "include/rttusb_device.h"
#include "cdc_vcom.h"

int usbd_cdc_putc(struct rt_serial_device *serial, char c)
{
    if (serial) {
        return ((const struct rt_uart_ops *)serial->ops)->putc(serial, c);
    }
    return -RT_ERROR;
}

int usbd_cdc_getc(struct rt_serial_device *serial)
{
    if (serial) {
        return ((const struct rt_uart_ops *)serial->ops)->getc(serial);
    }
    return -RT_ERROR;
}

rt_err_t usbd_cdc_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    if (serial) {
        return ((const struct rt_uart_ops *)serial->ops)->configure(serial, cfg);
    }
    return -RT_ERROR;
}

rt_err_t usbd_cdc_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    if (serial) {
        return ((const struct rt_uart_ops *)serial->ops)->control(serial, cmd, arg);
    }
    return -RT_ERROR;
}

rt_ssize_t usbd_cdc_transmit(struct rt_serial_device *serial, rt_uint8_t *buf, rt_size_t size, rt_uint32_t tx_flag)
{
    
    if (serial) {
        os_printf("%s %d\n",__FUNCTION__,__LINE__);
        return ((const struct rt_uart_ops *)serial->ops)->transmit(serial, buf, size, tx_flag);
    }
    return -RT_ERROR;
}