/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef USBH_VIDEO_H
#define USBH_VIDEO_H

#include "rtthread.h"
#include "include/rttusb_host.h"
#include "include/usb_video.h"
#include "dev/usb/hgusb20_v1_dev_api.h"

#define USBH_VIDEO_PPB                 1     //usbh video 软件双缓存区使能

#define USBH_VIDEO_FORMAT_UNCOMPRESSED 0
#define USBH_VIDEO_FORMAT_MJPEG        1
#define USBH_VIDEO_FORMAT_BASED        2

#define CONFIG_USBHOST_DEV_NAMELEN     16
#define CONFIG_ALTERSETTING_MAXLEN     10

#define USBH_VIDEO_MAX_FORMAT_NUM       5
#define USBH_VIDEO_MAX_FRAME_NUM       15

struct usbh_video_resolution {
    rt_uint16_t wWidth;
    rt_uint16_t wHeight;
    rt_uint32_t dwDefaultFrameInterval;
};

struct usbh_video_format {
    struct usbh_video_resolution frame[USBH_VIDEO_MAX_FRAME_NUM];
    rt_uint8_t format_type;
    rt_uint8_t num_of_frames;
};

struct usbh_videoframe {
    rt_uint8_t *frame_buf;
    rt_uint32_t frame_bufsize;
    rt_uint32_t frame_format;
    rt_uint32_t frame_size;
};

struct usbh_videostreaming {
    struct usbh_videoframe *frame;
    rt_uint32_t frame_format;
    rt_uint32_t bufoffset;
    rt_uint16_t width;
    rt_uint16_t height;
};

struct intf_altersetting_cfg {
    uep_desc_t ep_desc_t;
    rt_uint32_t altersetting_num;
    rt_uint32_t check_use;
};

struct usbh_video {
    //struct usbh_hubport *hport;
    struct uinstance* device;
    char devname[CONFIG_USBHOST_DEV_NAMELEN];
    struct uendpoint_descriptor isoin;  /* ISO IN endpoint */
    struct uendpoint_descriptor isoout; /* ISO OUT endpoint */

    upipe_t pipe_in;
    upipe_t pipe_out;

    rt_uint8_t ctrl_intf; /* interface number */
    rt_uint8_t data_intf; /* interface number */
    rt_uint8_t minor;
    struct video_probe_and_commit_controls probe;
    struct video_probe_and_commit_controls commit;
    rt_uint16_t isoin_mps;
    rt_uint16_t isoout_mps;
    bool is_opened;
    rt_uint8_t current_format;
    rt_uint16_t bcdVDC;
    rt_uint8_t num_of_intf_altsettings;
    rt_uint8_t num_of_formats;
    struct usbh_video_format format[USBH_VIDEO_MAX_FORMAT_NUM];
    rt_uint8_t *rx_buff;
    #if USBH_VIDEO_PPB
    volatile rt_uint8_t usbh_pingpang_flag;
    rt_uint8_t *rx_double_buff;
    #endif
    struct intf_altersetting_cfg intf_altersetting[CONFIG_ALTERSETTING_MAXLEN];
    rt_uint32_t cur_set_altersetting_num;
    rt_uint32_t video_rx_size;
    rt_uint32_t uvc_head;

    void *user_data;
};

#ifdef __cplusplus
extern "C" {
#endif

int usbh_video_get(struct usbh_video *video_class, rt_uint8_t request, rt_uint8_t intf, rt_uint8_t entity_id, rt_uint8_t cs, rt_uint8_t *buf, rt_uint16_t len);
int usbh_video_set(struct usbh_video *video_class, rt_uint8_t request, rt_uint8_t intf, rt_uint8_t entity_id, rt_uint8_t cs, rt_uint8_t *buf, rt_uint16_t len);

int usbh_video_open(struct usbh_video *video_class,
                    rt_uint8_t format_type,
                    rt_uint16_t wWidth,
                    rt_uint16_t wHeight,
                    rt_uint8_t altsetting);
int usbh_video_close(uinst_t device, struct usbh_video *video_class);

void usbh_video_list_info(struct usbh_video *video_class);

void rtt_usbh_video_irq(void * dev, rt_uint8_t ep, uhcd_t hcd);
void rtt_usb11h_video_irq(void * dev, rt_uint8_t ep , uhcd_t hcd);

rt_uint32_t rtt_usbh_video_dev_pipe_manage(rt_uint8_t dev_num, rt_uint8_t alloc_or_free);
rt_uint32_t rtt_usbh_video_user_open(rt_uint8_t dev_num);
rt_uint32_t rtt_usbh_video_user_close(rt_uint8_t dev_num);

void usbh_video_run(struct usbh_video *video_class);
void usbh_video_stop(struct usbh_video *video_class);

ucd_t rt_usbh_class_driver_video(void);

extern void usb_host_enum_finish_init_mjpeg(uint32_t uvc_format);
extern void usb_host_enum_finish_init_h264(uint32_t uvc_format);

#ifdef __cplusplus
}
#endif

#endif /* USBH_VIDEO_H */
