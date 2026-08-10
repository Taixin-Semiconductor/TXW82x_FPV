#include "app/screen/screen_memory.h"
#include "app/screen/screen_manager.h"
#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "keyWork.h"
#include "keyScan.h"
#include "stream_define.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "scale_msi/scale3_normal_msi.h"
#include "app/takephoto_module/takephoto.h"

extern void common_takephoto_over_dpi_api(struct msi *scale3, uint16_t normal_w, uint16_t noraml_h, uint16_t thumb_w, uint16_t thumb_h);
extern void common_takephoto_noraml_api(struct msi *jpg_normal_msi);
#define STREAM_MALLOC      av_psram_malloc
#define STREAM_FREE        av_psram_free
#define STREAM_ZALLOC      av_psram_zalloc
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

extern lv_indev_t *indev_keypad;
extern lv_style_t  g_style;

struct takephoto_ui_s
{
    screen_t   *screen;
    lv_group_t *now_group;
    uint16_t    w, h;

    struct msi *s;
    struct msi *display_msi;
    struct msi *takephoto_msi;
    uint8_t     owns_self;
};

static uint32_t self_key(uint32_t val)
{
    uint32_t key_ret = 0;
    if (val > 0)
    {
        if ((val & 0xff) == KEY_EVENT_SUP)
        {
            switch (val >> 8)
            {
                case AD_UP:
                    key_ret = 'q';
                    break;
                case AD_DOWN:
                    key_ret = 'e';
                    break;
                case AD_LEFT:
                    key_ret = 'a';
                    break;
                case AD_RIGHT:
                    key_ret = 'd';
                    break;
                case AD_PRESS:
                    key_ret = LV_KEY_ENTER;
                    break;
                default:
                    break;
            }
        }
    }
    return key_ret;
}

static void takephoto_on_back(lv_event_t *e)
{
    int32_t c = *((int32_t *) lv_event_get_param(e));
    if (c == 'q')
    {
        struct takephoto_ui_s *ui_s = (struct takephoto_ui_s *) lv_event_get_user_data(e);
        if (ui_s->screen)
        {
            screen_finish(ui_s->screen);
        }
    }
}

static void takephoto_action(lv_event_t *e)
{
    struct takephoto_ui_s *ui_s = (struct takephoto_ui_s *) lv_event_get_user_data(e);
    msi_do_cmd(ui_s->takephoto_msi, MSI_CMD_START, 1, 0);
}

static void takephoto_group_enter(struct takephoto_ui_s *ui_s)
{
    if (!ui_s || !ui_s->screen || ui_s->now_group)
    {
        return;
    }
    ui_s->now_group = lv_group_create();
    if (!ui_s->now_group)
    {
        return;
    }

    lv_group_add_obj(ui_s->now_group, ui_s->screen->root);
    lv_group_focus_obj(ui_s->screen->root);
    if (indev_keypad)
    {
        lv_indev_set_group(indev_keypad, ui_s->now_group);
    }
}

static void takephoto_group_leave(struct takephoto_ui_s *ui_s)
{
    if (!ui_s || !ui_s->now_group)
    {
        return;
    }

    lv_group_del(ui_s->now_group);
    ui_s->now_group = NULL;
}

static void takephoto_stream_create(struct takephoto_ui_s *ui_s)
{
    if (!ui_s || ui_s->s)
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
    ui_s->s         = scale3_normal_msi(S_PREVIEW_SCALE3, &cfg);
    if (ui_s->s)
    {
        msi_do_cmd(ui_s->s, MSI_CMD_SCALE3_NORMAL, MSI_SCALE3_START, 0x01);
        ui_s->s->enable = 1;
    }

    ui_s->display_msi = msi_find2(VDD_MSI, 0, 0, 0);
    msi_add_output(ui_s->s, NULL, ui_s->display_msi, NULL);

    ui_s->takephoto_msi = new_takephoto_msi(FRAMEBUFF_SOURCE_CAMERA0, FSTYPE_VIDEO_VPP_DATA0);
    msi_add_output(NULL, AUTO_JPG, ui_s->takephoto_msi, NULL);
}

static void takephoto_stream_destroy(struct takephoto_ui_s *ui_s)
{
    if (!ui_s)
    {
        return;
    }

    if (ui_s->s)
    {
        ui_s->s->enable = 0;
        msi_do_cmd(ui_s->s, MSI_CMD_SCALE3_NORMAL, MSI_SCALE3_START, 0);
        msi_destroy(ui_s->s);
        ui_s->s = NULL;
    }
    if (ui_s->display_msi)
    {
        msi_put(ui_s->display_msi);
        ui_s->display_msi = NULL;
    }
    if (ui_s->takephoto_msi)
    {
        msi_destroy(ui_s->takephoto_msi);
        ui_s->takephoto_msi = NULL;
    }
}

