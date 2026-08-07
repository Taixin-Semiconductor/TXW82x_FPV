#include "sys_config.h"
#include "typesdef.h"
#include <csi_kernel.h>
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "dev.h"
#include "devid.h"
#include "hal/gpio.h"
#include "hal/scale.h"
#include "hal/lcdc.h"
#include "hal/spi.h"
#include "hal/rotate.h"
#include "hal/dma.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/vpp/hgvpp.h"
#include "dev/scale/hgscale.h"
#include "dev/jpg/hgjpg.h"
#include "dev/lcdc/hglcdc.h"
#include "osal/semaphore.h"
#include "lib/lcd/lcd.h"
#include "osal/event.h"
#include "lib/lcd/gui.h"
#include "lib/heap/sysheap.h"
#include "hal/dma2d.h"
#include "hal/csc.h"
#include "dev/csc/hgcsc.h"
#include "lib/video/vpp/vpp_dev.h"
extern uint8 *yuvbuf;
extern uint8 *yuvbuf1;

uint8 *scaler2buf_y;
uint8 *scaler2buf_u;
uint8 *scaler2buf_v;

uint8 *scaler2_dir_srambuf;
extern uint8 *video_decode_mem;
extern uint8 *video_psram_mem; 
extern uint8 *video_dma2d_mem;
extern uint8 *video_dma2d_mem1;

extern uint8 *video_psram_mem1;
extern uint8 *video_psram_mem2;
extern uint8 *video_dma2d_mem1_cache;
extern uint8 *video_decode_mem;
extern uint8 *video_decode_mem1;
extern volatile uint8 dma2d_part;
extern volatile uint32 dma2d_flash;

#if LCD_FROM_DMA2D_BUF
extern uint8 video_psram_dma2d_mem[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2]; 
extern uint8 video_psram_dma2d_mem1[320*240+320*240/2];
extern uint8 video_psram_dma2d_mem1_cache[320*240+320*240/2];
#endif

struct os_event scale2_mutex;

static struct os_semaphore dma2d_sem   = {0,NULL};

void scale3_done(uint32 irq_flag,uint32 irq_data,uint32 param1);
void scale3_dma2d_done(uint32 irq_flag,uint32 irq_data,uint32 param1);
void scale3_doublebuf_done(uint32 irq_flag,uint32 irq_data,uint32 param1);
void scale3_ov_isr(uint32 irq_flag,uint32 irq_data,uint32 param1);
void scale3_error_pending_isr(uint32 irq_flag,uint32 irq_data,uint32 param1);
extern void scale2_done(uint32 irq_flag,uint32 irq_data,uint32 param1);
extern void scale2_ov_isr(uint32 irq_flag,uint32 irq_data,uint32 param1);

void scale_dma2d_init()
{
	os_sema_init(&dma2d_sem,0);
}

void scale_dma2d_down(int32 tmo_ms)
{
	os_sema_down(&dma2d_sem,tmo_ms);
	//_os_printf("$\n");
}

void scale_dma2d_up()
{
	os_sema_up(&dma2d_sem);
	//_os_printf("@\n");
}

void scale_ov_isr(uint32 irq_flag,uint32 irq_data,uint32 param1){
	//struct scale_device *scale_dev = (struct scale_device *)irq_data;
	_os_printf("O");
}


void scale_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	_os_printf("D");
}

volatile uint8 thumb_done = 0;
void scale2_thumb_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	thumb_done = 1;
	os_printf("scale2 thumb..\r\n");
	os_event_set(&scale2_mutex, BIT(0), NULL);
}

void scale2_rgb_ov_isr(uint32 irq_flag,uint32 irq_data,uint32 param1){
	os_printf("scale2 rgb ov..\r\n");
}

void scale3_allframe_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	os_printf("scale3 all frame..\r\n");
}

void scale3_thumb_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	thumb_done = 1;
	os_printf("scale2 thumb..\r\n");
}

void scale_soft_ov_isr(uint32 irq_flag,uint32 irq_data,uint32 param1){
	//struct scale_device *scale_dev = (struct scale_device *)irq_data;
	//_os_printf("ov");
}


extern uint32_t yuv_buf_line(uint8_t which);
void scale_from_vpp(struct scale_device *scale_dev,uint32 yuvbuf_addr,uint32 s_w,uint32 s_h,uint32 d_w,uint32 d_h){
	scale_set_in_out_size(scale_dev,s_w,s_h,d_w,d_h);
	scale_set_step(scale_dev,s_w,s_h,d_w,d_h);
	scale_set_line_buf_num(scale_dev,yuv_buf_line(0));
	scale_set_in_yaddr(scale_dev,yuvbuf_addr);
	scale_set_in_uaddr(scale_dev,yuvbuf_addr+s_w*yuv_buf_line(0));
	scale_set_in_vaddr(scale_dev,yuvbuf_addr+s_w*yuv_buf_line(0)+s_w*yuv_buf_line(0)/4);	
	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale_done,(uint32)scale_dev);	
	scale_request_irq(scale_dev,INBUF_OV,(scale_irq_hdl )&scale_ov_isr,(uint32)scale_dev);
	scale_set_data_from_vpp(scale_dev,1);  
	scale_open(scale_dev);
}

void scale2_isr_done(uint32 irq_flag,uint32 irq_data,uint32 param1){

	_os_printf("%s\r\n",__func__);
}


