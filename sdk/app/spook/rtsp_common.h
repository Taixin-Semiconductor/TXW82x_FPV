#ifndef __RTSP_COMMON_H
#define __RTSP_COMMON_H
#include "spook_config.h"
#include "frame.h"
#include "encoder-audio.h"
#include "encoder-jpeg.h"
#include "rtp-audio.h"
#include "rtp.h"
#include "stream.h"
#include "rtp_media.h"
struct rtsp_session;
struct rtsp_track;
struct rtsp_source;


typedef void (*rtsp_creat)(struct rtsp_source *source,void *priv );
typedef void (*rtsp_release)(struct rtsp_source *source );
typedef void (*rtsp_play_fn)(struct rtsp_source *source );
typedef struct session *(*rtsp_path_register)( char *path, void *d );


struct rtsp_session {
	struct rtsp_session *next;
	struct rtsp_session *prev;
	struct session *sess;
	struct rtsp_source *source;
	char path[64];
	int playing;
	int closed;
};

struct rtsp_track {
	int index;
	struct rtsp_source *source;
	struct stream_destination *stream;
	int ready;
	struct rtp_media *rtp;
};


struct rtsp_source {
	struct rtsp_session *sess_list;
	struct rtsp_track track[MAX_TRACKS];
	struct os_task handle;
	void *signal;
	struct rtp_node live_node;
	char *cache_buf;
	int cache_buf_len;
	int head_len; //保留头部的长度
	//创建线程以及资源的回调函数,参数就是source
	rtsp_creat creat;
	//释放线程以及资源的回调函数,参数就是source
	rtsp_release release;
	rtsp_play_fn play;
	void *priv;
};


void *rtsp_start_block(void);
int rtsp_end_block( void *d );
int set_video_track( const char *name, void *d );
int set_video_h264_track( const char *name, void *d );
int set_audio_track( char *name, void *d );
struct session *rtsp_open( char *path, void *d );
void register_live_fn(struct rtsp_source *source,rtsp_creat creat,rtsp_release release,rtsp_play_fn play);
int rtsp_set_path( const char *path, void *d,rtsp_path_register register_fn );

#endif