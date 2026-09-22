#include "basic_include.h"
#include "fatfs/osal_file.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/multimedia/msi.h"
#include "stream_define.h"
#include "lib/video/muxer/odml_mux.h"
#include "app/video_app/file_thumb.h"
#include "audio_msi/audio_adc.h"
#include "video_record.h"

int ex_parse_jpg(uint8_t *jpg_buf, uint32_t maxsize, uint32_t *w, uint32_t *h);
struct msi *avi_thumb_msi_init(const char *filename, uint8_t srcID, uint8_t filter);

#define STREAM_LIBC_FREE    os_free
#define STREAM_LIBC_ZALLOC  os_zalloc

#ifndef ODML_MAX_SINGLE_SIZE
#define ODML_MAX_SINGLE_SIZE             (100 * 1024 * 1024)
#endif

#define ODML_BUFFER_TIME                 2500U

enum
{
    MSI_ODML_START       = BIT(0),
    MSI_ODML_STOP        = BIT(1),
    MSI_ODML_THREAD_DEAD = BIT(2),
};

enum
{
    ODML_RECORD_ERR_NONE,
    ODML_RECORD_ERR_STOP,
    ODML_RECORD_ERR_NO_SD,
    ODML_RECORD_ERR_NO_BUF,
};

struct odml_record_msi_s
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
};

static uint16_t odml_get_node_size(uint8_t video_fps, uint8_t audio_en)
{
    uint16_t audio_fps = 0;

    if (audio_en)
    {
        audio_fps = (1000U + AUADC_TIME_INTERVAL - 1U) / AUADC_TIME_INTERVAL;
    }

    return ((video_fps + audio_fps) * ODML_BUFFER_TIME + 1000U - 1U) / 1000U;
}

static int odml_record_write_cb(void *fp, void *data, int flen)
{
    return osal_fwrite(data, 1, flen, (F_FILE *) fp);
}

static int odml_pre_avi_seek(F_FILE *fp, uint32_t offset)
{
    int      ret      = RET_OK;
    uint32_t filesize = osal_fsize(fp);

    if (filesize != offset)
    {
        ret = osal_fseek(fp, offset);
        if (ret != FR_OK)
        {
            _os_printf("%s %d fseek failed, ret: %d\n", __FUNCTION__, __LINE__, ret);
            return ret;
        }

        ret = osal_ftruncate(fp);
        if (ret != FR_OK)
        {
            _os_printf("%s %d ftruncate failed, ret: %d\n", __FUNCTION__, __LINE__, ret);
            return ret;
        }

        _os_printf("avi size: %d\n", osal_fsize(fp));
        ret = osal_fseek(fp, 0);
        if (ret != FR_OK)
        {
            _os_printf("%s %d fseek failed, ret: %d\n", __FUNCTION__, __LINE__, ret);
        }
    }

    return ret;
}

static void odml_avi_seek(F_FILE *fp, int32_t offset)
{
    if (fp)
    {
        osal_fseek(fp, offset);
    }
}

