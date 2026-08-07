#ifndef _HGPDM_V0_H_
#define _HGPDM_V0_H_
#include "hal/pdm.h"

#ifdef __cplusplus
extern "C" {
#endif


struct hgpdm_v0 {
    struct pdm_device   dev;
    uint32              hw;
    pdm_irq_hdl         irq_hdl;
    uint32              irq_data;
    uint16              irq_num;
    uint16              opened:1,
                        dsleep:1;
#ifdef CONFIG_SLEEP
    struct {
        uint32 con;
        uint32 dmacon;
        uint32 dmastadr;
        uint32 dmalen;
    }bp_regs;
    uint32               bp_irq_flags;
    pdm_irq_hdl          bp_irq_hdl;
    uint32               bp_irq_data;
    os_mutex_t      bp_suspend_lock;
    os_mutex_t      bp_resume_lock;
#endif
};


int32 hgpdm_v0_attach(uint32 dev_id, struct hgpdm_v0 *pdm);


#ifdef __cplusplus
}
#endif
#endif

