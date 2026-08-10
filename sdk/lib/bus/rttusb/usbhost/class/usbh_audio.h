/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef USBH_AUDIO_H
#define USBH_AUDIO_H

#include "rtthread.h"
#include "include/rttusb_host.h"
#include "uaudioreg.h"
#include "dev/usb/hgusb20_v1_dev_api.h"
#include "dev/usb/usb11_v0/hgusb11_v0_host_api.h"
#include "lib/audio/uac/uac_host.h"
#include "audio_usbh_msi.h"

#ifndef CONFIG_USBHOST_MAX_AUDIO_CLASS
#define CONFIG_USBHOST_MAX_AUDIO_CLASS 1
#endif

#define CONFIG_USBHOST_MAX_INTF_ALTSETTINGS 8
#define CONFIG_USBHOST_DEV_NAMELEN 16

struct usbh_audio_format_type {
    rt_uint8_t channels;
    rt_uint8_t format_type;
    rt_uint8_t bitresolution;
    rt_uint8_t sampfreq_num;
    rt_uint32_t sampfreq[3];
};

/**
 * bSourceID in feature_unit = input_terminal_id
 * bSourceID in output_terminal = feature_unit_id
 * terminal_link_id = input_terminal_id or output_terminal_id (if input_terminal_type or output_terminal_type is 0x0101)
 *
 *
*/
struct usbh_audio_module {
    const char *name;
    rt_uint8_t data_intf;
    rt_uint8_t input_terminal_id;
    rt_uint16_t input_terminal_type;
    rt_uint16_t input_channel_config;
    rt_uint8_t output_terminal_id;
    rt_uint16_t output_terminal_type;
    rt_uint8_t feature_unit_id;
    rt_uint8_t feature_unit_controlsize;
    rt_uint8_t feature_unit_controls[8];
    rt_uint8_t terminal_link_id;
    struct usbh_audio_format_type altsetting[CONFIG_USBHOST_MAX_INTF_ALTSETTINGS];
};

struct usbh_audio {
    //struct usbh_hubport *hport;
    struct uinstance* device;
    char devname[CONFIG_USBHOST_DEV_NAMELEN];
    struct uendpoint_descriptor isoin;  /* ISO IN endpoint */
    struct uendpoint_descriptor isoout; /* ISO OUT endpoint */

    upipe_t pipe_in;
    upipe_t pipe_out;

    rt_uint8_t ctrl_intf; /* interface number */
    rt_uint8_t minor;
    rt_uint16_t isoin_mps;
    rt_uint16_t isoout_mps;
    bool is_opened;
    rt_uint16_t bcdADC;
    rt_uint8_t bInCollection;
    rt_uint8_t num_of_intf_altsettings;
    struct usbh_audio_module module[2];
    rt_uint8_t module_num;
    rt_uint8_t *rx_buff;

    void *user_data;
};

#ifdef __cplusplus
extern "C" {
#endif

int usbh_audio_open(struct usbh_audio *audio_class, const char *name, rt_uint32_t samp_freq);
int usbh_audio_close(struct usbh_audio *audio_class, const char *name);
int usbh_audio_get_min_volume(struct usbh_audio *audio_class, const char *name, rt_uint16_t *min_volume);
int usbh_audio_get_max_volume(struct usbh_audio *audio_class, const char *name, rt_uint16_t *max_volume);
int usbh_audio_get_cur_volume(struct usbh_audio *audio_class, const char *name, rt_uint16_t *cur_volume);
int usbh_audio_get_res_volume(struct usbh_audio *audio_class, const char *name, rt_uint16_t *res_volume);
int usbh_audio_set_volume(struct usbh_audio *audio_class, const char *name, rt_uint16_t volume_hex);
int usbh_audio_set_volume_db(struct usbh_audio *audio_class, const char *name, int volume_db, int min_volume_db, int max_volume_db);
int usbh_audio_set_mute(struct usbh_audio *audio_class, const char *name, bool mute);

void rtt_usbh_audio_irq(void * dev, rt_uint32_t irq, rt_uint8_t ep);

rt_uint32_t rtt_usbh_audio_dev_pipe_mange(rt_uint8_t dev_num, const char *name, rt_uint8_t alloc_or_free);

rt_uint32_t rtt_usbh_audio_user_open();
rt_uint32_t rtt_usbh_audio_user_close();
rt_uint32_t rtt_usbh_audio_user_stop();
rt_uint32_t rtt_usbh_audio_user_start();

void usbh_audio_run(struct usbh_audio *audio_class);
void usbh_audio_stop(struct usbh_audio *audio_class);

#ifdef __cplusplus
}
#endif

#endif /* USBH_AUDIO_H */
