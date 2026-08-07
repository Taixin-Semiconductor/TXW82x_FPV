#include "sys_config.h"	
#include "tx_platform.h"
#include "osal/string.h"
#include <csi_kernel.h>

#include "stream_frame.h"
#include "osal/task.h"
#include "osal_file.h"
#include "video_app/video_app.h"
#include "lwip/api.h"
#include "lwip/sockets.h"
#include "lwip/etharp.h"
#include "utlist.h"
#include "jpgdef.h"
#include "lib/lcd/lcd.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/tcpip.h"
#include "netif/ethernetif.h"
#include "dhcpd_eloop/dhcpd_eloop.h"
#include "hal/gpio.h"
#include "hal/pwm.h"
#include "hal/uart.h"
#include "hal/csc.h"
#include "dev/csc/hgcsc.h"
#include "lib/video/dvp/jpeg/jpg.h"

 
#define MIN(a,b) ( (a)>(b)?b:a )
#define MAX(a,b) ( (a)>(b)?a:b )
#define H264_DIR "0:/DCIM"
#define H264_FILE_NAME "264"

uint8_t name_rec_photo_h264[32];
static uint32_t a2i(char *str)
{
    uint32_t ret = 0;
    uint32_t indx =0;
	char str_buf[32];
	memset(str_buf,0,32);
	while(str[indx] != '\0'){
		if(str[indx] == '.') break;
		str_buf[indx] = str[indx];
		indx++;
	}
	printf("str_buf:%s  str:%s\r\n",str_buf,str);
	indx = 0;
    while(str_buf[indx] != '\0'){
        if(str_buf[indx] >= '0' && str_buf[indx] <='9'){
            ret = ret*10+str_buf[indx]-'0';
        }
        indx ++;
    }
    return ret;
}

static void *creat_h264_file(char *dir_name)
{
    DIR  avi_dir;
    FRESULT ret;
    FILINFO finfo;
	uint32_t indx= 0;
	void *fp;
	char filepath[64];
    ret = f_opendir(&avi_dir,dir_name);
    if(ret != FR_OK){
        f_mkdir(dir_name);
        f_opendir(&avi_dir,dir_name);
    }
    while(1){
        ret = f_readdir(&avi_dir,&finfo);
        if(ret != FR_OK || finfo.fname[0] == 0) break;
        indx = MAX(indx,a2i(finfo.fname));
    }
    f_closedir(&avi_dir);    
    indx++;

	sprintf(filepath,"%s/h%d.%s",dir_name,indx,H264_FILE_NAME);
	printf("filepath:%s\r\n",filepath);
	fp = osal_fopen(filepath,"wb+");
	return fp;
}


static int opcode_func(stream *s,void *priv,int opcode)
{
	int res = 0;
	//_os_printf("%s:%d\topcode:%d\n",__FUNCTION__,__LINE__,opcode);
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
		default:
			//Ä¬ÈÏ¶¼·µ»Ø³É¹¦
		break;
	}
	return res;
}

uint8_t uart4_buf_h264[16];
uint8_t uart4_buf_h264_result[16];
static struct os_semaphore uart4_buf_sem = {0,NULL};
static struct os_semaphore h264_buf_sem = {0,NULL};

void uart_user_sema_init()
{
	os_sema_init(&uart4_buf_sem,0);
}

void uart_user_sema_down(int32 tmo_ms)
{
	os_sema_down(&uart4_buf_sem,tmo_ms);
}

void uart_user_sema_up()
{
	os_sema_up(&uart4_buf_sem);
}


void h264_user_sema_init()
{
	os_sema_init(&h264_buf_sem,0);
}

void h264_user_sema_down(int32 tmo_ms)
{
	os_sema_down(&h264_buf_sem,tmo_ms);
}

void h264_user_sema_up()
{
	os_sema_up(&h264_buf_sem);
}


