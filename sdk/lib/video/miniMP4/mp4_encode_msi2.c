#include "basic_include.h"

#include "fatfs/osal_file.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/multimedia/msi.h"
#include "osal/string.h"
#include "stream_define.h"
#include "audio_media_ctrl/audio_code_ctrl.h"
#include "audio_msi/audio_adc.h"
#include "app/video_app/file_thumb.h"

#ifdef PIN_FROM_PARAM
#include "pin_param.h"
#endif
#include "stream_define.h"
#include "mp4/mp4_encode.h"

struct msi *mp4_thumb_msi_init(const char *filename, uint8_t srcID, uint8_t filter);

// 结构体申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC os_malloc
#define STREAM_LIBC_FREE   os_free
#define STREAM_LIBC_ZALLOC os_zalloc

#define AVERAGE_BASE  (6)
#define MP4_MAX_COUNT (64)

#ifndef MAX_SINGLE_MP4_SIZE
#define MAX_SINGLE_MP4_SIZE (100 * 1024 * 1024)
#endif

enum
{
    MSI_MP4_STOP        = BIT(0),
    MSI_MP4_THREAD_DEAD = BIT(1),
};

enum
{
    MP4_ENCODE_ERR_NONE,
    MP4_ENCODE_ERR_STOP,
    MP4_ENCODE_ERR_NO_SD,
};

// 录制模式
enum
{
    MP4_MODE_NORMAL,     // 普通录像模式
    MP4_MODE_TIME_LAPSE, // 缩时录影模式
};

typedef void *(*file_create)(void **loop, char *file_name);
typedef void (*loop_free)(void **loop);

struct mp4_encode_msi_s
{
    struct msi     *msi;
    struct os_event evt;
    void           *loop; // 循环录像的句柄,定时检查是否足够空间以及是否需要删除文件
    uint8_t         filter_type;
    uint8_t         srcID;
    uint8_t         rec_time;
    uint32_t        audio_encode;
    uint16_t        rec_second;
    int16_t         max_video_count;
    file_create     file_create;
    loop_free       loop_free;
    uint8_t         mode; // 录制模式：普通录像 or 缩时录影
};

#define MIN(a, b) ((a) > (b) ? b : a)
#define MAX(a, b) ((a) > (b) ? a : b)
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

