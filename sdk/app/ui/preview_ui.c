/*************************************************************************************** 
这个主要是预览,通过scale3_msi将dvp/csi数据scale到对应的分辨率作为数据流发送出去
然后lcd_msi需要接收对应的数据去显示
这里只是产生对应数据流,数据用途实际需要终端流(比如lcd流)接收后去处理

                        ---->   R_VIDEO_P0(320x240)
                        |        (lcd_video_p0)
S_PREVIEW_SCALE3   -----|
(scale3)                |
                        ---->   R_VIDEO_P1(320x240)
                                 (lcd_video_p1)
***************************************************************************************/
#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "stream_frame.h"
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
struct preview_ui_s
{
    lv_group_t  *last_group;
    lv_obj_t    *base_ui;

    lv_group_t    *now_group;
    lv_obj_t    *now_ui;

    struct msi *s;

};

static void exit_preview_ui(lv_event_t * e)
{
    struct preview_ui_s *ui_s = (struct preview_ui_s *)lv_event_get_user_data(e);
    lv_indev_set_group(indev_keypad, ui_s->last_group);
    lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_group_del(ui_s->now_group);
    ui_s->now_group = NULL;
    msi_del_output(ui_s->s, NULL, R_VIDEO_P0);
    msi_del_output(ui_s->s, NULL, R_VIDEO_P1);
    ui_s->s->enable = 0;
    //关闭VIDEO P0,VIDEO P1的使能，释放占住的scale3的fb
    msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);   
    msi_cmd(R_VIDEO_P1, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);
    msi_destroy(ui_s->s);
    lv_obj_del(ui_s->now_ui);
}
//进入预览的ui,那么就要创建新的ui去显示预览图了
static void enter_preview_ui(lv_event_t * e)
{
    struct preview_ui_s *ui_s = (struct preview_ui_s *)lv_event_get_user_data(e);
    lv_obj_add_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *ui = lv_obj_create(lv_scr_act());  
    ui_s->now_ui = ui;
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));
    //绑定流到Video_P0,Video_P1显示
    
    ui_s->s = scale3_msi(S_PREVIEW_SCALE3);
    if (ui_s->s)
    {
        msi_add_output(ui_s->s, NULL, R_VIDEO_P0);
        msi_add_output(ui_s->s, NULL, R_VIDEO_P1);
        msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);
        msi_cmd(R_VIDEO_P1, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);
        ui_s->s->enable = 1;
    }

    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);

    lv_group_add_obj(group, ui);
    ui_s->now_group = group;
    lv_obj_add_event_cb(ui, exit_preview_ui, LV_EVENT_PRESSED, ui_s);
}

lv_obj_t *preview_ui(lv_group_t *group,lv_obj_t *base_ui)
{
    struct preview_ui_s *ui_s = (struct preview_ui_s*)STREAM_LIBC_ZALLOC(sizeof(struct preview_ui_s));
    ui_s->last_group = group;
    ui_s->base_ui = base_ui;
    lv_obj_t *btn =  lv_list_add_btn(base_ui, NULL, "preview");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_preview_ui, LV_EVENT_PRESSED, ui_s);
    return btn;
}