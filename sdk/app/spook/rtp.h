#ifndef __RTP_H
#define __RTP_H

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

/* These should probably be an enum */
#define	RTP_TRANS_UDP	1
#define RTP_TRANS_INTER	2

#define MAX_INTERLEAVE_CHANNELS	8

struct rtp_endpoint;
struct conn;
struct session;

struct rtp_endpoint {
	struct session *session;
	int payload;
	int max_data_size;
	unsigned int ssrc;
	unsigned int start_timestamp;
	unsigned int last_timestamp;
	unsigned int rtcp_send_timestamp;
	int seqnum;
	int packet_count;
	int octet_count;
	struct event *rtcp_send_event;
	struct timeval last_rtcp_recv;
	int trans_type;
	int sendEnable;
	union {
		struct {
			char sdp_addr[48];
			int sdp_port;
			int rtp_fd;
			struct event *rtp_event;
			int rtcp_fd;
			struct event *rtcp_event;
		} udp;
		struct {
			struct conn *conn;
			int rtp_chan;
			int rtcp_chan;
			int running;
		} inter;
	} trans;
};

typedef int (*get_sdp_func)( struct session *s, char *dest, int *len,
					char *path );
typedef int (*setup_func)( struct session *s, int track );
typedef void (*play_func)( struct session *s, double *start );
typedef void (*pause_func)( struct session *s );
typedef void (*teardown_func)( struct session *s, struct rtp_endpoint *ep );
typedef void (*session_close_func)( struct session *sess );
typedef void (*close_func)( struct session *s, struct rtp_endpoint *ep );
typedef void (*select_func)( struct session *s, struct rtp_endpoint *ep );

#define	MAX_TRACKS	4

struct session {
	struct session *next;
	struct session *prev;
	get_sdp_func get_sdp;
	setup_func setup;
	play_func play;
	pause_func pause;
	close_func closed;
	teardown_func teardown;
	select_func select_close;
	void *private;
	session_close_func control_close;
	void *control_private;
	struct timeval open_time;
	char addr[64];
	struct rtp_endpoint *ep[MAX_TRACKS];
	struct conn *conn; /* add by jornny */
	int connect_type;
};

struct loc_node {
	struct loc_node *next;
	struct loc_node *prev;
	char path[16];
};

typedef struct session *(*open_func)( char *path, void *d );

struct req {
	struct conn *conn;
	struct pmsg *req;
	struct pmsg *resp;
};

struct jpeg_fifo
{
	unsigned char		*buf;
	int 				len;
	unsigned char 		*scan_data;
	int 				scan_data_len;
	unsigned short int	dri_len[100];

	int 				dri_num;
	unsigned int 		items;
	//struct list_head 	list;
	//boolean             *pIsUsing;
};


#define CONN_PROTO_START	0
#define CONN_PROTO_RTSP		1
#define CONN_PROTO_HTTP		2
#define CONN_PROTO_SIP		3
#define CONN_PROTO_APPO		4
#include "osal/event.h"
enum
{
    RTSP_TCP_SEND_FINISH = BIT(0),     // done的时候报错
};
struct conn {
	struct conn *next;
	struct conn *prev;
	int fd;
	int running;
	uint8_t *sendbuf;
	int send_buf_len;
	struct os_event evt;
	struct sockaddr_in client_addr;
	struct event *read_event;
	struct event *write_event;
	int proto;
	unsigned char req_buf[512];
	int req_len;
	struct req *req_list;
	unsigned char send_buf[512];
	int send_buf_r;
	int send_buf_w;
	int drop_after;
	void *proto_state;

	/* add by jornny*/
	struct session *sess;
		
	/* Client Hardware Addr of this Conn*/
	//unsigned char chaddr[6];
};

/* There's probably a better place to put these... */
#define PUT_16(p,v) ((p)[0]=((v)>>8)&0xff,(p)[1]=(v)&0xff)
#define PUT_32(p,v) ((p)[0]=((v)>>24)&0xff,(p)[1]=((v)>>16)&0xff,(p)[2]=((v)>>8)&0xff,(p)[3]=(v)&0xff)
#define GET_16(p) (((p)[0]<<8)|(p)[1])
#define GET_32(p) (((p)[0]<<24)|((p)[1]<<16)|((p)[2]<<8)|(p)[3])

void random_bytes( unsigned char *dest, int len );
void random_id( unsigned char *dest, int len );
struct rtp_endpoint *new_rtp_endpoint( int payload );
int connect_udp_endpoint( struct rtp_endpoint *ep,
		struct in_addr dest_ip, int dest_port, int *our_port );
int avail_send_buf( struct conn *c );
int send_data( struct conn *c, unsigned char *d, int len );
int tcp_send_pmsg( struct conn *c, struct pmsg *msg, int len );
void connect_interleaved_endpoint( struct rtp_endpoint *ep,
		struct conn *conn, int rtp_chan, int rtcp_chan );
void rtsp_conn_disconnect( struct conn *c );
void http_conn_disconnect( struct conn *c );
void interleave_disconnect( struct conn *c, int chan );
int interleave_send( struct conn *conn, int chan, struct iovec *v, int count );
int interleave_recv( struct conn *c, int chan, unsigned char *d, int len );
void interleave_recv_rtcp( struct rtp_endpoint *ep, unsigned char *d, int len );
void del_rtp_endpoint( struct rtp_endpoint *ep );
void update_rtp_timestamp( struct rtp_endpoint *ep, int time_increment );
int send_rtp_packet( struct rtp_endpoint *ep, struct iovec *v, int count,
			unsigned int timestamp, int marker );
void new_rtsp_location( const char *path, char *realm, char *username, char *password,
			open_func open, void *private );
struct session *new_session(void);
void del_session( struct session *sess );
int print_session_list( char *s, int maxlen );
int http_handle_msg( struct req *req );
int rtsp_handle_msg( struct req *req );
void send_rtcp(uint8_t *data);
int new_rtcp_send( struct rtp_endpoint *ep, void *d );
unsigned char * get_send_rtp_packet_head(struct iovec *v ,int count,char *send_buf);
int send_rtp_packet_more( struct rtp_endpoint *ep, unsigned char *sendbuf, int sendLen,int times );
int set_send_rtp_packet_head( struct rtp_endpoint *ep, struct iovec *v, int count,unsigned int timestamp, int marker,unsigned char *send_buf );
#endif


