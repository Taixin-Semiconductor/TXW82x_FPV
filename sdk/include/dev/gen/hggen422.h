#ifndef HG_GEN422_H
#define HG_GEN422_H
#include "hal/gen422.h"


typedef enum {
	GEN422_GDONE_ISR = 0,
	GEN422_RDONE_ISR,
	GEN422_RD_SLOW_ISR,
	GEN422_WR_SLOW_ISR,
	GEN422_GEN_OV_ISR,
	GEN422_IRQ_NUM,
}GEN422_IRQ_E;


struct hggen422 {
    struct gen422_device       dev;
	uint32                  hw;
    uint32                  irq_num;
    uint32                  opened;
	uint32 *cfg_backup;
};


void hggen422_attach(uint32 dev_id, struct hggen422 *gen422);

#endif

