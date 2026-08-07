/***************************************************************************************
***************************************************************************************/
#include "lvgl/lvgl.h"
#include "lvgl_ui.h"
#include "keyWork.h"
#include "keyScan.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "avi_player_msi.h"
#include "audio_media_ctrl/audio_code_ctrl.h"
#include "fs/fatfs/osal_file.h"

struct msi *h264_decode_msg_msi(const char *name, uint16_t out_w, uint16_t out_h, uint16_t step_w, uint16_t step_h, uint32_t filter);
struct msi *h264_decode_msi(const char *name, uint8_t only_I_H264);
struct msi *mp4_demux_msi_init(const char *msi_name, const char *filename);
void        h264_drv_init();

#define CHECK_DIR               "MP4"
#define EXT_NAME                "*mp4"
#define PLAY_AVI_STREAM_NAME    (64)
#define ROUTE_PLAYER_MSI_TX_NUM (16)

enum
{
    PLAY_STATUS_STOP_PLAY = BIT(0),
    PLAY_STATUS_PLAY_1FPS = BIT(1), // 播放一帧,然后自动暂停
    PLAY_STATUS_PLAY_END  = BIT(2), // 播放完毕,需要重新播放
};

// 转发流的播放流结构体
struct player_forward_stream_s
{
    struct os_work    work;
    struct msi       *s;
    struct framebuff *data_s;

    uint32_t magic;
    uint32_t start_time;
    uint32_t last_fps_play_time; // 上一帧播放的时间(非系统时间,是视频帧的时间)
    uint8_t  play_status;        // 自己的播放状态,第一bit代表播放或者暂停
};

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

struct mp4_player_ui_s
{
    lv_group_t *last_group;
    lv_obj_t   *base_ui;
    uint16_t    w, h;

    lv_group_t *now_group;
    lv_obj_t   *now_ui;

    lv_timer_t *timer;
    lv_obj_t   *label_time;

    struct msi *P0_jpg_msi;
    struct msi *decode_msi;
    struct msi *mp4_s;
    struct msi *aac_msi;

    uint8_t *play_name;
    uint8_t  start : 1, rev : 7;
};

struct avi_list_param
{
    lv_group_t             *group;
    lv_obj_t               *ui;
    struct mp4_player_ui_s *ui_s;
};

typedef int (*creat_avi_list_ui)(const char *filename, void *data);

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
    struct mp4_player_ui_s *ui_s      = (struct mp4_player_ui_s *) lv_event_get_user_data(e);
    lv_obj_t               *list_item = lv_event_get_current_target(e);
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

// 进入回放
static void enter_playback(lv_event_t *e)
{
    set_lvgl_get_key_func(self_key);
    struct mp4_player_ui_s *ui_s      = (struct mp4_player_ui_s *) lv_event_get_user_data(e);
    lv_obj_t               *list_item = lv_event_get_current_target(e);
    lv_obj_t               *list      = list_item->user_data;
    // 将当前的流关闭,内部判断是否=NULL,这里就不判断了

    if (ui_s->mp4_s)
    {
        msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);
        msi_destroy(ui_s->mp4_s);
        ui_s->mp4_s = NULL;
    }

#if AUDIO_EN
    if (ui_s->aac_msi)
    {
        // msi_destroy(ui_s->aac_msi);
        audio_decode_deinit(ui_s->aac_msi);
        ui_s->aac_msi = NULL;
    }
#endif
    if (ui_s->play_name)
    {
        STREAM_FREE(ui_s->play_name);
        ui_s->play_name = NULL;
    }

    os_printf("ui_s->label_time:%X\n", ui_s->label_time);
    lv_label_set_text(ui_s->label_time, "00:00");

    // 重新打开一个视频文件
    char        path[64];
    const char *filename = lv_list_get_btn_text(list, list_item);
    os_printf("play filename:%s\n", filename);
    os_sprintf(path, "0:%s/%s", CHECK_DIR, filename);

    ui_s->play_name = (uint8_t *) STREAM_MALLOC(PLAY_AVI_STREAM_NAME);
    os_sprintf((char *) ui_s->play_name, "%s_%04d", filename, (uint32_t) os_jiffies());
    os_printf("struct msi play_name:%s addr:0x%x\n", ui_s->play_name, ui_s->play_name);
    ui_s->mp4_s = mp4_demux_msi_init((const char *) ui_s->play_name, (const char *) path);
    if (ui_s->mp4_s)
    {
        msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);
