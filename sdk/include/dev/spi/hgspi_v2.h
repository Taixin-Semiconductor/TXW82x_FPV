#ifndef _HGSPI_V2_H
#define _HGSPI_V2_H

#ifdef __cplusplus
extern "C" {
#endif



struct hgspi_v2 {
    struct spi_device   dev;
    uint32              hw ;
    spi_irq_hdl         irq_hdl ;
    uint32              irq_data;
    uint32              irq_num ;
    uint16              opened  ;
    uint16              spi_irq_flag_tx_done: 1,
                        spi_irq_flag_rx_done: 1;
};


int32 hgspi_v2_attach(uint32 dev_id, struct hgspi_v2 *spi);


#ifdef __cplusplus
}
#endif
#endif