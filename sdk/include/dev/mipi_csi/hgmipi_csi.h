#ifndef HG_MIPI_CSI_H
#define HG_MIPI_CSI_H
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/dvp.h"
#include "hal/csi2.h"

typedef enum {	
	CSI2_FOVIE_ISR = 0,
	CSI2_VSIP_ISR,
	CSI2_HSIP_ISR,
	MIPI_CSI_IRQ_NUM
}MIPI_CSI_IRQ_E;

struct hgmipi_csi {
    struct mipi_csi_device   dev;
	uint32              hw;
    mipi_csi_irq_hdl    irq_hdl;
    uint32              irq_data;
    uint32              irq_num;
    uint32              opened:1, use_dma:1;
};


int32 hgmipi_csi_attach(uint32 dev_id, struct hgmipi_csi *mipi_csi);

#endif


