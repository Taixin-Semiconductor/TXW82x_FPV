#ifndef __LVGL_UI_H
#define __LVGL_UI_H
#include "lvgl/lvgl.h"
lv_obj_t *preview_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *preview_usb_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *photo_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *preview_sim_video_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *preview_dvp_csi_usb_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *preview_qc_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *audio_dac_test_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *touch_pad_test_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *preview_encode_takephoto_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *avi_record_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *avi_loop_record_ui(lv_group_t *group,lv_obj_t *base_ui);
lv_obj_t *avi_playback_ui(lv_group_t *group,lv_obj_t *base_ui);

#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "scale_msi.h"
#include "jpg_concat_msi.h"
extern struct msi *jpg_decode_msg_msi(const char *name, uint16_t out_w, uint16_t out_h, uint16_t step_w, uint16_t step_h, uint32_t filter);
extern struct msi *sim_video_more_msi(const char *name, uint16_t w, uint16_t h, uint16_t *sim_filter);
extern struct msi *jpg_decode_msi(const char *name);
extern struct msi *route_msi(const char *name);
extern struct msi *sim_video_msi(const char *name, uint16_t w, uint16_t h, uint8_t top);
extern struct msi *gen420_msi_init(uint16_t w,uint16_t h,uint16_t *filter_type);
extern struct msi *jpg_thumb_msi_init(const char *msi_name, uint16_t filter);


void set_lvgl_get_key_func(void *func);
#endif