#include "lvgl/lvgl.h"
#include "project_config.h"
#include "lvgl_ui.h"

enum UI_MODE
{
    MINI_DV_UI,
    IPC_UI,
    BBM_UI,
    CHILDREN_UI,
    POCKET_CAMERA_UI,   //拇指相机ui的demo
    LLM_VISION_UI,
};

#ifndef DEFINE_UI
#define DEFINE_UI    POCKET_CAMERA_UI
#endif

lv_indev_t * indev_keypad;
extern lv_style_t g_style;


lv_obj_t *main_Mini_DV_ui(lv_obj_t *base_ui,lv_group_t *group)
{
    lv_obj_t *btn;
    //可以打印系统一些信息
    // btn = system_msg_ui(group,base_ui);
    //预览dvp的镜头数据
    btn = preview_ui(group,base_ui,320,240);
    btn = preview_usb_ui(group,base_ui);
    btn = photo_ui(group,base_ui);
    btn = preview_sim_video_ui(group,base_ui);
    btn = preview_dvp_csi_usb_ui(group,base_ui);
    btn = preview_qc_ui(group,base_ui);
    btn = audio_dac_test_ui(group,base_ui);
    btn = touch_pad_test_ui(group,base_ui);
    btn = preview_encode_takephoto_ui(group,base_ui);
    btn = avi_record_ui(group,base_ui);
    btn = avi_loop_record_ui(group,base_ui);
    btn = avi_playback_ui(group,base_ui);
    btn = mp4_player_ui(group,base_ui,320,240);
    btn = mp4_record_ui(group,base_ui,320,240);


    //可以进行录像拍照
    // btn = preview_encode_selct_ui(group,base_ui);
    //播放自己录制的视频文件
    // btn = player2_ui(group,base_ui);
    return base_ui;
}

lv_obj_t *main_pocket_camera_ui(lv_obj_t *base_ui,lv_group_t *group)
{
    lv_obj_t *btn;
    //可以打印系统一些信息
    // btn = system_msg_ui(group,base_ui);
    //预览dvp的镜头数据
    btn = preview_ui(group,base_ui,160,128);
    btn = takephoto_ui(group,base_ui,160,128);
    btn = mp4_player_ui(group,base_ui,160,128);
    btn = mp4_record_ui(group,base_ui,160,128);
    return base_ui;
}

extern struct msi *scale3_normal_msi2(const char *name, uint8_t force_stype, uint16_t ow, uint16_t oh);
lv_obj_t *main_ui(lv_obj_t *base_ui)
{
    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);
    lv_obj_t * ui = lv_list_create(lv_scr_act());
    // lv_obj_add_style(ui,&g_style,0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));
    switch(DEFINE_UI)
    {
        case MINI_DV_UI:
            main_Mini_DV_ui(ui,group);
        break;
        case IPC_UI:
        break;
        case BBM_UI:
			lv_obj_add_style(ui,&g_style,0);
        break;
        case CHILDREN_UI:
        break;
        case POCKET_CAMERA_UI:
            //支持大分辨拍照或者缩略图
            scale3_normal_msi2(S_PREVIEW_SCALE3, FSTYPE_YUV_P0, 168, 128);
            main_pocket_camera_ui(ui,group);
        break;
        case LLM_VISION_UI:
            main_LLM_vision_ui(ui,group);
        break;
    }
    return ui;
}
