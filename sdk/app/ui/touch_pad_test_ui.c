#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

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
struct touch_pad_test_ui_s
{
    lv_group_t  *last_group;
    lv_obj_t    *base_ui;

    lv_group_t    *now_group;
    lv_obj_t    *now_label; 
    lv_obj_t    *now_label2; 
    lv_obj_t    *now_btn;    
    lv_obj_t    *now_btn2;
    lv_obj_t    *now_ui;
};

static void btn_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* btn = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED) {
        static uint8_t cnt = 0;
        cnt++;
        lv_obj_t* label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Button: %d", cnt);
    }
}

static void exit_touch_pad_test_ui(lv_event_t* e) {
    struct touch_pad_test_ui_s *ui_s = (struct touch_pad_test_ui_s *)lv_event_get_user_data(e);
    lv_indev_set_group(indev_keypad, ui_s->last_group);
    lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_del(ui_s->now_label);
    lv_obj_del(ui_s->now_btn);
    lv_obj_del(ui_s->now_label2);
    lv_obj_del(ui_s->now_btn2);
    lv_group_del(ui_s->now_group);
    ui_s->now_group = NULL;
    ui_s->now_label = NULL;
    ui_s->now_btn = NULL;
    ui_s->now_label2 = NULL;
    ui_s->now_btn2 = NULL;

    lv_obj_del(ui_s->now_ui);
}

static void enter_touch_pad_test_ui(lv_event_t* e) {

    struct touch_pad_test_ui_s *ui_s = (struct touch_pad_test_ui_s *)lv_event_get_user_data(e);
    lv_obj_add_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *ui = lv_obj_create(lv_scr_act());  
    ui_s->now_ui = ui;
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));

    lv_obj_t* btn = lv_btn_create(ui);
    lv_obj_set_size(btn, 150, 50);
    lv_obj_center(btn);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);
    ui_s->now_btn = btn;

    lv_obj_t* btn2 = lv_btn_create(ui);
    lv_obj_set_size(btn2, 50, 50);
    lv_obj_center(btn2);
    lv_obj_align(btn2, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_add_event_cb(btn2, exit_touch_pad_test_ui, LV_EVENT_PRESSED, ui_s);
    ui_s->now_btn2 = btn2;

    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, "Button");
    lv_obj_center(label);
    ui_s->now_label = label;

    lv_obj_t* label2 = lv_label_create(btn2);
    lv_label_set_text(label2, LV_SYMBOL_CLOSE);
    lv_obj_center(label2);
    ui_s->now_label2 = label2;

    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);

    lv_group_add_obj(group, ui);
    ui_s->now_group = group;

    // lv_obj_add_event_cb(ui, exit_touch_pad_test_ui, LV_EVENT_PRESSED, ui_s);
}

lv_obj_t *touch_pad_test_ui(lv_group_t *group,lv_obj_t *base_ui)
{
    struct touch_pad_test_ui_s *ui_s = (struct touch_pad_test_ui_s*)STREAM_LIBC_ZALLOC(sizeof(struct touch_pad_test_ui_s));
    ui_s->last_group = group;
    ui_s->base_ui = base_ui;
    lv_obj_t *btn =  lv_list_add_btn(base_ui, NULL, "touch_pad_test");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_touch_pad_test_ui, LV_EVENT_PRESSED, ui_s);
    return btn;
}