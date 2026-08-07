#ifndef __CLASS_QUECTEL_H__
#define __CLASS_QUECTEL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <rtthread.h>

enum quectel_state {
    QUECTEL_STATE_UNKNOW,
    QUECTEL_STATE_CHECK_AT_STATUS,
    QUECTEL_STATE_CHECK_SIM_STATUS,
    QUECTEL_STATE_CHECK_CS_STATUS,
    QUECTEL_STATE_CHECK_PS_STATUS,
    QUECTEL_STATE_CHECK_USBNET_STATUS,
    QUECTEL_STATE_CONFIG_USBNET_STATUS,
    QUECTEL_STATE_CONFIG_PDP_CONTEXT,
    QUECTEL_STATE_ACTIVE_PDP_CONTEXT,
    QUECTEL_STATE_CHECK_IP_STATUS,
    QUECTEL_STATE_CONNECT_USB_ADAPTER,
    QUECTEL_STATE_INITIALIZED,
    QUECTEL_STATE_DEACTIVE_PDP_CONTEXT,
    QUECTEL_STATE_POWERDOWN,
};

struct usb_quectel_at {
    struct dev_obj dev;
    void *device;
    struct rt_thread recv_task;
    rt_uint8_t *at_cmd_buff;
    rt_uint32_t state;
    upipe_t pipe_in;
    upipe_t pipe_out;
    rt_uint8_t retry;
};

#ifdef __cplusplus
}
#endif

#endif