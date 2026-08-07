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
//#include <stdio.h>
//#include <fcntl.h>
#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>
#include <spook_config.h>
#include <csi_kernel.h>
#include "encoder-audio.h"

 
//需要修改
static void get_framerate( struct stream *s, int *fincr, int *fbase )
{
	/*struct jpeg_encoder *en = (struct jpeg_encoder *)s->private;*/

    spook_log( SL_DEBUG, "jpeg get_framerate" );
	*fincr = JPEG_FRAMEINC;
	*fbase = 25;//JPEG_FRAMERATE-4;
}

static void set_running( struct stream *s, int running )
{
}



/************************ CONFIGURATION DIRECTIVES ************************/

static void *start_block(void)
{
	struct audio_encoder *en;

	en = (struct audio_encoder *)malloc( sizeof( struct audio_encoder ) );
	en->output = NULL;

	return en;
}

static void get_back_audio_frame( struct frame *f, void *d )
{
	struct audio_encoder *en = (struct audio_encoder *)d;
	_os_printf("-");
	//exchange_frame( en->ex, new_frame() );
	deliver_frame_to_stream( f, en->output );
}




static int end_block( void *d )
{
	struct audio_encoder *en = (struct audio_encoder *)d;
	//int i;

	if( ! en->output )
	{
		spook_log( SL_ERR, "jpeg: missing output stream name" );
		return -1;
	}
 
	en->ex = new_exchanger_audio( 0, get_back_audio_frame, en );

	exchange_frame( en->ex, new_audio_frame() );

 
	
	return 0;
}

static int set_output( const char *name, void *d )
{
	struct audio_encoder *en = (struct audio_encoder *)d;

    spook_log( SL_DEBUG, "audio set_output" );
	en->output = new_stream( name, FORMAT_AUDIO, en );
	if( ! en->output )
	{
		spook_log( SL_ERR, "jpeg: unable to create stream \"%s\"", name );
		return -1;
	}
	en->output->get_framerate = get_framerate;
	en->output->set_running = set_running;
	
	return 0;
}


void *get_audio_ex(struct audio_encoder *en)
{
	return en->ex;
}


void * rtp_audio_init_ret(void)
{
	struct audio_encoder *en;
	
	en = start_block();
	set_output(AUDIO_ENCODER_NAME, en);
	end_block(en);
		
	return en->ex;
}

void * file_audio_init_ret(void)
{
	struct audio_encoder *en;
	
	en = start_block();
	set_output(FILE_JPEG_ENCODER_NAME, en);
	end_block(en);
		
	return en->ex;
}


void *rtsp_audio_encode_init(const char *encode_name)
{
	struct audio_encoder *en;
	en = start_block();
	set_output(encode_name, en);
	end_block(en);		
	return en->ex;
}


