#ifndef HG_DUAL_H
#define HG_DUAL_H
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/dual_org.h"


typedef enum {
	ORG0_SD_ISR = 0,
	ORG1_SD_ISR,
	ORG_RD_DONE_IE,
	ORG_RD_SLOW_IE,	
	DORG_IRQ_NUM,
}DORG_IRQ_E;


struct hgdual {
    struct dual_device   dev;
	uint32              hw;
    dual_irq_hdl         irq_hdl;
    uint32              irq_data;
    uint32              irq_num;
	int32               addr_count;
    uint32              opened:1, use_dma:1, save : 3, change_done : 1;
};
int32 hgdual_attach(uint32 dev_id, struct hgdual *dual);




#endif
