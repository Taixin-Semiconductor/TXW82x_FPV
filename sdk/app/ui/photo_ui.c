/***************************************************************************************
***************************************************************************************/
#include "app/screen/screen_memory.h"
#include "app/screen/screen.h"
#include "app/screen/screen_manager.h"
#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "stream_define.h"
#include "keyWork.h"
#include "keyScan.h"
#include "fatfs/osal_file.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "hal/vdd.h"
#include <string.h>
#include "lib/multimedia/txmplayer.h"

#define CHECK_DIR "IMG"
#define EXT_NAME  "*jpg"

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

struct photo_ui_s
{
    screen_t   *screen;
    lv_obj_t   *base_ui;
    lv_group_t *now_group;
    lv_obj_t   *now_ui;
    lv_obj_t   *label_path;
    int32_t     txmplayer_streamid;
    uint8_t     owns_self;
};

struct photo_list_param
{
    lv_group_t        *group;
    lv_obj_t          *ui;
    struct photo_ui_s *ui_s;
};
typedef int (*list_ui)(const char *filename, void *data);

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
    os_printf("%s:%d\n", __FUNCTION__, __LINE__);
    set_lvgl_get_key_func(self_key);
    struct photo_ui_s *ui_s      = (struct photo_ui_s *) lv_event_get_user_data(e);
    lv_obj_t          *list_item = lv_event_get_current_target(e);
    if (ui_s->now_group)
    {
        lv_indev_set_group(indev_keypad, ui_s->now_group);
    }
    if (list_item->user_data)
    {
        // 由于是子控件回调函数需要删除父控件,所以需要用到异步删除,否则删除内部链表会有异常
        lv_obj_del_async(list_item->user_data);
    }
}

static void photo_group_enter(struct photo_ui_s *ui_s)
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

static void photo_group_leave(struct photo_ui_s *ui_s)
{
    if (!ui_s || !ui_s->now_group)
    {
        return;
    }

    lv_group_del(ui_s->now_group);
    ui_s->now_group = NULL;
}

// 读取图片,发送出去显示
static void play_photo(lv_event_t *e)
{
    set_lvgl_get_key_func(self_key);
    struct photo_ui_s *ui_s      = (struct photo_ui_s *) lv_event_get_user_data(e);
    lv_obj_t          *list_item = lv_event_get_current_target(e);
    lv_obj_t          *list      = list_item->user_data;
    char               path[64];
    const char        *filename = lv_list_get_btn_text(list, list_item);
    os_sprintf(path, "/sd0/%s/%s", CHECK_DIR, filename);
    os_printf("path:%s\n", path);
    lv_label_set_text(ui_s->label_path, path);
    if (ui_s->txmplayer_streamid < 0)
    {
        ui_s->txmplayer_streamid = txmplayer_open(path, 0, NULL);
        struct vdd_rect vdd;
        vdd.width  = 0;
        vdd.height = 0;
        vdd.x      = 0;
        vdd.y      = 0;
        txmplayer_msi_cmd(ui_s->txmplayer_streamid, MSI_CMD_SET_VDD_RECT, (uint32_t) &vdd, 0);
    }
    else
    {
        txmplayer_reopen(ui_s->txmplayer_streamid, path);
    }
    exit_show_filelist(e);
}

static int photo_list_show(const char *filename, void *data)
{
    struct photo_list_param *param = (struct photo_list_param *) data;
    lv_obj_t                *btn   = lv_list_add_btn(param->ui, NULL, (const char *) filename);
    btn->user_data                 = (void *) param->ui;
    if (param->group)
    {
        lv_group_add_obj(param->group, btn);
    }

    // 回调函数,进入回放功能
    lv_obj_add_event_cb(btn, play_photo, LV_EVENT_CLICKED, param->ui_s);
    return 0;
}

// 遍历文件夹
// 遍历文件夹
static int each_read_file(list_ui fn, void *param)
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
        if (finfo.fsize > 0)
        {
            fn(finfo.fname, param);
        }

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
    os_printf("group:%X\n", group);
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
        struct photo_ui_s      *ui_s = (struct photo_ui_s *) lv_event_get_user_data(e);
        struct photo_list_param param;
        lv_obj_t               *list = lv_list_create(ui_s->now_ui);
        lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
        param.ui_s  = ui_s;
        param.ui    = list;
        param.group = lv_group_create();
        lv_indev_set_group(indev_keypad, param.group);
        // 创建exit的控件
        lv_obj_t *list_item  = lv_list_add_btn(list, NULL, "exit");
        list_item->user_data = (void *) list;
        lv_group_add_obj(param.group, list_item);
        lv_obj_add_event_cb(list_item, exit_show_filelist, LV_EVENT_SHORT_CLICKED, ui_s);

        each_read_file(photo_list_show, (void *) &param);

        lv_obj_add_event_cb(list, clear_list_group_ui, LV_EVENT_DELETE, param.group);
    }
}

