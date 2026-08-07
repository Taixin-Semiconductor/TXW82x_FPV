#include "basic_include.h"
#include "osal_file.h"
#include "stream_frame.h"
#include "lib/multimedia/msi.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "openDML.h"
#include "media.h"
#include "jpg_concat_msi.h"
#include "audio_msi/audio_adc.h"
#include "avi_record_msi.h"
#include "lib/video/dvp/jpeg/jpg.h"

//data申请空间函数
#define STREAM_MALLOC     av_psram_malloc
#define STREAM_FREE       av_psram_free
#define STREAM_ZALLOC     av_psram_zalloc

//结构体申请空间函数
#define STREAM_LIBC_MALLOC     av_malloc
#define STREAM_LIBC_FREE       av_free
#define STREAM_LIBC_ZALLOC     av_zalloc

#define RECORD_END_EVENT        (1 << 0)

#define AVI_DEBUG(fmt, ...)    os_printf(fmt, ##__VA_ARGS__)   

#define AVI_RECORD_DIR         "0:/AVI"


struct avi_record_msi_priv
{
    struct os_event event;
    uint32_t video_width;
    uint32_t video_height;
    uint8_t video_fps;
    uint32_t audio_frq;
    uint32_t record_time;       //时间单位为秒
    struct msi* jpg_msi;
    struct msi* avi_record_video_msi;
    struct msi* avi_record_audio_msi;
    user_callback   cb;
    void *user_priv;
    uint8_t thread_exit     : 1,
            thread_done     : 1,
            thread_running  : 1,
            reserve         : 5;
};

static struct avi_record_msi_priv *g_priv = NULL;

static int avi_record_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
//    struct avi_record_msi_priv *priv = (struct avi_record_msi_priv *)msi->priv;

    switch (cmd_id)
    {
        case MSI_CMD_PRE_DESTROY:
        {

        }
            break;

        case MSI_CMD_POST_DESTROY:
        {

        }
            break;
        
        case MSI_CMD_FREE_FB:
        {

        }
            break;

        default:
            break;
    }

    return ret;
}

static int avi_record_video_msi_cb(void *fp, void *data, int flen)
{
    return osal_fwrite(data, 1, flen, (F_FILE *)fp);
}

static int avi_record_audio_msi_cb(void *fp, void *data, int flen)
{
    return osal_fwrite(data, 1, flen, (F_FILE *)fp);
}

