#include "lvgl/lvgl.h"
#include "lv_timer.h"
#include "project_config.h"
#include "lvgl_ui.h"
#include "app/screen/screen_memory.h"
#include "app/screen/screen_manager.h"
#include "app/screen/screen_command.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "app/screen/screen_common.h"
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

struct main_ui_s
{
    screen_t   *screen;
    lv_group_t *group;
    lv_timer_t *cmd_timer;
};

static void main_ui_create(screen_t *screen, void *params)
{
    struct main_ui_s *ui_s = (struct main_ui_s *) params;

    if (!ui_s)
    {
        return;
    }

    ui_s->screen = screen;
    lv_obj_t *ui = lv_list_create(screen->root);
    screen_set_user_data(screen, ui_s);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));
    lv_obj_t *btn;
    btn = preview_ui(ui_s->group, ui, 0, 0);
    btn = takephoto_ui(ui_s->group, ui, 0, 0);
    btn = photo_ui(ui_s->group, ui);
    btn = mp4_player_ui(ui_s->group, ui, 0, 0);
    btn = mp4_record_ui(ui_s->group, ui, 0, 0);
}

static void main_ui_start(screen_t *screen)
{
    if (!screen)
    {
        return;
    }

    // lv_obj_clear_flag(screen->root, LV_OBJ_FLAG_HIDDEN);
}

static void main_ui_resume(screen_t *screen)
{
    struct main_ui_s *ui_s;

    if (!screen)
    {
        return;
    }

    ui_s = (struct main_ui_s *) screen_get_user_data(screen);
    if (ui_s && indev_keypad && ui_s->group)
    {
        lv_indev_set_group(indev_keypad, ui_s->group);
        if (!lv_group_get_focused(ui_s->group))
        {
            lv_group_focus_next(ui_s->group);
        }
    }
}

static void main_ui_pause(screen_t *screen)
{
    (void) screen;
}

static void main_ui_stop(screen_t *screen)
{
    (void) screen;
}

static void main_ui_destroy(screen_t *screen)
{
    struct main_ui_s *ui_s = (struct main_ui_s *) screen_get_user_data(screen);

    if (ui_s)
    {
        ui_s->screen = NULL;
    }
    screen_set_user_data(screen, NULL);
}

static void main_ui_restart(screen_t *screen)
{
    (void) screen;
}

static void main_ui_finish(screen_t *screen)
{
    (void) screen;
}

static void main_ui_get_lifecycle(screen_lifecycle_t *lifecycle)
{
    if (!lifecycle)
    {
        return;
    }

    *lifecycle = (screen_lifecycle_t) {
            .on_create  = main_ui_create,
            .on_start   = main_ui_start,
            .on_resume  = main_ui_resume,
            .on_pause   = main_ui_pause,
            .on_stop    = main_ui_stop,
            .on_destroy = main_ui_destroy,
            .on_restart = main_ui_restart,
            .on_finish  = main_ui_finish,
    };
}
void main_ui()
{
    screen_lifecycle_t lifecycle;
    lv_style_reset(&g_style);
    lv_style_init(&g_style);
    lv_style_set_bg_color(&g_style, lv_color_make(0x00, 0x00, 0x00));
    lv_style_set_shadow_color(&g_style, lv_color_make(0x00, 0x00, 0x00));
    lv_style_set_border_color(&g_style, lv_color_make(0x00, 0x00, 0x00));
    lv_style_set_outline_color(&g_style, lv_color_make(0x00, 0x00, 0x00));
    lv_style_set_radius(&g_style, 0);
    // 初始化屏幕管理器(框架)
    screen_manager_init();
    screen_command_init();

    // 启动主页
    struct main_ui_s *ui_s;
    ui_s        = (struct main_ui_s *) STREAM_LIBC_ZALLOC(sizeof(struct main_ui_s));
    ui_s->group = lv_group_create();

    if (indev_keypad && ui_s->group)
    {
        lv_indev_set_group(indev_keypad, ui_s->group);
    }
    if (!ui_s->screen)
    {
        main_ui_get_lifecycle(&lifecycle);
        ui_s->screen = screen_register("main_ui", &lifecycle);
        if (!ui_s->screen)
        {
            return;
        }
        ui_s->screen->keep_alive = 1;
    }
    if (screen_current() != ui_s->screen)
    {
        screen_push(ui_s->screen, ui_s);
    }

    return;
}
