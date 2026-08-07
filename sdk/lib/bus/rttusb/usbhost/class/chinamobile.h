#ifndef __CLASS_CHINAMOBILE_H__
#define __CLASS_CHINAMOBILE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <rtthread.h>

enum chinamobile_state {
    CHINAMOBILE_STATE_UNKNOW,
    CHINAMOBILE_STATE_CHECK_AT_STATUS,
    CHINAMOBILE_STATE_CHECK_SIM_STATUS,
    CHINAMOBILE_STATE_CHECK_PS_STATUS,
    CHINAMOBILE_STATE_CHECK_PDP_CONTEXT,
    CHINAMOBILE_STATE_CHECK_IP_STATUS,
    CHIANMOBILE_STATE_CHECK_BAND,
    CHINAMOBILE_STATE_INITIALIZED,
};

struct usb_chinamobile_at {
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