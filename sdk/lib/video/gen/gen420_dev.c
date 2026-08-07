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
#include "dev/gen/hggen420.h"
#include "lib/lcd/lcd.h"
#include "hal/jpeg.h"
#include "lib/video/vpp/vpp_dev.h"
#include "lib/video/gen/gen420_dev.h"
#include "lib/scale/scale_dev.h"


void gen420_run(uint8_t *psram_buf,uint32_t width,uint32_t high){
	struct gen420_device *gen420_dev;
	gen420_dev = (struct gen420_device *)dev_get(HG_GEN420_DEVID);		
	gen420_psram_adr(gen420_dev,(uint32_t)psram_buf,(uint32_t)psram_buf+width*high,(uint32_t)psram_buf+width*high+width*high/4);
	_os_printf("G");
	gen420_dma_run(gen420_dev);
}

void gen420_done_isr(uint32 irq,uint32 dev,uint32  param){
	//struct gen420_device *gen420_dev = (struct gen420_device *)dev;
	_os_printf("%s  %d\r\n",__func__,__LINE__);
}


void gen420_dev_init(uint32_t width,uint32_t high){
	struct gen420_device *gen420_dev;
	uint8_t *gen420_sram;
	gen420_dev = (struct gen420_device *)dev_get(HG_GEN420_DEVID);	
	gen420_sram = malloc(12*1024);
	gen420_open(gen420_dev);
	gen420_request_irq(gen420_dev,GEN420_DONE_ISR,(gen420_irq_hdl )&gen420_done_isr,(uint32_t)gen420_dev);;
	gen420_sram_linebuf_adr(gen420_dev,(uint32_t)gen420_sram,(uint32_t)gen420_sram+8*1024,(uint32_t)gen420_sram+10*1024);
	gen420_frame_size(gen420_dev,width,high);
	gen420_dst_h264_and_jpg(gen420_dev,0);
}