static int odml_record_running(struct msi *msi, uint32_t save_time, void *fp, const char *avi_filename, uint32_t filesize)
{
    int                       ret                = ODML_RECORD_ERR_NONE;
    int                       odml_ret           = 0;
    uint32_t                  odml_status        = 0;
    uint32_t                  write_start_time   = 0;
    uint32_t                  sys_start_time     = os_jiffies();
    uint32_t                  last_sync_time     = 0;
    uint32_t                  already_save_time  = 0;
    uint32_t                  fbtime             = 0;
    uint32_t                  video_count        = 0;
    uint32_t                  audio_count        = 0;
    uint32_t                  width              = 0;
    uint32_t                  height             = 0;
    uint32_t                  audio_first_time   = 0;
    uint32_t                  video_fps          = 0;
    uint32_t                  audio_sr           = 0;
    uint8_t                   stop_draining      = 0;
    uint8_t                   header_written     = 0;
    float                     time_diff          = 0;
    struct odml_record_msi_s *odml_record        = (struct odml_record_msi_s *) msi->priv;
    struct msi               *avi_thumb_msi      = NULL;
    struct framebuff         *fb                 = NULL;
    uint8_t                  *odml_header_buf    = NULL;
    AVI_INFO                 *odml_msg           = NULL;
    ODMLBUFF                 *odml_buff          = NULL;

    if (!fp)
    {
        os_sleep_ms(1);
        return ODML_RECORD_ERR_NO_SD;
    }

    video_fps              = odml_record->video_fps ? odml_record->video_fps : 25U;
    odml_record->rec_second = 0;

    odml_header_buf = (uint8_t *) STREAM_LIBC_ZALLOC(_ODML_AVI_HEAD_SIZE__);
    odml_msg        = (AVI_INFO *) STREAM_LIBC_ZALLOC(sizeof(AVI_INFO));
    odml_buff       = (ODMLBUFF *) STREAM_LIBC_ZALLOC(sizeof(ODMLBUFF));
    if (!odml_header_buf || !odml_msg || !odml_buff)
    {
        os_printf(KERN_ERR "%s %d malloc failed!\n", __FUNCTION__, __LINE__);
        ret = ODML_RECORD_ERR_NO_BUF;
        goto odml_record_running_clean_end;
    }

    if (odml_pre_avi_seek((F_FILE *) fp, filesize) != RET_OK)
    {
        os_printf(KERN_ERR "%s %d prepare file failed!\n", __FUNCTION__, __LINE__);
        ret = ODML_RECORD_ERR_NO_SD;
        goto odml_record_running_clean_end;
    }

    if (avi_filename && avi_filename[0])
    {
        avi_thumb_msi = avi_thumb_msi_init(avi_filename, FRAMEBUFF_SOURCE_USB, FSTYPE_NONE);
    }
    msi->enable = 1;

    while (fp)
    {
        os_event_wait(&odml_record->evt, MSI_ODML_STOP, &odml_status, OS_EVENT_WMODE_OR, 0);
        if (odml_status & MSI_ODML_STOP)
        {
            ret            = ODML_RECORD_ERR_STOP;
            msi->enable    = 0;
            stop_draining  = 1;
        }

        if (fb == NULL)
        {
            fb = msi_get_fb(msi, 1);
        }

        if (stop_draining && fb == NULL)
        {
            goto odml_record_running_end;
        }

        if (fb && fb->mtype == F_JPG)
        {
            if (write_start_time == 0)
            {
                write_start_time = fb->time;
                if (ex_parse_jpg(fb->data, fb->len, &width, &height))
                {
                    os_printf(KERN_ERR "%s %d parse jpg failed!\n", __FUNCTION__, __LINE__);
                    ret = ODML_RECORD_ERR_NO_SD;
                    goto odml_record_running_clean_end;
                }

                audio_sr            = odml_record->audio_en ? audio_adc_get_samplerate(MAIN_MIC_ID) : 0;
                odml_msg->win_w      = width;
                odml_msg->win_h      = height;
                odml_msg->frame_rate = video_fps;
                if (audio_sr)
                {
                    odml_msg->audiofrq = audio_sr;
                    odml_msg->pcm      = 1;
                }

                ODMLbuff_init(odml_buff);
                odml_buff->ef_time      = 1000U / video_fps;
                odml_buff->ef_fps       = video_fps;
                odml_buff->vframecnt    = 0;
                odml_buff->aframecnt    = 0;
                odml_buff->aframeSample = 0;
                odml_buff->sync_buf     = odml_header_buf;

                odml_ret = OMDLvideo_header_write(NULL, fp, odml_msg, (ODMLAVIFILEHEADER *) odml_header_buf);
                if (odml_ret < 0)
                {
                    os_printf(KERN_ERR "%s %d opendml write head failed!\n", __FUNCTION__, __LINE__);
                    ret = ODML_RECORD_ERR_NO_SD;
                    goto odml_record_running_clean_end;
                }
                header_written = 1;
            }

            _os_printf(KERN_INFO "O");
            odml_buff->cur_timestamp = fb->time;
            odml_ret = opendml_write_video2(odml_buff, fp, odml_record_write_cb, fb->len, fb->data);
            if (odml_ret < 0)
            {
                os_printf(KERN_ERR "%s %d opendml write video failed!\n", __FUNCTION__, __LINE__);
                ret = ODML_RECORD_ERR_NO_SD;
                goto odml_record_running_clean_end;
            }

            video_count++;
            msi_delete_fb(NULL, fb);
            fb = NULL;

            video_count += insert_frame(odml_buff, fp, &time_diff);
            odml_buff->last_timestamp = odml_buff->cur_timestamp;
            fbtime                    = odml_buff->cur_timestamp;

            if (!stop_draining && fbtime - write_start_time >= save_time)
            {
                goto odml_record_running_end;
            }
        }
        else if (odml_record->audio_en && write_start_time && fb && fb->mtype == MEDIA_DATA_AUDIO)
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
                    msi_delete_fb(NULL, fb);
                    fb = NULL;
                    continue;
                }
            }

            if (!odml_buff->aframeSample)
            {
                odml_buff->aframeSample = fb->len;
            }

            _os_printf(KERN_INFO "A");
            odml_ret = opendml_write_audio(odml_buff, fp, odml_record_write_cb, fb->len, fb->data);
            if (odml_ret < 0)
            {
                ret = ODML_RECORD_ERR_NO_SD;
                goto odml_record_running_clean_end;
            }
            audio_count++;
            msi_delete_fb(NULL, fb);
            fb = NULL;
        }
        else if (fb)
        {
            msi_delete_fb(NULL, fb);
            fb = NULL;
        }

        if (os_jiffies() - sys_start_time >= save_time + 30U * 1000U)
        {
            goto odml_record_running_clean_end;
        }

        if (fbtime - last_sync_time > 1000U)
        {
            last_sync_time            = fbtime;
            already_save_time         = fbtime - write_start_time;
            odml_record->rec_second   = already_save_time / 1000U;
            os_printf(KERN_DEBUG "odml second: %d\n", odml_record->rec_second);
        }
    }