static int mp4_encode_running(struct msi *msi, uint32_t save_time, void *fp, const char *h264_filename)
{
    int                      ret              = 0;
    struct mp4_encode_msi_s *mp4_encode       = (struct mp4_encode_msi_s *) msi->priv;
    struct msi              *mp4_thumb_msi    = NULL;
    void                    *mp4_msg          = NULL;
    struct framebuff        *fb               = NULL;
    int32_t                  error            = 0;
    uint32_t                 MP4_status       = 0;
    uint32_t                 write_start_time = os_jiffies();
    uint32_t                 nal_size;
    uint8_t                  nal_head_size;
    uint8_t                 *buf;
    uint8_t                 *nal_head_buf;
    uint8_t                 *last_nal_head_buf;
    uint8_t                  sps_pps_flag = 0;
    uint8_t                  h264_count   = 0;
    uint8_t                  asps_data[2];

    // 普通录像变量
    uint32_t now_time             = 0;
    uint32_t last_fb_time         = 0;
    uint32_t video_first_time     = 0;
    uint32_t audio_first_time     = 0;
    uint32_t second               = 0;
    uint32_t last_adjust_pts_time = os_jiffies();
    int      delta                = 40;
    int      average_pts          = delta * 90;
    int      acc_pts              = 0;
    int      acc_pts_tmp          = 0;

    // 缩时录影变量
    int      write_size = 0;
    uint32_t v_count    = 0;

    os_printf(KERN_DEBUG "mp4_encode->max_video_count:%d, mode:%s\r\n", mp4_encode->max_video_count, mp4_encode->mode == MP4_MODE_TIME_LAPSE ? "TIME_LAPSE" : "NORMAL");

    if (!fp)
    {
        os_sleep_ms(1);
        ret = MP4_ENCODE_ERR_NO_SD;
        goto mp4_encode_thread_end;
    }

    // MP4文件初始化 - 根据模式决定是否包含音频
    int has_audio = (mp4_encode->mode == MP4_MODE_NORMAL) && (mp4_encode->audio_encode == AAC_ENC);
    mp4_msg       = MP4_open_init((F_FILE *) fp, has_audio);
    if (!mp4_msg)
    {
        goto mp4_encode_thread_end;
    }

    // 配置音频和视频
    if (mp4_encode->mode == MP4_MODE_NORMAL && has_audio)
    {
        get_aac_config(AUADC_SAMPLERATE, asps_data);
        mp4_audio_cfg_init(mp4_msg, asps_data, sizeof(asps_data));
    }
    mp4_set_max_size(mp4_msg, MAX_SINGLE_MP4_SIZE);
    mp4_video_cfg_init(mp4_msg, 1280, 720);

    // 初始化PTS相关变量（普通录像使用）
    if (mp4_encode->mode == MP4_MODE_NORMAL)
    {
        acc_pts = (delta * 90);
    }

    // 启动缩略图生成
    mp4_thumb_msi   = mp4_thumb_msi_init(h264_filename, FRAMEBUFF_SOURCE_CAMERA0, FSTYPE_NONE);
    msi->enable     = 1;
    while (fp)
    {
        os_event_wait(&mp4_encode->evt, MSI_MP4_STOP, &MP4_status, OS_EVENT_WMODE_OR, 0);
        // 结束写卡
        if (MP4_status & MSI_MP4_STOP)
        {
            ret = 1;
            goto mp4_encode_thread_end;
        }
        fb = msi_get_fb(msi, 0);
        if (fb && (fb->mtype == F_H264))
        {
            struct fb_h264_s *h264_priv = (struct fb_h264_s *) fb->priv;
            mp4_encode->max_video_count--;

            // 普通录像：帧序号检查和PTS计算
            if (mp4_encode->mode == MP4_MODE_NORMAL)
            {
                h264_count++;
                if ((h264_priv->count != h264_count) && (h264_priv->type != 1))
                {
                    os_printf(KERN_ERR "%s:%d h264 frame lost,count:%d,expect:%d\ttype:%d\n", __FUNCTION__, __LINE__, h264_priv->count, h264_count, h264_priv->type);
                    goto mp4_encode_running_no_found_sps_pps;
                }
                h264_count = h264_priv->count;

                // PTS计算和调整
                if (last_fb_time == 0)
                {
                    last_fb_time = fb->time;
                }
                else
                {
                    average_pts += ((((fb->time - last_fb_time) * 90) >> AVERAGE_BASE) - (average_pts >> AVERAGE_BASE));
                }

                if (now_time == 0)
                {
                    now_time         = fb->time;
                    video_first_time = fb->time;
                }

                acc_pts += ((fb->time - last_fb_time) * 90);
                acc_pts -= (delta * 90);
                acc_pts_tmp = acc_pts > 0 ? acc_pts : -acc_pts;

                if (acc_pts_tmp / average_pts)
                {
                    if (os_jiffies() - last_adjust_pts_time > 1000)
                    {
                        delta                = (average_pts + (acc_pts / 60)) / 90;
                        last_adjust_pts_time = os_jiffies();
                    }
                }

                if (os_jiffies() - last_adjust_pts_time > 5000)
                {
                    last_adjust_pts_time = os_jiffies();
                    delta                = (average_pts) / 90;
                }

                last_fb_time = fb->time;
            }
            else
            {
                // 缩时录影：计数统计
                v_count++;
            }

            buf               = fb->data;
            nal_head_size     = 0;
            nal_head_buf      = buf;
            last_nal_head_buf = nal_head_buf;

        mp4_encode_thread_again:
            // 检查是否存在pps或者sps
            nal_head_buf = get_sps_pps_nal_size(nal_head_buf, 64, &nal_size, &nal_head_size);

            // 找到pps或者sps
            if (nal_head_buf)
            {
                // 重置写入时间（普通录像）
                if (!sps_pps_flag && mp4_encode->mode == MP4_MODE_NORMAL)
                {
                    write_start_time = os_jiffies();
                }

                error |= write_h264_pps_sps(mp4_msg, nal_head_buf, nal_size + nal_head_size);
                if (0 != error)
                {
                    os_printf(KERN_ERR "%s:%d\n", __FUNCTION__, __LINE__);
                    goto mp4_encode_thread_end;
                }

                nal_head_buf += (nal_size + nal_head_size);
                last_nal_head_buf = nal_head_buf;
                sps_pps_flag      = 1;

                goto mp4_encode_thread_again;
            }

            // 如果没有写过sps和pps,就跳过（普通录像）
            if (!sps_pps_flag)
            {
                now_time         = 0;
                video_first_time = 0;
                goto mp4_encode_running_no_found_sps_pps;
            }

            // 写剩余的nal数据
            if (mp4_encode->mode == MP4_MODE_TIME_LAPSE)
            {
                _os_printf(KERN_INFO "T");
                error |= write_h264_data(mp4_msg, last_nal_head_buf, fb->len - (last_nal_head_buf - fb->data), 120);
                write_size += (fb->len - (last_nal_head_buf - fb->data));
            }
            else
            {
                _os_printf(KERN_INFO "M");
                error |= write_h264_data(mp4_msg, last_nal_head_buf, fb->len - (last_nal_head_buf - fb->data), delta);
            }

            if (0 != error)
            {
                os_printf(KERN_ERR "%s:%d\n", __FUNCTION__, __LINE__);
                goto mp4_encode_thread_end;
            }
            now_time = fb->time;

        mp4_encode_running_no_found_sps_pps:
            msi_delete_fb(NULL, fb);
            fb = NULL;

            // 文件同步策略
            if (mp4_encode->mode == MP4_MODE_TIME_LAPSE)
            {
                // 缩时录影：每帧立即同步
                error |= mp4_syn(mp4_msg);
                if (0 != error)
                {
                    os_printf("%s:%d\n", __FUNCTION__, __LINE__);
                    goto mp4_encode_thread_end;
                }
            }
            else
            {
                // 普通录像：每秒同步一次
                if ((os_jiffies() - write_start_time) / 1000 > second)
                {
                    second                 = (os_jiffies() - write_start_time) / 1000;
                    mp4_encode->rec_second = second;
                    os_printf(KERN_DEBUG "save second: %d\n", second);
#if 1
                    error |= mp4_syn(mp4_msg);
                    if (0 != error)
                    {
                        os_printf(KERN_ERR "%s:%d\n", __FUNCTION__, __LINE__);
                        goto mp4_encode_thread_end;
                    }
#endif
                }
            }
        }
        else if (sps_pps_flag && fb && (fb->mtype == F_AUDIO) && (mp4_encode->mode == MP4_MODE_NORMAL) && (mp4_encode->audio_encode == AAC_ENC))
        {
            // 普通录像：音频处理
            if (audio_first_time == 0)
            {
                if (video_first_time != 0 && fb->time >= video_first_time)
                {
                    audio_first_time = fb->time;
                }
                else
                {
                    // 音频时间不对,直接删除
                    msi_delete_fb(NULL, fb);
                    fb = NULL;
                    continue;
                }
            }
            else
            {
                buf = fb->data;
                _os_printf("U");
                error |= write_aac_data(mp4_msg, buf + 7, fb->len - 7, 128);
                if (0 != error)
                {
                    goto mp4_encode_thread_end;
                }
            }
            msi_delete_fb(NULL, fb);
            fb = NULL;
        }
        else
        {
            msi_delete_fb(NULL, fb);
            fb = NULL;
        }

        // 检查录制时间
        if (os_jiffies() - write_start_time >= save_time)
        {
            goto mp4_encode_thread_end;
        }
        os_sleep_ms(1);
    }

mp4_encode_thread_end:
    mp4_encode->rec_second = 0;

    if (fb)
    {
        msi_delete_fb(NULL, fb);
    }

    if (mp4_msg)
    {
        mp4_deinit(mp4_msg);
    }

    if (fp)
    {
        osal_fclose(fp);
        fp = NULL;
    }

    // 清空缓冲区
    if (1)
    {
        msi->enable = 0;
        while (1)
        {
            fb = msi_get_fb(msi, 0);
            if (fb)
            {
                if (fb->mtype == F_H264)
                {
                    mp4_encode->max_video_count--;
                }
                msi_delete_fb(NULL, fb);
            }
            else
            {
                break;
            }
        }
    }

    if (mp4_thumb_msi)
    {
        msi_destroy(mp4_thumb_msi);
    }

    if (error)
    {
        os_printf(KERN_ERR "mp4 encode error:%d\n", error);
    }
    os_printf(KERN_DEBUG "mp4 encode end, mode:%s\n", mp4_encode->mode == MP4_MODE_TIME_LAPSE ? "TIME_LAPSE" : "NORMAL");
    return ret;
}

