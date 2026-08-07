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

const rtp_name live_dvp = {
	.video_encode_name     = JPG_ENCODER_NAME,
	.audio_encode_name      = AUDIO_AAC_ENCODER_NAME,
	.path            = "/webcam",
};


const rtp_name live_h264 = {
	.video_encode_name     = H264_ENCODER_NAME,
	.audio_encode_name      = AUDIO_AAC_ENCODER_NAME,
	.path            = "/h264",
	
};


const rtp_name live_h264_2 = {
	.video_encode_name     = H264_ENCODER_NAME,
	.audio_encode_name      = AUDIO_AAC_ENCODER_NAME,
	.path			 = "/loop/RECA/",
};

const rtp_name live_custom = {
	.video_encode_name     = JPG_ENCODER_NAME,
	.audio_encode_name      = AUDIO_AAC_ENCODER_NAME,
	.path            = "/custom",
};



extern void live_rtsp_init(const rtp_name *rtsp);
extern void live_h264_rtsp_init(const rtp_name *rtsp);
extern void webfile_rtsp_init(const rtp_name *rtsp);
extern void rtsp_mjpeg_live_init(const rtp_name *rtsp);
extern void rtsp_h264_live_init(const rtp_name *rtsp);

void *jpeg_encode_init(const char *encode_name);
void *h264_encode_init(const char *encode_name);
void *rtsp_audio_encode_init(const char *encode_name);
extern void custom_rtsp_jpeg_init(const rtp_name *rtsp);
static void spook_thread(void *d)
{
	init_random();
	global_init();

	jpeg_encode_init(JPG_ENCODER_NAME);
	h264_encode_init(H264_ENCODER_NAME);
	rtsp_audio_encode_init(AUDIO_AAC_ENCODER_NAME);

	rtsp_mjpeg_live_init(&live_dvp);
	rtsp_h264_live_init(&live_h264);
	custom_rtsp_jpeg_init(&live_custom);
	rtsp_h264_live_init(&live_h264_2);
	//webfile_rtsp_init(&webfile);
}

void spook_init(void)
{
	//thread_create(spook_thread, 0, TASK_SPOOK_PRIO, 0, 110*1024, "spook");
	spook_thread(0);
}