void scale_from_jpeg_config(struct scale_device *scale_dev,uint8_t dirtolcd,uint32 in_w,uint32 in_h,uint32 out_w,uint32 out_h,uint8_t larger){	
	uint32_t ow_n,oh_n;
	set_lcd_photo1_config(out_w,out_h,0);
	if(dirtolcd == 0){
		scale_close(scale_dev);
		scale_set_input_stream(scale_dev,MJPEG_DEC);
		scale_set_output_sram_or_frame(scale_dev,1);
		scale_set_in_out_size(scale_dev,in_w,in_h,out_w,out_h);
		ow_n = in_w*10/larger;
		oh_n = in_h*10/larger;	
		scale_set_step(scale_dev,ow_n,oh_n,out_w,out_h);
		in_w = (in_w - ow_n)/2; 
		in_h = (in_h - oh_n)/2;
		scale_set_start_addr(scale_dev,in_w,in_h);
		
		scale_set_out_yaddr(scale_dev,(uint32)video_decode_mem);
		scale_set_out_uaddr(scale_dev,(uint32)video_decode_mem+scale_p1_w*p1_h);
		scale_set_out_vaddr(scale_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4);
		if(scaler2buf_y == NULL){
			scaler2buf_y = malloc(0x20+scale_p1_w+21*SRAMBUF_WLEN*4);
			if(scaler2buf_y == NULL){
				_os_printf("%s %d\r\n",__func__,__LINE__);
				return;
			}
		}

		if(scaler2buf_u == NULL){
			scaler2buf_u = malloc(0x10+scale_p1_w/2+13*SRAMBUF_WLEN*4);
			if(scaler2buf_u == NULL){
				_os_printf("%s %d\r\n",__func__,__LINE__);
				return;
			}
		}
		if(scaler2buf_v == NULL){
			scaler2buf_v = malloc(0x10+scale_p1_w/2+13*SRAMBUF_WLEN*4);
			if(scaler2buf_v == NULL){
				_os_printf("%s %d\r\n",__func__,__LINE__);
				return;
			}
		}
		
		scale_set_srambuf_wlen(scale_dev,SRAMBUF_WLEN);
		//_os_printf("y_scaler_len:%d  uv_scaler_len:%d\r\n",0x20+scale_p1_w+16*SRAMBUF_WLEN*4+64,0x12+scale_p1_w/2+16*SRAMBUF_WLEN*4+64);
		_os_printf("scale2 y:%08x  u:%08x  v:%08x\r\n",scaler2buf_y,scaler2buf_u,scaler2buf_v);
		scale_linebuf_yuv_addr(scale_dev,(uint32)scaler2buf_y,(uint32)scaler2buf_u,(uint32)scaler2buf_v);			
//		scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale2_isr_done,(uint32)scale_dev);	
		scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale2_done,(uint32)scale_dev);	
		scale_request_irq(scale_dev,INBUF_OV,(scale_irq_hdl )&scale2_ov_isr,(uint32)scale_dev);
		scale_open(scale_dev); 
	}	
	else{
		scale_close(scale_dev);
		scale_set_output_sram_or_frame(scale_dev,1);
		scale_set_rotate_cfg(scale_dev,1);
		scale_set_line_buf_num(scale_dev,32);

		scale_set_in_out_size(scale_dev,in_w,in_h,out_w,out_h);
		ow_n = in_w*10/larger;
		oh_n = in_h*10/larger;			
		scale_set_step(scale_dev,ow_n,oh_n,out_w,out_h);
		in_w = (in_w - ow_n)/2; 
		in_h = (in_h - oh_n)/2;
		scale_set_start_addr(scale_dev,in_w,in_h);
		os_printf("freemem :%d\r\n", sysheap_freesize(&sram_heap));

		if(scaler2_dir_srambuf == NULL){
			scaler2_dir_srambuf = malloc(scale_p1_w*32+scale_p1_w*32/2);
			if(scaler2_dir_srambuf == NULL){
				return;
			}
		}
			
		scale_set_out_yaddr(scale_dev,(uint32)scaler2_dir_srambuf);
		scale_set_out_uaddr(scale_dev,(uint32)scaler2_dir_srambuf+scale_p1_w*32);
		scale_set_out_vaddr(scale_dev,(uint32)scaler2_dir_srambuf+scale_p1_w*32+scale_p1_w*32/4);	
		if(scaler2buf_y == NULL){
			scaler2buf_y = malloc(0x20+scale_p1_w+16*SRAMBUF_WLEN*4);
			if(scaler2buf_y == NULL){
				return;
			}
		}

		if(scaler2buf_u == NULL){
			scaler2buf_u = malloc(0x12+scale_p1_w/2+16*SRAMBUF_WLEN*4);
			if(scaler2buf_u == NULL){
				return;
			}
		}
		if(scaler2buf_v == NULL){
			scaler2buf_v = malloc(0x12+scale_p1_w/2+16*SRAMBUF_WLEN*4);
			if(scaler2buf_v == NULL){
				return;
			}
		}
		scale_linebuf_yuv_addr(scale_dev,(uint32)scaler2buf_y,(uint32)scaler2buf_u,(uint32)scaler2buf_v);		
		//scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale2_done,(uint32)scale_dev);	
		scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale2_isr_done,(uint32)scale_dev);	
		scale_request_irq(scale_dev,INBUF_OV,(scale_irq_hdl )&scale2_ov_isr,(uint32)scale_dev);

		scale_open(scale_dev); 
	}
}

void scale2_from_jpeg_config_for_msi(struct scale_device *scale_dev,uint32_t yinsram,uint32_t uinsram,uint32_t vinsram,uint32_t yuvoutbuf,uint32 in_w,uint32 in_h,uint32 out_w,uint32 out_h,uint8_t larger){	
	uint32_t ow_n,oh_n;
	//os_event_wait(&scale2_mutex, BIT(0), NULL,OS_EVENT_WMODE_CLEAR, osWaitForever);
	scale_close(scale_dev);
	scale_set_input_stream(scale_dev,MJPEG_DEC);
	scale_set_output_sram_or_frame(scale_dev,1);
	scale_set_in_out_size(scale_dev,in_w,in_h,out_w,out_h);
	ow_n = in_w*10/larger;
	oh_n = in_h*10/larger;	
	scale_set_step(scale_dev,ow_n,oh_n,out_w,out_h);
	in_w = (in_w - ow_n)/2; 
	in_h = (in_h - oh_n)/2;
	scale_set_start_addr(scale_dev,in_w,in_h);
	if((out_w%4) != 0){
		_os_printf("scale2 output need word aligned");
	}
	scale_set_out_yaddr(scale_dev,(uint32)yuvoutbuf);
	scale_set_out_uaddr(scale_dev,(uint32)yuvoutbuf+out_w*out_h);
	scale_set_out_vaddr(scale_dev,(uint32)yuvoutbuf+out_w*out_h+out_w*out_h/4);
	
	scale_set_srambuf_wlen(scale_dev,SRAMBUF_WLEN);
	scale_linebuf_yuv_addr(scale_dev,(uint32)yinsram,(uint32)uinsram,(uint32)vinsram);			
	scale_open(scale_dev); 

}



