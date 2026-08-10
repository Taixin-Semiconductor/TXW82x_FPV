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


#include <stdio.h>
#include "lwip\sockets.h"
#include "lwip\netif.h"
#include "lwip\dns.h"

#include "lwip\api.h"
#include "lwip\tcp.h"

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>
#include <rtp.h>
#include <rtp_media.h>
#include <spook_config.h>
#include "list.h"
#include "jpgdef.h"
#include "rtp.h"
#include "session.h"
#include "osal/sleep.h"
#include "lib/audio/audio_code/audio_code.h"

#define AUDIO_SAMPLE	8000

struct rtp_audio {
	//为了获取到frame,因为使用了链表形式
	struct frame *f;
	unsigned int timestamp;	
};

static int audio_get_sdp( char *dest, int len, int payload, int port, void *d )
{
	uint32_t audio_sample_rate = (uint32)d;
	_os_printf("audio_sample_rate:%d\n",audio_sample_rate);
	if(audio_sample_rate == 0)
	{
		audio_sample_rate = AUDIO_SAMPLE;//默认采样率
	}
	
	return snprintf( dest, len, "m=audio %d RTP/AVP 97\r\na=rtpmap:97 mpeg4-generic/%d/1\r\na=fmtp:97 profile-level-id=1; mode=AAC-hbr; config=1588; sizeLength=13; indexlength=3; indexdeltalength=3\r\n", port,audio_sample_rate);
	// return snprintf( dest, len, "m=audio %d RTP/AVP 97\r\na=rtpmap:97 PCMA/%d/1\r\n", port,audio_sample_rate);
	// return snprintf( dest, len, "m=audio %d RTP/AVP 97\r\na=rtpmap:97 L16/%d/1\r\n", port,audio_sample_rate);
}

static int audio_process_frame( struct frame *f, void *d )
{
	_os_printf("+");
	struct rtp_media *rtp = (struct rtp_media *)d;
	struct rtp_audio *out = (struct rtp_audio *)rtp->private;
	out->f = f;
	out->timestamp = f->timestamp*(rtp->sample_rate/(1000));
	return 1;
}

static int audio_get_payload( int payload, void *d )
{
	_os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return 97;
}

static int audio_send( struct rtp_endpoint *ep, void *d )
{
	return 0;
}

static void audio_build_au_header( unsigned char *au_hdr, int au_size )
{
	au_hdr[0] = 0x00;
	au_hdr[1] = 0x10;
	au_hdr[2] = ( au_size & 0x1FE0 ) >> 5;
	au_hdr[3] = ( au_size & 0x1F ) << 3;
}

static int audio_send_au_to_endpoint( struct rtp_endpoint *ep, uint32_t timestamp, uint8_t *au_data, uint32_t au_size, int times )
{
	struct iovec v[3];
	uint8_t au_hdr[4];
	int max_payload;
	uint32_t max_fragment_size;
	uint32_t offset = 0;
	uint32_t fragment_len;

	if( !ep || !ep->sendEnable || !au_data || au_size <= 0 ) return -1;
	if( au_size > 0x1FFF )
	{
		spook_log( SL_ERR, "AAC AU too large for 13-bit AU-size field: %d", au_size );
		ep->sendEnable = 0;
		return -1;
	}

	max_payload = rtp_get_payload_size_limit( ep, RTP_HEADER_SIZE );
	if( max_payload <= (int)sizeof( au_hdr ) )
	{
		ep->sendEnable = 0;
		return -1;
	}

	max_fragment_size = max_payload - sizeof( au_hdr );
	audio_build_au_header( au_hdr, au_size );
	v[1].iov_base = au_hdr;
	v[1].iov_len = sizeof( au_hdr );

	while( offset < au_size )
	{
		fragment_len = au_size - offset;
		if( fragment_len > max_fragment_size )
			fragment_len = max_fragment_size;

		v[2].iov_base = (void *)( au_data + offset );
		v[2].iov_len = fragment_len;
		if( rtp_sendmsg( ep, v, 3, timestamp, offset + fragment_len >= au_size, times ) < 0 )
		{
			ep->sendEnable = 0;
			return -1;
		}

		offset += fragment_len;
	}

	return 0;
}

static int audio_send_more( rtp_loop_search_ep search, void *ls, void *track, void *d )
{
	struct rtp_endpoint *ep;
	void *head;
	uint32_t au_size;
	struct rtp_audio *out = (struct rtp_audio *)d;
	struct framebuff *fb = (struct framebuff *)out->f->get_f;

	if( !fb || fb->len <= 7 )
	{
		return 0;
	}

	au_size = fb->len - 7;

	head = ls;
	while(head)
	{
		head = search(head,track,(void*)&ep);
		if(!ep)
		{
			continue;
		}
		if(ep->sendEnable)
		{
			if( audio_send_au_to_endpoint( ep, out->timestamp, fb->data + 7, au_size, 10 ) < 0 )
			{
				ep->sendEnable = 0;
			}
		}
		ep->sendEnable = 1;
	}
	return 0;
}

struct rtp_media *new_rtp_media_audio_stream( struct stream *stream )
{
	struct rtp_audio *out;
	int fincr, fbase;
	struct rtp_media *m;

	stream->get_framerate( stream, &fincr, &fbase );
	out = (struct rtp_audio *)malloc( sizeof( struct rtp_audio ) );
	out->f = NULL;
	out->timestamp = 0;
	m = new_rtp_rtcp_media( audio_get_sdp, audio_get_payload, audio_process_frame, audio_send, new_rtcp_send, out );
	if(m)
	{
		m->sample_rate = AUDIO_SAMPLE;//默认
		m->type = 1;	//音频
		m->send_more = audio_send_more;
		m->per_ms_incr = AUDIO_SAMPLE/(1000);
	}

	return m;
}

void del_rtp_media_audio_stream( struct rtp_media *m )
{
	struct rtp_audio *out = (struct rtp_audio *)m->private;
	if(out)
	{
		free(out);
	}

	if(m)
	{
		free(m);
	}
}
