#include "basic_include.h"
#include "fatfs/osal_file.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/multimedia/msi.h"
#include "stream_define.h"
#include "openDML.h"
#include "app/recorder/file_process.h"
#include "app/video_app/file_thumb.h"
#include "audio_msi/audio_adc.h"
#include "avi_record_msi.h"

int ex_parse_jpg(uint8_t *jpg_buf, uint32_t maxsize, uint32_t *w, uint32_t *h);
struct msi *avi_thumb_msi_init(const char *filename, uint8_t srcID, uint8_t filter);

#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

#define STREAM_LIBC_MALLOC os_malloc
#define STREAM_LIBC_FREE   os_free
#define STREAM_LIBC_ZALLOC os_zalloc

#ifndef MAX_SINGLE_AVI_SIZE
#define MAX_SINGLE_AVI_SIZE	(100 * 1024 * 1024)
#endif

#define AVI_RECORD_FPS 25U

#define AVI_DEBUG(fmt, ...) os_printf(fmt, ##__VA_ARGS__)

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
    AVI_RECORD_ERR_NO_BUF,
};

struct avi_record_msi_s
{
    struct msi         *msi;
    struct os_event     evt;
    struct file_process file_process;
    uint8_t             filter_type;
    uint8_t             srcID;
    uint8_t             mode;
    uint32_t            rec_time;
    uint32_t            rec_second;
    uint32_t            file_size;
    uint32_t            audio_encode;
};

static int avi_record_write_cb(void *fp, void *data, int flen)
{
    return osal_fwrite(data, 1, flen, (F_FILE *)fp);
}

static int pre_avi_seek(F_FILE *fp, uint32_t offset)
{
    int ret = RET_OK;
    uint32_t filesize  = osal_fsize(fp);
    if(filesize != offset)
    {
        ret = osal_fseek(fp, offset);
        if(ret != FR_OK)
        {
            _os_printf("%s %d fseek failed, ret: %d\n", __FUNCTION__, __LINE__, ret);
            return ret;
        }
        ret = osal_ftruncate(fp);
        if(ret != FR_OK)
        {
            _os_printf("%s %d ftruncate failed, ret: %d\n", __FUNCTION__, __LINE__, ret);
            return ret;
        }
        _os_printf("avi size: %d\n", osal_fsize(fp));
        ret = osal_fseek(fp, 0);
        if(ret != FR_OK)
        {
            _os_printf("%s %d fseek failed, ret: %d\n", __FUNCTION__, __LINE__, ret);
        }
    }
    return ret;
}

static void avi_seek(F_FILE *fp, int32_t offset)
{
    if(!fp)
    {
        return;
    }

    osal_fseek(fp, offset);
}

