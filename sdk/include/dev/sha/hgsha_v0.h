#ifndef _HGSHA_V0_H_
#define _HGSHA_V0_H_
#include "hal/sha.h"

#ifdef __cplusplus
extern "C" {
#endif



struct hgsha_v0 {
    struct sha_dev      dev;
    void                *hw;
    os_mutex_t     lock;
    os_semaphore_t done;
	os_semaphore_t using256;
    uint32              irq_num;
    uint32              alg_type;
    volatile uint32     flags_busy_or_idle   :1,
                        flags_in_the_interupt:1,
                        flags_got_result     :1,
                        flags_first_pack     :1,
                        flags_media_pack     :1,	
						flags_in_multi_mode:1,
						flags_last_mode       :1,
						flags_cur_mode       :1;
    void (*sha_irq_func)(void *args);
    void *irq_func_data;
};

int32 hgsha_v0_attach(uint32 dev_id, struct hgsha_v0 *sha);

#endif /* _hgsha256_H_ */