static void avi_record_msi_thread(void *arg)
{
    int ret = 0;
    void *fp = NULL;
    float time_diff = 0;
    int timeouts = 0;
    int video_insert_count = 0;
    int video_record_count = 0;
    int audio_record_count = 0;
    struct avi_record_msi_priv *priv = (struct avi_record_msi_priv *)arg;
    struct framebuff *video_fb = NULL;
    struct framebuff *audio_fb = NULL;

	uint8_t  *odml_header_buf = NULL;
	AVI_INFO *odml_msg        = NULL;
	ODMLBUFF *odml_buff       = NULL;

    odml_header_buf = (uint8_t  *)STREAM_LIBC_ZALLOC(_ODML_AVI_HEAD_SIZE__);
    odml_msg        = (AVI_INFO *)STREAM_LIBC_ZALLOC(sizeof(AVI_INFO));
    odml_buff       = (ODMLBUFF *)STREAM_LIBC_ZALLOC(sizeof(ODMLBUFF));
    if(!odml_header_buf || !odml_msg || !odml_buff)
    {
        AVI_DEBUG(KERN_ERR"%s %d malloc failed!\n",__FUNCTION__,__LINE__);
        goto __avi_record_end;
    }
    AVI_DEBUG(KERN_INFO"%s %d\n",__FUNCTION__,__LINE__);
    fp = create_video_file(AVI_RECORD_DIR);
    if (!fp)
    {
        AVI_DEBUG(KERN_ERR"%s %d create video file failed!\n",__FUNCTION__,__LINE__);
        goto __avi_record_end;
    }

	ODMLbuff_init(odml_buff);
	odml_buff->ef_time = 1000 / priv->video_fps;
	odml_buff->ef_fps = priv->video_fps;
	odml_buff->vframecnt = 0;
	odml_buff->aframecnt = 0;
	odml_buff->aframeSample = 0;                    //这里不赋值,等到第一帧音频帧来了才赋值
	odml_buff->sync_buf = odml_header_buf;
    
    if(priv->audio_frq)
    {
        odml_msg->audiofrq = priv->audio_frq; 
        odml_msg->pcm = 1;
    }

    odml_msg->win_w = priv->video_width;
    odml_msg->win_h = priv->video_height;
    AVI_DEBUG(KERN_INFO"%s %d\n",__FUNCTION__,__LINE__);
    ret = OMDLvideo_header_write(NULL, fp, odml_msg, (ODMLAVIFILEHEADER *)odml_header_buf);
    if (ret < 0) {
        AVI_DEBUG(KERN_ERR"%s %d opendml write head failed!\n",__FUNCTION__,__LINE__);
        goto __avi_record_end;
    }
    AVI_DEBUG(KERN_INFO"%s %d\n",__FUNCTION__,__LINE__);
    while(!priv->thread_exit)
    {
        if (priv->avi_record_video_msi)
        {
            video_fb = msi_get_fb(priv->avi_record_video_msi, 0);

            if (video_fb)
            {
                timeouts = 0;
                video_record_count ++;
                odml_buff->cur_timestamp = os_jiffies();
                ret = opendml_write_video2(odml_buff, fp, avi_record_video_msi_cb, video_fb->len,video_fb->data);
                if (ret < 0) {
                    AVI_DEBUG(KERN_ERR"%s %d opendml write VIDEO failed!\n",__FUNCTION__,__LINE__);
                    goto __avi_record_end;
                }
                msi_delete_fb(NULL, video_fb);
                video_insert_count = insert_frame(odml_buff,fp,&time_diff);
                video_record_count += video_insert_count;
                odml_buff->last_timestamp = odml_buff->cur_timestamp;
            }

        }

        if (priv->avi_record_audio_msi)
        {
            audio_fb = msi_get_fb(priv->avi_record_audio_msi, 0);

            if (audio_fb)
            {
                timeouts = 0;
                audio_record_count ++;
                if (!odml_buff->aframeSample) {
                    odml_buff->aframeSample = audio_fb->len;
                    os_printf("audio flen:%d\n",audio_fb->len);
                }
                ret = opendml_write_audio(odml_buff, fp, avi_record_audio_msi_cb, audio_fb->len, audio_fb->data);
                if (ret < 0) {
                    AVI_DEBUG(KERN_ERR"%s %d opendml write AUDIO failed!\n",__FUNCTION__,__LINE__);
                    goto __avi_record_end;
                }
                msi_delete_fb(NULL, audio_fb);        
            }
        }


        if (!video_fb && !audio_fb)
        {
            os_sleep_ms(1);
            if (fp)
            {
                timeouts++;
                if (timeouts > 1000)
                {
                    video_fb = NULL;
                    audio_fb = NULL;
                    AVI_DEBUG(KERN_ERR"%s %d timeout\n",__FUNCTION__,__LINE__);
                    break;
                }
            }
        }

        video_fb = NULL;
        audio_fb = NULL;

        if (video_record_count > 30 * priv->record_time)
        {
            AVI_DEBUG(KERN_INFO"%s %d success\n",__FUNCTION__,__LINE__);
            break;
        }

    }

    if (!audio_record_count) {
        odml_msg->pcm = 0;
    }

    stdindx_updata(fp, odml_buff);
    ODMLUpdateAVIInfo(fp, odml_buff, odml_msg->pcm, NULL, (ODMLAVIFILEHEADER *)odml_header_buf);

    AVI_DEBUG(KERN_INFO"%s %d video_count:%d audio_count:%d\n",__FUNCTION__,__LINE__,video_record_count,audio_record_count);

__avi_record_end:

    AVI_DEBUG(KERN_INFO"%s %d avi record end\n",__FUNCTION__,__LINE__);

    if (odml_header_buf) {
        STREAM_LIBC_FREE(odml_header_buf);
        odml_header_buf = NULL;
    }

    if (odml_msg) {
        STREAM_LIBC_FREE(odml_msg);
        odml_msg = NULL;
    }

    if (odml_buff) {
        STREAM_LIBC_FREE(odml_buff);
        odml_buff = NULL;
    }

    if (video_fb) {
        msi_delete_fb(NULL, video_fb);
        video_fb = NULL;
    }

    if (audio_fb) {
        msi_delete_fb(NULL, audio_fb);
        audio_fb = NULL;
    }

    if (priv->jpg_msi) {
        msi_del_output(priv->jpg_msi, NULL, R_RECORD_JPEG);
        msi_put(priv->jpg_msi);
        priv->jpg_msi = NULL;
    }

    if (priv->avi_record_audio_msi) {
        msi_destroy(priv->avi_record_audio_msi);
        priv->avi_record_audio_msi = NULL;
    }

    if (priv->avi_record_video_msi) {
        msi_destroy(priv->avi_record_video_msi);
        priv->avi_record_video_msi = NULL;
    }

    if(fp)
	{
		osal_fclose(fp);
		fp = NULL;
	}

    priv->thread_done = 1;
    priv->thread_running = 0;
    os_event_set(&priv->event, RECORD_END_EVENT, NULL);

    if (priv->cb) {
        priv->cb(priv->user_priv);
    }

}


