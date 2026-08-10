#ifndef _HGSPI_V1_HW_H_
#define _HGSPI_V1_HW_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup SPI MODULE REGISTER 
  * @{
  */

/***** CON0(for SPI) Register *****/
/*! SPI slave read status done interrupt enable
 */
#define LL_SPI_CON0_SLAVE_WRONGCMD_IE_EN                (1UL << 25)
/*! SPI slave read status done interrupt enable
 */
#define LL_SPI_CON0_SLAVE_WIREMODE_IE_EN                (1UL << 24)
/*! SPI slave read status done interrupt enable
 */
#define LL_SPI_CON0_SLAVE_RDSTATUS_IE_EN                (1UL << 23)
/*! SPI slave read data done interrupt enable
 */
#define LL_SPI_CON0_SLAVE_RDDATA_IE_EN                  (1UL << 22)
/*! SPI slave write data done interrupt enable
 */
#define LL_SPI_CON0_SLAVE_WRDATA_IE_EN                  (1UL << 21)
/*! SPI slave state enable
 */
#define LL_SPI_CON0_SLAVE_STATE_EN                      (1UL << 20)
/*! SPI frame size
 */
#define LL_SPI_CON0_FRAME_SIZE(n)                       (((n)&0x3F) << 14)
/*! Get SPI frame size
 */
#define LL_SPI_CON0_GET_FRAME_SIZE(n)                   (((n)>>14) & 0x3F)
/*! SPI slave CS(NSS) rising edge interrupt enable
 */
#define LL_SPI_CON0_NSS_POS_IE_EN                       (1UL << 13)
/*! SPI CS(NSS) pin enable
 */
#define LL_SPI_CON0_NSS_PIN_EN                          (1UL << 12)
/*! SPI CS(NSS) output high
 *  @note Only valid in SPI master mode
 */
#define LL_SPI_CON0_NSS_HIGH                            (1UL << 11)

/*! Input data synchronization enable in SPI slave mode
 */
#define LL_SPI_CON0_SLAVE_SYNC_EN                       (1UL << 7)
/*! Input data synchronization enable in SPI master mode
 */
#define LL_SPI_CON0_MASTER_SYNC_EN                      (1UL << 6)
/*! The SPI transfers data from the low bit
 */
#define LL_SPI_CON0_LSB_FIRST                           (1UL << 4)
/*! SPI wire mode
 */
#define LL_SPI_CON0_WIRE_MODE(n)                        (((n)&0x03) << 2)
/*! Get SPI wire mode
 */
#define LL_SPI_CON0_WIRE_MODE_GET(n)                    (((n)>>2) & 0x03)
/*! SPI mode
 */
#define LL_SPI_CON0_SPI_MODE(n)                         (((n)&0x03) << 0)


/***** CON1(for SPI) Register *****/
/*! SPI DMA done interrupt enable
 */
#define LL_SPI_CON1_DMA_IE_EN                           (1UL << 9)
/*! SPI FIFO overflow interrupt enable
 */
#define LL_SPI_CON1_BUF_OV_IE_EN                        (1UL << 8)
/*! SPI RX FIFO not empty interrupt enable
 */
#define LL_SPI_CON1_RX_BUF_NOT_EMPTY_IE_EN              (1UL << 7)
/*! SPI TX FIFO not full interrupt enable
 */
#define LL_SPI_CON1_TX_BUF_NOT_FULL_IE_EN               (1UL << 6)
/*! SPI transfers one frame interrupt enable
 */
#define LL_SPI_CON1_SSP_IE_EN                           (1UL << 5)
/*! SPI DMA enable
 */
#define LL_SPI_CON1_DMA_EN                              (1UL << 4)
/*! The SPI is set to the TX direction
 */
#define LL_SPI_CON1_TX_EN                               (1UL << 3)
/*! SPI working mode
 */
#define LL_SPI_CON1_MODE(n)                             (((n)&0x01) << 2)
/*! Get SPI working mode
 */
#define LL_SPI_CON1_GET_MODE(n)                         (((n)>>2) & 0x01)
/*! SPI/IIC selection
 */
#define LL_SPI_CON1_SPI_I2C_SEL(n)                      (((n)&0x01) << 1)

/*! SPI/IIC module enable
 */
#define LL_SPI_CON1_SSP_EN                              (1UL << 0)


/***** SSP CMD DATA(for SPI) Register *****/
/*! Write data(32bit) 
 */
#define LL_SPI_SSP_CMD_DATA_WRITE(n)                    (n)
/*! Read data(32bit)
 */
#define LL_SPI_SSP_CMD_DATA_READ(n)                     (n)


/***** BAUD(for SPI) Register *****/
/*! Set baud rate(16bit)
 */
#define LL_SPI_BAUD(n)                                  (n)


/***** DMA Tx LEN(for SPI) Register *****/
/*! Set DMA Tx length(12bit)
 */
#define LL_SPI_DMA_TX_LEN(n)                            (n)


/***** DMA Tx CNT(for SPI) Register *****/
/*! The length of the byte of the transmitted data(12bit)
 */
#define LL_SPI_DMA_TX_CNT(n)                            (n)


/***** DMA Tx STADR(for SPI) Register *****/
/*! DMA Tx start address(13bit)
 */
#define LL_SPI_DMA_TX_STADR(n)                          (n)


/***** DMA Rx LEN(for SPI) Register *****/
/*! Set DMA Rx length(12bit)
 */
#define LL_SPI_DMA_RX_LEN(n)                            (n)


/***** DMA Rx CNT(for SPI) Register *****/
/*! The length of the byte of the received data(12bit)
 */
#define LL_SPI_DMA_RX_CNT(n)                            (n)


/***** DMA Rx STADR(for SPI) Register *****/
/*! DMA Rx start address(13bit)
 */
#define LL_SPI_DMA_RX_STADR(n)                          (n)


/***** STA(for SPI) Register *****/
/*! SPI Slave wrong cmd pending
 */
#define LL_SPI_STA_SLAVE_WRONG_CMD_PENDING              (1UL << 28)
/*! SPI Slave wiremode change pending
 */
#define LL_SPI_STA_SLAVE_WIREMODE_CFG_PENDING           (1UL << 23)
/*! SPI Slave read status pending
 */
#define LL_SPI_STA_SLAVE_RDSTATUS_PENDING               (1UL << 22)
/*! SPI Slave read data pending
 */
#define LL_SPI_STA_SLAVE_RDDATA_PENDING                 (1UL << 21)
/*! SPI Slave write data pending
 */
#define LL_SPI_STA_SLAVE_WRDATA_PENDING                 (1UL << 20)
/*! Clear fifo
 */
#define LL_SPI_STA_CLEAR_BUF_CNT                        (1UL << 19)
/*! Get how many bytes of valid data in the FIFO
 */
#define LL_SPI_STA_BUF_CNT(n)                           (((n)>>16) & 0x07)
/*! SPI master receive data busy pending
 */
#define LL_SPI_STA_MASTER_RX_BUSY_PENDING               (1UL << 15)
/*! The SPI slave gets the CS pin state
 */
#define LL_SPI_STA_SLAVE_CS_STATE                       (1UL << 11)
/*! SPI module busy pending
 */
#define LL_SPI_STA_SSP_BUSY_PENDING                     (1UL << 10)
/*! SPI slave CS(NSS) rising edge detected pending
 */
#define LL_SPI_STA_NSS_POS_PENDING                      (1UL << 5)
/*! SPI DMA done pending
 */
#define LL_SPI_STA_DMA_PENDING                          (1UL << 4)
/*! SPI FIFO overflow pending
 */
#define LL_SPI_STA_BUF_OV_PENDING                       (1UL << 3)
/*! SPI FIFO empty pending
 */
#define LL_SPI_STA_BUF_EMPTY_PENDING                    (1UL << 2)
/*! SPI FIFO full pending
 */
#define LL_SPI_STA_BUF_FULL_PENDING                     (1UL << 1)
/*! SPI transfers one frame done pending
 */
#define LL_SPI_STA_DONE_PENDING                         (1UL << 0)
/***** SLAVESTA(for SPI) Register *****/
#define LL_SPI_SLAVE_STA_TX_LEN(n)                      (((n)&0xFF) << 16)
#define LL_SPI_SLAVE_STA_RX_LEN(n)                      (((n)&0xFF) << 8)
#define LL_SPI_SLAVE_STA_RX_READY_EN                    (1UL << 7)
#define LL_SPI_SLAVE_STA_TX_READY_EN                    (1UL << 6)
#define LL_SPI_SLAVE_STA_SW_RESERVED                    (1UL << 5)
#define LL_SPI_SLAVE_STA_WRONG_COMMAND                  (1UL << 4)
#define LL_SPI_SLAVE_STA_DATA_TOGGLE                    (1UL << 3)
#define LL_SPI_SLAVE_STA_TX_OVERFLOW                    (1UL << 2)
#define LL_SPI_SLAVE_STA_RX_READY_FLG                   (1UL << 1)
#define LL_SPI_SLAVE_STA_TX_READY_FLG                   (1UL << 0)


/***** SSP_DLY Register *****/
/*! Transfer delay time config
 @Note: t = APB_CLK * (n)
 */
#define LL_SPI_AND_IIC_SSP_DLY(n)                       ((n & 0xFF) << 0)











/** @brief SPI register structure
  * @{
  */
struct hgspi_v1_hw {
    __IO uint32_t CON0;
    __IO uint32_t CON1;
    __IO uint32_t CMD_DATA;
    __IO uint32_t BAUD;
    __IO uint32_t TDMALEN;
    __IO uint32_t RDMALEN;
    __IO uint32_t TDMACNT;
    __IO uint32_t RDMACNT;
    __IO uint32_t TSTADR;
    __IO uint32_t RSTADR;
    __IO uint32_t STA;
    __IO uint32_t SLAVESTA;
    __IO uint32_t SSP_DLY;
};


#ifdef __cplusplus
}
#endif

#endif /* _HGSPI_V1_HW_H_ */

