#ifndef HG_PRC_H
#define HG_PRC_H
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/prc.h"

typedef enum {
	PRC_ISR = 0,
	PRC_IRQ_NUM,
}PRC_IRQ_E;
 

struct hgprc {
    struct prc_device   dev;
    //struct hgdvp_hw     *hw;
	uint32              hw;
    prc_irq_hdl         irq_hdl;
    uint32              irq_data;
    uint32              irq_num;
    uint32              opened:1, use_dma:1;
};


void hgprc_attach(uint32 dev_id, struct hgprc *prc);

#endif

