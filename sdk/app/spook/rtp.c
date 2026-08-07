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
//#include "lwip\fcntl.h"
#include "lwip\api.h"
#include "lwip\tcp.h"

#include <event.h>
#include <log.h>
#include <frame.h>
#include <rtp.h>
#include "spook_config.h"
//#include <linux/linux_mutex.h>
#include "lwip/api.h"
#include <csi_kernel.h>
#include "rtp_media.h"
#include "lib/net/eloop/eloop.h"


int rtcp_send( struct rtp_endpoint *ep, uint8_t *pbuf,uint16 len);
extern int sendmsg2(int fd, struct msghdr *msg, unsigned flags);

static int rtp_port_start = 50000, rtp_port_end = 60000;
k_task_handle_t spook_rtcp_handle = NULL;

/*************rtp command to APP***********/
struct rtp_endpoint *new_rtp_endpoint( int payload )
{
	struct rtp_endpoint *ep;

	if( ! ( ep = (struct rtp_endpoint *)
			malloc( sizeof( struct rtp_endpoint ) ) ) )
		return NULL;
	ep->payload = payload;
	ep->max_data_size = MAX_DATA_PACKET_SIZE; /* default maximum */
	ep->ssrc = 0;
	random_bytes( (unsigned char *)&ep->ssrc, 4 );
	ep->start_timestamp = 0;
	random_bytes( (unsigned char *)&ep->start_timestamp, 4 );
	ep->start_timestamp &= 0xFFFF;
	ep->last_timestamp = ep->start_timestamp;
	ep->seqnum = 0;
	random_bytes( (unsigned char *)&ep->seqnum, 2 );
	ep->packet_count = 0;
	ep->octet_count = 0;
	gettimeofday( &ep->last_rtcp_recv, NULL );
	ep->trans_type = 0;
	ep->rtcp_send_timestamp = os_jiffies();
	ep->sendEnable = 1;
	return ep; 
}

void close_rtp_socket( void *ei, void *d )
{
	struct event* e = (struct event*)ei;
	int fd = (int)d;
	close( fd);
	_os_printf("%s fd:%d\n",__FUNCTION__,fd);
	e->flags |= EVENT_F_REMOVE;
}

//socket的关闭会通过eloop内部去关闭,这样就不存在有异步操作的问题了(移除对应事件和关闭socket)
void del_rtp_endpoint( struct rtp_endpoint *ep )
{
	switch( ep->trans_type )
	{
	case RTP_TRANS_UDP:
		
		eloop_remove_event( ep->trans.udp.rtp_event );
		//创建socket关闭的事件
		eloop_add_alarm(os_jiffies(),EVENT_F_ENABLED,close_rtp_socket,(void*)ep->trans.udp.rtp_fd);
		eloop_remove_event( ep->trans.udp.rtcp_event );
		eloop_add_alarm(os_jiffies(),EVENT_F_ENABLED,close_rtp_socket,(void*)ep->trans.udp.rtcp_fd);
		break;
	case RTP_TRANS_INTER:
		interleave_disconnect( ep->trans.inter.conn,
						ep->trans.inter.rtp_chan );
		interleave_disconnect( ep->trans.inter.conn,
						ep->trans.inter.rtcp_chan );
		break;
	}

	free( ep );
}

void update_rtp_timestamp( struct rtp_endpoint *ep, int time_increment )
{
	ep->last_timestamp += time_increment;
	ep->last_timestamp &= 0xFFFFFFFF;
}

static void udp_rtp_read( void *ei, void *d )
{
	struct rtp_endpoint *ep = (struct rtp_endpoint *)d;
	unsigned char buf[1024];
	int ret;
	ret = read( ep->trans.udp.rtp_fd, buf, sizeof( buf ) );
	if( ret > 0 )
	{ 
		/* some SIP phones don't send RTCP */
		gettimeofday( &ep->last_rtcp_recv, NULL );
		return;
	} else if( ret < 0 )
		spook_log( SL_VERBOSE, "error on UDP RTP socket: %s",
			strerror( get_errno() ) );
	else spook_log( SL_VERBOSE, "UDP RTP socket closed" );
	ep->session->select_close(ep->session, ep);
}

