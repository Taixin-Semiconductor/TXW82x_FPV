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
#include "audio_msi/audio_adc.h"
#include "fs/fatfs/osal_file.h"
#include "app/video_record/video_record.h"

#define RECORDER_DIR "0:MP4"
#define LVGL_MP4_MSI_NAME      "lvgl_mp4"
#define LV_OBJ_MP4_RECORD_FLAG LV_OBJ_FLAG_USER_1

#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

extern lv_indev_t *indev_keypad;
extern lv_style_t  g_style;

struct mp4_record_ui_s
{
    screen_t   *screen;
    lv_group_t *now_group;
    uint16_t    w, h;

    struct msi *s;
    struct msi *h264_msi;
    struct msi *mp4_msi;
    struct msi *aac_msi;
    struct msi *display_msi;

    uint8_t owns_self;
};

static void mp4_record_enter(lv_event_t *e);

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

static void *lvgl_mp4_file(struct file_process *file_process, char *filename, char *file_path, uint32_t file_size)
{
    os_sprintf(filename, "%s/%016d.mp4", file_process->rec_path, (uint32_t) os_jiffies());
    void *fp = osal_fopen_auto((const char *) filename, "w+", 0);
    return fp;
}

static void start_mp4_record_ui(lv_event_t *e)
{
    int32_t                 c    = *((int32_t *) lv_event_get_param(e));
    struct mp4_record_ui_s *ui_s = (struct mp4_record_ui_s *) lv_event_get_user_data(e);
    if (c == LV_KEY_ENTER)
    {
        if (!lv_obj_has_flag(ui_s->screen->root, LV_OBJ_MP4_RECORD_FLAG))
        {
            ui_s->h264_msi = msi_find(AUTO_H264, 1);
            if (ui_s->h264_msi)
            {
                lv_obj_add_flag(ui_s->screen->root, LV_OBJ_MP4_RECORD_FLAG);
                struct file_process mp4_file_process = {
                        .loop        = NULL,
                        .rec_path    = RECORDER_DIR,
                        .ext_name    = ".MP4",
                        .create_file = lvgl_mp4_file,
                        .loop_free   = NULL,
                        .lock_file   = NULL,
                };
                uint8_t       audio_flag = 0;
                txAudioInfo_t codec_info;
                codec_info.sample_rate = 8000;
                codec_info.channels    = 1;
                codec_info.frame_size  = 1024;
                ui_s->aac_msi          = aenc_get_msi(AUDIO_CODEC_AAC, "adc", &codec_info);
                if (ui_s->aac_msi)
                {
                    auadc_msi_add_output(AUSYS_AUAD, ui_s->aac_msi->name);
                    msi_do_cmd(ui_s->aac_msi, MSI_CMD_START, 0, 0);
                    msi_add_output(ui_s->aac_msi, NULL, NULL, LVGL_MP4_MSI_NAME);
                    audio_flag = 1;
                }

                struct video_record_cfg rec_cfg = {
                    .name = LVGL_MP4_MSI_NAME,
                    .srcID = FRAMEBUFF_SOURCE_CAMERA0,
                    .filter = FSTYPE_H264_VPP_DATA0,
                    .rec_time = 1,
                    .audio_en = audio_flag,
                    .mode = MP4_MODE_NORMAL,
                    .video_fps = 25,
                    .file_process = &mp4_file_process,
                };
                ui_s->mp4_msi = mp4_record_msi_init(&rec_cfg);

                if (ui_s->mp4_msi)
                {
                    msi_do_cmd(ui_s->mp4_msi, MSI_CMD_MEDIA_CTRL, MSI_MEDIA_CTRL_RECORD_START, 0);
                    msi_add_output(ui_s->h264_msi, NULL, NULL, LVGL_MP4_MSI_NAME);
                }
                else
                {
                    if (ui_s->aac_msi)
                    {
                        msi_del_output(ui_s->aac_msi, NULL, NULL, LVGL_MP4_MSI_NAME);
                        msi_put(ui_s->aac_msi);
                        ui_s->aac_msi = NULL;
                    }

                    if (ui_s->h264_msi)
                    {
                        msi_del_output(ui_s->h264_msi, NULL, NULL, LVGL_MP4_MSI_NAME);
                        msi_put(ui_s->h264_msi);
                        ui_s->h264_msi = NULL;
                    }
                }
            }
        }
        else
        {
            if (ui_s->mp4_msi)
            {
                msi_destroy(ui_s->mp4_msi);
                ui_s->mp4_msi = NULL;
            }
            if (ui_s->aac_msi)
            {
                msi_del_output(ui_s->aac_msi, NULL, NULL, LVGL_MP4_MSI_NAME);
                msi_put(ui_s->aac_msi);
                ui_s->aac_msi = NULL;
            }

            if (ui_s->h264_msi)
            {
                msi_del_output(ui_s->h264_msi, NULL, NULL, LVGL_MP4_MSI_NAME);
                msi_put(ui_s->h264_msi);
                ui_s->h264_msi = NULL;
            }
            lv_obj_clear_flag(ui_s->screen->root, LV_OBJ_MP4_RECORD_FLAG);
        }
    }
}

static void exit_mp4_record_ui(lv_event_t *e)
{
    int32_t c = *((int32_t *) lv_event_get_param(e));
    if (c == 'q')
    {
        struct mp4_record_ui_s *ui_s = (struct mp4_record_ui_s *) lv_event_get_user_data(e);
        if (ui_s->screen)
        {
            screen_finish(ui_s->screen);
        }
    }
}

