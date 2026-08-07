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

#include "typesdef.h"
//#include <linux/linux_mutex.h>
#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>
#include <pmsg.h>
#include <rtp.h>
#include "lwip/api.h"
//#include "leases.h"
//#include "dhcpd.h"


static int base64[] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 00 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 10 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, /* 20 */
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1,  0, -1, -1, /* 30 */
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, /* 40 */
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, /* 50 */
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /* 60 */
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, /* 70 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 80 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 90 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* A0 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* B0 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* C0 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* D0 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* E0 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1  /* F0 */
};

void write_access_log( char *path, struct sockaddr *addr, int code, char *req,
		int length, char *referer, char *user_agent );

struct listener {
	int fd;
};

/* all of tcp connetctions loop linked list */
static struct conn *conn_list = NULL;

static void do_read( void *ei, void *d );

void drop_conn( struct conn *c )
{

	if(c->read_event)
	{
		eloop_remove_event( c->read_event );
		c->read_event = NULL;
	}

	if(c->write_event)
	{
		eloop_remove_event( c->write_event );
		c->write_event = NULL;
	}
	if( c->fd >= 0 ) close( c->fd );
	c->fd = -1;
	os_event_del(&c->evt);
	if( c->next ) c->next->prev = c->prev;
	if( c->prev ) c->prev->next = c->next;
	else conn_list = c->next;

	if(c->proto_state)
	{
		free(c->proto_state);
		c->proto_state = NULL;
	}
	free( c );
	_os_printf("%s finish\n",__FUNCTION__);
}

static void conn_write( void *ei, void *d )
{
	spook_log( SL_DEBUG, "tcp_w" );
	struct conn *c = (struct conn *)d;
	int ret, len;
	struct session *sess;

	while( c->send_buf_r != c->send_buf_w )
	{
		if( c->send_buf_w < c->send_buf_r )
			len = sizeof( c->send_buf ) - c->send_buf_r;
		else
			len = c->send_buf_w - c->send_buf_r;
		ret = write( c->fd, c->send_buf + c->send_buf_r, len );
		if( ret <= 0 )
		{
			if( ret < 0 && get_errno() == EAGAIN ) {
				return;
			}			
			//drop_conn( c );

			goto conn_write_exit;
		}
		c->send_buf_r += ret;
		if( c->send_buf_r == sizeof( c->send_buf ) )
			c->send_buf_r = 0;
	}
	
	if( c->drop_after ) 
	{
		goto conn_write_exit;
	}
	else eloop_set_event_enabled( c->write_event, 0 );

	//缓冲buf已经发送完成,所以将读和写重置(这种设置方式,不能存在异步,所以只能全部在event_loop,event_loop相当于单线程)
	c->send_buf_r = 0;
	c->send_buf_w = 0;
	return;
conn_write_exit:
	sess = c->sess;
	_os_printf("close sess:%x\r\n",(int)sess);
	if(sess) {
		//sess->conn	= NULL;
		_os_printf("close sess->close:%x\r\n",(int)sess->closed);
		sess->closed( sess, NULL);
	}
	
	
	//关闭自己的事件和socket
	if(c->read_event)
	{
		eloop_remove_event( c->read_event );
		c->read_event = NULL;
	}
	if(c->write_event)
	{
		eloop_remove_event( c->write_event );
		c->write_event = NULL;
	}

	if( c->fd >= 0 ) 
	{	
		close( c->fd );
		c->fd = -1;
	}
	return;
}

int avail_send_buf( struct conn *c )
{
	if( c->send_buf_r > c->send_buf_w )
		return c->send_buf_r - c->send_buf_w - 1;
	else 
		return sizeof( c->send_buf ) - c->send_buf_w + c->send_buf_r - 1;
}

int send_data( struct conn *c, unsigned char *d, int len )
{
	if( avail_send_buf( c ) < len ) return 1;

	while( --len >= 0 )
	{
		c->send_buf[c->send_buf_w++] = *(d++);
		if( c->send_buf_w == sizeof( c->send_buf ) )
			c->send_buf_w = 0;
	}

	eloop_set_event_enabled( c->write_event,EVENT_F_ENABLED );

	return 0;
}

int tcp_send_pmsg( struct conn *c, struct pmsg *msg, int len )
{
	char buf[512];
	int i, f, totlen;

	if( msg->type != PMSG_RESP ) return -1;
	i = sprintf( buf, "%s %d %s\r\n", msg->proto_id, msg->sl.stat.code,
			msg->sl.stat.reason );
	for( f = 0; f < msg->header_count; ++f )
		i += sprintf( buf + i, "%s: %s\r\n", msg->fields[f].name,
				msg->fields[f].value );
	if( len >= 0 )
	{
		i += sprintf( buf + i, "Content-Length: %d\r\n\r\n", len );
		totlen = i + len;
	} else
	{
		i += sprintf( buf + i, "\r\n" );
		totlen = i;
	}
	if( avail_send_buf( c ) < totlen ) return -1;
	_os_printf("%s\r\n",buf);
	send_data( c, (unsigned char*)buf, i );
	return 0;   
}   	    

