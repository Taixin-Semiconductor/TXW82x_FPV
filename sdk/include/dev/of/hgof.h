#ifndef HG_OF_H
#define HG_OF_H
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/of.h"


struct hgof {
    struct of_device   dev;
	uint32              hw;
    uint32              irq_data;
    uint32              irq_num;
    uint32              opened:1, use_dma:1;
};

void hgof_attach(uint32 dev_id, struct hgof *of);
#endif