static int avi_record_running(struct msi *msi, uint32_t save_time, void *fp, const char *avi_filename, uint32_t filesize)
{
    int                      ret                = AVI_RECORD_ERR_NONE;
    uint32_t                 avi_status         = 0;
    uint32_t                 write_start_time   = 0;
    uint32_t                 sys_start_time     = os_jiffies();
    uint32_t                 already_save_time  = 0;
    uint32_t                 last_sync_time     = os_jiffies();
    uint32_t                 fbtime             = 0;
    uint32_t                 video_count        = 0;
    uint32_t                 audio_count        = 0;
    uint32_t                 video_insert_count = 0;
    int                      timeouts           = 0;
    float                    time_diff          = 0;
    uint32_t                 width              = 0;
    uint32_t                 height             = 0;
    uint32_t                 fps                = AVI_RECORD_FPS;
    uint32_t                 audio_frq          = 0;
    int                      odml_ret           = 0;
    struct msi              *avi_thumb_msi      = NULL;
    struct framebuff        *fb                 = NULL;
    struct avi_record_msi_s *avi_record         = (struct avi_record_msi_s *)msi->priv;
    uint8_t                 *odml_header_buf    = NULL;
    AVI_INFO                *odml_msg           = NULL;
    ODMLBUFF                *odml_buff          = NULL;

    if (!fp)
    {
        os_sleep_ms(1);
        return AVI_RECORD_ERR_NO_SD;
    }

    avi_record->rec_second = 0;

    odml_header_buf = (uint8_t *)STREAM_LIBC_ZALLOC(_ODML_AVI_HEAD_SIZE__);
    odml_msg        = (AVI_INFO *)STREAM_LIBC_ZALLOC(sizeof(AVI_INFO));
    odml_buff       = (ODMLBUFF *)STREAM_LIBC_ZALLOC(sizeof(ODMLBUFF));
    if (!odml_header_buf || !odml_msg || !odml_buff)
    {
        AVI_DEBUG(KERN_ERR "%s %d malloc failed!\n", __FUNCTION__, __LINE__);
        ret = AVI_RECORD_ERR_NO_BUF;
        goto avi_record_running_end;
    }

    if (pre_avi_seek(fp, filesize) != RET_OK)
    {
        AVI_DEBUG(KERN_ERR "%s %d prepare file failed!\n", __FUNCTION__, __LINE__);
        ret = AVI_RECORD_ERR_NO_SD;
        goto avi_record_running_end;
    }

    if (avi_filename && avi_filename[0])
    {
        avi_thumb_msi = avi_thumb_msi_init(avi_filename, FRAMEBUFF_SOURCE_USB, FSTYPE_NONE);
    }
    msi->enable = 1;

    while (fp)
    {
        os_event_wait(&avi_record->evt, MSI_AVI_STOP, &avi_status, OS_EVENT_WMODE_OR, 0);
        if (avi_status & MSI_AVI_STOP)
        {
            ret = AVI_RECORD_ERR_STOP;
            goto avi_record_running_end;
        }

        fb = msi_get_fb(msi, 0);

        if (fb && fb->mtype == F_JPG)
        {
            timeouts = 0;

            if (write_start_time == 0)
            {
                write_start_time = fb->time;
                if (ex_parse_jpg(fb->data, fb->len, &width, &height))
                {
                    AVI_DEBUG(KERN_ERR "%s %d parse jpg failed!\n", __FUNCTION__, __LINE__);
                    ret = AVI_RECORD_ERR_NO_SD;
                    goto avi_record_running_end;
                }

                audio_frq            = avi_record->audio_encode ? audio_adc_get_samplerate(AUSYS_AUAD) : 0;
                odml_msg->win_w      = width;
                odml_msg->win_h      = height;
                odml_msg->frame_rate = fps;
                if (audio_frq)
                {
                    odml_msg->audiofrq = audio_frq;
                    odml_msg->pcm      = 1;
                }

                ODMLbuff_init(odml_buff);
                odml_buff->ef_time      = 1000 / fps;
                odml_buff->ef_fps       = fps;
                odml_buff->vframecnt    = 0;
                odml_buff->aframecnt    = 0;
                odml_buff->aframeSample = 0;
                odml_buff->sync_buf     = odml_header_buf;

                odml_ret = OMDLvideo_header_write(NULL, fp, odml_msg, (ODMLAVIFILEHEADER *)odml_header_buf);
                if (odml_ret < 0)
                {
                    AVI_DEBUG(KERN_ERR "%s %d opendml write head failed!\n", __FUNCTION__, __LINE__);
                    ret = AVI_RECORD_ERR_NO_SD;
                    goto avi_record_running_end;
                }
            }

            video_count++;
            _os_printf(KERN_INFO "O");
            odml_buff->cur_timestamp = fb->time;
            odml_ret                 = opendml_write_video2(odml_buff, fp, avi_record_write_cb, fb->len, fb->data);
            if (odml_ret < 0)
            {
                AVI_DEBUG(KERN_ERR "%s %d opendml write video failed!\n", __FUNCTION__, __LINE__);
                ret = AVI_RECORD_ERR_NO_SD;
                goto avi_record_running_end;
            }

            msi_delete_fb(NULL, fb);
            fb = NULL;

            video_insert_count = insert_frame(odml_buff, fp, &time_diff);
            video_count += video_insert_count;
            odml_buff->last_timestamp = odml_buff->cur_timestamp;
            fbtime                    = odml_buff->cur_timestamp;

            already_save_time      = odml_buff->cur_timestamp - write_start_time;
            avi_record->rec_second = already_save_time / 1000U;

            if (already_save_time >= save_time)
            {
                goto avi_record_running_end;
            }
        }
        else if (avi_record->audio_encode && write_start_time && fb && fb->mtype == F_AUDIO)
        {
            timeouts = 0;
            audio_count++;
            _os_printf(KERN_INFO "A");
            if (!odml_buff->aframeSample)
            {
                odml_buff->aframeSample = fb->len;
            }
            odml_ret = opendml_write_audio(odml_buff, fp, avi_record_write_cb, fb->len, fb->data);
            if (odml_ret < 0)
            {
                AVI_DEBUG(KERN_ERR "%s %d opendml write audio failed!\n", __FUNCTION__, __LINE__);
                ret = AVI_RECORD_ERR_NO_SD;
                goto avi_record_running_end;
            }
            msi_delete_fb(NULL, fb);
            fb = NULL;
        }
        else if (fb)
        {
            msi_delete_fb(NULL, fb);
            fb = NULL;
        }
        else
        {
            os_sleep_ms(1);
            timeouts++;
            if (write_start_time && timeouts > 1000)
            {
                AVI_DEBUG(KERN_ERR "%s %d timeout\n", __FUNCTION__, __LINE__);
                break;
            }
        }

        if (os_jiffies() - sys_start_time >= save_time + 30 * 1000U)
        {
            break;
        }

        if (fbtime && fbtime - last_sync_time > 1000U)
        {
            last_sync_time = fbtime;
            already_save_time = fbtime - write_start_time;
            avi_record->rec_second = already_save_time / 1000U;
            os_printf(KERN_DEBUG "avi second: %d\n", avi_record->rec_second);
        }

        os_sleep_ms(1);
    }

avi_record_running_end:
    avi_record->rec_second = 0;

    if (fb)
    {
        msi_delete_fb(NULL, fb);
        fb = NULL;
    }

    if (odml_buff && odml_msg && write_start_time)
    {
        uint32_t file_end = osal_ftell((F_FILE *)fp);
        uint32_t final_end;

        avi_seek(fp, file_end);

        if (!audio_count)
        {
            odml_msg->pcm = 0;
        }
        stdindx_updata(fp, odml_buff);
        final_end = osal_ftell((F_FILE *)fp);
        ODMLUpdateAVIInfo(fp, odml_buff, odml_msg->pcm, NULL, (ODMLAVIFILEHEADER *)odml_header_buf);
        avi_seek(fp, final_end);
    }
    else if (fp)
    {
        avi_seek(fp, 0);
    }

    if (avi_thumb_msi)
    {
        msi_destroy(avi_thumb_msi);
    }

    if (odml_header_buf)
    {
        STREAM_LIBC_FREE(odml_header_buf);
    }

    if (odml_msg)
    {
        STREAM_LIBC_FREE(odml_msg);
    }

    if (odml_buff)
    {
        STREAM_LIBC_FREE(odml_buff);
    }

    if (fp)
    {
        osal_fclose(fp);
    }

    AVI_DEBUG(KERN_INFO "%s end ret:%d v:%d a:%d\n", __FUNCTION__, ret, video_count, audio_count);
    return ret;
}

