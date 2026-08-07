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

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>
#include <spook_config.h>
#include <csi_kernel.h>
#include "encoder-jpeg.h" 
  
extern volatile uint8_t framerate_c;

static void get_framerate( struct stream *s, int *fincr, int *fbase )
{
	/*struct jpeg_encoder *en = (struct jpeg_encoder *)s->private;*/

    spook_log( SL_DEBUG, "h264 get_framerate" );
	*fincr = H264_FRAMEINC;
	*fbase = 25;//JPEG_FRAMERATE-4;
}

static void set_running( struct stream *s, int running )
{
	struct h264_encoder *en = (struct h264_encoder *)s->private;

    spook_log( SL_DEBUG, "h264 set_running: %d", running);

	en->running = running;
	en->ex->scan_ready = running;
}


/************************ CONFIGURATION DIRECTIVES ************************/

static void *start_block(void)
{
	struct h264_encoder *en;

	en = (struct h264_encoder *)malloc( sizeof( struct h264_encoder ) );
	en->output = NULL;
	en->running = 0;

	return en;
}

static void get_back_frame( struct frame *f, void *d )
{
	struct h264_encoder *en = (struct h264_encoder *)d;
	//exchange_frame( en->ex, new_frame() );
	deliver_frame_to_stream( f, en->output );
}


static int end_block( void *d )
{
	struct h264_encoder *en = (struct h264_encoder *)d;
	//int i;

	if( ! en->output )
	{
		spook_log( SL_ERR, "h264: missing output stream name" );
		return -1;
	}

	en->ex = new_exchanger( EXCHANGER_SLOT_SIZE, get_back_frame, en );
	en->ex->ready = 0;
	en->ex->scan_ready = 0;
	//for( i = 0; i < EXCHANGER_SLOT_SIZE; ++i ) 
	exchange_frame( en->ex, new_frame() );
	
	return 0;
}

static int set_output( const char *name, void *d )
{
	struct h264_encoder *en = (struct h264_encoder *)d;

    spook_log( SL_DEBUG, "h264 set_output" );
	en->output = new_stream( name, FORMAT_H264, en );
	if( ! en->output )
	{
		spook_log( SL_ERR, "h264: unable to create stream \"%s\"", name );
		return -1;
	}
	en->output->get_framerate = get_framerate;
	en->output->set_running = set_running;
	
	return 0;
}


void *get_video_ex_h264(struct h264_encoder *en)
{
	return en->ex;
}


void * file_h264_init_ret(void)
{
	struct h264_encoder *en;
	
	en = start_block();
	set_output(FILE_H264_ENCODER_NAME, en);
	end_block(en);
		
	return en->ex;
}


void *h264_encode_init(const char *encode_name)
{
	struct h264_encoder *en;
	en = start_block();
	set_output(encode_name, en);
	end_block(en);		
	return en->ex;
}
