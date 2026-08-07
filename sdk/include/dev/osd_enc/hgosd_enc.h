#ifndef HG_OSD_ENC_H
#define HG_OSD_ENC_H
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/osd_enc.h"


typedef enum {
	ENC_DONE_IRQ,
	ENC_IRQ_NUM
}OSD_ENC_IRQ_E;


struct hgosd {
    struct osdenc_device   dev;
	uint32              hw;
    osdenc_irq_hdl         irq_hdl;
    uint32              irq_data;
    uint32              irq_num;
    uint32              opened:1, use_dma:1;
};
int32 hgosdenc_attach(uint32 dev_id, struct hgosd *lcdc);

#endif