static void exit_player_ui(lv_event_t *e)
{
    int32_t c = *((int32_t *) lv_event_get_param(e));
    if (c == 'q')
    {
        struct photo_ui_s *ui_s = (struct photo_ui_s *) lv_event_get_user_data(e);
        if (ui_s->txmplayer_streamid >= 0)
        {
            txmplayer_close(ui_s->txmplayer_streamid);
            ui_s->txmplayer_streamid = -1;
        }
        set_lvgl_get_key_func(NULL);
        if (ui_s->screen)
        {
            screen_finish(ui_s->screen);
        }
    }
}

static void photo_create(screen_t *screen, void *params)
{
    struct photo_ui_s *ui_s = (struct photo_ui_s *) params;

    if (!ui_s)
    {
        ui_s = (struct photo_ui_s *) SCREEN_MALLOC(sizeof(struct photo_ui_s));
        if (!ui_s)
        {
            return;
        }
        memset(ui_s, 0, sizeof(struct photo_ui_s));
        ui_s->txmplayer_streamid = -1;
        ui_s->owns_self          = 1;
    }

    ui_s->screen = screen;
    screen_set_user_data(screen, ui_s);

    lv_obj_t *ui = screen->root;
    ui_s->now_ui = ui;
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));

    ui_s->label_path = lv_label_create(ui);
    lv_label_set_text(ui_s->label_path, "");
    lv_obj_align(ui_s->label_path, LV_ALIGN_TOP_MID, 0, 0);

    // 弹出菜单栏,用于切换分辨率
    lv_obj_add_event_cb(ui, show_filelist, LV_EVENT_KEY, ui_s);
    lv_obj_add_event_cb(ui, exit_player_ui, LV_EVENT_KEY, ui_s);

    ui_s->now_group = lv_group_create();
    lv_group_add_obj(ui_s->now_group, ui);
}

static void photo_start(screen_t *screen)
{
    struct photo_ui_s *ui_s;

    if (!screen)
    {
        return;
    }

    ui_s = (struct photo_ui_s *) screen_get_user_data(screen);
    set_lvgl_get_key_func(self_key);
}

static void photo_resume(screen_t *screen)
{
    (void) screen;
    struct photo_ui_s *ui_s;
    ui_s = (struct photo_ui_s *) screen_get_user_data(screen);
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

static void photo_pause(screen_t *screen)
{
    struct photo_ui_s *ui_s = (struct photo_ui_s *) screen_get_user_data(screen);
    photo_group_leave(ui_s);
    set_lvgl_get_key_func(NULL);
}

static void photo_stop(screen_t *screen)
{
    (void) screen;
}

static void photo_destroy(screen_t *screen)
{
    struct photo_ui_s *ui_s = (struct photo_ui_s *) screen_get_user_data(screen);

    photo_group_leave(ui_s);
    if (ui_s && ui_s->txmplayer_streamid >= 0)
    {
        txmplayer_close(ui_s->txmplayer_streamid);
        ui_s->txmplayer_streamid = -1;
    }
    if (ui_s)
    {
        ui_s->screen     = NULL;
        ui_s->now_ui     = NULL;
        ui_s->label_path = NULL;
        if (ui_s->owns_self)
        {
            SCREEN_FREE(ui_s);
        }
    }
    screen_set_user_data(screen, NULL);
}

static void photo_restart(screen_t *screen)
{
    (void) screen;
}

static void photo_finish(screen_t *screen)
{
    (void) screen;
}

static void photo_get_lifecycle(screen_lifecycle_t *lifecycle)
{
    if (!lifecycle)
    {
        return;
    }

    *lifecycle = (screen_lifecycle_t) {
            .on_create  = photo_create,
            .on_start   = photo_start,
            .on_resume  = photo_resume,
            .on_pause   = photo_pause,
            .on_stop    = photo_stop,
            .on_destroy = photo_destroy,
            .on_restart = photo_restart,
            .on_finish  = photo_finish,
    };
}

static void enter_player_ui(lv_event_t *e)
{
    struct photo_ui_s *ui_s = (struct photo_ui_s *) lv_event_get_user_data(e);
    screen_lifecycle_t lifecycle;

    if (!ui_s)
    {
        return;
    }

    if (!ui_s->screen)
    {
        photo_get_lifecycle(&lifecycle);
        ui_s->screen = screen_register("photo", &lifecycle);
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

lv_obj_t *photo_ui(lv_group_t *group, lv_obj_t *base_ui)
{
    struct photo_ui_s *ui_s = (struct photo_ui_s *) STREAM_LIBC_ZALLOC(sizeof(struct photo_ui_s));
    if (!ui_s)
    {
        return NULL;
    }
    ui_s->base_ui            = base_ui;
    ui_s->txmplayer_streamid = -1;

    lv_obj_t *btn = lv_list_add_btn(base_ui, NULL, "photo");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_player_ui, LV_EVENT_SHORT_CLICKED, ui_s);
    return btn;
}
