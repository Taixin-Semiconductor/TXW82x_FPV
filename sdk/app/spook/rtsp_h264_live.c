#include "sys_config.h"
#include "tx_platform.h"
#include <csi_kernel.h>
#include "lwip\sockets.h"
#include "lwip\netif.h"
#include "lwip\dns.h"
#include "lwip\api.h"
#include "lwip\tcp.h"

#include "rtsp_common.h"
#include "osal/string.h"
#include "stream_define.h"
#include "video_app/video_app.h"
#include "video_app/video_app_h264_msi.h"
#include "stream_frame.h"
#include "log.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "frame.h"
#include "audio_media_ctrl/audio_code_ctrl.h"
#include "audio_msi/audio_adc.h"
#include "app/video_app/file_thumb.h"

extern struct msi *mp4_demux_msi_init(const char *msi_name, const char *filename);
extern struct msi *rtsp_msi_init(const char *name, uint8_t filter_type,uint8_t srcID);

extern void spook_send_thread_stream(struct rtsp_priv *r);
static void self_thread(void *d)
{
    struct rtsp_priv *r = (struct rtsp_priv *) d;
    spook_send_thread_stream(r);
}
// 创建实时预览的的线程
static void self_creat(struct rtsp_source *source, void *priv)
{
    source->priv           = (struct rtsp_priv *) os_zalloc(sizeof(struct rtsp_priv));
    struct rtsp_priv *r    = (struct rtsp_priv *) source->priv;
    uint8_t          *path = (uint8_t *) priv;
    const char       *match_path = (const char *) priv;
    uint8_t default_value = 0;
    os_printf("path:%s\n", path);

    while((*match_path) && (*match_path != '?'))
    {
        match_path++;
    }

    if(*match_path == '?')
    {
        match_path++;
    }
    
    os_printf("match_path:%s\n",match_path);
    //匹配?的后缀(仅仅特定匹配)
    if(*match_path)
    {
        default_value = *match_path - '0';
        if(default_value > 1)
        {
            default_value = 0;
        }
    }
    os_printf("default_value:%d\n",default_value);
    uint8_t mp4_flag = 0;
    if (source->priv)
    {
        r->live_node = &source->live_node;
        if (memcmp(path, "/loop/RECA/", strlen("/loop/RECA/")) == 0)
        {
            uint8_t filepath[64] = {0};
            os_sprintf((char *) filepath, REC_PATH "/%s", path + strlen("/loop/RECA/"));
            // 尝试打开文件
            os_printf("filepath:%s\n", filepath);
            mp4_flag     = 1;
            r->video_msi = mp4_demux_msi_init("demux_mp4", (char *) filepath);
#if AUDIO_EN == 1
            if (r->video_msi)
            {
                r->a_msi = rtsp_audio_msi_init(R_RTP_AUDIO);
                msi_add_output(r->video_msi, NULL, R_RTP_AUDIO);
            }
#endif
        }
        else
        {
#if AUDIO_EN == 1
            r->a_msi = rtsp_audio_msi_init(R_RTP_AUDIO);
            if (audio_encode_init(AAC_ENC, AUADC_SAMPLERATE) != NULL)
            {
                auadc_msi_add_output(audio_code_msi_name(AAC_ENC));
                r->audio_coder_ret = 1;
            }
            if (r->audio_coder_ret)
            {
                audio_code_add_output(AAC_ENC, R_RTP_AUDIO);
            }
#endif
            r->video_msi = msi_find(AUTO_H264, 1);
        }
        if (r->video_msi)
        {
            if (mp4_flag)
            {
                r->v_msi = rtsp_msi_init(R_RTP_JPEG_H264, FSTYPE_H264_FILE,FRAMEBUFF_SOURCE_NONE);
            }
            else
            {
                if(default_value == 0)
                {
                    #ifndef FORCE_SCALE_TO_H264
                    r->v_msi = rtsp_msi_init(R_RTP_JPEG_H264, FSTYPE_H264_VPP_DATA0,FRAMEBUFF_SOURCE_CAMERA0);
                    #else
                    r->v_msi = rtsp_msi_init(R_RTP_JPEG_H264, FSTYPE_H264_SCALER_DATA,FRAMEBUFF_SOURCE_CAMERA0);
                    #endif
                }
                else if(default_value == 1)
                {
                    r->v_msi = rtsp_msi_init(R_RTP_JPEG_H264, FSTYPE_H264_GEN420_DATA,FRAMEBUFF_SOURCE_NONE);
                }
            }

            // r->write_test_sd_msi = mp4_encode_msi_init("mp4_write");
            if (r->v_msi)
            {
                msi_add_output(r->video_msi, NULL, R_RTP_JPEG_H264); // 将video_msi的数据输出到R_RTP_JPEG_H264的msi,即v_msi
                msi_do_cmd(r->video_msi, MSI_CMD_VIDEO_DEMUX_CTRL, MSI_VIDEO_DEMUX_START, 0);
                OS_TASK_INIT("live_rtsp", &source->handle, self_thread, r, OS_TASK_PRIORITY_NORMAL, NULL, 1024);
            }
        }
        // 这里没有增加容错
        else
        {
            os_printf("%s h264 msi fail\n", __FUNCTION__);
            return;
        }
    }
    else
    {
        os_printf("%s not enough space\n", __FUNCTION__);
    }

    return;
}

