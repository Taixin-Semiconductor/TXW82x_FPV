#ifndef _HGSDIO20_SLAVE_H_
#define _HGSDIO20_SLAVE_H_
#include "hal/sdio_slave.h"

#ifdef __cplusplus
extern "C" {
#endif

struct hgsdio20_slave {
    struct sdio_slave dev;
    void  *hw;
    sdio_slave_irq_hdl irq_hdl;
    uint32 irq_data;
    os_mutex_t tx_lock;
    os_semaphore_t tx_done;
    uint8 *rx_buff;

    uint8  irq_num;
    uint8  rst_irq_num;
    uint8  flags;
    uint8  opened: 1, rev: 7;

    uint16 rx_len;
    uint8  speed, rev2;
    uint8  func0_cis[17];
    uint8  func1_cis[53];
    uint64 mntr_tick;
};

int32 hgsdio20_slave_attach(uint32 dev_id, struct hgsdio20_slave *slave);

#ifdef __cplusplus
}
#endif

#endif /* _HGSDIO20_SLAVE_H_ */

