/**
 * @file hgspi_v3.c
 * @author bxd
 * @brief normal spi
 * @version
 * TXW81X
 * @date 2023-08-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "dev/spi/hgspi_v3.h"
#include "hgspi_v3_hw.h"

/**********************************************************************************/
/*                        SPI LOW LAYER FUNCTION                                  */
/**********************************************************************************/
static int32 hgspi_v3_switch_hal_work_mode(enum spi_work_mode work_mode) {

    switch (work_mode) {
        case (SPI_MASTER_MODE   ):
            return 0;
            break;
        case (SPI_SLAVE_MODE    ):
            return 1;
            break;
        case (SPI_SLAVE_FSM_MODE):
            return 2;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hgspi_v3_switch_hal_wire_mode(enum spi_wire_mode wire_mode) {

    switch (wire_mode) {
        case (SPI_WIRE_NORMAL_MODE):
            return 0;
            break;
        case (SPI_WIRE_SINGLE_MODE):
            return 1;
            break;
        case (SPI_WIRE_DUAL_MODE  ):
            return 2;
            break;
        case (SPI_WIRE_QUAD_MODE  ):
            return 3;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hgspi_v3_switch_hal_clk_mode(enum spi_wire_mode clk_mode) {

    switch (clk_mode) {
        case (SPI_CPOL_0_CPHA_0):
            return 0;
            break;
        case (SPI_CPOL_0_CPHA_1):
            return 1;
            break;
        case (SPI_CPOL_1_CPHA_0):
            return 2;
            break;
        case (SPI_CPOL_1_CPHA_1):
            return 3;
            break;
        default:
            return -1;
            break;
    }
}




static inline void hgspi_v3_disable(struct hgspi_v3_hw *p_spi)
{
    p_spi->CON1 &= ~LL_SPI_CON1_SSP_EN;
}

static inline void hgspi_v3_enable(struct hgspi_v3_hw *p_spi)
{
    p_spi->CON1 |= LL_SPI_CON1_SSP_EN;
}

static inline void hgspi_v3_set_dir_tx(struct hgspi_v3_hw *p_spi)
{
    p_spi->CON1 |= LL_SPI_CON1_TX_EN;
}

static inline void hgspi_v3_set_dir_rx(struct hgspi_v3_hw *p_spi)
{
    p_spi->CON1 &= ~LL_SPI_CON1_TX_EN;
}

/**
  * @brief  SPI module test whether spi is send over function
  * @param  p_spi       : Select the initialized SPI module pointer
  * @retval 1           : over
  *         0           : not over
  */
static inline int32 hgspi_v3_get_tx_fifo_empty(struct hgspi_v3_hw *p_spi)
{
    return ((p_spi->STA1 & LL_SPI_STA_BUF_EMPTY_PENDING) != 0);
}

/**
  * @brief  SPI module test whether spi is busy function
  * @param  p_spi       : Select the initialized SPI module pointer
  * @retval 1           : busy
  *         0           : not busy
  */
static inline int32 hgspi_v3_get_busy(struct hgspi_v3_hw *p_spi)
{
    return ((p_spi->STA1 & LL_SPI_STA_SSP_BUSY_PENDING) != 0);
}

/**
  * @brief  SPI module wait spi send over function
  * @param  p_spi       : Select the initialized SPI module pointer
  * @retval None
  */
static inline void hgspi_v3_wait_tx_done(struct hgspi_v3_hw *p_spi)
{
    while (!hgspi_v3_get_tx_fifo_empty(p_spi));
    while (hgspi_v3_get_busy(p_spi));
}

/**
  * @brief  SPI module test whether spi is ready to receive function
  * @param  p_spi       : Select the initialized SPI module pointer
  * @retval 1           : ready
  *         0           : not ready
  */
static inline int32 hgspi_v3_get_rx_ready_sta(struct hgspi_v3_hw *p_spi)
{
    return (!(p_spi->STA1 & LL_SPI_STA_BUF_EMPTY_PENDING));
}

/**
  * @brief  SPI module read data from spi receive fifo function
  * @param  p_spi       : Select the initialized SPI module pointer
  * @retval data received
  */
static inline int32 hgspi_v3_get_data(struct hgspi_v3_hw *p_spi)
{
    return (int32_t)p_spi->CMD_DATA;
}

static inline void hgspi_v3_set_data(struct hgspi_v3_hw *p_spi, uint32 data)
{
    p_spi->CMD_DATA = data;
}


/**
 * @brief   receive one char from spi in poll method, blocked function
 * @param[in]   p_spi : spi register structure pointer
 * @retval  data received by the spi
 */
static inline int32 hgspi_v3_poll_rcv_dat(struct hgspi_v3_hw *p_spi)
{
    hgspi_v3_set_dir_rx(p_spi);
    /** wait until spi is ready to receive */
    while (!hgspi_v3_get_rx_ready_sta(p_spi)); /* blocked */
    /** receive data */
    return hgspi_v3_get_data(p_spi);
}

/**
 * @brief   Set spi mode
 * @param   p_spi     : spi register structure pointer
 * @param   mode      : TYPE_ENUM_LL_SPI_MODE
 * @retval  None
 */
static inline void hgspi_v3_set_mode(struct hgspi_v3_hw *p_spi, uint32 mode)
{
    hgspi_v3_wait_tx_done(p_spi);

    hgspi_v3_disable(p_spi);
    p_spi->CON0 = (p_spi->CON0 &~ LL_SPI_CON0_SPI_MODE(0x3)) | (mode);
    hgspi_v3_enable(p_spi);
}

/**
  * @brief  SPI module test whether spi is ready to send function
  * @param  p_spi       : Select the initialized SPI module pointer
  * @retval 1           : ready
  *         0           : not ready
  */
static inline uint32 hgspi_v3_get_tx_ready_sta(struct hgspi_v3_hw *p_spi)
{
    return (!(p_spi->STA1 & LL_SPI_STA_BUF_FULL_PENDING));
}

/**
 * @brief   send char by spi in poll method, blocked function
 * @param[in]   p_spi : spi register structure pointer
 * @param[in]   data        data to be sent
 */
static inline void hgspi_v3_poll_send_dat(struct hgspi_v3_hw *p_spi, uint32 data)
{
    hgspi_v3_set_dir_tx(p_spi);
    /** wait until spi is ready to send */
    while (!hgspi_v3_get_tx_ready_sta(p_spi)); /* blocked */
    /** send char */
    hgspi_v3_set_data(p_spi, data);
}

/**
  * @brief  Low layer SPI sets wire mode
  * @param  p_spi    : The structure pointer of the SPI group is selected
  * @param  wire_mode: The wire mode to be set
  * @retval None
  */
static int32 hgspi_v3_set_wire_mode(struct hgspi_v3_hw *p_spi, uint32 wire_mode)
{
    hgspi_v3_wait_tx_done(p_spi);

    hgspi_v3_disable(p_spi);
    p_spi->CON0 = (p_spi->CON0 & (~ LL_SPI_CON0_WIRE_MODE(0x3))) | LL_SPI_CON0_WIRE_MODE(wire_mode);
    hgspi_v3_enable(p_spi);
    
    return RET_OK;
}

/**
  * @brief  Low layer SPI gets wire mode
  * @param  p_spi    : The structure pointer of the SPI group (SPI0, SPI1) is selected
  * @retval Return the spi wire mode
  */
static uint32 hgspi_v3_get_wire_mode(struct hgspi_v3_hw *p_spi)
{
    return LL_SPI_CON0_WIRE_MODE_GET(p_spi->CON0);
}

static int32 hgspi_v3_set_frame_size(struct hgspi_v3_hw *p_spi, uint32 frame_size) {
    if (frame_size < 0 || frame_size > 32) {
        return -EINVAL;
    }

    hgspi_v3_disable(p_spi);
    p_spi->CON0 = (p_spi->CON0 &~ LL_SPI_CON0_FRAME_SIZE(0x3F)) | LL_SPI_CON0_FRAME_SIZE(frame_size);
    hgspi_v3_enable(p_spi);

    return RET_OK;
}

//Understand it as a host(spi slave fsm mode)
static inline int32 hgspi_v3_spi_slave_fsm_get_rx_done_sta(struct hgspi_v3_hw *p_spi) {
    return (p_spi->SLAVESTA & LL_SPI_SLAVE_STA_TX_READY_FLG) ? FALSE : TRUE;
}

//Understand it as a host(spi slave fsm mode)
static inline void hgspi_v3_spi_slave_fsm_set_rx_ready_sta(struct hgspi_v3_hw *p_spi, uint32 len) {
    p_spi->SLAVESTA &= ~ LL_SPI_SLAVE_STA_TX_LEN(0xFF);
    p_spi->SLAVESTA |=   LL_SPI_SLAVE_STA_TX_LEN(len - 1)     |
                         LL_SPI_SLAVE_STA_TX_READY_EN         | 
                         LL_SPI_SLAVE_STA_TX_READY_FLG;
}

//Understand it as a host(spi slave fsm mode)
static inline int32 hgspi_v3_spi_slave_fsm_get_tx_done_sta(struct hgspi_v3_hw *p_spi) {
    return (p_spi->SLAVESTA & LL_SPI_SLAVE_STA_RX_READY_FLG) ? FALSE : TRUE;
}

//Understand it as a host(spi slave fsm mode)
static inline void hgspi_v3_spi_slave_fsm_set_tx_ready_sta(struct hgspi_v3_hw *p_spi, uint32 len) {
    p_spi->SLAVESTA &= ~ LL_SPI_SLAVE_STA_RX_LEN(0xFF);
    p_spi->SLAVESTA |=   LL_SPI_SLAVE_STA_RX_LEN(len - 1)     |
                         LL_SPI_SLAVE_STA_RX_READY_EN         | 
                         LL_SPI_SLAVE_STA_RX_READY_FLG;
}

static inline int32 hgspi_v3_spi_set_len_threshold(struct hgspi_v3 *p_spi, uint16 len_threshold) {
    p_spi->len_threshold = len_threshold;

    return RET_OK;
}


static inline void hgspi_v3_set_cs(struct hgspi_v3_hw *p_spi, uint32 value)
{
    if (value) {
        p_spi->CON0 |= LL_SPI_CON0_NSS_HIGH;
    } else {
        p_spi->CON0 &= ~LL_SPI_CON0_NSS_HIGH;
    }
}

static inline int32 hgspi_v3_complete(struct hgspi_v3 *p_spi)
{
    struct hgspi_v3 *dev = p_spi;
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;

    /* spi tx */
    if (hw->CON1 & LL_SPI_CON1_TX_EN) {
        /* spi slave */
        if (hw->CON1 & LL_SPI_CON1_MODE(1)) {
            /* SPI slave use dma transfer, so dma done pending is 1, transfer has done */
            /* SPI slave detect cs rising, so transfer has done when cs rising */
            while ((!(hw->STA1 & LL_SPI_STA_NSS_POS_PENDING)) && (!(dev->spi_tx_done)));
            hw->STA1 =  LL_SPI_STA_NSS_POS_PENDING;
            dev->spi_tx_done  = 0;
        }
        /* spi master */
        else if (!(hw->CON1 & LL_SPI_CON1_MODE(1))) {
            while (!(dev->spi_tx_done));
            dev->spi_tx_done = 0;
        }

    } 
    /* spi rx */
    else {
        /* spi slave */
        if (hw->CON1 & LL_SPI_CON1_MODE(1)) {
            /* SPI slave use dma transfer, so dma done pending is 1, transfer has done */
            /* SPI slave detect cs rising, so transfer has done when cs rising */
            while ((!(hw->STA1 & LL_SPI_STA_NSS_POS_PENDING)) && (!(dev->spi_rx_done)));
            hw->STA1 =  LL_SPI_STA_NSS_POS_PENDING;
            dev->spi_rx_done  = 0;
        }
        /* spi master */
        else if (!(hw->CON1 & LL_SPI_CON1_MODE(1))) {
            while (!(dev->spi_rx_done));
            dev->spi_rx_done = 0;
        }
    }



    return RET_OK;
}

static int32 hgspi_v3_cs(struct hgspi_v3 *dev, uint32 cs, uint32 value)
{
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;

    hgspi_v3_set_cs(hw, value);

    return RET_OK;
}


static int32 hgspi_v3_set_cfg_none_cs(struct hgspi_v3 *dev, uint32 enable) {

    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;

    if (enable) {
        hw->CON0 = (hw->CON0) &~ (LL_SPI_CON0_NSS_PIN_EN);
    } else {
        hw->CON0 |= (LL_SPI_CON0_NSS_PIN_EN);
    }

    return RET_OK;
}

static int32 hgspi_v3_read_dma_rx_cnt(struct hgspi_v3 *dev, uint32 *cnt_buf) {

    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;

    if (!cnt_buf) {
        return RET_ERR;
    }

    *cnt_buf = hw->RDMACNT;

    return RET_OK;
}

static int32 hgspi_v3_kick_dma_rx(struct hgspi_v3 *dev, uint32 *rx_buf, uint32 len) {

    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;

    if ((!rx_buf) || (!len)) {
        return RET_ERR;
    }

    /* 
     *硬件要求：
     *为防止内部8bit移位器残留bit，影响后续正常接发
     *软件清除buf_cnt
    */

    hw->STA1 = LL_SPI_STA_CLR_BUF_CNT;
    
    /* 
        硬件要求：未达到DMA done的情况下，重新kick DMA
        需要先把DMA的EN关掉
    */
    hw->CON1 &= ~ LL_SPI_CON1_DMA_EN;
    
    hw->RDMACNT = 0;
    hw->RSTADR  = (uint32)rx_buf;
    hw->RDMALEN = len;
    hgspi_v3_set_dir_rx(hw);
    hw->CON1 |= LL_SPI_CON1_DMA_EN;

    return RET_OK;
}

static int32 hgspi_v3_kick_dma_tx(struct hgspi_v3 *dev, uint32 *tx_buf, uint32 len) {

    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;

    if ((!tx_buf) || (!len)) {
        return RET_ERR;
    }

    /* 
     *硬件要求：
     *为防止内部8bit移位器残留bit，影响后续正常接发
     *软件清除buf_cnt
    */

    hw->STA1 = LL_SPI_STA_CLR_BUF_CNT;

    
    /* 
        硬件要求：未达到DMA done的情况下，重新kick DMA
        需要先把DMA的EN关掉
    */
    hw->CON1 &= ~ LL_SPI_CON1_DMA_EN;


    hw->TDMACNT = 0;
    hw->TSTADR  = (uint32)tx_buf;
    hw->TDMALEN = len;
    hgspi_v3_set_dir_tx(hw);
    hw->CON1 |= LL_SPI_CON1_DMA_EN;

    return RET_OK;
}

static int32 hgspi_v3_set_timeout(struct hgspi_v3 *dev, uint32 time) {

    dev->timeout = time;

    return RET_OK;
}

static int32 hgspi_v3_set_pingpang(struct hgspi_v3 *dev,uint32 enable){
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;

    if (enable) {
        hw->CON1 |= LL_SPI_CON1_PING_PONG_EN(1);
    } else {
        hw->CON1 &= ~(LL_SPI_CON1_PING_PONG_EN(1));
    }	
	
    return RET_OK;
}

static int32 hgspi_v3_set_pingpang_adr0(struct hgspi_v3 *dev,uint32 buf0){
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;
	hw->RSTADR  = buf0;	
    return RET_OK;
}

static int32 hgspi_v3_set_pingpang_adr1(struct hgspi_v3 *dev,uint32 buf1){
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;
	hw->RSTADR1  = buf1;	
    return RET_OK;
}

static int32 hgspi_v3_set_len(struct hgspi_v3 *dev,uint32 len){
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;
	hw->RDMALEN  = len;	
    return RET_OK;
}

static int32 hgspi_v3_set_line_len(struct hgspi_v3 *dev,uint32 len){
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;
	hw->LINE_LEN  = len;	
    return RET_OK;
}

static int32 hgspi_v3_set_high_speed(struct hgspi_v3 *dev,uint32 cfg){
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;
	hw->CON0  &= ~(LL_SPI_CON0_SPI_HIGH_SPEED(3));
	hw->CON0  |= (LL_SPI_CON0_SPI_HIGH_SPEED(cfg));
	
    return RET_OK;
}

static int32 hgspi_v3_kick_dma(struct hgspi_v3 *dev){
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;
	hw->CON1  |= LL_SPI_CON1_DMA_EN;
	
    return RET_OK;
}

static int32 hgspi_v3_rxtimeout_cfg(struct hgspi_v3 *dev,uint32 enable,uint32 timeout){
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;
	if(enable){
		hw->RXTIMEOUTCON  |= (hw->RXTIMEOUTCON &~ (0x1fffffff<<1)) | (timeout<<1);
		hw->RXTIMEOUTCON  |= BIT(0);
	}else{
		hw->RXTIMEOUTCON  &= ~BIT(0);
	}
	

	
    return RET_OK;
}


static int32 hgspi_v3_set_vsync_timeout(struct hgspi_v3 *dev,uint32 timeout){
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;
	hw->VSYNC_TCON  &= ~(0x1fffff<<8);
	hw->VSYNC_TCON  |= (timeout<<8);
	
    return RET_OK;
}

static int32 hgspi_v3_set_hsync_timeout(struct hgspi_v3 *dev,uint32 timeout){
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;
	hw->HSYNC_TCON  &= ~(0xffff<<8);
	hw->HSYNC_TCON  |= (timeout<<8);
	
    return RET_OK;
}

static int32 hgspi_v3_set_vsync_head(struct hgspi_v3 *dev,uint32 head){
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;
	hw->VSYNC_TCON  &= ~(0x3f<<1);
	hw->VSYNC_TCON  |= (head<<1);
	
    return RET_OK;
}

static int32 hgspi_v3_set_hsync_head(struct hgspi_v3 *dev,uint32 head){
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;
	hw->HSYNC_TCON  &= ~(0x3f<<0);
	hw->HSYNC_TCON  |= (head<<0);
	
    return RET_OK;
}


static int32 hgspi_v3_set_vsync_en(struct hgspi_v3 *dev,uint32 enable){
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;
    if (enable) {
        hw->VSYNC_TCON |= BIT(0);
    } else {
        hw->VSYNC_TCON &= ~BIT(0);
    }
    return RET_OK;
}


static int32 hgspi_v3_set_lsb_first(struct hgspi_v3 *dev, uint32 enable) {

    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;

    if (enable) {
        hw->CON0 |= LL_SPI_CON0_LSB_FIRST;
    } else {
        hw->CON0 &= ~LL_SPI_CON0_LSB_FIRST;
    }

    return RET_OK;
}

static int32 hgspi_v3_set_slave_sw_reserved(struct hgspi_v3 *dev, uint32 enable) {

    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;

    if (enable) {
        hw->SLAVESTA |= LL_SPI_SLAVE_STA_SW_RESERVED;
    } else {
        hw->SLAVESTA &= ~LL_SPI_SLAVE_STA_SW_RESERVED;
    }

    return RET_OK;
}

static int32 hgspi_v3_get_dma_pending(struct hgspi_v3 *dev) {

    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;

    if ((hw->STA1 & LL_SPI_STA_DMA_PENDING))
        return TRUE;
    else
        return FALSE;

}

static int32 hgspi_v3_clear_dma_pending(struct hgspi_v3 *dev, uint32 enable) {

    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw*)dev->hw;

    if(enable) {
        hw->STA1 = LL_SPI_STA_DMA_PENDING;
    }

    return RET_OK;

}


/**********************************************************************************/
/*                          SPI ATTCH FUNCTION                                    */
/**********************************************************************************/

static int32 hgspi_v3_open(struct spi_device *p_spi, uint32 clk_freq, uint32 work_mode,uint32 wire_mode, uint32 clk_mode) {

    struct hgspi_v3 *dev   = (struct hgspi_v3 *)p_spi;
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;
    uint32 con0_reg        = 0;
    uint32 con1_reg        = 0;
    uint32 clk_div_cnt;
    int32  work_mode_to_reg;
    int32  wire_mode_to_reg;
    int32  clk_mode_to_reg ;

    if (dev->opened) {
        if (!dev->dsleep) {
            return -EBUSY;
        }
    }

    /* hal enum */
    work_mode_to_reg = hgspi_v3_switch_hal_work_mode(work_mode);
    wire_mode_to_reg = hgspi_v3_switch_hal_wire_mode(wire_mode);
    clk_mode_to_reg  = hgspi_v3_switch_hal_clk_mode(clk_mode  );
    
    if (((-1) == work_mode_to_reg) || \
        ((-1) == wire_mode_to_reg) || \
        ((-1) == clk_mode_to_reg )) {
        
        return -EINVAL;
    }

    /* pin config */
    if(pin_func(p_spi->dev.dev_id, 1) != RET_OK) {
        return RET_ERR;
    }

    
    /*
     * open SPI clk
     */
    if (SPI0_BASE == (uint32)hw) {
        sysctrl_spi0_clk_open();
    } else if (SPI1_BASE == (uint32)hw) {
        sysctrl_spi1_clk_open();
    } else if (SPI2_BASE == (uint32)hw) {
        sysctrl_spi2_clk_open();
    }

    /* clear reg */
    hw->CON0     = 0;
    hw->CON1     = 0;
    hw->TIMECON  = 0;
    hw->TDMALEN  = 0;
    hw->RDMALEN  = 0;
    hw->TSTADR   = 0;
    hw->RSTADR   = 0;
    hw->SLAVESTA = 0;


    hgspi_v3_disable(hw);

    con0_reg = LL_SPI_CON0_SPI_MODE(clk_mode_to_reg  )                        | //clk mode
               LL_SPI_CON0_WIRE_MODE(wire_mode_to_reg)                        | //wire mode
               ((work_mode_to_reg == 2) ? (LL_SPI_CON0_SLAVE_STATE_EN):(0UL)) | //slave fsm mode
               LL_SPI_CON0_FRAME_SIZE(8)                                      | //frame size
               LL_SPI_CON0_SSOE(1)                                            | //nss io_oe enable
               LL_SPI_CON0_NSS_PIN_EN;

    con1_reg = LL_SPI_CON1_SPI_I2C_SEL(0x0)                                   |
               ((work_mode_to_reg == 2) ? (LL_SPI_CON1_MODE(1)) : LL_SPI_CON1_MODE(work_mode_to_reg));

    hw->CON0 = con0_reg  ;
    hw->CON1 = con1_reg  ;
    hw->STA1 = 0xFFFFFFFF;

    /* only master mode need config clk */
    if (work_mode_to_reg == 0) {
        /* set spi clk */
        clk_div_cnt = peripheral_clock_get(HG_APB0_PT_SPI0) / 2 / clk_freq - 1;
        ASSERT((clk_div_cnt >= 0) && (clk_div_cnt <= 65535));
        hw->TIMECON = (hw->TIMECON &~ (LL_SPI_TIMECON_BAUD(0xFFFF))) | LL_SPI_TIMECON_BAUD(clk_div_cnt);
    }

    if (!(hw->CON0 & LL_SPI_CON0_SLAVE_STATE_EN)) {
        /* open dma done interrupt for write&read func */
        hw->CON1 |= LL_SPI_CON1_DMA_IE_EN;
    }
    
    irq_enable(dev->irq_num);

    /* set CS high */
    hgspi_v3_set_cs(hw, 1);

    /* enable spi */
    hgspi_v3_enable(hw);
    
    dev->opened = 1;
    dev->dsleep = 0;
    /* config default tx&rx len threshold, uint:byte */
    dev->len_threshold        = 16;
    /* other config init */
    dev->spi_irq_flag_rx_done = 0;
    dev->spi_irq_flag_tx_done = 0;
    dev->spi_tx_done          = 0;
    dev->spi_rx_done          = 0;
    dev->spi_tx_async         = 0;
    dev->spi_rx_async         = 0;
    dev->timeout              = osWaitForever;
    
    return RET_OK;
}

static int32 hgspi_v3_close(struct spi_device *p_spi) {

    struct hgspi_v3 *dev = (struct hgspi_v3 *)p_spi;
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;

    if (!dev->opened) {
        return RET_OK;
    }

    /*
     * close SPI clk
     */
    if (SPI0_BASE == (uint32)hw) {
        sysctrl_spi0_clk_close();
    } else if (SPI1_BASE == (uint32)hw) {
        sysctrl_spi1_clk_close();
    } else if (SPI2_BASE == (uint32)hw) {
        sysctrl_spi2_clk_close();
    }


    irq_disable(dev->irq_num     );
    pin_func(p_spi->dev.dev_id, 0);
    hgspi_v3_set_cs(hw, 1);
    hgspi_v3_disable(hw  );
    
    dev->opened = 0;
    dev->dsleep = 0;
    
    /* Just to be sure: config again default tx&rx len threshold when close, uint:byte */
    dev->len_threshold        = 16;
    /* other setting clear */
    dev->spi_irq_flag_rx_done = 0;
    dev->spi_irq_flag_tx_done = 0;
    dev->spi_tx_done          = 0;
    dev->spi_rx_done          = 0;
    dev->spi_tx_async         = 0;
    dev->spi_rx_async         = 0;
    dev->timeout              = osWaitForever;
    
    return RET_OK;
}

static int32 hgspi_v3_write(struct spi_device *p_spi, const void *buf, uint32 size) {

    struct hgspi_v3 *dev = (struct hgspi_v3 *)p_spi;
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;
    uint32 frame_size, index, data;
    uint32 num, addr;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    /* if mode is slave fsm mode */
    if (hw->CON0 & LL_SPI_CON0_SLAVE_STATE_EN) {
        /* 发送异常退出会卡住下一次发送 */
        //while (!hgspi_v3_spi_slave_fsm_get_tx_done_sta(hw));
        hw->TDMACNT = 0;
        /* 清掉BUF cnt，对于slave fsm模式有影响，因为不知道接收时刻，写清除buf会影响可能正在接收的过程 */
        //hw->STA = LL_SPI_STA_CLEAR_BUF_CNT;
        hw->TDMALEN = size;
        hw->TSTADR  = (uint32)buf;
        hgspi_v3_spi_slave_fsm_set_tx_ready_sta(hw, size/8);
        return RET_OK;
    }

    os_mutex_lock(&dev->os_spi_lock, osWaitForever);
    
    /* Clear the last cs rising pending */
    hw->STA1 = LL_SPI_STA_NSS_POS_PENDING;
    /* Clear the rx_dma cnt & tx_dma_cnt */
    hw->TDMACNT = 0;
    /* 清掉BUF cnt */
    hw->STA1 = LL_SPI_STA_CLEAR_BUF_CNT;

    /* clear the tx done pending */
    dev->spi_tx_done  = 0;
    dev->spi_tx_async = 0;

    hgspi_v3_set_dir_tx(hw);

    /* len <= 5(default) use cpu tx dev->tx_len_threshold*/
    if ((size <= dev->len_threshold) && (!__in_interrupt())) {

        /* if mode == slave, use dma tx */
        if (hw->CON1 & LL_SPI_CON1_MODE(1)) {
            goto hgspi_v3_dma_tx;
        }

        addr       = (uint32)buf;
        frame_size = LL_SPI_CON0_GET_FRAME_SIZE(hw->CON0);
        index      = (frame_size > 8) + (frame_size > 16);
        num        = size >> index;

        /* Note: "spi slave mode" set cs is invaild, so it is ok */
        //hgspi_v3_set_cs(hw, 0);

        while(num--) {
            switch(index) {
                case 0:
                    data = *(uint8  *)addr;
                    break;
                case 1:
                    data = *(uint16 *)addr;
                    break;
                default:
                    data = *(uint32 *)addr;
                    break;
            }
            addr += 1<<index;

            while(!hgspi_v3_get_tx_ready_sta(hw));
            hgspi_v3_set_data(hw, data);
        }

        /* Only required for the master mode */
        if(!LL_SPI_CON1_GET_MODE(hw->CON1)) {
            hgspi_v3_wait_tx_done(hw);
        }

        /* Note: "spi slave mode" set cs is invaild, so it is ok */
        //hgspi_v3_set_cs(hw, 1);

        /* cpu tx done irq handle */
        if (dev->spi_irq_flag_tx_done) {
            if (dev->irq_hdl) {
                dev->irq_hdl(SPI_IRQ_FLAG_TX_DONE, dev->irq_data, (uint32)buf, size);
            }
        }
    }
    /* len > 5(default) use dma tx*/
    else {

hgspi_v3_dma_tx:

        while (hw->STA1 & LL_SPI_STA_SSP_BUSY_PENDING);
    
        /* Note: "spi slave mode" set cs is invaild, so it is ok */
        //hgspi_v3_set_cs(hw, 0);
        dev->spi_tx_async = 1;

        hw->TSTADR  = (uint32)buf;
        hw->TDMALEN = size;
        hw->CON1   |= LL_SPI_CON1_DMA_EN;

        #if 0
        //using semaphore for waiting tx done
        os_sema_down(&dev->os_spi_tx_done, osWaitForever);
        #else
        /* others: using semaphore to wait */
        if (1) {
            //using semaphore for waiting tx done
            os_sema_down(&dev->os_spi_tx_done, dev->timeout);
            ///dev->irq_hdl(SPI_IRQ_FLAG_TX_DONE, dev->irq_data, buf, size);
        }
        /* 0: using while to wait */
        else {
            dev->spi_tx_async = 0;
            hgspi_v3_complete(dev);
        }
        #endif
    }

    /* 防止timeout后，突然来数据，触发dma中断 */
    /* 正常接收接收完成，写它没有事 */
    hw->CON1 &= ~LL_SPI_CON1_DMA_EN;
    
    os_mutex_unlock(&dev->os_spi_lock);

    return RET_OK;
}

static int32 hgspi_v3_read(struct spi_device *p_spi, void *buf, uint32 size) {

    struct hgspi_v3 *dev = (struct hgspi_v3 *)p_spi;
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;
    uint32 frame_size, index, data;
    uint32 num, addr;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    /* if mode is slave fsm mode */
    if (hw->CON0 & LL_SPI_CON0_SLAVE_STATE_EN) {
        /* 接收异常退出会卡住下一次接收 */
        //while (!hgspi_v3_spi_slave_fsm_get_rx_done_sta(hw));
        hw->RDMACNT = 0;
        /* 清掉BUF cnt，对于slave fsm模式有影响，因为不知道接收时刻，写清除buf会影响可能正在接收的过程 */
        //hw->STA = LL_SPI_STA_CLEAR_BUF_CNT;
        hw->RDMALEN = size;
        hw->RSTADR  = (uint32)buf;
        hgspi_v3_spi_slave_fsm_set_rx_ready_sta(hw, size/8);
        return RET_OK;
    }

    os_mutex_lock(&dev->os_spi_lock, osWaitForever);
    
    /* Clear the last cs rising pending */
    hw->STA1 =  LL_SPI_STA_NSS_POS_PENDING;
    /* Clear the rx_dma cnt & tx_dma_cnt */
    hw->RDMACNT = 0;
    /* 清掉BUF cnt */
    hw->STA1 = LL_SPI_STA_CLEAR_BUF_CNT;

    /* clear the rx done pending */
    dev->spi_rx_done  = 0;
    dev->spi_rx_async = 0;

    hgspi_v3_set_dir_rx(hw);

    /* len <= 5(default) use cpu tx dev->tx_len_threshold*/
    if ((size <= dev->len_threshold) && (!__in_interrupt())) {

        /* if mode == slave, use dma rx */
        if (hw->CON1 & LL_SPI_CON1_MODE(1)) {
            goto hgspi_v3_dma_rx;
        }
        
        addr       = (uint32)buf;
        frame_size = LL_SPI_CON0_GET_FRAME_SIZE(hw->CON0);
        index      = (frame_size > 8) + (frame_size > 16);
        num        = size >> index;

        /* Note: "spi slave mode" set cs is invaild, so it is ok */
        //hgspi_v3_set_cs(hw, 0);
        
        /* Only required for the master mode */
        if(!LL_SPI_CON1_GET_MODE(hw->CON1)) {
            hw->RDMALEN = LL_SPI_DMA_RX_LEN(num);
            hgspi_v3_set_data(hw, 0x00);
        }
        
        while(num--) {
            
            while(!hgspi_v3_get_rx_ready_sta(hw));
            data = hgspi_v3_get_data(hw);
        
            switch(index) {
                case 0:
                    *(uint8 *)addr = data;
                    break;
                case 1:
                    *(uint16 *)addr = data;
                    break;
                default:
                    *(uint32 *)addr = data;
                    break;
            }
            addr += 1<<index;
       }

        /* Note: "spi slave mode" set cs is invaild, so it is ok */
        //hgspi_v3_set_cs(hw, 1);

        /* cpu rx done irq handle */
        if (dev->spi_irq_flag_rx_done) {
            if (dev->irq_hdl) {
                dev->irq_hdl(SPI_IRQ_FLAG_RX_DONE, dev->irq_data, (uint32)buf, size);
            }
        }
    }
    /* len > 5(default) use dma tx*/
    else {

hgspi_v3_dma_rx:

        while (hw->STA1 & LL_SPI_STA_SSP_BUSY_PENDING);
    
        /* Note: "spi slave mode" set cs is invaild, so it is ok */
        //hgspi_v3_set_cs(hw, 0);
        dev->spi_rx_async = 1;
        
        hw->RSTADR  = (uint32)buf;
        hw->RDMALEN = size;
        hw->CON1   |= LL_SPI_CON1_DMA_EN;

        #if 0
        //using semaphore for waiting tx done
        os_sema_down(&dev->os_spi_rx_done, osWaitForever);
        #else
        /* others: using semaphore to wait */
        if (1) {
            //using semaphore for waiting tx done
            os_sema_down(&dev->os_spi_rx_done, dev->timeout);
            //dev->irq_hdl(SPI_IRQ_FLAG_RX_DONE, dev->irq_data, buf, size);
        }
        /* 0: using while to wait */
        else {
            dev->spi_rx_async = 0;
            hgspi_v3_complete(dev);
        }
        #endif


    }

    /* 防止timeout后，突然来数据，触发dma中断 */
    /* 正常接收接收完成，写它没有事 */
    hw->CON1 &= ~LL_SPI_CON1_DMA_EN;

    os_mutex_unlock(&dev->os_spi_lock);

    return RET_OK;
}

#ifdef CONFIG_SLEEP
static int32 hgspi_v3_suspend(struct dev_obj *obj)
{
    struct hgspi_v3 *dev = (struct hgspi_v3 *)obj;
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_OK;
    }

    if(os_mutex_lock(&dev->os_spi_lock, osWaitForever)) {
        return RET_ERR;
    }

    if (0 > os_mutex_lock(&dev->bp_suspend_lock, 10000)) {
        return RET_ERR;
    }


    /*!
     * Close the SPI 
     */
    hw->CON1 &= ~ BIT(0);


    pin_func(dev->dev.dev.dev_id, 0);


    /*
     * close irq
     */
    irq_disable(dev->irq_num);

    /*
     * clear pending  
     */
    hw->STA1    = 0xFFFFFFFF;

    os_memset((void *)&dev->bp_regs, 0, sizeof(dev->bp_regs));

    /* 
     * save the reglist
     */
    dev->bp_regs.con0     = hw->CON0    ;
    dev->bp_regs.con1     = hw->CON1    ;
    dev->bp_regs.timecon  = hw->TIMECON ;
    dev->bp_regs.tdmalen  = hw->TDMALEN ;
    dev->bp_regs.rdmalen  = hw->RDMALEN ;
    dev->bp_regs.tstadr   = hw->TSTADR  ;
    dev->bp_regs.rstadr   = hw->RSTADR  ;
    dev->bp_regs.slavesta = hw->SLAVESTA;


    /*
     * save the irq_hdl created by user
     */
    dev->bp_irq_hdl  = dev->irq_hdl;
    dev->bp_irq_data = dev->irq_data;

    /*
     * close SPI clk
     */
    if (SPI0_BASE == (uint32)hw) {
        sysctrl_spi0_clk_close();
    } else if (SPI1_BASE == (uint32)hw) {
        sysctrl_spi1_clk_close();
    } else if (SPI2_BASE == (uint32)hw) {
        sysctrl_spi2_clk_close();
    }

    dev->dsleep = 1;

    os_mutex_unlock(&dev->bp_suspend_lock);

    return RET_OK;
}

static int32 hgspi_v3_resume(struct dev_obj *obj)
{
    struct hgspi_v3 *dev = (struct hgspi_v3 *)obj;
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;

    if ((!dev->opened) || (!dev->dsleep)) {
        return RET_OK;
    }

    if (0 > os_mutex_lock(&dev->bp_resume_lock, 10000)) {
        return RET_ERR;
    }

    /* pin config */
    if (pin_func(dev->dev.dev.dev_id , 1) != RET_OK) {
        return RET_ERR;
    }

    /* 
     * recovery the SPI clk
     */
    if (SPI0_BASE == (uint32)hw) {
        sysctrl_spi0_clk_open();
    } else if (SPI1_BASE == (uint32)hw) {
        sysctrl_spi1_clk_open();
    } else if (SPI2_BASE == (uint32)hw) {
        sysctrl_spi2_clk_open();
    }


    /*
     * recovery the reglist from sram
     */
    hw->CON0     = dev->bp_regs.con0    ;
    hw->CON1     = dev->bp_regs.con1    ;
    hw->TIMECON  = dev->bp_regs.timecon ;
    hw->TDMALEN  = dev->bp_regs.tdmalen ;
    hw->RDMALEN  = dev->bp_regs.rdmalen ;
    hw->TSTADR   = dev->bp_regs.tstadr  ;
    hw->RSTADR   = dev->bp_regs.rstadr  ;
    hw->SLAVESTA = dev->bp_regs.slavesta;

    /*
     * recovery the irq handle and data
     */ 
    dev->irq_hdl  = dev->bp_irq_hdl;
    dev->irq_data = dev->bp_irq_data;

    os_memset((void *)&dev->bp_regs, 0, sizeof(dev->bp_regs));

    /*!
     * Open the SPI 
     */
    hw->CON1 |= BIT(0);

    /*
     * open irq
     */
    irq_enable(dev->irq_num);

    dev->dsleep = 0;

    os_mutex_unlock(&dev->bp_resume_lock);
    os_mutex_unlock(&dev->os_spi_lock);
    return RET_OK;
}
#endif


static int32 hgspi_v3_ioctl(struct spi_device *p_spi, uint32 cmd, uint32 param1, uint32 param2)
{
    int32 ret_val = RET_OK;
    struct hgspi_v3 *dev = (struct hgspi_v3 *)p_spi;
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;


    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }


    switch(cmd) {
        case (SPI_SET_FRAME_SIZE   ):
            ret_val = hgspi_v3_set_frame_size(hw, param1);
            break;
        case (SPI_WIRE_MODE_SET    ):
            ret_val = hgspi_v3_set_wire_mode(hw , param1);
            break;
        case (SPI_WIRE_MODE_GET    ):
            ret_val = hgspi_v3_get_wire_mode(hw);
            break;
        case (SPI_SET_LEN_THRESHOLD):
            ret_val = hgspi_v3_spi_set_len_threshold(dev, param1);
            break;
        case (SPI_SET_CS):
            ret_val = hgspi_v3_cs(dev, param1, param2);
            break;
        case (SPI_CFG_SET_NONE_CS):
            ret_val = hgspi_v3_set_cfg_none_cs(dev, param1);
            break;
        case (SPI_READ_DMA_RX_CNT):
            ret_val = hgspi_v3_read_dma_rx_cnt(dev, (uint32 *)param1);
            break;
        case (SPI_KICK_DMA_RX):
            ret_val = hgspi_v3_kick_dma_rx(dev, (uint32 *)param1, param2);
            break;
        case (SPI_KICK_DMA_TX):
            ret_val = hgspi_v3_kick_dma_tx(dev, (uint32 *)param1, param2);
            break;
        case (SPI_SET_TIMEOUT):
            ret_val = hgspi_v3_set_timeout(dev, param1);
            break;
        case (SPI_SET_LSB_FIRST):
            ret_val = hgspi_v3_set_lsb_first(dev, param1);
            break;
        case (SPI_SET_SLAVE_SW_RESERVED):
            ret_val = hgspi_v3_set_slave_sw_reserved(dev, param1);
            break;
		
		case (SPI_SENSOR_PINGPANG_EN):
			ret_val = hgspi_v3_set_pingpang(dev, param1);
			break;
		case (SPI_SENSOR_SET_ADR0):
			ret_val = hgspi_v3_set_pingpang_adr0(dev, param1);
			break;	
		case (SPI_SENSOR_SET_ADR1):
			ret_val = hgspi_v3_set_pingpang_adr1(dev, param1);
			break;		
		case (SPI_SENSOR_BUF_LEN):
			ret_val = hgspi_v3_set_len(dev, param1);
			break;
		case (SPI_SENSOR_VSYNC_TIMEOUT):
			ret_val = hgspi_v3_set_vsync_timeout(dev, param1);
			break;
		case (SPI_SENSOR_VSYNC_HEAD_CNT):
			ret_val = hgspi_v3_set_vsync_head(dev, param1);
			break;
		case (SPI_SENSOR_HSYNC_TIMEOUT):
			ret_val = hgspi_v3_set_hsync_timeout(dev, param1);
			break;
		case (SPI_SENSOR_HSYNC_HEAD_CNT):
			ret_val = hgspi_v3_set_hsync_head(dev, param1);
			break;		
		case (SPI_SENSOR_VSYNC_EN):
			ret_val = hgspi_v3_set_vsync_en(dev, param1);
			break;			
		case (SPI_HIGH_SPEED_CFG):
			ret_val = hgspi_v3_set_high_speed(dev, param1);
			break;
		case (SPI_KICK_DMA_EN):
			ret_val = hgspi_v3_kick_dma(dev);				
			break;
		case (SPI_RX_TIMEOUT_CFG):	
			ret_val = hgspi_v3_rxtimeout_cfg(dev,1,param2);				
			break;
        case (SPI_GET_DMA_PENDING):
            ret_val = hgspi_v3_get_dma_pending(dev);
            break;
        case (SPI_CLEAR_DMA_PENDING):
            ret_val = hgspi_v3_clear_dma_pending(dev, param1);
            break;
		case (SPI_SENSOR_LINE_LEN_CFG):
			ret_val = hgspi_v3_set_line_len(dev, param1);
			break;
		default:
            ret_val = -ENOTSUPP;
            break;
    }
    
    return ret_val;
}

/* interrupt handler */
static void hgspi_v3_irq_handler(void *data)
{
    struct hgspi_v3 *dev = (struct hgspi_v3 *)data;
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;

    if ((hw->CON1 & LL_SPI_CON1_DMA_IE_EN) && (hw->STA1 & LL_SPI_STA_DMA_PENDING)) {
        /* clear interrupt flag */
        hw->STA1 = LL_SPI_STA_DMA_PENDING;
        /* For spi write&read, Note: "spi slave mode" set cs is invaild */
        //hgspi_v3_set_cs(hw, 1);

        /* spi transfer done, recording for tx_sync & rx_sync */
        if (!dev->spi_rx_async) {
            dev->spi_rx_done = 1;
        }

        if (!dev->spi_tx_async) {
            dev->spi_tx_done = 1;
        }

        //spi module async tx 
        if (dev->spi_tx_async) {
            dev->spi_tx_async = 0;
            os_sema_up(&dev->os_spi_tx_done);
        }
        
        //spi module async rx 
        if (dev->spi_rx_async){
            dev->spi_rx_async = 0;
            os_sema_up(&dev->os_spi_rx_done);
        }

        if (hw->CON1 & LL_SPI_CON1_TX_EN) {
            if (dev->irq_hdl && dev->spi_irq_flag_tx_done) {
                dev->irq_hdl(SPI_IRQ_FLAG_TX_DONE, dev->irq_data, BIT(29) | hw->TSTADR, hw->TDMACNT);
            }
        } else {
            if (dev->irq_hdl && dev->spi_irq_flag_rx_done) {
                dev->irq_hdl(SPI_IRQ_FLAG_RX_DONE, dev->irq_data, BIT(29) | hw->RSTADR, hw->RDMACNT);
            }
        }
    }



    if ((hw->CON1 & LL_SPI_CON1_BUF_OV_IE_EN) && (hw->STA1 & LL_SPI_STA_BUF_OV_PENDING)) {
        /* clear interrupt flag */
        hw->STA1 = LL_SPI_STA_BUF_OV_PENDING;
        
        if (dev->irq_hdl) {
            dev->irq_hdl(SPI_IRQ_FLAG_FIFO_OVERFLOW, dev->irq_data, 0, 0);
        }
    }

    if ((hw->CON1 & LL_SPI_CON1_RX_TIMEOUT_IE(1)) && (hw->STA1 & LL_SPI_STA_RX_TIMEOUT_PEND)) {
        /* clear interrupt flag */
        hw->STA1 = LL_SPI_STA_RX_TIMEOUT_PEND;
        if (dev->irq_hdl) {
            dev->irq_hdl(SPI_IRQ_FLAG_RX_TIMEOUT, dev->irq_data, 0, 0);
        }
    }


    if ((hw->CON0 & LL_SPI_CON0_NSS_POS_IE_EN) && (hw->STA1 & LL_SPI_STA_NSS_POS_PENDING)) {
        /* clear interrupt flag */
        hw->STA1 = LL_SPI_STA_NSS_POS_PENDING;
        
        if (dev->irq_hdl) {
            dev->irq_hdl(SPI_IRQ_FLAG_CS_RISING, dev->irq_data, 0, 0);
        }

        /* spi salve transfer done when detect cs rising */
        if ((hw->CON1 & LL_SPI_CON1_MODE(1))) {
            if (!dev->spi_tx_async) {
                dev->spi_tx_done = 1;
            }
            
            if (!dev->spi_rx_async) {
                dev->spi_rx_done = 1;
            }
        }
    }


    //// SLAVE FSM IRQ ////
    if ((hw->CON0 & LL_SPI_CON0_SLAVE_WRDATA_IE_EN) && (hw->STA1 & LL_SPI_STA_SLAVE_WRDATA_PENDING)) {
        /* clear interrupt flag */
        hw->STA1 = LL_SPI_STA_SLAVE_WRDATA_PENDING;
        
        if (dev->irq_hdl) {
            dev->irq_hdl(SPI_IRQ_FLAG_RX_DONE, dev->irq_data, BIT(29) | hw->TSTADR, hw->TDMACNT);
        }
    }
    if ((hw->CON0 & LL_SPI_CON0_SLAVE_RDDATA_IE_EN) && (hw->STA1 & LL_SPI_STA_SLAVE_RDDATA_PENDING)) {
        /* clear interrupt flag */
        hw->STA1 = LL_SPI_STA_SLAVE_RDDATA_PENDING;
        if (dev->irq_hdl) {
            dev->irq_hdl(SPI_IRQ_FLAG_TX_DONE, dev->irq_data, BIT(29) | hw->RSTADR, hw->RDMACNT);
        }
    }
    if ((hw->CON0 & LL_SPI_CON0_SLAVE_RDSTATUS_IE_EN) && (hw->STA1 & LL_SPI_STA_SLAVE_RDSTATUS_PENDING)) {
        /* clear interrupt flag */
        hw->STA1 = LL_SPI_STA_SLAVE_RDSTATUS_PENDING;
        
        if (dev->irq_hdl) {
            dev->irq_hdl(SPI_IRQ_FLAG_SLAVE_FSM_READ_STATUS, dev->irq_data, 0, 0);
        }
    }
    if ((hw->CON0 & LL_SPI_CON0_SLAVE_WRONG_CMD_IE) && (hw->STA1 & LL_SPI_STA_SLAVE_WRONG_CMD_PENDING)) {
        /* clear interrupt flag */
        hw->STA1 = LL_SPI_STA_SLAVE_WRONG_CMD_PENDING;
        
        if (dev->irq_hdl) {
            dev->irq_hdl(SPI_IRQ_FLAG_SLAVE_FSM_WRONG_CMD, dev->irq_data, 0, 0);
        }
    }
}

static int32 hgspi_v3_request_irq(struct spi_device *p_spi, uint32 irq_flag, spi_irq_hdl irqhdl, uint32 irq_data)
{
    struct hgspi_v3 *dev = (struct hgspi_v3 *)p_spi;
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    dev->irq_hdl  = irqhdl;
    dev->irq_data = irq_data;
    request_irq(dev->irq_num, hgspi_v3_irq_handler, dev);

    if (irq_flag & SPI_IRQ_FLAG_TX_DONE) {
        if (hw->CON0 & LL_SPI_CON0_SLAVE_STATE_EN) {
            hw->CON0 |= LL_SPI_CON0_SLAVE_RDDATA_IE_EN;
        } else {
            dev->spi_irq_flag_tx_done = 1;
            hw->CON1 |= LL_SPI_CON1_DMA_IE_EN;
        }
    }

    if (irq_flag & SPI_IRQ_FLAG_RX_DONE) {
        if (hw->CON0 & LL_SPI_CON0_SLAVE_STATE_EN) {
            hw->CON0 |= LL_SPI_CON0_SLAVE_WRDATA_IE_EN;
        } else {
            dev->spi_irq_flag_rx_done = 1;
            hw->CON1 |= LL_SPI_CON1_DMA_IE_EN;
        }
    }

	if (irq_flag & SPI_IRQ_FLAG_RX_TIMEOUT) {
		hw->CON1 |= LL_SPI_CON1_RX_TIMEOUT_IE(1);
	}

    if (irq_flag & SPI_IRQ_FLAG_CS_RISING) {
        hw->CON1 |= LL_SPI_CON0_NSS_POS_IE_EN;
    }

    if (irq_flag & SPI_IRQ_FLAG_FIFO_OVERFLOW) {
        hw->CON1 |= LL_SPI_CON1_BUF_OV_IE_EN;
    }

    if (irq_flag & SPI_IRQ_FLAG_SLAVE_FSM_READ_STATUS) {
        hw->CON0 |= LL_SPI_CON0_SLAVE_RDSTATUS_IE_EN;
    }
    
    if (irq_flag & SPI_IRQ_FLAG_SLAVE_FSM_WRONG_CMD) {
        hw->CON0 |= LL_SPI_CON0_SLAVE_WRONG_CMD_IE;
    }
    
    irq_enable(dev->irq_num);
    
    return RET_OK;
}

static int32 hgspi_v3_release_irq(struct spi_device *p_spi, uint32 irq_flag)
{
    struct hgspi_v3 *dev = (struct hgspi_v3 *)p_spi;
    struct hgspi_v3_hw *hw = (struct hgspi_v3_hw *)dev->hw;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }


    if (irq_flag & SPI_IRQ_FLAG_TX_DONE) {
        if (hw->CON0 & LL_SPI_CON0_SLAVE_STATE_EN) {
            hw->CON0 &= ~LL_SPI_CON0_SLAVE_RDDATA_IE_EN;
        } else {
            dev->spi_irq_flag_tx_done = 0;
            hw->CON1 &= ~ LL_SPI_CON1_DMA_IE_EN;
        }
    }

    if (irq_flag & SPI_IRQ_FLAG_RX_DONE) {
        if (hw->CON0 & LL_SPI_CON0_SLAVE_STATE_EN) {
            hw->CON0 &= ~LL_SPI_CON0_SLAVE_WRDATA_IE_EN;
        } else {
            dev->spi_irq_flag_rx_done = 0;
            hw->CON1 &= ~ LL_SPI_CON1_DMA_IE_EN;
        }
    }

    if (irq_flag & SPI_IRQ_FLAG_CS_RISING) {
        hw->CON1 &= ~ LL_SPI_CON0_NSS_POS_IE_EN;
    }

    if (irq_flag & SPI_IRQ_FLAG_FIFO_OVERFLOW) {
        hw->CON1 &= ~ LL_SPI_CON1_BUF_OV_IE_EN;
    }

    if (irq_flag & SPI_IRQ_FLAG_SLAVE_FSM_READ_STATUS) {
        hw->CON0 &= ~ LL_SPI_CON0_SLAVE_RDSTATUS_IE_EN;
    }
    
    if (irq_flag & SPI_IRQ_FLAG_SLAVE_FSM_WRONG_CMD) {
        hw->CON0 &= ~ LL_SPI_CON0_SLAVE_WRONG_CMD_IE;
    }
    return RET_OK;
}

