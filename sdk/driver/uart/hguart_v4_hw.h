#ifndef _HGUART_V4_HW_H_
#define _HGUART_V4_HW_H_

#ifdef __cplusplus
extern "C" {
#endif

    

/***** UARTCON *****/
/*! Timeout interrupt en
 */
#define LL_SIMPLE_UART_CON_TO_IE                                (1UL << 31)

/*! Timeout function en
 */
#define LL_SIMPLE_UART_CON_TO_EN                                (1UL << 30)

/*! Timeout pending
 */
#define LL_SIMPLE_UART_CON_CLRTOPEND                            (1UL << 29)

/*! Transmission completed The 1byte flag was cleared to zero
 */
#define LL_SIMPLE_UART_CON_CLRTXDONE                            (1UL << 28)
/*! The frame receiving error detection flag is cleared to zero
 */
#define LL_SIMPLE_UART_CON_CLRFERR                              (1UL << 27)
/*! The receive cache register is not null flag cleared
 */
#define LL_SIMPLE_UART_CON_CLRRXDONE                            (1UL << 26)
/*! DMA interrupt flag cleared to zero
 */
#define LL_SIMPLE_UART_CON_CLRDMAPEND                           (1UL << 25)
/*! Receive address interrupt flag cleared to zero
 */
#define LL_SIMPLE_UART_CON_CLRRXADDRPEND                        (1UL << 24)
/*! Transmission completed 1byte flag. This bit is 0 when the module is disabled
 */
#define LL_SIMPLE_UART_CON_TXDONE                               (1UL << 21)
/*! Frame receiving error detection flag
 */
#define LL_SIMPLE_UART_CON_FERR                                 (1UL << 20)
/*! Receive cache register is not empty flag, this bit is 0 when the module is not enabled
 */
#define LL_SIMPLE_UART_CON_RXBUFNOTEMPTY                        (1UL << 19)
/*! The send cache register is an empty flag. This bit is 0 when the module is disabled
 */
#define LL_SIMPLE_UART_CON_TXBUFEMPTY                           (1UL << 18)
/*! The DMA completion flag bit, which is set to 1 when a send or receive is complete using DMA
 */
#define LL_SIMPLE_UART_CON_DMAPEND                              (1UL << 17)
/*! Receive address interrupt flag RXADDRPEND 
 */
#define LL_SIMPLE_UART_CON_RXADDRPEND                           (1UL << 16)

/*! RX_TIMEOUT PENDING
 */
#define LL_SIMPLE_UART_CON_TO_PENDING                           (1UL << 15)

/*! Select UART output with which PWM carrier:
 */
#define LL_SIMPLE_UART_CON_CARRIER_SEL(n)                       (((n)&0x09) << 11)
/*! Set the CARRIER_SEL bit to the PWM carrier mode enable bit. Specify the carrier_sel bit for the specific PWM carrier of the TIMER  
 */
#define LL_SIMPLE_UART_CON_PWM_CARRIER_EN                       (1UL << 10)
/*! The receiving address was interrupted. Procedure
 */
#define LL_SIMPLE_UART_CON_RXADDRIE                             (1UL << 9)
/*! Error detection interrupt enabled    
 */
#define LL_SIMPLE_UART_CON_FERRIE                               (1UL << 8)
/*! DMA interrupt enabled
 */
#define LL_SIMPLE_UART_CON_DMA_IE                               (1UL << 7)
/*! Stop bit setting
 */
#define LL_SIMPLE_UART_CON_STOPBIT                              (1UL <<  6)
/*! The 9bit function was enabled
 */
#define LL_SIMPLE_UART_CON_BIT9EN                               (1UL <<  5)
/*! The Uart function was enabled 
 */
#define LL_SIMPLE_UART_CON_UARTEN                               (1UL <<  4)
/*! Send data inversely 
 */
#define LL_SIMPLE_UART_CON_TXINV                                (1UL <<  3)
/*! The received data is reversed
 */
#define LL_SIMPLE_UART_CON_RXINV                                (1UL <<  2)
/*! Send 1byte to enable the interrupt
 */
#define LL_SIMPLE_UART_CON_UARTTXIE                             (1UL <<  1)
/*! The receiving interrupt function was enabled
 */
#define LL_SIMPLE_UART_CON_UARTRXIE                             (1UL <<  0)




/***** UARTBAUD *****/
/*! Baud rate setting
 */
#define LL_SIMPLE_UART_UARTBAUD(n)                             (((n)&0x0003FFFF) << 0)



/***** UARTDATA *****/
/*! Data register
 */
#define LL_SIMPLE_UART_UARTDATA(n)                             (((n)&0x000000FF) << 0)


/***** UARTTOCON *****/
/*! TOCON register
 */
#define LL_SIMPLE_UART_TOCON(n)                                (((n)&0x0000FFFF) << 0)


/***** UARTDMAADR *****/
/*! DMAADR register
 */
#define LL_SIMPLE_UART_UARTDMAADR(n)                           (((n)&0xFFFFFFFF) << 0)



/***** UARTDMALEN *****/
/*! DMALEN register
 */
#define LL_SIMPLE_UART_UARTDMALEN(n)                            (((n)&0x000007FF) << 0)



/***** UARTDMACON *****/
/*!DMACON register
 */
#define LL_SIMPLE_UART_UART_TX_USEDMA_KEY                       (1UL << 3)
#define LL_SIMPLE_UART_UART_RX_USEDMA_KEY                       (1UL << 2)
#define LL_SIMPLE_UART_UART_TX_USEDMA                           (1UL << 1)
#define LL_SIMPLE_UART_UART_RX_USEDMA                           (1UL << 0)


/**
  * @brief Simple Universal Synchronous Asynchronous Receiver Transmitter
  */

struct hguart_v4_hw{
    __IO uint32_t CON;
    __IO uint32_t BAUD;
    __IO uint32_t DATA;
    __IO uint32_t TOCON;
    __IO uint32_t DMAADR;
    __IO uint32_t DMALEN;
    __IO uint32_t DMACON;
    __IO uint32_t DMACNT;
};


#ifdef __cplusplus
}
#endif

#endif /* _HGUART_V4_HW_H_ */

