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
	int max_send_size;
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


#if 0
static int audio_send( struct rtp_endpoint *ep, void *d )
{
	//_os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	struct rtp_audio *out = (struct rtp_audio*)d;
	uint8_t *inter,*rtphdr,*realdata;
	int size = -1;
	int timeouts = 0;
	int fd = ep->trans.udp.rtp_fd;
	uint32_t i;

	struct sockaddr_in rtpaddr;
	unsigned int namelen = sizeof( rtpaddr );

	if( getsockname( fd, (struct sockaddr *)&rtpaddr, &namelen ) < 0 ) {
		spook_log( SL_ERR, "sendmsg getsockname error");
	}



	inter = audio_send_buf;
	rtphdr = inter;
	realdata = rtphdr + 12;
	struct frame *f = out->f;
	uint8_t *real_buf = GET_AUDIO_BUF(f->get_f);

	//_os_printf("out:%X\n",out);
	//_os_printf("real_buf:%X\t%X\t%X\t%d\n",real_buf,real_buf[0],real_buf[1],f->length);
	//memcpy(realdata,real_buf,f->length);
	for(i=0;i<f->length;i+=2)
	{
		realdata[i] = real_buf[i+1];
		realdata[i+1] = real_buf[i];
	}
	//_os_printf("length111:%d\n",f->length);
	//_os_printf("real:%X\t%X\t%d\n",realdata[0],realdata[1],out->timestamp);
	ep->last_timestamp = ( ep->start_timestamp + out->timestamp )& 0xFFFFFFFF;


	inter[0] = 2 << 6; /* version */
	//帧结束
	inter[1] = ep->payload | 0x80;

	PUT_16(rtphdr + 2, ep->seqnum );
	PUT_32(rtphdr+4, ep->last_timestamp);
	PUT_32( rtphdr + 8, ep->ssrc );
	ep->seqnum = ( ep->seqnum + 1 ) & 0xFFFF;

	while(size < 0 )
	{
		//size = sendto(fd, sendtobuf, total_len, MSG_DONTWAIT, (struct sockaddr *)&rtpaddr, namelen);
		//-8是音频过来前8字节是空的
		size = sendto(fd, audio_send_buf, f->length+12, MSG_DONTWAIT, (struct sockaddr *)&rtpaddr, namelen);
		//_os_printf("seq:%d\t%X\t%X\n",ep->seqnum,audio_send_buf[12],audio_send_buf[13] );
		timeouts++;

		if(timeouts>20)
		{
			break;
		}
		if(timeouts%2==0)
		{
			os_sleep_ms(1);
		}

		
	}

	//累计发送的数据量大小
	ep->octet_count += (f->length+12);

	//累计发送的数据包数量,由于现在音频只有1K左右,所以这里+1,实际要看发送多少包,然后增加多少包
	++ep->packet_count;

	

	
	//_os_printf("%s:%d\ttime:%d\n",__FUNCTION__,__LINE__,krhino_sys_tick_get());
	return 0;
}
#else
static int audio_send( struct rtp_endpoint *ep, void *d )
{

	return 0;
}
#endif




//设置数据头部,并且将音频数据拷贝到需要发送的buf中,返回需要发送的buf长度
static int set_send_rtp_packet_head_audio(struct rtp_endpoint *ep, unsigned int timestamp, int marker,unsigned char *real_buf,unsigned char *send_buf,int plen )
{
	uint8_t *inter,*rtphdr;
//	unsigned char *data_buf = send_buf+12;
	inter = send_buf;
	rtphdr = inter;
//	int i;
	ep->last_timestamp = ( ep->start_timestamp + timestamp )& 0xFFFFFFFF;

	inter[0] = 2 << 6; /* version */
	if(marker)
	{
		inter[1] = ep->payload | 0x80;
	}
	else
	{
		inter[1] = ep->payload;
	}

	PUT_32(rtphdr+4, ep->last_timestamp);
	PUT_32( rtphdr + 8, ep->ssrc );


	PUT_16(rtphdr + 2, ep->seqnum );
	ep->seqnum = ( ep->seqnum + 1 ) & 0xFFFF;
	++ep->packet_count;
	ep->octet_count += (plen);

	// // 音频数据进行大小端互换
	// for(i=0;i<plen;i+=2)
	// {
	// 	data_buf[i] = real_buf[i+1];
	// 	data_buf[i+1] = real_buf[i];
	// }
	return plen+12;
}