volatile uint8_t scale3_oe_select = 0;
int32 uart_user_handler_irq(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2)
{
	struct uart_device *rx_uart = (struct uart_device *)dev_get(HG_UART1_DEVID);
	memcpy(uart4_buf_h264_result,uart4_buf_h264,16);
	if(UART_IRQ_FLAG_TIME_OUT == irq){
		uart_gets(rx_uart,uart4_buf_h264,16);
		if(uart4_buf_h264[0]!=0xff){
			//rx_send_flag=1;
		}
		
		printf("uart4_buf_result:%d\n",uart4_buf_h264_result[0]);
		if(uart4_buf_h264_result[0] == 0xfa){
			if(scale3_oe_select == 0){
				scale3_oe_select = 1;
			}else{
				scale3_oe_select = 0;
			}
		}else
	    {
			uart_user_sema_up();
		}
	}

	if(UART_IRQ_FLAG_DMA_RX_DONE == irq){
		uart_gets(rx_uart,uart4_buf_h264,16);
		//printf("uart4_buf111:%d\n",uart4_buf[0]);
		
		uart_user_sema_up();
	}
	return 0;
}


void h264_wakeup_from_net(){
	uart_user_sema_up();
}

void uart_user_rx_config(){
	uart_user_sema_init();
#if 1
	struct uart_device *rx_uart = (struct uart_device *)dev_get(HG_UART1_DEVID);
	//uart_open(rx_uart,921600);
	uart_open(rx_uart,921600);
	uart_ioctl(rx_uart,UART_IOCTL_CMD_SET_TIME_OUT,9216/5,1);
	uart_ioctl(rx_uart,UART_IOCTL_CMD_USE_DMA,1,0);	//ÒªÊÖ¶¯¿ªÆôÊÇ·ñÎªdma
	uart_request_irq(rx_uart,uart_user_handler_irq,UART_IRQ_FLAG_TIME_OUT|UART_IRQ_FLAG_DMA_RX_DONE,(uint32_t)64);
	uart_gets(rx_uart,uart4_buf_h264,16);
#endif	
}

uint32 h264_write_len = 0;
uint8 *buf1_goble; 
uint8 *buf2_goble;
uint8 *buf_save_goble; 
uint8 buf_select = 0;



uint32_t h264_sram_buf[512];
void h264_write_sdcard_thread(uint32 arg_bf){
	void *fp;
	uint8_t w_card = 1;
	uint32_t offset_wr = 0;
	
	uint32_t ofset_len = 0;


retry_create:	
	fp = creat_h264_file(H264_DIR);
	if(fp == NULL){
		printf("create file 264 error\r\n");
		w_card = 0;
	}else{
		printf("create 264 file ok\r\n");
		w_card = 1;
	}

	ofset_len = 0;
	
	
	while(1){
		h264_user_sema_down(-1);
		offset_wr = 0;
		if(w_card){
			printf("S(%d)",h264_write_len);
			ofset_len += h264_write_len;
			if(buf_select == 0){
				osal_fwrite(buf2_goble,1,h264_write_len,fp);
			}
			else{
				osal_fwrite(buf1_goble,1,h264_write_len,fp);
			}
			
			printf("E(%d)",ofset_len);
			
			if(ofset_len > 10*1024*1024){
				osal_fclose(fp);
				goto retry_create;
			}
		}
		

	}
}


extern volatile uint8 h264_error;

