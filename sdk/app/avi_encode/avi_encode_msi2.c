#include "basic_include.h"
#include "fatfs/osal_file.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/multimedia/msi.h"
#include "stream_define.h"
#include "app/recorder/file_process.h"

void    *avimuxer_init2(void *fp, uint32_t max_size, int w, int h, int frate, int gop, int h265, int sampnum);
uint32_t avimuxer_video2(void *ctx, unsigned char *buf, int len, int key, unsigned pts, uint8_t insert);
uint32_t avimuxer_audio2(void *ctx, unsigned char *buf, int len, int key, unsigned pts);
void     avimuxer_sync(void *ctx);
void     avimuxer_exit2(void *ctx);
struct msi *avi_thumb_msi_init(const char *filename, uint8_t srcID, uint8_t filter);

// 结构体申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC os_malloc
#define STREAM_LIBC_FREE   os_free
#define STREAM_LIBC_ZALLOC os_zalloc

#ifndef MAX_SINGLE_AVI_SIZE
#define MAX_SINGLE_AVI_SIZE (100 * 1024 * 1024)
#endif

enum
{
    MSI_AVI_START       = BIT(0),
    MSI_AVI_STOP        = BIT(1),
    MSI_AVI_THREAD_DEAD = BIT(2),
};

enum
{
    AVI_ENCODE_ERR_NONE,
    AVI_ENCODE_ERR_STOP,
    AVI_ENCODE_ERR_NO_SD,
};

struct avi_encode_msi_s
{
    struct msi     *msi;
    struct os_event evt;
    struct file_process file_process;
    uint16_t        filter_type;
    uint8_t         rec_time;
    uint16_t        rec_second;
    uint32_t        audio_encode;
};

static int avi_encode_running(struct msi *msi, uint32_t save_time, void *fp, const char *avi_filename, uint32_t filesize)
{
    int      ret               = 0;
    uint32_t res               = 0;
    uint32_t AVI_status        = 0;
    uint32_t write_start_time  = 0;
    uint32_t sys_start_time    = os_jiffies();
    uint32_t count_fps         = 0;
    uint32_t audio_fps         = 0;
    uint32_t last_syn_time     = os_jiffies();
    uint32_t already_save_time = 0;
    uint32_t fbtime            = 0;

    struct avi_encode_msi_s *avi_encode     = (struct avi_encode_msi_s *) msi->priv;
    struct msi              *avi_thumb_msi  = NULL;
    struct framebuff        *fb             = NULL;
    uint32_t                 fps            = 25;
    uint32_t                 fps_time       = 1000 / fps;
    void                    *ctx = avimuxer_init2(fp, filesize, 1280, 720, fps, 0, 0, 0);

    os_printf(KERN_INFO"fp:%X\tctx:%X\n", fp, ctx);
    if (!fp || !ctx)
    {
        os_sleep_ms(1);
        ret = AVI_ENCODE_ERR_NO_SD;
        goto avi_encode_thread_end;
    }

    avi_thumb_msi = avi_thumb_msi_init(avi_filename, FRAMEBUFF_SOURCE_USB, FSTYPE_NONE);
    msi->enable = 1;

    while (fp)
    {
        os_event_wait(&avi_encode->evt, MSI_AVI_STOP, &AVI_status, OS_EVENT_WMODE_OR, 0);
        // 结束写卡
        if (AVI_status & MSI_AVI_STOP)
        {
            ret = 1;
            os_printf("%s:%d", __FUNCTION__, __LINE__);
            goto avi_encode_thread_end;
        }
        fb = msi_get_fb(msi, 0);
        if (fb && fb->mtype == F_JPG)
        {
            if (write_start_time == 0)
            {
                write_start_time = fb->time;
            }
            count_fps++;
            _os_printf(KERN_INFO "O");
            if ((fb->time - write_start_time) / fps_time > count_fps)
            {
                uint32_t insert_num = ((fb->time - write_start_time) / fps_time) - count_fps;
                for (int i = 0; i < insert_num; i++)
                {
                    res |= avimuxer_video2(ctx, fb->data, fb->len, 1, 40, 1);
                    count_fps++;
                }
            }
            res |= avimuxer_video2(ctx, fb->data, fb->len, 1, 40, 0);
            fbtime = fb->time;

            msi_delete_fb(NULL, fb);
            fb = NULL;
            if (res || fbtime - write_start_time >= save_time)
            {
                goto avi_encode_thread_end;
            }
        }
        // 音频添加
        else if (avi_encode->audio_encode && write_start_time && fb && fb->mtype == F_AUDIO)
        {
            _os_printf(KERN_INFO "A");
            audio_fps++;
            res |= avimuxer_audio2(ctx, fb->data, fb->len, 0, 0);
            msi_delete_fb(NULL, fb);
            fb = NULL;
            if (res)
            {
                goto avi_encode_thread_end;
            }
        }
        else if (fb)
        {
            msi_delete_fb(NULL, fb);
            fb = NULL;
        }
        else
        {
            os_sleep_ms(1);
        }

        // 如果系统时间超过了30s依然没有保存完成,就直接退出
        if (os_jiffies() - sys_start_time >= save_time + 30 * 1000)
        {
            goto avi_encode_thread_end;
        }

        if (os_jiffies() - last_syn_time > 1000)
        {
            // avimuxer_sync(ctx);
            last_syn_time = os_jiffies();
            already_save_time++;
            avi_encode->rec_second = already_save_time;
            os_printf(KERN_INFO"sync:%d %d %d\n", already_save_time, count_fps, audio_fps);
        }

        os_sleep_ms(1);
    }

avi_encode_thread_end:

    avi_encode->rec_second = 0;

    os_printf(KERN_EMERG "%s:%d\tres:%d\n", __FUNCTION__, __LINE__, res);

    if (fb)
    {
        msi_delete_fb(NULL, fb);
    }

    if (ctx)
    {
        avimuxer_exit2(ctx);
    }

    if (fp)
    {
        osal_fclose(fp);
        fp = NULL;
    }

    if (avi_thumb_msi)
    {
        msi_destroy(avi_thumb_msi);
    }

    // os_printf("save time:%d\tv_count:%d\n",(uint32_t)os_jiffies(),v_count);
    os_printf("avi encode end\n");
    return ret;
}

