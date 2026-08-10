#ifndef __CLASS_RNDIS_H__
#define __CLASS_RNDIS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <rtthread.h>
#include "hal/netdev.h"

struct usb_rndis {
    struct netdev ndev;
    netdev_input_cb input_cb;
    void *input_priv;
    void *device;
    struct rt_thread recv_task;
    struct rt_timer keepalive_timer;
    rt_uint8_t *msg_buffer;
    rt_uint8_t *data_buffer;
    //收到的下一个buf的内容，先暂存在这个buf，用于下次接收到后重组
    rt_uint8_t *ts_buffer;
    rt_uint32_t ts_saveLength;
    rt_uint32_t req_id;
    rt_uint8_t mac[6];
    upipe_t pipe_in;
    upipe_t pipe_out;
    upipe_t pipe_int;
    rt_uint8_t link_up: 1, ready: 1, resv: 6;
};

rt_err_t rt_usbh_rndis_run(struct usb_rndis *rndis);
rt_err_t rt_usbh_rndis_stop(struct usb_rndis *rndis);
rt_err_t rt_usbh_host_rndis_attach(struct usb_rndis *rndis);
void rndis_ctrl_recv(void *context);
void rndis_data_recv(void *context);

#ifdef __cplusplus
}
#endif

#endif