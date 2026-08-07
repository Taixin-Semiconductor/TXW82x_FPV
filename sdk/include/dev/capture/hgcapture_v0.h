#ifndef _HGCAPTURE_V0_H
#define _HGCAPTURE_V0_H
#include "hal/capture.h"

#ifdef __cplusplus
extern "C" {
#endif


#define HGCAPTURE_MAX_CAPTURE_CHANNEL 4

enum hgcapture_v0_func_cmd {
    HGCAPTURE_V0_FUNC_CMD_MASK                  = BIT(7),
    HGCAPTURE_V0_FUNC_CMD_INIT                  = HGCAPTURE_V0_FUNC_CMD_MASK | 1,
    HGCAPTURE_V0_FUNC_CMD_DEINIT                = HGCAPTURE_V0_FUNC_CMD_MASK | 2,
    HGCAPTURE_V0_FUNC_CMD_START                 = HGCAPTURE_V0_FUNC_CMD_MASK | 3,
    HGCAPTURE_V0_FUNC_CMD_STOP                  = HGCAPTURE_V0_FUNC_CMD_MASK | 4,
    /* IOCTL CMD */

    /* IRQ_FLAG */
    HGCAPTURE_V0_FUNC_CMD_REQUEST_IRQ_CAPTURE   = HGCAPTURE_V0_FUNC_CMD_MASK | 5,
    HGCAPTURE_V0_FUNC_CMD_REQUEST_IRQ_OVERFLOW  = HGCAPTURE_V0_FUNC_CMD_MASK | 6,
    
    HGCAPTURE_V0_FUNC_CMD_RELEASE_IRQ           = HGCAPTURE_V0_FUNC_CMD_MASK | 7,

    HGCAPTURE_V0_FUNC_CMD_SUSPEND               = HGCAPTURE_V0_FUNC_CMD_MASK | 8,
    HGCAPTURE_V0_FUNC_CMD_RESUME                = HGCAPTURE_V0_FUNC_CMD_MASK | 9,
};

struct hgcapture_v0_config {
    uint32           irq_data;
    capture_irq_hdl  irq_hdl;
    uint16           func_cmd;
    /* Only for config REG in lower layer code */
    uint16           cap_sel:1, 
                     cap_pol:1;
};

struct hgcapture_v0 {
    struct capture_device  dev;
    void                  *channel[HGCAPTURE_MAX_CAPTURE_CHANNEL];
    uint8                  opened[HGCAPTURE_MAX_CAPTURE_CHANNEL];
};



int32 hgcapture_v0_attach(uint32 dev_id, struct hgcapture_v0 *capture);


#ifdef __cplusplus
}
#endif
#endif
