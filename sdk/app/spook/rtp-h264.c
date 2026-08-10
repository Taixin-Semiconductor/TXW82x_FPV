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
#include "stream_define.h"

#define H264_SAMPLE	90000

struct rtp_h264 {
	//为了获取到frame,因为使用了链表形式
	unsigned char *d;//链表第一帧数据
	struct frame *f;
	unsigned int timestamp;	
	uint8_t first_flag;
	uint8_t last_count;
};

static int h264_get_sdp( char *dest, int len, int payload, int port, void *d )
{
	uint32_t h264_sample_rate = (uint32)d;
	_os_printf("h264_sample_rate:%d\n",h264_sample_rate);
	if(h264_sample_rate == 0)
	{
		h264_sample_rate = H264_SAMPLE;//默认采样率
	}
	
	return snprintf( dest, len, "m=video %d RTP/AVP 96\r\na=rtpmap:96 H264/%d\r\na=decode_buf=300\r\n", port,h264_sample_rate);
}

static int h264_process_frame( struct frame *f, void *d )
{
	struct rtp_media *rtp = (struct rtp_media *)d;
	struct rtp_h264 *out = (struct rtp_h264 *)rtp->private;
	_os_printf("+");
	out->f = f;
	out->d = f->d;
	out->timestamp = f->timestamp*(rtp->sample_rate/1000);
	return 1;
}

static int h264_get_payload( int payload, void *d )
{
	return FORMAT_H264;
}


static int h264_send( struct rtp_endpoint *ep, void *d )
{
	return 0;
}

static int h264_send_nalu( struct rtp_endpoint *ep, uint32_t timestamp, uint8_t *nalu, uint32_t nalu_len, uint8_t marker, int times, int max_payload )
{
	struct iovec v[3];
	uint8_t fu_hdr[2];
	uint32_t offset;
	uint32_t chunk;

	if( !ep || !ep->sendEnable || !nalu || nalu_len <= 0 )
	{
		return -1;
	}

	if( nalu_len <= max_payload )
	{
		// single NALU
		v[1].iov_base = (void *)nalu;
		v[1].iov_len = nalu_len;
		if( rtp_sendmsg( ep, v, 2, timestamp, marker, times ) < 0 )
		{
			ep->sendEnable = 0;
			return -1;
		}
		return 0;
	}

	if( nalu_len <= 1 || max_payload <= 2 )
	{
		ep->sendEnable = 0;
		return -1;
	}

	// FU-A
	fu_hdr[0] = ( nalu[0] & 0xE0 ) | 28;
	for( offset = 1; offset < nalu_len; offset += chunk )
	{
		chunk = nalu_len - offset;
		if( chunk > max_payload - 2 )
		{
			chunk = max_payload - 2;
		}
		fu_hdr[1] = nalu[0] & 0x1F;
		if( offset == 1 )
		{
			fu_hdr[1] |= 0x80;
		}
		
		if( offset + chunk >= nalu_len )
		{
			fu_hdr[1] |= 0x40;
		}

		v[1].iov_base = fu_hdr;
		v[1].iov_len = sizeof( fu_hdr );
		v[2].iov_base = (void *)( nalu + offset );
		v[2].iov_len = chunk;
		if( rtp_sendmsg( ep, v, 3, timestamp, offset + chunk >= nalu_len ? marker : 0, times ) < 0 )
		{
			ep->sendEnable = 0;
			return -1;
		}
	}

	return 0;
}

static void h264_send_nalu_to_endpoint( struct rtp_endpoint *ep, struct rtp_h264 *out, uint8_t *nalu, uint32_t nalu_len, uint8_t marker, int times )
{
	int max_payload;

	if( !ep || !ep->sendEnable )
	{
		return;
	}
	max_payload = rtp_get_payload_size_limit( ep, RTP_HEADER_SIZE );
	if( max_payload <= 0 )
	{
		ep->sendEnable = 0;
		return;
	}
	if( h264_send_nalu( ep, out->timestamp, nalu, nalu_len, marker, times, max_payload ) < 0 )
	{
		ep->sendEnable = 0;
	}
}