uint32_t* avi_record_msi_init(uint32_t video_width, uint32_t video_height, uint8_t video_fps, uint32_t audio_frq, uint32_t record_time, user_callback cb, void *user_priv)
{
    AVI_DEBUG("video_width:%d video_height:%d video_fps:%d audio_frq:%d record_time:%d\n",video_width,video_height,video_fps,audio_frq,record_time);
    void *ret = NULL;
    struct avi_record_msi_priv *priv = NULL;
    if (!g_priv) {
        priv = (struct avi_record_msi_priv *)STREAM_LIBC_ZALLOC(sizeof(struct avi_record_msi_priv)); 
        AVI_DEBUG("priv:%x\n",priv);
        if (!priv) {
            AVI_DEBUG(KERN_ERR"%s %d malloc failed!\n",__FUNCTION__,__LINE__);
            return NULL;
        }

        os_event_init(&priv->event);
        
        g_priv = priv;
    } else {
        priv = g_priv;

        if (os_event_wait(&priv->event, RECORD_END_EVENT, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0)) {
            AVI_DEBUG(KERN_INFO"%s %d thread is running\n",__FUNCTION__,__LINE__);
            return (uint32_t*)priv;
        }
    }

    priv->thread_exit    = 0;
    priv->thread_done    = 0;
    priv->thread_running = 1;
    priv->video_width    = video_width;
    priv->video_height   = video_height;
    priv->video_fps      = video_fps;
    priv->audio_frq      = audio_frq;
    priv->record_time    = record_time;
    if (cb) {
        priv->cb = cb;
        priv->user_priv = user_priv;
    }


    if (video_fps) {
        priv->avi_record_video_msi = msi_new(R_RECORD_JPEG, 2, 0);
        if (priv->avi_record_video_msi)
        {
            priv->avi_record_video_msi->priv = priv;
            priv->avi_record_video_msi->action = avi_record_msi_action;
            priv->avi_record_video_msi->enable = 1;
        }

        //MJPEG 默认从 VPP BUF0 接收数据
        priv->jpg_msi = msi_find(AUTO_JPG, 1);
        if (priv->jpg_msi)
        {
            msi_add_output(priv->jpg_msi, NULL, R_RECORD_JPEG);
            priv->jpg_msi->enable = 1;
        }
    }

    if (audio_frq) {
        priv->avi_record_audio_msi = msi_new(R_RECORD_AUDIO, 8, 0);
        if (priv->avi_record_audio_msi)
        {
            //默认已经打开audio adc msi数据流，此处只做绑定操作
            auadc_msi_add_output(R_RECORD_AUDIO);
            priv->avi_record_audio_msi->priv = priv;
            priv->avi_record_audio_msi->action = avi_record_msi_action;
            priv->avi_record_audio_msi->enable = 1;
        }
    }
    AVI_DEBUG(KERN_INFO"%s %d\n",__FUNCTION__,__LINE__);
    AVI_DEBUG(KERN_INFO"%s %d jpg_msi:%x video_msi:%x audio_msi:%x\n",__FUNCTION__,__LINE__,priv->jpg_msi,priv->avi_record_video_msi,priv->avi_record_audio_msi);
    ret = os_task_create("avi_record_msi", avi_record_msi_thread, priv, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);
    if (!ret) {
        os_event_set(&priv->event, RECORD_END_EVENT, NULL);
        priv = NULL;
    }

    return (uint32_t*)priv;
}

int avi_record_msi_deinit()
{
    int timeout = 1000;
    struct avi_record_msi_priv *priv = g_priv;
    if (priv) {
        priv->thread_exit = 1;
        while (os_event_wait(&priv->event, RECORD_END_EVENT, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0)) {
            os_sleep_ms(1);
            timeout--;
            if (!timeout) {
                AVI_DEBUG(KERN_ERR"%s %d timeout!\n",__FUNCTION__,__LINE__);
                return -2;
            }
        }

        if (priv->jpg_msi) {
            msi_del_output(priv->jpg_msi, NULL, R_RECORD_JPEG);
            msi_put(priv->jpg_msi);
            priv->jpg_msi = NULL;
        }
    
        if (priv->avi_record_audio_msi) {
            auadc_msi_del_output(R_RECORD_AUDIO);
            msi_destroy(priv->avi_record_audio_msi);
            priv->avi_record_audio_msi = NULL;
        }
    
        if (priv->avi_record_video_msi) {
            msi_destroy(priv->avi_record_video_msi);
            priv->avi_record_video_msi = NULL;
        }

        g_priv = NULL;
        STREAM_LIBC_FREE(priv);
        return 0;
    }
    return -1;
}