/***************************************************************************************
***************************************************************************************/
#include "app/screen/screen_memory.h"
#include "app/screen/screen_manager.h"
#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "keyWork.h"
#include "keyScan.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "avi_player_msi.h"
#include "hal/vdd.h"
#include "lib/multimedia/txmplayer.h"

#define CHECK_DIR               "MP4"
#define EXT_NAME                "*mp4"
#define PLAY_AVI_STREAM_NAME    (64)
#define ROUTE_PLAYER_MSI_TX_NUM (16)

enum
{
    PLAY_STATUS_STOP_PLAY = BIT(0),
    PLAY_STATUS_PLAY_1FPS = BIT(1),
    PLAY_STATUS_PLAY_END  = BIT(2),
};

#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

extern lv_style_t  g_style;
extern lv_indev_t *indev_keypad;

struct mp4_player_ui_s
{
    screen_t   *screen;
    lv_group_t *now_group;
    uint16_t    w, h;

    lv_timer_t *timer;
    lv_obj_t   *label_time;

    uint8_t *play_name;
    char     play_time_buf[32];
    int32_t  txmplayer_streamid;
    uint8_t  start : 1, owns_self : 1, rev : 6;
};

struct avi_list_param
{
    lv_group_t             *group;
    lv_obj_t               *ui;
    struct mp4_player_ui_s *ui_s;
};

typedef int (*creat_avi_list_ui)(const char *filename, void *data);

static void mp4_player_enter(lv_event_t *e);

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

static void exit_show_filelist(lv_event_t *e)
{
    set_lvgl_get_key_func(self_key);
    struct mp4_player_ui_s *ui_s      = (struct mp4_player_ui_s *) lv_event_get_user_data(e);
    lv_obj_t               *list_item = lv_event_get_current_target(e);
    if (ui_s->now_group)
    {
        lv_indev_set_group(indev_keypad, ui_s->now_group);
    }
    if (list_item->user_data)
    {
        lv_obj_del_async(list_item->user_data);
    }
}

static void enter_playback(lv_event_t *e)
{
    set_lvgl_get_key_func(self_key);
    struct mp4_player_ui_s *ui_s      = (struct mp4_player_ui_s *) lv_event_get_user_data(e);
    lv_obj_t               *list_item = lv_event_get_current_target(e);
    lv_obj_t               *list      = list_item->user_data;

    if (ui_s->play_name)
    {
        STREAM_FREE(ui_s->play_name);
        ui_s->play_name = NULL;
    }

    os_printf("ui_s->label_time:%X\n", ui_s->label_time);
    lv_label_set_text(ui_s->label_time, "00:00:00/00:00:00");

    char        path[64];
    const char *filename = lv_list_get_btn_text(list, list_item);
    os_printf("play filename:%s\n", filename);
    os_sprintf(path, "/sd0/%s/%s", CHECK_DIR, filename);
    ui_s->play_name = (uint8_t *) STREAM_MALLOC(PLAY_AVI_STREAM_NAME);
    os_sprintf((char *) ui_s->play_name, "%s_%04d", filename, (uint32_t) os_jiffies());
    os_printf("struct msi play_name:%s addr:0x%x\n", ui_s->play_name, ui_s->play_name);
    if (ui_s->txmplayer_streamid < 0)
    {
        ui_s->txmplayer_streamid = txmplayer_open(path, 0, NULL);
        struct vdd_rect vdd;
        vdd.width  = ui_s->w;
        vdd.height = ui_s->h;
        vdd.x      = 0;
        vdd.y      = 0;
        txmplayer_msi_cmd(ui_s->txmplayer_streamid, MSI_CMD_SET_VDD_RECT, (uint32_t) &vdd, 0);
    }
    else
    {
        txmplayer_reopen(ui_s->txmplayer_streamid, path);
    }
    ui_s->start = 1;
    exit_show_filelist(e);
}

static int mp4_list_show(const char *filename, void *data)
{
    struct avi_list_param *param = (struct avi_list_param *) data;
    lv_obj_t              *btn   = lv_list_add_btn(param->ui, NULL, (const char *) filename);
    btn->user_data               = (void *) param->ui;
    if (param->group)
    {
        lv_group_add_obj(param->group, btn);
    }

    lv_obj_add_event_cb(btn, enter_playback, LV_EVENT_CLICKED, param->ui_s);
    return 0;
}

static int each_read_file(creat_avi_list_ui fn, void *param)
{
    DIR     dir;
    FILINFO finfo;
    FRESULT fr;
    if (!fn)
    {
        goto each_read_file_end;
    }

    fr = f_findfirst(&dir, &finfo, CHECK_DIR, EXT_NAME);

    while (fr == FR_OK && finfo.fname[0] != 0)
    {
        fn(finfo.fname, param);
        fr = f_findnext(&dir, &finfo);
    }

    f_closedir(&dir);
each_read_file_end:
    os_printf("%s:%d\tret:%d\n", __FUNCTION__, __LINE__, fr);
    return fr;
}

