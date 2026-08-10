#include "sys_config.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "lib/video/mipi_csi/mipi_csi.h"
#include "lib/video/vpp/vpp_dev.h"
#include "hal/h264.h"
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
#include "lib/video/dual/dual_org_dev.h"


#define DUAL_MALLOC                   av_psram_malloc
#define DUAL_FREE                     av_psram_free
#define DUAL_ZALLOC                   av_psram_zalloc

static uint8_t *psram_photo_buf  = NULL;
static uint8_t *psram_photo_buf1 = NULL;
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

void dorg_double_sensor(uint32 src0_w,uint32 src0_h,uint32 src1_w,uint32 src1_h,uint32 src0_fmt,uint32 src1_fmt,uint8_t dvp_role, uint8_t csi0_role, uint8_t csi1_role)
{
    struct dual_device *dual_dev;
	struct jpg_device *jpeg_dev;
	struct isp_device *isp_dev;
    uint8_t *psram_frame_buf0;
    uint8_t *psram_frame_buf1;
	uint32  buf0_size = 0;
	uint32  buf1_size = 0;
	uint8_t type_master = 0;
	uint8_t type_slave = 0;
	
	isp_dev  = (struct isp_device  *)dev_get(HG_ISP_DEVID);
	dual_dev = (struct dual_device *)dev_get(HG_DUALORG_DEVID);
	jpeg_dev = (struct jpg_device *)dev_get(HG_JPG0_DEVID);

    if (src0_fmt == YUV422) {
        buf0_size = src0_w * src0_h * 2;
    } else {
        buf0_size = src0_w * src0_h * (8 + (src0_fmt - 1) * 2) / 8;
    }

    if (src1_fmt == YUV422) {
        buf1_size = src1_w * src1_h * 2;
    } else {
        buf1_size = src1_w * src1_h * (8 + (src1_fmt - 1) * 2) / 8;
    }

	psram_frame_buf0  = (uint8_t *)av_psram_malloc(buf0_size);
	psram_frame_buf1  = (uint8_t *)av_psram_malloc(buf1_size);

    if (!psram_frame_buf0 || !psram_frame_buf1) {
		os_printf(KERN_ERR"%s malloc size : %d %d err!", __func__, buf0_size , buf1_size);
        return;
    }
	os_printf("dual_org psram addr:%08x %08x size:%d %d\r\n", psram_frame_buf0, psram_frame_buf1, buf0_size, buf1_size);

	// 设备存在，则更新实际类型
	if(video_msg.dvp_type  != 0)    video_msg.dvp_type	= dvp_role;
	if(video_msg.csi0_type != 0)    video_msg.csi0_type = csi0_role;
	if(video_msg.csi1_type != 0)    video_msg.csi1_type = csi1_role;

	if(video_msg.dvp_type == 1)         type_master = IN_DVP0;
	else if(video_msg.csi0_type == 1)   type_master = IN_MIPI_CSI0;
	else if(video_msg.csi1_type == 1)   type_master = IN_MIPI_CSI1;
	
	if(video_msg.dvp_type == 2)         type_slave = IN_DVP0;
	else if(video_msg.csi0_type == 2)   type_slave = IN_MIPI_CSI0;
	else if(video_msg.csi1_type == 2)   type_slave = IN_MIPI_CSI1;

	dual_init(dual_dev);
	dual_input_type(dual_dev,0,type_master);        //主 dvp
	dual_input_type(dual_dev,1,type_slave);   
//	dual_input_type(dual_dev,1,IN_DVP0);   //副 mipi
	dual_work_mode(dual_dev,0);
	//dual_input_src_num(dual_dev,2);
	dual_input_src_num(dual_dev,(video_msg.video_num>1)?2:1);
	dual_open_hdr(dual_dev,0);
	dual_size_cfg(dual_dev,src0_w,src1_w,src0_fmt,src1_fmt);

#if 0 //master:gc1084, slave:gc1084
	dual_timer_cfg(dual_dev,32,2,512); //读的速度
	dual_fs_trig(dual_dev,420);//主摄计数420出fysnc给副
	dual_rd_trig(dual_dev,422,425); //计数422主开始读，计数425行副开始读

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
//	//master:gc2053, slave:jxv03
//	dual_timer_cfg(dual_dev,32,1,256);	
//	dual_fs_trig(dual_dev,734);
//	dual_rd_trig(dual_dev,300,400);
	
	//gc2053+xs9950,isp=384MHz
	dual_timer_cfg(dual_dev,32,2,256);	
	dual_fs_trig(dual_dev,525);
	dual_rd_trig(dual_dev,525,500-50);

#endif
	//dual_timer_cfg(dual_dev,16,20,500);	
	//dual_rd_trig(dual_dev,16,336);	
#endif

#if 0//gc1084+xs9950,isp=384MHz
	dual_timer_cfg(dual_dev,32,3,256);	
	dual_fs_trig(dual_dev,500);
	dual_rd_trig(dual_dev,500,500-50);
#endif

	dual_wr_cnt_cfg(dual_dev,src0_w,src0_h,src1_w,src1_h,src0_fmt,src1_fmt);
	dual_set_addr(dual_dev,(uint32_t)psram_frame_buf0,(uint32_t)psram_frame_buf1);
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


void dorg_double_sensor_deinit()
{
	struct dual_device* dual_dev = (struct dual_device *)dev_get(HG_DUALORG_DEVID);
	dual_close(dual_dev);

	if (psram_photo_buf) {
		DUAL_FREE(psram_photo_buf);
		psram_photo_buf = NULL;
	}

	if (psram_photo_buf1) {
		DUAL_FREE(psram_photo_buf1);
		psram_photo_buf1 = NULL;
	}
}

void dual_org_debug_config(uint8 dbg_io0,uint8 dbg_io1,uint8 dbg_io2,uint8 dbg_io3)
{
    sysctrl_ace_peris_access_cpu_all(ACE_MIX_TOP|ACE_GPIO_TOP|ACE_EFUSE_CTRL|ACE_SYS_SEC_TOP|ACE_PMU|ACE_BASEBAND1|ACE_BASEBAND|ACE_RFDIGITAL|ACE_RFDIGCAL_TOP);

    SYSCTRL_REG_OPT(
        SYSCTRL->SYS_CON0 |= BIT(3);
    	SYSCTRL->SYS_CON1 |= BIT(21);
    	SYSCTRL->CLK_CON2 |= BIT(22);
    );
	gpio_iomap_output(dbg_io0, GPIO_IOMAP_OUT_DBGPATH_DBGO_0); //dbg0
	gpio_iomap_output(dbg_io1, GPIO_IOMAP_OUT_DBGPATH_DBGO_1); //dbg1
	gpio_iomap_output(dbg_io2, GPIO_IOMAP_OUT_DBGPATH_DBGO_2); //dbg2
	gpio_iomap_output(dbg_io3, GPIO_IOMAP_OUT_DBGPATH_DBGO_3); //dbg3
 
	//dbg0
    *(volatile uint32 *)0x40062ef0 &= ~(0xff<<0); 
    *(volatile uint32 *)0x40062ef0 |= (5)<<0; //5:dual_wr0
    //dbg1
    *(volatile uint32 *)0x40062ef0 &= ~(0xff<<8); 
    *(volatile uint32 *)0x40062ef0 |= (6)<<8; //6:dual_wr1
    //dbg2
    *(volatile uint32 *)0x40062ef0 &= ~(0xff<<16); 
    *(volatile uint32 *)0x40062ef0 |= (7)<<16;//7:dual_rd
    //dbg3
    *(volatile uint32 *)0x40062ef0 &= ~(0xff<<24); 
    *(volatile uint32 *)0x40062ef0 |= (8)<<24;//8:dual_free, 正常情况这个IO不会翻转
}