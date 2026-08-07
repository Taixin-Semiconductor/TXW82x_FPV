#ifndef _HGSPI_V1_H_
#define _HGSPI_V1_H_
#include "hal/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

struct hgspi_v1 {
    struct spi_device       dev;
    uint32                  hw;
    uint32                  irq_num;
    spi_irq_hdl             irq_hdl;
    uint32                  irq_data;
    //os_mutex_t         os_spi_tx_lock;
    //os_mutex_t         os_spi_rx_lock;
    os_mutex_t         os_spi_lock;
    os_semaphore_t     os_spi_tx_done;
    os_semaphore_t     os_spi_rx_done;
    uint32                  timeout;
    uint16                  len_threshold;
    uint16                  spi_tx_done          :1,
                            spi_rx_done          :1,
                            spi_irq_flag_tx_done :1,
                            spi_irq_flag_rx_done :1,
                            spi_tx_async         :1,
                            spi_rx_async         :1,
                            opened               :1,
                            dsleep               :1;
#ifdef CONFIG_SLEEP
    struct {
        uint32 con0;
        uint32 con1;
        uint32 baud;
        uint32 tdmalen;
        uint32 rdmalen;
        uint32 tstadr;
        uint32 rstadr;
        uint32 slavesta;
        uint32 ssp_dly;
    }bp_regs;
    uint32               bp_irq_flags;
    spi_irq_hdl          bp_irq_hdl;
    uint32               bp_irq_data;
    os_mutex_t      bp_suspend_lock;
    os_mutex_t      bp_resume_lock;
#endif

};

int32 hgspi_v1_attach(uint32 dev_id, struct hgspi_v1 *p_spi);

#ifdef __cplusplus
}
#endif

#endif /* _HGSPI_V1_H_ */