static void udp_rtcp_read( void *ei, void *d )
{
	#if JPG_EN	
	static uint8_t rtcp_speed_count = 0x80;
	#endif
	struct rtp_endpoint *ep = (struct rtp_endpoint *)d;
	unsigned char buf[1024];
	int ret;
	ret = read( ep->trans.udp.rtcp_fd, buf, sizeof( buf ) );
	if( ret > 0 )
	{
#if JPG_EN		
		//Wifi_data_receive(buf,ret);
		//_os_printf("(%d)rtcp get:%d   ===== %x%x\r\n",os_jiffies(),buf[12],buf[0],buf[1]);
		if((buf[0] == 0x81)&&(buf[1] == 0xc9)){
			if(buf[12] != 0){
				rtcp_speed_count++;
				if(rtcp_speed_count >= 0x83){
					rtcp_speed_count = 0x82;
				}
			}

			if(buf[12] == 0){
				rtcp_speed_count--;
				if(rtcp_speed_count <= 0x7d){
					rtcp_speed_count = 0x7e;
				}
			}
		}
#endif		
		gettimeofday( &ep->last_rtcp_recv, NULL );
		return;
	} else if( ret < 0 )
		spook_log( SL_VERBOSE, "error on UDP RTCP socket: %s",
			strerror( get_errno() ) );
	else spook_log( SL_VERBOSE, "UDP RTCP socket closed" );
	ep->session->select_close(ep->session, ep);
}

void interleave_recv_rtcp( struct rtp_endpoint *ep, unsigned char *d, int len )
{
	spook_log( SL_DEBUG, "received RTCP packet from client INTERLEAVE" );
	gettimeofday( &ep->last_rtcp_recv, NULL );	
}

int g_timeout = 0;


#define RECORDER_RTCP_NTP		0	// 使用录风者时可以开启，降低延时

int new_rtcp_send( struct rtp_endpoint *ep, void *d )/*RFC3550*/
{
	//struct rtp_jpeg *out = (struct rtp_jpeg *)d;
	struct rtp_media *rtp = (struct rtp_media*)d;
	if(ep->trans_type == RTP_TRANS_UDP)
	{
	int res;
//	unsigned char buf[16384];
	unsigned char buf[128];
	unsigned int ntp_sec, ntp_usec;
	unsigned int rtp_timestamp;
	uint16 len_send;
	int len = 6;//字符串长度,"taixin"
	if(ep == NULL || ep->session == NULL){
		_os_printf("send rtcp no session!!\n");
		return -1;
	}
	if(os_jiffies()-ep->rtcp_send_timestamp < 5000)
	{
		return -1;
	}

		//ep->last_timestamp = ( ep->start_timestamp + timestamp )& 0xFFFFFFFF;
		os_printf("%s:%d\n",__FUNCTION__,__LINE__);
		ep->rtcp_send_timestamp = os_jiffies();
		rtp_timestamp = ep->start_timestamp + ep->rtcp_send_timestamp * rtp->per_ms_incr;
	#if RECORDER_RTCP_NTP
		ntp_sec = 0;
		ntp_usec = 0;
	#else
		struct timeval now;
		gettimeofday( &now, NULL );
		ntp_sec = now.tv_sec + 0x83AA7E80;
		ntp_usec = (double)( (double)now.tv_usec * (double)0x4000000 ) / 15625.0;
	#endif
		
		/*spook_log( SL_DEBUG, "ssrc=%u, ntp_sec=%u, ntp_usec=%u last_timestamp=%u packet_count=%d octet_count=%d",
				x->ssrc, ntp_sec, ntp_usec, x->last_timestamp, x->packet_count, x->octet_count );*/
		//_os_printf("ssrc:%X\ttime:%d\tstart:%d\n",ep->ssrc,ep->rtcp_send_timestamp,ep->start_timestamp);
		buf[0] = 2 << 6; // version
		buf[1] = 200; // packet type is Sender Report
		PUT_16( buf + 2, 6 ); // length in words minus one
		PUT_32( buf + 4, ep->ssrc );
		PUT_32( buf + 8, ntp_sec );
		PUT_32( buf + 12, ntp_usec );
		PUT_32( buf + 16, rtp_timestamp);//ep->rtcp_send_timestamp*per_ms_incr+ep->start_timestamp );
		PUT_32( buf + 20, ep->packet_count );
		PUT_32( buf + 24, ep->octet_count );
		buf[28] = ( 2 << 6 ) | 1; // version; source count = 1
		buf[29] = 202; // packet type is Source Description    SDES
		PUT_16( buf + 30, (4+4+2+len)/4-1 ); // length in words minus one      sr = 5word   5-1 = 4    !!!!!!!
	//	PUT_16( buf + 30, 4 ); // length in words minus one
		PUT_32( buf + 32, ep->ssrc );
		buf[36] = 0x01; // field type is CNAME    36/4=9   52/4 = 13
		buf[37] = len; // text length
	//	buf[37] = 16; // text length
		memcpy( buf + 38, "taixin", len );
	//	memcpy( buf + 38, pbuf, 16 );
		len_send = (32+4+2+len);


		res = send( ep->trans.udp.rtcp_fd, buf, len_send, 0 );
	}


	return -1;
}


