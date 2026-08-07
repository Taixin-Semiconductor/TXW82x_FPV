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
#include <pmsg.h>
#include <rtp.h>
//#include "common.h"
#include "osal/string.h"



void write_access_log( char *path, struct sockaddr *addr, int code, char *req,
		int length, char *referer, char *user_agent );

struct rtsp_session {
	struct rtsp_session *next;
	struct rtsp_session *prev;
	char id[32];
	struct session *sess;
};

struct rtsp_location {
	struct loc_node node;
	char realm[128];
	char username[128];
	char password[128];
	open_func open;
	void *private;
};

struct rtsp_conn {
	struct {
		struct rtp_endpoint *ep;
		int rtcp;
	} ichan[MAX_INTERLEAVE_CHANNELS];
};

static struct rtsp_location *rtsp_loc_list = NULL;
static struct rtsp_session *sess_list = NULL;
static char *get_local_path( char *path )
{
	char *c;
	static char *root = "/";

	if( os_strncasecmp( path, "rtsp://", 7 ) ) return NULL;
	for( c = path + 7; *c != '/'; ++c ) if( *c == 0 ) return root;
	return c;
}

static struct loc_node *node_find_location( struct loc_node *list,
						char *path, int len )
{
	struct loc_node *loc;

	for( loc = list; loc; loc = loc->next )
	{
		//_os_printf("%s path:%s\tloc:%s\tlen:%d\tres:%d\t%X\t%d\n",__FUNCTION__,path,loc->path,len,loc->path[len],'/',strncmp( path, loc->path, strlen( loc->path ) ));
		//if( (! strncmp( path, loc->path, strlen( loc->path ) )) && ( loc->path[len] == '/' || loc->path[len] == 0 ) )
		if( ! strncmp( path, loc->path, strlen( loc->path ) ))
		{
			return loc;
		}
	}
	return NULL;
}

static struct rtsp_location *find_rtsp_location( char *uri, char *base, int *track )
{
	char *path, *c, *end;
	int len;

	if( ! ( path = get_local_path( uri ) ) ) return NULL;
	len = strlen( path );

	if( track )
	{
		*track = -1;
		if( ( c = strrchr( path, '/' ) ) &&
				! strncmp( c, "/track", 6 ) )
		{
			*track = strtol( c + 6, &end, 10 );
			if( ! *end ) len -= strlen( c );
		}
	}

	if( len > 1 && path[len - 1] == '/' ) --len;

	if( base )
	{
		strncpy( base, path, len );
		base[len] = 0;
	}

	return (struct rtsp_location *)node_find_location(
			(struct loc_node *)rtsp_loc_list, path, len );
}

static void init_location( struct loc_node *node, const char *path,
				struct loc_node **list )
{
	node->next = *list;
	node->prev = NULL;
	if( node->next ) node->next->prev = node;
	*list = node;
	strcpy( node->path, path );
}

void new_rtsp_location( const char *path, char *realm, char *username, char *password,
			open_func open, void *private )
{
	struct rtsp_location *loc;

	loc = (struct rtsp_location *)malloc( sizeof( struct rtsp_location ) );
	init_location( (struct loc_node *)loc, path,
			(struct loc_node **)&rtsp_loc_list );
	if( realm ) strcpy( loc->realm, realm );
	else loc->realm[0] = 0;
	if( username ) strcpy( loc->username, username );
	else loc->username[0] = 0;
	if( password ) strcpy( loc->password, password );
	else loc->password[0] = 0;
	loc->open = open;
	loc->private = private;
}

static void rtsp_session_close( struct session *sess )
{
	struct rtsp_session *rs = (struct rtsp_session *)sess->control_private;

	spook_log( SL_DEBUG, "freeing session %s", rs->id );
	if( rs->next ) rs->next->prev = rs->prev;
	if( rs->prev ) rs->prev->next = rs->next;
	else sess_list = rs->next;
	free( rs );
}

static struct rtsp_session *new_rtsp_session( struct session *sess )
{
	struct rtsp_session *rs;

	rs = (struct rtsp_session *)malloc( sizeof( struct rtsp_session ) );
	rs->next = sess_list;
	rs->prev = NULL;
	if( rs->next ) rs->next->prev = rs;
	sess_list = rs;
	random_id( (unsigned char *)rs->id, 30 );
	rs->sess = sess;
	sess->control_private = rs;
	sess->control_close = rtsp_session_close;
	return rs;
}

