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
#include "stream_define.h"
#include "log.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "frame.h"
#include "audio_msi/audio_adc.h"
#include "jpg_concat_msi.h"
#include "gen420_hardware_msi.h"
#include "demo/app_common.h"

extern void spook_send_thread_stream(struct rtsp_priv *r);
static void self_thread(void *d)
{
	struct rtsp_priv *r = (struct rtsp_priv*)d;
	spook_send_thread_stream(r);
}

static int32 get_dev_cb(const struct dev_obj *dev, void *arg)
{
    struct dev_obj **dev_param = (struct dev_obj **)arg;
    if(dev && dev_param)
    {
        *dev_param = (struct dev_obj *)dev;
    }

    return 1;
}

//创建实时预览的的线程
static void self_creat(struct rtsp_source *source,void *priv)
{
	source->priv = (struct rtsp_priv*)os_zalloc(sizeof(struct rtsp_priv));
	struct rtsp_priv *r = (struct rtsp_priv*)source->priv;
	if(source->priv)
	{	
		r->live_node = &source->live_node;
		r->video_msi = msi_find(AUTO_JPG, 1);
		if(r->video_msi)
		{
			r->v_msi = rtsp_msi_init(R_RTP_JPEG,~0,0);

			struct dev_obj *audio_dev = NULL;
            dev_walk(DEV_TYPE_MIC, get_dev_cb, (void *)&audio_dev);
            if(audio_dev)
			{
				struct dev_hotplug_info *hotplug_info = (struct dev_hotplug_info *) audio_dev->info;
                txAudioInfo_t *audio_info = (txAudioInfo_t *) hotplug_info->priv;
				txAudioInfo_t codec_info;
	            codec_info.sample_rate = audio_info->sample_rate;
	            codec_info.channels = audio_info->channels;
	            codec_info.frame_size = 1024;
				r->a_msi = rtsp_audio_msi_init(R_RTP_AUDIO2);
	            r->audio_msi = aenc_get_msi(AUDIO_CODEC_AAC, "adc", &codec_info);
				if(r->audio_msi) 
				{
	                auadc_msi_add_output(MAIN_MIC_ID, r->audio_msi->name);
	                msi_do_cmd(r->audio_msi, MSI_CMD_START, 0, 0);
					msi_add_output(r->audio_msi, NULL, NULL, R_RTP_AUDIO2);
				}
			}

			if(r->v_msi)
			{
				msi_add_output(r->video_msi, NULL, NULL, R_RTP_JPEG);
				OS_TASK_INIT("live_rtsp_mjpeg", &source->handle, self_thread, r, OS_TASK_PRIORITY_NORMAL + 2, NULL, 1024);
			}
		}
		//这里没有增加容错
		else
		{
			os_printf("%s jpg_concat_msi_init_start fail\n",__FUNCTION__);
			return;
		}
	}
	else
	{
		os_printf("%s not enough space\n",__FUNCTION__);
	}

	return;
}

static void self_destory(struct rtsp_source *source)
{
	void 			 *tmp = source->handle.hdl;
	struct rtsp_priv *r   = source->priv;
	if(r)
	{
		if(r->video_msi)
		{
			msi_del_output(r->video_msi,NULL,NULL, R_RTP_JPEG);
			//msi_destroy(r->video_msi);
			msi_put(r->video_msi);
			r->video_msi = NULL;
		}
		rtsp_msi_deinit((void*)r->v_msi);

		if(r->audio_msi) 
		{
            msi_del_output(r->audio_msi, NULL, NULL, R_RTP_AUDIO2);
            msi_put(r->audio_msi);
		}

		rtsp_audio_msi_deinit((void*)r->a_msi);
		os_free(source->priv);
		source->priv = NULL;
	}
	if(tmp)
	{
		os_task_del(&source->handle);
	}
}