//端口发送数据,
int fd_send_data_audio(int fd,unsigned char *sendbuf,int sendLen,int times)
{
	int size = -1;
	int timeouts = 0;
	struct sockaddr_in rtpaddr;
	unsigned int namelen = sizeof( rtpaddr );
	if( getsockname( fd, (struct sockaddr *)&rtpaddr, &namelen ) < 0 ) {
		spook_log( SL_ERR, "sendmsg getsockname error");
	}

	
	while(size < 0 )
	{
		//size = sendto(fd, sendtobuf, total_len, MSG_DONTWAIT, (struct sockaddr *)&rtpaddr, namelen);
		size = sendto(fd, sendbuf, sendLen, MSG_DONTWAIT, (struct sockaddr *)&rtpaddr, namelen);
		//_os_printf("P:%d ",size);
		timeouts++;
		if(timeouts>times)
		{

			break;
		}
		if(size < 0)
		{
			os_sleep_ms(3);
		}		
	}
	if(size<0)
	{
		_os_printf("%s err size:%d\n",__FUNCTION__,size);
		return -1;
	}
	else
	{
		return 0;
	}
}

//数据发送
int send_rtp_packet_more_audio( struct rtp_endpoint *ep, unsigned char *sendbuf, int sendLen,int times )
{
	fd_send_data_audio(ep->trans.udp.rtp_fd,sendbuf,sendLen,times);
	return 0;
}
//支持大于1460的音频数量
static int audio_send_more( rtp_loop_search_ep search,void *ls,void *track, void *d ,void *cache_buf,int cache_buf_len)
{
	int max_data_size;
	unsigned char *send_buf;
	int plen;
	struct rtp_endpoint *ep;
	void *head;
	int send_total_len;
	int i;
	uint32_t audio_flen = 0;

	struct rtp_audio *out = (struct rtp_audio *)d;
	struct framebuff *fb = (struct framebuff *)out->f->get_f;

	uint8_t *real_buf = cache_buf + 12;
	real_buf[0] = 0x00;
	real_buf[1] = 0x10;
	real_buf[2] = ((fb->len-7) & 0x1FE0) >> 5;
	real_buf[3] = ((fb->len-7) & 0x1F) << 3;
	os_memcpy(real_buf+4, fb->data+7, fb->len-7);
	audio_flen = fb->len-7+4;

	//12是音频中rtp的头
	max_data_size = out->max_send_size-12;

	for( i = 0; i < audio_flen; i += plen )
	{
		if((audio_flen - i) > max_data_size)
		{
			plen = max_data_size;
		}
		else
		{
			plen = audio_flen - i;
		}

		send_buf = (unsigned char *)cache_buf;

		//重新赋值ls的头
		head = ls;
		while(head)
		{
			//获取ep
			head = search(head,track,(void*)&ep);
			if(ep)
			{
				//数据内容是一样的,修改头部就可以了
				send_total_len = set_send_rtp_packet_head_audio(ep,out->timestamp, plen + i == audio_flen,NULL,send_buf,plen );
				send_rtp_packet_more(ep, send_buf, send_total_len,10 );
				// os_printf("rtp send audio:%d\n",send_total_len);
			}

		}
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
	out->max_send_size = MAX_DATA_PACKET_SIZE;
	//return new_rtp_media( audio_get_sdp, audio_get_payload,audio_process_frame, audio_send, out );
	m = new_rtp_rtcp_media( audio_get_sdp, audio_get_payload,audio_process_frame, audio_send,new_rtcp_send, out );
	if(m)
	{
		m->sample_rate = AUDIO_SAMPLE;//默认
		m->type = 1;	//音频
		m->send_more = audio_send_more;
		m->per_ms_incr = AUDIO_SAMPLE/(1000);
	}    

	return m;
}


