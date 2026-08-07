#ifndef _HGUART_V2_H
#define _HGUART_V2_H
#include "hal/uart.h"

#ifdef __cplusplus
extern "C" {
#endif


#define __PSRAM_ADDR_START (0x18800000)
#define __PSRAM_ADDR_END   (0x18800000 + (8*1024*1024))

struct hguart_v2 {
    struct uart_device   dev;
    uint32 hw;
    uart_irq_hdl         irq_hdl;
    uint32               irq_data;
    uint16               irq_num;
    uint16               opened:1, dsleep:1, use_dma:1;
    uint32 flag;
#ifdef CONFIG_SLEEP
    struct {
        uint32 con;
        uint32 baud;
        uint32 tstadr;
        uint32 rstadr;
        uint32 tdmalen;
        uint32 rdmalen;
        uint32 dmacon;
        uint32 rs485_con;
        uint32 rs485_det;
        uint32 rs485_tat;
        uint32 tocon;
    }bp_regs;
    uint32               bp_irq_flags;
    uart_irq_hdl         bp_irq_hdl;
    uint32               bp_irq_data;
#endif
};


int32 hguart_v2_attach(uint32 dev_id, struct hguart_v2 *uart);


#ifdef __cplusplus
}
#endif
#endif
