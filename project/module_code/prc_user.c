#include "sys_config.h"	
#include "tx_platform.h"
#include "osal/string.h"

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
#include "lib/video/dvp/jpeg/jpg.h"
#include "lib/video/vpp/vpp_dev.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "hal/csc.h"
#include "dev/csc/hgcsc.h"
#include "dev/prc/hgprc.h"
#include "hal/jpeg.h"



extern Vpp_stream photo_msg;
extern uint8 video_psram_config_mem[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2];
volatile uint8_t scale3_oe_select = 0;
volatile uint8_t *prc_buf;
volatile uint8_t *prc_line_buf;
volatile uint8_t prc_num = 0;


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
			//默认都返回成功
		break;
	}
	return res;
}

void prc_done_isr(uint32 irq,uint32 dev,uint32  param){

}

volatile uint8 csc_finish = 0;
void csc_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	csc_finish = 1;	
}

void jpg_prc_pixel_done(uint32 irq_flag,uint32 irq_data,uint32 param1,uint32 param2){
	struct prc_device *prc_dev;
	prc_dev    = (struct prc_device *)dev_get(HG_PRC_DEVID);
	//printf("P:(%d)",prc_num);
	hw_memcpy(prc_line_buf, prc_buf+prc_num*800*16*2, 800*16*2);
	prc_set_src_addr(prc_dev,prc_line_buf);
	prc_run(prc_dev);
	prc_num++;
	if(prc_num == 480/16){
		//printf("first kick\r\n");
		prc_num = 0;
	}	
}


void jpg_user_demo_thread(){
	uint8_t *buf;
	struct data_structure *get_f = NULL;
    struct stream_jpeg_data_s *dest_list;
    struct stream_jpeg_data_s *dest_list_tmp;
	struct stream_jpeg_data_s *el,*tmp;
	struct scale_device *scale3_dev;
	struct prc_device *prc_dev;
	struct csc_device *csc_dev;
	struct vpp_device *vpp_dev;
	struct jpg_device *jpeg_dev;
	uint8_t *prc_sram;
	uint8_t *prc_linesram;
	csc_dev    = (struct csc_device *)dev_get(HG_CSC_DEVID);
	prc_dev    = (struct prc_device *)dev_get(HG_PRC_DEVID);
	scale3_dev = (struct scale_device *)dev_get(HG_SCALE3_DEVID);
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
	jpeg_dev = (struct jpg_device *)dev_get(HG_JPG0_DEVID);
	stream *s = NULL;
	uint32_t node_len;
	FRESULT res;
	FATFS *fs;
	DWORD fre_clust, fre_sect, tot_sect;
	uint32_t flen;
	uint32_t num = 0;
	uint8_t *jpeg_buf_addr = NULL;
	void *fp;
	
	os_sleep_ms(10000);
	buf = (uint8*)av_psram_malloc(800*480*2);

	csc_init(csc_dev);
	csc_set_type(csc_dev, 1);
	csc_set_photo_size(csc_dev, 800, 480);
	csc_set_format(csc_dev, CSC_YUV420P, CSC_YUYV422);
	csc_set_input_addr(csc_dev,video_psram_config_mem,video_psram_config_mem+800*480,video_psram_config_mem+800*480+800*480/4);
	csc_set_output_addr(csc_dev,buf,buf+800*480,buf+800*480+800*480/2);
	csc_request_irq(csc_dev,CSC_DONE_IRQ,(csc_irq_hdl )&csc_done,(uint32)csc_dev);
	csc_finish = 0;
	printf("csc kick run..\r\n");
	csc_start_run(csc_dev);
	while(csc_finish == 0);
	printf("yuv420P  %08x====> yuv422 finish  %08x\r\n",video_psram_config_mem,buf);

	
	printf(".........................................................prc start..................................................................\r\n");
	scale_close(scale3_dev);
	
	prc_init(prc_dev);
	prc_set_width(prc_dev,800);
	prc_sram = malloc(800*16+800*16/2);    //yuv420p
	prc_linesram = malloc(800*16+800*16);  //yuv422
	prc_line_buf = prc_linesram;
	prc_set_yaddr(prc_dev,prc_sram);	
	prc_set_uaddr(prc_dev,prc_sram+16*800);	
	prc_set_vaddr(prc_dev,prc_sram+16*800+4*800);	
	prc_set_yuv_mode(prc_dev,0);
	hw_memcpy(prc_line_buf, buf, 800*16*2);
	prc_set_src_addr(prc_dev,prc_line_buf);
	prc_num++;
	prc_buf = buf;
	prc_request_irq(prc_dev,PRC_ISR,(prc_irq_hdl )&prc_done_isr,prc_dev);

	photo_msg.out0_w = 800;
	photo_msg.out0_h = 480;

	s = open_stream_available(R_RTP_JPEG,0,8,opcode_func,NULL);
	jpg_request_irq(jpeg_dev,(jpg_irq_hdl )&jpg_prc_pixel_done,JPG_IRQ_FLAG_PIXEL_DONE,jpeg_dev);
	start_jpeg();
	
	prc_first_start(prc_dev);
	prc_run(prc_dev);

	while(1){
		get_f = recv_real_data(s);
		if(get_f){
			dest_list = (struct stream_jpeg_data_s *)GET_DATA_BUF(get_f);
			dest_list_tmp = dest_list;
			flen = get_stream_real_data_len(get_f);
			node_len = GET_NODE_LEN(get_f);
			jpeg_buf_addr = (uint8_t *)GET_JPG_BUF(get_f);
			os_printf("**************************************************************************************jpgbuf:%08x  flen:%d\r\n",jpeg_buf_addr,flen);
			
			free_data(get_f);
			get_f = NULL;

		}else{
			os_sleep_ms(10);
		}
	}
}


void user_hardware_config()
{
	k_task_handle_t jpg_task_handle;
	csi_kernel_task_new((k_task_entry_t)jpg_user_demo_thread, "prc_thread", NULL, 25, 0, NULL, 4096, &jpg_task_handle);
}



