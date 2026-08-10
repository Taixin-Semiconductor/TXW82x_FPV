#ifndef _HGSHA_V0_H_
#define _HGSHA_V0_H_
#include "hal/sha.h"

#ifdef __cplusplus
extern "C" {
#endif



struct hgsha_v1 {
    struct sha_dev      dev;
    void                *hw;
    os_mutex_t          lock;
    os_semaphore_t      done;
    uint32              irq_num;
    uint32              flags;
    void (*irq_func)(void *args);
    void *irq_data;
    struct sha_ctx      ctx;
	struct sha_ctx      *pctx;
};

int32 hgsha_v1_attach(uint32 dev_id, struct hgsha_v1 *sha);

#endif /* _hgsha256_H_ */
