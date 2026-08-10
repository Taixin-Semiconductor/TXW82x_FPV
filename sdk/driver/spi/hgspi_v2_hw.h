#ifndef _HGGPIO_V4_HW_H_
#define _HGGPIO_V4_HW_H_

#ifdef __cplusplus
extern "C" {
#endif

/***** SPICON *****/
/*! Interrupt flag clear, write 1 to clear, write 0 to have no effect
 */
#define LL_SIMPLE_SPI_CON_SPIINTCLR(n)             (((n)&0x01) << 10)
/*! SPI interrupt flag
 */
#define LL_SIMPLE_SPI_CON_SPIINT(n)                (((n)&0x01) <<  9)
/*! SPI status flag
 */
#define LL_SIMPLE_SPI_CON_SPIDONE(n)               (((n)&0x01) <<  8)
/*! DMA enable flag
 */
#define LL_SIMPLE_SPI_CON_SPIDISYNCEN(n)           (((n)&0x01) <<  7)
/*! Master or slave control bits
 */
#define LL_SIMPLE_SPI_CON_SPISM(n)                 (((n)&0x01) <<  6)
/*! Send or receive control bits
 */
#define LL_SIMPLE_SPI_CON_SPIRXTX(n)               (((n)&0x01) <<  5)
/*! 2-wire or 3-wire mode selection
 */
#define LL_SIMPLE_SPI_CON_SPI2W3W(n)               (((n)&0x01) <<  4)
/*! SPI interrupt enable bit
 */
#define LL_SIMPLE_SPI_CON_SPIINTEN(n)              (((n)&0x01) <<  3)
/*! Sampling mode selection bit
 */
#define LL_SIMPLE_SPI_CON_SPISMPSEL(n)             (((n)&0x01) <<  2)
/*! Clock line idle state selection bit
 */
#define LL_SIMPLE_SPI_CON_SPIIDST(n)               (((n)&0x01) <<  1)
/*! SPI enable bit
 */
#define LL_SIMPLE_SPI_CON_SPIEN(n)                 (((n)&0x01) <<  0)


/*****SPIBAUD *****/
/*! Baud rate setting
 */
#define LL_SIMPLE_SPI_SPIBAUD(n)                   (((n)&0x0000FFFF) << 0)


/***** SPIDATA *****/
/*! Data register
 */
#define LL_SIMPLE_SPI_SPIDATA(n)                   (((n)&0x000000FF) << 0)


struct hgspi_v2_hw {
    __IO uint32_t CON;
    __IO uint32_t BAUD;
    __IO uint32_t DATA;
};


#ifdef __cplusplus
}
#endif

#endif /* _HGGPIO_V4_HW_H_ */

