#ifndef HG_GEN420_H
#define HG_GEN420_H
#include "hal/gen420.h"


typedef enum {
	GEN420_DONE_ISR = 0,
	GEN420_PIXEL_FAST_ISR,
	GEN420_FRAME_FAST_ISR,
	GEN420_IRQ_NUM,
}GEN420_IRQ_E;


struct hggen420 {
    struct gen420_device       dev;
	uint32                  hw;
    uint32                  irq_num;
    uint32              opened:1, use_dma:1, dsleep:1;
	uint32 *cfg_backup;
};

void hggen420_attach(uint32 dev_id, struct hggen420 *gen420);

#endif
