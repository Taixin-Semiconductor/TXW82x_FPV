#ifndef __RTP_MEDIA_H
#define __RTP_MEDIA_H
#include "frame.h"
#include "stream.h"

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

typedef int (*rtp_media_get_sdp_func)( char *dest, int len, int payload,
						int port, void *d );
typedef int (*rtp_media_get_payload_func)( int payload, void *d );
typedef int (*rtp_media_frame_func)( struct frame *f, void *d );
typedef int (*rtp_media_send_func)( struct rtp_endpoint *ep, void *d );
typedef void* (*rtp_loop_search_ep)( void *head,void *track,void **ep );
typedef int (*rtp_media_send_more_func)( rtp_loop_search_ep search,void *ls, void *track, void *d,void *cache_buf,int len);	//track是live那边的结构体



struct rtp_media {
	rtp_media_get_sdp_func get_sdp;
	rtp_media_get_payload_func get_payload;
	rtp_media_frame_func frame;
	rtp_media_send_func send;
	rtp_media_send_more_func send_more;
	rtp_media_send_func rtcp_send;
	void *private;
	int per_ms_incr;
	uint32_t sample_rate;
	uint8_t type;
};


struct rtp_media *new_rtp_media( rtp_media_get_sdp_func get_sdp,
	rtp_media_get_payload_func get_payload, rtp_media_frame_func frame,
	rtp_media_send_func send, void *private );
struct rtp_media *new_rtp_media_jpeg_stream( struct stream *stream );
struct rtp_media *new_rtp_media_h264_stream( struct stream *stream );
#endif

