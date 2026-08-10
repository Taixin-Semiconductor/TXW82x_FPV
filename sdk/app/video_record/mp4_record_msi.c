#include "basic_include.h"
#include "fatfs/osal_file.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/multimedia/msi.h"
#include "lib/video/muxer/mux_file.h"
#include "lib/video/muxer/mp4_mux.h"
#include "osal/string.h"
#include "stream_define.h"
#include "audio_msi/audio_adc.h"
#include "stream_define.h"
#include "video_msi.h"
#include "app/video_app/file_thumb.h"
#include "file_process.h"
#include "video_record.h"

// data 申请空间函数
#define STREAM_MALLOC       av_psram_malloc
#define STREAM_FREE         av_psram_free
#define STREAM_ZALLOC       av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC  os_malloc
#define STREAM_LIBC_FREE    os_free
#define STREAM_LIBC_ZALLOC  os_zalloc

#ifndef MP4_MAX_SINGLE_SIZE
#define MP4_MAX_SINGLE_SIZE             (100 * 1024 * 1024)
#endif

// 预录时长
#ifndef MP4_EVENT_PRERECORD_MS
#define MP4_EVENT_PRERECORD_MS          3000U
#endif

// 缓冲时长
#define MP4_BUFFER_TIME                 2500U
// 预录时间，配置缓冲区大小
#define MP4_EVENT_BUFFER_TIME           3000U

// 音频多帧合并写入
#define MP4_AUDIO_BATCH_SIZE            1U
#define MP4_AUDIO_BATCH_TIMEOUT_MS      ((MP4_AUDIO_BATCH_SIZE * 1000U) / 8U + 20U)
#define MP4_AUDIO_BATCH_MAX_FRAME_SIZE  1024U
#define MP4_AUDIO_BATCH_BUF_SIZE        (MP4_AUDIO_BATCH_SIZE * MP4_AUDIO_BATCH_MAX_FRAME_SIZE)

#define AVERAGE_BASE                    6

enum
{
    MSI_MP4_START       = BIT(0),
    MSI_MP4_STOP        = BIT(1),
    MSI_MP4_THREAD_DEAD = BIT(2),
    MSI_MP4_EVENT_START = BIT(3),
    MSI_MP4_EVENT_STOP  = BIT(4),
};

enum
{
    MP4_RECORD_ERR_NONE,
    MP4_RECORD_ERR_STOP,
    MP4_RECORD_ERR_EVENT_STOP,
    MP4_RECORD_ERR_NO_SD,
};

struct mp4_event_buffer_s
{
    struct framebuff **frames;
    uint16_t           node_size;
    uint16_t           head;
    uint16_t           tail;
    uint16_t           count;
    uint32_t           latest_time;
};

struct mp4_event_ctx_s
{
    uint32_t                  trigger_time;
    struct mp4_event_buffer_s event_buffer;
};

struct mp4_record_msi_s
{
    struct msi             *msi;
    struct os_event         evt;
    struct file_process     file_process;
    struct mp4_event_ctx_s *event;
    void                   *fb;
    uint8_t                 filter_type;
    uint8_t                 srcID;
    uint8_t                 mode;   // 普通录像 / 缩时录影 / 事件触发
    uint8_t                 audio_en;
    uint16_t                video_fps;
    uint16_t                audio_sr;
    uint32_t                rec_time;
    uint32_t                rec_second;
    uint8_t                *audio_batch_buf;
    uint32_t                audio_batch_buf_size;
    uint32_t                timeLapse_count;
    uint32_t                file_size;

};

static uint16_t mp4_get_node_size(uint8_t video_fps, uint16_t audio_sr, uint8_t audio_en, uint8_t mode)
{
    uint16_t audio_fps = 0;
    uint16_t node_size = 0;

    if (audio_en)
    {
        audio_fps = (audio_sr + 1024U - 1U) / 1024U;
    }
    
    if (mode == MP4_MODE_EVENT)
    {
        node_size = ((video_fps + audio_fps) * (MP4_EVENT_BUFFER_TIME + 1000U) + 1000U - 1U) / 1000U;
    }
    else
    {
        node_size = ((video_fps + audio_fps) * MP4_BUFFER_TIME + 1000U - 1U) / 1000U;
    }
    
    return node_size;
}

static int mp4_is_event_mode(const struct mp4_record_msi_s *mp4_record)
{
    return mp4_record->mode == MP4_MODE_EVENT && mp4_record->event != NULL;
}

static int mp4_is_realtime_mode(const struct mp4_record_msi_s *mp4_record)
{
    return mp4_record->mode != MP4_MODE_TIME_LAPSE;
}

static int mp4_has_audio(const struct mp4_record_msi_s *mp4_record)
{
    return mp4_is_realtime_mode(mp4_record) && mp4_record->audio_en;
}

static const char *mp4_mode_str(const struct mp4_record_msi_s *mp4_record)
{
    if (mp4_record->mode == MP4_MODE_TIME_LAPSE)
    {
        return "TIME_LAPSE";
    }
    if (mp4_record->mode == MP4_MODE_EVENT)
    {
        return "EVENT";
    }
    return "NORMAL";
}

static void mp4_event_buffer_drop_head(struct mp4_record_msi_s *mp4_record)
{
    struct mp4_event_ctx_s    *event = mp4_record->event;
    struct mp4_event_buffer_s *event_buffer;
    struct framebuff          *fb;

    if (event == NULL)
    {
        return;
    }

    event_buffer = &event->event_buffer;
    if (event_buffer->count == 0)
    {
        return;
    }

    fb                                       = event_buffer->frames[event_buffer->head];
    event_buffer->frames[event_buffer->head] = NULL;
    event_buffer->head                       = (event_buffer->head + 1) % event_buffer->node_size;
    event_buffer->count--;
    if (event_buffer->count == 0)
    {
        event_buffer->tail        = 0;
        event_buffer->head        = 0;
        event_buffer->latest_time = 0;
    }

    if (fb)
    {
        msi_delete_fb(NULL, fb);
    }
}