void scale_soft_from_psram_to_enc(struct scale_device *scale_dev,uint8_t * psram_data,uint32_t w,uint32 h,uint32_t ow,uint32_t oh){
	uint16 icount = 2;	
	_os_printf("%s  %d*%d====>%d*%d\r\n",__func__,w,h,ow,oh);
	scale_close(scale_dev);
	scale_set_in_out_size(scale_dev,w,h,ow,oh);
	scale_set_step(scale_dev,w,h,ow,oh);
	scale_set_start_addr(scale_dev,0,0);
	scale_set_data_from_vpp(scale_dev,0);  
	scale_set_line_buf_num(scale_dev,16/*32*/);       //soft的line buf
	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale_done,(uint32)scale_dev);	
	scale_request_irq(scale_dev,INBUF_OV,(scale_irq_hdl )&scale_soft_ov_isr,(uint32)scale_dev);
	scale_open(scale_dev);	
	
	scale_set_inbuf_num(scale_dev,0,0);
	if(scale_get_inbuf_num(scale_dev) == 0){
		icount = 2;
		scale_set_new_frame(scale_dev,1);
	}else{
		return;
	}	


	hw_memcpy(yuvbuf,psram_data,(8/*16*/*icount)*w);
	hw_memcpy(yuvbuf+(8/*16*/*icount)*w,psram_data+h*w,(8/*16*/*icount)*w/4);
	hw_memcpy(yuvbuf+(8/*16*/*icount)*w+(8/*16*/*icount)*w/4,psram_data+h*w+h*w/4,(8/*16*/*icount)*w/4);
	scale_set_in_yaddr(scale_dev,(uint32)yuvbuf);
	scale_set_in_uaddr(scale_dev,(uint32)yuvbuf+(8/*16*/*icount)*w);
	scale_set_in_vaddr(scale_dev,(uint32)yuvbuf+(8/*16*/*icount)*w+(8/*16*/*icount)*w/4);


	scale_set_inbuf_num(scale_dev,icount*8/*16*/-1,15/*31*/);	 //0~31


	while(1){
		if(scale_get_heigh_cnt(scale_dev) > ((icount-1)*8/*16*/) )
		{			
			if((icount%2) == 0){
				//os_printf("get_height_cnt:%d===>up head\r\n",scale_get_heigh_cnt(scale_dev));
				hw_memcpy(yuvbuf,                  psram_data+(8/*16*/*icount)*w,                            8/*16*/*w);
				hw_memcpy(yuvbuf+16/*32*/*w,             psram_data+h*w+(8/*16*/*icount)*w/4,                    8/*16*/*w/4);
				hw_memcpy(yuvbuf+16/*32*/*w+16/*32*/*w/4,      psram_data+h*w+h*w/4+(8/*16*/*icount)*w/4,              8/*16*/*w/4);	
				
			}else{
				//os_printf("get_height_cnt:%d===>up tail\r\n",scale_get_heigh_cnt(scale_dev));
				hw_memcpy(yuvbuf+8/*16*/*w,                  psram_data+(8/*16*/*icount)*w,                       8/*16*/*w);
				hw_memcpy(yuvbuf+16/*32*/*w+16/*32*/*w/8,           psram_data+h*w+(8/*16*/*icount)*w/4,               8/*16*/*w/4);
				hw_memcpy(yuvbuf+16/*32*/*w+16/*32*/*w/4+16/*32*/*w/8,    psram_data+h*w+h*w/4+(8/*16*/*icount)*w/4,         8/*16*/*w/4);	
			}
			icount++;
			if(icount == (h/8/*16*/)){			
				if((icount%2) == 1)     //3   start
					scale_set_inbuf_num(scale_dev,icount*8/*16*/,8/*16*/);
				else
					scale_set_inbuf_num(scale_dev,icount*8/*16*/,0);


				break;
			}
			if((icount%2) == 1)     //3   start
				scale_set_inbuf_num(scale_dev,icount*8/*16*/,8/*16*/);
			else
				scale_set_inbuf_num(scale_dev,icount*8/*16*/,0);
		}
	}
	
}

volatile uint8 scale_take_photo = 1;
void scale_take_photo_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	scale_take_photo = 1;
	os_printf("===============================================================================================take photo done\r\n");
}	

void scale_from_soft_to_jpg(struct scale_device *scale_dev,uint32 yuvbuf_addr,uint32 s_w,uint32 s_h,uint32 d_w,uint32 d_h){
	uint8 line_num;
	struct vpp_device *vpp_dev;
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
	scale_close(scale_dev);
	vpp_set_itp_enable(vpp_dev,0);	
	_os_printf("take:%d %d %d\r\n",s_h,d_h,((s_h*16)/d_h));
	line_num = (((s_h*16)/d_h) + ((((s_h*16)/d_h)%2)?1:0) + 2)/2; 
	_os_printf("line_num:%d\r\n",line_num*3*2);
	scale_set_in_out_size(scale_dev,s_w,s_h,d_w,d_h);
	scale_set_step(scale_dev,s_w,s_h,d_w,d_h);
	scale_set_line_buf_num(scale_dev,line_num*3*2);

	//scale_set_in_yaddr(scale_dev,itp_sram_linbuf);
	//scale_set_in_uaddr(scale_dev,itp_sram_linbuf+line_num*3*2*s_w);
	//scale_set_in_vaddr(scale_dev,itp_sram_linbuf+line_num*3*2*s_w+line_num*3*2*s_w/4);
	scale_set_in_yaddr(scale_dev,(uint32)yuvbuf);
	scale_set_in_uaddr(scale_dev,(uint32)yuvbuf+s_w*32);
	scale_set_in_vaddr(scale_dev,(uint32)yuvbuf+s_w*32+s_w*8);	
	
	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale_take_photo_done,(uint32)scale_dev);	
	scale_request_irq(scale_dev,INBUF_OV,(scale_irq_hdl )&scale_ov_isr,(uint32)scale_dev);	
	scale_set_data_from_vpp(scale_dev,0);
	scale_set_inbuf_num(scale_dev,0,0);
	scale_open(scale_dev);
	scale_set_new_frame(scale_dev,1);
	
	vpp_set_itp_y_addr(vpp_dev,yuvbuf_addr);
	vpp_set_itp_u_addr(vpp_dev,yuvbuf_addr+s_w*s_h);
	vpp_set_itp_v_addr(vpp_dev,yuvbuf_addr+s_w*s_h+s_w*s_h/4);
	vpp_set_itp_linebuf(vpp_dev,line_num);	
	vpp_set_itp_auto_close(vpp_dev,1);
	vpp_set_itp_enable(vpp_dev,1);		
}


