#ifndef HG_CSC_H
#define HG_CSC_H
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/csc.h"


typedef enum {
	CSC_DONE_IRQ,
	CSC_IRQ_NUM
}CSC_IRQ_E;


struct hgcsc {
    struct csc_device   dev;
	uint32              hw;
    csc_irq_hdl         irq_hdl;
    uint32              irq_data;
    uint32              irq_num;
	int32               addr_count;
    uint32              opened:1, use_dma:1;
};
int32 hgcsc_attach(uint32 dev_id, struct hgcsc *csc);
#endif