static void mp4_event_buffer_reset(struct mp4_record_msi_s *mp4_record)
{
    struct mp4_event_ctx_s *event = mp4_record->event;

    if (event == NULL)
    {
        return;
    }

    while (event->event_buffer.count)
    {
        mp4_event_buffer_drop_head(mp4_record);
    }
}

static struct framebuff *mp4_event_buffer_first(const struct mp4_record_msi_s *mp4_record)
{
    const struct mp4_event_ctx_s *event = mp4_record->event;

    if (event == NULL || event->event_buffer.count == 0)
    {
        return NULL;
    }
    return event->event_buffer.frames[event->event_buffer.head];
}

static struct framebuff *mp4_event_buffer_pop(struct mp4_record_msi_s *mp4_record)
{
    struct mp4_event_ctx_s    *event = mp4_record->event;
    struct mp4_event_buffer_s *event_buffer;
    struct framebuff          *fb;

    if (event == NULL)
    {
        return NULL;
    }

    event_buffer = &event->event_buffer;
    if (event_buffer->count == 0)
    {
        return NULL;
    }

    fb                                       = event_buffer->frames[event_buffer->head];
    event_buffer->frames[event_buffer->head] = NULL;
    event_buffer->head                       = (event_buffer->head + 1) % event_buffer->node_size;
    event_buffer->count--;
    if (event_buffer->count == 0)
    {
        event_buffer->tail        = 0;
        event_buffer->head        = 0;
        event_buffer->latest_time = 0;
    }
    return fb;
}

static void mp4_event_buffer_push(struct mp4_record_msi_s *mp4_record, struct framebuff *fb)
{
    struct mp4_event_ctx_s    *event = mp4_record->event;
    struct mp4_event_buffer_s *event_buffer;
    struct framebuff          *first_fb;

    if (event == NULL || fb == NULL)
    {
        return;
    }

    event_buffer = &event->event_buffer;
    while (event_buffer->count >= event_buffer->node_size)
    {
        mp4_event_buffer_drop_head(mp4_record);
    }

    event_buffer->frames[event_buffer->tail] = fb;
    event_buffer->tail                       = (event_buffer->tail + 1) % event_buffer->node_size;
    event_buffer->count++;
    event_buffer->latest_time = fb->time;

    while (event_buffer->count)
    {
        first_fb = mp4_event_buffer_first(mp4_record);
        if (first_fb == NULL)
        {
            mp4_event_buffer_drop_head(mp4_record);
            continue;
        }

        if (event_buffer->latest_time < first_fb->time)
        {
            break;
        }

        if (event_buffer->latest_time - first_fb->time <= event_buffer->node_size)
        {
            break;
        }
        mp4_event_buffer_drop_head(mp4_record);
    }
}

static int mp4_event_buffer_prepare(struct mp4_record_msi_s *mp4_record)
{
    struct mp4_event_ctx_s    *event = mp4_record->event;
    struct mp4_event_buffer_s *event_buffer;
    uint32_t                   target_time = 0;
    int32_t                    first_i_off = -1;
    int32_t                    start_off   = -1;
    uint16_t                   i;

    if (event == NULL)
    {
        return RET_ERR;
    }

    event_buffer = &event->event_buffer;
    if (event_buffer->count == 0)
    {
        return RET_ERR;
    }

    if (event->trigger_time > MP4_EVENT_PRERECORD_MS)
    {
        target_time = event->trigger_time - MP4_EVENT_PRERECORD_MS;
    }

    for (i = 0; i < event_buffer->count; i++)
    {
        struct framebuff *fb = event_buffer->frames[(event_buffer->head + i) % event_buffer->node_size];
        struct fb_h264_s *priv;

        if (!fb || fb->mtype != F_H264)
        {
            continue;
        }

        priv = (struct fb_h264_s *) fb->priv;
        if (priv && priv->type == 1)
        {
            if (first_i_off < 0)
            {
                first_i_off = i;
            }
            if (fb->time <= target_time)
            {
                start_off = i;
            }
        }
    }

    if (start_off < 0)
    {
        start_off = first_i_off;
    }
    if (start_off < 0)
    {
        return RET_ERR;
    }

    while (start_off-- > 0)
    {
        mp4_event_buffer_drop_head(mp4_record);
    }

    while (event_buffer->count)
    {
        struct framebuff *fb = mp4_event_buffer_first(mp4_record);
        if (fb && fb->mtype == F_H264)
        {
            return RET_OK;
        }
        mp4_event_buffer_drop_head(mp4_record);
    }

    return RET_ERR;
}

static struct framebuff *mp4_get_next_fb(struct msi *msi)
{
    struct mp4_record_msi_s *mp4_record = (struct mp4_record_msi_s *) msi->priv;
    struct framebuff        *fb         = NULL;

    if (mp4_is_event_mode(mp4_record))
    {
        fb = mp4_event_buffer_pop(mp4_record);
    }

    if (fb == NULL)
    {
        fb = msi_get_fb(msi, 1);
    }

    return fb;
}