void scale_from_h264_config(struct scale_device *scale_dev,uint32 in_w,uint32 in_h,uint32 out_w,uint32 out_h,uint8_t larger){
	uint32_t ow_n,oh_n;
	set_lcd_photo1_config(out_w,out_h,0);
	scale_close(scale_dev);
	scale_set_input_stream(scale_dev,H264_DEC);
	scale_set_output_sram_or_frame(scale_dev,1);

	scale_set_in_out_size(scale_dev,in_w,in_h,out_w,out_h);
	ow_n = in_w*10/larger;
	oh_n = in_h*10/larger;		
	scale_set_step(scale_dev,ow_n,oh_n,out_w,out_h);
	in_w = (in_w - ow_n)/2; 
	in_h = (in_h - oh_n)/2;
	scale_set_start_addr(scale_dev,in_w,in_h);
	
	scale_set_out_yaddr(scale_dev,(uint32)video_decode_mem);
	scale_set_out_uaddr(scale_dev,(uint32)video_decode_mem+scale_p1_w*p1_h);
	scale_set_out_vaddr(scale_dev,(uint32)video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4);
	_os_printf("scale_p1_w:%d  p1_h:%d\r\n",scale_p1_w,p1_h);
	if(scaler2buf_y == NULL){
		_os_printf("ysram:%d\r\n",0x20+scale_p1_w+16*SRAMBUF_WLEN*4);
		scaler2buf_y = malloc(0x20+scale_p1_w+16*SRAMBUF_WLEN*4);
		if(scaler2buf_y == NULL){
			_os_printf("y room malloc error\r\n");
			return;
		}
	}
	
	if(scaler2buf_u == NULL){
		_os_printf("usram:%d\r\n",0x12+scale_p1_w/2+16*SRAMBUF_WLEN*4);
		scaler2buf_u = malloc(0x12+scale_p1_w/2+16*SRAMBUF_WLEN*4);
		if(scaler2buf_u == NULL){
			_os_printf("u room malloc error\r\n");
			return;
		}
	}
	if(scaler2buf_v == NULL){
		_os_printf("vsram:%d\r\n",0x12+scale_p1_w/2+16*SRAMBUF_WLEN*4);
		scaler2buf_v = malloc(0x12+scale_p1_w/2+16*SRAMBUF_WLEN*4);
		if(scaler2buf_v == NULL){
			_os_printf("v room malloc error\r\n");
			return;
		}
	}
	scale_linebuf_yuv_addr(scale_dev,(uint32)scaler2buf_y,(uint32)scaler2buf_u,(uint32)scaler2buf_v);
	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale2_done,(uint32)scale_dev);	
	scale_request_irq(scale_dev,INBUF_OV,(scale_irq_hdl )&scale2_ov_isr,(uint32)scale_dev);
	scale_open(scale_dev);
}



void scale2_all_frame(struct scale_device *scale_dev,uint8_t type,uint32 in_w,uint32 in_h,uint32 out_w,uint32 out_h,uint32 src_addr,uint32 des_addr){
	uint32_t ow_n,oh_n;
	//os_mutex_lock(&scale2_mutex, osWaitForever);
	os_event_wait(&scale2_mutex, BIT(0), NULL,OS_EVENT_WMODE_CLEAR, osWaitForever);
	scale_close(scale_dev);
	scale_set_input_stream(scale_dev,type);
	scale_set_output_sram_or_frame(scale_dev,0);
	if(type == FRAME_YUV420P){
		scale_set_in_yaddr(scale_dev,(uint32)src_addr);
		scale_set_in_uaddr(scale_dev,(uint32)src_addr+in_w*in_h);
		scale_set_in_vaddr(scale_dev,(uint32)src_addr+in_w*in_h+in_w*in_h/4);		
		scale_set_out_yaddr(scale_dev,(uint32)des_addr);
		scale_set_out_uaddr(scale_dev,(uint32)des_addr+out_w*out_h);
		scale_set_out_vaddr(scale_dev,(uint32)des_addr+out_w*out_h+out_w*out_h/4);	
	}else if(type == FRAME_YUV422P){
		scale_set_in_yaddr(scale_dev,(uint32)src_addr);
		scale_set_in_uaddr(scale_dev,(uint32)src_addr+in_w*in_h);
		scale_set_in_vaddr(scale_dev,(uint32)src_addr+in_w*in_h+in_w*in_h/2);		
		scale_set_out_yaddr(scale_dev,(uint32)des_addr);
		scale_set_out_uaddr(scale_dev,(uint32)des_addr+out_w*out_h);
		scale_set_out_vaddr(scale_dev,(uint32)des_addr+out_w*out_h+out_w*out_h/4);
	}
	else if(type == FRAME_YUV444P){
		scale_set_in_yaddr(scale_dev,(uint32)src_addr);
		scale_set_in_uaddr(scale_dev,(uint32)src_addr+in_w*in_h);
		scale_set_in_vaddr(scale_dev,(uint32)src_addr+in_w*in_h+in_w*in_h);
		scale_set_out_yaddr(scale_dev,(uint32)des_addr);
		scale_set_out_uaddr(scale_dev,(uint32)des_addr+out_w*out_h);
		scale_set_out_vaddr(scale_dev,(uint32)des_addr+out_w*out_h+out_w*out_h/4);	
	}else if(type == FRAME_RGB888P){
		scale_set_in_yaddr(scale_dev,(uint32)src_addr);
		scale_set_in_uaddr(scale_dev,(uint32)src_addr+in_w*in_h);
		scale_set_in_vaddr(scale_dev,(uint32)src_addr+in_w*in_h+in_w*in_h);
		scale_set_out_yaddr(scale_dev,(uint32)des_addr);
		scale_set_out_uaddr(scale_dev,(uint32)des_addr+out_w*out_h);
		scale_set_out_vaddr(scale_dev,(uint32)des_addr+out_w*out_h+out_w*out_h);			
	}


	scale_set_in_out_size(scale_dev,in_w,in_h,out_w,out_h);
	ow_n = in_w;
	oh_n = in_h;		
	scale_set_step(scale_dev,ow_n,oh_n,out_w,out_h);
	in_w = (in_w - ow_n)/2; 
	in_h = (in_h - oh_n)/2;
	scale_set_start_addr(scale_dev,in_w,in_h);
	
	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale2_thumb_done,(uint32)scale_dev);	
	scale_request_irq(scale_dev,INBUF_OV,(scale_irq_hdl )&scale2_rgb_ov_isr,(uint32)scale_dev);

	scale_open(scale_dev);
	scale_set_new_frame(scale_dev,1);
	
}

void scale2_mutex_init(){
	os_event_init(&scale2_mutex);
	os_event_set(&scale2_mutex, BIT(0), NULL);
}

