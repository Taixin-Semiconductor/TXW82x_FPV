#ifndef _HGI2C_V1_HW_H
#define _HGI2C_V1_HW_H

#ifdef __cplusplus
 extern "C" {
#endif

/** @addtogroup IIC MODULE REGISTER
  * @{
  */

/***** CON0(for IIC) Register *****/
#define LL_IIC_CON0_SBC_IE(n)                           (((n)&0x01) << 30)

#define LL_IIC_CON0_SBC_EN(n)                           (((n)&0x01) << 29)

#define LL_IIC_CON0_ALERT_IE(n)                         (((n)&0x01) << 28)

#define LL_IIC_CON0_NOSTRETCH_EN(n)                     (((n)&0x01) << 27)

#define LL_IIC_CON0_SMBDEV_ADR_EN(n)                    (((n)&0x01) << 26)

#define LL_IIC_CON0_SMBALERT_EN(n)                      (((n)&0x01) << 25)

#define LL_IIC_CON0_SMBHOST_ADR_EN(n)                   (((n)&0x01) << 24)

/*! IIC received NACK signal interrupt enable
 */
#define LL_IIC_CON0_STRONG_DRV_EN                       (1UL << 23)
/*! IIC received NACK signal interrupt enable
 */
#define LL_IIC_CON0_RX_NACK_IE_EN                       (1UL << 22)
/*! IIC arbitration loss interrupt enable
 */
#define LL_IIC_CON0_AL_IE_EN                            (1UL << 21)
/*! IIC received STOP signal interrupt enable
 */
#define LL_IIC_CON0_STOP_IE_EN                          (1UL << 20)

#define LL_IIC_CON0_ADR_MTH_IE(n)                       (((n)&0x01) << 19)

#define LL_IIC_CON0_I2C_FILTER_MAX(n)                   (((n)&0x1F) << 14)

#define LL_IIC_CON0_I2C_AL_EN(n)                        (((n)&0x01) << 13)
/*! IIC slave broadcast interrupt enable
 */
#define LL_IIC_CON0_BROADCAST_IE_EN                     (1UL << 12)

#define LL_IIC_CON0_ISP_CMD_LEN(n)                      (((n)&0x0F) << 8)

#define LL_IIC_CON0_ISP_MODE_EN                         (1UL << 7)

#define LL_IIC_CON0_TIMEOUTB_IE(n)                      (((n)&0x01) << 3)

#define LL_IIC_CON0_TIMEOUTA_IE(n)                      (((n)&0x01) << 2)
/*! The IIC responds to the NACK signal after receiving the data
 */
#define LL_IIC_CON0_TX_NACK                             (1UL << 1)
/*! IIC slave address bit width
 */
#define LL_IIC_CON0_SLAVE_ADR_WIDTH(n)                  (((n)&0x01) << 0)


/***** CON1(for IIC) Register *****/

#define LL_IIC_CON1_PING_PONG_EN(n)                     ((n&0x1) << 12)

#define LL_IIC_CON1_CLRBUFCNT_CLRSSP_EN(n)              ((n&0x1) << 11)

#define LL_IIC_CON1_RX_TIMEOUT_IE(n)                    ((n&0x1) << 10)

/*! IIC DMA interrupt enable
 */
#define LL_IIC_CON1_DMA_IE_EN                           (1UL << 9)
/*! IIC FIFO overflow interrupt enable
 */
#define LL_IIC_CON1_BUF_OV_IE_EN                        (1UL << 8)
/*! IIC RX FIFO not empty interrupt enable
 */
#define LL_IIC_CON1_RX_BUF_NOT_EMPTY_IE_EN              (1UL << 7)
/*! IIC TX FIFO not full interrupt enable
 */
#define LL_IIC_CON1_TX_BUF_NOT_FULL_IE_EN               (1UL << 6)
/*! IIC transfers one frame interrupt enable
 */
#define LL_IIC_CON1_SSP_IE_EN                           (1UL << 5)
/*! IIC DMA enable
 */
#define LL_IIC_CON1_DMA_EN                              (1UL << 4)
/*! The IIC is set to the TX direction
 */
#define LL_IIC_CON1_TX_EN                               (1UL << 3)
/*! IIC working mode
 */
#define LL_IIC_CON1_MODE(n)                             (((n)&0x01) << 2)
/*! IIC module selet
 */
#define LL_IIC_CON1_IIC_SEL                             (1UL << 1)

/*! SPI/IIC module enable
 */
#define LL_IIC_CON1_SSP_EN                              (1UL << 0)


/***** SSP CMD DATA(for IIC) Register *****/
/*! IIC master transfers START signal
 */
#define LL_IIC_CMD_DATA_START_BIT_EN                    (1UL << 8)
/*! IIC master transfers STOP signal
 */
#define LL_IIC_CMD_DATA_STOP_BIT_EN                     (1UL << 9)
/*! Write data 
 */
#define LL_IIC_CMD_DATA_WRITE(n)                        (((n)&0xFF) << 0)
/*! Read data 
 */
#define LL_IIC_CMD_DATA_READ(n)                         (((n)>>0) & 0xFF)


/***** BAUD(for IIC) Register *****/
#define LL_IIC_TIMECON_PRESC(n)                         (((n)&0x0F) << 28)

#define LL_IIC_TIMECON_SCLDEL(n)                        (((n)&0x0F) << 24)

#define LL_IIC_TIMECON_SDADEL(n)                        (((n)&0x0F) << 20)

#define LL_IIC_TIMECON_SCLH(n)                          (((n)&0x3FF) << 10)

#define LL_IIC_TIMECON_SCLL(n)                          (((n)&0x3FF) << 0)


/***** DMA Tx LEN(for IIC) Register *****/
/*! Set DMA Tx length(12bit)
 */
#define LL_IIC_DMA_TX_LEN(n)                            (n)


/***** DMA Tx CNT(for IIC) Register *****/
/*! The length of the byte of the received data(12bit)
 */
#define LL_IIC_DMA_TX_CNT(n)                            (n)


/***** DMA Tx STADR(for IIC) Register *****/
/*! DMA Tx start address(13bit)
 */
#define LL_IIC_DMA_TX_STADR(n)                          (n)


/***** DMA Rx LEN(for IIC) Register *****/
/*! Set DMA Rx length(12bit)
 */
#define LL_IIC_DMA_RX_LEN(n)                            (n)


/***** DMA Rx CNT(for IIC) Register *****/
/*! The length of the byte of the received data(12bit)
 */
#define LL_IIC_DMA_RX_CNT(n)                            (n)


/***** DMA Rx STADR(for IIC) Register *****/
/*! DMA Rx start address(13bit)
 */
#define LL_IIC_DMA_RX_STADR(n)                          (n)


/***** STA1(for IIC) Register *****/
#define LL_IIC_STA1_HSYNC_PEND_CLR_BUF_CNT               (1UL << 31)

#define LL_IIC_STA1_HSYNC_PEND                           (1UL << 21)

#define LL_IIC_STA1_VSYNC_PEND                           (1UL << 20)

#define LL_IIC_STA1_DMA_PPBUF_SEL                        (1UL << 19)

#define LL_IIC_STA1_RX_TIMEOUT_PEND                      (1UL << 18)

#define LL_IIC_STA1_MODF                                 (1UL << 17)

#define LL_IIC_STA1_SLAVE_WRONG_CMD_PENDING              (1UL << 16)

#define LL_IIC_STA1_SLAVE_WIREMODE_CFG_PENDING           (1UL << 15)

#define LL_IIC_STA1_SLAVE_RDSTATUS_PENDING               (1UL << 14)

#define LL_IIC_STA1_SLAVE_RDDATA_PENDING                 (1UL << 13)

#define LL_IIC_STA1_SLAVE_WRDATA_PENDING                 (1UL << 12)

#define LL_IIC_STA1_CLEAR_BUF_CNT                        (1UL << 9)
/*! Get how many bytes of valid data in the FIFO
 */
#define LL_IIC_STA1_BUF_CNT(n)                           (((n)& 0x07) )

#define LL_IIC_STA1_MASTER_RX_BUSY_PENDING               (1UL << 8)

#define LL_IIC_STA1_SLAVE_CS_STATE                       (1UL << 7)

#define LL_IIC_STA1_SSP_BUSY_PENDING                     (1UL << 6)

#define LL_IIC_STA1_NSS_POS_PENDING                      (1UL << 5)

#define LL_IIC_STA1_DMA_PENDING                          (1UL << 4)

#define LL_IIC_STA1_BUF_OV_PENDING                       (1UL << 3)

#define LL_IIC_STA1_BUF_EMPTY_PENDING                    (1UL << 2)

#define LL_IIC_STA1_BUF_FULL_PENDING                     (1UL << 1)

#define LL_IIC_STA1_DONE_PENDING                         (1UL << 0)


/***** STA2(for IIC) Register *****/
#define LL_IIC_STA2_STATE(n)                             ((n&0x07)<<16)
#define LL_IIC_STA2_TIMEOUTB_PEND(n)                     ((n&0x01)<<13)
#define LL_IIC_STA2_TIMEOUTA_PEND(n)                     ((n&0x01)<<12)
#define LL_IIC_STA2_SBC_PEND(n)                          ((n&0x01)<<11)
#define LL_IIC_STA2_ALERT_PEND(n)                        ((n&0x01)<<10)
#define LL_IIC_STA2_I2C_AL(n)                            ((n&0x01)<<9)
#define LL_IIC_STA2_RX_NACK(n)                           ((n&0x01)<<8)
#define LL_IIC_STA2_STOP_PEND(n)                         ((n&0x01)<<7)
#define LL_IIC_STA2_I2C_BUS_BUSY(n)                      ((n&0x01)<<6)
#define LL_IIC_STA2_SLV_RW(n)                            ((n&0x01)<<5)
#define LL_IIC_STA2_ADRMTHCODE(n)                        ((n&0x07)<<2)
#define LL_IIC_STA2_SLV_ADDRED(n)                        ((n&0x01)<<1)
#define LL_IIC_STA2_ADR_MTH_PEND(n)                      ((n&0x01)<<0)


/***** OWNADRCON(for IIC) Register *****/
#define LL_IIC_OWNADRCON_OWN_ADR2_MASK(n)                ((n&0x07)<<24)
#define LL_IIC_OWNADRCON_OWN_ADR2(n)                     ((n&0x7F)<<17)
#define LL_IIC_OWNADRCON_OWN_ADR2_EN(n)                  ((n&0x01)<<16)
#define LL_IIC_OWNADRCON_OWN_ADR1_S(n)                   ((n&0x01)<<11)
#define LL_IIC_OWNADRCON_OWN_ADR1_EN(n)                  ((n&0x01)<<10)
#define LL_IIC_OWNADRCON_OWN_ADR1(n)                     ((n&0x3FF)<<0)


/***** RBUF(for IIC) Register *****/
#define LL_IIC_RBUF(n)                                   ((n))

/***** TIMEOUTCON(for IIC) Register *****/
#define LL_IIC_TIMEOUTCON_TIMEOUTB_EN(n)                 ((n&01)<<31)
#define LL_IIC_TIMEOUTCON_TIMEOUTB(n)                    ((n&0xFFF)<<16)
#define LL_IIC_TIMEOUTCON_TIMEOUTA_EN(n)                 ((n&01)<<15)
#define LL_IIC_TIMEOUTCON_TIMEOUTA_S(n)                  ((n&01)<<12)
#define LL_IIC_TIMEOUTCON_TIMEOUTA(n)                    ((n&0xFFF)<<0)

/***** RX TIMEOUTCON(for IIC) Register *****/
#define LL_IIC_RXTIMEOUTCON_RX_TIMEOUT(n)                 ((n&0x1ffffff7)<<1)
#define LL_IIC_RXTIMEOUTCON_RX_TIMEOUT_EN(n)              ((n&0x1)<<0)




typedef enum {
    /*! IIC does not transmit signals other than ACK.
     */
    LL_IIC_NONE_FLAG    = 0,
    /*! IIC sends START signal
     */
    LL_IIC_START_FLAG   = 1,
    /*! IIC sends STOP signal
     */
    LL_IIC_STOP_FLAG    = 2,
    /*! IIC sends NACK signal
     */
    LL_IIC_NACK_FLAG    = 4,
} TYPE_ENUM_LL_IIC_FLAG;



/**
  * @brief IIC 
  */
struct hgi2c_v1_hw {
    __IO uint32_t CON0;
    __IO uint32_t CON1;
    __IO uint32_t CMD_DATA;
    __IO uint32_t TIMECON;
    __IO uint32_t TDMALEN;
    __IO uint32_t RDMALEN;
    __IO uint32_t TDMACNT;
    __IO uint32_t RDMACNT;
    __IO uint32_t TSTADR;
    __IO uint32_t RSTADR;
    __IO uint32_t STA1;
    __IO uint32_t STA2;
    __IO uint32_t SLAVESTA;
    __IO uint32_t OWNADRCON;
    __IO uint32_t RBUF;
    __IO uint32_t TIMEOUTCON;
    __IO uint32_t RXTIMEOUTCON;
    __IO uint32_t RSTADR1;
    __IO uint32_t VSYNC_TCON;
    __IO uint32_t HSYNC_TCON;
};


#ifdef __cplusplus
}
#endif


#endif /* _HGI2C_V1_HW_H */