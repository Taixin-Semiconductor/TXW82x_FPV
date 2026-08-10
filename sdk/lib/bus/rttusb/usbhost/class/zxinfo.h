#ifndef __CLASS_ZXINFO_H__
#define __CLASS_ZXINFO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <rtthread.h>

#define USB_VENDOR_ID_ZXINFO   0x3361
#define USB_PRODUCT_ID_ZXINFO  0x7B6E // ZX800

enum zxinfo_state {
    ZXINFO_STATE_UNKNOW,
    ZXINFO_STATE_CHECK_AT_STATUS,
    ZXINFO_STATE_CHECK_SIM_STATUS,
    ZXINFO_STATE_CHECK_CS_STATUS,
    ZXINFO_STATE_CHECK_PS_STATUS,
    ZXINFO_STATE_CHECK_USBNET_STATUS,
    ZXINFO_STATE_CONFIG_USBNET_STATUS,
    ZXINFO_STATE_CONFIG_PDP_CONTEXT,
    ZXINFO_STATE_ACTIVE_PDP_CONTEXT,
    ZXINFO_STATE_CHECK_IP_STATUS,
    ZXINFO_STATE_CONNECT_USB_ADAPTER,
    ZXINFO_STATE_INITIALIZED,
    ZXINFO_STATE_DEACTIVE_PDP_CONTEXT,
    ZXINFO_STATE_POWERDOWN,
};

struct usb_zxinfo_at {
    struct dev_obj dev;
    void *device;
    struct rt_thread recv_task;
    rt_uint8_t *at_cmd_buff;
    rt_uint32_t state;
    upipe_t pipe_in;
    upipe_t pipe_out;
    rt_uint8_t retry;
};

void rt_usbh_zxinfo_at_run(void *arg);
void rt_usbh_zxinfo_at_stop(void *arg);

#ifdef __cplusplus
}
#endif

#endif