#ifndef HG_ROTATE_H
#define HG_ROTATE_H
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/rotate.h"


struct hgrotate {
    struct rotate_device   dev;
	uint32              hw;
    uint32              opened:1, use_dma:1;
};

int32 hgrotate_attach(uint32 dev_id, struct hgrotate *rotate);
#endif