void yuv_blk_reduce(uint8 *des, uint8* src,uint32 des_w,uint32 des_h, uint32 src_w, uint32_t src_h,uint32 x,uint32 y){
	struct dma2d_blkcpy_param blkcpy;
	int32 ret;
	struct dma2d_device *dev = (struct dma2d_device *)dev_get(HG_DMA2D_DEVID);
	x = x/4;
	//Y
	blkcpy.src_addr 			= (uint32)src;
	blkcpy.dst_addr 			= (uint32)des;
	blkcpy.color_mode			= DMA2D_COLOR_TYPE_ARGB8888;
	blkcpy.src_pixel_width		= src_w/4;
	blkcpy.dst_pixel_width		= des_w/4;
	blkcpy.blk_pixel_width		= des_w/4;
	blkcpy.blk_pixel_height 	= des_h;		
	blkcpy.src_pixel_start_height = y;
	blkcpy.src_pixel_start_width  = x;
	blkcpy.dst_pixel_start_height = 0;
	blkcpy.dst_pixel_start_width  = 0;	
	dma2d_blkcpy(dev, &blkcpy);
	ret = dma2d_check_status(dev);
	if(ret){
		_os_printf("dma2d error....\r\n");
	}

	
	//U
	blkcpy.src_addr 			= (uint32)src+src_w*src_h;
	blkcpy.dst_addr 			= (uint32)des+des_w*des_h;
	blkcpy.color_mode			= DMA2D_COLOR_TYPE_ARGB8888;
	blkcpy.src_pixel_width		= (src_w/2)/4;
	blkcpy.dst_pixel_width		= (des_w/2)/4;
	blkcpy.blk_pixel_width		= (des_w/2)/4;
	blkcpy.blk_pixel_height 	= des_h/2;		
	blkcpy.src_pixel_start_height = y/2;
	blkcpy.src_pixel_start_width  = x/2;
	blkcpy.dst_pixel_start_height = 0;
	blkcpy.dst_pixel_start_width  = 0;			
	dma2d_blkcpy(dev, &blkcpy);
	ret = dma2d_check_status(dev);
	if(ret){
		_os_printf("dma2d error....\r\n");
	}

	//V
	blkcpy.src_addr 			= (uint32)src+src_w*src_h+src_w*src_h/4;
	blkcpy.dst_addr 			= (uint32)des+des_w*des_h+des_w*des_h/4;
	blkcpy.color_mode			= DMA2D_COLOR_TYPE_ARGB8888;
	blkcpy.src_pixel_width		= (src_w/2)/4;
	blkcpy.dst_pixel_width		= (des_w/2)/4;
	blkcpy.blk_pixel_width		= (des_w/2)/4;
	blkcpy.blk_pixel_height 	= des_h/2;		
	blkcpy.src_pixel_start_height = y/2;
	blkcpy.src_pixel_start_width  = x/2;
	blkcpy.dst_pixel_start_height = 0;
	blkcpy.dst_pixel_start_width  = 0;
	dma2d_blkcpy(dev, &blkcpy);
	ret = dma2d_check_status(dev);
	if(ret){
		_os_printf("dma2d error....\r\n");
	}

}

void yuv_blk_cpy(uint8 *des, uint8* src,uint32 des_w,uint32 des_h, uint32 src_w, uint32_t src_h,uint32 x,uint32 y){
	struct dma2d_blkcpy_param blkcpy;
	int32 ret;
	struct dma2d_device *dev = (struct dma2d_device *)dev_get(HG_DMA2D_DEVID);
	x = x/4;
	//Y
	blkcpy.src_addr 			= (uint32)src;
	blkcpy.dst_addr 			= (uint32)des;
	blkcpy.color_mode			= DMA2D_COLOR_TYPE_ARGB8888;
	blkcpy.src_pixel_width		= src_w/4;
	blkcpy.dst_pixel_width		= des_w/4;
	blkcpy.blk_pixel_width		= src_w/4;
	blkcpy.blk_pixel_height 	= src_h;		
	blkcpy.src_pixel_start_height = 0;
	blkcpy.src_pixel_start_width  = 0;
	blkcpy.dst_pixel_start_height = y;
	blkcpy.dst_pixel_start_width  = x;	
	dma2d_blkcpy(dev, &blkcpy);
	ret = dma2d_check_status(dev);
	if(ret){
		_os_printf("dma2d error....\r\n");
	}
	
	//U
	blkcpy.src_addr 			= (uint32)src+src_w*src_h;
	blkcpy.dst_addr 			= (uint32)des+des_w*des_h;
	blkcpy.color_mode			= DMA2D_COLOR_TYPE_ARGB8888;
	blkcpy.src_pixel_width		= (src_w/2)/4;
	blkcpy.dst_pixel_width		= (des_w/2)/4;
	blkcpy.blk_pixel_width		= (src_w/2)/4;
	blkcpy.blk_pixel_height 	= src_h/2;		
	blkcpy.src_pixel_start_height = 0;
	blkcpy.src_pixel_start_width  = 0;
	blkcpy.dst_pixel_start_height = y/2;
	blkcpy.dst_pixel_start_width  = x/2;			
	dma2d_blkcpy(dev, &blkcpy);
	ret = dma2d_check_status(dev);
	if(ret){
		_os_printf("dma2d error....\r\n");
	}
	
	//V
	blkcpy.src_addr 			= (uint32)src+src_w*src_h+src_w*src_h/4;
	blkcpy.dst_addr 			= (uint32)des+des_w*des_h+des_w*des_h/4;
	blkcpy.color_mode			= DMA2D_COLOR_TYPE_ARGB8888;
	blkcpy.src_pixel_width		= (src_w/2)/4;
	blkcpy.dst_pixel_width		= (des_w/2)/4;
	blkcpy.blk_pixel_width		= (src_w/2)/4;
	blkcpy.blk_pixel_height 	= src_h/2;		
	blkcpy.src_pixel_start_height = 0;
	blkcpy.src_pixel_start_width  = 0;
	blkcpy.dst_pixel_start_height = y/2;
	blkcpy.dst_pixel_start_width  = x/2;
	dma2d_blkcpy(dev, &blkcpy);
	ret = dma2d_check_status(dev);
	if(ret){
		_os_printf("dma2d error....\r\n");
	}

} 

