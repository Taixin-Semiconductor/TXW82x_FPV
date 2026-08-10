#ifndef _HG_SYSAES_V2_H_
#define _HG_SYSAES_V2_H_
#include "hal/sysaes.h"

#ifdef __cplusplus
extern "C" {
#endif

struct hg_sysaes_v2 {
    struct sysaes_dev   dev;
    void               *hw;
    os_mutex_t     lock;
    os_semaphore_t done;
    uint32              irq_num;
    uint32              flags;
#ifdef CONFIG_SLEEP
    uint32              *regs;
#endif    
};

int32 hg_sysaes_v2_attach(uint32 dev_id, struct hg_sysaes_v2 *sysaes);

#endif /* _HG_SYSAES_V2_H_ */