static void avi_record_thread(void *d)
{
    int                      ret          = 0;
    uint32_t                 avi_status   = 0;
    struct msi              *msi          = (struct msi *)d;
    struct avi_record_msi_s *avi_record   = (struct avi_record_msi_s *)msi->priv;
    struct file_process     *file_process = &avi_record->file_process;
    void                    *fp           = NULL;
    char                     filename[64];
    char                     filepath[64];
    uint32_t                 filesize     = 0;

    msi_get(msi);
    os_event_wait(&avi_record->evt, MSI_AVI_START | MSI_AVI_STOP, &avi_status, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);
    if (avi_status & MSI_AVI_STOP)
    {
        goto avi_record_thread_end;
    }

    while (msi)
    {
        filesize    = ((avi_record->rec_time / 60) + (avi_record->rec_time % 60 ? 1 : 0)) * avi_record->file_size;
        msi->enable = 1;

        if (file_process->create_file)
        {
            fp = file_process->create_file(file_process, filename, filepath, filesize);
        }

        ret = avi_record_running(msi, avi_record->rec_time * 1000U, fp, filename, filesize);

        msi->enable = 0;

        if (file_process->lock_file && ret != AVI_RECORD_ERR_NO_SD)
        {
            file_process->lock_file(filename, filepath);
        }

        if (ret)
        {
            if (file_process->loop_free)
            {
                file_process->loop_free(&file_process->loop);
            }

            if (ret == AVI_RECORD_ERR_NO_SD || ret == AVI_RECORD_ERR_NO_BUF)
            {
                avi_status = 0;
                os_event_wait(&avi_record->evt, MSI_AVI_STOP, &avi_status, OS_EVENT_WMODE_OR, 1000);
                if (avi_status & MSI_AVI_STOP)
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

avi_record_thread_end:
    while (1)
    {
        struct framebuff *fb = msi_get_fb(msi, 0);
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
}

static int32_t avi_record_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t                  ret        = RET_OK;
    struct avi_record_msi_s *avi_record = (struct avi_record_msi_s *)msi->priv;

    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
            os_event_wait(&avi_record->evt, MSI_AVI_THREAD_DEAD, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);
            os_event_del(&avi_record->evt);
            STREAM_LIBC_FREE(avi_record);
            break;

        case MSI_CMD_PRE_DESTROY:
            os_event_set(&avi_record->evt, MSI_AVI_STOP, NULL);
            break;

        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *)param1;

            if (fb->mtype == F_JPG && avi_record->filter_type != (uint8_t)~0)
            {
                if (avi_record->srcID != 0 && fb->srcID != avi_record->srcID)
                {
                    ret = RET_ERR;
                    break;
                }

                ret = RET_ERR;
                if (avi_record->filter_type == fb->stype)
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
                *(uint32_t *)param1 = (rflags & (MSI_AVI_THREAD_DEAD | MSI_AVI_STOP)) ? 0 : 1;
            }
        }
        break;

        case MSI_CMD_MEDIA_CTRL:
        {
            uint32_t cmd_self = (uint32_t)param1;
            uint32_t arg      = (uint32_t)param2;
            switch (cmd_self)
            {
                case MSI_MEDIA_CTRL_GET_RECTIME:
                    *(uint32_t *)arg = avi_record->rec_second;
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

                default:
                    break;
            }
        }
        break;

        default:
            break;
    }

    return ret;
}