void rotate_line_to_des(struct dma2d_device *dev,uint32_t src,uint32_t des,uint16_t sw,uint16_t sh,uint16_t dw,uint32 x){
	struct dma2d_blkcpy_param blkcpy;
	int ret;
	//_os_printf("rotate:%x  %x  sw:%d  sh:%d   dw:%d   x:%d\r\n",src,des,sw,sh,dw,x);
	x = x/4;
	blkcpy.src_addr 			= (uint32)src;
	blkcpy.dst_addr 			= (uint32)des;
	blkcpy.color_mode			= DMA2D_COLOR_TYPE_ARGB8888;
	blkcpy.src_pixel_width		= sw/4;
	blkcpy.dst_pixel_width		= dw/4;
	blkcpy.blk_pixel_width		= sw/4;
	blkcpy.blk_pixel_height 	= sh;		
	blkcpy.src_pixel_start_height = 0;
	blkcpy.src_pixel_start_width  = 0;
	blkcpy.dst_pixel_start_height = 0;
	blkcpy.dst_pixel_start_width  = x;	
	dma2d_blkcpy(dev, &blkcpy);
	ret = dma2d_check_status(dev);
	if(ret){
		_os_printf("dma2d error....\r\n");
	}	

}

void data_recfg_for_yuv(struct dma2d_device *dev,uint32_t src,uint32_t des,uint16_t sw,uint16_t sh,uint16 dw,uint32 x){
	struct dma2d_blkcpy_param blkcpy;
	int ret;
	//_os_printf("src:%08x  des:%08x  sw:%d  sh:%d  dw:%d  x:%d\r\n",src,des,sw,sh,dw,x);
	blkcpy.src_addr 			= (uint32)src;
	blkcpy.dst_addr 			= (uint32)des;
	blkcpy.color_mode			= DMA2D_COLOR_TYPE_A8;
	blkcpy.src_pixel_width		= sw;
	blkcpy.dst_pixel_width		= dw;
	blkcpy.blk_pixel_width		= 1;
	blkcpy.blk_pixel_height 	= sh;		
	blkcpy.src_pixel_start_height = 0;
	blkcpy.src_pixel_start_width  = 0;
	blkcpy.dst_pixel_start_height = 0;
	blkcpy.dst_pixel_start_width  = x;	
	dma2d_blkcpy(dev, &blkcpy);
	ret = dma2d_check_status(dev);
	if(ret){
		_os_printf("dma2d error....\r\n");
	}		
}

void data_dma2d_cfg(struct dma2d_device *dev,uint32_t src,uint32_t des,uint16_t sw,uint16_t sh,uint16 dw,uint32 x){
	struct dma2d_blkcpy_param blkcpy;
	int ret;
	blkcpy.src_addr 			= (uint32)src;
	blkcpy.dst_addr 			= (uint32)des;
	blkcpy.color_mode			= DMA2D_COLOR_TYPE_A8;
	blkcpy.src_pixel_width		= sw;
	blkcpy.dst_pixel_width		= dw;
	blkcpy.blk_pixel_width		= sw;
	blkcpy.blk_pixel_height 	= sh;		
	blkcpy.src_pixel_start_height = 0;
	blkcpy.src_pixel_start_width  = 0;
	blkcpy.dst_pixel_start_height = 0;
	blkcpy.dst_pixel_start_width  = x;	
	//_os_printf("src:%x des:%x sw:%d  dw:%d  sh:%d\r\n",src,des,sw,dw,sh);
	dma2d_blkcpy(dev, &blkcpy);
	ret = dma2d_check_status(dev);
	if(ret){
		_os_printf("dma2d error....\r\n");
	}

}

//uint8_t rotate_sram[864*16*2*2]__attribute__((aligned(4)));
extern struct dma_device *m2mdma;
//800*480  : 200ms         320*240  : 50ms
void rotate_video_rgb_memory(uint32_t src,uint32 dst,uint32 sram_line_rom,uint16_t w,uint16_t h){
	uint32_t line_offset;
	uint32_t des_sram;
	uint16_t itk;
	uint8_t opt_len;
	struct rotate_device *rot_dev;
	rot_dev = (struct rotate_device *)dev_get(HG_ROTATE_DEVID);
	struct dma2d_device *dma2d_dev = (struct dma2d_device *)dev_get(HG_DMA2D_DEVID);

	line_offset = w*16*2;
	des_sram = sram_line_rom + line_offset;

	itk = h;
	rotate_set_size(rot_dev,w,16);
	while(itk != 0)	{
		if(itk >= 16){
			opt_len = 16;
		}else{
			opt_len = itk;
			rotate_set_size(rot_dev,w,itk);
		}
		
		hw_memcpy((void*)sram_line_rom,(void*)(src+(h - itk)*w*2),w*opt_len*2);
		rotate_set_addr(rot_dev,sram_line_rom,des_sram);
		rotate_run(rot_dev,ROT_90,ROT_16B);
		while(rotate_get_sta(rot_dev) != RET_OK);

		rotate_line_to_des(dma2d_dev,des_sram,dst,opt_len,w,h,h-itk);
		
		itk = itk - opt_len;
	}
	

}


