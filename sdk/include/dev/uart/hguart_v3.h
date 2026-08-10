#ifndef _HGUART_V3_H
#define _HGUART_V3_H

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Simple uart operating handle structure
  * @{
  */
struct hguart_v3 {
    struct uart_device   dev     ;
    uint32               hw      ;
    uart_irq_hdl         irq_hdl ;
    uint32               irq_data;
    uint16               irq_num ;
    uint16               opened  ;
};


int32 hguart_v3_attach(uint32 dev_id, struct hguart_v3 *uart);


#ifdef __cplusplus
}
#endif

#endif
