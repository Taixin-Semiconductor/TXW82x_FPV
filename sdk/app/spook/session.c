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
//#include "sys_config.h"
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

static struct session *sess_list = NULL;

struct rtp_media *new_rtp_media( rtp_media_get_sdp_func get_sdp,
	rtp_media_get_payload_func get_payload, rtp_media_frame_func frame,
	rtp_media_send_func send, void *private )
{
	struct rtp_media *m;

	if( ! ( m = (struct rtp_media *)malloc( sizeof( *m ) ) ) )
		return NULL;
	m->get_sdp = get_sdp;
	m->get_payload = get_payload;
	m->frame = frame;
	m->send = send;
	m->private = private;
	m->rtcp_send = NULL;
	return m;
}

	struct rtp_media *new_rtp_rtcp_media( rtp_media_get_sdp_func get_sdp,
	rtp_media_get_payload_func get_payload, rtp_media_frame_func frame,
	rtp_media_send_func send,rtp_media_send_func rtcp_send, void *private )
{
	struct rtp_media *m;

	if( ! ( m = (struct rtp_media *)malloc( sizeof( *m ) ) ) )
		return NULL;
	m->get_sdp = get_sdp;
	m->get_payload = get_payload;
	m->frame = frame;
	m->send = send;
	m->rtcp_send = rtcp_send;
	m->private = private;
	return m;
}

void del_session( struct session *sess )
{
	if( sess->next ) sess->next->prev = sess->prev;
	if( sess->prev ) sess->prev->next = sess->next;
	else sess_list = sess->next;
	if( sess->control_close ) sess->control_close( sess ); /* callback: rtsp_session_close */
	if(sess->conn)
	{
		//有点绕,判断conn是否正常,正常情况下,两个sess是一样的,则清空,说明这个不是从do_read去关闭,是异常关闭
		_os_printf("%s:%d had conn\n",__FUNCTION__,__LINE__);
		if(sess->conn->sess == sess)
		{
			sess->conn->sess = NULL;
		}
	}
	_os_printf("%s session:%X\n",__FUNCTION__,(int)sess);
	 
	free( sess );
}

void resetnet_delsession(void)
{
	struct session *sess;
	for(sess = sess_list; sess != NULL; sess = sess->next) {
		sess->conn->sess = NULL;
		sess->teardown( sess, NULL );//live_teardown
	}
}

struct session *new_session(void)
{
	struct session *sess;
	int i;

	sess = (struct session *)malloc( sizeof( struct session ) );
	sess->next = sess_list;
	sess->prev = NULL;
	if( sess->next ) sess->next->prev = sess;
	sess_list = sess;
	sess->get_sdp = NULL;
	sess->setup = NULL;
	sess->play = NULL;
	sess->pause = NULL;
	sess->private = NULL;
	sess->control_private = NULL;
	sess->control_close = NULL;
	sess->conn    = NULL;
	gettimeofday( &sess->open_time, NULL );
	strcpy( sess->addr, "(unconnected)" );
	for( i = 0; i < MAX_TRACKS; ++i ) sess->ep[i] = NULL;
	return sess;
}

int print_session_list( char *s, int maxlen )
{
	struct session *sess;
	int i = 0;

	for( sess = sess_list; sess; sess = sess->next )
		i += sprintf( s + i, "%s %ld\n",
					sess->addr, sess->open_time.tv_sec );
	return i;
}
