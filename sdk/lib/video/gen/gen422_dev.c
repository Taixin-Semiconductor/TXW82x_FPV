#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "lib/video/dvp/cmos_sensor/csi_V2.h"
#include "devid.h"
#include "hal/gpio.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/vpp/hgvpp.h"
#include "dev/csi/hgdvp.h"
#include "dev/gen/hggen422.h"
#include "lib/lcd/lcd.h"
#include "hal/jpeg.h"
#include "lib/video/vpp/vpp_dev.h"
#include "lib/video/gen/gen422_dev.h"
#include "lib/scale/scale_dev.h"


void gen422_done_isr(uint32 irq,uint32 dev,uint32  param){
	//struct gen420_device *gen420_dev = (struct gen420_device *)dev;
	_os_printf("%s  %d\r\n",__func__,__LINE__);
}

void gen422_rdone_isr(uint32 irq,uint32 dev,uint32  param){
	//struct gen420_device *gen420_dev = (struct gen420_device *)dev;
	_os_printf("%s  %d\r\n",__func__,__LINE__);
}

void gen422_rd_slow_isr(uint32 irq,uint32 dev,uint32  param){
	//struct gen420_device *gen420_dev = (struct gen420_device *)dev;
	_os_printf("%s  %d\r\n",__func__,__LINE__);
}

void gen422_wr_slow_isr(uint32 irq,uint32 dev,uint32  param){
	//struct gen420_device *gen420_dev = (struct gen420_device *)dev;
	_os_printf("%s  %d\r\n",__func__,__LINE__);
}

void gen422_ov_isr(uint32 irq,uint32 dev,uint32  param){
	//struct gen420_device *gen420_dev = (struct gen420_device *)dev;
	_os_printf("%s  %d\r\n",__func__,__LINE__);
}

void gen422_init(uint32_t w,uint32_t h,uint8_t sigle_frm){
	struct gen422_device *gen422_dev;
	uint8_t *gen420_sram;
	gen422_dev = (struct gen422_device *)dev_get(HG_GEN422_DEVID);	
	gen422_open(gen422_dev);
	gen422_request_irq(gen422_dev,GEN422_GDONE_ISR,(gen422_irq_hdl )&gen422_done_isr,(uint32)gen422_dev);
	gen422_request_irq(gen422_dev,GEN422_RDONE_ISR,(gen422_irq_hdl )&gen422_rdone_isr,(uint32)gen422_dev);
	gen422_request_irq(gen422_dev,GEN422_RD_SLOW_ISR,(gen422_irq_hdl )&gen422_rd_slow_isr,(uint32)gen422_dev);
	gen422_request_irq(gen422_dev,GEN422_WR_SLOW_ISR,(gen422_irq_hdl )&gen422_wr_slow_isr,(uint32)gen422_dev);
	gen422_request_irq(gen422_dev,GEN422_GEN_OV_ISR,(gen422_irq_hdl )&gen422_ov_isr,(uint32)gen422_dev);
	
	gen420_sram = malloc(w*6);
	gen422_sram_linebuf_adr(gen422_dev,(uint32)gen420_sram,(uint32)gen420_sram+w*4,(uint32)gen420_sram+w*5);
	gen422_yuv_mode(gen422_dev,0);
	gen422_input_from_sram_or_psram(gen422_dev,0);
	gen422_output_single_mode(gen422_dev,sigle_frm);
	gen422_frame_time(gen422_dev,65530,1000/16,16);
	gen422_frame_size(gen422_dev,w,h);

	gen422_output_enable(gen422_dev,1,0);
}

void gen422_set_frame(uint32_t psram0,uint32_t psram1,uint32_t w,uint32_t h){
	struct gen422_device *gen422_dev;
	gen422_dev = (struct gen422_device *)dev_get(HG_GEN422_DEVID);	
	gen422_psram_adr(gen422_dev,psram0,psram1,w,h);
}

void gen422_run(){
	struct gen422_device *gen422_dev;
	gen422_dev = (struct gen422_device *)dev_get(HG_GEN422_DEVID);	
	gen422_dma_run(gen422_dev);
	gen422_sw_enable(gen422_dev,1);
}

