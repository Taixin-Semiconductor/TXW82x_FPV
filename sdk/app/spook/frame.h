/*
 * Copyright (C) 2004 Nathan Lutchansky <lutchann@litech.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
/*
#define FORMAT_EMPTY		0
#define FORMAT_RAW_RGB24	1
#define FORMAT_RAW_UYVY		2
#define FORMAT_RAW_BGR24	3
#define FORMAT_PCM		64
#define FORMAT_MPEG4		100
#define FORMAT_JPEG		101
#define FORMAT_MPV		102
#define FORMAT_H263		103
#define FORMAT_MPA		200
#define FORMAT_ALAW		201
*/
#ifndef __FRAME_H
#define __FRAME_H
#include "spook_config.h"
#include "jpgdef.h"
#include "sys_config.h"
#include "stream_frame.h"
#include "basic_include.h"
#include "lib/multimedia/msi.h"




#define FORMAT_JPEG		101
#define FORMAT_H264		96
#define FORMAT_AUDIO	97


enum
{
	RECORD_FLAG,
	SAVE_PIC_FLAG,
	WIFI_FPV_FLAG,

};

struct frame;

typedef int (*frame_destructor)( struct frame *f, void *d );
typedef void (*free_func)(void*node);

struct frame
{
	int ref_count;
	int size;
	int format;
	int width;
	int height;
	int length;
	int node_len;
	int first_length;
	unsigned char *d;//链表第一帧数据
	void * get_f;
	struct jpeg_fifo *j;
	uint32_t timestamp;
	free_func free;
};


void init_frame_heap( int size, int count );
int get_max_frame_size(void);
struct frame *new_frame(void);
void ref_frame( struct frame *f );
void unref_frame( struct frame *f );


struct frame_slot {
	struct frame_slot *next;
	struct frame_slot *prev;
	int pending;
	struct frame *f;
};

typedef void (*frame_deliver_func)( struct frame *f, void *d );
typedef void (*frame_disconnect_func)( struct frame *f, void *d );


struct frame_exchanger {
	int ready;
	int scan_ready;

	frame_deliver_func f;
	struct frame *jf;
	void *d;
};



struct rtp_node
{
	void *video_ex;  //frame_exchanger
	void *audio_ex;
	void *priv;

};


struct rtsp_priv
{
	struct rtp_node *live_node;
	//stream *video_s;
	//stream *audio_s;
	stream *webfile_s;
	struct msi *video_msi;
	struct msi *v_msi;
	struct msi *a_msi;
	int32 audio_coder_ret;
};





struct frame_exchanger *new_exchanger( int slots,
					frame_deliver_func func, void *d );

/* master functions */
int exchange_frame( struct frame_exchanger *ex, struct frame *frame );

/* slave functions */
struct frame *get_next_frame( struct frame_exchanger *ex, int wait );
void deliver_frame( struct frame_exchanger *ex, struct frame *f );



void *spook_register_video(const char *name,const char *action_name,int node_num);
void *spook_register_audio(const char *name,const char *action_name,int node_num,void *afmemt);
void spook_send_thread2(void *d);
int32_t spook_register_del_msg(void *signal);
struct frame_exchanger *new_exchanger_audio( int slots,frame_deliver_func func, void *d );
struct frame *new_audio_frame(void);

struct msi *rtsp_msi_init(const char *name,uint8_t filter_type,uint8_t srcID);
struct msi *rtsp_audio_msi_init(const char *name);
void rtsp_msi_deinit(struct msi *msi);
void rtsp_audio_msi_deinit(struct msi *msi);
#endif



