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

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>

struct converter {
	struct stream_destination *input;
	struct stream *output;
};

static struct stream *stream_list = NULL;
/*
static void get_framerate( struct stream *s, int *fincr, int *fbase )
{
	struct stream_destination *dest =
		(struct stream_destination *)s->private;

	dest->stream->get_framerate( dest->stream, fincr, fbase );
}

static void set_running( struct stream *s, int running )
{
	set_waiting( (struct stream_destination *)s->private, running );
}
*/
void deliver_frame_to_stream( struct frame *f, void *d )
{
	struct stream *s = (struct stream *)d; /* d: jpeg_encoder->output */
	struct stream_destination *dest;

	for( dest = s->dest_list; dest; dest = dest->next )
	{
		/* destination is waitint for frame ?*/
		if( ! dest->waiting ) {
			if(dest->disconnect_frame){
				dest->disconnect_frame( f, dest->d );
			}
			continue;
		} 
		/*rtsp callback: next_live_frame, dest->d: source->track[t]*/
		/*http callback: jpeg_next_frame, dest->d: source->track[t]*/
		dest->process_frame( f, dest->d );
		if(dest->disconnect_frame){			
			dest->disconnect_frame( f, dest->d );
		}
	}
	unref_frame( f );
}

static struct stream_destination *new_dest( struct stream *s,
			frame_deliver_func process_frame, void *d )
{
	struct stream_destination *dest;

	dest = (struct stream_destination *)
			malloc( sizeof( struct stream_destination ) );
	dest->next = s->dest_list;
	dest->prev = NULL;
	if( dest->next ) dest->next->prev = dest;
	s->dest_list = dest;
	dest->stream = s;
	dest->waiting = 0;
	dest->process_frame = process_frame;
	dest->disconnect_frame = NULL;
	dest->d = d;

	return dest;
}


void disconnect_stream(struct stream_destination *dest,frame_disconnect_func disconnect_frame){
	dest->disconnect_frame = disconnect_frame;
}

struct stream_destination *connect_to_stream( const char *name,
		frame_deliver_func process_frame, void *d )
{
	struct stream *s;
	int found_one = 0;

	for( s = stream_list; s; s = s->next )
		if( ! strcmp( s->name, name ) )
			return new_dest( s, process_frame, d );
	if( found_one )
		spook_log( SL_ERR, "unable to convert stream %s", name );
	return NULL;
}

void del_stream( struct stream *s )
{
	if( s->next ) s->next->prev = s->prev;
	if( s->prev ) s->prev->next = s->next;
	else stream_list = s->next;
	free( s );
}

struct stream *new_stream( const char *name, int format, void *d )
{
	struct stream *s;

	s = (struct stream *)malloc( sizeof( struct stream ) );
	
	s->next = stream_list;
	s->prev = NULL;
	if( s->next ) s->next->prev = s;
	stream_list = s;
	s->name = name;
	s->format = format;
	s->dest_list = NULL;
	s->get_framerate = NULL;
	s->set_running = NULL;
	s->private = d;
	return s;
}

void set_waiting( struct stream_destination *dest, int waiting )
{
	struct stream *s = dest->stream;

	/* We call set_running every time a destination starts listening,
	 * or when the last destination stops listening.  It is good to know
	 * when new listeners come so maybe the source can send a keyframe. */

	if( dest->waiting )
	{
		if( waiting ) return; /* no change in status */
		dest->waiting = 0;

		/* see if anybody else is listening */
		for( dest = s->dest_list; dest; dest = dest->next )
			waiting |= dest->waiting;
		if( waiting ) return; /* others are still listening */
	} else
	{
		if( ! waiting ) return; /* no change in status */
		dest->waiting = 1;
	}

	s->set_running( s, waiting );
}