// 测试文件保存,保存一分钟吧,一直录卡,直到关闭msi
static void avi_encode_thread(void *d)
{
    int                      ret        = 0;
    uint32_t                 avi_status = 0;
    struct msi              *msi        = (struct msi *) d;
    struct avi_encode_msi_s *avi_encode = (struct avi_encode_msi_s *) msi->priv;
	struct file_process     *file_process = &avi_encode->file_process;
    void                    *fp         = NULL;
    char                     filename[64];
    char                     filepath[64];
    uint32_t                 filesize   = 0;

    msi_get(msi);
    os_event_wait(&avi_encode->evt, MSI_AVI_START, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);

    while(msi)
    {
        filesize = avi_encode->rec_time * MAX_SINGLE_AVI_SIZE;
        if(file_process->create_file)
        {
            fp = file_process->create_file(file_process, filename, filepath, filesize);
        }
        ret         = avi_encode_running(msi, avi_encode->rec_time * 60 * 1000, fp, filename, filesize);
        msi->enable = 0;
        if(file_process->lock_file)
        {
            file_process->lock_file(filename, filepath);
        }

        os_printf(KERN_EMERG "%s %d end\n", __FUNCTION__, __LINE__);
        if (ret)
        {
            if (file_process->loop_free)
            {
                file_process->loop_free(&file_process->loop);
            }
            if(ret == AVI_ENCODE_ERR_NO_SD)
            {
                avi_status = 0;
                os_event_wait(&avi_encode->evt, MSI_AVI_STOP, &avi_status, OS_EVENT_WMODE_OR, 1000);
                if(avi_status & MSI_AVI_STOP)
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

    os_printf(KERN_DEBUG "%s %d end, ret:%d\n", __FUNCTION__, __LINE__, ret);

    struct framebuff *fb;
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

    os_event_set(&avi_encode->evt, MSI_AVI_THREAD_DEAD, NULL);
    msi_put(msi);
    os_printf("%s:%d end\n", __FUNCTION__, __LINE__);
}

static int32_t avi_encode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t                  ret        = RET_OK;
    struct avi_encode_msi_s *avi_encode = (struct avi_encode_msi_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
            os_event_wait(&avi_encode->evt, MSI_AVI_THREAD_DEAD, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);
            os_event_del(&avi_encode->evt);
            STREAM_LIBC_FREE(avi_encode);
            break;
        case MSI_CMD_PRE_DESTROY:
            os_event_set(&avi_encode->evt, MSI_AVI_STOP, NULL);
            break;

        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;

            if (fb->mtype == F_JPG && avi_encode->filter_type != (uint16_t) ~0)
            {
                ret = RET_ERR;
                if (avi_encode->filter_type == fb->stype)
                {
                    ret = RET_OK;
                }
            }
        }
        break;

        case MSI_CMD_GET_RUNNING:
        {
            uint32_t rflags = 0;
            os_event_wait(&avi_encode->evt, MSI_AVI_THREAD_DEAD | MSI_AVI_STOP, &rflags, OS_EVENT_WMODE_OR, 0);
            if (param1)
            {
                *(uint32_t *) param1 = (rflags & (MSI_AVI_THREAD_DEAD | MSI_AVI_STOP)) ? 0 : 1;
            }
        }
        break;
        case MSI_CMD_MEDIA_CTRL:
        {
            uint32_t cmd_self = (uint32_t) param1;
            switch(cmd_self)
            {
                case MSI_MEDIA_CTRL_GET_RECTIME:
                    *(uint32_t *) param2 = avi_encode->rec_second;
                    break;
                case MSI_MEDIA_CTRL_RECORD_START:
                    os_event_set(&avi_encode->evt, MSI_AVI_START, NULL);
                    break;
            }
        }
        break;
    }
    return ret;
}

struct msi *avi_encode_msi2_init(const char *avi_msi_name, uint16_t filter_type, uint8_t rec_time, 
                                uint32_t audio_encode, struct file_process *file_process, uint8_t mode)
{
    struct avi_encode_msi_s *avi_encode = NULL;
    struct msi              *msi        = msi_new(avi_msi_name, 64, NULL);
    if (msi && !msi->priv)
    {
        avi_encode = (struct avi_encode_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct avi_encode_msi_s));
        ASSERT(avi_encode);
        avi_encode->filter_type  = filter_type;
        avi_encode->rec_time     = rec_time;
        os_memcpy(&avi_encode->file_process, file_process, sizeof(struct file_process));
        avi_encode->audio_encode = audio_encode;
        msi->priv                = avi_encode;
        os_event_init(&avi_encode->evt);
        avi_encode->msi = msi;
        msi->action     = avi_encode_msi_action;
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
            goto avi_encode_msi_init_end;
        }
    }

    void *avi_hdl = os_task_create("avi_encode", avi_encode_thread, msi, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 2048);
    os_printf("avi_hdl:%X\n", avi_hdl);
    if (!avi_hdl && avi_encode)
    {
        os_event_set(&avi_encode->evt, MSI_AVI_THREAD_DEAD, NULL);
    }
avi_encode_msi_init_end:
    return msi;
}
