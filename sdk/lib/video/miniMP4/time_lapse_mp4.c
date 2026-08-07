
/**********************************************************************
 * 缩时录影的基本代码demo
 *********************************************************************/
#include "basic_include.h"

#include "fatfs/osal_file.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/multimedia/msi.h"
#include "osal/string.h"
#include "stream_define.h"


#include "audio_media_ctrl/audio_code_ctrl.h"
#include "audio_msi/audio_adc.h"
#include "mp4/mp4_encode.h"
// 结构体申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC os_malloc
#define STREAM_LIBC_FREE   os_free
#define STREAM_LIBC_ZALLOC os_zalloc

enum
{
    MSI_MP4_STOP        = BIT(0),
    MSI_MP4_THREAD_DEAD = BIT(1),
};

#define TIME_LAPSE_DIR "0:/DCIM"
#define MP4_FILE_NAME  "mp4"

struct mp4_encode_msi_s
{
    struct msi     *msi;
    struct os_event evt;
    uint16_t        rec_time;
    uint16_t        filter_type;
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

static void *creat_mp4_file(char *dir_name)
{

    void    *fp;
    char     filepath[64];
    uint32_t indx = os_jiffies() % 99999999;
    void    *dir  = osal_opendir(dir_name);
    if (!dir)
    {
        osal_fmkdir(dir_name);
    }
    else
    {
        osal_closedir(dir);
    }

    sprintf(filepath, "%s/%08d.%s", dir_name, indx, MP4_FILE_NAME);
    printf("filepath:%s\r\n", filepath);
    fp = osal_fopen(filepath, "wb+");
    return fp;
}

static void *creat_mp4_file2(char *dir_name, char *filename)
{

    void    *fp;
    char     filepath[64];
    uint32_t indx = os_jiffies();
    void    *dir  = osal_opendir(dir_name);
    if (!dir)
    {
        osal_fmkdir(dir_name);
    }
    else
    {
        osal_closedir(dir);
    }
    sprintf(filename, "%016d.jpg", indx);
    sprintf(filepath, "%s/%016d.%s", dir_name, indx, MP4_FILE_NAME);
    printf("filepath:%s\r\n", filepath);
    fp = osal_fopen(filepath, "wb+");
    return fp;
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
static int mp4_encode_running(struct msi *msi, uint32_t save_time)
{
    int                      ret           = 1;
    struct mp4_encode_msi_s *mp4_encode    = (struct mp4_encode_msi_s *) msi->priv;
    //struct msi              *mp4_thumb_msi = NULL;
    void                    *mp4_msg       = NULL;
    void                    *fp            = NULL;
    struct framebuff        *fb            = NULL;
    int                      error         = 0;
    uint8_t                  nal_head_size;
    uint32_t                 nal_size;
    char                     h264_filename[64];

    uint8_t *buf;
    uint8_t *nal_head_buf;
    uint8_t *last_nal_head_buf;

    int      write_size       = 0;
    uint32_t MP4_status       = 0;
    uint32_t write_start_time = os_jiffies();
    //uint8_t  sps_pps_flag     = 0;
    uint8_t  last_num         = 0;
    (void) last_num;
    uint32_t v_count         = 0;
    //uint32_t audio_save_time = 0;
    // 参考minimp4的值去配置
    //uint8_t  asps_data[2]    = {0x15, 0x88};

    fp = creat_mp4_file2(TIME_LAPSE_DIR, h264_filename);
    if (!fp)
    {
        os_sleep_ms(1);
        goto mp4_encode_thread_end;
    }

    mp4_msg = MP4_open_init(fp, 0);
    if (!mp4_msg)
    {
        goto mp4_encode_thread_end;
    }
    mp4_video_cfg_init(mp4_msg, 1280, 720);
    ret = 0;
    os_printf(KERN_INFO "%s:%d\n", __FUNCTION__, __LINE__);
    while (fp)
    {
    //mp4_encode_running_again:
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
            buf               = fb->data;
            nal_head_size     = 0;
            nal_head_buf      = buf;
            // 记录最后一次nal的头位置,如果没有pps或者sps,这个就是I帧或者P帧
            last_nal_head_buf = nal_head_buf;
        mp4_encode_thread_again:

            // 检查对应头部是否是nal
            // nal_head_size = get_nal_size(buf + buf_offset, 64, 0,&cur_offset);
            // 检查是否存在pps或者sps,只是检查64byte就好了,硬件产生
            nal_head_buf = get_sps_pps_nal_size(nal_head_buf, 64, &nal_size, &nal_head_size);

            // 找到pps或者sps
            if (nal_head_buf)
            {
                error |= write_h264_pps_sps(mp4_msg, nal_head_buf, nal_size + nal_head_size);
                if (0 != error)
                {
                    os_printf("%s:%d\n", __FUNCTION__, __LINE__);
                    goto mp4_encode_thread_end;
                }

                // 查看下一个nal是否为pps或者sps
                nal_head_buf += (nal_size + nal_head_size);
                last_nal_head_buf = nal_head_buf;

                goto mp4_encode_thread_again;
            }

            // 写剩余的nal数据
            _os_printf(KERN_INFO "T");
            v_count++;
            error |= write_h264_data(mp4_msg, last_nal_head_buf, fb->len - (last_nal_head_buf - fb->data), 120);
            if (0 != error)
            {
                goto mp4_encode_thread_end;
            }

            write_size += (fb->len - (last_nal_head_buf - fb->data));
            msi_delete_fb(NULL, fb);
            fb = NULL;
            if (os_jiffies() - write_start_time >= save_time)
            {
                goto mp4_encode_thread_end;
            }

            // 同步,缩时录影直接同步,不需要考虑定时,因为本身就是很久才录制一次
            error |= mp4_syn(mp4_msg);
            if (0 != error)
            {
                os_printf("%s:%d\n", __FUNCTION__, __LINE__);
                goto mp4_encode_thread_end;
            }
        }
        else
        {
            msi_delete_fb(NULL, fb);
            fb = NULL;
        }
        os_sleep_ms(1);
    }

mp4_encode_thread_end:
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
    // 如果遇到写入错误之类,直接删除缓冲区的吧,防止写卡慢导致后面的帧不是连续的
    if (error)
    {
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
    }
    os_printf("mp4 encode end\n");
    return ret;
}

// 测试文件保存,保存一分钟吧,一直录卡,直到关闭msi
static void mp4_encode_thread(void *d)
{
    struct msi              *msi        = (struct msi *) d;
    struct mp4_encode_msi_s *mp4_encode = (struct mp4_encode_msi_s *) msi->priv;
    int                      ret        = 0;
    //int                      res;
    msi_get(msi);
    while (msi)
    {
        msi->enable = 1;
        ret         = mp4_encode_running(msi, 60 * 1000 * mp4_encode->rec_time);
        msi->enable = 0;
        os_printf("%s:%d end\n", __FUNCTION__, __LINE__);
        if (ret)
        {
            break;
        }
    }
//mp4_encode_thread_end:

    os_event_set(&mp4_encode->evt, MSI_MP4_THREAD_DEAD, NULL);
    msi_put(msi);
    os_printf("%s:%d end\n", __FUNCTION__, __LINE__);
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
            if (fb->mtype == F_H264 && mp4_encode->filter_type != (uint16_t) ~0)
            {
                ret = RET_ERR;
                if (mp4_encode->filter_type == fb->stype)
                {
                    struct fb_h264_s *h264_s = (struct fb_h264_s *) fb->priv;
                    // 只是支持I帧
                    if (h264_s->type == 1)
                    {
                        // os_printf(KERN_INFO"recv I frame\n");
                        ret = RET_OK;
                    }
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
    }
    return ret;
}

struct msi *time_lapse_mp4_encode_msi2_init(const char *mp4_msi_name, uint16_t filter_type, uint16_t rec_time)
{
    struct mp4_encode_msi_s *mp4_encode = NULL;
    struct msi              *msi        = msi_new(mp4_msi_name, 64, NULL);
    if (msi && !msi->priv)
    {
        mp4_encode = (struct mp4_encode_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct mp4_encode_msi_s));
        ASSERT(mp4_encode);
        mp4_encode->filter_type = filter_type;
        mp4_encode->rec_time    = rec_time;
        msi->priv               = mp4_encode;
        os_event_init(&mp4_encode->evt);
        mp4_encode->msi = msi;
        msi->action     = MP4_encode_msi_action;
        msi->enable     = 1;
    }
    else
    {
        // 不要重复打开,同一个名称需要等待上一次写入完成后并且关闭后才可以重新打开
        // 因为这里new了一次,所以要destroy一次(new和destroy要配对,实际msi的destroy是在另一个地方)
        if (msi)
        {
            msi_destroy(msi);
            msi = NULL;
            goto mp4_encode_msi_init_end;
        }
    }

    void *mp4_hdl = os_task_create("time_lapse_mp4", mp4_encode_thread, msi, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);
    os_printf("mp4_hdl:%X\n", mp4_hdl);
    if (!mp4_hdl && mp4_encode)
    {
        os_event_set(&mp4_encode->evt, MSI_MP4_THREAD_DEAD, NULL);
    }
mp4_encode_msi_init_end:
    return msi;
}
