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

#include <stdlib.h>
#include <stdio.h>
//#include <test_util.h>
#include <csi_config.h>
 
#include <event.h>
#include <log.h>
#include <frame.h>
#include <list.h>
//#include <linux/linux_mutex.h>
#include "spook_config.h"
#include "rtp.h"

#include "lwip/api.h"
#include <csi_kernel.h>

#include "csi_core.h"
#include "list.h"

#include "jpgdef.h"


#include "osal/sleep.h"

#ifdef USB_EN
#include "dev/usb/uvc_host.h"
#endif

#include "osal/string.h"


#include "stream_frame.h"
#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"




 //data申请空间函数
 #define STREAM_MALLOC     av_psram_malloc
 #define STREAM_FREE       av_psram_free
 #define STREAM_ZALLOC     av_psram_zalloc
 
 //结构体申请空间函数
 #define STREAM_LIBC_MALLOC     av_malloc
 #define STREAM_LIBC_FREE       av_free
 #define STREAM_LIBC_ZALLOC     av_zalloc
 
 
 #define MAX_RTSP_JPG_RECV 28
 struct jpg_rtsp_msi {
	 struct msi    *msi;
	 uint8_t filter_type;	//暂时只是支持某个类型的数据(0xffff是不过滤,只是判断stype)
	 uint8_t srcID;
	 //struct os_event evt;
	 //RBUFFER_DEF(data, struct framebuff *, MAX_RTSP_JPG_RECV);
 };
 
#define MAX_RTSP_AUDIO_RECV 10
struct rtsp_audio_msi {
	struct msi *msi;
};
 
static int32 rtsp_jpg_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
	struct jpg_rtsp_msi *jpg_rtsp = (struct jpg_rtsp_msi *)msi->priv;
	int32_t ret = RET_OK;
	//os_printf("cmd_id:%d\n",cmd_id);
	switch (cmd_id) {
		case MSI_CMD_POST_DESTROY:
		{
			os_printf("%s:%d\taddr:%X\n",__FUNCTION__,__LINE__,RETURN_ADDR());
			STREAM_LIBC_FREE(jpg_rtsp);
		}
		break;
		case MSI_CMD_TRANS_FB:
		{
		  struct framebuff *fb = (struct framebuff *)param1;
		  //来源要匹配
		  if(jpg_rtsp->srcID && jpg_rtsp->srcID != fb->srcID)
		  {
			ret = RET_ERR;
			break;
		  }
		  if(jpg_rtsp->filter_type != (uint8_t)~0)
		  {
			ret = RET_ERR;
			if(jpg_rtsp->filter_type == fb->stype)
			{
				ret = RET_OK;
			}
		  }
		}
		break;

		case MSI_CMD_SET_SPEED: //设置播放倍速，控制数据读取行为
			break;
		default:
			break;
	}
	return ret;
}

static int32_t rtsp_audio_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
	struct rtsp_audio_msi *audio_rtsp = (struct rtsp_audio_msi*)msi->priv;
    int32_t ret = RET_OK;
    switch(cmd_id) {
        case MSI_CMD_TRANS_FB:
        {
			ret = RET_OK;
            break;
        }       
        case MSI_CMD_POST_DESTROY:
        {
			STREAM_LIBC_FREE(audio_rtsp);
            ret = RET_OK;
            break;                
        }			
        default:
            break;    
    }
    return ret;
}

struct msi *rtsp_msi_init(const char *name,uint8_t filter_type,uint8_t srcID)
{
	struct msi *rtsp_msi = msi_new(name,MAX_RTSP_JPG_RECV,NULL);
	struct jpg_rtsp_msi *jpg_rtsp = rtsp_msi->priv;

	if(!jpg_rtsp)
	{
		jpg_rtsp = (struct jpg_rtsp_msi*)STREAM_LIBC_ZALLOC(sizeof(struct jpg_rtsp_msi));
		jpg_rtsp->filter_type = filter_type;
		jpg_rtsp->srcID = srcID;
		rtsp_msi->priv = (void*)jpg_rtsp;
		rtsp_msi->action = rtsp_jpg_action;
		jpg_rtsp->msi = rtsp_msi;
		rtsp_msi->priv = (void*)jpg_rtsp;
	}
	rtsp_msi->enable = 1;
	return rtsp_msi;
}
 
struct msi *rtsp_audio_msi_init(const char *name)
{
	struct msi *rtsp_audio_msi = msi_new(name, MAX_RTSP_AUDIO_RECV,NULL);
	struct rtsp_audio_msi *audio_rtsp = rtsp_audio_msi->priv;

	if(!audio_rtsp) {
		audio_rtsp = (struct rtsp_audio_msi*)STREAM_LIBC_ZALLOC(sizeof(struct rtsp_audio_msi));
		rtsp_audio_msi->priv = (void*)audio_rtsp;
		rtsp_audio_msi->action = (msi_action)rtsp_audio_action;
		audio_rtsp->msi = rtsp_audio_msi;
	}
	rtsp_audio_msi->enable = 1;
	return rtsp_audio_msi;
}
 
void rtsp_msi_deinit(struct msi *msi)
{
	if(msi)
	{
		msi->enable = 0;
		msi_destroy(msi);
	}

}

void rtsp_audio_msi_deinit(struct msi *msi)
{
	if(msi)
	{
		msi->enable = 0;
		msi_destroy(msi);
	}
}

struct frame *new_frame(void)
{
	struct frame *f;

	//新帧
	f = (struct frame*)STREAM_LIBC_MALLOC(sizeof(struct frame));

