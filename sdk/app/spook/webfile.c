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
#include "avidemux.h"
#define WEBFILE_BASE_PATH	"DCIM"

extern void spook_send_thread_stream(struct rtsp_priv *r);
extern uint32_t creat_stream(struct rtsp_priv *r,const char *video_name,const char *audio_name);
extern uint32_t destory_stream(struct rtsp_priv *r);
extern uint32_t stop_play_avi_stream(stream* s);


void rtsp_webfile_thread(void *d)
{
	struct rtsp_priv *r = (struct rtsp_priv*)d;
	os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	spook_send_thread_stream(r);
}


stream* creat_play_avi_stream(const char *name,char *filepath,char *recv_name);
//创建实时预览的的线程
void webfile_rtsp_creat(struct rtsp_source *source,void *priv)
{
	char *path = (char*)priv;
	//void *fp = NULL;
	//struct aviinfo *info = NULL;
	char filePath[64] = {0};
	if(path)
	{
		_os_printf("%s path:%s\n",__FUNCTION__,path);
		char *base = path;
		char *c;
		int filePathLen = strlen(base);
		if(filePathLen > 1 && path[filePathLen - 1] == '/')
		{
		  --filePathLen;
		}

		c = strchr (base+1, '/');
		filePathLen -= c - base;

		filePath[0] = '0';
		filePath[1] = ':';
		memcpy(filePath+2,WEBFILE_BASE_PATH,strlen(WEBFILE_BASE_PATH));
		strncpy (filePath+2+strlen(WEBFILE_BASE_PATH), c, filePathLen);
		filePath[2+filePathLen+strlen(WEBFILE_BASE_PATH)] = '\0';

	}
	else
	{
		goto webfile_rtsp_creat_end;
	}


	//先去打开一下文件?然后判断一下?
	uint32_t res;
	source->priv = (struct rtsp_priv*)os_malloc(sizeof(struct rtsp_priv));
	struct rtsp_priv *r = (struct rtsp_priv*)source->priv;
	if(source->priv)
	{	
		r->live_node = &source->live_node;
		res = creat_stream(r,R_RTP_JPEG,NULL);
		if(!res)
		{
			csi_kernel_task_new((k_task_entry_t)rtsp_webfile_thread, "rtsp", r, 9, 0, NULL, 1024,&source->thread_handle);


			//创建播放文件的流
			r->webfile_s = creat_play_avi_stream(S_WEBJPEG,filePath,R_RTP_JPEG);
			void *webfile_handle;
			extern void play_web_avi_thread(void *d);
			if(r->webfile_s)
			{
				_os_printf("webfile_s1:%X\n",(unsigned int)r->webfile_s);
				//如果创建任务失败,需要close一次
				open_stream_again(r->webfile_s);
				//打开对应的文件
				csi_kernel_task_new((k_task_entry_t)play_web_avi_thread, "play_avi", r->webfile_s, 9, 0, NULL, 1024,&webfile_handle);
			}
		}
	}
	else
	{
		_os_printf("%s not enough space\n",__FUNCTION__);
	}
	webfile_rtsp_creat_end:
	return;
}


//创建实时预览的的线程
void webfile_rtsp_destory(struct rtsp_source *source)
{
	void *tmp = source->thread_handle;
	struct rtsp_priv *r = source->priv;
	if(r)
	{
		if(r->webfile_s)
		{
			stop_avi_stream(r->webfile_s);
		}
		destory_stream(r);
		os_free(source->priv);
		source->priv = NULL;
	}
	source->thread_handle = NULL;
	if(tmp)
	{
		csi_kernel_task_del(tmp);
	}
	

}





static int webfile_get_sdp( struct session *s, char *dest, int *len,
				char *path )
{
	struct rtsp_session *ls = (struct rtsp_session *)s->private;
	int i = 0, t;
	char *addr = "IP4 0.0.0.0";
	_os_printf("%s:%d\tpath:%s\n",__FUNCTION__,__LINE__,path);
	struct avi_msg msg;
	char filePath[64] = {0};
	uint32_t msk;
	if(path)
	{
		_os_printf("%s path:%s\n",__FUNCTION__,path);
		char *base = path;
		char *c;
		int filePathLen = strlen(base);
		if(filePathLen > 1 && path[filePathLen - 1] == '/')
		{
		  --filePathLen;
		}

		c = strchr (base+1, '/');
		filePathLen -= c - base;

		filePath[0] = '0';
		filePath[1] = ':';
		memcpy(filePath+2,WEBFILE_BASE_PATH,strlen(WEBFILE_BASE_PATH));
		strncpy (filePath+2+strlen(WEBFILE_BASE_PATH), c, filePathLen);
		filePath[2+filePathLen+strlen(WEBFILE_BASE_PATH)] = '\0';
	}

	_os_printf("filePath:%s\n",filePath);
	msk = get_avidemux_parse_stream(filePath,&msg);


	if( s->ep[0] && s->ep[0]->trans_type == RTP_TRANS_UDP )
		addr = s->ep[0]->trans.udp.sdp_addr;

	i = snprintf( dest, *len,"v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=Test\r\na=type:broadcast\r\nt=0 0\r\nc=IN %s\r\n", addr );


	for( t = 0; t < MAX_TRACKS && ls->source->track[t].rtp; ++t )
	{
		int port;

		//if( ! ls->source->track[t].ready ) return 0;

		if( s->ep[t] && s->ep[t]->trans_type == RTP_TRANS_UDP )
			port = s->ep[t]->trans.udp.sdp_port;
		else
			port = 0;


		os_printf("sdp type:%d\n",ls->source->track[t].rtp->type);
		if(ls->source->track[t].rtp->type == 0)
		{
			i += ls->source->track[t].rtp->get_sdp( dest + i, *len - i,96 + t, port,ls->source->track[t].rtp->private );
		}
		else
		{
			if(!(msk&BIT(1)))
			{
				continue;
			}
			ls->source->track[t].rtp->sample_rate = msg.audio_sample_rate;
			i += ls->source->track[t].rtp->get_sdp( dest + i, *len - i,96 + t, port,(void*)msg.audio_sample_rate);
		}
		if( port == 0 ) // XXX What's a better way to do this?
			i += sprintf( dest + i, "a=control:track%d\r\n", t );
	}
	
	*len = i;
	return t;
}

static struct session *webfile_rtsp_open( char *path, void *d )
{
	//默认的,各自模式可以各自去修改
	struct session *sess = rtsp_open(path,d);
	if(sess)
	{
		sess->get_sdp = webfile_get_sdp;
	}
	return sess;
}

static int webfile_set_path( const char *path, void *d )
{
  	spook_log (SL_DEBUG, "live set_path %s", path);

	/*if( num_tokens == 2 )*/
	{
		new_rtsp_location( path, NULL, NULL, NULL,webfile_rtsp_open, d );
		return 0;
	}
	spook_log( SL_ERR, "rtsp-handler: syntax: Path <path> [<realm> <username> <password>]" );
	return -1;
}

void webfile_rtsp_init(const rtp_name *rtsp)
{
	struct rtsp_source *source;
	source = rtsp_start_block();
	webfile_set_path( rtsp->path, source );
	set_video_track( (char*)rtsp->jpg_name, source );
	//set_audio_track( (char*)rtsp->audio_name, source );
	register_live_fn(source,webfile_rtsp_creat,webfile_rtsp_destory);
	rtsp_end_block(source);
	return ;
}