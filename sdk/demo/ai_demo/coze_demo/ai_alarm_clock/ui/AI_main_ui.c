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
#include "ui/ui_manager.h"
#include "ai_main_ui.h"
#include "app/screen/screen_msg_queue.h"
// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

// lv_indev_t       *indev_keypad;
extern lv_style_t g_style;
struct main_ui_s
{
    screen_t   *screen;
    lv_group_t *group;
    lv_timer_t *cmd_timer;
};

static uint32_t cmd_del_ui(struct screen_common_s *m)
{
    screen_pop_id(m->screen_id);
    return 0;
}

typedef uint32_t (*ui_create_fn)(struct screen_common_s *m);

static ui_create_fn ui_create_fns[SCREEN_MAX] = {
        [SCREEN_DEL]   = cmd_del_ui,
        [AI_SCREEN]    = extern_ai_education_screen_create,
        [MUSIC_SCREEN] = extern_media_player_ui_create,
};

// 命令是队列形式
static void app_command_poll(struct _lv_timer_t *d)
{
    (void) d;
    screen_command_t *node;
    // 轮询命令,从列表获取命令,并执行,当前主要是启动ui的命令
    // 使用ui管理器来打开ui和关闭ui
    // ui响应内容命令由谁来触发?
    node = (screen_command_t *) screen_command_recv();
    struct screen_common_s *cmd;
    if (node)
    {
        cmd = (struct screen_common_s *) node->params;
        if (cmd->ui_id < SCREEN_MAX && ui_create_fns[cmd->ui_id])
        {
            cmd->screen_id = ui_create_fns[cmd->ui_id](cmd);
        }
        else
        {
            cmd->screen_id = 0;
        }
        screen_command_finish(node);
    }
    screen_msg_queue_dispatch();
    return;
}

void ai_main_ui()
{
    lv_style_reset(&g_style);
    lv_style_init(&g_style);
    lv_style_set_bg_color(&g_style, lv_color_make(0x00, 0x00, 0x00));
    lv_style_set_shadow_color(&g_style, lv_color_make(0x00, 0x00, 0x00));
    lv_style_set_border_color(&g_style, lv_color_make(0x00, 0x00, 0x00));
    lv_style_set_outline_color(&g_style, lv_color_make(0x00, 0x00, 0x00));
    lv_style_set_radius(&g_style, 0);
    lv_timer_t        *cmd_timer;
    cmd_timer = lv_timer_create(app_command_poll, 1, NULL);
    lv_timer_set_repeat_count(cmd_timer, -1);
    screen_manager_init();
    screen_command_init();
    ui_manager_init();
    return ;
}