static void mp4_record_create(screen_t *screen, void *params)
{
    struct mp4_record_ui_s *ui_s = (struct mp4_record_ui_s *) params;

    if (!ui_s)
    {
        ui_s = (struct mp4_record_ui_s *) SCREEN_MALLOC(sizeof(struct mp4_record_ui_s));
        if (!ui_s)
        {
            return;
        }
        memset(ui_s, 0, sizeof(struct mp4_record_ui_s));
        ui_s->owns_self = 1;
    }

    ui_s->screen = screen;
    screen_set_user_data(screen, ui_s);

    lv_obj_t *ui = screen->root;
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));

    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);
    lv_group_add_obj(group, ui);
    ui_s->now_group = group;

    lv_obj_add_event_cb(ui, start_mp4_record_ui, LV_EVENT_KEY, ui_s);
    lv_obj_add_event_cb(ui, exit_mp4_record_ui, LV_EVENT_KEY, ui_s);
}

static void mp4_record_start(screen_t *screen)
{
    struct mp4_record_ui_s *ui_s;

    if (!screen)
    {
        return;
    }

    ui_s = (struct mp4_record_ui_s *) screen_get_user_data(screen);
    if (!ui_s)
    {
        return;
    }

    set_lvgl_get_key_func(self_key);

    if (!ui_s->s)
    {
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
    }
}

static void mp4_record_resume(screen_t *screen)
{
    struct mp4_record_ui_s *ui_s;

    if (!screen)
    {
        return;
    }

    ui_s = (struct mp4_record_ui_s *) screen_get_user_data(screen);
    if (!ui_s)
    {
        return;
    }

    if (ui_s->now_group && indev_keypad)
    {
        set_lvgl_get_key_func(self_key);
        lv_indev_set_group(indev_keypad, ui_s->now_group);
        if (!lv_group_get_focused(ui_s->now_group))
        {
            lv_group_focus_next(ui_s->now_group);
        }
    }
}

static void mp4_record_pause(screen_t *screen)
{
    struct mp4_record_ui_s *ui_s;

    if (!screen)
    {
        return;
    }

    ui_s = (struct mp4_record_ui_s *) screen_get_user_data(screen);
    if (!ui_s)
    {
        return;
    }

    set_lvgl_get_key_func(NULL);
}

static void mp4_record_stop(screen_t *screen)
{
    (void) screen;
}

static void mp4_record_destroy(screen_t *screen)
{
    struct mp4_record_ui_s *ui_s;

    if (!screen)
    {
        return;
    }

    ui_s = (struct mp4_record_ui_s *) screen_get_user_data(screen);
    if (!ui_s)
    {
        return;
    }

    if (ui_s->mp4_msi)
    {
        msi_destroy(ui_s->mp4_msi);
        ui_s->mp4_msi = NULL;
    }
    if (ui_s->aac_msi)
    {
        msi_del_output(ui_s->aac_msi, NULL, NULL, LVGL_MP4_MSI_NAME);
        msi_put(ui_s->aac_msi);
        ui_s->aac_msi = NULL;
    }
    if (ui_s->h264_msi)
    {
        msi_del_output(ui_s->h264_msi, NULL, NULL, LVGL_MP4_MSI_NAME);
        msi_put(ui_s->h264_msi);
        ui_s->h264_msi = NULL;
    }

    if (ui_s->display_msi)
    {
        msi_put(ui_s->display_msi);
        ui_s->display_msi = NULL;
    }
    if (ui_s->s)
    {
        msi_do_cmd(ui_s->s, MSI_CMD_SCALE3_NORMAL, MSI_SCALE3_START, 0);
        msi_destroy(ui_s->s);
        ui_s->s = NULL;
    }
    msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);

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

static void mp4_record_restart(screen_t *screen)
{
    (void) screen;
}

static void mp4_record_finish(screen_t *screen)
{
    (void) screen;
}

static void mp4_record_get_lifecycle(screen_lifecycle_t *lifecycle)
{
    if (!lifecycle)
    {
        return;
    }

    *lifecycle = (screen_lifecycle_t) {
            .on_create  = mp4_record_create,
            .on_start   = mp4_record_start,
            .on_resume  = mp4_record_resume,
            .on_pause   = mp4_record_pause,
            .on_stop    = mp4_record_stop,
            .on_destroy = mp4_record_destroy,
            .on_restart = mp4_record_restart,
            .on_finish  = mp4_record_finish,
    };
}

static void mp4_record_enter(lv_event_t *e)
{
    struct mp4_record_ui_s *ui_s = (struct mp4_record_ui_s *) lv_event_get_user_data(e);
    screen_lifecycle_t      lifecycle;

    if (!ui_s)
    {
        return;
    }

    if (!ui_s->screen)
    {
        mp4_record_get_lifecycle(&lifecycle);
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

lv_obj_t *mp4_record_ui(lv_group_t *group, lv_obj_t *base_ui, uint16_t w, uint16_t h)
{
    struct mp4_record_ui_s *ui_s = (struct mp4_record_ui_s *) STREAM_LIBC_ZALLOC(sizeof(struct mp4_record_ui_s));
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

    lv_obj_t *btn = lv_list_add_btn(base_ui, NULL, "mp4_record_ui");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, mp4_record_enter, LV_EVENT_SHORT_CLICKED, ui_s);
    return btn;
}