int connect_udp_endpoint( struct rtp_endpoint *ep,
		struct in_addr dest_ip, int dest_port, int *our_port )
{
	struct sockaddr_in rtpaddr, rtcpaddr;
	int port, success = 0, max_tries, rtpfd = -1, rtcpfd = -1;
	unsigned int i;

	rtpaddr.sin_family = rtcpaddr.sin_family = AF_INET;
	rtpaddr.sin_addr.s_addr = rtcpaddr.sin_addr.s_addr = 0;

	port = rtp_port_start + rand() % ( rtp_port_end - rtp_port_start );
	if( port & 0x1 ) ++port;
	max_tries = ( rtp_port_end - rtp_port_start + 1 ) / 2;

	for( i = 0; i < max_tries; ++i )
	{
		if( port + 1 > rtp_port_end ) port = rtp_port_start;
		rtpaddr.sin_port = htons( port );
		rtcpaddr.sin_port = htons( port + 1 );
		if( rtpfd < 0 &&
			( rtpfd = socket( PF_INET, SOCK_DGRAM, 0 ) ) < 0 )
		{
			spook_log( SL_WARN, "unable to create UDP RTP socket: %s",
					strerror( get_errno() ) );
			return -1;
		}
		if( rtcpfd < 0 &&
			( rtcpfd = socket( PF_INET, SOCK_DGRAM, 0 ) ) < 0 )
		{
			spook_log( SL_WARN, "unable to create UDP RTCP socket: %s",
					strerror( get_errno() ) );
			close( rtpfd );
			return -1;
		}
		if( bind( rtpfd, (struct sockaddr *)&rtpaddr,
					sizeof( rtpaddr ) ) < 0 )
		{
			if( get_errno() == EADDRINUSE )
			{
				port += 2;
				continue;
			}
			spook_log( SL_WARN, "strange error when binding RTP socket: %s",
					strerror( get_errno() ) );
			close( rtpfd );
			close( rtcpfd );
			return -1;
		}
		if( bind( rtcpfd, (struct sockaddr *)&rtcpaddr,
					sizeof( rtcpaddr ) ) < 0 )
		{
			if( get_errno() == EADDRINUSE )
			{
				close( rtpfd );
				rtpfd = -1;
				port += 2;
				continue;
			}
			spook_log( SL_WARN, "strange error when binding RTCP socket: %s",
					strerror( get_errno() ) );
			close( rtpfd );
			close( rtcpfd );
			return -1;
		}
		success = 1;
		break;
	}
	if( ! success )
	{
		spook_log( SL_WARN, "ran out of UDP RTP ports!" );
		return -1;
	}
	rtpaddr.sin_family = rtcpaddr.sin_family = AF_INET;
	rtpaddr.sin_addr = rtcpaddr.sin_addr = dest_ip;
	rtpaddr.sin_port = htons( dest_port );
	rtcpaddr.sin_port = htons( dest_port + 1 );
	if( connect( rtpfd, (struct sockaddr *)&rtpaddr,
				sizeof( rtpaddr ) ) < 0 )
	{
		spook_log( SL_WARN, "strange error when connecting RTP socket: %s",
				strerror( get_errno() ) );
		close( rtpfd );
		close( rtcpfd );
		return -1;
	}
	if( connect( rtcpfd, (struct sockaddr *)&rtcpaddr,
				sizeof( rtcpaddr ) ) < 0 )
	{
		spook_log( SL_WARN, "strange error when connecting RTCP socket: %s",
				strerror( get_errno() ) );
		close( rtpfd );
		close( rtcpfd );
		return -1;
	}
	i = sizeof( rtpaddr );
	if( getsockname( rtpfd, (struct sockaddr *)&rtpaddr, &i ) < 0 )
	{
		spook_log( SL_WARN, "strange error from getsockname: %s",
				strerror( get_errno() ) );
		close( rtpfd );
		close( rtcpfd );
		return -1;
	}