static struct rtsp_session *get_session( char *id )
{
	struct rtsp_session *rs;

	if( ! id ) return NULL;
	for( rs = sess_list; rs; rs = rs->next )
		if( ! strcmp( rs->id, id ) ) break;
	return rs;
}

void rtsp_conn_disconnect( struct conn *c )
{
	struct rtsp_conn *rc = (struct rtsp_conn *)c->proto_state;
	int i;

	for( i = 0; i < MAX_INTERLEAVE_CHANNELS; ++i )
		if( rc->ichan[i].ep && ! rc->ichan[i].rtcp )
			rc->ichan[i].ep->session->closed(
					rc->ichan[i].ep->session,
					rc->ichan[i].ep );
	free( rc );
}

void interleave_disconnect( struct conn *c, int chan )
{
	struct rtsp_conn *rc = (struct rtsp_conn *)c->proto_state;

	rc->ichan[chan].ep = NULL;
}

int interleave_recv( struct conn *c, int chan, unsigned char *d, int len )
{
	struct rtsp_conn *rc = (struct rtsp_conn *)c->proto_state;

	if( chan >= MAX_INTERLEAVE_CHANNELS || ! rc->ichan[chan].ep ) return -1;
	if( rc->ichan[chan].rtcp )
		interleave_recv_rtcp( rc->ichan[chan].ep, d, len );
	return 0;
}

