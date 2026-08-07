/***************************************************************************************
***************************************************************************************/
#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "keyWork.h"
#include "keyScan.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "audio_msi/audio_adc.h"
#include "audio_media_ctrl/audio_code_ctrl.h"
#include "file_process.h"
#include "fs/fatfs/osal_file.h"
#include "file_process.h"
#define RECORDER_DIR "0:MP4"

struct msi *mp4_encode_msi2_init(const char *mp4_msi_name, uint8_t srcID, uint8_t filter_type, uint8_t rec_time, uint32_t audio_encode, struct file_process *file_process, uint8_t mode);

#define LVGL_MP4_MSI_NAME      "lvgl_mp4"
#define LV_OBJ_MP4_RECORD_FLAG LV_OBJ_FLAG_USER_1

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

struct mp4_record_ui_s
{
    lv_group_t *last_group;
    lv_obj_t   *base_ui;
    uint16_t    w, h;

    lv_group_t *now_group;
    lv_obj_t   *now_ui;

    lv_timer_t *timer;
    lv_obj_t   *label_time;

    struct msi *s;
    struct msi *h264_msi;
    struct msi *mp4_msi;
    struct msi *aac_msi;

    uint8_t *play_name;
    uint8_t start : 1, rev : 7;
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
        if (!lv_obj_has_flag(ui_s->now_ui, LV_OBJ_MP4_RECORD_FLAG))
        {
            os_printf("%s:%d\n", __FUNCTION__, __LINE__);
            ui_s->h264_msi = msi_find(AUTO_H264, 1);
            if (ui_s->h264_msi)
            {
                lv_obj_add_flag(ui_s->now_ui, LV_OBJ_MP4_RECORD_FLAG);
                struct file_process mp4_file_process = {
                        .loop        = NULL,
                        .rec_path    = RECORDER_DIR,
                        .ext_name    = ".MP4",
                        .create_file = lvgl_mp4_file,
                        .loop_free   = NULL,
                        .lock_file   = NULL,
                };
                uint8_t    audio_flag = 0;
                AUENC_INIT auenc_init;
                auenc_init.destroy_self = 0;
                auenc_init.src_msi      = get_auadc_msi(AUSYS_AUAD);
                ui_s->aac_msi           = audio_encode_init(AAC_ENC, audio_adc_get_samplerate(AUSYS_AUAD), &auenc_init);
                if (ui_s->aac_msi)
                {
                    audio_code_add_output(ui_s->aac_msi, LVGL_MP4_MSI_NAME);
                    audio_flag = 1;
                }

                ui_s->mp4_msi = mp4_encode_msi2_init(LVGL_MP4_MSI_NAME, FRAMEBUFF_SOURCE_CAMERA0, FSTYPE_H264_VPP_DATA0, 1, audio_flag, &mp4_file_process, 0);

                if (ui_s->mp4_msi)
                {
                    msi_do_cmd(ui_s->mp4_msi, MSI_CMD_MEDIA_CTRL, MSI_MEDIA_CTRL_RECORD_START, 0);
                    msi_add_output(ui_s->h264_msi, NULL, LVGL_MP4_MSI_NAME);
                }
                else
                {
                    if (ui_s->aac_msi)
                    {
                        audio_code_del_output(ui_s->aac_msi, LVGL_MP4_MSI_NAME);
                        audio_encode_deinit(ui_s->aac_msi);
                        ui_s->aac_msi = NULL;
                    }

                    if (ui_s->h264_msi)
                    {
                        msi_del_output(ui_s->h264_msi, NULL, LVGL_MP4_MSI_NAME);
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
                audio_code_del_output(ui_s->aac_msi, LVGL_MP4_MSI_NAME);
                audio_encode_deinit(ui_s->aac_msi);
                ui_s->aac_msi = NULL;
            }

            if (ui_s->h264_msi)
            {
                msi_del_output(ui_s->h264_msi, NULL, LVGL_MP4_MSI_NAME);
                msi_put(ui_s->h264_msi);
                ui_s->h264_msi = NULL;
            }
            lv_obj_clear_flag(ui_s->now_ui, LV_OBJ_MP4_RECORD_FLAG);
        }
    }
}

static void exit_mp4_record_ui(lv_event_t *e)
{
    int32_t c = *((int32_t *) lv_event_get_param(e));
    if (c == 'q')
    {
        struct mp4_record_ui_s *ui_s = (struct mp4_record_ui_s *) lv_event_get_user_data(e);
        lv_indev_set_group(indev_keypad, ui_s->last_group);
        lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
        lv_group_del(ui_s->now_group);
        ui_s->now_group = NULL;
        if(ui_s->s)
        {
            msi_do_cmd(ui_s->s, MSI_CMD_SCALE3_NORMAL, MSI_SCALE3_START, 0);
        }
        msi_destroy(ui_s->s);
        // 关闭VIDEO P0的使能，释放占住的scale3的fb
        msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);

        if (ui_s->mp4_msi)
        {
            msi_destroy(ui_s->mp4_msi);
            ui_s->mp4_msi = NULL;
        }
        if (ui_s->aac_msi)
        {
            audio_code_del_output(ui_s->aac_msi, LVGL_MP4_MSI_NAME);
            audio_encode_deinit(ui_s->aac_msi);
            ui_s->aac_msi = NULL;
        }

        if (ui_s->h264_msi)
        {
            msi_del_output(ui_s->h264_msi, NULL, LVGL_MP4_MSI_NAME);
            msi_put(ui_s->h264_msi);
            ui_s->h264_msi = NULL;
        }
        lv_obj_del(ui_s->now_ui);
        set_lvgl_get_key_func(NULL);
    }
}

static void enter_mp4_record_ui(lv_event_t *e)
{
    set_lvgl_get_key_func(self_key);
    struct mp4_record_ui_s *ui_s = (struct mp4_record_ui_s *) lv_event_get_user_data(e);
    lv_obj_add_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *ui = lv_obj_create(lv_scr_act());
    ui_s->now_ui = ui;
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));
    // 绑定流到Video_P0显示

    ui_s->s = scale3_normal_msi2(S_PREVIEW_SCALE3, FSTYPE_YUV_P0, ui_s->w, ui_s->h);
    if (ui_s->s)
    {
        msi_do_cmd(ui_s->s, MSI_CMD_SCALE3_NORMAL, MSI_SCALE3_START, 0x01);
        msi_add_output(ui_s->s, NULL, R_VIDEO_P0);
        msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);
        ui_s->s->enable = 1;
    }

    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);

    lv_group_add_obj(group, ui);
    ui_s->now_group = group;

    lv_obj_add_event_cb(ui, start_mp4_record_ui, LV_EVENT_KEY, ui_s);
    lv_obj_add_event_cb(ui, exit_mp4_record_ui, LV_EVENT_KEY, ui_s);
}

lv_obj_t *mp4_record_ui(lv_group_t *group, lv_obj_t *base_ui, uint16_t w, uint16_t h)
{
    struct mp4_record_ui_s *ui_s = (struct mp4_record_ui_s *) STREAM_LIBC_ZALLOC(sizeof(struct mp4_record_ui_s));
    ui_s->last_group             = group;
    ui_s->base_ui                = base_ui;
    ui_s->w                      = w;
    ui_s->h                      = h;
    lv_obj_t *btn                = lv_list_add_btn(base_ui, NULL, "mp4_record_ui");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_mp4_record_ui, LV_EVENT_SHORT_CLICKED, ui_s);
    return btn;
}