	//int tos = 0xe0;
	//setsockopt(rtpfd, IPPROTO_IP, IP_TOS, (void *)&tos , sizeof(tos));

//	int tos = 0xb8;
//	setsockopt(rtpfd, IPPROTO_IP, IP_TOS, (void *)&tos , sizeof(tos));

	ep->max_data_size = MAX_DATA_PACKET_SIZE; /* good guess for preventing fragmentation */
	ep->trans_type = RTP_TRANS_UDP;
	sprintf( ep->trans.udp.sdp_addr, "IP4 %s",
				inet_ntoa( rtpaddr.sin_addr ) );
	ep->trans.udp.sdp_port = ntohs( rtpaddr.sin_port );
	ep->trans.udp.rtp_fd = rtpfd;
	ep->trans.udp.rtcp_fd = rtcpfd;
	ep->trans.udp.rtp_event = eloop_add_fd( rtpfd, EVENT_READ, EVENT_F_ENABLED, udp_rtp_read, ep );
	ep->trans.udp.rtcp_event = eloop_add_fd( rtcpfd, EVENT_READ, EVENT_F_ENABLED, udp_rtcp_read, ep );
	spook_log( SL_DEBUG, "connect_udp_endpoint add_fd_event udp_rtp_read udp_rtcp_read");
	*our_port = port;

	return 0;
}

void connect_interleaved_endpoint( struct rtp_endpoint *ep,
		struct conn *conn, int rtp_chan, int rtcp_chan )
{
	ep->trans_type = RTP_TRANS_INTER;
	ep->trans.inter.conn = conn;
	ep->trans.inter.rtp_chan = rtp_chan;
	ep->trans.inter.rtcp_chan = rtcp_chan;
	ep->trans.inter.running = 1;
}

