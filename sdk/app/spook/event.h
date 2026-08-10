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

#include "lwip\sockets.h"
#include "list.h"
#include "lib/net/eloop/eloop.h"

//#include "mutex_os.h"


#ifndef _EVENT_H
#define _EVENT_H
#define EVENT_TIME		1
#define EVENT_FD		2
#define EVENT_ALWAYS		3
#define EVENT_SOFT_QUEUE	4
#define EVENT_F_ENABLED		1
#define EVENT_F_REMOVE		2
#define EVENT_F_ONESHOT		4
#define EVENT_F_RUNNING		8

struct event_info;

typedef void (*callback)( struct event_info *e, void *d );










struct event_info {
	struct event *e;
	int type;
	void *data;
};

int time_diff( eloop_time_ref *tr_start, eloop_time_ref *tr_end );
int time_ago( eloop_time_ref *tr );

void time_now( eloop_time_ref *tr );
void time_add( eloop_time_ref *tr, int msec );
void time_future( eloop_time_ref *tr, int msec );

struct soft_queue *new_soft_queue( int length );
void *get_next_event( struct soft_queue *sq );
int soft_queue_add( struct soft_queue *sq, void *d );
struct event *add_timer_event( int msec, unsigned int flags, callback f, void *d );
struct event *add_alarm_event( eloop_time_ref *t, unsigned int flags, callback f, void *d );
void resched_event( struct event *e, eloop_time_ref *t );
struct event *add_fd_event( int fd, int write, unsigned int flags, callback f, void *d );
struct event *add_always_event( unsigned int flags, callback f, void *d );
struct event *add_softqueue_event( struct soft_queue *sq, unsigned int flags,
		callback f, void *d );
void remove_event( struct event *e );
void set_event_interval( struct event *e, int msec );
void set_event_enabled( struct event *e, int enabled );
int get_event_enabled( struct event *e );
void exit_event_loop(void);
void event_loop( int single );

#endif /* EVENT_H */