void rotate_video_yuv_memory(uint32_t src,uint32 dst,uint32 sram_line_rom,uint16_t w,uint16_t h){
	uint32_t line_offset;
	uint32_t des_sram;
	uint16_t itk;
	uint8_t opt_len;
	struct rotate_device *rot_dev;
	rot_dev = (struct rotate_device *)dev_get(HG_ROTATE_DEVID);
	struct dma2d_device *dma2d_dev = (struct dma2d_device *)dev_get(HG_DMA2D_DEVID);

//y 16行数据
	line_offset = w*16;
	des_sram = sram_line_rom + line_offset;

	itk = h;
	rotate_set_size(rot_dev,w/2,16);
	while(itk != 0)	{
		if(itk >= 16){
			opt_len = 16;
		}else{
			opt_len = itk;
			rotate_set_size(rot_dev,w/2,itk);
		}
		
		hw_memcpy((void*)sram_line_rom,(void*)(src+(h - itk)*w),w*opt_len);
		rotate_set_addr(rot_dev,sram_line_rom,des_sram);
		rotate_run(rot_dev,ROT_90,ROT_16B);
		while(rotate_get_sta(rot_dev) != RET_OK);
		
		data_recfg_for_yuv(dma2d_dev,des_sram,sram_line_rom,2,w*opt_len/2,1,0);
		data_recfg_for_yuv(dma2d_dev,des_sram+1,sram_line_rom+w*opt_len/2,2,w*opt_len/2,1,0);
		data_dma2d_cfg(dma2d_dev,sram_line_rom+w*opt_len/2,des_sram,opt_len,w/2,opt_len*2,0);
		data_dma2d_cfg(dma2d_dev,sram_line_rom,des_sram,opt_len,w/2,opt_len*2,opt_len);
		
		
		rotate_line_to_des(dma2d_dev,des_sram,dst,opt_len,w,h,h-itk);
		
		itk = itk - opt_len;
	}

//u 8行
	src = src+w*h;
	dst = dst+w*h;
	
	w = w/2;
	h = h/2;
	itk = h;

	rotate_set_size(rot_dev,w/2,8);
	while(itk != 0)	{
		if(itk >= 8){
			opt_len = 8;
		}else{
			opt_len = itk;
			rotate_set_size(rot_dev,w/2,itk);
		}
		
		hw_memcpy((void*)sram_line_rom,(void*)(src+(h - itk)*w),w*opt_len);
		rotate_set_addr(rot_dev,sram_line_rom,des_sram);
		rotate_run(rot_dev,ROT_90,ROT_16B);
		while(rotate_get_sta(rot_dev) != RET_OK);
		
		data_recfg_for_yuv(dma2d_dev,des_sram,sram_line_rom,2,w*opt_len/2,1,0);
		data_recfg_for_yuv(dma2d_dev,des_sram+1,sram_line_rom+w*opt_len/2,2,w*opt_len/2,1,0);
		data_dma2d_cfg(dma2d_dev,sram_line_rom+w*opt_len/2,des_sram,opt_len,w/2,opt_len*2,0);
		data_dma2d_cfg(dma2d_dev,sram_line_rom,des_sram,opt_len,w/2,opt_len*2,opt_len);
		
		rotate_line_to_des(dma2d_dev,des_sram,dst,opt_len,w,h,h-itk);
		itk = itk - opt_len;
	}

//v 8行
	src = src+w*h;
	dst = dst+w*h;
	
	itk = h;

	rotate_set_size(rot_dev,w/2,8);
	while(itk != 0)	{
		if(itk >= 8){
			opt_len = 8;
		}else{
			opt_len = itk;
			rotate_set_size(rot_dev,w/2,itk);
		}
		
		hw_memcpy((void*)sram_line_rom,(void*)(src+(h - itk)*w),w*opt_len);
		rotate_set_addr(rot_dev,sram_line_rom,des_sram);
		rotate_run(rot_dev,ROT_90,ROT_16B);
		while(rotate_get_sta(rot_dev) != RET_OK);

		data_recfg_for_yuv(dma2d_dev,des_sram,sram_line_rom,2,w*opt_len/2,1,0);
		data_recfg_for_yuv(dma2d_dev,des_sram+1,sram_line_rom+w*opt_len/2,2,w*opt_len/2,1,0);
		data_dma2d_cfg(dma2d_dev,sram_line_rom+w*opt_len/2,des_sram,opt_len,w/2,opt_len*2,0);
		data_dma2d_cfg(dma2d_dev,sram_line_rom,des_sram,opt_len,w/2,opt_len*2,opt_len);

		rotate_line_to_des(dma2d_dev,des_sram,dst,opt_len,w,h,h-itk);
		itk = itk - opt_len;
	}


}


volatile uint8 test_csc_finish = 0;
void test_csc_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	test_csc_finish = 1;	
}


void scale_to_lcd_updata(){
	
	uint8_t *push_lcd;
	
	while(1){
		scale_dma2d_down(-1);
		if(dma2d_flash%3 == 0){              
			push_lcd = video_psram_mem;
		}else if(dma2d_flash%3 == 1){
			push_lcd = video_psram_mem1;
		}else{
			push_lcd = video_psram_mem2;
		}
	

		if(dma2d_part&BIT(1)){     //大分辨率部分
			hw_memcpy(push_lcd,video_dma2d_mem,800*480+800*480/2);           //满屏
			//rotate_video_yuv_memory(video_dma2d_mem,push_lcd,rotate_sram,800,480);
			yuv_blk_cpy(push_lcd,video_dma2d_mem1_cache,800,480,320,240,240,0);		
		}


		if(dma2d_part&BIT(0)){     //小分辨率部分
			hw_memcpy(video_dma2d_mem1_cache,video_dma2d_mem1,320*240+320*240/2);
			yuv_blk_cpy(push_lcd,video_dma2d_mem1_cache,800,480,320,240,240,0);      //小屏
		}
		
		//cache_deal_writeback();
		dma2d_flash++;
	}
}

void scale_to_lcd_config(uint32_t iw,uint32_t ih){	
	struct scale_device *scale_dev;
	k_task_handle_t dma2d_task_handle;
	scale_dev = (struct scale_device *)dev_get(HG_SCALE3_DEVID);
#if SCALE_DIRECT_TO_LCD
	scale_set_in_out_size(scale_dev,iw,ih,SCALE_WIDTH,SCALE_HIGH);
	scale_set_step(scale_dev,iw,ih,SCALE_WIDTH,SCALE_HIGH);
	scale_set_start_addr(scale_dev,0,0);
	scale_set_dma_to_memory(scale_dev,0);
	scale_set_data_from_vpp(scale_dev,1);  
	scale_set_line_buf_num(scale_dev,32);       //vpp的line buf
	scale_set_in_yaddr(scale_dev,(uint32)yuvbuf1);
	scale_set_in_uaddr(scale_dev,(uint32)yuvbuf1+iw*32);
	scale_set_in_vaddr(scale_dev,(uint32)yuvbuf1+iw*32+iw*8);	
#else
	
#if LCD_FROM_DMA2D_BUF
	scale_dma2d_init();
	csi_kernel_task_new((k_task_entry_t)scale_to_lcd_updata, "dma2d_thread", NULL, 25, 0, NULL, 2048, &dma2d_task_handle);
	video_dma2d_mem  = video_psram_dma2d_mem;
	video_dma2d_mem1 = video_psram_dma2d_mem1;
	video_dma2d_mem1_cache = video_psram_dma2d_mem1_cache;
#endif

	scale_set_in_out_size(scale_dev,iw,ih,SCALE_WIDTH,SCALE_HIGH);
#if LCD_33_WVGA
	scale_set_step(scale_dev,iw,ih,854,480);
	//scale_set_step(scale_dev,iw,ih,SCALE_WIDTH,640);
#else
	scale_set_step(scale_dev,iw,ih,SCALE_WIDTH,SCALE_HIGH);
#endif
	scale_set_start_addr(scale_dev,0,0);
	scale_set_dma_to_memory(scale_dev,1);
	scale_set_data_from_vpp(scale_dev,1);  
	scale_set_line_buf_num(scale_dev,20);       //vpp的line buf
	scale_is_allframe_or_linebuf(scale_dev,0);
//	scale_set_in_yaddr(scale_dev,(uint32)yuvbuf);
//	scale_set_in_uaddr(scale_dev,(uint32)yuvbuf+photo_msg.in_w*32);
//	scale_set_in_vaddr(scale_dev,(uint32)yuvbuf+photo_msg.in_w*32+photo_msg.in_w*8);
	scale_set_in_yaddr(scale_dev,(uint32)yuvbuf1);
	scale_set_in_uaddr(scale_dev,(uint32)yuvbuf1+iw*20);
	scale_set_in_vaddr(scale_dev,(uint32)yuvbuf1+iw*20+iw*5);
#if LCD_FROM_DMA2D_BUF
	scale_set_out_yaddr(scale_dev,(uint32)video_dma2d_mem);
	scale_set_out_uaddr(scale_dev,(uint32)video_dma2d_mem+SCALE_CONFIG_W*SCALE_HIGH);
	scale_set_out_vaddr(scale_dev,(uint32)video_dma2d_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
#else
	scale_set_out_yaddr(scale_dev,(uint32)video_psram_mem);
	scale_set_out_uaddr(scale_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH);
	scale_set_out_vaddr(scale_dev,(uint32)video_psram_mem+SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/4);
#endif

#endif
#if LCD_THREE_BUF
	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale3_done,(uint32)scale_dev);	
#elif LCD_FROM_DMA2D_BUF
	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale3_dma2d_done,(uint32)scale_dev);
