#include "sys_config.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "lib/video/mipi_csi/mipi_csi.h"
#include "lib/video/vpp/vpp_dev.h"
#include "lib/video/h264/h264_drv.h"
#include "devid.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "hal/dual_org.h"
#include "dev/scale/hgscale.h"
#include "dev/dual/hgdual_org.h"
#include "hal/gpio.h"
#include "hal/jpeg.h"
#include "lib/heap/av_psram_heap.h"

volatile uint32_t dual_arg[4];

void dual_org0_done_isr(uint32 irq,uint32 dev,uint32  param){
	struct jpg_device *jpeg_dev;	
	uint32_t *dl_dev;
	dl_dev = (uint32_t *)dev;
	_os_printf("{0}");
	video_msg.video_type_last = video_msg.video_type_cur;
	video_msg.video_type_cur = ISP_VIDEO_0; 
	jpeg_dev = (struct jpg_device  *)dl_dev[1];//dev_get(HG_JPG0_DEVID);
	jpg_set_oe_state(jpeg_dev,1);
//    if (param == 1) dual_save_addr_cfg((void *)dl_dev[0]);

}

void dual_org1_done_isr(uint32 irq,uint32 dev,uint32  param){
	uint32 org1_num;
	static int32_t org_select = 0; 
	struct jpg_device *jpeg_dev;	
    struct dual_device *dual_dev;
	struct isp_device *isp_dev;
	uint32_t *dl_dev;
	dl_dev = (uint32_t *)dev;
	dual_dev = (struct dual_device *)dl_dev[0];//dev_get(HG_DUALORG_DEVID);
	jpeg_dev = (struct jpg_device  *)dl_dev[1];//dev_get(HG_JPG0_DEVID);
	isp_dev  = (struct isp_device  *)dl_dev[2];//dev_get(HG_ISP_DEVID);
	org1_num = dl_dev[3];

	_os_printf("{1}");

	if(org1_num == 2){
        org_select = isp_sensor_slave_index(isp_dev);
		if (org_select == 1)
		{
			dual_input_type(dual_dev,1,IN_MIPI_CSI0);
			video_msg.video_type_last = video_msg.video_type_cur;
			video_msg.video_type_cur  = ISP_VIDEO_2; 			
		} else if (org_select == 2) {
			dual_input_type(dual_dev,1,IN_MIPI_CSI1);
			video_msg.video_type_last = video_msg.video_type_cur;
			video_msg.video_type_cur  = ISP_VIDEO_1; 			
		} else {
            os_printf("sensor slave index : %d err\r\n", org_select);
        }
	}else{
		video_msg.video_type_last = video_msg.video_type_cur;
		video_msg.video_type_cur = ISP_VIDEO_1; 		
	}
	
	jpg_set_oe_state(jpeg_dev,0);
//    if (param == 2) dual_save_addr_cfg((void *)dual_dev);
}

void dual_org_rd_done_isr(uint32 irq,uint32 dev,uint32  param){
	//_os_printf("(rd)");
}

void dual_org_rd_slow_isr(uint32 irq,uint32 dev,uint32  param){
	_os_printf("(rd slow)");
}

