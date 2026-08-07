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
#include "lib/video/dvp/jpeg/jpg.h"
#include "lib/video/vpp/vpp_dev.h"
#include "lib/video/dvp/cmos_sensor/csi.h"

#define MIN(a,b) ( (a)>(b)?b:a )
#define MAX(a,b) ( (a)>(b)?a:b )
#define JPG_DIR "0:/DCIM"
#define JPG_FILE_NAME "jpg"


extern Vpp_stream photo_msg;
extern uint8 video_psram_config_mem[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2];
volatile uint8_t scale3_oe_select = 0;

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
	//printf("str_buf:%s  str:%s\r\n",str_buf,str);
	indx = 0;
    while(str_buf[indx] != '\0'){
        if(str_buf[indx] >= '0' && str_buf[indx] <='9'){
            ret = ret*10+str_buf[indx]-'0';
        }
        indx ++;
    }
    return ret;
}


static void *creat_jpg_file(char *dir_name)
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

	sprintf(filepath,"%s/h%d.%s",dir_name,indx,JPG_FILE_NAME);
	//printf("filepath:%s\r\n",filepath);
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
			//默认都返回成功
		break;
	}
	return res;
}

extern uint8 *yuvbuf;
void jpg_user_demo_thread(){
	struct data_structure *get_f = NULL;
    struct stream_jpeg_data_s *dest_list;
    struct stream_jpeg_data_s *dest_list_tmp;
	struct stream_jpeg_data_s *el,*tmp;
	struct scale_device *scale_dev;
	struct scale_device *scale2_dev;
	struct scale_device *scale3_dev;
	struct vpp_device *vpp_dev;
	struct jpg_device *jpeg_dev;
	struct jpg_device *jpeg1_dev;
	scale_dev = (struct scale_device *)dev_get(HG_SCALE1_DEVID);
	scale2_dev = (struct scale_device *)dev_get(HG_SCALE2_DEVID);
	scale3_dev = (struct scale_device *)dev_get(HG_SCALE3_DEVID);
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
	
	os_sleep_ms(5000);
	printf(".........................................................gen422 start..................................................................\r\n");
	scale_close(scale3_dev);
	gen422_init(800,480,1);
	vpp_close(vpp_dev);
	vpp_cfg(800,480,IN_GEN422);
	scale_from_vpp(scale_dev,yuvbuf,800,480,1280,720);
	photo_msg.out0_w = 1280;
	photo_msg.out0_h = 720;

	s = open_stream_available(R_RTP_JPEG,0,8,opcode_func,NULL);
	start_jpeg();


	


	
	gen422_set_frame(video_psram_config_mem,NULL,800,480);
	printf("...kick gen422\r\n");
//	gen422_run();
	while(1){
		get_f = recv_real_data(s);
		if(get_f){
			dest_list = (struct stream_jpeg_data_s *)GET_DATA_BUF(get_f);
			dest_list_tmp = dest_list;
			flen = get_stream_real_data_len(get_f);
			node_len = GET_NODE_LEN(get_f);
			jpeg_buf_addr = (uint8_t *)GET_JPG_BUF(get_f);
			os_printf("**************************************************************************************jpgbuf:%08x  flen:%d\r\n",jpeg_buf_addr,flen);
			fp = creat_jpg_file(JPG_DIR);
			osal_fwrite(jpeg_buf_addr,1,flen,fp);
			osal_fclose(fp);
			free_data(get_f);
			get_f = NULL;
			gen422_run();
		}else{
			os_sleep_ms(10);
		}
	}
}


void user_hardware_config()
{
	k_task_handle_t jpg_task_handle;
	csi_kernel_task_new((k_task_entry_t)jpg_user_demo_thread, "gen422_thread", NULL, 25, 0, NULL, 4096, &jpg_task_handle);
}


