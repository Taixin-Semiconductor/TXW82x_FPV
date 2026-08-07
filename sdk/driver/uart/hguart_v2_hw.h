#ifndef _HGUART_V2_HW_H_
#define _HGUART_V2_HW_H_

#ifdef __cplusplus
extern "C" {
#endif

/***** UARTCON *****/
/*! RX buffer trigger threshold
 */
#define LL_UART_CON_RBUF_TRIG(n)                    (((n)&0x03) << 18)
/*! Singal RTS_N enable
 */
#define LL_UART_CON_RTS_EN                          (1UL << 17)
/*! Singal CTS_N enable
 */
#define LL_UART_CON_CTS_EN                          (1UL << 16)
/*! Transmission complete interrupt enable
 */
#define LL_UART_CON_TCIE_EN                         (1UL << 15)
/*! UART with carrier output enable.  
 * @note UART0 outputs pwm of TIMER0, and UART1 outputs pwm of TIMER1.
 */
#define LL_UART_CON_TMR_PWM_EN                      (1UL << 14)
 /*! Frame error interrupt enable
 * @note A frame error refers to a low-level signal received by rx during
 *       the stop bit.
 */
#define LL_UART_CON_FERR_IE_EN                      (1UL << 11)
/*! TX buffer empty interrupt enable
 */
#define LL_UART_CON_TXBUF_EMPTY_IE_EN               (1UL << 10)
/*! RX buffer not empty interrupt enable
 */
#define LL_UART_CON_RXBUF_NEMPTY_IE_EN              (1UL << 9)
/*! Inverted TX signal
 */
#define LL_UART_CON_TX_INV_EN                       (1UL << 8)
/*! Inverted RX signal
 */
#define LL_UART_CON_RX_INV_EN                       (1UL << 7)
/*! Odd parity
 * @note Parity and 9bit data transfer cannot be used at the same time.
 */
#define LL_UART_CON_ODD_EN                          (1UL << 6)
/*! parity enable
 * @note Parity and 9bit data transfer cannot be used at the same time.
 */
#define LL_UART_CON_PARITY_EN                       (1UL << 5)
/*! 9bit data transfer enable
 * @note Parity and 9bit data transfer cannot be used at the same time.
 */
#define LL_UART_CON_BIT9_EN                         (1UL << 4)
/*! Stop bit selection
 */
#define LL_UART_CON_STOP_BIT(n)                     (((n)&0x01) << 3)
/*! Work mode selection
 */
#define LL_UART_CON_WORK_MODE(n)                    (((n)&0x03) << 1)
/*! UART module enable
 */
#define LL_UART_CON_UART_EN                         (1UL << 0)

/***** UARTSTA *****/
/*! Transmission complete pending
 */
#define LL_UART_STA_TC_PENDING                      (1UL << 12)
/*! RX timeout detection pending
 * @note Only UART0 has this feature.
 */
#define LL_UART_STA_TO_PEND                         (1UL << 11)
/*! RX parity error pending
 * @note 4 bits corrspond to 4 frame data in rx buffer.
 */
#define LL_UART_STA_PERR_PEND(n)                    (((n)>>7) & 0x0F)
/*! The amount of data in the rx fifo
 */
#define LL_UART_STA_RX_CNT(n)                       (((n)>>4) & 0x07)
/*! Frame error pending
 */
#define LL_UART_STA_FERR_PENDING                    (1UL << 3)
/*! RX FIFO overflow pending
 */
#define LL_UART_STA_RX_BUF_OV                       (1UL << 2)
/*! RX FIFO not empty pending
 */
#define LL_UART_STA_RX_BUF_NOT_EMPTY                (1UL << 1)
/*! TX FIFO empty pending
 */
#define LL_UART_STA_TX_BUF_EMPTY                    (1UL << 0)

/***** DMACON *****/
/*! RX DMA parity error interrupt enable
 * @note Only UART1 has this feature
 */
#define LL_UART_DMACON_RX_DMA_PERR_IE_EN            (1UL << 4)
/*! RX DMA interrupt enable
 * @note Only UART1 has this feature
 */
#define LL_UART_DMACON_RX_DMA_IE_EN                 (1UL << 3)
/*! TX DMA interrupt enable
 * @note Only UART1 has this feature
 */
#define LL_UART_DMACON_TX_DMA_IE_EN                 (1UL << 2)
/*! RX DMA enable
 * @note Only UART1 has this feature
 */
#define LL_UART_DMACON_RX_DMA_EN                    (0x0101UL << 1)
/*! TX DMA enable
 * @note Only UART1 has this feature
 */
#define LL_UART_DMACON_TX_DMA_EN                    (0x0101UL << 0)

/***** DMASTA *****/
/*! RX DMA parity error pending
 * @note Only UART1 has this feature
 */
#define LL_UART_DMASTA_RX_DMA_PERR                  (1UL << 2)
/*! RX DMA pending
 * @note Only UART1 has this feature
 */
#define LL_UART_DMASTA_RX_DMA_PEND                  (1UL << 1)
/*! TX DMA pending
 * @note Only UART1 has this feature
 */
#define LL_UART_DMASTA_TX_DMA_PEND                  (1UL << 0)

/***** UART_RS485_CON *****/
/*! RS485 RE enable
 * @note Only UART1 has this feature
 */
#define LL_UART_RS485_CON_RE_EN                     (1UL << 9)
/*! RS485 DE enable
 * @note Only UART1 has this feature
 */
#define LL_UART_RS485_CON_DE_EN                     (1UL << 8)
/*! RS485 work mode
 * @note Only UART1 has this feature
 */
#define LL_UART_RS485_CON_RS485_MODE(n)             (((n)&0x01) << 3)
/*! RS485 RE polarity
 * @note Only UART1 has this feature
 */
#define LL_UART_RS485_CON_RE_POL(n)                 (((n)&0x01) << 2)
/*! RS485 DE polarity
 * @note Only UART1 has this feature
 */
#define LL_UART_RS485_CON_DE_POL(n)                 (((n)&0x01) << 1)
/*! RS485 enable
 * @note Only UART1 has this feature
 */
#define LL_UART_RS485_CON_RS485_EN                  (1UL << 0)

/***** UART_RS485_DET *****/
/*! The time interval between the end of STOP BIT and DE invalid. The unit
 *  is the uart module clock.
 * @note Only UART1 has this feature
 */
#define LL_UART_RS485_DET_DE_DAT(n)                 (((n)&0x01FF) << 16)
/*! The time interval between the time DE is valid and the START BIT is sent.
 *  The unit is the uart module clock.
 * @note Only UART1 has this feature
 */
#define LL_UART_RS485_DET_DE_AT(n)                  (((n)&0x01FF) << 0)

/***** UART_RS485_TAT *****/
/*! The time interval between the valid of RE and DE valid, the unit is the
 *  uart module clock.
 * @note Only UART1 has this feature
 */
#define LL_UART_RS485_TAT_RE2DE_T(n)                (((n)&0xFFFF) << 16)
/*! The time interval between DE valid and RE valid, the unit is the uart
 *  module clock.
 * @note Only UART1 has this feature
 */
#define LL_UART_RS485_TAT_DE2RE_T(n)                (((n)&0xFFFF) << 0)

/***** UART_TOCON *****/
/*! Timeout time configure
 */
#define LL_UART_TOCON_TO_BIT_LEN(n)                 (((n)&0xFFFF) << 16)
/*! Timeout interrupt enable
 */
#define LL_UART_TOCON_TO_IE_EN                   	(1UL << 1)
/*! Timeout enable
 */
#define LL_UART_TOCON_TO_EN                      	(1UL << 0)

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
}UART_TypeDef;