odml_record_running_clean_end:
    if (fb)
    {
        msi_delete_fb(NULL, fb);
        fb = NULL;
    }

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

odml_record_running_end:
    odml_record->rec_second = 0;

    if (header_written)
    {
        if (!audio_count)
        {
            odml_msg->pcm = 0;
        }

        odml_avi_seek((F_FILE *) fp, osal_ftell((F_FILE *) fp));
        stdindx_updata(fp, odml_buff);
        uint32_t final_end = osal_ftell((F_FILE *) fp);
        ODMLUpdateAVIInfo(fp, odml_buff, odml_msg->pcm, NULL, (ODMLAVIFILEHEADER *) odml_header_buf);
        odml_avi_seek((F_FILE *) fp, final_end);
    }

    if (!header_written && fp)
    {
        odml_avi_seek((F_FILE *) fp, 0);
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

    os_printf(KERN_DEBUG "odml encode end, ret: %d, v: %d, a: %d\n", ret, video_count, audio_count);
    return ret;
}

static void odml_record_thread(void *d)
{
    int                       ret          = ODML_RECORD_ERR_NONE;
    uint32_t                  odml_status  = 0;
    struct msi               *msi          = (struct msi *) d;
    struct odml_record_msi_s *odml_record  = (struct odml_record_msi_s *) msi->priv;
    struct file_process      *file_process = &odml_record->file_process;
    void                     *fp           = NULL;
    char                      filename[64];
    char                      filepath[64];
    uint32_t                  filesize     = 0;

    msi_get(msi);
    os_event_wait(&odml_record->evt, MSI_ODML_START | MSI_ODML_STOP, &odml_status,
                  OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);
    if (odml_status & MSI_ODML_STOP)
    {
        goto odml_record_thread_end;
    }

    while (msi)
    {
        filesize = ((odml_record->rec_time / 60U) + (odml_record->rec_time % 60U ? 1U : 0U)) * odml_record->file_size;
        if (file_process->create_file)
        {
            fp = file_process->create_file(file_process, filename, filepath, filesize);
        }

        ret = odml_record_running(msi, odml_record->rec_time * 1000U, fp, filename, filesize);
        msi->enable = 0;

        if (file_process->lock_file && ret != ODML_RECORD_ERR_NO_SD)
        {
            file_process->lock_file(filename, filepath);
        }

        if (ret)
        {
            if (file_process->loop_free)
            {
                file_process->loop_free(&file_process->loop);
            }

            if (ret == ODML_RECORD_ERR_NO_SD || ret == ODML_RECORD_ERR_NO_BUF)
            {
                odml_status = 0;
                os_event_wait(&odml_record->evt, MSI_ODML_STOP, &odml_status, OS_EVENT_WMODE_OR, 1000);
                if (odml_status & MSI_ODML_STOP)
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

odml_record_thread_end:
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

    os_event_set(&odml_record->evt, MSI_ODML_THREAD_DEAD, NULL);
    msi_put(msi);
    os_printf(KERN_DEBUG "%s: %d end\n", __FUNCTION__, __LINE__);
}

static int32_t odml_record_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t                   ret         = RET_OK;
    struct odml_record_msi_s *odml_record = (struct odml_record_msi_s *) msi->priv;

    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
            os_event_wait(&odml_record->evt, MSI_ODML_THREAD_DEAD, NULL,
                          OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, -1);
            os_event_del(&odml_record->evt);
            STREAM_LIBC_FREE(odml_record);
            break;

        case MSI_CMD_PRE_DESTROY:
            os_event_set(&odml_record->evt, MSI_ODML_STOP, NULL);
            break;

        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;

            if (fb->mtype == F_JPG && odml_record->filter != (uint8_t) ~0)
            {
                if (odml_record->srcID != 0 && fb->srcID != odml_record->srcID)
                {
                    ret = RET_ERR;
                    break;
                }

                ret = RET_ERR;
                if (odml_record->filter == fb->stype)
                {
                    ret = RET_OK;
                }
            }
        }
        break;

        case MSI_CMD_GET_RUNNING:
        {
            uint32_t rflags = 0;

            os_event_wait(&odml_record->evt, MSI_ODML_THREAD_DEAD | MSI_ODML_STOP, &rflags, OS_EVENT_WMODE_OR, 0);
            if (param1)
            {
                *(uint32_t *) param1 = (rflags & (MSI_ODML_THREAD_DEAD | MSI_ODML_STOP)) ? 0 : 1;
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
                    *(uint32_t *) arg = odml_record->rec_second;
                    break;

                case MSI_MEDIA_CTRL_RECORD_START:
                    os_event_set(&odml_record->evt, MSI_ODML_START, NULL);
                    break;

                case MSI_MEDIA_CTRL_SET_RECORD_SIZE:
                    odml_record->file_size = arg;
                    break;

                case MSI_MEDIA_CTRL_SET_RECORD_SEC:
                    odml_record->rec_time = arg;
                    break;
            }
        }
        break;
    }

    return ret;
}