int send_rtp_packet( struct rtp_endpoint *ep, struct iovec *v, int count,
			unsigned int timestamp, int marker )
{
	static unsigned int timestamp_start = 0;
	unsigned char send_enable = 1;
	static unsigned char send_drop = 0;
	unsigned char rtphdr[12];
	struct msghdr mh;
	int i;
	static int retrans = 0;
	/*int send_count = 0;*/


	if(timestamp_start != timestamp){
		send_enable = 1;
		send_drop = 0;
		timestamp_start = timestamp;
	}


	if(send_drop == 1){
		send_enable = 0;
	}


	ep->last_timestamp = ( ep->start_timestamp + timestamp )
					& 0xFFFFFFFF;


	/*
	spook_log( SL_DEBUG, "RTP: payload %d, seq %u, time %u, marker %d",
		ep->payload, ep->seqnum, ep->last_timestamp, marker );*/


	rtphdr[0] = 2 << 6; /* version */
	rtphdr[1] = ep->payload;
	if( marker ) rtphdr[1] |= 0x80;
	PUT_16( rtphdr + 2, ep->seqnum );
	PUT_32( rtphdr + 4, ep->last_timestamp );
	PUT_32( rtphdr + 8, ep->ssrc );


	v[0].iov_base = rtphdr;
	v[0].iov_len = 12;

	switch( ep->trans_type )
	{
	case RTP_TRANS_UDP:
		memset( &mh, 0, sizeof( mh ) );
		mh.msg_iov = v;
		mh.msg_iovlen = count;
retry:	
		if(send_enable){
			if( sendmsg2( ep->trans.udp.rtp_fd, &mh, 0 ) < 0 )
			{
				if(0 && (get_errno() != ENOMEM) && (get_errno() != EBADF) && (get_errno() != EAGAIN))
				{
					_os_printf("get_errno():%d\n",get_errno());
					#if 0
					spook_log( SL_VERBOSE, "error sending UDP RTP frame: %s %d	 %d",
						strerror( get_errno() ),get_errno() ,ep->trans.udp.rtp_fd);
					ep->session->closed( ep->session, ep );
					#endif
					return -1;
				}
				else
				{
					csi_kernel_delay(5);
					retrans++;
					if(retrans < 5)
						goto retry;
					else
						send_drop = 1;
				}
			}
			else
			{
				retrans = 0;
			}
		}	
		break;
	case RTP_TRANS_INTER:
		if( interleave_send( ep->trans.inter.conn,
				ep->trans.inter.rtp_chan, v, count ) < 0 )
		{
			if((get_errno() != 12) && (get_errno() != 9))
			{
				spook_log( SL_VERBOSE, "error sending interleaved RTP frame" );			
				ep->session->closed( ep->session, ep );
				return -1;
			}
		}
		break;
	}



	for( i = 0; i < count; ++i ) ep->octet_count += v[i].iov_len;
	++ep->packet_count;
	ep->seqnum = ( ep->seqnum + 1 ) & 0xFFFF;


	return 0;
}


extern int fd_send_data(int fd,unsigned char *sendbuf,int sendLen,int times);
extern int set_send_data_head(unsigned char *send_buf,struct msghdr *msg);
extern unsigned char *get_send_real_data(struct msghdr *msg,char *sendtobuf);