void dorg_double_sensor(uint32 src0_w,uint32 src0_h,uint32 src1_w,uint32 src1_h,uint32 src0_raw_num,uint32 src1_raw_num,uint8_t dvp_type,uint8_t csi0_type,uint8_t csi1_type){
    struct dual_device *dual_dev;
	struct jpg_device *jpeg_dev;
	struct isp_device *isp_dev;
    uint8_t *psram_photo_buf;
    uint8_t *psram_photo_buf1;
	uint8_t raw_dw;
	uint8_t type_master = 0;
	uint8_t type_slave = 0;
	
	isp_dev  = (struct isp_device  *)dev_get(HG_ISP_DEVID);
	dual_dev = (struct dual_device *)dev_get(HG_DUALORG_DEVID);
	jpeg_dev = (struct jpg_device *)dev_get(HG_JPG0_DEVID);
	
	raw_dw = 8+(src0_raw_num-1)*2;
	psram_photo_buf  = (uint8_t *)av_psram_malloc((src0_w*src0_h*raw_dw)/8);
	raw_dw = 8+(src1_raw_num-1)*2;
	psram_photo_buf1 = (uint8_t *)av_psram_malloc((src1_w*src1_h*raw_dw)/8);
	_os_printf("psram:%08x  %08x  size:%d\r\n",psram_photo_buf,psram_photo_buf1,(src1_w*src1_h*raw_dw)/8);
	if(video_msg.dvp_type == 1){
		video_msg.dvp_type	= dvp_type;
	}

	if(video_msg.csi0_type == 1){
		video_msg.csi0_type = csi0_type;
	}else if(video_msg.csi1_type == 1){
		video_msg.csi1_type = csi0_type;
	}

	if(video_msg.csi1_type == 1){
		video_msg.csi1_type = csi1_type;
	}
	
	

	if(video_msg.dvp_type == 1){
		type_master = IN_DVP0;
	}else if(video_msg.csi0_type == 1){
		type_master = IN_MIPI_CSI0;
	}else{
		type_master = IN_MIPI_CSI1;
	}

	if(video_msg.dvp_type == 2){
		type_slave = IN_DVP0;
	}else if(video_msg.csi0_type == 2){
		type_slave = IN_MIPI_CSI0;
	}else{
		type_slave = IN_MIPI_CSI1;
	}	
	
	dual_init(dual_dev);
	dual_input_type(dual_dev,0,type_master);        //主 dvp
	dual_input_type(dual_dev,1,type_slave);   
//	dual_input_type(dual_dev,1,IN_DVP0);   //副 mipi
	dual_work_mode(dual_dev,0);
	//dual_input_src_num(dual_dev,2);
	dual_input_src_num(dual_dev,(video_msg.video_num>1)?2:1);
	dual_open_hdr(dual_dev,0);
	dual_size_cfg(dual_dev,src0_w,src1_w,src0_raw_num,src1_raw_num);

#if 0 //master:gc1084, slave:gc1084
	dual_timer_cfg(dual_dev,32,2,512); 
	dual_fs_trig(dual_dev,420);
	dual_rd_trig(dual_dev,422,425); 

#elif 0 //master:2336p, slave:gc1084
	dual_timer_cfg(dual_dev,32,1,256);	
	dual_fs_trig(dual_dev,400);
	dual_rd_trig(dual_dev,330,540);	
#else //master:sc1346, slave:gc1084

#if ISP_TUNNING_EN
	dual_timer_cfg(dual_dev,16,2,256);	
	dual_fs_trig(dual_dev,460);
	dual_rd_trig(dual_dev,10,452);
#else
	dual_timer_cfg(dual_dev,16,2,1000);	
	dual_fs_trig(dual_dev,460);
	dual_rd_trig(dual_dev,475,452);
#endif
	//dual_timer_cfg(dual_dev,16,20,500);	
	//dual_rd_trig(dual_dev,16,336);	
#endif


	dual_wr_cnt_cfg(dual_dev,src0_w,src0_h,src1_w,src1_h,src0_raw_num,src1_raw_num);
	dual_set_addr(dual_dev,(uint32_t)psram_photo_buf,(uint32_t)psram_photo_buf1);
	//dual_set_addr(dual_dev,0x28000000,0x28151800);
	dual_arg[0] = (uint32_t)dual_dev;
	dual_arg[1] = (uint32_t)jpeg_dev;
	dual_arg[2] = (uint32_t)isp_dev;
	//dual_arg[3] = 2;
	dual_arg[3] = (video_msg.video_num>2)?2:1;
	dual_request_irq(dual_dev,ORG0_SD_ISR,(dual_irq_hdl )&dual_org0_done_isr,(uint32_t)dual_arg);
	dual_request_irq(dual_dev,ORG1_SD_ISR,(dual_irq_hdl )&dual_org1_done_isr,(uint32_t)dual_arg);
	dual_request_irq(dual_dev,ORG_RD_DONE_IE,(dual_irq_hdl )&dual_org_rd_done_isr,(uint32_t)dual_arg);
	dual_request_irq(dual_dev,ORG_RD_SLOW_IE,(dual_irq_hdl )&dual_org_rd_slow_isr,(uint32_t)dual_arg);
	dual_open(dual_dev);
}


