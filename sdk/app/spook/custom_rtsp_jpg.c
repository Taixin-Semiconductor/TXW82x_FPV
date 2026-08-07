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
#include "stream_frame.h"
#include "log.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "frame.h"
#include "audio_media_ctrl/audio_code_ctrl.h"
#include "audio_msi/audio_adc.h"
#include "jpg_concat_msi.h"
#include "gen420_hardware_msi.h"

extern void spook_send_thread_stream(struct rtsp_priv *r);
extern void err_rtsp_stream(struct rtp_node *node);
static void self_thread(void *d)
{
	struct rtsp_priv *r = (struct rtsp_priv*)d;
	spook_send_thread_stream(r);
}

static void err_exit_thread(void *d)
{
	struct rtp_node *node = (struct rtp_node *)d;
	err_rtsp_stream(node);
}
//创建实时预览的的线程
static void self_creat(struct rtsp_source *source,void *priv)
{
    const char *path = (const char *)priv;
    while((*path) && (*path != '?'))
    {
        path++;
    }
    os_printf("match path:%s\n",path);
	source->priv = (struct rtsp_priv*)os_zalloc(sizeof(struct rtsp_priv));
	struct rtsp_priv *r = (struct rtsp_priv*)source->priv;
	if(source->priv && *path == '?')
	{	
        path++;
		r->live_node = &source->live_node;
		r->video_msi = msi_find(path, 0);
        os_printf("video_msi :%s\tmsi:%X\n",path,r->video_msi);
		if(r->video_msi)
		{
			r->v_msi = rtsp_msi_init("custom_rtp",~0,0);
			if(r->v_msi)
			{
				msi_add_output(r->video_msi, NULL, "custom_rtp");
				OS_TASK_INIT("live_rtsp", &source->handle, self_thread	, r, OS_TASK_PRIORITY_NORMAL, NULL,1024);
			}
		}
		else
		{
			os_printf("%s jpg_concat_msi_init_start fail\n",__FUNCTION__);
            OS_TASK_INIT("live_rtsp_err", &source->handle, err_exit_thread	, &source->live_node, OS_TASK_PRIORITY_NORMAL, NULL,1024);
			return;
		}

	}
	else
	{
		os_printf("%s not enough space\n",__FUNCTION__);
        OS_TASK_INIT("live_rtsp_err", &source->handle, err_exit_thread	, &source->live_node, OS_TASK_PRIORITY_NORMAL, NULL,1024);
	}

	return;
}


static void self_destory(struct rtsp_source *source)
{
	//stop_jpeg();
	void *tmp = source->handle.hdl;
	struct rtsp_priv *r = source->priv;
	if(r)
	{
		if(r->video_msi)
		{
			msi_del_output(r->video_msi,NULL,"custom_rtp");
			msi_put(r->video_msi);
			r->video_msi = NULL;
		}
		rtsp_msi_deinit((void*)r->v_msi);
		os_free(source->priv);
		source->priv = NULL;
	}
	if(tmp)
	{
		os_task_del(&source->handle);
	}
	
}

static int self_get_sdp( struct session *s, char *dest, int *len,
				char *path )
{
	struct rtsp_session *ls = (struct rtsp_session *)s->private;
	int i = 0;
	int t = 0;
	char *addr = "IP4 0.0.0.0";
	os_printf("%s:%d\tpath:%s\n",__FUNCTION__,__LINE__,path);

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

void custom_rtsp_jpeg_init(const rtp_name *rtsp)
{
	struct rtsp_source *source;
	source = rtsp_start_block();
	rtsp_set_path( rtsp->path, source,self_rtsp_open );
	set_video_track( (char*)rtsp->video_encode_name, source );
	register_live_fn(source,self_creat,self_destory,NULL);
	rtsp_end_block(source);
	return ;
}
