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
#include "app/screen/screen_memory.h"
#include "app/screen/screen.h"
#include "app/screen/screen_manager.h"
#include "stream_define.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/multimedia/msi_names.h"
#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "screen_common.h"
// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

extern lv_style_t  g_style;
extern lv_indev_t *indev_keypad;

struct preview_ui_s
{
    screen_t   *screen;
    lv_group_t *now_group;
    struct msi *s;
    struct msi *display_msi;
    uint16_t    w, h;
    uint8_t     started : 1, own_self : 1, rev : 6;
};

static void preview_stream_start(struct preview_ui_s *ui_s)
{

    if (!ui_s || ui_s->started)
    {
        return;
    }
    struct scale3_cfg cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.ow          = ui_s->w;
    cfg.oh          = ui_s->h;
    cfg.force_stype = FSTYPE_YUV_P0;
    cfg.splice_en   = 0;
    cfg.start       = 1;
    // 绑定流到Video_P0,Video_P1显示
    ui_s->s         = scale3_normal_msi(S_PREVIEW_SCALE3, &cfg);
    if (!ui_s->s)
    {
        return;
    }

    ui_s->display_msi = msi_find2(VDD_MSI, 0, 0, 0);
    if (!ui_s->display_msi)
    {
        msi_destroy(ui_s->s);
        ui_s->s = NULL;
        return;
    }
    msi_do_cmd(ui_s->s, MSI_CMD_SCALE3_NORMAL, MSI_SCALE3_START, 1);
    msi_add_output(ui_s->s, NULL, ui_s->display_msi, NULL);
    ui_s->started = 1;
}

static void preview_stream_stop(struct preview_ui_s *ui_s)
{
    if (!ui_s || !ui_s->started)
    {
        return;
    }
    lv_group_del(ui_s->now_group);
    ui_s->now_group = NULL;

    if (ui_s->s)
    {
        msi_do_cmd(ui_s->s, MSI_CMD_SCALE3_NORMAL, MSI_SCALE3_START, 0);
        msi_destroy(ui_s->s);
        ui_s->s = NULL;
    }
    if (ui_s->display_msi)
    {
        msi_put(ui_s->display_msi);
        ui_s->display_msi = NULL;
    }

    ui_s->started = 0;
}

static void preview_on_back(lv_event_t *e)
{
    (void) e;

    // screen_pop();
    struct preview_ui_s *ui_s = (struct preview_ui_s *) lv_event_get_user_data(e);
    if (!ui_s->screen)
    {
        return;
    }
    screen_finish(ui_s->screen);
}

static void preview_create(screen_t *screen, void *params)
{
    struct preview_ui_s *ui_s = (struct preview_ui_s *) params;

    // 由外部创建的UI,需要重新申请ui_s
    if (!ui_s)
    {
        ui_s = SCREEN_MALLOC(sizeof(struct preview_ui_s));
        if (!ui_s)
        {
            return;
        }
        memset(ui_s, 0, sizeof(struct preview_ui_s));
        ui_s->own_self = 1;
    }

    ui_s->now_group = lv_group_create();
    lv_group_add_obj(ui_s->now_group, screen->root);
    ui_s->screen = screen;
    screen_set_user_data(screen, ui_s);

    lv_obj_add_style(screen->root, &g_style, 0);
    lv_obj_set_size(screen->root, LV_PCT(100), LV_PCT(100));
    lv_obj_add_event_cb(screen->root, preview_on_back, LV_EVENT_PRESSED, ui_s);
}

static void preview_start(screen_t *screen)
{
    // struct preview_ui_s *ui_s = (struct preview_ui_s *) screen_get_user_data(screen);
    if (!screen)
    {
        return;
    }
}

static void preview_resume(screen_t *screen)
{
    struct preview_ui_s *ui_s = (struct preview_ui_s *) screen_get_user_data(screen);

    if (ui_s && indev_keypad && ui_s->now_group)
    {
        lv_indev_set_group(indev_keypad, ui_s->now_group);
        if (!lv_group_get_focused(ui_s->now_group))
        {
            lv_group_focus_next(ui_s->now_group);
        }
    }
    preview_stream_start((struct preview_ui_s *) screen_get_user_data(screen));
}

// 这里正常应该是要关闭scale3
static void preview_pause(screen_t *screen)
{
    // preview_stream_stop((struct preview_ui_s *) screen_get_user_data(screen));
}

static void preview_stop(screen_t *screen)
{
    (void) screen;
}

static void preview_destroy(screen_t *screen)
{
    struct preview_ui_s *ui_s = (struct preview_ui_s *) screen_get_user_data(screen);

    preview_stream_stop(ui_s);
    if (ui_s)
    {
        ui_s->screen = NULL;
        if (ui_s->own_self)
        {
            SCREEN_FREE(ui_s);
        }
    }
    screen_set_user_data(screen, NULL);
}

static void preview_restart(screen_t *screen)
{
    (void) screen;
}

static void preview_finish(screen_t *screen)
{
    (void) screen;
}


static void screen_manager_demo_preview_get_lifecycle(screen_lifecycle_t *lifecycle)
{
    if (!lifecycle)
    {
        return;
    }

    *lifecycle = (screen_lifecycle_t) {
            .on_create  = preview_create,
            .on_start   = preview_start,
            .on_resume  = preview_resume,
            .on_pause   = preview_pause,
            .on_stop    = preview_stop,
            .on_destroy = preview_destroy,
            .on_restart = preview_restart,
            .on_finish  = preview_finish,
    };
}

static void enter_preview_ui(lv_event_t *e)
{
    struct preview_ui_s *ui_s = (struct preview_ui_s *) lv_event_get_user_data(e);
    screen_lifecycle_t   lifecycle;

    if (!ui_s)
    {
        return;
    }
    if (!ui_s->screen)
    {
        screen_manager_demo_preview_get_lifecycle(&lifecycle);
        ui_s->screen = screen_register("preview", &lifecycle);
        if (ui_s->screen)
        {
            ui_s->screen->keep_alive = 1;
        }
    }

    if (ui_s->screen)
    {
        screen_push(ui_s->screen, ui_s);
    }

    os_printf("%s:%d\tui_s->screen id:%d\n", __func__, __LINE__, ui_s->screen->id);
}


lv_obj_t *preview_ui(lv_group_t *group, lv_obj_t *base_ui, uint16_t w, uint16_t h)
{
    lv_obj_t            *btn;
    struct preview_ui_s *ui_s;

    ui_s = (struct preview_ui_s *) STREAM_LIBC_ZALLOC(sizeof(struct preview_ui_s));
    if (!ui_s)
    {
        return NULL;
    }
    if (w == 0 || h == 0)
    {
        // 使用全屏显示
        ui_s->w = LV_HOR_RES;
        ui_s->h = LV_VER_RES;
    }
    else
    {
        ui_s->w = w;
        ui_s->h = h;
    }
    btn = lv_list_add_btn(base_ui, NULL, "preview");
    if (group)
    {
        lv_group_add_obj(group, btn);
    }
    lv_obj_add_event_cb(btn, enter_preview_ui, LV_EVENT_PRESSED, ui_s);
    return btn;
}
