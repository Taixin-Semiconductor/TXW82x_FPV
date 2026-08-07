#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "stream_frame.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "audio_media_ctrl/audio_media_ctrl.h"


//data申请空间函数
#define STREAM_MALLOC     av_psram_malloc
#define STREAM_FREE       av_psram_free
#define STREAM_ZALLOC     av_psram_zalloc

//结构体申请空间函数
#define STREAM_LIBC_MALLOC     av_malloc
#define STREAM_LIBC_FREE       av_free
#define STREAM_LIBC_ZALLOC     av_zalloc


extern lv_indev_t * indev_keypad;
extern lv_style_t g_style;
struct audio_dac_test_ui_s
{
    lv_group_t  *last_group;
    lv_obj_t    *base_ui;

    lv_group_t    *now_group;
    lv_obj_t    *now_label;      
    lv_obj_t    *now_ui;
};

static void exit_audio_dac_test_ui(lv_event_t * e)
{
    struct audio_dac_test_ui_s *ui_s = (struct audio_dac_test_ui_s *)lv_event_get_user_data(e);
    lv_indev_set_group(indev_keypad, ui_s->last_group);
    lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_del(ui_s->now_label);
    lv_group_del(ui_s->now_group);
    ui_s->now_group = NULL;
    ui_s->now_label = NULL;

    #if AUDIO_EN
    audio_file_play_stop();
    #endif

    lv_obj_del(ui_s->now_ui);
}


static void enter_audio_dac_test_ui(lv_event_t * e)
{
    struct audio_dac_test_ui_s *ui_s = (struct audio_dac_test_ui_s *)lv_event_get_user_data(e);
    lv_obj_add_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *ui = lv_obj_create(lv_scr_act());  
    ui_s->now_ui = ui;
    // lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));

    lv_obj_t *label = lv_label_create(lv_scr_act());
	lv_obj_set_size(label, LV_PCT(100), LV_PCT(100));
	lv_label_set_text(label, "audio_dac_test");
	lv_obj_align(label,LV_ALIGN_CENTER,0,0);
    ui_s->now_label = label;

    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);

    lv_group_add_obj(group, ui);
    ui_s->now_group = group;

    #if AUDIO_EN
    audio_file_play_stop();
    msi_cmd("R_AUDAC", MSI_CMD_AUDAC, MSI_AUDAC_SET_VOLUME, 30);
    audio_file_play_init("0:count.wav", 1);
    #endif

    lv_obj_add_event_cb(ui, exit_audio_dac_test_ui, LV_EVENT_PRESSED, ui_s);
}

lv_obj_t *audio_dac_test_ui(lv_group_t *group,lv_obj_t *base_ui)
{
    struct audio_dac_test_ui_s *ui_s = (struct audio_dac_test_ui_s*)STREAM_LIBC_ZALLOC(sizeof(struct audio_dac_test_ui_s));
    ui_s->last_group = group;
    ui_s->base_ui = base_ui;
    lv_obj_t *btn =  lv_list_add_btn(base_ui, NULL, "audio_dac_test");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_audio_dac_test_ui, LV_EVENT_PRESSED, ui_s);
    return btn;
}