#include "lvgl/lvgl.h"
#include "project_config.h"
#include "lvgl_ui.h"

#define MINI_DV_UI 	0
#define IPC_UI		1
#define BBM_UI 		2
#define CHILDREN_UI 3

#define DEFINE_UI    MINI_DV_UI

lv_indev_t * indev_keypad;
extern lv_style_t g_style;


lv_obj_t *main_Mini_DV_ui(lv_obj_t *base_ui,lv_group_t *group)
{
    lv_obj_t *btn;
    //可以打印系统一些信息
    // btn = system_msg_ui(group,base_ui);
    //预览dvp的镜头数据
    btn = preview_ui(group,base_ui);
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


    //可以进行录像拍照
    // btn = preview_encode_selct_ui(group,base_ui);
    //播放自己录制的视频文件
    // btn = player2_ui(group,base_ui);
    return base_ui;
}


lv_obj_t *main_ui(lv_obj_t *base_ui)
{
    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);
    lv_obj_t * ui = lv_list_create(lv_scr_act());
    // lv_obj_add_style(ui,&g_style,0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));

    #if DEFINE_UI == MINI_DV_UI
        main_Mini_DV_ui(ui,group);
    #elif DEFINE_UI == IPC_UI
        //main_IPC_ui(ui,group);
    #elif DEFINE_UI == BBM_UI
		//main_BBM_ui(ui,group);
    #elif DEFINE_UI == CHILDREN_UI
        //main_children_ui(ui,group);
    #endif
    return ui;
}