static void self_destory(struct rtsp_source *source)
{
    // stop_jpeg();
    void             *tmp = source->handle.hdl;
    struct rtsp_priv *r   = source->priv;
    if (r)
    {
        if (r->video_msi)
        {
            msi_del_output(r->video_msi, NULL, R_RTP_JPEG_H264);
            msi_put(r->video_msi);
            r->video_msi = NULL;
        }
        rtsp_msi_deinit((void *) r->v_msi);
#if AUDIO_EN == 1
        if (r->audio_coder_ret)
        {
            audio_code_del_output(AAC_ENC, R_RTP_AUDIO);
            if (audio_encode_deinit(AAC_ENC) == RET_OK)
            {
                auadc_msi_del_output(audio_code_msi_name(AAC_ENC));
            }
        }
        rtsp_audio_msi_deinit((void *) r->a_msi);
#endif
        os_free(source->priv);
        source->priv = NULL;
    }
    if (tmp)
    {
        os_task_del(&source->handle);
    }
}

static void self_play(struct rtsp_source *source)
{
    struct rtsp_priv *r = (struct rtsp_priv *) source->priv;
    if (r->v_msi)
    {
        msi_do_cmd(r->video_msi, MSI_CMD_VIDEO_DEMUX_CTRL, MSI_VIDEO_DEMUX_START, 0);
    }
}

static int self_get_sdp(struct session *s, char *dest, int *len, char *path)
{
    struct rtsp_session *ls   = (struct rtsp_session *) s->private;
    int                  i    = 0;
    int                  t    = 0;
    char                *addr = "IP4 0.0.0.0";
    os_printf("%s:%d\tpath:%s\n", __FUNCTION__, __LINE__, path);

    if (s->ep[0] && s->ep[0]->trans_type == RTP_TRANS_UDP)
    {
        addr = s->ep[0]->trans.udp.sdp_addr;
    }

    i = snprintf(dest, *len, "v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=Test\r\na=type:broadcast\r\nt=0 0\r\nc=IN %s\r\n", addr);
    for (t = 0; t < MAX_TRACKS && ls->source->track[t].rtp; ++t)
    {
        int port;

        if (s->ep[t] && s->ep[t]->trans_type == RTP_TRANS_UDP)
        {
            port = s->ep[t]->trans.udp.sdp_port;
        }
        else
        {
            port = 0;
        }

        if (ls->source->track[t].rtp->type == 0)
        {
            i += ls->source->track[t].rtp->get_sdp(dest + i, *len - i, 96 + t, port, ls->source->track[t].rtp->private);
        }
        else
        {
            i += ls->source->track[t].rtp->get_sdp(dest + i, *len - i, 96 + t, port, NULL);
        }

        if (port == 0) // XXX What's a better way to do this?
        {
            i += sprintf(dest + i, "a=control:track%d\r\n", t);
        }
    }
    *len = i;
    return t;
}

static struct session *self_rtsp_open(char *path, void *d)
{
    // 默认的,各自模式可以各自去修改
    struct session *sess = rtsp_open(path, d);
    if (sess)
    {
        sess->get_sdp = self_get_sdp;
    }
    return sess;
}

void rtsp_h264_live_init(const rtp_name *rtsp)
{
    struct rtsp_source *source;
    source = rtsp_start_block();
    rtsp_set_path(rtsp->path, source, self_rtsp_open);
    set_video_h264_track((char *) rtsp->video_encode_name, source);
#if AUDIO_EN == 1
    set_audio_track((char *) rtsp->audio_encode_name, source);
#endif
    register_live_fn(source, self_creat, self_destory, self_play);
    rtsp_end_block(source);
    return;
}