#if AUDIO_EN
        AUDEC_INIT audec_init;
        audec_init.track_type    = MEDIA_TRACK;
        audec_init.priority      = play_interruptible;
        audec_init.direct_to_dac = 1;
        audec_init.use_tpc       = 0;
        audec_init.destroy_self  = 0;
        audec_init.src_msi       = ui_s->mp4_s;
        ui_s->aac_msi            = audio_decode_init(AAC_DEC, 0, &audec_init);
#endif
        // 文件开始解析
        msi_add_output(ui_s->mp4_s, NULL, ui_s->P0_jpg_msi->name);
        msi_do_cmd(ui_s->mp4_s, MSI_CMD_VIDEO_DEMUX_CTRL, MSI_VIDEO_DEMUX_START, 0);
        ui_s->start = 1;
    }
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

    // 回调函数,进入回放功能
    lv_obj_add_event_cb(btn, enter_playback, LV_EVENT_CLICKED, param->ui_s);
    return 0;
}

// 遍历文件夹
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
        struct mp4_player_ui_s *ui_s = (struct mp4_player_ui_s *) lv_event_get_user_data(e);
        struct avi_list_param   param;
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

        each_read_file(mp4_list_show, (void *) &param);

        lv_obj_add_event_cb(list, clear_list_group_ui, LV_EVENT_DELETE, param.group);
    }
}

static void player_locate_ctrl_ui(lv_event_t *e)
{
    // struct mp4_player_ui_s *ui_s = (struct mp4_player_ui_s *) lv_event_get_user_data(e);
    int32_t c = *((int32_t *) lv_event_get_param(e));
    // printf("c:%d\n",c);
    switch (c)
    {
        // 后退
        case 'a':
        {
        }

        break;
        // 前进
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
    if (ui_s->mp4_s)
    {
        ui_s->start++;

        if (ui_s->start)
        {
            msi_do_cmd(ui_s->mp4_s, MSI_CMD_VIDEO_DEMUX_CTRL, MSI_VIDEO_DEMUX_START, 0);
        }
        else
        {
            msi_do_cmd(ui_s->mp4_s, MSI_CMD_VIDEO_DEMUX_CTRL, MSI_VIDEO_DEMUX_PAUSE, 0);
        }
    }
}
static void exit_player_ui(lv_event_t *e)
{
    struct mp4_player_ui_s *ui_s = (struct mp4_player_ui_s *) lv_event_get_user_data(e);
    int32_t                 c    = *((int32_t *) lv_event_get_param(e));
    if (c == 'q')
    {
        lv_timer_del(ui_s->timer);
        struct mp4_player_ui_s *ui_s = (struct mp4_player_ui_s *) lv_event_get_user_data(e);
        lv_indev_set_group(indev_keypad, ui_s->last_group);
        lv_obj_clear_flag(ui_s->base_ui, LV_OBJ_FLAG_HIDDEN);
        lv_group_del(ui_s->now_group);
        ui_s->now_group = NULL;

        msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 0);
        msi_destroy(ui_s->P0_jpg_msi);
        msi_destroy(ui_s->decode_msi);
        if (ui_s->mp4_s)
        {
            msi_destroy(ui_s->mp4_s);
            ui_s->mp4_s = NULL;
        }

#if AUDIO_EN
        if (ui_s->aac_msi)
        {
            // msi_destroy(ui_s->aac_msi);
            audio_decode_deinit(ui_s->aac_msi);
            ui_s->aac_msi = NULL;
        }
#endif

        ui_s->mp4_s      = NULL;
        ui_s->decode_msi = NULL;
        ui_s->P0_jpg_msi = NULL;

        lv_obj_del(ui_s->now_ui);
        if (ui_s->play_name)
        {
            STREAM_FREE(ui_s->play_name);
            ui_s->play_name = NULL;
        }
        set_lvgl_get_key_func(NULL);
    }
}

