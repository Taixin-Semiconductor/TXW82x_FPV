#include "basic_include.h"
#include "fatfs/osal_file.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/multimedia/msi.h"
#include "stream_define.h"
#include "audio_msi/audio_adc.h"
#include "app/video_app/file_thumb.h"
#include "app/video_record/file_process.h"
#include "lib/video/muxer/avi_mux.h"
#include "lib/video/muxer/mux_file.h"
#include "video_record.h"

struct msi *avi_thumb_msi_init(const char *filename, uint8_t srcID, uint8_t filter);

// 结构体申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC os_malloc
#define STREAM_LIBC_FREE   os_free
#define STREAM_LIBC_ZALLOC os_zalloc

#ifndef AVI_MAX_SINGLE_SIZE
#define AVI_MAX_SINGLE_SIZE                 (100 * 1024 * 1024)
#endif

#define AVI_BUFFER_TIME                     2500U

// 音频多帧合并写入
#define AVI_AUDIO_BATCH_SIZE                1U
#define AVI_AUDIO_BATCH_TIMEOUT_MS          ((AVI_AUDIO_BATCH_SIZE * AUADC_TIME_INTERVAL) + 20U)
#define AVI_AUDIO_BATCH_MAX_FRAME_SIZE      1024U
#define AVI_AUDIO_BATCH_BUF_SIZE            (AVI_AUDIO_BATCH_SIZE * AVI_AUDIO_BATCH_MAX_FRAME_SIZE)

enum
{
    MSI_AVI_START       = BIT(0),
    MSI_AVI_STOP        = BIT(1),
    MSI_AVI_THREAD_DEAD = BIT(2),
};

enum
{
    AVI_RECORD_ERR_NONE,
    AVI_RECORD_ERR_STOP,
    AVI_RECORD_ERR_NO_SD,
};

struct avi_record_msi_s
{
    struct msi          *msi;
    struct os_event     evt;
    struct file_process file_process;
    uint8_t             filter;
    uint8_t             srcID;
    uint32_t            rec_time;
    uint32_t            rec_second;
    uint32_t            file_size;
    uint8_t             audio_en;
    uint16_t            video_fps;
    uint8_t            *audio_batch_buf;
    uint32_t            audio_batch_buf_size;
};

static uint16_t avi_get_node_size(uint8_t video_fps, uint8_t audio_en)
{
    uint16_t audio_fps = 0;

    if (audio_en)
    {
        audio_fps = (1000U + AUADC_TIME_INTERVAL - 1U) / AUADC_TIME_INTERVAL;
    }

    return ((video_fps + audio_fps) * AVI_BUFFER_TIME + 1000U - 1U) / 1000U;
}

static void avi_record_audio_fb_free(struct framebuff **fb)
{
    if (fb && *fb)
    {
        msi_delete_fb(NULL, *fb);
        *fb = NULL;
    }
}

static uint32_t avi_record_audio_batch_write(void *ctx, struct avi_record_msi_s *avi_record, struct framebuff **audio_batch, uint32_t *audio_batch_cnt)
{
    uint32_t total_len = 0;
    uint32_t offset    = 0;
    uint32_t res       = 0;
    uint8_t  can_batch = 1;

    if (!audio_batch_cnt || *audio_batch_cnt == 0)
    {
        return 0;
    }

    if (!ctx)
    {
        for (uint32_t i = 0; i < *audio_batch_cnt; i++)
        {
            avi_record_audio_fb_free(&audio_batch[i]);
        }
        *audio_batch_cnt = 0;
        return 0;
    }

    for (uint32_t i = 0; i < *audio_batch_cnt; i++)
    {
        if (!audio_batch[i] || !audio_batch[i]->data || audio_batch[i]->len == 0)
        {
            can_batch = 0;
            continue;
        }
        total_len += audio_batch[i]->len;
    }

    if (!avi_record || AVI_AUDIO_BATCH_SIZE <= 1U || !avi_record->audio_batch_buf || total_len > avi_record->audio_batch_buf_size)
    {
        can_batch = 0;
    }

    if (can_batch)
    {
        for (uint32_t i = 0; i < *audio_batch_cnt; i++)
        {
            os_memcpy(avi_record->audio_batch_buf + offset, audio_batch[i]->data, audio_batch[i]->len);
            offset += audio_batch[i]->len;
            avi_record_audio_fb_free(&audio_batch[i]);
        }

        _os_printf(KERN_INFO "A%d", *audio_batch_cnt);
        res |= avimuxer_audio(ctx, avi_record->audio_batch_buf, total_len);
    }
    else
    {
        for (uint32_t i = 0; i < *audio_batch_cnt; i++)
        {
            if (audio_batch[i] && audio_batch[i]->data && audio_batch[i]->len > 0)
            {
                _os_printf(KERN_INFO "A");
                res |= avimuxer_audio(ctx, audio_batch[i]->data, audio_batch[i]->len);
            }
            avi_record_audio_fb_free(&audio_batch[i]);
        }
    }

    *audio_batch_cnt = 0;
    return res;
}