static void clear_list_group_ui(lv_event_t *e)
{
    lv_group_t *group = (lv_group_t *) lv_event_get_user_data(e);
    if (group)
    {
        lv_group_del(group);
    }
}

static void show_filelist(lv_event_t *e)
{
    int32_t c = *((int32_t *) lv_event_get_param(e));
    if (c == 'e')
    {
        set_lvgl_get_key_func(NULL);
        struct mp4_player_ui_s *ui_s = (struct mp4_player_ui_s *) lv_event_get_user_data(e);
        struct avi_list_param   param;
        lv_obj_t               *list = lv_list_create(ui_s->screen->root);
        lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
        param.ui_s  = ui_s;
        param.ui    = list;
        param.group = lv_group_create();
        lv_indev_set_group(indev_keypad, param.group);
        lv_obj_t *list_item  = lv_list_add_btn(list, NULL, "exit");
        list_item->user_data = (void *) list;
        lv_group_add_obj(param.group, list_item);
        lv_obj_add_event_cb(list_item, exit_show_filelist, LV_EVENT_SHORT_CLICKED, ui_s);

        each_read_file(mp4_list_show, (void *) &param);

        lv_obj_add_event_cb(list, clear_list_group_ui, LV_EVENT_DELETE, param.group);
    }
}

static void player_locate_ctrl_ui(lv_event_t *e)
{
    int32_t c = *((int32_t *) lv_event_get_param(e));
    switch (c)
    {
        case 'a':
        {
        }
        break;
        case 'd':
        {
        }
        break;
        default:
            break;
    }
}

static void player_ctrl_ui(lv_event_t *e)
{
    struct mp4_player_ui_s *ui_s = (struct mp4_player_ui_s *) lv_event_get_user_data(e);
    ui_s->start++;
    if (ui_s->start)
    {
    }
    else
    {
    }
}

static void exit_player_ui(lv_event_t *e)
{
    int32_t c = *((int32_t *) lv_event_get_param(e));
    if (c == 'q')
    {
        struct mp4_player_ui_s *ui_s = (struct mp4_player_ui_s *) lv_event_get_user_data(e);
        if (ui_s->screen)
        {
            screen_finish(ui_s->screen);
        }
    }
}

static void player_ui_timer(lv_timer_t *t)
{
    struct mp4_player_ui_s *ui_s = (struct mp4_player_ui_s *) t->user_data;
    uint32_t                play_time, tot_time;
    uint32_t                h, m, s;
    uint32_t                h1, m1, s1;
    if (ui_s->txmplayer_streamid >= 0)
    {
        play_time = txmplayer_playtime(ui_s->txmplayer_streamid, &tot_time);
        play_time /= 1000;
        tot_time /= 1000;
        h  = play_time / 3600;
        m  = (play_time % 3600) / 60;
        s  = play_time % 60;
        h1 = tot_time / 3600;
        m1 = (tot_time % 3600) / 60;
        s1 = tot_time % 60;
        char time_label[32];
        os_sprintf(time_label, "%02d:%02d:%02d/%02d:%02d:%02d", h, m, s, h1, m1, s1);
        if (!(strcmp(time_label, ui_s->play_time_buf) == 0))
        {
            lv_label_set_text(ui_s->label_time, time_label);
            memcpy(ui_s->play_time_buf, time_label, strlen(time_label) + 1);
        }
    }
}

static void mp4_player_create(screen_t *screen, void *params)
{
    struct mp4_player_ui_s *ui_s = (struct mp4_player_ui_s *) params;

    if (!ui_s)
    {
        ui_s = (struct mp4_player_ui_s *) SCREEN_MALLOC(sizeof(struct mp4_player_ui_s));
        if (!ui_s)
        {
            return;
        }
        memset(ui_s, 0, sizeof(struct mp4_player_ui_s));
        ui_s->txmplayer_streamid = -1;
        ui_s->owns_self          = 1;
    }

    ui_s->screen = screen;
    screen_set_user_data(screen, ui_s);

    lv_obj_t *ui = screen->root;
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));

    ui_s->label_time = lv_label_create(ui);
    lv_label_set_text(ui_s->label_time, "00:00:00/00:00:00");
    lv_obj_align(ui_s->label_time, LV_ALIGN_TOP_MID, 0, 0);
    ui_s->timer = lv_timer_create(player_ui_timer, 100, (void *) ui_s);

    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);
    lv_group_add_obj(group, ui);
    ui_s->now_group = group;
    lv_obj_add_event_cb(ui, exit_player_ui, LV_EVENT_KEY, ui_s);
    lv_obj_add_event_cb(ui, player_ctrl_ui, LV_EVENT_SHORT_CLICKED, ui_s);
    lv_obj_add_event_cb(ui, player_locate_ctrl_ui, LV_EVENT_KEY, ui_s);
    lv_obj_add_event_cb(ui, show_filelist, LV_EVENT_KEY, ui_s);
}