static void takephoto_create(screen_t *screen, void *params)
{
    struct takephoto_ui_s *ui_s = (struct takephoto_ui_s *) params;

    if (!ui_s)
    {
        ui_s = (struct takephoto_ui_s *) SCREEN_MALLOC(sizeof(struct takephoto_ui_s));
        if (!ui_s)
        {
            return;
        }
        memset(ui_s, 0, sizeof(struct takephoto_ui_s));
        ui_s->owns_self = 1;
    }

    ui_s->screen = screen;
    screen_set_user_data(screen, ui_s);

    lv_obj_add_style(screen->root, &g_style, 0);
    lv_obj_set_size(screen->root, LV_PCT(100), LV_PCT(100));

    lv_obj_add_event_cb(screen->root, takephoto_on_back, LV_EVENT_KEY, ui_s);
    lv_obj_add_event_cb(screen->root, takephoto_action, LV_EVENT_SHORT_CLICKED, ui_s);

    takephoto_group_enter(ui_s);
}

static void takephoto_start(screen_t *screen)
{
    struct takephoto_ui_s *ui_s;

    if (!screen)
    {
        return;
    }

    ui_s = (struct takephoto_ui_s *) screen_get_user_data(screen);
    if (!ui_s)
    {
        return;
    }

    set_lvgl_get_key_func(self_key);
    takephoto_stream_create(ui_s);
}

static void takephoto_resume(screen_t *screen)
{
    struct takephoto_ui_s *ui_s;

    if (!screen)
    {
        return;
    }

    ui_s = (struct takephoto_ui_s *) screen_get_user_data(screen);
    if (!ui_s)
    {
        return;
    }

    if (ui_s->now_group && indev_keypad)
    {
        lv_indev_set_group(indev_keypad, ui_s->now_group);
    }
}

static void takephoto_pause(screen_t *screen)
{
    struct takephoto_ui_s *ui_s;

    if (!screen)
    {
        return;
    }

    ui_s = (struct takephoto_ui_s *) screen_get_user_data(screen);
    if (!ui_s)
    {
        return;
    }

    set_lvgl_get_key_func(NULL);
}

static void takephoto_stop(screen_t *screen)
{
    (void) screen;
}

static void takephoto_destroy(screen_t *screen)
{
    struct takephoto_ui_s *ui_s;

    if (!screen)
    {
        return;
    }

    ui_s = (struct takephoto_ui_s *) screen_get_user_data(screen);
    if (!ui_s)
    {
        return;
    }

    takephoto_stream_destroy(ui_s);
    takephoto_group_leave(ui_s);
    ui_s->screen = NULL;
    if (ui_s->owns_self)
    {
        SCREEN_FREE(ui_s);
    }
    screen_set_user_data(screen, NULL);
}

static void takephoto_restart(screen_t *screen)
{
    (void) screen;
}

static void takephoto_finish(screen_t *screen)
{
    (void) screen;
}

static void takephoto_get_lifecycle(screen_lifecycle_t *lifecycle)
{
    if (!lifecycle)
    {
        return;
    }

    *lifecycle = (screen_lifecycle_t) {
            .on_create  = takephoto_create,
            .on_start   = takephoto_start,
            .on_resume  = takephoto_resume,
            .on_pause   = takephoto_pause,
            .on_stop    = takephoto_stop,
            .on_destroy = takephoto_destroy,
            .on_restart = takephoto_restart,
            .on_finish  = takephoto_finish,
    };
}

static void enter_takephoto_ui(lv_event_t *e)
{
    struct takephoto_ui_s *ui_s = (struct takephoto_ui_s *) lv_event_get_user_data(e);
    screen_lifecycle_t     lifecycle;

    if (!ui_s)
    {
        return;
    }

    if (!ui_s->screen)
    {
        takephoto_get_lifecycle(&lifecycle);
        ui_s->screen = screen_register("takephoto", &lifecycle);
        if (ui_s->screen)
        {
            ui_s->screen->keep_alive = 1;
        }
    }

    if (ui_s->screen)
    {
        screen_push(ui_s->screen, ui_s);
    }
}

lv_obj_t *takephoto_ui(lv_group_t *group, lv_obj_t *base_ui, uint16_t w, uint16_t h)
{
    struct takephoto_ui_s *ui_s;

    ui_s = (struct takephoto_ui_s *) STREAM_LIBC_ZALLOC(sizeof(struct takephoto_ui_s));
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

    lv_obj_t *btn = lv_list_add_btn(base_ui, NULL, "takephoto");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_takephoto_ui, LV_EVENT_SHORT_CLICKED, ui_s);
    return btn;
}
