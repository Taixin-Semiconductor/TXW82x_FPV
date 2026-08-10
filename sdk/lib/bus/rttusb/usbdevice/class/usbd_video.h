/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef USBD_VIDEO_H
#define USBD_VIDEO_H

#include <rtthread.h>
#include "include/usb_video.h"
#include "lib/multimedia/msi.h"

#ifdef __cplusplus
extern "C" {
#endif

enum usbd_video_mode
{
    USBD_VIDEO_CLOSE = 0,
    USBD_VIDEO_OPEN,
};

struct video_entity_info {
    rt_uint8_t bDescriptorSubtype;
    rt_uint8_t bEntityId;
    rt_uint16_t wTerminalType;
};

struct usbd_video_msg
{
    rt_uint8_t * flush_info_addr;
    rt_uint8_t * dma_buff;
    rt_uint8_t   copy_len;
    rt_uint8_t   control_selector;
};

struct usbd_video_process_unit_cur_info
{
    rt_uint32_t wWhiteBalance_Component;
    rt_uint16_t wBacklightCompensation;
    rt_uint16_t wBrightness;
    rt_uint16_t wContrast;
    rt_uint16_t wHue;
    rt_uint16_t wHue_Auto;
    rt_uint16_t wSaturation;
    rt_uint16_t wSharpness;
    rt_uint16_t wGamma;
    rt_uint16_t wGain;
    rt_uint16_t wWhiteBalance_Temprature;
    rt_uint16_t wWhiteBalance_Temprature_Auto;
    rt_uint16_t wWhiteBalance_Component_Auto;
    rt_uint16_t wDigital_Multiplier;
    rt_uint8_t  wDigital_Multiplier_Limit;
    rt_uint8_t  bPowerLineFrequency;
};

typedef void (*uvc_set_resolution)(uint32_t in_weight, uint32_t in_height, uint32_t out_weight, uint32_t out_height, uint32_t en);

struct usbd_video_ops
{
    uvc_set_resolution  mjpeg_set_resolution;
    uvc_set_resolution  h264_set_resolution;   
};

struct usbd_video_priv {
    struct video_probe_and_commit_controls probe __attribute__((aligned(4)));
    struct video_probe_and_commit_controls commit __attribute__((aligned(4)));
    struct usbd_video_process_unit_cur_info process_unit_info;
    rt_uint8_t power_mode;
    rt_uint8_t error_code;
    struct video_entity_info info[3];
    struct rt_messagequeue video_mq;
    struct usbd_video_ops ops;
};

/*
 * uvc class device type
 */

 struct uvc_class_device
 {
     rt_device_t  dev;
     rt_event_t   event;
     rt_uint8_t   open_count;
 
     rt_uint8_t  *buffer;
     rt_uint32_t  buffer_index;
 
     uep_t        ep;
     rt_thread_t  thread;
     struct msi *recv_msi;
     struct msi *cur_msi;
 };

/* Init video interface driver */
static rt_bool_t usbd_uvc_init(struct ufunction *func);
static void usbd_video_init_intf(uint8_t busid, uint32_t dwFrameInterval, uint32_t dwMaxVideoFrameSize, uint32_t dwMaxPayloadTransferSize);
static rt_err_t usbd_video_open(uint8_t busid, ufunction_t func);
static rt_err_t usbd_video_close(uint8_t busid, ufunction_t func);
static uint32_t usbd_video_mjpeg_payload_fill(uint8_t busid, uint8_t *input, uint32_t input_len, uint8_t *output, uint32_t *out_len);

#ifdef __cplusplus
}
#endif

#endif /* USBD_VIDEO_H */