static int self_get_sdp( struct session *s, char *dest, int *len, char *path )
{
	struct rtsp_session *ls = (struct rtsp_session *)s->private;
	int i = 0;
	int t = 0;
	char *addr = "IP4 0.0.0.0";
	os_printf("%s:%d\tpath:%s\n", __FUNCTION__, __LINE__, path);

	if( s->ep[0] && s->ep[0]->trans_type == RTP_TRANS_UDP )
		addr = s->ep[0]->trans.udp.sdp_addr;

	i = snprintf( dest, *len,"v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=Test\r\na=type:broadcast\r\nt=0 0\r\nc=IN %s\r\n", addr );
	for( t = 0; t < MAX_TRACKS && ls->source->track[t].rtp; ++t )
	{
		int port;

		if( s->ep[t] && s->ep[t]->trans_type == RTP_TRANS_UDP )
			port = s->ep[t]->trans.udp.sdp_port;
		else
			port = 0;

		if(ls->source->track[t].rtp->type == 0)
		{
			i += ls->source->track[t].rtp->get_sdp( dest + i, *len - i,96 + t, port,ls->source->track[t].rtp->private );
		}
		else
		{
			i += ls->source->track[t].rtp->get_sdp( dest + i, *len - i,96 + t, port,NULL);
		}
		
		if( port == 0 ) // XXX What's a better way to do this?
			i += sprintf( dest + i, "a=control:track%d\r\n", t );
	}
	*len = i;
	return t;
}

static struct session *self_rtsp_open( char *path, void *d )
{
	//默认的,各自模式可以各自去修改
	struct session *sess = rtsp_open(path,d);
	if(sess)
	{
		sess->get_sdp = self_get_sdp;
	}
	return sess;
}

extern void *jpeg_encode_init(const char *encode_name);
extern void jpeg_encode_deinit(void *d);
extern void *rtsp_audio_encode_init(const char *encode_name);
extern void rtsp_audio_encode_deinit(void *d);

static void self_track_deinit(struct rtsp_source *source)
{
	_os_printf("%s %d\r\n", __FUNCTION__, __LINE__);
	if(source)
	{
		void *video_en = get_frame_encode(source->live_node.video_ex);
		void *audio_en = get_frame_encode(source->live_node.audio_ex);
		if(video_en)
		{
			jpeg_encode_deinit(video_en);
			video_en = NULL;
		}
		if(audio_en)
		{
			rtsp_audio_encode_deinit(audio_en);
			audio_en = NULL;
		}
		del_track(source);
	}
}

static int self_set_track(void *d)
{
	_os_printf("%s %d\r\n", __FUNCTION__, __LINE__);
	struct rtsp_source *source = (struct rtsp_source *)d;
	if(rtsp_end_block(source) != 0)
	{
		if(!find_stream((char*)source->rtp_name->video_encode_name))
		{
			jpeg_encode_init((char*)source->rtp_name->video_encode_name);
		}
		set_video_track((char*)source->rtp_name->video_encode_name, source);

        struct dev_obj *audio_dev = NULL;
        dev_walk(DEV_TYPE_MIC, get_dev_cb, (void *)&audio_dev);
        if(audio_dev)
        {
			if(!find_stream((char*)source->rtp_name->audio_encode_name))
			{
				rtsp_audio_encode_init((char*)source->rtp_name->audio_encode_name);
			}
			set_audio_track((char*)source->rtp_name->audio_encode_name, source);
		}

		register_live_fn(source, self_creat, self_destory, NULL, self_track_deinit);
		return rtsp_end_block(source);
	}
	return 0;
}

void rtsp_mjpeg_set_path(const rtp_name *rtsp)
{
	struct rtsp_source *source;
	source = rtsp_start_block();
	source->rtp_name = rtsp;
	rtsp_set_path(rtsp->path, source, self_rtsp_open, self_set_track);
}

void rtsp_mjpeg_live_init(const rtp_name *rtsp)
{
	struct rtsp_source *source;
	source = rtsp_start_block();
	rtsp_set_path(rtsp->path, source, self_rtsp_open, NULL);
	set_video_track((char*)rtsp->video_encode_name, source);
    struct dev_obj *audio_dev = NULL;
    dev_walk(DEV_TYPE_MIC, get_dev_cb, (void *)&audio_dev);
    if(audio_dev)
    {
		set_audio_track((char*)rtsp->audio_encode_name, source);
    }
	register_live_fn(source, self_creat, self_destory, NULL, NULL);
	rtsp_end_block(source);
	return;
}
