#ifndef HG_VPP_H
#define HG_VPP_H
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/vpp.h"

typedef enum {
	HSIE_ISR = 0,
	VSIE_ISR,
	FRAME_DONE_ISR,
	SCIE_ISR,	
	LOVIE_ISR,	
	IPF_OV_ISR,
	MDPIE_ISR,
	ITP_OV_ISR,
	ITP_DONE_ISR,
	VPP_IRQ_NUM,
}VPP_IRQ_E;
 

struct hgvpp {
    struct vpp_device   dev;
    //struct hgdvp_hw     *hw;
	uint32              hw;
    vpp_irq_hdl         irq_hdl;
    uint32              irq_data;
    uint32              irq_num;
    uint32              opened:1, use_dma:1,exit:1;
};


int32 hgvpp_attach(uint32 dev_id, struct hgvpp *vpp);

#endif

