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
#define DYNAMIC_FRAGMENT_SIZE 1


struct rtp_h264 {
	//为了获取到frame,因为使用了链表形式
	unsigned char *d;//链表第一帧数据
	struct frame *f;
	unsigned int timestamp;	
	int max_send_size;
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


//设置数据头部,并且将音频数据拷贝到需要发送的buf中,返回需要发送的buf长度
static int set_send_rtp_packet_head_h264(struct rtp_endpoint *ep, unsigned int timestamp, int marker,unsigned char *real_buf,unsigned char *send_buf,int plen )
{
	uint8_t *inter,*rtphdr;
	//unsigned char *data_buf = send_buf+12;
	inter = send_buf;
	rtphdr = inter;
	//int i;
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

	return plen+12;
}


//端口发送数据,
int fd_send_data_h264(int fd,unsigned char *sendbuf,int sendLen,int times)
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
int send_rtp_packet_more_h264( struct rtp_endpoint *ep, unsigned char *sendbuf, int sendLen,int times )
{
	fd_send_data_h264(ep->trans.udp.rtp_fd,sendbuf,sendLen,times);
	return 0;
}


//支持大于1460的音频数量
static int h264_send_more( rtp_loop_search_ep search,void *ls,void *track, void *d,void *cache_buf,int cache_buf_len )
{
	int max_data_size;
	unsigned char *send_buf;
	uint8_t is_i_frame = 0;
	int plen;
	int plen_offset;
	int send_first = 1;
	int send_end = 0;
	struct rtp_endpoint *ep;
	void *head;
	int send_total_len;
	int i = 0;
	int itk = 0;
	char *sps_pps_buf;

	
	struct rtp_h264 *out = (struct rtp_h264 *)d;
	struct framebuff *fb = (struct framebuff *)out->f->get_f;
	uint8_t *real_buf = fb->data;
	int h264_flen = fb->len;	
	#if DYNAMIC_FRAGMENT_SIZE
	uint8_t send_times = 0;
	int dynamic_fragment_size;
	#endif

	if(fb->priv)
	{
		send_buf = cache_buf;
		
		max_data_size = out->max_send_size-12;
		struct fb_h264_s *h264 = (struct fb_h264_s *)fb->priv;
		real_buf = fb->data+1 + h264->start_len;//第一byte是数据类型(I帧  P帧 B帧)
		//I帧,先发送pps和sps
		//os_printf("h264->type:%d\tfb:%X\tpriv:%X\tcount:%d\n",h264->type,fb,fb->priv,h264->count);
		if(h264->type == 1)
		{
			//plen = 19;
			is_i_frame = 1;
			head = ls;
			while(head)
			{
				head = search(head,track,(void*)&ep);
				if(ep)
				{
					//uint16_t rtp_head_len;
					//数据内容是一样的,修改头部就可以了
					int start = 12;
					plen = 0;
					sps_pps_buf = (char*)&send_buf[start];
					sps_pps_buf[plen++] = 0x78;
					sps_pps_buf[plen++] = (h264->sps_len>>8)&0xff;
					sps_pps_buf[plen++] = h264->sps_len;
					memcpy(&sps_pps_buf[plen],h264->sps,h264->sps_len);
					plen+= h264->sps_len;
					sps_pps_buf[plen++] = (h264->pps_len>>8)&0xff;
					sps_pps_buf[plen++] = h264->pps_len;
					memcpy(&sps_pps_buf[plen],h264->pps,h264->pps_len);
					plen += h264->pps_len;

					send_total_len = set_send_rtp_packet_head_h264(ep,out->timestamp, 0,NULL,send_buf,plen );
					send_rtp_packet_more(ep, send_buf, send_total_len,30 );
				}
			}
		}

		h264_flen = fb->len - 1 - h264->start_len;
		plen_offset = 0;
		send_first = 1;
		for( i = 0; i < h264_flen; i += plen )
		{
			#if DYNAMIC_FRAGMENT_SIZE
			if(h264_flen < max_data_size && send_times == 0)
			{
				send_times = 1;
				dynamic_fragment_size = is_i_frame ? max_data_size : 6;
			}
			else
				dynamic_fragment_size = max_data_size;
			
			if((h264_flen - i) > dynamic_fragment_size)
			{
				plen = dynamic_fragment_size;
				send_end = 0;
			}
			#else
			if((h264_flen - i) > max_data_size)
			{
				plen = max_data_size;
				send_end = 0;
			}
			#endif
			else
			{
				plen = h264_flen - i;
				send_end = 1;
			}

			send_buf = cache_buf;
			os_memcpy(&send_buf[itk+14],&real_buf[plen_offset],plen);


			plen_offset += plen;

			if(plen_offset == h264_flen){
				send_end = 1;
			}

			if(send_first)
			{
				send_buf[12] = 0x7c;
				if(is_i_frame == 1)
					send_buf[13] = 0x85;
				else
					send_buf[13] = 0x81;
				
				send_first = 0;
			}else{
				if(send_end){
					send_buf[12] = 0x7c;
					if(is_i_frame == 1)
						send_buf[13] = 0x45;
					else
						send_buf[13] = 0x41;
				}else{
					send_buf[12] = 0x7c;
					if(is_i_frame == 1)
						send_buf[13] = 0x05;
					else
						send_buf[13] = 0x01;
				}
			}
			
			//重新赋值ls的头
			head = ls;
			while(head)
			{
				//获取ep
				head = search(head,track,(void*)&ep);
				if(ep)
				{
					//数据内容是一样的,修改头部就可以了
					send_total_len = set_send_rtp_packet_head_h264(ep,out->timestamp, plen + i == h264_flen,NULL,send_buf,plen );
					if(ep->sendEnable)
					{
						send_rtp_packet_more(ep, send_buf, send_total_len+2,30 );
					}
					
				}
			}

		}
	}

	else
	{
		//real_buf = real_buf+4;
		//h264_flen = h264_flen-4;
		if((real_buf[3] == 0x01) && (real_buf[4] == 0x67) && (real_buf[5] == 0x4d)){
			is_i_frame = 1;
		}else{
			is_i_frame = 0;
		}
		
		max_data_size = out->max_send_size-12;
		//h264_flen = sizeof(h264_demo_I);
		//is_i_frame = 1;

		send_buf = cache_buf;

		if(is_i_frame == 1){
			plen = 20;
			head = ls;
			head = search(head,track,(void*)&ep);
			if(ep)
			{
				//数据内容是一样的,修改头部就可以了
				send_total_len = set_send_rtp_packet_head_h264(ep,out->timestamp, plen + i == h264_flen,NULL,send_buf,plen );
				send_buf[12] = 0x78;
				send_buf[13] = 0x00;
				send_buf[14] = 0x0b;
				for(itk = 0;itk < 11;itk++){
					send_buf[14+1+itk] = real_buf[4+itk];
				}
				send_buf[14+11+1] = 0x00;
				send_buf[14+11+2] = 0x04;
				for(itk = 0;itk < 4;itk++){
					send_buf[14+11+2+1+itk] = real_buf[4+11+4+itk];
				}		
				
				send_rtp_packet_more(ep, send_buf, send_total_len,31 );
			}
		}

		if(is_i_frame == 1){
			h264_flen = h264_flen-4-10-4-4-4-1;
		}else{
			h264_flen = h264_flen-4;
		}
		
		plen_offset = 0;
		send_first = 1;
		for( i = 0; i < h264_flen; i += plen )
		{
			#if DYNAMIC_FRAGMENT_SIZE
			if(h264_flen < max_data_size && send_times == 0)
			{
				send_times = 1;
				dynamic_fragment_size = is_i_frame ? max_data_size : 6;
			}
			else
				dynamic_fragment_size = max_data_size;
			
			if((h264_flen - i) > dynamic_fragment_size)
			{
				plen = dynamic_fragment_size;
				send_end = 0;
			}
			#else
			if((h264_flen - i) > max_data_size)
			{
				plen = max_data_size;
				send_end = 0;
			}
			#endif
			else
			{
				plen = h264_flen - i;
				send_end = 1;
			}

			send_buf = cache_buf;
			if(is_i_frame == 1){
				for(itk = 0;itk < plen;itk++){
					send_buf[itk+14] = real_buf[28+itk+plen_offset];
				}
			}else{
				for(itk = 0;itk < plen;itk++){
					send_buf[itk+14] = real_buf[5+itk+plen_offset];
				}
			}

			plen_offset += plen;

			if(plen_offset == h264_flen){
				send_end = 1;
			}

			if(send_first)
			{
				send_buf[12] = 0x7c;
				if(is_i_frame == 1)
					send_buf[13] = 0x85;
				else
					send_buf[13] = 0x81;
				
				send_first = 0;
			}else{
				if(send_end){
					send_buf[12] = 0x7c;
					if(is_i_frame == 1)
						send_buf[13] = 0x45;
					else
						send_buf[13] = 0x41;
				}else{
					send_buf[12] = 0x7c;
					if(is_i_frame == 1)
						send_buf[13] = 0x05;
					else
						send_buf[13] = 0x01;
				}
			}
			
			//重新赋值ls的头
			head = ls;
			while(head)
			{
				//获取ep
				head = search(head,track,(void*)&ep);
				if(ep)
				{
					//数据内容是一样的,修改头部就可以了
					send_total_len = set_send_rtp_packet_head_h264(ep,out->timestamp, plen + i == h264_flen,NULL,send_buf,plen );
					send_rtp_packet_more(ep, send_buf, send_total_len+2,30 );
				}

			}
		}
	}

	head = ls;
	while(head)
	{
		//获取ep
		head = search(head,track,(void*)&ep);
		if(ep)
		{
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
	out->max_send_size = MAX_DATA_PACKET_SIZE;
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