static void log_request( struct req *req, int code, int length )
{
	char *ref, *ua;

	ref = get_header( req->req, "referer" );
	ua = get_header( req->req, "user-agent" );
	write_access_log( NULL, (struct sockaddr *)&req->conn->client_addr,
		code, (char *)req->conn->req_buf, length,
		ref ? ref : "-", ua ? ua : "-" );
}



static int handle_unknown( struct req *req )
{
	log_request( req, 501, 0 );
	req->conn->drop_after = 1;
	req->resp = new_pmsg( 256 );
	req->resp->type = PMSG_RESP;
	req->resp->proto_id = add_pmsg_string( req->resp, req->req->proto_id );
	req->resp->sl.stat.code = 501;
	req->resp->sl.stat.reason =
		add_pmsg_string( req->resp, "Not Implemented" );
	copy_headers( req->resp, req->req, "CSeq" );
	tcp_send_pmsg( req->conn, req->resp, -1 );
	return 0;
}

static int  handle_request( struct conn *c )
{
	int ret;
	struct req *req;

	if( c->req_buf[0] == 0 ) return -1;

	if( ( req = malloc( sizeof( struct req ) ) ) == NULL ) return -1;
	req->conn = c;
	req->resp = NULL;
	req->req = new_pmsg( c->req_len );
	req->req->msg_len = c->req_len;
	memcpy( req->req->msg, c->req_buf, req->req->msg_len);
	if( parse_pmsg( req->req ) < 0 )
	{
		free_pmsg( req->req );
		free( req );
		return -1;
	}
	spook_log( SL_VERBOSE, "client request: '%s' '%s' '%s'",
			req->req->sl.req.method, req->req->sl.req.uri,
			req->req->proto_id );

	switch( c->proto )
	{
	case CONN_PROTO_RTSP:
		ret = rtsp_handle_msg( req );
		break;
	default:
		ret = handle_unknown( req );
		break;
	}
	if( ret <= 0 )
	{
		free_pmsg( req->req );
		if( req->resp ) free_pmsg( req->resp );
		free( req );
	}
	return ret < 0 ? -1 : 0;
}

static int parse_client_data( struct conn *c )
{
	char *a, *b;

	switch( c->proto )
	{
	case CONN_PROTO_START:
		if( ( a = strchr( (const char*)c->req_buf, '\n' ) ) )
		{
			*a = 0;
			if( ! ( b = strrchr( (const char*)c->req_buf, ' ' ) ) ) return -1;
			if( ! strncmp( b + 1, "HTTP/", 5 ) )
				c->proto = CONN_PROTO_HTTP;
			else if( ! strncmp( b + 1, "RTSP/", 5 ) )
				c->proto = CONN_PROTO_RTSP;
			else if( ! strncmp( b + 1, "APPO/", 5 ) )
				c->proto = CONN_PROTO_APPO;
			else return -1;
			*a = '\n';
			return 1;
		}
		break;
	case CONN_PROTO_RTSP:
		if( c->req_len < 4 ) return 0;
		if( c->req_buf[0] == '$' )
		{
			spook_log( SL_DEBUG, "interleave_recv\n");
			if( c->req_len < GET_16( c->req_buf + 2 ) + 4 )
				return 0;
			#if 0
			interleave_recv( c, c->req_buf[1], c->req_buf + 4,
						GET_16( c->req_buf + 2 ) );
			#else
			c->req_len -= (GET_16( c->req_buf + 2 ) + 4);
			
			#endif
			/* should really just delete the packet from the
			 * buffer, not the entire buffer */
			//c->req_len = 0;
		} else
		{
			if( ! strstr( (const char*)c->req_buf, (const char*)"\r\n\r\n" ) ) return 0;
			if( handle_request( c ) < 0 ) return -1;
			/* should really just delete the request from the
			 * buffer, not the entire buffer */
			c->req_len = 0;
		}
		break;

	}
	
	return 0;
}

