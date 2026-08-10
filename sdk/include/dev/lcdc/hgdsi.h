#ifndef HG_DSI_H
#define HG_DSI_H
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/dsi.h"


struct hgdsi {
    struct dsi_device   dev;
	uint32              hw;
    dsi_irq_hdl         irq_hdl;
    uint32              irq_data;
    uint32              irq_num;
    uint32              opened:1, use_dma:1;
};


int32 hgdsi_attach(uint32 dev_id, struct hgdsi *dsi);


#endif
