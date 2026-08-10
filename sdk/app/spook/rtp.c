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
	_os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	ep->session->select_close(ep->session, ep);
}

static void udp_rtcp_read( void *ei, void *d )
{
	struct rtp_endpoint *ep = (struct rtp_endpoint *)d;
	unsigned char buf[1024];
	int ret;
	ret = read( ep->trans.udp.rtcp_fd, buf, sizeof( buf ) );
	if( ret > 0 )
	{
		gettimeofday( &ep->last_rtcp_recv, NULL );
		return;
	} else if( ret < 0 )
		spook_log( SL_VERBOSE, "error on UDP RTCP socket: %s",
			strerror( get_errno() ) );
	else spook_log( SL_VERBOSE, "UDP RTCP socket closed" );
	_os_printf("%s:%d\n",__FUNCTION__,__LINE__);
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
	// struct rtp_media *rtp = (struct rtp_media*)d;
	if(ep->trans_type == RTP_TRANS_UDP)
	{
		int res;
		unsigned char buf[64];
		unsigned int ntp_sec, ntp_usec;
		unsigned int rtp_timestamp;
		uint16 len_send;
		uint16 len = 6; //字符串长度,"taixin"

		if(ep == NULL || ep->session == NULL){
			_os_printf("send rtcp no session!!\n");
			return -1;
		}

		if(os_jiffies()-ep->rtcp_send_timestamp < 5000)
		{
			return -1;
		}

		os_printf("%s:%d\n",__FUNCTION__,__LINE__);
		ep->rtcp_send_timestamp = os_jiffies();
		rtp_timestamp = ep->last_timestamp;
	#if RECORDER_RTCP_NTP
		ntp_sec = 0;
		ntp_usec = 0;
	#else
		struct timeval now;
		gettimeofday( &now, NULL );
		ntp_sec = now.tv_sec + 0x83AA7E80;
		ntp_usec = (double)( (double)now.tv_usec * (double)0x4000000 ) / 15625.0;
	#endif

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
		PUT_32( buf + 32, ep->ssrc );
		buf[36] = 0x01; // field type is CNAME    36/4=9   52/4 = 13
		buf[37] = len; // text length
		memcpy( buf + 38, "taixin", len );
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


	// int tos = 0xe0;
	// setsockopt(rtpfd, IPPROTO_IP, IP_TOS, (void *)&tos , sizeof(tos));

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

int rtp_get_packet_size_limit( const struct rtp_endpoint *ep )
{
	if( ep && ep->trans_type == RTP_TRANS_INTER ) 
	{
		return RTP_TCP_MAX_PACKET_SIZE;
	}
	
	if( ep && ep->max_data_size > 0 ) 
	{
		return ep->max_data_size;
	}
	
	return MAX_DATA_PACKET_SIZE;
}

int rtp_get_payload_size_limit( const struct rtp_endpoint *ep, int packet_overhead )
{
	int packet_limit = rtp_get_packet_size_limit( ep );

	if( packet_overhead < 0 )
	{
		return 0;
	}
	if( packet_limit <= packet_overhead )
	{
		return 0;
	}
	return packet_limit - packet_overhead;
}

static void rtp_build_header( struct rtp_endpoint *ep, uint32_t timestamp, uint8_t marker, uint8_t *rtphdr )
{
	ep->last_timestamp = ( ep->start_timestamp + timestamp ) & 0xFFFFFFFF;

	rtphdr[0] = 2 << 6;
	rtphdr[1] = ep->payload;
	if( marker ) 
	{
		rtphdr[1] |= 0x80;
	}

	PUT_16( rtphdr + 2, ep->seqnum );
	PUT_32( rtphdr + 4, ep->last_timestamp );
	PUT_32( rtphdr + 8, ep->ssrc );
}

static uint32_t rtp_get_payload_len( struct iovec *v, int count )
{
	uint32_t payload_len = 0;

	for( int i = 1; i < count; ++i )
	{
		payload_len += v[i].iov_len;
	}
		
	return payload_len;
}

static void rtp_advance_iov( struct iovec **iov, int *count, int bytes )
{
	while( bytes > 0 && *count > 0 )
	{
		if( bytes >= (int)(*iov)[0].iov_len )
		{
			bytes -= (*iov)[0].iov_len;
			++(*iov);
			--(*count);
		}
		else
		{
			(*iov)[0].iov_base = (unsigned char *)(*iov)[0].iov_base + bytes;
			(*iov)[0].iov_len -= bytes;
			bytes = 0;
		}
	}
}

static int rtp_send_udp( int fd, struct iovec *iov, int count, int retries )
{
	struct msghdr mh;
	int retry_count = 0;
	int total_len = 0;
	int ret;

	for( int i = 0; i < count; ++i )
	{
		total_len += iov[i].iov_len;
	}

	while( retry_count <= retries )
	{
		memset( &mh, 0, sizeof( mh ) );
		mh.msg_iov = iov;
		mh.msg_iovlen = count;
		ret = sendmsg( fd, &mh, MSG_DONTWAIT );
		if( ret == total_len )
		{
			return 0;
		}
		if( get_errno() == ENOMEM )
		{
			return -1;
		}
		if( ret >= 0 || get_errno() != EAGAIN )
		{
			_os_printf( "%s %d, err: %d\n", __FUNCTION__, __LINE__, get_errno() );
			return -1;
		}
		++retry_count;
		os_sleep_ms( 2 );
	}

	_os_printf( "%s %d, err: %d\n", __FUNCTION__, __LINE__, get_errno() );
	return -1;
}

static int rtp_send_tcp( struct rtp_endpoint *ep, struct iovec *v, int count)
{
	struct conn *conn = ep->trans.inter.conn;
	struct iovec local_iov[17];
	struct iovec *send_iov = local_iov;
	uint8_t interleave_hdr[4];
	struct msghdr mh;
	int payload_len = 0;
	int send_count;
	uint32_t start_time = os_jiffies();
	int ret;

	if( !conn || conn->fd <= 0 || !conn->running )
	{
		return -1;
	}

	if( count + 1 > (int)( sizeof( local_iov ) / sizeof( local_iov[0] ) ) )
	{
		_os_printf( "too many interleaved iov entries: %d", count + 1 );
		return -1;
	}

	for( int i = 0; i < count; ++i ) payload_len += v[i].iov_len;
	if( payload_len > RTP_TCP_MAX_PACKET_SIZE )
	{
		_os_printf( "interleaved RTP packet too large: %d", payload_len );
		return -1;
	}
	interleave_hdr[0] = '$';
	interleave_hdr[1] = ep->trans.inter.rtp_chan;
	PUT_16( interleave_hdr + 2, payload_len );

	local_iov[0].iov_base = interleave_hdr;
	local_iov[0].iov_len = sizeof( interleave_hdr );
	memcpy( &local_iov[1], v, sizeof( v[0] ) * count );
	send_count = count + 1;

	os_event_wait( &conn->evt, RTSP_TCP_WRITE_MUTEX, NULL, OS_EVENT_WMODE_AND | OS_EVENT_WMODE_CLEAR, -1 );

	while( send_count > 0 )
	{
		memset( &mh, 0, sizeof( mh ) );
		mh.msg_iov = send_iov;
		mh.msg_iovlen = send_count;
		os_event_wait( &conn->evt, RTSP_TCP_READ_MUTEX, NULL, OS_EVENT_WMODE_AND | OS_EVENT_WMODE_CLEAR, -1 );
		ret = sendmsg( conn->fd, &mh, MSG_DONTWAIT );
		os_event_set( &conn->evt, RTSP_TCP_READ_MUTEX, NULL );

		if( ret > 0 )
		{
			rtp_advance_iov( &send_iov, &send_count, ret );
			continue;
		}

		if( ret == 0 || get_errno() != EAGAIN )
		{
			ret = -1;
			break;
		}

		if( os_jiffies() - start_time >= 4000 )
		{
			ret = -1;
			break;
		}

		os_sleep_ms( 1 );
	}

	os_event_set( &conn->evt, RTSP_TCP_WRITE_MUTEX, NULL );
	if( send_count <= 0 )
	{
		return 0;
	}
	if( ret < 0 )
	{
		ep->sendEnable = 0;
		if( ep->session && ep->session->closed )
		{
			ep->session->closed( ep->session, ep );
		}
		_os_printf( "%s %d, err: %d\n", __FUNCTION__, __LINE__, get_errno() );
	}

	return ret;
}

int rtp_sendmsg( struct rtp_endpoint *ep, struct iovec *v, int count, uint32_t timestamp, uint8_t marker, int retries )
{
	uint8_t rtphdr[12];
	uint32_t payload_len;
	int ret = -1;

	if( !ep || !v || count <= 0 )
	{
		_os_printf("%s %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if( !ep->sendEnable )
	{
		return -1;
	}

	rtp_build_header( ep, timestamp, marker, rtphdr );
	v[0].iov_base = rtphdr;
	v[0].iov_len = sizeof( rtphdr );
	payload_len = rtp_get_payload_len( v, count );

	switch( ep->trans_type )
	{
	case RTP_TRANS_UDP:
		ret = rtp_send_udp( ep->trans.udp.rtp_fd, v, count, retries > 0 ? retries : 10 );
		if( ret < 0 ) 
		{
			ep->sendEnable = 0;
		}
		break;
	case RTP_TRANS_INTER:
		ret = rtp_send_tcp( ep, v, count );
		break;
	default:
		ret = -1;
		break;
	}

	if( ret < 0 ) 
	{
		return -1;
	}
	
	ep->octet_count += payload_len;
	++ep->packet_count;
	ep->seqnum = ( ep->seqnum + 1 ) & 0xFFFF;
	return 0;
}
