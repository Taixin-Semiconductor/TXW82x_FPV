#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "dev.h"
#include "devid.h"
#include "hal/gpio.h"
#include "hal/lcdc.h"
#include "hal/spi.h"
#include "hal/osd_enc.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/osd_enc/hgosd_enc.h"
#include "osal/semaphore.h"
#include "lib/lcd/lcd.h"
#include "osal/mutex.h"
#include "lib/lcd/gui.h"
void lcd_sema_up();
void osdenc_done(uint32 irq_flag,uint32 irq_data,uint32 param1){
	struct osdenc_device *osdenc_dev = (struct osdenc_device *)irq_data;
	printf("%s %d\r\n",__func__,osd_enc_dlen(osdenc_dev));




	lcd_sema_up();
}

void osd_enc_init(){
	struct osdenc_device *osdenc_dev;
	osdenc_dev = (struct osdenc_device *)dev_get(HG_OSD_ENC_DEVID); 
	osd_enc_open(osdenc_dev);
	osd_enc_tran_config(osdenc_dev,0xFFFFFF,0xFFFFFF,0x000000,0x000000);
	osd_enc_set_format(osdenc_dev,1);
	osd_enc_request_irq(osdenc_dev,ENC_DONE_IRQ,(osdenc_irq_hdl )&osdenc_done,(uint32)osdenc_dev);
}