/**
 * @brief  enable RS485 RE pin
 * @param  p_uart : The structure pointer of the UART
 * @retval None
 */
__STATIC_INLINE void ll_uart485_re_enable(UART_TypeDef *p_uart) {
   p_uart->RS485_CON |= LL_UART_RS485_CON_RE_EN;
}

/**
 * @brief  disable RS485 RE pin
 * @param  p_uart : The structure pointer of the UART
 * @retval None
 */
__STATIC_INLINE void ll_uart485_re_disable(UART_TypeDef *p_uart) {
   p_uart->RS485_CON &= (~LL_UART_RS485_CON_RE_EN);
}

/**
 * @brief  enable RS485 DE pin
 * @param  p_uart : The structure pointer of the UART
 * @retval None
 */
__STATIC_INLINE void ll_uart485_de_enable(UART_TypeDef *p_uart) {
   p_uart->RS485_CON |= LL_UART_RS485_CON_DE_EN;
}

/**
 * @brief  disable RS485 DE pin
 * @param  p_uart : The structure pointer of the UART
 * @retval None
 */
__STATIC_INLINE void ll_uart485_de_disable(UART_TypeDef *p_uart) {
   p_uart->RS485_CON &= (~LL_UART_RS485_CON_DE_EN);
}


#ifdef __cplusplus
}
#endif

#endif /* _HGUART_V2_HW_H_ */

