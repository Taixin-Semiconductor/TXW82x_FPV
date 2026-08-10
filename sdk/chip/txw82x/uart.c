#include "basic_include.h"

#define LL_UART_CON_UART_EN                         (1UL << 0)
#define LL_UART_STA_TX_BUF_EMPTY                    (1UL << 0)

typedef struct hguart_v2_hw {
    __IO uint32_t CON;
    __IO uint32_t BAUD;
    __IO uint32_t DATA;
    __IO uint32_t STA;
    __IO uint32_t TSTADR;
    __IO uint32_t RSTADR;
    __IO uint32_t TDMALEN;
    __IO uint32_t RDMALEN;
    __IO uint32_t TDMACNT;
    __IO uint32_t RDMACNT;
    __IO uint32_t DMACON;
    __IO uint32_t DMASTA;
    __IO uint32_t RS485_CON;
    __IO uint32_t RS485_DET;
    __IO uint32_t RS485_TAT;
    __IO uint32_t TOCON;
} UART_TypeDef;

uint8 uart_getc(struct uart_device *uart)
{
    return 0;
}

int32 uart_putc(struct uart_device *uart, int8 value)
{
    struct hguart_v2_hw *hw  = (struct hguart_v2_hw *)CoreSetting->dbg_uart_dev;

    if ( ((uint32_t)hw != UART0_BASE) && 
         ((uint32_t)hw != UART1_BASE) && 
         ((uint32_t)hw != UART4_BASE) && 
         ((uint32_t)hw != UART5_BASE) && 
         ((uint32_t)hw != UART6_BASE) ) {
        return -EIO;
    }
         
    if (hw && (hw->CON & LL_UART_CON_UART_EN)) {
        while (!(hw->STA & LL_UART_STA_TX_BUF_EMPTY));
        hw->DATA = value;
        return RET_OK;
    } else {
        return -EIO;
    }
}

#if 0
void atcmd_printf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    hgvprintf(format, ap);
    va_end(ap);
}
#endif

