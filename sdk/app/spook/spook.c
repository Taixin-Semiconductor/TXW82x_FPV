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
#include <rtp.h>
#include "spook_config.h"
#include "spook.h"
#include "sys_config.h"
#include "encoder-audio.h"
#include "encoder-jpeg.h"

unsigned long random_key;
static int init_random(void)
{
	random_key = 0x12345678;
	return 0;
}

uint32_t get_random(void)
{ 
	return random_key;
}

void random_bytes( unsigned char *dest, int len )
{
	int i;

	for( i = 0; i < len; ++i )
		dest[i] = (random_key++) & 0xff;
}

void random_id( unsigned char *dest, int len )
{
	int i;

	for( i = 0; i < len / 2; ++i )
		sprintf( (char *)(dest + i * 2), "%02X",
				(unsigned int)( (random_key++) & 0xff ) );
	dest[len] = 0;
}

void global_init(void)
{
	config_port(SPOOK_PORT);
}

extern void spook_send_thread(void *d);
extern int web_init(void);
extern int live_init(const rtp_name *jpg_name);

const rtp_name live_jpg = {
	.video_encode_name		= JPG_ENCODER_NAME,
	.audio_encode_name		= AUDIO_AAC_ENCODER_NAME_JPG,
	.path            		= "/webcam",
};

const rtp_name live_h264 = {
	.video_encode_name		= H264_ENCODER_NAME,
	.audio_encode_name		= AUDIO_AAC_ENCODER_NAME_H264,
	.path            		= "/h264",
};

const rtp_name live_h264_rec = {
	.video_encode_name		= H264_ENCODER_NAME_REC,
	.audio_encode_name		= AUDIO_AAC_ENCODER_NAME_REC,
	.path			 		= "/loop/RECA/",
};

const rtp_name live_custom = {
	.video_encode_name		= JPG_ENCODER_NAME_CUSTOM,
	.audio_encode_name		= AUDIO_AAC_ENCODER_NAME_CUSTOM,
	.path            		= "/custom",
};

extern void rtsp_mjpeg_set_path(const rtp_name *rtsp);
extern void rtsp_h264_set_path(const rtp_name *rtsp);
extern void rtsp_custom_set_path(const rtp_name *rtsp);

static void spook_thread(void *d)
{
	init_random();
	global_init();

	rtsp_mjpeg_set_path(&live_jpg);
	rtsp_h264_set_path(&live_h264);
	rtsp_custom_set_path(&live_custom);
}

void spook_init(void)
{
	//thread_create(spook_thread, 0, TASK_SPOOK_PRIO, 0, 110*1024, "spook");
	spook_thread(0);
}


