#include <stdio.h>
#include "lwip\sockets.h"
#include "lwip\netif.h"
#include "lwip\dns.h"
#include "lwip\api.h"
#include "lwip\tcp.h"
#include "log.h"
#include "sys_config.h"
#include "osal/string.h"
#include "rtsp_common.h"
#include "event.h"


#ifdef send
#undef send
#endif


static int rtsp_get_sdp( struct session *s, char *dest, int *len,
				char *path )
{
	struct rtsp_session *ls = (struct rtsp_session *)s->private;
	int i = 0, t;
	char *addr = "IP4 0.0.0.0";
	_os_printf("%s:%d\tpath:%s\n",__FUNCTION__,__LINE__,path);

	if( s->ep[0] && s->ep[0]->trans_type == RTP_TRANS_UDP )
		addr = s->ep[0]->trans.udp.sdp_addr;

	i = snprintf( dest, *len,
		"v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=Test\r\na=type:broadcast\r\nt=0 0\r\nc=IN %s\r\n", addr );

		_os_printf("%s:%d\n",__FUNCTION__,__LINE__);

	for( t = 0; t < MAX_TRACKS && ls->source->track[t].rtp; ++t )
	{
		int port;

		//if( ! ls->source->track[t].ready ) return 0;

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



static int rtsp_setup( struct session *s, int t )
{
	struct rtsp_session *ls = (struct rtsp_session *)s->private;
	int payload = 96 + t;

	//判断source是否已经创建了线程,如果没有创建线程,则在这里创建线程
	//线程创建成功后,实际内部不会进行太多操作,主要是读取视频和音频数据
	//直到play执行后,对应位被置位后,才会进行发送
	//如果teardown后,如果没有ls链表,则将线程删除,释放空间
	//删除线程,就要同时考虑将对应视频和音频接入的框架流disable掉,所以是不是应该有一个释放资源的回调函数呢
	//所以source是不是应该有一个创建线程和释放线程的回调函数,用的是自身的句柄
	//线程传入的参数可以通过source中的track查找,不用管t的值,而是看初始化的时候创建多少个track决定,配合创建线程


	//线程创建
	if(!ls->source->handle.hdl)
	{
		if(ls->source->creat)
		{
			//为source申请cache_buf的空间
			ls->source->cache_buf = (char *)os_malloc( ls->source->cache_buf_len+ls->source->head_len );
			if( ! ls->source->cache_buf )
			{
				os_printf("rtsp source cache_buf malloc failed\n");
				return -1;
			}
			else
			{
				os_printf("malloc rtsp source cache_buf:%X\n",ls->source->cache_buf);
			}
			ls->source->creat(ls->source,ls->path);
		}
	}
	else
	{
		_os_printf("live thread already run:%X\t%X\n",(int)ls->source->handle.hdl,(int)ls->source);
	}

	if( ! ls->source->track[t].rtp ) return -1;

	if( ls->source->track[t].rtp->get_payload )
		payload = ls->source->track[t].rtp->get_payload( payload,
					ls->source->track[t].rtp->private );
	s->ep[t] = new_rtp_endpoint( payload );
	s->ep[t]->session = s;
	
	return 0;
}


static void rtsp_play( struct session *s, double *start )
{
	struct rtsp_session *ls = (struct rtsp_session *)s->private;
	int t;
	
	if( start ) *start = -1;
	ls->playing = 1;
	for( t = 0; t < MAX_TRACKS && ls->source->track[t].rtp; ++t )
	{
			//对应位置位,然后就线程会自动发送数据
			struct rtsp_track *track = &ls->source->track[t];
			track->ready = 1;
			//clear_init_done(track->rtp->private);
			if( s->ep[t] ) set_waiting( ls->source->track[t].stream, 1 );
	}
	if(ls->source->play)
	{
		ls->source->play(ls->source);
	}
}

static void track_check_running( struct rtsp_source *source, int t )
{
	struct rtsp_session *ls;

	for( ls = source->sess_list; ls; ls = ls->next )
		if( ls->playing && ls->sess->ep[t] ) return;

	set_waiting( source->track[t].stream, 0 );
}
extern void drop_conn( struct conn *c );
void rtsp_release_event(void *ei, void *d)
{
	_os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	struct session *s = (struct session*)d;
	struct rtsp_session *ls = (struct rtsp_session *)s->private;
	struct conn *c  = s->conn;
	if(c)
	{
		drop_conn( c );
	}
	os_free( ls );
	del_session( s );
} 


static void rtsp_teardown( struct session *s, struct rtp_endpoint *ep )
{
	_os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	struct rtsp_session *ls = (struct rtsp_session *)s->private;
	//struct rtsp_source *source = ls->source;
	int i, remaining = 0;

	for( i = 0; i < MAX_TRACKS && ls->source->track[i].rtp; ++i )
	{
		if( ! s->ep[i] ) continue;
		if( ! ep || s->ep[i] == ep )
		{
			del_rtp_endpoint( s->ep[i] );
			s->ep[i] = NULL;
			track_check_running( ls->source, i );
		} else ++remaining;
	}

	if( remaining == 0 )
	{
		//解锁ls链表
		if( ls->next ) ls->next->prev = ls->prev;
		if( ls->prev ) ls->prev->next = ls->next;
		else ls->source->sess_list = ls->next;
		//启动event将事件移除
		//断开链表后,开启eloop的alarm事件,清除对应的资源
		eloop_add_alarm(os_jiffies(),EVENT_F_ENABLED,rtsp_release_event,(void*)s);
	}

}

static void rtsp_closed( struct session *s, struct rtp_endpoint *ep )
{
	_os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	struct rtsp_session *ls = (struct rtsp_session *)s->private;
	ls->closed = 1;
}

static void rtsp_select_close( struct session *s, struct rtp_endpoint *ep)
{
	struct rtsp_session *ls = (struct rtsp_session *)s->private;
	if(ls->source->handle.hdl)
	{
		_os_printf("%s: %d, set closed\r\n", __FUNCTION__, __LINE__);
		s->closed(s, NULL);
	}
	else
	{
		_os_printf("%s: %d, set teardown\r\n", __FUNCTION__, __LINE__);
		s->teardown(s, NULL);
	}
}

struct session *rtsp_open( char *path, void *d )
{
	struct rtsp_source *source = (struct rtsp_source *)d;
	struct rtsp_session *ls;
	ls = (struct rtsp_session *)os_malloc( sizeof( struct rtsp_session ) );
	ls->next = source->sess_list;
	if( ls->next ) ls->next->prev = ls;
	source->sess_list = ls;
	ls->prev = NULL;
	ls->sess = new_session();
	ls->source = source;
	ls->playing = 0;
	ls->sess->get_sdp = rtsp_get_sdp;
	ls->sess->setup = rtsp_setup;
	ls->sess->play = rtsp_play;
	ls->sess->teardown = rtsp_teardown;
	ls->sess->closed = rtsp_closed;
	ls->sess->select_close = rtsp_select_close;
	ls->sess->private = ls;
	ls->closed = 0;
	if(strlen(path)<sizeof(ls->path))
	{
		memcpy(ls->path,path,strlen(path)+1);
	}
	else
	{
		_os_printf("%s path too long:%d\n",__FUNCTION__,strlen(path));
	}
	return ls->sess;
}

static void *loop_search_ep(void *in_head,void *in_track,void **ep)
{
	struct rtsp_session *ls;
	struct rtsp_session *head = (struct rtsp_session *)in_head;
	struct rtsp_track *track = (struct rtsp_track *)in_track;
	for( ls = head; ls; ls = ls->next )
	{
		
		if( ls->playing && track->ready && ls->sess->ep[track->index] )
		{
			*ep = ls->sess->ep[track->index];
			return ls->next;
		}
	}
	*ep = NULL;
	return NULL;
}


static void rtsp_common_send( struct frame *f, void *d )
{
	struct rtsp_track *track = (struct rtsp_track *)d;/*d: source->track[t]*/
	struct rtsp_session *ls;
	if(!f)
	{
		return;
	}



	if(!track->ready)
	{
		return;
	}
	//dest_status_msg(track,track->ready);
	/*struct rtsp_session *next;*/
	//这里添加获取jpeg的节点头，发送完一帧后，再循环到这里，看是否有下一帧数据，如果有，那就继续发送，省去多return一次的时间
	/*callback: jpeg_process_frame, track->rtp->private: rtp_jpeg *out*/

	if( ! track->rtp->frame( f, track->rtp ) )						//这个按理说不用一直运行的，只运行一次则可以
	{
		return;
	}

	if(track->rtp->send_more)
	{
		ls = track->source->sess_list;
		if(!ls->closed){
			track->rtp->send_more( loop_search_ep,ls, track,track->rtp->private,track->source->cache_buf+track->source->head_len,track->source->cache_buf_len );
		}
		
	}

	for( ls = track->source->sess_list; ls; ls = ls->next )
	{
		if( ls->playing && track->ready && ls->sess->ep[track->index] )
		{
			/*callback: jpeg_send;ls->sess->ep[track->index], track->rtp->private: out*/
			//track->rtp->send( ls->sess->ep[track->index], track->rtp->private );
			if(track->rtp && track->rtp->rtcp_send)
			{
				track->rtp->rtcp_send( ls->sess->ep[track->index], track->rtp );
			}
		}
	}
}


static void rtsp_frame_end( struct frame *f, void *d ){
	struct rtsp_track *track = (struct rtsp_track *)d;/*d: source->track[t]*/
	struct rtsp_session *ls;
	struct rtsp_source *source;

	for( ls = track->source->sess_list; ls; ls = ls->next )
	{
		source = ls->source;
		if( !f || ls->closed)
		{
			/*callback: jpeg_send;ls->sess->ep[track->index], track->rtp->private: out*/
			//track->rtp->send( ls->sess->ep[track->index], track->rtp->private );
			ls->sess->teardown( ls->sess, ls->sess->ep[track->index] );

			//如果有teardown后,则需要重新去轮询,因为之前的ls被释放了,虽然效率低,但考虑到设备数不多,这样应该还好
			ls = track->source->sess_list;
			if(!ls)
			{
				if(f)
				{
					//FREE_JPG_NODE(f->get_f);
					unref_frame( f );
				}
				//因为所有的东西被释放,所以就要将线程和资源释放
				if(source->release)
				{
					if(source->cache_buf)
					{
						os_printf("free rtsp source cache_buf:%X\n",source->cache_buf);
						os_free(source->cache_buf);
						source->cache_buf = NULL;
						
					}
					source->release(source);
				}
				break;
			}
		}
	}
}


void *rtsp_start_block(void)
{
	struct rtsp_source *source;
	int i;

   	spook_log (SL_DEBUG, "live start_block");

	source = (struct rtsp_source *)os_malloc( sizeof( struct rtsp_source ) );
	//初始化source
	memset(source,0,sizeof( struct rtsp_source ));
	source->cache_buf_len = SPOOK_CACHE_BUF_LEN;
	source->head_len = SPOOK_CACHE_BUF_HEAD_LEN;
	//配置默认值
	for( i = 0; i < MAX_TRACKS; ++i )
	{
		source->track[i].index = i;
		source->track[i].source = source;
		source->track[i].stream = NULL;
		source->track[i].ready = 0;
		source->track[i].rtp = NULL;
		
		
	}

	return source;
}


int rtsp_end_block( void *d )
{
	struct rtsp_source *source = (struct rtsp_source *)d;

    spook_log (SL_DEBUG, "live end_block");

	if( ! source->track[0].rtp )
	{
		spook_log( SL_ERR, "live: no media sources specified!" );
		return -1;
	}

	return 0;
}

#include "osal/string.h"
//视频注册
int set_video_track( const char *name, void *d )
{
	struct rtsp_source *source = (struct rtsp_source *)d;
	int t;
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    spook_log (SL_DEBUG, "live set_track %s", name);

	for( t = 0; t < MAX_TRACKS && source->track[t].rtp; ++t );
    spook_log (SL_DEBUG, "live set_track %s", name);
	if( t == MAX_TRACKS )
	{
		spook_log( SL_ERR, "live: exceeded maximum number of tracks" );
		return -1;
	}
    spook_log (SL_DEBUG, "live set_track %s", name);
	if( ! ( source->track[t].stream = connect_to_stream( name,rtsp_common_send, &source->track[t] ) ) )
	{
		spook_log( SL_ERR,"live: unable to connect to stream \"%s\"", name );
		return -2;
	}

	spook_log (SL_DEBUG, "live set_track %s", name);
	source->live_node.video_ex = get_video_ex(source->track[t].stream->stream->private);
	

	disconnect_stream(source->track[t].stream,rtsp_frame_end);

	source->track[t].rtp = new_rtp_media_jpeg_stream(source->track[t].stream->stream );
    os_printf("%s source->track[t]:%X\trtp:%X\n",__FUNCTION__,source->track[t],source->track[t].rtp);
	if( ! source->track[t].rtp ) return -3;

	set_waiting( source->track[t].stream, 0 );

	return 0;
}



//视频注册
int set_video_h264_track( const char *name, void *d )
{
	struct rtsp_source *source = (struct rtsp_source *)d;
	int t;
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    spook_log (SL_DEBUG, "live set_track %s", name);

	for( t = 0; t < MAX_TRACKS && source->track[t].rtp; ++t );
    spook_log (SL_DEBUG, "live set_track %s", name);
	if( t == MAX_TRACKS )
	{
		spook_log( SL_ERR, "live: exceeded maximum number of tracks" );
		return -1;
	}
    spook_log (SL_DEBUG, "live set_track %s", name);
	if( ! ( source->track[t].stream = connect_to_stream( name,rtsp_common_send, &source->track[t] ) ) )
	{
		spook_log( SL_ERR,"live: unable to connect to stream \"%s\"", name );
		return -2;
	}

	spook_log (SL_DEBUG, "live set_track %s", name);
	source->live_node.video_ex = get_video_ex(source->track[t].stream->stream->private);
	

	disconnect_stream(source->track[t].stream,rtsp_frame_end);

	source->track[t].rtp = new_rtp_media_h264_stream(source->track[t].stream->stream );
    os_printf("%s source->track[t]:%X\trtp:%X\n",__FUNCTION__,source->track[t],source->track[t].rtp);
	if( ! source->track[t].rtp ) return -3;

	set_waiting( source->track[t].stream, 0 );

	return 0;
}

//音频注册
int set_audio_track( char *name, void *d )
{
	struct rtsp_source *source = (struct rtsp_source *)d;
	int t;

    spook_log (SL_DEBUG, "live set_track %s", name);

	for( t = 0; t < MAX_TRACKS && source->track[t].rtp; ++t );

	if( t == MAX_TRACKS )
	{
		spook_log( SL_ERR, "live: exceeded maximum number of tracks" );
		return -1;
	}

	if( ! ( source->track[t].stream = connect_to_stream(name,rtsp_common_send, &source->track[t] ) ) )
	{
		spook_log( SL_ERR,"live: unable to connect to stream \"%s\"", name );
		return -1;
	}
			
	source->live_node.audio_ex = get_audio_ex(source->track[t].stream->stream->private);
	os_printf("live audio_ex:%X\n",source->live_node.audio_ex);
	disconnect_stream(source->track[t].stream,rtsp_frame_end);
	
	//要重新新建
	source->track[t].rtp = new_rtp_media_audio_stream(source->track[t].stream->stream );


	//默认打开
	source->track[t].ready = 0;

	if( ! source->track[t].rtp ) return -1;

	set_waiting( source->track[t].stream, 0 );

	return 0;
}


int rtsp_set_path( const char *path, void *d,rtsp_path_register register_fn )
{
  	spook_log (SL_DEBUG, "live set_path %s", path);

	/*if( num_tokens == 2 )*/
	{
		new_rtsp_location( path, NULL, NULL, NULL,
				register_fn, d );
		return 0;
	}
	spook_log( SL_ERR, "rtsp-handler: syntax: Path <path> [<realm> <username> <password>]" );
	return -1;
}

void register_live_fn(struct rtsp_source *source,rtsp_creat creat,rtsp_release release,rtsp_play_fn play_fn)
{
	source->creat = creat;
	source->release = release;
	source->play = play_fn;
}