static int unbase64( unsigned char *d, int len, int *remain )
{
	int src = 0, dest = 0, i;
	unsigned char enc[4];

	for(;;)
	{
		for( i = 0; i < 4; ++src )
		{
			if( src >= len )
			{
				if( i > 0 ) memcpy( d + dest, enc, i );
				*remain = i;
				return dest + i;
			}
			if( base64[d[src]] < 0 ) continue;
			enc[i++] = d[src];
		}
		d[dest] = ( ( base64[enc[0]] << 2 ) & 0xFC ) |
				( base64[enc[1]] >> 4 );
		d[dest + 1] = ( ( base64[enc[1]] << 4 ) & 0xF0 ) |
				( base64[enc[2]] >> 2 );
		d[dest + 2] = ( ( base64[enc[2]] << 6 ) & 0xC0 ) |
				base64[enc[3]];
		if( enc[3] == '=' )
		{
			if( enc[2] == '=' ) dest += 1;
			else dest += 2;
		} else dest += 3;
	}
}

#if 0
static unsigned char s_ptemp[300] = {"TEARDOWN rtsp://192.168.1.1:7070/webcam/ RTSP/1.0\r\nCSeq: 6\r\nUser-Agent: Lavf56.40.101\r\nSession: 9B9C9D9E9FA0A1A2A3A4A5A6A7A8A9\r\n\r\n4A5A6A7A8A9\r\n\r\n"};
#endif

	


static void do_read( void *ei, void *d )
{
	_os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	spook_log( SL_DEBUG, "tcp do_read" );
	struct conn *c = (struct conn *)d;
	int ret;
	struct session *sess = NULL;

	for(;;)
	{
		ret = read(c->fd,c->req_buf + c->req_len,sizeof( c->req_buf ) - c->req_len - 1 );

		if( ret <= 0 )
		{
			if( ret < 0 )
			{
				if( get_errno() == EAGAIN ) {
					spook_log( SL_VERBOSE,"get_errno() === %x", get_errno()  );
					return;
				}
				spook_log( SL_VERBOSE,"get_errno() = %x", get_errno()  );
				spook_log( SL_VERBOSE, "closing TCP connection to client due to read error: %s  ret:%d",strerror( get_errno() ) ,ret );	
			} 
			else {
				spook_log( SL_VERBOSE, "client closed TCP connection %x c:%X", c->sess,c );
				/* add by jornny */
                /* should be initialized by NULL */
			}

            spook_log( SL_DEBUG, "free conn");
			//drop_conn( c );


			/* add by jornny */
            /* drop_conn has free c, so can not use c->sess !!!*/
			sess = c->sess;



			//关闭自己的事件和socket
			if(c->read_event)
			{
				eloop_remove_event( c->read_event );
				c->read_event = NULL;
			}
			if(c->write_event)
			{
				eloop_remove_event( c->write_event );
				c->write_event = NULL;
			}

			if( c->fd >= 0 ) 
			{	
				close( c->fd );
				c->fd = -1;
			}
			c->running = 0;
			_os_printf("close sess:%x\r\n",(int)sess);
			if(sess) {
				//sess->conn    = NULL;
				_os_printf("close sess->close:%x\r\n",(int)sess->closed);
				sess->closed( sess, NULL);
			}
			//没有sess,说明连接还没有正式
			else
			{
				_os_printf("%s:%d\n",__FUNCTION__,__LINE__);
				drop_conn( c );
			}


			return;
		}

		c->req_len += ret;

		if( c->req_len == sizeof( c->req_buf ) - 1 )
		{
			spook_log( SL_VERBOSE, "malformed request from client; exceeded maximum size" );
			
			return;
		}


		c->req_buf[c->req_len] = 0;
		while( ( ret = parse_client_data( c ) ) > 0 );
		if( ret < 0 )
		{
			_os_printf("%s:%d\n",__FUNCTION__,__LINE__);
			//drop_conn( c );
			return;
		}
	}
}

extern int g_timeout;
uint8_t sta_mac[8];

