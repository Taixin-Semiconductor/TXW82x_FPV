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
volatile uint8_t scale3_oe_select = 0;
extern volatile uint8_t jpg_auto_dec;
void jpg_user_demo_thread(){
	struct data_structure *get_f = NULL;
    struct stream_jpeg_data_s *dest_list;
    struct stream_jpeg_data_s *dest_list_tmp;
	struct stream_jpeg_data_s *el,*tmp;
	struct scale_device *scale_dev;
	struct vpp_device *vpp_dev;
	struct jpg_device *jpeg_dev;
	struct jpg_device *jpeg1_dev;
	scale_dev = (struct scale_device *)dev_get(HG_SCALE2_DEVID);	
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
	jpeg_dev = (struct jpg_device *)dev_get(HG_JPG0_DEVID);
	jpeg1_dev = (struct jpg_device *)dev_get(HG_JPG1_DEVID);
	stream *s = NULL;
	uint32_t node_len;
	FRESULT res;
	FATFS *fs;
	DWORD fre_clust, fre_sect, tot_sect;
	uint32_t flen;
	uint32_t num = 0;
	uint8_t *jpeg_buf_addr = NULL;
	void *fp;
	s = open_stream_available(R_RTP_JPEG,0,8,opcode_func,NULL);
	start_jpeg();
	printf("open jpeg......\r\n");
	 
	//vpp_open(vpp_dev);
#if SCALE2_DIRECT_TO_LCD	
	scale_from_jpeg_config(scale_dev,1,1280,720,480,320,10);
#endif
	while(1){
		get_f = recv_real_data(s);
		if(get_f){
			dest_list = (struct stream_jpeg_data_s *)GET_DATA_BUF(get_f);
			dest_list_tmp = dest_list;
			flen = get_stream_real_data_len(get_f);
			node_len = GET_NODE_LEN(get_f);
			jpeg_buf_addr = (uint8_t *)GET_JPG_BUF(get_f);
			os_printf("**************************************************************************************jpgbuf:%08x  flen:%d\r\n",jpeg_buf_addr,flen);
			//vpp_close(vpp_dev); 
#if SCALE2_DIRECT_TO_LCD	
			//jpg_auto_rekick_cfg(jpeg1_dev,1);
			jpg_auto_dec = 0;
			sys_dcache_clean_range(jpeg_buf_addr,flen);
			jpg_decode_to_lcd(jpeg_buf_addr,1280,720,480,320);			

			while(jpg_auto_dec != 1)
				os_sleep_ms(1);
#endif				
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
	csi_kernel_task_new((k_task_entry_t)jpg_user_demo_thread, "jpg_thread", NULL, 25, 0, NULL, 4096, &jpg_task_handle);
}