	return f;
}

struct frame *new_audio_frame(void)
{
	struct frame *f;

	//新帧
	f = (struct frame*)STREAM_LIBC_MALLOC(sizeof(struct frame));
	return f;
}


struct frame_exchanger *new_exchanger_audio( int slots,
					frame_deliver_func func, void *d )
{
	struct frame_exchanger *ex;
	ex = (struct frame_exchanger *)STREAM_LIBC_MALLOC( sizeof( struct frame_exchanger ) );

	ex->f = func;
	ex->d = d;
	return ex;
}


struct frame_exchanger *new_exchanger( int slots,
					frame_deliver_func func, void *d )
{
	struct frame_exchanger *ex;

	ex = (struct frame_exchanger *)
			STREAM_LIBC_MALLOC( sizeof( struct frame_exchanger ) );
	ex->f = func;
	ex->d = d;



	/* read_mutex is used to avoid  closing the tcp when frame is sending through sendmsg */




	
	return ex;
}

int exchange_frame( struct frame_exchanger *ex, struct frame *frame )
{
	ex->jf= frame;
	return 0;
}



void unref_frame( struct frame *f )
{
//	int r;
	//del_frame
	if(!f)
	{
		return;
	}
	DEL_JPG_FRAME((struct framebuff *)f->get_f);
}





static int opcode_func(stream *s,void *priv,int opcode)
{
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
            enable_stream(s,1);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		case STREAM_RECV_DATA_FINISH:
		break;

		case STREAM_CLOSE_EXIT:
			os_printf("%s:%d close\n",__FUNCTION__,__LINE__);
		break;
		


		default:
			//默认都返回成功
		break;
	}
	return res;
}

//正常这个就是运行就退出
void err_rtsp_stream(struct rtp_node *node)
{
	struct frame_exchanger *ex = node->video_ex;
	while(1)
	{
		ex->f( NULL, ex->d );
		os_sleep_ms(1);
	}
}

void spook_send_thread_stream(struct rtsp_priv *r)
{
	struct rtp_node *node = r->live_node;	
	struct frame_exchanger *ex = node->video_ex;
	struct frame_exchanger *audio_ex = node->audio_ex;
	struct jpg_rtsp_msi *jpg_rtsp = r->v_msi?(struct jpg_rtsp_msi *)r->v_msi->priv:NULL;
	struct rtsp_audio_msi *audio_rtsp = r->a_msi?(struct rtsp_audio_msi *)r->a_msi->priv:NULL;
	struct frame *jpeg;
	struct frame *audio;
	struct framebuff *fb = NULL;
	struct framebuff *audio_fb = NULL;

	int count_times = 0;
	int get_times_out = 0;
	int cnt_num = 0;
	uint32_t time_for_count;
	uint32_t time = 0;

	time_for_count = os_jiffies();
	uint8_t *scan_data;
	//这里while用是否图传标志去判断
	while(1) 
	{
		if(audio_rtsp) {
			audio_fb = msi_get_fb(audio_rtsp->msi, 0);
			if(audio_fb && audio_fb->mtype != F_AUDIO) {
				msi_delete_fb(NULL, audio_fb);
				audio_fb = NULL;				
			}
			if(audio_fb) {
				audio = audio_ex->jf;
				audio->get_f = (void*)audio_fb;
				audio->length = audio_fb->len;
				audio->timestamp = audio_fb->time;
				audio->format = FORMAT_AUDIO;		
				audio_ex->f(audio,audio_ex->d);
			}
		}
		fb = msi_get_fb(jpg_rtsp->msi, 0);
		if(fb && (fb->mtype != F_H264 && fb->mtype != F_JPG))
		{
			msi_delete_fb(NULL, fb);
			fb = NULL;
		}
		if (fb)
		{
				//_os_printf(KERN_INFO"time:%d\n",fb->time);
				jpeg = ex->jf;//ex->slave_cur->f;
				//指针赋值
				jpeg->get_f = (void*)fb;
				jpeg->node_len = fb->len;
				//主要是扫描用scan_buf
				scan_data = (uint8_t*)fb->data;
				jpeg->d = (uint8_t*)fb->data;
				jpeg->first_length = fb->len;


				cnt_num++;

				if((os_jiffies() - time_for_count) > 1000){
						time_for_count = os_jiffies();
						_os_printf(KERN_INFO"cnt_num:%d\r\n",cnt_num);
						cnt_num = 0;
				}
				jpeg->length = fb->len;

				

				jpeg->format = FORMAT_JPEG;

				jpeg->timestamp = fb->time;
				//_os_printf("jpeg time:%d\n",jpeg->timestamp);
				/*
				* callback: get_back_frame( struct frame *f, void *d )
				* d: (jpeg_encoder *)en
				*/
				//_os_printf("#####");
				//_os_printf("ex:%X\tf:%X\n",ex,ex->f);
				_os_printf("#");
				//os_printf("ex->f:%X\n",ex->f);
				ex->f( jpeg, ex->d );

		}

		if(fb)
		{
			ex->ready = 0;
			count_times++;
			if(count_times>25)
			{
				_os_printf("time:%lld\n",os_jiffies()-time);
				count_times = 0;
				time = os_jiffies();
			}
		}
		if(!fb && !audio_fb)
		{
			os_sleep_ms(1);
			get_times_out++;
			if(get_times_out>1000)
			{
				ex->f( NULL, ex->d );
				get_times_out = 0;
			}
			continue;
		}
		else
		{
			get_times_out = 0;
		}

		fb = NULL;


	}
}