int interleave_send( struct conn *c, int chan, struct iovec *v, int count )
{
	unsigned char head[4];
	int len = 0, i; 

	for( i = 0; i < count; ++i ) len += v[i].iov_len;
	if( avail_send_buf( c ) < len + 4 ) return 1;
	head[0] = '$';
	head[1] = chan;
	PUT_16( head + 2, len );
	send_data( c, head, 4 );
	for( i = 0; i < count; ++i )
		send_data( c, v[i].iov_base, v[i].iov_len );

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

static int rtsp_create_reply( struct req *req, int code, char *reason )
{
	if( ! ( req->resp = new_pmsg( 512 ) ) ) return -1;
	req->resp->type = PMSG_RESP;
	req->resp->proto_id = add_pmsg_string( req->resp, "RTSP/1.0" );
	req->resp->sl.stat.code = code;
	req->resp->sl.stat.reason = add_pmsg_string( req->resp, reason );
	copy_headers( req->resp, req->req, "CSeq" );
	return 0;
}

static void rtsp_send_error( struct req *req, int code, char *reason )
{
	log_request( req, code, 0 );
	rtsp_create_reply( req, code, reason );
	tcp_send_pmsg( req->conn, req->resp, -1 );
}
/*
static int rtsp_verify_auth( struct req *req, char *realm,
				char *username, char *password )
{
	int ret = check_digest_response( req->req, realm, username, password );

	if( ret > 0 ) return 0;

	log_request( req, 401, 0 );
	rtsp_create_reply( req, 401, "Unauthorized" );
	add_digest_challenge( req->resp, realm, ret == 0 ? 1 : 0 );
	tcp_send_pmsg( req->conn, req->resp, -1 );
	return -1;
}
*/
static int handle_OPTIONS( struct req *req )
{
	rtsp_create_reply( req, 200, "OK" );
	add_header( req->resp, "Public",
			"DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE" );
	log_request( req, 200, 0 );
	tcp_send_pmsg( req->conn, req->resp, -1 );
	return 0;
}

static int handle_DESCRIBE( struct req *req )
{
	char sdp[512+128], path[64], hdr[128];
	int sdp_len;
	struct rtsp_location *loc;
	struct session *sess;

	spook_log( SL_DEBUG, "describing streams under '%s'", req->req->sl.req.uri );
	//_os_printf("%s:%d url:%s\n",__FUNCTION__,__LINE__,req->req->sl.req.uri);

	if( ! ( loc = find_rtsp_location( req->req->sl.req.uri,
						path, NULL ) ) ||
			! ( sess = loc->open( path, loc->private ) ) )
	{
		_os_printf("no found uri:%s\tloc:%X\n",req->req->sl.req.uri,(int)loc);
		rtsp_send_error( req, 404, "Not Found" );
		return 0;
	}
	
	/*
	if( loc->realm[0] && rtsp_verify_auth( req, loc->realm,
				loc->username, loc->password ) < 0 )
		return 0;*/

	sdp_len = sizeof( sdp ) - 2;
	if( sess->get_sdp( sess, sdp, &sdp_len, path ) > 0 )
	{
		
		rtsp_create_reply( req, 200, "OK" );
		sprintf( hdr, "%s/", req->req->sl.req.uri );
		//add_header( req->resp, "Content-Base", hdr );
		add_header( req->resp, "Content-Type", "application/sdp" );
		add_header( req->resp, "Cache-Control", "no-cache" );
		add_header( req->resp, "Server", "Hisilicon RTSP Streaming Media Server/1.0.0" );
		
		log_request( req, 200, sdp_len );

		
		if( tcp_send_pmsg( req->conn, req->resp, sdp_len ) >= 0 )
			send_data( req->conn, (unsigned char *)sdp, sdp_len );
	} else
	{
		_os_printf("sdp:%x  %x\n",(int)sess->get_sdp,(int)sess->get_sdp( sess, sdp, &sdp_len, path));
		rtsp_send_error( req, 404, "Not Found" );
	}

	sess->select_close(sess, NULL);

	return 0;
}

static int rtsp_udp_setup( struct session *s, int track,
				struct req *req, char *t )
{
	char *p, *end, trans[128];
	int cport, server_port;

	if( ! ( p = strstr( t, "client_port" ) ) || *(p + 11) != '=' )
		return -1;
	cport = strtol( p + 12, &end, 10 );
	if( end == p + 12 ) return -1;
	spook_log( SL_DEBUG, "client requested UDP port %d", cport );

	if( connect_udp_endpoint( s->ep[track], req->conn->client_addr.sin_addr,
				cport, &server_port ) < 0 )
		return -1;

	spook_log( SL_VERBOSE, "our port is %d, client port is %d",
			server_port, cport );

	sprintf( trans, "RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d",
		cport, cport + 1, server_port, server_port + 1 );
	add_header( req->resp, "Transport", trans );
	return 0;
}

static int rtsp_interleave_setup( struct session *s, int track,
				struct req *req, char *t )
{
	struct rtsp_conn *rc = (struct rtsp_conn *)req->conn->proto_state;
	char *p, *end, trans[128];
	int rtp_chan = -1, rtcp_chan = -1, i;

	if( ( p = strstr( t, "interleaved" ) ) )
	{
		if( *(p + 11) != '=' ) return -1;
		rtp_chan = strtol( p + 12, &end, 10 );
		rtcp_chan = rtp_chan + 1; // XXX make better parser
		if( end == p + 12 ) return -1;
		if( rtp_chan < 0 || rtcp_chan < 0 ||
				rtp_chan >= MAX_INTERLEAVE_CHANNELS ||
				rtcp_chan >= MAX_INTERLEAVE_CHANNELS )
			return -1;
		spook_log( SL_VERBOSE, "requested interleave channels %d-%d",
				rtp_chan, rtcp_chan );
		if( rc && ( rc->ichan[rtp_chan].ep ||
					rc->ichan[rtcp_chan].ep ) )
			return -1;
	} else
	{
		spook_log( SL_VERBOSE, "requested any interleave channel" );
		if( rc )
		{
			for( i = 0; i < MAX_INTERLEAVE_CHANNELS; i += 2 )
				if( ! rc->ichan[i].ep && ! rc->ichan[i + 1].ep )
					break;
			if( i >= MAX_INTERLEAVE_CHANNELS ) return -1;
			rtp_chan = i;
			rtcp_chan = i + 1;
		} else
		{
			rtp_chan = 0;
			rtcp_chan = 1;
		}
	}

	if( ! rc )
	{
		rc = (struct rtsp_conn *)malloc( sizeof( struct rtsp_conn ) );
		if( ! rc )
		{
			spook_log( SL_ERR,
				"out of memory on malloc rtsp_conn" );
			return -1;
		}
		for( i = 0; i < MAX_INTERLEAVE_CHANNELS; ++i )
			rc->ichan[i].ep = NULL;
		req->conn->proto_state = rc;
	}

	rc->ichan[rtp_chan].ep = s->ep[track];
	rc->ichan[rtp_chan].rtcp = 0;
	rc->ichan[rtcp_chan].ep = s->ep[track];
	rc->ichan[rtcp_chan].rtcp = 1;

	connect_interleaved_endpoint( s->ep[track], req->conn,
						rtp_chan, rtcp_chan );

	sprintf( trans, "RTP/AVP/TCP;unicast;interleaved=%d-%d",
			rtp_chan, rtcp_chan );
	add_header( req->resp, "Transport", trans );
	return 0;
}

static int handle_SETUP( struct req *req )
{
	char *t, *sh, path[256];
	int track, ret;
	struct session *s;
	struct rtsp_session *rs = NULL;
	struct rtsp_location *loc;

	if( ! ( loc = find_rtsp_location( req->req->sl.req.uri,
					path, &track ) ) ||
			track < 0 || track >= MAX_TRACKS )
	{
		rtsp_send_error( req, 404, "Not Found" );
		return 0;
	}
	/*
	if( loc->realm[0] && rtsp_verify_auth( req, loc->realm,
				loc->username, loc->password ) < 0 )
		return 0;*/

	if( ! ( t = get_header( req->req, "Transport" ) ) )
	{
		_os_printf("%s %d\r\n",__func__,__LINE__);	
		// XXX better error reply
		rtsp_send_error( req, 461, "Unspecified Transport" );
		return 0;
	}

	if( ! ( sh = get_header( req->req, "Session" ) ) )
	{
		/*callback: live_open */
		if( ! ( s = loc->open( path, loc->private ) ) )
		{
			rtsp_send_error( req, 404, "Not Found" );
			return 0;
		}
		sprintf( s->addr, "IP4 %s",
				inet_ntoa( req->conn->client_addr.sin_addr ) );
	} else if( ! ( rs = get_session( sh ) ) )
	{
		rtsp_send_error( req, 454, "Session Not Found" );
		return 0;
	} else s = rs->sess;

	if( s->ep[track] )
	{
		_os_printf("%s %d\r\n",__func__,__LINE__);
		// XXX better error reply
		rtsp_send_error( req, 461, "Unsupported Transport" );
		return 0;
	}

	/* callback: live_setup */
	if( s->setup( s, track ) < 0 )
	{
		rtsp_send_error( req, 404, "Not Found" );
		if( ! rs ) s->select_close(s, NULL);

		return 0;
	}

	spook_log( SL_DEBUG, "setting up RTP (%s)", t );

	rtsp_create_reply( req, 200, "OK" );

	if( ! strncasecmp( t, "RTP/AVP/TCP", 11 ) ) 
	{
//		ret = -1;
		ret = rtsp_interleave_setup( s, track, req, t );
	}
	else if( ( ! strncasecmp( t, "RTP/AVP", 7 ) && t[7] != '/' ) ||
			! strncasecmp( t, "RTP/AVP/UDP", 11 ) )
		ret = rtsp_udp_setup( s, track, req, t );
	else ret = -1;

	if( ret < 0 )
	{
		free_pmsg( req->resp );
		_os_printf("%s %d\r\n",__func__,__LINE__);
		rtsp_send_error( req, 461, "Unsupported Transport" );
		s->select_close(s, s->ep[track]);
	} else
	{
		if( ! rs ) rs = new_rtsp_session( s );
		add_header( req->resp, "Session", rs->id );
		log_request( req, 200, 0 );
		tcp_send_pmsg( req->conn, req->resp, -1 );

		/* add by jornny */
		rs->sess->conn = req->conn;
		req->conn->sess = s; 
	}

	return 0;
}

static int handle_PLAY( struct req *req )
{
	struct rtsp_session *rs;
	double start;
	int have_start = 0, i, p, ret;
	char *range, hdr[512];
	if( ! ( rs = get_session( get_header( req->req, "Session" ) ) ) )
	{
		rtsp_send_error( req, 454, "Session Not Found" );
		return 0;
	}

	range = get_header( req->req, "Range" );
	if( range && sscanf( range, "npt=%lf-", &start ) == 1 )
	{
		have_start = 1;
		spook_log( SL_VERBOSE,
			"starting streaming for session %s at position %.4f",
			rs->id, start );
	} else spook_log( SL_VERBOSE,
			"starting streaming for session %s", rs->id );

	/* callback:live_play( struct session *s, double *start ) */
	rs->sess->play( rs->sess, have_start ? &start : NULL );

	rtsp_create_reply( req, 200, "OK" );
	add_header( req->resp, "Session", rs->id );
	if( have_start && start >= 0 )
	{
		spook_log( SL_DEBUG, "backend seeked to %.4f", start );
		sprintf( hdr, "npt=%.4f-", start );
		add_header( req->resp, "Range", hdr );
		p = 0;
		for( i = 0; i < MAX_TRACKS; ++i )
		{
			if( ! rs->sess->ep[i] ) continue;
			ret = snprintf( hdr + p, sizeof( hdr ) - p,
				"url=%s/track%d;seq=%d;rtptime=%u,",
				req->req->sl.req.uri, i,
				rs->sess->ep[i]->seqnum,
				rs->sess->ep[i]->last_timestamp );
			if( ret >= sizeof( hdr ) - p )
			{
				p = -1;
				break;
			}
			p += ret;
		}
		if( p > 0 )
		{
			hdr[p - 1] = 0; /* Kill last comma */
			add_header( req->resp, "RTP-Info", hdr );
		}
	}
	log_request( req, 200, 0 );
	tcp_send_pmsg( req->conn, req->resp, -1 );
#ifdef FLY_CONTROL_RECEIVE
#endif
	/* to test audio adc when transporting picture */
//	extern void auadc_init(void);
//	auadc_init();

	return 0;
}

static int handle_PAUSE( struct req *req )
{
	struct rtsp_session *rs;

	if( ! ( rs = get_session( get_header( req->req, "Session" ) ) ) )
	{
		rtsp_send_error( req, 454, "Session Not Found" );
		return 0;
	}

	if( ! rs->sess->pause )
	{
		rtsp_send_error( req, 501, "Not Implemented" );
		return 0;
	}

	spook_log( SL_VERBOSE, "pausing session %s", rs->id );

	rs->sess->pause( rs->sess );

	rtsp_create_reply( req, 200, "OK" );
	add_header( req->resp, "Session", rs->id );
	log_request( req, 200, 0 );
	tcp_send_pmsg( req->conn, req->resp, -1 );
	return 0;
}

static int handle_TEARDOWN( struct req *req )
{
	struct rtsp_location *loc;
	struct rtsp_session *rs;
	int track;
	_os_printf("handle_teardown\r\n");
	if( ! ( loc = find_rtsp_location( req->req->sl.req.uri,
						NULL, &track ) ) ||
			track >= MAX_TRACKS )
	{
		rtsp_send_error( req, 404, "Not Found" );
		return 0;
	}

	if( ! ( rs = get_session( get_header( req->req, "Session" ) ) ) )
	{
		rtsp_send_error( req, 454, "Session Not Found" );
		return 0;
	}
	spook_log( SL_VERBOSE, "tearing down %s in session %s",
			req->req->sl.req.uri, rs->id );

	rtsp_create_reply( req, 200, "OK" );
	add_header( req->resp, "Session", rs->id );

	/* This might destroy the session, so do it after creating the reply 
	* callback function: live_teardown */
	rs->sess->closed( rs->sess, track < 0 ? NULL : rs->sess->ep[track] );

	/* add by jornny */
	//req->conn->sess = NULL; 

	log_request( req, 200, 0 );
	tcp_send_pmsg( req->conn, req->resp, -1 );
	return 0;
}

static int handle_unknown( struct req *req )
{
	rtsp_send_error( req, 501, "Not Implemented" );
	return 0;
}

uint8 spook_handle_open = 1;

int rtsp_handle_msg( struct req *req )
{
	int ret;
	_os_printf("===========%s=============\r\n",req->req->sl.req.method);

	if(spook_handle_open == 0){
		ret = handle_unknown( req );
		return ret;		
	}
	
	if( ! os_strcasecmp( req->req->sl.req.method, "OPTIONS" ) )
		ret = handle_OPTIONS( req );
	else if( ! os_strcasecmp( req->req->sl.req.method, "DESCRIBE" ) )
		ret = handle_DESCRIBE( req );
	else if( ! os_strcasecmp( req->req->sl.req.method, "SETUP" ) )
		ret = handle_SETUP( req );
	else if( ! os_strcasecmp( req->req->sl.req.method, "PLAY" ) )
		ret = handle_PLAY( req );
	else if( ! os_strcasecmp( req->req->sl.req.method, "PAUSE" ) )
		ret = handle_PAUSE( req );
	else if( ! os_strcasecmp( req->req->sl.req.method, "TEARDOWN" ) )
		ret = handle_TEARDOWN( req );
	else ret = handle_unknown( req );
	return ret;
}