static const struct spi_hal_ops spi_v3_ops = {
    .open             = hgspi_v3_open,
    .close            = hgspi_v3_close,
    .ioctl            = hgspi_v3_ioctl,
    .read             = hgspi_v3_read,
    .write            = hgspi_v3_write,
    .request_irq      = hgspi_v3_request_irq,
    .release_irq      = hgspi_v3_release_irq,
#ifdef CONFIG_SLEEP 
    .ops.suspend      = hgspi_v3_suspend,
    .ops.resume       = hgspi_v3_resume,
#endif
};

int32 hgspi_v3_attach(uint32 dev_id, struct hgspi_v3 *p_spi)
{
    p_spi->opened               = 0;
    p_spi->dsleep               = 0;
    p_spi->irq_hdl              = NULL;
    p_spi->irq_data             = 0;
    p_spi->timeout              = osWaitForever;
    p_spi->spi_tx_done          = 0;
    p_spi->spi_tx_done          = 0;
    p_spi->spi_tx_async         = 0;
    p_spi->spi_rx_async         = 0;
    p_spi->spi_irq_flag_tx_done = 0;
    p_spi->spi_irq_flag_rx_done = 0;
    p_spi->dev.dev.ops          = (const struct devobj_ops *)&spi_v3_ops;
#ifdef CONFIG_SLEEP
    os_mutex_init(&p_spi->bp_suspend_lock);
    os_mutex_init(&p_spi->bp_resume_lock);
#endif
    os_sema_init(&p_spi->os_spi_tx_done, 0);
    os_sema_init(&p_spi->os_spi_rx_done, 0);
    //os_mutex_init(&p_spi->os_spi_tx_lock);
    //os_mutex_init(&p_spi->os_spi_rx_lock);
    os_mutex_init(&p_spi->os_spi_lock);
    request_irq(p_spi->irq_num, hgspi_v3_irq_handler, p_spi);
    dev_register(dev_id, (struct dev_obj *)p_spi);
    return RET_OK;
}