void h264_user_demo_thread(){
	struct h264_device *h264_dev;
	struct data_structure *get_f = NULL;
    struct stream_h264_data_s *dest_list;
    struct stream_h264_data_s *dest_list_tmp;
	struct stream_h264_data_s *el,*tmp;
	k_task_handle_t h264_write_task_handle;
	uint8  file_num = 0;
	uint8 *buf_cache; 
	uint32 cache_len = 0;
	uint32 sd_save_len = 0;
	uint32 cache_num = 0;
	uint32 itk = 0;
	//uint8_t cnt_264;
	//uint8_t wdat;
	stream *s = NULL;
	void *fp;
	uint32_t node_len;
	uint32_t total_len = 0;
	FRESULT res;
	FATFS *fs;
	DWORD fre_clust, fre_sect, tot_sect;
	uint32_t flen;
	uint32_t ofset_len = 0;
	uint32_t num = 0;
	uint8_t w_card = 1;
	uint8_t w_cache_run = 0;
	uint8_t *h264_buf_addr = NULL;	
	buf1_goble = (uint8*)av_psram_malloc(512*1024);
	buf2_goble = (uint8*)av_psram_malloc(512*1024);
	//buf_save_goble = (uint8*)av_psram_malloc(2*1024*1024);
	h264_dev = (struct h264_device *)dev_get(HG_H264_DEVID);	
	uart_user_rx_config();
	h264_user_sema_init();
	//csi_kernel_task_new((k_task_entry_t)h264_write_sdcard_thread, "h264_write_card_thread", NULL, 25, 0, NULL, 2048, &h264_write_task_handle);
	mp4_demo();
	
//h264_write_card:	
	

	uart_user_sema_down(-1);
	printf("uart up..........................................................................................................\r\n");
	s = open_stream_available(R_RECORD_H264,0,8,opcode_func,NULL);
h264_write_card:		
	file_num++;
	h264_set_oe_select(h264_dev,1,file_num%2);
	h264_enc(VPP_DATA0,1280,720,0,0,0);
	//h264_enc(SCALER_DATA,1920,1088,0,0,0);

	ofset_len = 0;

	//cnt_264 = 0;
	//wdat = 0x55;


	cache_len = 0;
	cache_num = 0;
	w_cache_run = 0;
	buf_select = 0;
	start_h264();
	while(1){
		get_f = recv_real_data(s);
		if(get_f){
			printf("get_f(%08x)",get_f);
			dest_list = (struct stream_h264_data_s *)GET_DATA_BUF(get_f);
			dest_list_tmp = dest_list;
			flen = get_stream_real_data_len(get_f);
			node_len = GET_NODE_LEN(get_f);
			h264_buf_addr = (uint8_t *)GET_JPG_BUF(get_f);
			
#if 0
			for(itk = 0;itk < flen;itk++){
				h264_buf_addr[itk] = wdat;
				cnt_264++;
				if((cnt_264%4) == 0){
					wdat = 0x55;
					cnt_264 = 0;
				}
				else if((cnt_264%4) == 1)
					wdat = 0xAA;
				else if((cnt_264%4) == 2)
					wdat = 0x66;
				else if((cnt_264%4) == 3){
					wdat = 0xCC;
				}
			}
#endif			
//			printf("addr:%08x  len:%d\r\n",h264_buf_addr,flen);
			if(buf_select == 0)
				hw_memcpy(buf1_goble + cache_len,h264_buf_addr,flen);
			else
				hw_memcpy(buf2_goble + cache_len,h264_buf_addr,flen);
			

			printf("F(%d)",flen);
			
			sd_save_len += flen;
			cache_len += flen;
			cache_num++;			
			if(cache_num == 8){
				w_cache_run = 1;
			}
			
			if(w_cache_run)
			{
				if(buf_select == 0)
					printf("\r\nSBUF1\r\n");
				else
					printf("\r\nSBUF2\r\n");
				
				buf_select ^= BIT(0);
				h264_write_len = cache_len;
				h264_user_sema_up();
				printf("W(%d)",h264_write_len);
				printf("ofset_len:%x\r\n",ofset_len);
				ofset_len += cache_len;
				
				if(ofset_len > 10*1024*1024){
					w_card = 0;
					printf("CARD END:%d",ofset_len);
					stop_h264();
					free_data(get_f);
					while(get_f)
					{
						get_f = recv_real_data(s);
						free_data(get_f);
					}
					get_f = NULL;
					os_sleep_ms(1000);
					goto h264_write_card;
				}
			}

			if(cache_num == 8){
				cache_num = 0;
				cache_len = 0;
				w_cache_run = 0;
			}
			
			printf("free(%08x)",get_f);
			free_data(get_f);
			get_f = NULL;

		}else{
			
			if(h264_error == 1){
				w_card = 0;
				printf("CARD END");
				stop_h264();
				osal_fclose(fp);
				free_data(get_f);
				get_f = NULL;
				os_sleep_ms(1000);
				goto h264_write_card;
			}
			
			os_sleep_ms(10);
		}
	}
}

volatile uint8 csc_finish = 0;
void csc_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	csc_finish = 1;	
}

__psram_data uint8 h264_dec_file[1*1024*1024] __aligned(32);
extern uint8 *video_decode_mem;
extern uint32 p1_w,p1_h;
extern uint32 scale_p1_w;

