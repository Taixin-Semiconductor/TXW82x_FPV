#ifndef _HGI2S_V0_H_
#define _HGI2S_V0_H_
#include "hal/i2s.h"

#ifdef __cplusplus
extern "C" {
#endif



/** @brief I2S operating handle structure
  * @{
  */
struct hgi2s_v0 {
    struct i2s_device   dev      ;
    uint32              hw       ;
    i2s_irq_hdl         irq_hdl  ;
    uint32              irq_data ;
    uint16              irq_num  ;
    uint16              opened   :1,
                        dsleep   :1,
                        duplex_en:1;
#ifdef CONFIG_SLEEP
    struct {
        uint32 con;
        uint32 bit_set;
        uint32 baud;
        uint32 ws_con;
        uint32 dma_stadr;
        uint32 dma_len;
    }bp_regs;
    uint32               bp_irq_flags;
    i2s_irq_hdl          bp_irq_hdl;
    uint32               bp_irq_data;
    os_mutex_t      bp_suspend_lock;
    os_mutex_t      bp_resume_lock;
#endif
};


int32 hgi2s_v0_attach(uint32 dev_id, struct hgi2s_v0 *i2s);



#ifdef __cplusplus
}
#endif

#endif /* _HGI2S_V0_H_ */
