#ifndef _HGI2S_V0_HW_H
#define _HGI2S_V0_HW_H

#ifdef __cplusplus
    extern "C" {
#endif

/** @addtogroup IIS MODULE REGISTER 
  * @{
  */
    
    /***** CON Register *****/
    /*! Filter the RX data from IO
     */
#define LL_I2S_CON_RXDBSBPS(n)                          (((n)&0x01) << 14)
    /*! Filter the WSCLK clock from IO
     */
#define LL_I2S_CON_WSCLKDBSBPS(n)                       (((n)&0x01) << 13)
    /*! Filter the BCLK clock from IO
     */
#define LL_I2S_CON_BCLKDBSBPS(n)                        (((n)&0x01) << 12)
    /*! I2S work mode select
     */
#define LL_I2S_CON_MCLK_OE(n)                           (((n)&0x01) << 11)
    /*! I2S channel mode select
     */
#define LL_I2S_CON_OV_PEND_IE(n)                        (((n)&0x01) << 10)
    /*! I2S BCK clock polarity
     */
#define LL_I2S_CON_HF_PEND_IE(n)                        (((n)&0x01) << 9)
    /*! I2S WS clock polarity
     */
#define LL_I2S_CON_WORKMODE(n)                          (((n)&0x01) << 8)
    /*! sign extension function
     */
#define LL_I2S_CON_MONO(n)                              (((n)&0x03) << 6)
    /*! I2S mode select
     */
#define LL_I2S_CON_BCKPOL(n)                            (((n)&0x01) << 5)
    /*! I2S serial data format
     */
#define LL_I2S_CON_WSPOL(n)                             (((n)&0x01) << 4)
    /*! Receive channel select
     */
#define LL_I2S_CON_MODE(n)                              (((n)&0x01) << 3)
    /*! Transmit channel select
     */
#define LL_I2S_CON_FRMT(n)                              (((n)&0x03) << 1)
    /*! I2S enable
     */
#define LL_I2S_CON_ENABLE(n)                            (((n)&0x01) << 0)
    
    
    
    /***** BIT_SET Register *****/
    /*! I2S bit set
     */
#define LL_I2S_BIT_SET_IISBIT(n)                        (((n)&0x1F) << 0)


    /***** BUAD Register *****/
    /*! I2S baud set
     */
#define LL_I2S_BAUD_SET_BUAD(n)                         (((n)&0x3F) << 0)


    /***** WSCON Register *****/
    /*! I2S wscon set
     */
#define LL_I2S_WSCON_SET_WSCON(n)                       (((n)&0x3F) << 0)


    /***** STA Register *****/
    /*! I2S DMA data over pending
     */
#define LL_I2S_STA_DMA_OV_PENGING(n)                    (((n)&0x01) << 5)
    /*! I2S DMA data half pending
     */
#define LL_I2S_STA_DMA_HF_PENGING(n)                    (((n)&0x01) << 4)
    /*! I2S transmit done pending
     */
#define LL_I2S_STA_DONE_PENGING(n)                      (((n)&0x01) << 3)
    /*! I2S work state pending
     */
#define LL_I2S_STA_WORK_STATE_PENGING(n)                (((n)&0x01) << 2)
    /*! I2S fifo write full pending
     */
#define LL_I2S_STA_FIFO_WFULL_PENGING(n)                (((n)&0x01) << 1)
    /*! I2S fifo read empty pending
     */
#define LL_I2S_STA_FIFO_REMPTY_PENGING(n)               (((n)&0x01) << 0)

    /***** STADR0 Register *****/
    /*! I2S DMA address config
     */
#define LL_I2S_STADR_DMA_ADDR0(n)                       (((n)&0xFFFFFFFF) << 0)

    /***** DMALEN Register *****/
    /*! I2S DMA len config
     */
#define LL_I2S_DMALEN_LEN(n)                            (((n)&0xFFFF) << 0)



/** @brief I2S register structure
  * @{
  */
struct hgi2s_v0_hw {
    __IO uint32_t CON;
    __IO uint32_t BIT_SET;
    __IO uint32_t BAUD;
    __IO uint32_t WS_CON;
    __IO uint32_t STA;
    __IO uint32_t DMA_STADR;
    __IO uint32_t RESERVE;
    __IO uint32_t DMA_LEN;
};


#ifdef __cplusplus
}
#endif

#endif /* _HGI2S_V0_HW_H */