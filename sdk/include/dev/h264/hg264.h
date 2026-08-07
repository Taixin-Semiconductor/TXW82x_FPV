#ifndef HG_H264_H
#define HG_H264_H
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/h264.h"


typedef enum {
	H264_FRAME_DONE,
	H264_BUF0_FULL,
	H264_BUF1_FULL,
	H264_SOFT_SLOW,
	H264_FRAME_FAST,
	H264_PIXEL_FAST,
	H264_PIXEL_DONE,
	H264_ENC_TIME_OUT,	
	H264_DEC_FRAME_ERR,
	H264_BS_LEN_ERR,
	H264_CFG_W_ERR,	
	H264_IRQ_NUM
}H264_IRQ_E;


struct hg264 {
    struct h264_device   dev;
	uint32              hw;
    h264_irq_hdl         irq_hdl;
    uint32              irq_data;
    uint32              irq_num;
	int32               addr_count;
    volatile uint32              opened:1, use_dma:1,err:1, dsleep:1,h264_clt:1,h264_close_status:1,h264_oe_enable:1,h264_oe_used:1;
	uint32 *cfg_backup;
};
int32 hg264_attach(uint32 dev_id, struct hg264 *lcdc);



#endif