static int avi_record_running(struct msi *msi, uint32_t save_time, void *fp, const char *avi_filename, uint32_t filesize)
{
    int      ret               = AVI_RECORD_ERR_NONE;
    uint32_t res               = 0;
    uint32_t AVI_status        = 0;
    uint32_t write_start_time  = 0;
    uint32_t sys_start_time    = os_jiffies();
    uint32_t count_fps         = 0;
    uint32_t last_syn_time     = 0;
    uint32_t already_save_time = 0;
    uint32_t fbtime            = 0;
    uint32_t audio_first_time  = 0;
    uint8_t  holding_lock      = 0;
    uint8_t  stop_draining     = 0;

    struct avi_record_msi_s *avi_record     = (struct avi_record_msi_s *) msi->priv;
    struct msi              *avi_thumb_msi  = NULL;
    struct framebuff        *fb             = NULL;
    uint32_t                 video_fps      = avi_record->video_fps ? avi_record->video_fps : 25U;
    void                    *mux_file       = NULL;
    file_ops_t               file_ops;
    void                    *ctx            = NULL;

    /* 音频批量写入: 累积N帧后拼接成一块一次写入 */
    struct framebuff *audio_batch[AVI_AUDIO_BATCH_SIZE] = {0};
    uint32_t          audio_batch_cnt         = 0;
    uint32_t          audio_batch_start_time  = 0;

    if (fp)
    {
        mux_file = mux_file_open((F_FILE *) fp, filesize, MUX_FILE_ALIGN_EN);
        if (mux_file)
        {
            mux_file_get_ops(mux_file, &file_ops);
            ctx = avimuxer_init_with_file(fp, &file_ops, filesize, 1280, 720, video_fps, 0, avi_record->audio_en);
        }
    }

    os_printf(KERN_INFO"fp:%X\tctx:%X\n", fp, ctx);
    if (!fp || !mux_file || !ctx)
    {
        os_sleep_ms(1);
        ret = AVI_RECORD_ERR_NO_SD;
        goto avi_record_thread_clean_end;
    }

    avi_thumb_msi = avi_thumb_msi_init(avi_filename, FRAMEBUFF_SOURCE_USB, FSTYPE_NONE);
    msi->enable = 1;

    while (fp)
    {
        os_event_wait(&avi_record->evt, MSI_AVI_STOP, &AVI_status, OS_EVENT_WMODE_OR, 0);
        // 结束写卡
        if (AVI_status & MSI_AVI_STOP)
        {
            ret = AVI_RECORD_ERR_STOP;
            msi->enable = 0;
            stop_draining = 1;
        }

        if (fb == NULL)
        {
            fb = msi_get_fb(msi, 1);
            // 写卡锁控制: 仅need_lock=1时使用mutex，持有锁且缓冲区已写完且无音频缓存时释放锁
            if(avi_record->file_process.need_lock && holding_lock && (fb == NULL || os_jiffies() - mult_record.start_time > 500) && audio_batch_cnt == 0)
            {
                holding_lock = 0;
                mult_record.start_time = 0;
                os_mutex_unlock(&mult_record.mutex);
            }
        }

        if (stop_draining && fb == NULL)
        {
            goto avi_record_thread_end;
        }

        // 有数据要写时加锁，仅need_lock=1时使用mutex
        if (avi_record->file_process.need_lock && fb != NULL && !holding_lock)
        {
            os_mutex_lock(&mult_record.mutex, osWaitForever);
            mult_record.start_time = os_jiffies();
            holding_lock = 1;
        }
        if (fb && fb->mtype == F_JPG)
        {
            if (write_start_time == 0)
            {
                write_start_time = fb->time;
            }
            _os_printf(KERN_INFO "O");
            uint32_t elapsed_frame_count = (uint32_t) (((uint64_t) (fb->time - write_start_time) * video_fps) / 1000U);
            uint32_t expected_frame_count = elapsed_frame_count + 1U;
            if (expected_frame_count > count_fps + 1U)
            {
                uint32_t insert_num = expected_frame_count - count_fps - 1U;
                for (uint32_t i = 0; i < insert_num; i++)
                {
                    res |= avimuxer_video(ctx, fb->data, fb->len, 1, 1);
                    count_fps++;
                }
            }
            res |= avimuxer_video(ctx, fb->data, fb->len, 1, 0);
            count_fps++;

            fbtime = fb->time;
            msi_delete_fb(NULL, fb);
            fb = NULL;

            if (res)
            {
                goto avi_record_thread_clean_end;
            }

            if(!stop_draining && fbtime - write_start_time >= save_time)
            {
                goto avi_record_thread_end;
            }
        }
        // 音频添加
        else if (avi_record->audio_en && write_start_time && fb && fb->mtype == MEDIA_DATA_AUDIO)
        {
            if (audio_first_time == 0)
            {
                if (fb->time >= write_start_time)
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
            fb = NULL;
            if (audio_batch_cnt == 1)
            {
                audio_batch_start_time = os_jiffies();
            }

            if (audio_batch_cnt >= AVI_AUDIO_BATCH_SIZE)
            {
                res |= avi_record_audio_batch_write(ctx, avi_record, audio_batch, &audio_batch_cnt);
                audio_batch_start_time = 0;
                if (res)
                {
                    goto avi_record_thread_clean_end;
                }
            }
        }
        else if (fb)
        {
            msi_delete_fb(NULL, fb);
            fb = NULL;
        }

        // 如果系统时间超过了30s依然没有保存完成,就直接退出
        if (os_jiffies() - sys_start_time >= save_time + 30 * 1000)
        {
            goto avi_record_thread_clean_end;
        }

        // 音频批量写入超时检查: 未满batch但超时则flush
        if (audio_batch_cnt > 0 && audio_batch_start_time &&
            (stop_draining || os_jiffies() - audio_batch_start_time >= AVI_AUDIO_BATCH_TIMEOUT_MS))
        {
            res |= avi_record_audio_batch_write(ctx, avi_record, audio_batch, &audio_batch_cnt);
            audio_batch_start_time = 0;
            if (res)
            {
                goto avi_record_thread_clean_end;
            }
        }

        if (ctx)
        {
            uint8_t sync_lock = 0;
            if (avi_record->file_process.need_lock && !holding_lock)
            {
                os_mutex_lock(&mult_record.mutex, osWaitForever);
                sync_lock = 1;
            }
            avimuxer_sync_time(ctx, 1000);
            if (sync_lock)
            {
                os_mutex_unlock(&mult_record.mutex);
            }
        }

        if (fbtime - last_syn_time > 1000)
        {
            // avimuxer_sync(ctx);
            last_syn_time = fbtime;
            already_save_time = fbtime - write_start_time;
            avi_record->rec_second = already_save_time / 1000;
            // os_printf(KERN_INFO"sync:%d %d\n", already_save_time, count_fps);
            os_printf(KERN_DEBUG "avi second: %d\n", already_save_time / 1000);
        }
    }

avi_record_thread_clean_end:
    if (fb)
    {
        msi_delete_fb(NULL, fb);
		fb = NULL;
    }

    msi->enable = 0;
    while(1)
    {
        fb = msi_get_fb(msi, 0);
        if(fb)
        {
            msi_delete_fb(NULL, fb);
        }
        else
        {
            break;
        }
    }

avi_record_thread_end:

    avi_record->rec_second = 0;

    os_printf(KERN_EMERG "%s:%d\tres:%d\n", __FUNCTION__, __LINE__, res);

    if (ctx)
    {
        uint8_t deinit_lock = 0;
        if (avi_record->file_process.need_lock && !holding_lock)
        {
            os_mutex_lock(&mult_record.mutex, osWaitForever);
            deinit_lock = 1;
        }
        if (audio_batch_cnt > 0)
        {
            res |= avi_record_audio_batch_write(ctx, avi_record, audio_batch, &audio_batch_cnt);
        }
        avimuxer_exit(ctx);
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

    // 释放写卡锁
    if (holding_lock)
    {
        os_mutex_unlock(&mult_record.mutex);
        holding_lock = 0;
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

    os_printf("avi encode end\n");
    return ret;
}

// 测试文件保存,保存一分钟吧,一直录卡,直到关闭msi
static void avi_record_thread(void *d)
{
    int                      ret        = 0;
    uint32_t                 avi_status = 0;
    struct msi              *msi        = (struct msi *) d;
    struct avi_record_msi_s *avi_record = (struct avi_record_msi_s *) msi->priv;
	struct file_process     *file_process = &avi_record->file_process;
    void                    *fp         = NULL;
    char                     filename[64];
    char                     filepath[64];
    uint32_t                 filesize   = 0;

    msi_get(msi);
    os_event_wait(&avi_record->evt, MSI_AVI_START, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);

    while(msi)
    {
        filesize = ((avi_record->rec_time / 60) + (avi_record->rec_time % 60 ? 1 : 0)) * avi_record->file_size;
        if(file_process->create_file)
        {
            fp = file_process->create_file(file_process, filename, filepath, filesize);
        }
        ret         = avi_record_running(msi, avi_record->rec_time * 1000, fp, filename, filesize);
        msi->enable = 0;
        if(file_process->lock_file && ret != AVI_RECORD_ERR_NO_SD)
        {
            file_process->lock_file(filename, filepath);
        }
        
        if (ret)
        {
            if (file_process->loop_free)
            {
                file_process->loop_free(&file_process->loop);
            }
            if(ret == AVI_RECORD_ERR_NO_SD)
            {
                avi_status = 0;
                os_event_wait(&avi_record->evt, MSI_AVI_STOP, &avi_status, OS_EVENT_WMODE_OR, 1000);
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

    os_event_set(&avi_record->evt, MSI_AVI_THREAD_DEAD, NULL);
    msi_put(msi);
    os_printf("%s:%d end\n", __FUNCTION__, __LINE__);
}

static int32_t avi_record_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t                  ret        = RET_OK;
    struct avi_record_msi_s *avi_record = (struct avi_record_msi_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
            os_event_wait(&avi_record->evt, MSI_AVI_THREAD_DEAD, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);
            os_event_del(&avi_record->evt);
            if (avi_record->audio_batch_buf)
            {
                STREAM_FREE(avi_record->audio_batch_buf);
                avi_record->audio_batch_buf = NULL;
            }
            STREAM_LIBC_FREE(avi_record);
            // 引用计数减少，最后一个实例销毁时释放mutex
            mult_record.count--;
            if (mult_record.count == 0 && mult_record.init)
            {
                os_mutex_del(&mult_record.mutex);
                mult_record.init = 0;
            }
            break;
        case MSI_CMD_PRE_DESTROY:
            os_event_set(&avi_record->evt, MSI_AVI_STOP, NULL);
            break;

        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;

            if (fb->mtype == F_JPG && avi_record->filter != (uint8_t) ~0)
            {
                if (avi_record->srcID != 0 && fb->srcID != avi_record->srcID)
                {
                    ret = RET_ERR;
                    break;
                }
                ret = RET_ERR;
                if (avi_record->filter == fb->stype)
                {
                    ret = RET_OK;
                }
            }
        }
        break;

        case MSI_CMD_GET_RUNNING:
        {
            uint32_t rflags = 0;
            os_event_wait(&avi_record->evt, MSI_AVI_THREAD_DEAD | MSI_AVI_STOP, &rflags, OS_EVENT_WMODE_OR, 0);
            if (param1)
            {
                *(uint32_t *) param1 = (rflags & (MSI_AVI_THREAD_DEAD | MSI_AVI_STOP)) ? 0 : 1;
            }
        }
        break;
        case MSI_CMD_MEDIA_CTRL:
        {
            uint32_t cmd_self = (uint32_t) param1;
            uint32_t arg = (uint32_t) param2;
            switch(cmd_self)
            {
                case MSI_MEDIA_CTRL_GET_RECTIME:
                    *(uint32_t *) arg = avi_record->rec_second;
                    break;
                case MSI_MEDIA_CTRL_RECORD_START:
                    os_event_set(&avi_record->evt, MSI_AVI_START, NULL);
                    break;
                case MSI_MEDIA_CTRL_SET_RECORD_SIZE:
                    avi_record->file_size = arg;
                    break;
                case MSI_MEDIA_CTRL_SET_RECORD_SEC:
                    avi_record->rec_time = arg;
                    break;
            }
        }
        break;
    }
    return ret;
}

struct msi *avi_record_msi_init(struct video_record_cfg *cfg)
{
    uint16_t                 video_fps  = cfg->video_fps ? cfg->video_fps : 25U;
    uint16_t                 node_size  = avi_get_node_size(video_fps, cfg->audio_en);
    uint8_t                  is_new     = 0;
    struct avi_record_msi_s *avi_record = NULL;
    struct msi              *msi        = msi_new(cfg->name, node_size, &is_new);
    if (is_new)
    {
        avi_record = (struct avi_record_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct avi_record_msi_s));
        ASSERT(avi_record);
        avi_record->filter       = cfg->filter;
        avi_record->srcID        = cfg->srcID;
        avi_record->rec_time     = (uint32_t) cfg->rec_time * 60U;
        avi_record->file_size    = AVI_MAX_SINGLE_SIZE;
        avi_record->audio_en     = cfg->audio_en;
        avi_record->video_fps    = video_fps;
        avi_record->audio_batch_buf_size = (avi_record->audio_en && AVI_AUDIO_BATCH_SIZE > 1U) ? AVI_AUDIO_BATCH_BUF_SIZE : 0;
        if (avi_record->audio_batch_buf_size > 0)
        {
            avi_record->audio_batch_buf = (uint8_t *)STREAM_MALLOC(avi_record->audio_batch_buf_size);
            if (!avi_record->audio_batch_buf)
            {
                avi_record->audio_batch_buf_size = 0;
            }
        }

        if(cfg->file_process == NULL)
        {
            // 配置默认值
            avi_record->file_process.loop = NULL;
            avi_record->file_process.rec_path = REC_PATH;
            avi_record->file_process.ext_name = AVI_EXTENSION_NAME;
            avi_record->file_process.create_file = rec_create_file;
            avi_record->file_process.loop_free = rec_loop_free;
            avi_record->file_process.lock_file = NULL;
            avi_record->file_process.need_lock = 0;
        }
        else
        {
            os_memcpy(&avi_record->file_process, cfg->file_process, sizeof(struct file_process));
        }
        
        msi->priv = avi_record;
        os_event_init(&avi_record->evt);
        avi_record->msi = msi;
        msi->action = avi_record_msi_action;
    }
    else
    {
        if (msi)
        {
            msi_destroy(msi);
            msi = NULL;
        }
        goto avi_record_msi_init_end;
    }

    if(avi_record->file_process.need_lock && !mult_record.init)
    {
        os_mutex_init(&mult_record.mutex);
        mult_record.init = 1;
    }
    mult_record.count++;
    
    void *avi_hdl = os_task_create("avi_record", avi_record_thread, msi, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 2048);
    os_printf("avi_hdl:%X\n", avi_hdl);
    if (!avi_hdl && avi_record)
    {
        if (avi_record->audio_batch_buf)
        {
            STREAM_FREE(avi_record->audio_batch_buf);
            avi_record->audio_batch_buf = NULL;
        }
        os_event_set(&avi_record->evt, MSI_AVI_THREAD_DEAD, NULL);
    }
avi_record_msi_init_end:
    return msi;
}