static int mp4_event_wait_record_start(struct msi *msi)
{
    struct mp4_record_msi_s *mp4_record = (struct mp4_record_msi_s *) msi->priv;
    struct mp4_event_ctx_s  *event      = mp4_record->event;

    if (event == NULL)
    {
        return MP4_RECORD_ERR_STOP;
    }

    while (1)
    {
        struct framebuff *fb         = NULL;
        uint32_t          mp4_status = 0;

        os_event_wait(&mp4_record->evt, MSI_MP4_STOP | MSI_MP4_EVENT_STOP | MSI_MP4_EVENT_START, &mp4_status, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
        if (mp4_status & MSI_MP4_STOP)
        {
            _os_printf("%s %d\r\n", __func__, __LINE__);
            return MP4_RECORD_ERR_STOP;
        }

        if (mp4_status & MSI_MP4_EVENT_STOP)
        {
            event->trigger_time = 0;
            continue;
        }

        if (mp4_status & MSI_MP4_EVENT_START)
        {
            if (event->event_buffer.count)
            {
                event->trigger_time = event->event_buffer.latest_time;
            }
            else
            {
                event->trigger_time = os_jiffies();
            }

            if (mp4_event_buffer_prepare(mp4_record) == RET_OK)
            {
                _os_printf("%s %d\r\n", __func__, __LINE__);
                return MP4_RECORD_ERR_NONE;
            }
        }

        if (mp4_record->fb)
        {
            fb = mp4_record->fb;
            mp4_record->fb = NULL;
        }
        else
        {
            fb = msi_get_fb(msi, 0);
        }

        if (fb)
        {
            if (fb->mtype == F_H264 || (fb->mtype == MEDIA_DATA_AUDIO && mp4_has_audio(mp4_record)))
            {
                mp4_event_buffer_push(mp4_record, fb);
            }
            else
            {
                msi_delete_fb(NULL, fb);
            }
        }

        if (event->trigger_time && mp4_event_buffer_prepare(mp4_record) == RET_OK)
        {
            _os_printf("%s %d\r\n", __func__, __LINE__);
            return MP4_RECORD_ERR_NONE;
        }
        os_sleep_ms(1); // 任务调度
    }
}
static uint32_t a2i(char *str)
{
    uint32_t ret  = 0;
    uint32_t indx = 0;
    char     str_buf[32];
    memset(str_buf, 0, 32);
    while (str[indx] != '\0')
    {
        if (str[indx] == '.')
        {
            break;
        }
        str_buf[indx] = str[indx];
        indx++;
    }
    // printf("str_buf:%s  str:%s\r\n",str_buf,str);
    indx = 0;
    while (str_buf[indx] != '\0')
    {
        if (str_buf[indx] >= '0' && str_buf[indx] <= '9')
        {
            ret = ret * 10 + str_buf[indx] - '0';
        }
        indx++;
    }
    return ret;
}

static void get_aac_config(uint32_t samplerate, uint8_t *config_buf)
{
    uint8_t samplerate_index = 0;
    switch (samplerate)
    {
        case 48000:
            samplerate_index = 0x3;
            break;
        case 44100:
            samplerate_index = 0x4;
            break;
        case 36000:
            samplerate_index = 0x5;
            break;
        case 24000:
            samplerate_index = 0x6;
            break;
        case 22050:
            samplerate_index = 0x7;
            break;
        case 16000:
            samplerate_index = 0x8;
            break;
        case 12000:
            samplerate_index = 0x9;
            break;
        case 11025:
            samplerate_index = 0xA;
            break;
        case 8000:
            samplerate_index = 0xB;
            break;
        default:
            break;
    }
    config_buf[0] = (0x02 << 3) | (samplerate_index >> 1);
    config_buf[1] = ((samplerate_index & 0x1) << 7) | (0x01 << 3);
}

// 获取nal的size,从0开始搜索,返回的是的nal头的size,offset相对于头的偏移(通过多次调用,可以用于计算nal_size)
// 返回0代表搜索不到nal的头
static uint8_t get_nal_size(uint8_t *buf, uint32_t size, uint32_t *offset)
{
    uint32_t pos = 0;
    while ((size - pos) > 3)
    {
        if (buf[pos] == 0 && buf[pos + 1] == 0 && buf[pos + 2] == 1)
        {
            *offset = pos;
            return 3;
        }

        if (buf[pos] == 0 && buf[pos + 1] == 0 && buf[pos + 2] == 0 && buf[pos + 3] == 1)
        {
            *offset = pos;
            return 4;
        }

        pos++;
    }
    return 0;
}

// 只是获取pps和sps的nalsize
static uint8_t *get_sps_pps_nal_size(uint8_t *buf, uint32_t size, uint32_t *nal_size, uint8_t *head_size)
{
    uint32_t offset;
    uint8_t  nal_head_size = get_nal_size(buf, size, &offset);
    uint8_t  nal_type;
    uint8_t *ret_buf = NULL;
    // 找到头部,检查类型
    if (nal_head_size && offset + nal_head_size < size)
    {
        nal_type = buf[nal_head_size + offset] & 0x1f;

        // 找到sps和pps就返回长度和偏移(相对buf的偏移)
        if (nal_type == 7 || nal_type == 8)
        {
            // 查找下一个nal
            nal_head_size = get_nal_size(buf + offset + nal_head_size, size - (offset + nal_head_size), nal_size);
            if (nal_head_size)
            {
                // 偏移到nal的头部

                ret_buf    = buf + offset;
                // 返回nal的头size
                *head_size = nal_head_size;
            }
        }
    }

    return ret_buf;
}

static void mp4_record_audio_fb_free(struct framebuff **fb)
{
    if (fb && *fb)
    {
        msi_delete_fb(NULL, *fb);
        *fb = NULL;
    }
}

static uint32_t mp4_record_audio_batch_write(void *mp4_msg, struct mp4_record_msi_s *mp4_record, struct framebuff **audio_batch, uint32_t *audio_batch_cnt, uint32_t audio_duration_ticks)
{
    uint32_t total_len = 0;
    uint32_t offset    = 0;
    uint32_t res       = 0;
    uint8_t  can_batch = 1;
    uint32_t sizes[MP4_AUDIO_BATCH_SIZE];
    uint32_t duration_ticks[MP4_AUDIO_BATCH_SIZE];

    if (!audio_batch_cnt || *audio_batch_cnt == 0)
    {
        return 0;
    }

    if (!mp4_msg)
    {
        for (uint32_t i = 0; i < *audio_batch_cnt; i++)
        {
            mp4_record_audio_fb_free(&audio_batch[i]);
        }
        *audio_batch_cnt = 0;
        return 0;
    }

    for (uint32_t i = 0; i < *audio_batch_cnt; i++)
    {
        if (!audio_batch[i] || audio_batch[i]->len <= 7)
        {
            can_batch = 0;
            continue;
        }
        sizes[i]     = audio_batch[i]->len - 7;
        duration_ticks[i] = audio_duration_ticks;
        total_len += sizes[i];
    }

    if (!mp4_record || MP4_AUDIO_BATCH_SIZE <= 1U || !mp4_record->audio_batch_buf || total_len > mp4_record->audio_batch_buf_size)
    {
        can_batch = 0;
    }

    if (can_batch)
    {
        for (uint32_t i = 0; i < *audio_batch_cnt; i++)
        {
            os_memcpy(mp4_record->audio_batch_buf + offset, audio_batch[i]->data + 7, sizes[i]);
            offset += sizes[i];
            mp4_record_audio_fb_free(&audio_batch[i]);
        }
        _os_printf(KERN_INFO "U%d", *audio_batch_cnt);
        res |= write_aac_data_batch(mp4_msg, mp4_record->audio_batch_buf, total_len, sizes, duration_ticks, *audio_batch_cnt);
    }
    else
    {
        for (uint32_t i = 0; i < *audio_batch_cnt; i++)
        {
            if (audio_batch[i] && audio_batch[i]->len > 7)
            {
                _os_printf(KERN_INFO "U");
                res |= write_aac_data(mp4_msg, audio_batch[i]->data + 7, audio_batch[i]->len - 7, audio_duration_ticks);
            }
            mp4_record_audio_fb_free(&audio_batch[i]);
        }
    }

    *audio_batch_cnt = 0;
    return res;
}

static int mp4_record_running(struct msi *msi, uint32_t save_time, void *fp, const char *h264_filename, uint32_t filesize)
{
    int                      ret        = MP4_RECORD_ERR_NONE;
    struct mp4_record_msi_s *mp4_record = (struct mp4_record_msi_s *) msi->priv;
    struct framebuff        *fb         = mp4_record->fb; // 获取缓冲的一帧数据
    mp4_record->fb                      = NULL;
    struct msi *mp4_thumb_msi           = NULL;
    void       *mp4_msg                 = NULL;
    void       *mux_file                = NULL;
    file_ops_t  file_ops;
    int32_t     error                   = 0;
    uint32_t    MP4_status              = 0;
    uint32_t    write_start_time        = os_jiffies();
    uint32_t    nal_size                = 0;
    uint8_t     nal_head_size           = 0;
    uint8_t    *buf                     = NULL;
    uint8_t    *nal_head_buf            = NULL;
    uint8_t    *last_nal_head_buf       = NULL;
    uint8_t     sps_pps_flag            = 0;
    uint8_t     h264_count              = 0;
    uint8_t     asps_data[2]            = {0};

    uint32_t now_time                   = 0;
    uint32_t last_fb_time               = 0;
    uint32_t video_first_time           = 0;
    uint32_t audio_first_time           = 0;
    uint32_t second                     = 0;
    uint32_t last_adjust_pts_time       = os_jiffies();
    uint32_t video_fps                  = mp4_record->video_fps ? mp4_record->video_fps : 25U;
    int      video_duration_ticks       = (MP4_VIDEO_TIMESCALE + video_fps / 2U) / video_fps;
    int      average_pts                = video_duration_ticks;
    int      acc_pts                    = 0;
    int      acc_pts_tmp                = 0;

    int      write_size                 = 0;
    uint32_t v_count                    = 0;
    uint8_t  holding_lock               = 0;
    uint8_t  stop_draining              = 0;

    uint32_t audio_batch_cnt            = 0;
    uint32_t audio_batch_start_time     = 0;
    uint32_t audio_sr                   = mp4_record->audio_sr;
    uint32_t audio_duration_ticks       = 0;
    struct framebuff *audio_batch[MP4_AUDIO_BATCH_SIZE] = {0};

    os_printf(KERN_INFO "%s\r\n", __func__);

    if (!fp)
    {
        os_sleep_ms(1);
        ret = MP4_RECORD_ERR_NO_SD;
        goto mp4_record_running_clean_end;
    }

    // 根据模式决定是否包含音频
    int has_audio = mp4_has_audio(mp4_record);
    mux_file      = mux_file_open((F_FILE *) fp, filesize, MUX_FILE_ALIGN_EN);
    if (mux_file)
    {
        mux_file_get_ops(mux_file, &file_ops);
        mp4_msg = mp4_open_init_with_file((F_FILE *) fp, &file_ops, has_audio);
	}
    if (!mp4_msg)
    {
        goto mp4_record_running_clean_end;
    }

    // 配置音频和视频
    if (has_audio)
    {
        get_aac_config(audio_sr, asps_data);
        mp4_audio_cfg_init(mp4_msg, asps_data, sizeof(asps_data));
        mp4_set_audio_samplerate(mp4_msg, audio_sr);
        audio_duration_ticks = 1024U;
    }
    mp4_set_max_size(mp4_msg, filesize);
    // mp4_video_cfg_init(mp4_msg, 1280, 720);

    // 初始化PTS相关变量
    if (mp4_is_realtime_mode(mp4_record))
    {
        acc_pts = video_duration_ticks;
    }

    // 启动缩略图生成
    mp4_thumb_msi = new_mp4_thumb_msi(h264_filename, FRAMEBUFF_SOURCE_CAMERA0, FSTYPE_NONE);
    if (mp4_thumb_msi)
    {
        msi_add_output(NULL, AUTO_JPG, mp4_thumb_msi, NULL);
    }
    msi->enable = 1;
    while (fp)
    {
        os_event_wait(&mp4_record->evt, MSI_MP4_STOP | MSI_MP4_EVENT_STOP, &MP4_status, OS_EVENT_WMODE_OR, 0);
        // 结束写卡
        if (MP4_status & MSI_MP4_STOP)
        {
            ret = MP4_RECORD_ERR_STOP;
            msi->enable   = 0;
            stop_draining = 1;
        }

        if (mp4_is_event_mode(mp4_record) && (MP4_status & MSI_MP4_EVENT_STOP))
        {
            ret = MP4_RECORD_ERR_EVENT_STOP;
            goto mp4_record_running_event_end;
        }

        if (fb == NULL)
        {
            fb = mp4_get_next_fb(msi);
            // 写卡锁控制: 仅need_lock=1时使用mutex，持有锁且缓冲区已写完时释放锁
            if (mp4_record->file_process.need_lock && holding_lock && (fb == NULL || os_jiffies() - mult_record.start_time > 500) && audio_batch_cnt == 0)
            {
                holding_lock           = 0;
                mult_record.start_time = 0;
                os_mutex_unlock(&mult_record.mutex);
            }
        }

        if (stop_draining && fb == NULL)
        {
            goto mp4_record_running_end;
        }

        // 有数据要写时加锁，仅need_lock=1时使用mutex
        if (mp4_record->file_process.need_lock && fb != NULL && !holding_lock)
        {
            os_mutex_lock(&mult_record.mutex, osWaitForever);
            mult_record.start_time = os_jiffies();
            holding_lock           = 1;
        }

        if (fb && (fb->mtype == F_H264))
        {
            struct fb_h264_s *h264_priv = (struct fb_h264_s *) fb->priv;

            // 帧序号检查和PTS计算
            if (mp4_is_realtime_mode(mp4_record))
            {
                // 检查录制时间
                if (!stop_draining && video_first_time != 0 && fb->time >= video_first_time && fb->time - video_first_time >= save_time)
                {
                    if (h264_priv->type == 1)
                    {
                        mp4_record->fb = fb;
                        fb             = NULL;
                        if (mp4_is_event_mode(mp4_record))
                        {
                            goto mp4_record_running_event_end;
                        }
                        goto mp4_record_running_end;
                    }
                }
                // 超时退出 3s
                if (!stop_draining && os_jiffies() - write_start_time >= save_time + 3000)
                {
                    if (mp4_is_event_mode(mp4_record))
                    {
                        goto mp4_record_running_event_end;
                    }
                    goto mp4_record_running_clean_end;
                }

                h264_count++;
                if ((h264_priv->count != h264_count) && (h264_priv->type != 1))
                {
                    os_printf(KERN_ERR "%s: %d h264 frame lost,count: %d,expect: %d\ttype: %d\n", __FUNCTION__, __LINE__, h264_priv->count, h264_count, h264_priv->type);
                    goto mp4_record_running_no_found_sps_pps;
                }
                h264_count = h264_priv->count;

                // PTS计算和调整
                if (last_fb_time == 0)
                {
                    last_fb_time = fb->time;
                }
                else
                {
                    average_pts += ((((fb->time - last_fb_time) * MP4_VIDEO_TICKS_PER_MS) >> AVERAGE_BASE) - (average_pts >> AVERAGE_BASE));
                }

                if (now_time == 0)
                {
                    now_time         = fb->time;
                    video_first_time = fb->time;
                }

                acc_pts += ((fb->time - last_fb_time) * MP4_VIDEO_TICKS_PER_MS);
                acc_pts -= video_duration_ticks;
                acc_pts_tmp = acc_pts > 0 ? acc_pts : -acc_pts;

                if (average_pts > 0 && acc_pts_tmp / average_pts)
                {
                    if (os_jiffies() - last_adjust_pts_time > 1000)
                    {
                        video_duration_ticks = average_pts + (acc_pts / 60);
                        last_adjust_pts_time = os_jiffies();
                    }
                }

                if (os_jiffies() - last_adjust_pts_time > 5000)
                {
                    last_adjust_pts_time = os_jiffies();
                    if (average_pts > 0)
                    {
                        video_duration_ticks = average_pts;
                    }
                }

                last_fb_time = fb->time;
            }
            else
            {
                if (mp4_record->timeLapse_count <= v_count)
                {
                    goto mp4_record_running_clean_end;
                }
                v_count++;
            }

            buf               = fb->data;
            nal_head_size     = 0;
            nal_head_buf      = buf;
            last_nal_head_buf = nal_head_buf;

        mp4_record_running_again:
            // 检查是否存在pps或者sps
            nal_head_buf = get_sps_pps_nal_size(nal_head_buf, 64, &nal_size, &nal_head_size);

            // 找到pps或者sps
            if (nal_head_buf)
            {
                // 重置写入时间
                if (!sps_pps_flag && mp4_is_realtime_mode(mp4_record))
                {
                    write_start_time = os_jiffies();
                }
                mp4_video_cfg_init(mp4_msg, h264_priv->w, h264_priv->h);
                error |= write_h264_pps_sps(mp4_msg, nal_head_buf, nal_size + nal_head_size);
                if (0 != error)
                {
                    os_printf(KERN_ERR "%s:%d\n", __FUNCTION__, __LINE__);
                    if (mp4_is_event_mode(mp4_record))
                    {
                        goto mp4_record_running_event_clean_end;
                    }
                    goto mp4_record_running_clean_end;
                }

                nal_head_buf += (nal_size + nal_head_size);
                last_nal_head_buf = nal_head_buf;
                sps_pps_flag      = 1;

                goto mp4_record_running_again;
            }

            // 如果没有写过sps和pps，就跳过
            if (!sps_pps_flag)
            {
                now_time         = 0;
                video_first_time = 0;
                goto mp4_record_running_no_found_sps_pps;
            }

            // 写剩余的nal数据
            if (!mp4_is_realtime_mode(mp4_record))
            {
                _os_printf(KERN_INFO "T");
                error |= write_h264_data(mp4_msg, last_nal_head_buf, fb->len - (last_nal_head_buf - fb->data), video_duration_ticks);
                write_size += (fb->len - (last_nal_head_buf - fb->data));
            }
            else
            {
                _os_printf(KERN_INFO "M");
                error |= write_h264_data(mp4_msg, last_nal_head_buf, fb->len - (last_nal_head_buf - fb->data), video_duration_ticks);
            }

            if (0 != error)
            {
                os_printf(KERN_ERR "%s: %d\n", __FUNCTION__, __LINE__);
                if (mp4_is_event_mode(mp4_record))
                {
                    goto mp4_record_running_event_clean_end;
                }
                goto mp4_record_running_clean_end;
            }
            now_time = fb->time;

        mp4_record_running_no_found_sps_pps:
            msi_delete_fb(NULL, fb);
            fb = NULL;

            if (!mp4_is_realtime_mode(mp4_record))
            {
                // 每帧立即同步
                error |= mp4_sync(mp4_msg);
                if (0 != error)
                {
                    os_printf("%s:%d\n", __FUNCTION__, __LINE__);
                    if (mp4_is_event_mode(mp4_record))
                    {
                        goto mp4_record_running_event_clean_end;
                    }
                    goto mp4_record_running_clean_end;
                }
            }
            else
            {
                if ((now_time - video_first_time) / 1000 > second)
                {
                    second                 = (now_time - video_first_time) / 1000;
                    mp4_record->rec_second = second;
                    os_printf(KERN_DEBUG "mp4 second: %d\n", second);
                }
            }
        }
        else if (sps_pps_flag && fb && (fb->mtype == MEDIA_DATA_AUDIO) && has_audio)
        {
            if (audio_first_time == 0)
            {
                if (video_first_time != 0 && fb->time >= video_first_time)
                {
                    audio_first_time = fb->time;
                }
                else
                {
                    _os_printf("P");
                    // 音频时间不对,直接删除
                    msi_delete_fb(NULL, fb);
                    fb = NULL;
                    continue;
                }
            }
            audio_batch[audio_batch_cnt++] = fb;
            fb                             = NULL;

            if (audio_batch_cnt == 1)
            {
                audio_batch_start_time = os_jiffies();
            }

            if (audio_batch_cnt >= MP4_AUDIO_BATCH_SIZE)
            {
                error |= mp4_record_audio_batch_write(mp4_msg, mp4_record, audio_batch, &audio_batch_cnt, audio_duration_ticks);
                audio_batch_start_time = 0;
                if (0 != error)
                {
                    os_printf(KERN_ERR "%s:%d\n", __FUNCTION__, __LINE__);
                    if (mp4_is_event_mode(mp4_record))
                    {
                        goto mp4_record_running_event_clean_end;
                    }
                    goto mp4_record_running_clean_end;
                }
            }
        }
        else if (fb)
        {
            msi_delete_fb(NULL, fb);
            fb = NULL;
        }

        if (audio_batch_cnt > 0 && audio_batch_start_time && (stop_draining || os_jiffies() - audio_batch_start_time >= MP4_AUDIO_BATCH_TIMEOUT_MS))
        {
            error |= mp4_record_audio_batch_write(mp4_msg, mp4_record, audio_batch, &audio_batch_cnt, audio_duration_ticks);
            audio_batch_start_time = 0;
            if (0 != error)
            {
                if (mp4_is_event_mode(mp4_record))
                {
                    goto mp4_record_running_event_clean_end;
                }
                goto mp4_record_running_clean_end;
            }
        }

        if (mp4_msg && sps_pps_flag && mp4_is_realtime_mode(mp4_record))
        {
            uint8_t sync_lock = 0;
            if (mp4_record->file_process.need_lock && !holding_lock)
            {
                os_mutex_lock(&mult_record.mutex, osWaitForever);
                sync_lock = 1;
            }
            error |= mp4_sync_time(mp4_msg, 1000);
            if (sync_lock)
            {
                os_mutex_unlock(&mult_record.mutex);
            }

            if (0 != error)
            {
                os_printf(KERN_ERR "%s:%d\n", __FUNCTION__, __LINE__);
                if (mp4_is_event_mode(mp4_record))
                {
                    goto mp4_record_running_event_clean_end;
                }
                goto mp4_record_running_clean_end;
            }
        }
    }

mp4_record_running_clean_end:
    if (fb)
    {
        msi_delete_fb(NULL, fb);
        fb = NULL;
    }

    // 清空缓冲区
    msi->enable = 0;
    while (1)
    {
        fb = msi_get_fb(msi, 0);
        if (fb)
        {
            msi_delete_fb(NULL, fb);
        }
        else
        {
            break;
        }
    }

mp4_record_running_event_clean_end:
    if (fb)
    {
        msi_delete_fb(NULL, fb);
        fb = NULL;
    }

mp4_record_running_event_end:
    // 清空预录缓冲区
    if (mp4_is_event_mode(mp4_record))
    {
        while (1)
        {
            fb = mp4_event_buffer_pop(mp4_record);
            if (fb)
            {
                msi_delete_fb(NULL, fb);
            }
            else
            {
                break;
            }
        }
    }

mp4_record_running_end:

    // 释放写卡锁
    if (mp4_msg && audio_batch_cnt > 0)
    {
        error |= mp4_record_audio_batch_write(mp4_msg, mp4_record, audio_batch, &audio_batch_cnt, audio_duration_ticks);
    }

    if (holding_lock)
    {
        os_mutex_unlock(&mult_record.mutex);
        holding_lock = 0;
    }

    mp4_record->rec_second = 0;

    if (mp4_msg)
    {
        uint8_t deinit_lock = 0;
        if (mp4_record->file_process.need_lock)
        {
            os_mutex_lock(&mult_record.mutex, osWaitForever);
            deinit_lock = 1;
        }
        error |= mp4_deinit(mp4_msg);
        if (deinit_lock)
        {
            os_mutex_unlock(&mult_record.mutex);
        }
    }
    if (mux_file)
    {
        mux_file_close(mux_file);
        mux_file = NULL;
    }

    if (fp)
    {
        osal_fclose(fp);
        fp = NULL;
    }

    if (mp4_thumb_msi)
    {
        msi_del_output(NULL, AUTO_JPG, mp4_thumb_msi, NULL);
        msi_destroy(mp4_thumb_msi);
    }

    if (error)
    {
        os_printf(KERN_ERR "mp4 encode error: %d\n", error);
    }
    os_printf(KERN_DEBUG "mp4 encode end, mode: %s\n", mp4_mode_str(mp4_record));
    return ret;
}

static void mp4_record_thread(void *d)
{
    int                      ret          = 0;
    uint32_t                 mp4_status   = 0;
    struct msi              *msi          = (struct msi *) d;
    struct mp4_record_msi_s *mp4_record   = (struct mp4_record_msi_s *) msi->priv;
    struct file_process     *file_process = &mp4_record->file_process;
    void                    *fp           = NULL;
    uint32_t                 filesize     = 0;
    char                     filename[64];
    char                     filepath[64];

    msi_get(msi);
    os_event_wait(&mp4_record->evt, MSI_MP4_START | MSI_MP4_STOP, &mp4_status, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);
    if (mp4_status & MSI_MP4_STOP)
    {
        goto mp4_record_thread_end;
    }
    while (msi)
    {
        filesize = mp4_record->file_size ? ((mp4_record->rec_time / 60) + (mp4_record->rec_time % 60 ? 1 : 0)) * mp4_record->file_size : 0;
        _os_printf("filesize: %d\n", filesize);
        if (mp4_is_event_mode(mp4_record))
        {
            msi->enable = 1;
            ret         = mp4_event_wait_record_start(msi);
            if (ret == MP4_RECORD_ERR_STOP)
            {
                break;
            }

            struct framebuff *first_fb = NULL;
            first_fb                   = mp4_event_buffer_first(mp4_record);
            file_process->frame_time   = first_fb ? first_fb->time : 0;
        }

        if (file_process->create_file)
        {
            if (!mp4_is_event_mode(mp4_record) && mp4_record->fb)
            {
                struct framebuff *fb     = mp4_record->fb;
                file_process->frame_time = fb->time;
            }
            fp = file_process->create_file(file_process, filename, filepath, filesize);
        }

        // 如果是缩时录影，寻找到I帧再继续录制，这里需要发送一个命令，强行产生一帧I帧
        if (file_process->start_encode)
        {
            file_process->param = mp4_record->mode;
            file_process->start_encode(file_process);
        }

        ret = mp4_record_running(msi, mp4_record->rec_time * 1000, fp, filename, filesize);

        file_process->frame_time = 0;

        if (file_process->lock_file && ret != MP4_RECORD_ERR_NO_SD)
        {
            file_process->lock_file(filename, filepath);
        }

        if (file_process->end_encode)
        {
            file_process->param = ret;
            file_process->end_encode(file_process);
        }

        if (mp4_is_event_mode(mp4_record))
        {
            mp4_record->event->trigger_time = 0;
            mp4_event_buffer_reset(mp4_record);
            os_event_clear(&mp4_record->evt, MSI_MP4_EVENT_START, NULL);
            os_event_clear(&mp4_record->evt, MSI_MP4_EVENT_STOP, NULL);

            if (ret == MP4_RECORD_ERR_STOP)
            {
                break;
            }

            if (ret == MP4_RECORD_ERR_EVENT_STOP)
            {
                os_printf("mp4 event stop\n");
            }

            if (ret == MP4_RECORD_ERR_NO_SD && file_process->loop_free)
            {
                file_process->loop_free(&file_process->loop);
            }
            os_printf(KERN_DEBUG "%s %d end, ret: %d\n", __FUNCTION__, __LINE__, ret);
            continue;
        }

        if (ret)
        {
            if (file_process->loop_free)
            {

                file_process->loop_free(&file_process->loop);
            }
            // 如果是sd异常,延迟1s然后重新尝试重新录像(同时检测是否要停止)
            if (ret == MP4_RECORD_ERR_NO_SD)
            {
                mp4_status = 0;
                os_event_wait(&mp4_record->evt, MSI_MP4_STOP, &mp4_status, OS_EVENT_WMODE_OR, 1000);
                if (mp4_status & MSI_MP4_STOP)
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
        os_printf(KERN_DEBUG "%s %d end, ret: %d\n", __FUNCTION__, __LINE__, ret);
    }

mp4_record_thread_end:
    if (mp4_is_event_mode(mp4_record))
    {
        mp4_record->event->trigger_time = 0;
        mp4_event_buffer_reset(mp4_record);
    }
    os_event_set(&mp4_record->evt, MSI_MP4_THREAD_DEAD, NULL);
    msi_put(msi);
    os_printf(KERN_DEBUG "%s: %d end\n", __FUNCTION__, __LINE__);
}

static int32_t mp4_record_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t                  ret        = RET_OK;
    struct mp4_record_msi_s *mp4_record = (struct mp4_record_msi_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
            os_event_wait(&mp4_record->evt, MSI_MP4_THREAD_DEAD, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);
            if (mp4_is_event_mode(mp4_record))
            {
                mp4_record->event->trigger_time = 0;
                mp4_event_buffer_reset(mp4_record);
            }
            if (mp4_record->fb)
            {
                msi_delete_fb(NULL, (struct framebuff *) mp4_record->fb);
                mp4_record->fb = NULL;
            }
            if (mp4_record->event)
            {
                STREAM_LIBC_FREE(mp4_record->event);
                mp4_record->event = NULL;
            }
            if (mp4_record->audio_batch_buf)
            {
                STREAM_FREE(mp4_record->audio_batch_buf);
                mp4_record->audio_batch_buf = NULL;
            }
            os_event_del(&mp4_record->evt);
            STREAM_LIBC_FREE(mp4_record);
            // 引用计数减少，最后一个实例销毁时释放mutex
            mult_record.count--;
            if (mult_record.count == 0 && mult_record.init)
            {
                os_mutex_del(&mult_record.mutex);
                mult_record.init = 0;
            }
            break;
        case MSI_CMD_PRE_DESTROY:
            os_event_set(&mp4_record->evt, MSI_MP4_STOP, NULL);
            break;
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            // 暂时接收所有的音频
            if (fb->mtype == MEDIA_DATA_AUDIO)
            {
                // 缩时录影不接收音频
                if (mp4_record->mode == MP4_MODE_TIME_LAPSE)
                {
                    ret = RET_ERR;
                }
            }
            else if (fb->mtype == F_H264 && mp4_record->filter_type != (uint16_t) ~0)
            {
                if (mp4_record->srcID != 0 && fb->srcID != mp4_record->srcID)
                {
                    ret = RET_ERR;
                    break;
                }
                ret = RET_ERR;
                if (mp4_record->filter_type == fb->stype)
                {
                    ret = RET_OK;
                }
            }
        }
        break;
        case MSI_CMD_GET_RUNNING:
        {
            uint32_t rflags = 0;
            os_event_wait(&mp4_record->evt, MSI_MP4_THREAD_DEAD | MSI_MP4_STOP, &rflags, OS_EVENT_WMODE_OR, 0);
            if (param1)
            {
                *(uint32_t *) param1 = (rflags & (MSI_MP4_THREAD_DEAD | MSI_MP4_STOP)) ? 0 : 1;
            }
        }
        break;
        case MSI_CMD_MEDIA_CTRL:
        {
            uint32_t cmd_self = (uint32_t) param1;
            uint32_t arg      = (uint32_t) param2;
            switch (cmd_self)
            {
                case MSI_MEDIA_CTRL_GET_RECTIME:
                {
                    *(uint32_t *) arg = mp4_record->rec_second;
                }
                break;
                case MSI_MEDIA_CTRL_RECORD_START:
                {
                    os_event_set(&mp4_record->evt, MSI_MP4_START, NULL);
                }
                break;
                case MSI_MEDIA_CTRL_SET_RECORD_SIZE:
                {
                    mp4_record->file_size = arg;
                }
                break;
                case MSI_MEDIA_CTRL_SET_RECORD_SEC:
                {
                    mp4_record->rec_time        = arg;
                    mp4_record->timeLapse_count = arg * (mp4_record->video_fps);
                }
                break;
                case MSI_MEDIA_CTRL_EVENT_START:
                {
                    if (mp4_is_event_mode(mp4_record))
                    {
                        os_event_set(&mp4_record->evt, MSI_MP4_EVENT_START, NULL);
                    }
                }
                break;
                case MSI_MEDIA_CTRL_EVENT_STOP:
                {
                    if (mp4_is_event_mode(mp4_record))
                    {
                        os_event_set(&mp4_record->evt, MSI_MP4_EVENT_STOP, NULL);
                    }
                }
                break;
            }
        }
        break;
    }
    return ret;
}

struct msi *mp4_record_msi_init(struct video_record_cfg *cfg)
{
    uint16_t                 audio_sr   = cfg->audio_en ? audio_adc_get_samplerate(AUSYS_AUAD) : 0;
    uint16_t                 node_size  = mp4_get_node_size(cfg->video_fps, audio_sr, cfg->audio_en, cfg->mode);
    uint8_t                  is_new     = 0;
    struct msi              *msi        = msi_new(cfg->name, node_size, &is_new);
    struct mp4_record_msi_s *mp4_record = NULL;
    if (is_new)
    {
        mp4_record = (struct mp4_record_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct mp4_record_msi_s));
        ASSERT(mp4_record);
        mp4_record->filter_type     = cfg->filter;
        mp4_record->srcID           = cfg->srcID;
        mp4_record->rec_time        = (uint32_t) cfg->rec_time * 60U;
        mp4_record->mode            = cfg->mode; // 设置录制模式
        mp4_record->video_fps       = cfg->video_fps;
        mp4_record->audio_sr        = audio_sr;
        mp4_record->timeLapse_count = cfg->rec_time * 60 * (cfg->video_fps);
        mp4_record->file_size       = MP4_MAX_SINGLE_SIZE;
        if (cfg->mode == MP4_MODE_EVENT)
        {
            mp4_record->event = (struct mp4_event_ctx_s *) STREAM_LIBC_ZALLOC(sizeof(struct mp4_event_ctx_s) + sizeof(struct framebuff *) * node_size);
            if (mp4_record->event == NULL)
            {
                STREAM_LIBC_FREE(mp4_record);
                mp4_record = NULL;
                if (msi)
                {
                    msi_destroy(msi);
                    msi = NULL;
                }
                goto mp4_record_msi_init_end;
            }
            *mp4_record->event->event_buffer.frames = (struct framebuff *)(mp4_record->event + 1);
            mp4_record->event->event_buffer.node_size = node_size;
        }
        if (cfg->file_process == NULL)
        {
            // 配置默认值
            mp4_record->file_process.rec_path    = REC_PATH;
            mp4_record->file_process.ext_name    = MP4_EXTENSION_NAME;
            mp4_record->file_process.create_file = rec_create_file;
            mp4_record->file_process.loop_free   = rec_loop_free;
        }
        else
        {
            os_memcpy(&mp4_record->file_process, cfg->file_process, sizeof(struct file_process));
        }

        if (cfg->mode == MP4_MODE_TIME_LAPSE)
        {
            // 缩时录影无音频
            mp4_record->audio_en = 0;
        }
        else
        {
            mp4_record->audio_en = cfg->audio_en;
        }
        mp4_record->audio_batch_buf_size = (mp4_record->audio_en && MP4_AUDIO_BATCH_SIZE > 1U) ? MP4_AUDIO_BATCH_BUF_SIZE : 0;
        if (mp4_record->audio_batch_buf_size > 0)
        {
            mp4_record->audio_batch_buf = (uint8_t *) STREAM_MALLOC(mp4_record->audio_batch_buf_size);
            if (!mp4_record->audio_batch_buf)
            {
                mp4_record->audio_batch_buf_size = 0;
            }
        }

        os_event_init(&mp4_record->evt);
        mp4_record->msi = msi;
        msi->priv       = mp4_record;
        msi->action     = mp4_record_msi_action;
    }
    else
    {
        if (msi)
        {
            msi_destroy(msi);
            msi = NULL;
        }
        goto mp4_record_msi_init_end;
    }

    if (mp4_record->file_process.need_lock && !mult_record.init)
    {
        os_mutex_init(&mult_record.mutex);
        mult_record.init = 1;
    }
    mult_record.count++;

    void *mp4_hdl = os_task_create("mp4_record", mp4_record_thread, msi, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 2048);
    os_printf(KERN_DEBUG "mp4_hdl: %x\n", mp4_hdl);
    if (!mp4_hdl && mp4_record)
    {
        os_event_set(&mp4_record->evt, MSI_MP4_THREAD_DEAD, NULL);
    }
mp4_record_msi_init_end:
    return msi;
}