extern uint8_t rtsp_send_audio;
//数据发送
void rtsp_tcp_send_event(void *ei, void *d)
{
	struct rtp_endpoint *ep = (struct rtp_endpoint *)d;
	struct conn *conn = (struct conn *)ep->session->conn;
	int offset = 0;
	int send_len;
	uint32_t start_time = os_jiffies();
	if(conn->fd > 0 && conn->running)
	{
		int len;
		//给到eloop去发送数据吧
		uint8_t *tcp_head = conn->sendbuf - SPOOK_CACHE_BUF_HEAD_LEN;
		tcp_head[0] = 0x24; // 0x24 = 00100100
		//通过payload去识别音频
		if(ep->payload == 97)
			tcp_head[1] = 0x02; // 0x00 = 00000000;
		else
			tcp_head[1] = 0x00;
		tcp_head[2] = (conn->send_buf_len >> 8) & 0xFF; // 高字节
		tcp_head[3] = conn->send_buf_len & 0xFF; // 低字节
		send_len = conn->send_buf_len+SPOOK_CACHE_BUF_HEAD_LEN;
		again:
		len = lwip_send( conn->fd, tcp_head+offset, send_len,0);
		if(len == 0)
		{
			goto rtsp_tcp_send_event_end;
		}
		if(len < 0)
		{
			if(get_errno() == EAGAIN)
			{
				if(os_jiffies() - start_time >= 3000)
				{
					ep->sendEnable = 0;
					ep->session->closed( ep->session, ep );
					os_printf("%s:%d\ttimeout\n",__FUNCTION__,__LINE__);
					goto rtsp_tcp_send_event_end;
				}
				os_sleep_ms(1);
				goto again;
			}
			else
			{
				ep->sendEnable = 0;
				ep->session->closed( ep->session, ep );
				goto rtsp_tcp_send_event_end;
			}

		}
		if(len != send_len)
		{
			//os_printf("send len:%d\tconn->send_buf_len:%d\n",len,conn->send_buf_len);
			offset += len;
			send_len -= len;
			if(send_len > 0)
			{
				goto again;
			}
		}

		//os_printf("conn->send_buf_len:%d\tlen:%d\n",conn->send_buf_len,len);
	}
rtsp_tcp_send_event_end:
	os_event_set(&conn->evt, RTSP_TCP_SEND_FINISH, NULL);
} 
int send_rtp_packet_more( struct rtp_endpoint *ep, unsigned char *sendbuf, int sendLen,int times )
{
	int res = -1;
	if(ep->trans_type == RTP_TRANS_UDP)
	{
		res = fd_send_data(ep->trans.udp.rtp_fd,sendbuf,sendLen,times);
		//有一次发送失败,则这张图片不完整,设置标志位,等待下一张图
		if(res != 0)
		{
			ep->sendEnable = 0;
		}
	}
	else
	{
		if(ep->session->conn->fd > 0 && ep->session->conn->running)
		{
			ep->session->conn->sendbuf = sendbuf; //发送数据的buf
			ep->session->conn->send_buf_len = sendLen; //发送数据的长度
			//RTSP_TCP_SEND_FINISH
			eloop_add_alarm(os_jiffies(),EVENT_F_ENABLED,rtsp_tcp_send_event,(void*)ep);
			os_event_wait(&ep->session->conn->evt, RTSP_TCP_SEND_FINISH, NULL, OS_EVENT_WMODE_CLEAR, -1);
		}
		
		//os_printf("%s:%d\n",__FUNCTION__,__LINE__);

	}

	return 0;
}

//设置数据头部
int set_send_rtp_packet_head( struct rtp_endpoint *ep, struct iovec *v, int count,unsigned int timestamp, int marker,unsigned char *send_buf )
{
	unsigned char rtphdr[12];
	struct msghdr mh;
	int i;
	int total_len = 0;
	/*int send_count = 0;*/

	ep->last_timestamp = ( ep->start_timestamp + timestamp )& 0xFFFFFFFF;

	/*
	spook_log( SL_DEBUG, "RTP: payload %d, seq %u, time %u, marker %d",
		ep->payload, ep->seqnum, ep->last_timestamp, marker );*/
	if(ep->sendEnable)
	{
		rtphdr[0] = 2 << 6; /* version */
		rtphdr[1] = ep->payload;
		if( marker ) rtphdr[1] |= 0x80;
		PUT_16( rtphdr + 2, ep->seqnum );
		PUT_32( rtphdr + 4, ep->last_timestamp );
		PUT_32( rtphdr + 8, ep->ssrc );

		v[0].iov_base = rtphdr;
		v[0].iov_len = 12;

		memset( &mh, 0, sizeof( mh ) );
		mh.msg_iov = v;
		mh.msg_iovlen = count;	
		total_len = set_send_data_head(send_buf,&mh);
	}
	for( i = 0; i < count; ++i ) ep->octet_count += v[i].iov_len;
	++ep->packet_count;
	ep->seqnum = ( ep->seqnum + 1 ) & 0xFFFF;
	return total_len;
}



//获取需要发送数据buf头
unsigned char * get_send_rtp_packet_head(struct iovec *v ,int count,char *sendtobuf)
{
	unsigned char rtphdr[12];
	struct msghdr mh;
	//无效
	v[0].iov_base = rtphdr;
	v[0].iov_len = 12;
	memset( &mh, 0, sizeof( mh ) );
	mh.msg_iov = v;
	mh.msg_iovlen = count;	
	return get_send_real_data(&mh,sendtobuf); //返回发送数据的buf地址
}

