#ifndef __CLASS_YUGE_H__
#define __CLASS_YUGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <rtthread.h>

#define USB_VENDOR_ID_YUGE   0x19D1
#define USB_PRODUCT_ID_YUGE  0x1003 // YM310 X09

enum quectel_state {
    YUGE_STATE_UNKNOW,
    YUGE_STATE_CHECK_AT_STATUS,
    YUGE_STATE_CHECK_SIM_STATUS,
    YUGE_STATE_CHECK_CS_STATUS,
    YUGE_STATE_CHECK_PS_STATUS,
    YUGE_STATE_CHECK_USBNET_STATUS,
    YUGE_STATE_CONFIG_USBNET_STATUS,
    YUGE_STATE_CONFIG_PDP_CONTEXT,
    YUGE_STATE_ACTIVE_PDP_CONTEXT,
    YUGE_STATE_CHECK_IP_STATUS,
    YUGE_STATE_CONNECT_USB_ADAPTER,
    YUGE_STATE_INITIALIZED,
    YUGE_STATE_DEACTIVE_PDP_CONTEXT,
    YUGE_STATE_POWERDOWN,
};

struct usb_yuge_at {
    struct dev_obj dev;
    void *device;
    struct rt_thread recv_task;
    rt_uint8_t *at_cmd_buff;
    rt_uint32_t state;
    upipe_t pipe_in;
    upipe_t pipe_out;
    rt_uint8_t retry;
};

void rt_usbh_yuge_at_run(void *arg);
void rt_usbh_yuge_at_stop(void *arg);

#ifdef __cplusplus
}
#endif

#endif