static void mp4_player_start(screen_t *screen)
{
    struct mp4_player_ui_s *ui_s;

    if (!screen)
    {
        return;
    }

    ui_s = (struct mp4_player_ui_s *) screen_get_user_data(screen);
    if (!ui_s)
    {
        return;
    }
}

static void mp4_player_resume(screen_t *screen)
{
    struct mp4_player_ui_s *ui_s;

    if (!screen)
    {
        return;
    }

    ui_s = (struct mp4_player_ui_s *) screen_get_user_data(screen);
    if (!ui_s)
    {
        return;
    }
    ui_s = (struct mp4_player_ui_s *) screen_get_user_data(screen);
    if (ui_s && indev_keypad && ui_s->now_group)
    {
        set_lvgl_get_key_func(self_key);
        lv_indev_set_group(indev_keypad, ui_s->now_group);
        if (!lv_group_get_focused(ui_s->now_group))
        {
            lv_group_focus_next(ui_s->now_group);
        }
    }
}

static void mp4_player_pause(screen_t *screen)
{
    struct mp4_player_ui_s *ui_s;

    if (!screen)
    {
        return;
    }

    ui_s = (struct mp4_player_ui_s *) screen_get_user_data(screen);
    if (!ui_s)
    {
        return;
    }

    set_lvgl_get_key_func(NULL);
}

static void mp4_player_stop(screen_t *screen)
{
    (void) screen;
}

static void mp4_player_destroy(screen_t *screen)
{
    struct mp4_player_ui_s *ui_s;

    if (!screen)
    {
        return;
    }

    ui_s = (struct mp4_player_ui_s *) screen_get_user_data(screen);
    if (!ui_s)
    {
        return;
    }

    if (ui_s->timer)
    {
        lv_timer_del(ui_s->timer);
        ui_s->timer = NULL;
    }

    if (ui_s->txmplayer_streamid >= 0)
    {
        txmplayer_close(ui_s->txmplayer_streamid);
        ui_s->txmplayer_streamid = -1;
    }

    if (ui_s->play_name)
    {
        STREAM_FREE(ui_s->play_name);
        ui_s->play_name = NULL;
    }

    if (ui_s->now_group)
    {
        lv_group_del(ui_s->now_group);
        ui_s->now_group = NULL;
    }

    if (ui_s)
    {
        ui_s->screen = NULL;
        if (ui_s->owns_self)
        {
            SCREEN_FREE(ui_s);
        }
    }
    screen_set_user_data(screen, NULL);
}

static void mp4_player_restart(screen_t *screen)
{
    (void) screen;
}

static void mp4_player_finish(screen_t *screen)
{
    (void) screen;
}

static void mp4_player_get_lifecycle(screen_lifecycle_t *lifecycle)
{
    if (!lifecycle)
    {
        return;
    }

    *lifecycle = (screen_lifecycle_t) {
            .on_create  = mp4_player_create,
            .on_start   = mp4_player_start,
            .on_resume  = mp4_player_resume,
            .on_pause   = mp4_player_pause,
            .on_stop    = mp4_player_stop,
            .on_destroy = mp4_player_destroy,
            .on_restart = mp4_player_restart,
            .on_finish  = mp4_player_finish,
    };
}

static void mp4_player_enter(lv_event_t *e)
{
    struct mp4_player_ui_s *ui_s = (struct mp4_player_ui_s *) lv_event_get_user_data(e);
    screen_lifecycle_t      lifecycle;

    if (!ui_s)
    {
        return;
    }

    if (!ui_s->screen)
    {
        mp4_player_get_lifecycle(&lifecycle);
        ui_s->screen = screen_register(NULL, &lifecycle);
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

lv_obj_t *mp4_player_ui(lv_group_t *group, lv_obj_t *base_ui, uint16_t w, uint16_t h)
{
    struct mp4_player_ui_s *ui_s = (struct mp4_player_ui_s *) STREAM_LIBC_ZALLOC(sizeof(struct mp4_player_ui_s));
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
    ui_s->txmplayer_streamid = -1;

    lv_obj_t *btn = lv_list_add_btn(base_ui, NULL, "mp4_player");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, mp4_player_enter, LV_EVENT_SHORT_CLICKED, ui_s);
    return btn;
}