void data_deal(uint8_t *src,uint8_t *des,uint32_t w,uint32_t h){
	uint8_t *r;
	uint8_t *g;
	uint8_t *b;
	uint32_t i;
	r = src;
	g = &src[w*h];
	b = &src[2*w*h];
	printf("r:%08x   g:%08x   b:%08x   adr:%08x \r\n",r,g,b,des );
	for(i = 0;i < w*h;i++){

		des[i*3]   = b[i];
		des[i*3+1] = g[i];
		des[i*3+2] = r[i];	
	
	}
}

void h264_user_demo_dec_thread(){
	void *fp;
	uint8_t *buf;
	uint32_t offset_sps_start = 0;
	uint8_t *buf2; 
	uint8_t *buf3;
	uint32_t itk = 0;
	uint32_t offset_sps_len = 0;
	uint32_t data_count;
	uint8_t frame_start_find;
	uint8_t *photo_sd_cache = NULL;
	uint32_t data_len;
	uint32_t file_ip_len;
	uint32_t offset_len;
	uint32_t tmep;
	uint8_t name[16];
	uint8_t *pt8;
	uint8_t *pt8_dec;
	struct h264_device *h264_dev;
	struct csc_device *csc_dev;
	struct scale_device *scale2_dev;
	struct scale_device *scale3_dev;
	csc_dev = (struct csc_device *)dev_get(HG_CSC_DEVID);
	h264_dev = (struct h264_device *)dev_get(HG_H264_DEVID);	
	scale2_dev = (struct scale_device *)dev_get(HG_SCALE2_DEVID);	
	scale3_dev = (struct scale_device *)dev_get(HG_SCALE3_DEVID);	
	os_printf("csc_dev :%x\r\n", csc_dev);
	scale_from_h264_config(scale2_dev,1280,720,320,240,10);
	//scale_from_h264_config(scale2_dev,1280,720,320,240,10);
	jpeg_file_get(name,0,"H");
	printf("name:%s\r\n",name);
	sprintf((char *)name_rec_photo_h264,"%s%s","0:DCIM/",name);
	fp = osal_fopen((const char*)name_rec_photo_h264,"rb");
	if(!fp){
		_os_printf("f_open %s error\r\n",name_rec_photo_h264);
		goto h264_dec_end;
	}
	data_len = osal_fsize(fp);
	_os_printf("data_len:%d\r\n",data_len);
	photo_sd_cache = malloc(1024);

	data_count = 1024;
	frame_start_find = 0;
	file_ip_len = 0;
	pt8_dec = h264_dec_file;
	while(data_len != 0){
		if(data_len > 1024){
			osal_fread(photo_sd_cache,1,data_count,fp);
			data_len = data_len - 1024;
		}
		else{
			osal_fread(photo_sd_cache,1,data_len,fp);		
			data_count = data_len; 
			data_len = 0;
		}

		
		
		pt8 = photo_sd_cache;
		for(itk = 0;itk < data_count;itk++){
			if(pt8[itk] == 0x00){
				//if((pt8[itk+1] == 0x0100)&&(pt8[itk+2] == 0x4d67))
				if((pt8[itk+1] == 0x00)&&(pt8[itk+2] == 0x00)&&(pt8[itk+3] == 0x01)&&(pt8[itk+4] == 0x67)&&(pt8[itk+5] == 0x4d)){

					if(offset_sps_start == 1){
						h264_dec_file[offset_sps_len] = 0x00;
						h264_dec_file[offset_sps_len+1] = 0x00;
						h264_dec_file[offset_sps_len+2] = 0x00;
						h264_dec_file[offset_sps_len+3] = 0x01;						
						sys_dcache_clean_range(h264_dec_file,offset_sps_len+4);
						printf("dec len:%d\r\n",offset_sps_len);
						h264_dec_src_264(h264_dec_file,offset_sps_len,1280,720);
						printf("dec end close\r\n");
					}

				
					offset_sps_start = 1;
					offset_sps_len = 0;
#if 0					
					if(frame_start_find == 0){
						frame_start_find = 1;
					}else{
						frame_start_find = 0;
						offset_len = data_count - itk*2;
						
					}
#endif					
				}
			}

			if(offset_sps_start == 1){
				pt8_dec[offset_sps_len] = pt8[itk];
				offset_sps_len++;	
			}

			
 		}

#if 0 
		if(frame_start_find == 0){
			tmep = data_count - offset_len;
			hw_memcpy(h264_dec_file+file_ip_len,photo_sd_cache,tmep);
			file_ip_len = file_ip_len+tmep;
			os_printf("file_ip_len:%d\r\n",file_ip_len);
			h264_dec_file[file_ip_len] = 0x00;
			h264_dec_file[file_ip_len+1] = 0x00;
			h264_dec_file[file_ip_len+2] = 0x00;
			h264_dec_file[file_ip_len+3] = 0x01;
			sys_dcache_clean_range(h264_dec_file,file_ip_len);
			h264_dec_src_264(h264_dec_file,file_ip_len,1280,720);
			//h264_dec_src_264(h264_dec_file,file_ip_len,1280,720);	
			hw_memcpy(h264_dec_file,photo_sd_cache+tmep,offset_len);
			file_ip_len = offset_len;
			frame_start_find = 1;
			
		}else{
			hw_memcpy(h264_dec_file+file_ip_len,photo_sd_cache,data_count-offset_sps);
			//printf("data_count:%d\r\n",data_count);
			file_ip_len += data_count;		
		}
#endif

	}
	osal_fclose(fp);
	
	

h264_dec_end:
	buf = (uint8*)av_psram_malloc(scale_p1_w*p1_h*3);
	buf3 = (uint8*)av_psram_malloc(scale_p1_w*p1_h*3);
	buf2= (uint8*)av_psram_malloc(160*120*3);
	//buf2 = (uint8*)av_psram_malloc(scale_p1_w*p1_h*3);
	
	
#if 1
	csc_init(csc_dev);
	csc_set_type(csc_dev, 1);
	csc_set_photo_size(csc_dev, 320, 240);
	csc_set_format(csc_dev, CSC_YUV420P, CSC_YUV444P);
	csc_set_input_addr(csc_dev,video_decode_mem,video_decode_mem+scale_p1_w*p1_h,video_decode_mem+scale_p1_w*p1_h+scale_p1_w*p1_h/4);
	csc_set_output_addr(csc_dev,buf,buf+scale_p1_w*p1_h,buf+scale_p1_w*p1_h*2);
	csc_request_irq(csc_dev,CSC_DONE_IRQ,(csc_irq_hdl )&csc_done,(uint32)csc_dev);
	csc_finish = 0;
	printf("csc kick run..\r\n");
	csc_start_run(csc_dev);
	while(csc_finish == 0);
	printf("yuv420P  %08x====> yuv444P finish  %08x\r\n",video_decode_mem,buf);
	csc_init(csc_dev);
	csc_set_type(csc_dev, 0);
	csc_set_format(csc_dev, CSC_YUV444P, CSC_RGB565);
	csc_set_input_addr(csc_dev,buf,buf+scale_p1_w*p1_h,buf+scale_p1_w*p1_h*2);
	csc_set_output_addr(csc_dev,buf3,buf3+scale_p1_w*p1_h,buf3+scale_p1_w*p1_h*2);
	csc_finish = 0;
	csc_start_run(csc_dev);
	while(csc_finish == 0);	
	printf("yuv444P  %08x ====> rgb565 finish  %08x\r\n",buf,buf3);
	scale2_all_frame(scale2_dev,FRAME_YUV420P,320,240,160,120,video_decode_mem,buf2);
	printf("yuv420P  %08x ====> yuv420p finish  %08x\r\n",video_decode_mem,buf2);
#else	
	scale3_all_frame(scale3_dev,320,240,160,120,0,1,video_decode_mem,buf);
	os_sleep_ms(10);
	scale2_all_frame(scale2_dev,FRAME_RGB888P,160,120,80,80,buf,buf2);
#endif
	
	while(1){
		os_sleep_ms(1000);
	}
}

void user_hardware_config()
{
	k_task_handle_t h264_task_handle;
	
	//csi_kernel_task_new((k_task_entry_t)h264_user_demo_dec_thread, "h264_thread", NULL, 25, 0, NULL, 2048, &h264_task_handle);
	csi_kernel_task_new((k_task_entry_t)h264_user_demo_thread, "h264_thread", NULL, 25, 0, NULL, 2048, &h264_task_handle);
}