struct msi *avi_record_msi_init(const char *avi_msi_name, uint8_t srcID, uint8_t filter_type, uint8_t rec_time,
                                uint32_t audio_encode, struct file_process *file_process, uint8_t mode)
{
    uint8_t                  is_new     = 0;
    struct avi_record_msi_s *avi_record = NULL;
    struct msi              *msi        = msi_new(avi_msi_name, 64, &is_new);

    if (is_new)
    {
        avi_record = (struct avi_record_msi_s *)STREAM_LIBC_ZALLOC(sizeof(struct avi_record_msi_s));
        ASSERT(avi_record);
        avi_record->filter_type  = filter_type;
        avi_record->srcID        = srcID;
        avi_record->mode         = mode;
        avi_record->rec_time     = (uint32_t)rec_time * 60U;
        avi_record->file_size    = MAX_SINGLE_AVI_SIZE;
        avi_record->audio_encode = audio_encode;

        if (file_process == NULL)
        {
            avi_record->file_process.loop        = NULL;
            avi_record->file_process.rec_path    = REC_PATH;
            avi_record->file_process.ext_name    = AVI_EXTENSION_NAME;
            avi_record->file_process.create_file = rec_create_file;
            avi_record->file_process.loop_free   = rec_loop_free;
            avi_record->file_process.lock_file   = NULL;
        }
        else
        {
            os_memcpy(&avi_record->file_process, file_process, sizeof(struct file_process));
        }

        msi->priv       = avi_record;
        msi->action     = avi_record_msi_action;
        msi->enable     = 1;
        avi_record->msi = msi;
        os_event_init(&avi_record->evt);
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

    void *avi_hdl = os_task_create("avi_record_msi", avi_record_thread, msi, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 2048);
    AVI_DEBUG("avi_hdl:%X\n", avi_hdl);
    if (!avi_hdl && avi_record)
    {
        os_event_set(&avi_record->evt, MSI_AVI_THREAD_DEAD, NULL);
    }

avi_record_msi_init_end:
    return msi;
}
