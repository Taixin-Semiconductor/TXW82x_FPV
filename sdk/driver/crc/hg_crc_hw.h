#ifndef _HG_CRC_HW_H_
#define _HG_CRC_HW_H_

#ifdef __cplusplus
extern "C" {
#endif

enum hg_crc_flags {
    HGCRC_FLAGS_SUSPEND = BIT(0),
    HGCRC_FLAGS_HOLD    = BIT(1),
};

/***** CRC_CFG Register *****/
/*! CRC interrupt enable 
 */
#define LL_CRC_CFG_INT_EN                          (1UL << 0)
/*! CRC shift bit direction 
 */
#define LL_CRC_CFG_BIT_ORDER_LEFT                  (1UL << 1)
#define LL_CRC_CFG_BIT_ORDER_RIGHT                 (0UL << 1)
/*! CRC POLY width: 5/7/8/16/32 
 */
#define LL_CRC_CFG_POLY_BITS(n)                    (((n)&0x3F) << 8)
/*! CRC DMA data wait clock every time 
 */
#define LL_CRC_CFG_DMAWAIT_CLOCK(n)                (((n)&0x7) << 16)
/*! CRC TCP mode enable 
 */
#define LL_CRC_CFG_TCP_MODE_EN                     (1UL << 24)


/***** CRC_KST Register *****/
/*! CRC pending clear 
 */
#define LL_CRC_KST_DMA_PENDING_CLR                 (1UL << 0)


/***** CRC_STA Register *****/
/*! CRC pending 
 */
#define LL_CRC_STA_DMA_PENDING                     (1UL << 0) 


/***** CRC_INIT Register *****/
/***** CRC_INV Register *****/
/***** CRC_POLY Register *****/
/***** CRC_DMA_ADDR Register *****/
/***** CRC_DMA_LEN Register *****/
/***** CRC_CRC_OUT Register *****/



/**
  * @brief CRC
  */
struct hg_crc_hw {
    __IO uint32 CRC_CFG;                        // 0x0c
    __IO uint32 CRC_INIT;                       // 0x04
    __IO uint32 CRC_INV;                        // 0x08
    __IO uint32 CRC_POLY;                       // 0x0c
    __IO uint32 CRC_KST;                        // 0x10
    __IO uint32 CRC_STA;                        // 0x14
         uint32 RESERVED0;
    __IO uint32 DMA_ADDR;                       // 0x1c
    __IO uint32 DMA_LEN;                        // 0x20
    __IO uint32 CRC_OUT;                        // 0x24
} ;

#ifdef __cplusplus
}
#endif

#endif /* _HG_CRC_H_ */