#else
	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale3_doublebuf_done,(uint32)scale_dev);	
#endif
	scale_request_irq(scale_dev,INBUF_OV,(scale_irq_hdl )&scale3_ov_isr,(uint32)scale_dev);
	scale_request_irq(scale_dev,ERROR_PEND,(scale_irq_hdl )&scale3_error_pending_isr,(uint32)scale_dev);
	scale_open(scale_dev);
}


void scale3_to_memory_for_thumb(uint16_t iw,uint16_t ih,uint16_t ow,uint16_t oh,uint32_t outadr,uint32_t yuvsram,uint16_t lanenum){
	struct scale_device *scale_dev;
	scale_dev = (struct scale_device *)dev_get(HG_SCALE3_DEVID);

	scale_set_in_out_size(scale_dev,iw,ih,ow,oh);
	scale_set_step(scale_dev,iw,ih,ow,oh);

	scale_set_start_addr(scale_dev,0,0);
	scale_set_dma_to_memory(scale_dev,1);
	scale_set_data_from_vpp(scale_dev,1);  
	scale_set_line_buf_num(scale_dev,lanenum);       //vpp的line buf
	scale_is_allframe_or_linebuf(scale_dev,0);	
	
	scale_set_in_yaddr(scale_dev,(uint32)yuvsram);
	scale_set_in_uaddr(scale_dev,(uint32)yuvsram+iw*lanenum);
	scale_set_in_vaddr(scale_dev,(uint32)yuvsram+iw*lanenum+iw*lanenum/4);	

	scale_set_out_yaddr(scale_dev,(uint32)outadr);
	scale_set_out_uaddr(scale_dev,(uint32)outadr+ow*oh);
	scale_set_out_vaddr(scale_dev,(uint32)outadr+ow*oh+ow*oh/4);
	
	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale3_thumb_done,(uint32)scale_dev);	
	scale_request_irq(scale_dev,INBUF_OV,(scale_irq_hdl )&scale3_ov_isr,(uint32)scale_dev);
	scale_request_irq(scale_dev,ERROR_PEND,(scale_irq_hdl )&scale3_error_pending_isr,(uint32)scale_dev);

	scale_open(scale_dev);
}

//input_format 0:yuv  1:rgb    output_format  0:yuv   1:rgb
void scale3_all_frame(struct scale_device *scale_dev,uint32_t in_w,uint32_t in_h,uint32_t out_w,uint32_t out_h,uint8_t input_format,uint8_t output_format,uint32_t src,uint32_t dst){
	uint32_t ow_n,oh_n;
	scale_close(scale_dev);	
	scale_set_dma_to_memory(scale_dev,1);
	scale_set_data_from_vpp(scale_dev,0); 	
	scale_is_allframe_or_linebuf(scale_dev,1);
	scale_set_input_format(scale_dev,input_format);
	scale_set_output_format(scale_dev,output_format);
	if(input_format == 0){
		scale_set_in_yaddr(scale_dev,(uint32)src);
		scale_set_in_uaddr(scale_dev,(uint32)src+in_w*in_h);
		scale_set_in_vaddr(scale_dev,(uint32)src+in_w*in_h+in_w*in_h/4);
	}else{
		scale_set_in_yaddr(scale_dev,(uint32)src);
		scale_set_in_uaddr(scale_dev,(uint32)src+in_w*in_h);
		scale_set_in_vaddr(scale_dev,(uint32)src+in_w*in_h*2);
	}
	
	if(output_format == 0){
		scale_set_out_yaddr(scale_dev,(uint32)dst);
		scale_set_out_uaddr(scale_dev,(uint32)dst+out_w*out_h);
		scale_set_out_vaddr(scale_dev,(uint32)dst+out_w*out_h+out_w*out_h/4);
	}else{
		scale_set_out_yaddr(scale_dev,(uint32)dst);
		scale_set_out_uaddr(scale_dev,(uint32)dst+out_w*out_h);
		scale_set_out_vaddr(scale_dev,(uint32)dst+out_w*out_h*2);
	}

	
	scale_set_in_out_size(scale_dev,in_w,in_h,out_w,out_h);
	ow_n = in_w;
	oh_n = in_h;		
	scale_set_step(scale_dev,ow_n,oh_n,out_w,out_h);
	in_w = (in_w - ow_n)/2; 
	in_h = (in_h - oh_n)/2;
	scale_set_start_addr(scale_dev,in_w,in_h);

	
	scale_request_irq(scale_dev,FRAME_END,(scale_irq_hdl )&scale3_allframe_done,(uint32)scale_dev);	
	scale_request_irq(scale_dev,INBUF_OV, (scale_irq_hdl )&scale3_ov_isr,(uint32)scale_dev);
	scale_open(scale_dev);
	scale_set_new_frame(scale_dev,1);
}

