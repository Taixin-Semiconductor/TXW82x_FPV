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
#include "scale_msi/scale3_normal_msi.h"
#include "takephoto_module/takephoto.h"

extern void common_takephoto_over_dpi_api(struct msi *scale3, uint16_t normal_w, uint16_t noraml_h, uint16_t thumb_w, uint16_t thumb_h);
extern void common_takephoto_noraml_api(struct msi *jpg_normal_msi);
// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

extern lv_indev_t *indev_keypad;
extern lv_style_t  g_style;
struct takephoto_ui_s
{
    lv_group_t *last_group;
    lv_obj_t   *base_ui;
    uint16_t    w, h;

    lv_group_t *now_group;
    lv_obj_t   *now_ui;

    struct msi *s;
    struct msi *over_dpi_msi;
    struct msi *thumb_msi;
    struct msi *scale1_jpg_recode_msi;
    struct msi *jpg_normal_msi;
};

static void exit_takephoto_ui(lv_event_t *e)
{
    struct takephoto_ui_s *ui_s = (struct takephoto_ui_s *) lv_event_get_user_data(e);
    lv_indev_set_group(indev_keypad, ui_s->last_group);
    lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_group_del(ui_s->now_group);
    ui_s->now_group = NULL;
    msi_del_output(ui_s->s, NULL, R_VIDEO_P0);
    msi_del_output(ui_s->s, NULL, R_VIDEO_P1);
    ui_s->s->enable = 0;
    // 关闭VIDEO P0,VIDEO P1的使能，释放占住的scale3的fb
    msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);
    msi_cmd(R_VIDEO_P1, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);

    if (ui_s->s)
    {
        msi_do_cmd(ui_s->s, MSI_CMD_SCALE3_NORMAL, MSI_SCALE3_START, 0);
        if (ui_s->thumb_msi)
        {
            msi_del_output(ui_s->s, NULL, ui_s->thumb_msi->name);
            msi_put(ui_s->thumb_msi);
        }
        if (ui_s->over_dpi_msi)
        {
            msi_del_output(ui_s->s, NULL, ui_s->over_dpi_msi->name);
            msi_put(ui_s->over_dpi_msi);
        }
    }

    if (ui_s->scale1_jpg_recode_msi)
    {
        msi_put(ui_s->scale1_jpg_recode_msi);
    }

    if(ui_s->jpg_normal_msi)
    {
        msi_put(ui_s->jpg_normal_msi);
    }
    msi_destroy(ui_s->s);
    lv_obj_del(ui_s->now_ui);
}

static void takephoto_action(lv_event_t *e)
{
    struct takephoto_ui_s *ui_s = (struct takephoto_ui_s *) lv_event_get_user_data(e);
    // 没有找到重新编码的msi
    if (!ui_s->scale1_jpg_recode_msi)
    {
        return;
    }

    //测试代码,如果是需要插值拍照
    if (1)
    {
        // 设置需要拍照的分辨率,这里是demo,暂时固定,实际需要通过设置文件去配置
        uint32_t w       = 2560;
        uint32_t h       = 1440;
        uint32_t dpi_w_h = w << 16 | h;
        msi_do_cmd(ui_s->scale1_jpg_recode_msi, MSI_CMD_SCALE1, MSI_SCALE1_RESET_DPI, dpi_w_h);
        os_printf("takephoto_action\n");

        // 缩略图320x240
        // 原图yuv
        common_takephoto_over_api(w, h, 320, 240,1);
    }
    //非插值拍照
    else
    {
        if(ui_s->jpg_normal_msi)
        {
            common_takephoto_noraml_api(ui_s->jpg_normal_msi);
        }
    }
}

// 进入预览的ui,那么就要创建新的ui去显示预览图了
static void enter_takephoto_ui(lv_event_t *e)
{
    struct takephoto_ui_s *ui_s = (struct takephoto_ui_s *) lv_event_get_user_data(e);
    lv_obj_add_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *ui = lv_obj_create(lv_scr_act());
    ui_s->now_ui = ui;
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));
    // 绑定流到Video_P0,Video_P1显示
    ui_s->s = scale3_normal_msi2(S_PREVIEW_SCALE3, FSTYPE_YUV_P0, ui_s->w, ui_s->h);
    if (ui_s->s)
    {

        msi_do_cmd(ui_s->s, MSI_CMD_SCALE3_NORMAL, MSI_SCALE3_START, 0x01);
        msi_add_output(ui_s->s, NULL, R_VIDEO_P0);
        msi_add_output(ui_s->s, NULL, R_VIDEO_P1);
        msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);
        msi_cmd(R_VIDEO_P1, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);

        ui_s->over_dpi_msi = msi_find(SR_OVER_DPI_JPG, 1);
        if (ui_s->over_dpi_msi)
        {
            msi_add_output(ui_s->s, NULL, ui_s->over_dpi_msi->name);
        }

        ui_s->thumb_msi = msi_find(SR_OVER_DPI_THUMB_JPG, 1);
        if (ui_s->thumb_msi)
        {
            msi_add_output(ui_s->s, NULL, ui_s->thumb_msi->name);
        }

        ui_s->jpg_normal_msi = msi_find(R_JPG_THUMB, 1);
        ui_s->s->enable      = 1;
    }
    ui_s->scale1_jpg_recode_msi = msi_find(R_SCALE1_JPG_RECODE, 1);

    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);

    lv_group_add_obj(group, ui);
    ui_s->now_group = group;
    lv_obj_add_event_cb(ui, exit_takephoto_ui, LV_EVENT_LONG_PRESSED, ui_s);
    lv_obj_add_event_cb(ui, takephoto_action, LV_EVENT_SHORT_CLICKED, ui_s);
}

lv_obj_t *takephoto_ui(lv_group_t *group, lv_obj_t *base_ui, uint16_t w, uint16_t h)
{
    struct takephoto_ui_s *ui_s = (struct takephoto_ui_s *) STREAM_LIBC_ZALLOC(sizeof(struct takephoto_ui_s));
    ui_s->last_group            = group;
    ui_s->base_ui               = base_ui;
    ui_s->w                     = w;
    ui_s->h                     = h;
    lv_obj_t *btn               = lv_list_add_btn(base_ui, NULL, "takephoto");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_takephoto_ui, LV_EVENT_SHORT_CLICKED, ui_s);
    return btn;
}