static void h264_send_sps_pps( struct rtp_endpoint *ep, struct rtp_h264 *out, struct fb_h264_s *h264, int retries )
{
	// stap-a: 1B头 + 2B sps_len + sps + 2B pps_len + pps
	struct iovec v[6];
	unsigned char stap_hdr;
	unsigned char sps_len[2];
	unsigned char pps_len[2];
	int max_payload;
	int stap_payload_len;

	if( !ep || !ep->sendEnable || !h264 || !h264->sps || !h264->pps || !h264->sps_len || !h264->pps_len ) 
	{
		_os_printf("%s %d\n", __FUNCTION__, __LINE__);
		return;
	}

	max_payload = rtp_get_payload_size_limit( ep, RTP_HEADER_SIZE );
	if( max_payload <= 0 )
	{
		ep->sendEnable = 0;
		_os_printf("%s %d\n", __FUNCTION__, __LINE__);
		return;
	}

	stap_payload_len = 1 + 2 + h264->sps_len + 2 + h264->pps_len;
	if( stap_payload_len > max_payload )
	{
		h264_send_nalu_to_endpoint( ep, out, h264->sps, h264->sps_len, 0, retries );
		if( ep->sendEnable )
		{
			h264_send_nalu_to_endpoint( ep, out, h264->pps, h264->pps_len, 0, retries );
		}
		return;
	}

	stap_hdr = ( h264->sps[0] & 0xE0 ) | 24;
	PUT_16( sps_len, h264->sps_len );
	PUT_16( pps_len, h264->pps_len );
	v[1].iov_base = &stap_hdr;
	v[1].iov_len = 1;
	v[2].iov_base = sps_len;
	v[2].iov_len = 2;
	v[3].iov_base = h264->sps;
	v[3].iov_len = h264->sps_len;
	v[4].iov_base = pps_len;
	v[4].iov_len = 2;
	v[5].iov_base = h264->pps;
	v[5].iov_len = h264->pps_len;
	if( rtp_sendmsg( ep, v, 6, out->timestamp, 0, retries ) < 0 )
	{
		ep->sendEnable = 0;
	}
}

static int h264_send_more( rtp_loop_search_ep search, void *ls, void *track, void *d )
{
	struct rtp_h264 *out = (struct rtp_h264 *)d;
	struct framebuff *fb = (struct framebuff *)out->f->get_f;
	struct fb_h264_s *h264;
	uint8_t *nalu;
	uint32_t nalu_len;
	struct rtp_endpoint *ep;
	void *head;

	if( !fb || !fb->data || fb->len <= 0 )
	{
		return -1;
	}

	h264 = (struct fb_h264_s *)fb->priv;
	if( h264 )
	{
		if( h264->type == 1 )
		{
			out->first_flag = 1;
		}
		else if( out->last_count != h264->count )
		{
			out->first_flag = 0;
			os_printf(KERN_NOTICE"drop frame\n");
		}

		if( !out->first_flag )
		{
			return -1;
		}

		nalu = fb->data + h264->start_len;
		nalu_len = fb->len - h264->start_len;
		if( nalu_len <= 0 )
		{
			return -1;
		}

		out->last_count = h264->count + 1;
		head = ls;
		while( head )
		{
			head = search( head, track, (void *)&ep );
			if( !ep ) 

			{
				continue;
			}
			
			if( !ep->sendEnable )
			{
				continue;
			}

			if( h264->type == 1 )
			{	
				h264_send_sps_pps( ep, out, h264, 30 );
			}

			if( ep->sendEnable )
			{
				h264_send_nalu_to_endpoint( ep, out, nalu, nalu_len, 1, 30 );
			}

			ep->sendEnable = 1;
		}
	}

	return 0;
}

struct rtp_media *new_rtp_media_h264_stream( struct stream *stream )
{
	struct rtp_h264 *out;
	int fincr, fbase;
	struct rtp_media *m;

	stream->get_framerate( stream, &fincr, &fbase );
	out = (struct rtp_h264 *)malloc( sizeof( struct rtp_h264 ) );
	out->f = NULL;
	out->timestamp = 0;
	//return new_rtp_media( audio_get_sdp, audio_get_payload,audio_process_frame, audio_send, out );
	m = new_rtp_rtcp_media( h264_get_sdp, h264_get_payload,h264_process_frame, h264_send,new_rtcp_send, out );
	if(m)
	{
		m->sample_rate = H264_SAMPLE;//默认
		m->type = 1;	//音频
		m->send_more = h264_send_more;
		m->per_ms_incr = H264_SAMPLE/(1000);
	}
	return m;
}

void del_rtp_media_h264_stream( struct rtp_media *m )
{
	struct rtp_h264 *out = (struct rtp_h264 *)m->private;
	if(out)
	{
		free(out);
	}

	if(m)
	{
		free(m);
	}
}