static void mp4_encode_thread(void *d)
{
    int                      ret        = 0;
    uint32_t                 MP4_status = 0;
    struct msi              *msi        = (struct msi *) d;
    struct mp4_encode_msi_s *mp4_encode = (struct mp4_encode_msi_s *) msi->priv;
    void                    *fp         = NULL;
    char                     filename[64];

    msi_get(msi);
    while (msi)
    {
        // msi->enable = 1;
        if (mp4_encode->file_create)
        {
            fp = mp4_encode->file_create(&mp4_encode->loop, filename);
        }
        ret         = mp4_encode_running(msi, mp4_encode->rec_time * 60 * 1000, fp, filename);
        msi->enable = 0;
        os_printf(KERN_DEBUG "%s:%d end\n", __FUNCTION__, __LINE__);
        if (ret)
        {
            if (mp4_encode->loop_free)
            {
                mp4_encode->loop_free(&mp4_encode->loop);
            }
            // 如果是sd异常,延迟1s然后重新尝试重新录像(同时检测是否要停止)
            if (ret == MP4_ENCODE_ERR_NO_SD)
            {
                MP4_status = 0;
                os_event_wait(&mp4_encode->evt, MSI_MP4_STOP, &MP4_status, OS_EVENT_WMODE_OR, 1000);
                if (MP4_status & MSI_MP4_STOP)
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    os_event_set(&mp4_encode->evt, MSI_MP4_THREAD_DEAD, NULL);
    msi_put(msi);
    os_printf(KERN_DEBUG "%s:%d end\n", __FUNCTION__, __LINE__);
}

static int32_t MP4_encode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t                  ret        = RET_OK;
    struct mp4_encode_msi_s *mp4_encode = (struct mp4_encode_msi_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
            os_event_wait(&mp4_encode->evt, MSI_MP4_THREAD_DEAD, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);
            os_event_del(&mp4_encode->evt);
            STREAM_LIBC_FREE(mp4_encode);
            break;
        case MSI_CMD_PRE_DESTROY:
            os_event_set(&mp4_encode->evt, MSI_MP4_STOP, NULL);
            break;

        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            // 暂时接收所有的音频
            if (fb->mtype == F_AUDIO)
            {
                // 普通录像接收音频，缩时录影不接收
                if (mp4_encode->mode == MP4_MODE_TIME_LAPSE)
                {
                    ret = RET_ERR;
                }
            }
            else if (fb->mtype == F_H264 && mp4_encode->filter_type != (uint16_t) ~0)
            {

                if (mp4_encode->srcID != 0 && fb->srcID != mp4_encode->srcID)
                {
                    ret = RET_ERR;
                    break;
                }
                ret = RET_ERR;
                if (mp4_encode->filter_type == fb->stype)
                {
                    ret = RET_OK;
                }

                // 帧选择策略
                if (ret == RET_OK)
                {
                    if (mp4_encode->mode == MP4_MODE_TIME_LAPSE)
                    {
                        // 缩时录影：只接收I帧
                        struct fb_h264_s *priv = (struct fb_h264_s *) fb->priv;
                        if (priv->type != 1)
                        {
                            ret = RET_ERR;
                        }
                    }
                    else
                    {
                        // 普通录像：缓冲区满时只接收I帧
                        if (mp4_encode->max_video_count > MP4_MAX_COUNT)
                        {
                            struct fb_h264_s *priv = (struct fb_h264_s *) fb->priv;
                            if (priv->type != 1)
                            {
                                ret = RET_ERR;
                                os_printf(KERN_ERR "h264 drop:%d\n", mp4_encode->max_video_count);
                            }
                        }
                    }
                }

                if (ret == RET_OK)
                {
                    mp4_encode->max_video_count++;
                }
            }
        }
        break;

        case MSI_CMD_GET_RUNNING:
        {
            uint32_t rflags = 0;
            os_event_wait(&mp4_encode->evt, MSI_MP4_THREAD_DEAD | MSI_MP4_STOP, &rflags, OS_EVENT_WMODE_OR, 0);
            if (param1)
            {
                *(uint32_t *) param1 = (rflags & (MSI_MP4_THREAD_DEAD | MSI_MP4_STOP)) ? 0 : 1;
            }
        }
        break;
        case MSI_CMD_MEDIA_CTRL:
        {
            *(uint32_t *) param1 = mp4_encode->rec_second;
        }
        break;
    }
    return ret;
}

// 统一的初始化函数，通过mode参数控制录制模式
struct msi *mp4_encode_msi2_init(const char *mp4_msi_name, uint8_t srcID, uint8_t filter_type, uint8_t rec_time, uint32_t audio_encode, void *file_create, void *loop_free, uint8_t mode)
{
    struct mp4_encode_msi_s *mp4_encode = NULL;
    struct msi              *msi        = msi_new(mp4_msi_name, 96, NULL);
    if (msi && !msi->priv)
    {
        mp4_encode = (struct mp4_encode_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct mp4_encode_msi_s));
        ASSERT(mp4_encode);
        mp4_encode->filter_type = filter_type;
        mp4_encode->srcID       = srcID;
        mp4_encode->rec_time    = rec_time;
        mp4_encode->mode        = mode; // 设置录制模式
        msi->priv               = mp4_encode;
        os_event_init(&mp4_encode->evt);
        mp4_encode->msi = msi;
        msi->action     = MP4_encode_msi_action;
        msi->enable     = 1;

        // 根据模式设置默认参数
        if (mode == MP4_MODE_TIME_LAPSE)
        {
            // 缩时录影：无音频，使用固定文件创建函数
            mp4_encode->audio_encode = 0;
        }
        else
        {
            // 普通录像：支持音频
            mp4_encode->audio_encode = audio_encode;
        }
        mp4_encode->file_create = file_create;
        mp4_encode->loop_free   = loop_free;
    }
    else
    {
        if (msi)
        {
            msi_destroy(msi);
            msi = NULL;
            goto mp4_encode_msi_init_end;
        }
    }

    void *mp4_hdl = os_task_create("mp4", mp4_encode_thread, msi, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 2048);
    os_printf(KERN_DEBUG "mp4_hdl:%X\n", mp4_hdl);
    if (!mp4_hdl && mp4_encode)
    {
        os_event_set(&mp4_encode->evt, MSI_MP4_THREAD_DEAD, NULL);
    }
mp4_encode_msi_init_end:
    return msi;
}