static void player_ui_timer(lv_timer_t *t)
{
    struct mp4_player_ui_s *ui_s = (struct mp4_player_ui_s *) t->user_data;
    if (ui_s->mp4_s)
    {
    }
}

static void enter_player_ui(lv_event_t *e)
{
    set_lvgl_get_key_func(self_key);
    // demo主要是h264的解码,所以这里初始化一下
    h264_drv_init();
    struct mp4_player_ui_s *ui_s    = (struct mp4_player_ui_s *) lv_event_get_user_data(e);
    lv_obj_t               *base_ui = ui_s->base_ui;
    lv_obj_add_flag(base_ui, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *ui = lv_obj_create(lv_scr_act());
    ui_s->now_ui = ui;
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));

    // SR_OTHER_JPG接收数据,然后发送到decode
    ui_s->P0_jpg_msi = h264_decode_msg_msi(SR_OTHER_JPG, ui_s->w, ui_s->h, ui_s->w, ui_s->h, 0);
    if (ui_s->P0_jpg_msi)
    {

        msi_do_cmd(ui_s->P0_jpg_msi, MSI_CMD_DECODE_JPEG_MSG, MSI_JPEG_DECODE_FORCE_TYPE, FSTYPE_YUV_P0);
        ui_s->P0_jpg_msi->enable = 1;
    }

    // decode发送到S_NEWPLAYER控制速度后,转发到P0
    ui_s->decode_msi = h264_decode_msi("h264_decode", 0);
    if (ui_s->decode_msi)
    {
        msi_add_output(ui_s->P0_jpg_msi, NULL, ui_s->decode_msi->name);
        msi_add_output(ui_s->decode_msi, NULL, R_VIDEO_P0);
        msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);
        ui_s->decode_msi->enable = 1;
    }

    // 创建lvgl的timer,用于获取播放的时间

    ui_s->label_time = lv_label_create(ui);
    lv_label_set_text(ui_s->label_time, "00:00");
    lv_obj_align(ui_s->label_time, LV_ALIGN_TOP_MID, 0, 0);
    ui_s->timer = lv_timer_create(player_ui_timer, 100, (void *) ui_s);

    lv_group_t *group;
    group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);
    lv_group_add_obj(group, ui);
    ui_s->now_group = group;
    lv_obj_add_event_cb(ui, exit_player_ui, LV_EVENT_KEY, ui_s);
    // 控制暂停、播放
    lv_obj_add_event_cb(ui, player_ctrl_ui, LV_EVENT_SHORT_CLICKED, ui_s);

    // 快进快退,一次尝试快进5秒,一次尝试快退5秒
    lv_obj_add_event_cb(ui, player_locate_ctrl_ui, LV_EVENT_KEY, ui_s);

    // 弹出菜单栏,用于切换分辨率
    lv_obj_add_event_cb(ui, show_filelist, LV_EVENT_KEY, ui_s);
}

lv_obj_t *mp4_player_ui(lv_group_t *group, lv_obj_t *base_ui, uint16_t w, uint16_t h)
{
    struct mp4_player_ui_s *ui_s = (struct mp4_player_ui_s *) STREAM_LIBC_ZALLOC(sizeof(struct mp4_player_ui_s));
    ui_s->last_group             = group;
    ui_s->base_ui                = base_ui;
    ui_s->play_name              = NULL;
    ui_s->w                      = w;
    ui_s->h                      = h;

    lv_obj_t *btn = lv_list_add_btn(base_ui, NULL, "mp4_player");
    lv_group_add_obj(group, btn);
    lv_obj_add_event_cb(btn, enter_player_ui, LV_EVENT_SHORT_CLICKED, ui_s);
    return btn;
}