struct msi *odml_record_msi_init(struct video_record_cfg *cfg)
{
    uint16_t                  video_fps  = cfg->video_fps ? cfg->video_fps : 25U;
    uint16_t                  node_size  = odml_get_node_size(video_fps, cfg->audio_en);
    uint8_t                   is_new     = 0;
    struct odml_record_msi_s *odml_record = NULL;
    struct msi               *msi        = msi_new(cfg->name, node_size, &is_new);

    if (is_new)
    {
        odml_record = (struct odml_record_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct odml_record_msi_s));
        ASSERT(odml_record);
        odml_record->filter    = cfg->filter;
        odml_record->srcID     = cfg->srcID;
        odml_record->rec_time  = (uint32_t) cfg->rec_time * 60U;
        odml_record->file_size = ODML_MAX_SINGLE_SIZE;
        odml_record->audio_en  = cfg->audio_en;
        odml_record->video_fps = video_fps;

        if (cfg->file_process == NULL)
        {
            odml_record->file_process.loop        = NULL;
            odml_record->file_process.rec_path    = REC_PATH;
            odml_record->file_process.ext_name    = AVI_EXTENSION_NAME;
            odml_record->file_process.create_file = rec_create_file;
            odml_record->file_process.loop_free   = rec_loop_free;
            odml_record->file_process.lock_file   = NULL;
            odml_record->file_process.need_lock   = 0;
        }
        else
        {
            os_memcpy(&odml_record->file_process, cfg->file_process, sizeof(struct file_process));
        }

        msi->priv          = odml_record;
        msi->action        = odml_record_msi_action;
        odml_record->msi   = msi;
        os_event_init(&odml_record->evt);
    }
    else
    {
        if (msi)
        {
            msi_destroy(msi);
            msi = NULL;
        }
        goto odml_record_msi_init_end;
    }

    void *odml_hdl = os_task_create("odml_record", odml_record_thread, msi,
                                    OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 2048);
    os_printf(KERN_DEBUG "odml_hdl: %x\n", odml_hdl);
    if (!odml_hdl && odml_record)
    {
        os_event_set(&odml_record->evt, MSI_ODML_THREAD_DEAD, NULL);
    }

odml_record_msi_init_end:
    return msi;
}