extern uint8_t keep_send;
static void do_accept( void *ei, void *d )
{
	spook_log( SL_DEBUG, "tcp do_accept" );	
	struct listener *listener = (struct listener *)d;
	int fd;
	unsigned int i;
	struct sockaddr_in addr;
	struct conn *c;

	_os_printf("%s  %d\n",__func__,__LINE__);
	i = sizeof( addr );
	if( ( fd = accept( listener->fd, (struct sockaddr *)&addr, &i ) ) < 0 )
	{
		spook_log( SL_WARN, "error accepting TCP connection: %s",
				strerror( get_errno() ) );
		return;
	}
	spook_log( SL_VERBOSE, "accepted connection from %s:%d",
			inet_ntoa( addr.sin_addr ), ntohs( addr.sin_port ) );

	if( fcntl( fd, F_SETFL, O_NONBLOCK ) < 0 )
		spook_log( SL_INFO, "error setting O_NONBLOCK on socket: %s",
				strerror( get_errno() ) );

	i = 0;
	if( setsockopt( fd, IPPROTO_TCP/*SOL_SOCKET*/ /*SOL_TCP*/, TCP_NODELAY, &i, sizeof( i ) ) < 0 )
		spook_log( SL_INFO, "error setting TCP_NODELAY on socket: %s",
				strerror( get_errno() ) );


	//int tos = 0xe0;
	//setsockopt(fd, IPPROTO_IP, IP_TOS, (void *)&tos , sizeof(tos));

	#if 0
    /* Find the dhcp Offered Addr based the IP Addr of this Connection  */
    p_dhcp_addr = find_lease_by_yiaddr(addr.sin_addr.s_addr);

	_os_printf("%s  %d\n",__func__,__LINE__);
	if (NULL != p_dhcp_addr)
	{
		for (c = conn_list; c != NULL; c = c->next)
		{
			ret = memcmp(c->chaddr, p_dhcp_addr->chaddr, 6);
			if (0 == ret)
			{
				/* If the same Connection Client Mac Addr has existed, delete the previous Connection */		
				if (g_timeout == 0)
				{
					//parse_client_data(c);		
					if(c->sess) 
						c->sess->closed( c->sess, NULL);
					
				}
				else
				{
					g_timeout = 0;
				}
				
				drop_conn(c);	
				break;//只有一个ip是一样的,直接退出,否则drop_conn会更改conn_list链表,导致这里for出错
			}
			
		}

	}
	#endif
	int keepalive = 1;
	int keepidle = 10;
	int keepintvl = 3;
	int keepcnt = 3;
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));
	setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, (void *)&keepidle, sizeof(keepidle));
	setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepintvl, sizeof(keepintvl));
	setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepcnt, sizeof(keepcnt));

	c = (struct conn *)malloc( sizeof( struct conn ) );
	_os_printf("%s c:%X\n",__FUNCTION__,(int)c);
	c->next = conn_list;
	if( c->next ) c->next->prev = c;
	c->prev = NULL;
	c->fd = fd;
	c->client_addr = addr;
	c->proto = CONN_PROTO_START;
	c->req_len = 0;
	c->req_list = NULL;
	c->read_event = eloop_add_fd( fd, EVENT_READ, EVENT_F_ENABLED, do_read, c );
	c->write_event = eloop_add_fd( fd, EVENT_WRITE, EVENT_F_DISABLED, conn_write, c );
	eloop_set_event_enabled( c->write_event, EVENT_F_DISABLED );
	c->send_buf_r = c->send_buf_w = 0;
	c->drop_after = 0;
	c->proto_state = NULL;
    c->sess = NULL; /* should be initialized by NULL */
	c->running = 1;
	os_event_init(&c->evt);
	conn_list = c;
 
    /* Store the Client MAC Addr */
	//memcpy(c->chaddr, p_dhcp_addr->chaddr, 6);

	//memcpy(sta_mac, p_dhcp_addr->chaddr, 6);
	//_os_printf("STA con[%x %x %x %x %x %x]\n",sta_mac[0],sta_mac[1],sta_mac[2],sta_mac[3],sta_mac[4],sta_mac[5]);

	spook_log( SL_DEBUG, "TCP do_accept %d", fd);
}




static int tcp_listen( int port )
{
	struct sockaddr_in addr;
	struct listener *listener;
	int opt, fd;

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = 0;
	addr.sin_port = htons( port );

	if( ( fd = socket( PF_INET, SOCK_STREAM, 0 ) ) < 0 )
	{
		spook_log( SL_ERR, "error creating listen socket: %s",
				strerror( get_errno() ) );
		return -1;
	}
	opt = 1;
	if( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof( opt ) ) < 0 )
		spook_log( SL_WARN, "ignoring error on setsockopt: %s",
				strerror( get_errno() ) );
	if( bind( fd, (struct sockaddr *)&addr, sizeof( addr ) ) < 0 )
	{
		spook_log( SL_ERR, "unable to bind to tcp socket: %s",
				strerror( get_errno() ) );
		close( fd );
		return -1;
	}
	if( listen( fd, 5 ) < 0 )
	{
		spook_log( SL_ERR,
			"error when attempting to listen on tcp socket: %s",
			strerror( get_errno() ) );
		close( fd );
		return -1;
	}

	listener = (struct listener *)malloc( sizeof( struct listener ) );
	listener->fd = fd;

	eloop_add_fd( fd, EVENT_READ, EVENT_F_ENABLED, do_accept, listener );

	spook_log( SL_INFO, "listening on tcp port %d", port );
	_os_printf("port:%d\tfd:%d\n",port,fd);

	return 0;
}

/********************* GLOBAL CONFIGURATION DIRECTIVES ********************/

int config_port( int port )
{
	if( port <= 0 || port > 65535 )
	{
		spook_log( SL_ERR, "invalid listen port %d", port );
		return -1;
	}

	return tcp_listen( port );		//listen the 7070 port everytime
}
