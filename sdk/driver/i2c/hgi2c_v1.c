/**
 * @file hgi2c_v1.c
 * @author bxd
 * @brief iic
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
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "hal/i2c.h"
#include "dev/i2c/hgi2c_v1.h"
#include "hgi2c_v1_hw.h"

/**********************************************************************************/
/*                        IIC LOW LAYER FUNCTION                                  */
/**********************************************************************************/

static inline void hgi2c_v1_set_dir_tx(struct hgi2c_v1_hw *p_i2c)
{
    p_i2c->CON1 |= LL_IIC_CON1_TX_EN;
}

static inline void hgi2c_v1_set_dir_rx(struct hgi2c_v1_hw *p_i2c)
{
    p_i2c->CON1 &= ~LL_IIC_CON1_TX_EN;
}

static inline void hgi2c_v1_set_ack(struct hgi2c_v1_hw *p_i2c)
{
    p_i2c->CON0 &= ~LL_IIC_CON0_TX_NACK;
}

static inline void hgi2c_v1_set_nack(struct hgi2c_v1_hw *p_i2c)
{
    p_i2c->CON0 |= LL_IIC_CON0_TX_NACK;
}

static inline void hgi2c_v1_set_write_table_mode(struct hgi2c_v1_hw *p_i2c)
{
    p_i2c->CON0 |= LL_IIC_CON0_ISP_MODE_EN;
}
static inline void hgi2c_v1_set_write_table_cmd_len(struct hgi2c_v1_hw *p_i2c, uint8 len)
{
    p_i2c->CON0 = (p_i2c->CON0 & ~LL_IIC_CON0_ISP_CMD_LEN(0XF)) | LL_IIC_CON0_ISP_CMD_LEN(len);
}
static inline int32 hgi2c_v1_get_nack_pending(struct hgi2c_v1_hw *p_i2c)
{
    return ((p_i2c->STA2 & LL_IIC_STA2_RX_NACK(0x1)));	
}

static inline int32 hgi2c_v1_get_busy(struct hgi2c_v1_hw *p_i2c)
{
    __NOP();__NOP();
    return ((p_i2c->STA1 & LL_IIC_STA1_SSP_BUSY_PENDING));
}

static inline int32 hgi2c_v1_get_rx_ready_sta(struct hgi2c_v1_hw *p_i2c)
{
    return (!(p_i2c->STA1 & LL_IIC_STA1_BUF_EMPTY_PENDING));
}

static inline int32 hgi2c_v1_get_tx_ready_sta(struct hgi2c_v1_hw *p_i2c)
{
    return (!(p_i2c->STA1 & LL_IIC_STA1_BUF_FULL_PENDING));
}
static inline int32 hgi2c_v1_get_tx_table_sta(struct hgi2c_v1_hw *p_i2c)
{
    return ((p_i2c->STA1 & LL_IIC_STA1_DMA_PENDING));
}


static inline uint32 hgi2c_v1_get_slave_address(struct hgi2c_v1_hw *p_i2c)
{
    return ((p_i2c->OWNADRCON) & 0x3FF);
}


static inline void hgi2c_v1_write_data(struct hgi2c_v1_hw *p_i2c,uint32 data)
{
    ((p_i2c)->CMD_DATA = (data));
}

static inline uint32 hgi2c_v1_get_slave_address_bitwidth(struct hgi2c_v1_hw *p_i2c)
{
    return p_i2c->OWNADRCON & LL_IIC_OWNADRCON_OWN_ADR1_S(0x1);
}

static inline uint32 hgi2c_v1_is_slave_mode(struct hgi2c_v1_hw *p_i2c)
{
    return ((p_i2c->CON1 >> 2) & BIT(0));
}

void Delay_nopCnt(uint32 cnt)
{
    volatile uint32 dat;
    dat = cnt;
    while(dat-->0)
    {
    	;
    }
}

static inline uint32 hgi2c_v1_set_slave_addr(struct hgi2c_v1_hw *hw, uint32 addr) {

    if (addr > 0x3FF) {
        return RET_ERR;
    }

    hw->OWNADRCON = (hw->OWNADRCON &~ LL_IIC_OWNADRCON_OWN_ADR1(0x3FF)) | LL_IIC_OWNADRCON_OWN_ADR1(addr);

    return RET_OK;
}

void  hgi2c_v1_master_tx(struct hgi2c_v1_hw *p_iic, uint8 data, uint8 flag)
{
    uint32 tx_data = data;
    
    if(flag & LL_IIC_START_FLAG) {
        tx_data |= LL_IIC_CMD_DATA_START_BIT_EN;
    }

    if(flag & LL_IIC_STOP_FLAG) {
        tx_data |= LL_IIC_CMD_DATA_STOP_BIT_EN;
    }
    
    hgi2c_v1_set_dir_tx(p_iic);
    hgi2c_v1_write_data(p_iic,tx_data);
    
    /* Waiting for transmission to complete */
    while(hgi2c_v1_get_busy(p_iic));
    /* Set to input to prevent pulling SCL */
    hgi2c_v1_set_dir_rx(p_iic);
}


static uint8 hgi2c_v1_master_rx(struct hgi2c_v1_hw *p_iic, uint8 flag)
{
    uint32 data = 0;
    
    if(flag & LL_IIC_NACK_FLAG) {
        p_iic->CON0 |= LL_IIC_CON0_TX_NACK;
    } else {
        p_iic->CON0 &= ~LL_IIC_CON0_TX_NACK;
    }
    if(flag & LL_IIC_START_FLAG) {
        data |= LL_IIC_CMD_DATA_START_BIT_EN;
    }
    if(flag & LL_IIC_STOP_FLAG) {
        data |= LL_IIC_CMD_DATA_STOP_BIT_EN;
    }
    
    hgi2c_v1_set_dir_rx(p_iic);
    /* Configure the number of data to receive */
    p_iic->RDMALEN   = LL_IIC_DMA_RX_LEN(1);
    /* When receiving, you need to write the flag to be sent
       by the host first.
    */
    //p_iic->CMD_DATA  = data;
    hgi2c_v1_write_data(p_iic,data);
    /* Waiting for transmission to complete */
    while(hgi2c_v1_get_busy(p_iic));
	//Delay_nopCnt(2000);
    return p_iic->CMD_DATA;
}


/* Baudrate is SCL frequency for master mode, and is delay time in APB0_CLK uint for slave mode */
static int32 hgi2c_v1_set_baudrate(struct i2c_device *i2c, uint32 baudrate)
{
    struct hgi2c_v1 *dev = (struct hgi2c_v1 *)i2c;
    struct hgi2c_v1_hw *hw = (struct hgi2c_v1_hw *)dev->hw;

    uint32 sclh_cnt = 0;

    /*!
     * 预分频默认16，即tPRESC = 1/(SYSCLK/16)
     * sclh_cnt = (T/2) / (tPRESC)，其中(T/2) = ((1/baudrate)/2)
     */
    sclh_cnt = peripheral_clock_get(HG_APB0_PT_IIC0)/(16*2*baudrate);
    
    ASSERT((sclh_cnt >= 0) && (sclh_cnt <= 2048));

    hw->TIMECON = LL_IIC_TIMECON_PRESC(16-1)                |\
                  LL_IIC_TIMECON_SCLH(sclh_cnt-1)           |\
                  LL_IIC_TIMECON_SCLL(sclh_cnt-1)           |\
                  LL_IIC_TIMECON_SDADEL((sclh_cnt/2)-1)     |\
                  LL_IIC_TIMECON_SCLDEL((sclh_cnt/2)-1)     ;
    
    return RET_OK;
}

static int32 hgi2c_v1_strong_output(struct i2c_device *i2c, uint32 enable) {
    struct hgi2c_v1 *dev = (struct hgi2c_v1*)i2c;
    struct hgi2c_v1_hw *hw = (struct hgi2c_v1_hw *)dev->hw;

    
    if (enable) {
        hw->CON0 |=   LL_IIC_CON0_STRONG_DRV_EN;
    } else {
        hw->CON0 &= ~ LL_IIC_CON0_STRONG_DRV_EN;
    }
    
    return RET_OK;
}


static int32 hgi2c_v1_filtering(struct i2c_device *i2c, uint32 value) {
    struct hgi2c_v1 *dev = (struct hgi2c_v1*)i2c;
    struct hgi2c_v1_hw *hw = (struct hgi2c_v1_hw *)dev->hw;
    
    if (value < 0 || value > 0x1F) {
        return -EINVAL;
    }

    hw->CON0 = (hw->CON0 &~(LL_IIC_CON0_I2C_FILTER_MAX(0xFF))) | LL_IIC_CON0_I2C_FILTER_MAX(value);
    
    return RET_OK;
}

/**********************************************************************************/
/*                          IIC ATTCH FUNCTION                                    */
/**********************************************************************************/

static int32 hgi2c_v1_open(struct i2c_device *i2c, enum i2c_mode mode_sel, enum i2c_addr_mode addr_bit_sel, uint32 addr) {

    struct hgi2c_v1 *dev = (struct hgi2c_v1 *)i2c;
    struct hgi2c_v1_hw *hw = (struct hgi2c_v1_hw *)dev->hw;

    if (dev->opened) {
        if (!dev->dsleep) {
            return -EBUSY;
        }
    }

    if (pin_func(i2c->dev.dev_id, 1) != RET_OK) {
        return RET_ERR;
    }

    /*
     * open IIC clk
     */
    if (SPI0_BASE == (uint32)hw) {
        sysctrl_spi0_clk_open();
    } else if (SPI1_BASE == (uint32)hw) {
        sysctrl_spi1_clk_open();
    } else if (SPI2_BASE == (uint32)hw) {
        sysctrl_spi2_clk_open();
    }


    /* clear reg */
     hw->CON0         = 0;
     hw->CON1         = 0;
     hw->TIMECON      = 0;
     hw->TDMALEN      = 0;
     hw->RDMALEN      = 0;
     hw->TSTADR       = 0;
     hw->RSTADR       = 0;
     hw->OWNADRCON    = 0;
     hw->RXTIMEOUTCON = 0;
     hw->STA1         = 0xFFFFFFFF;
     hw->STA2         = 0xFFFFFFFF;

    
    hw->CON1 = LL_IIC_CON1_IIC_SEL;
    if(mode_sel == IIC_MODE_MASTER) {
        hw->CON1 |= LL_IIC_CON1_MODE(0x0);
    } else {
        hw->CON1 |= LL_IIC_CON1_MODE(0x1);
        if(addr_bit_sel == IIC_ADDR_10BIT) {
            hw->OWNADRCON = LL_IIC_OWNADRCON_OWN_ADR1_S(0x1);
        } else {
            hw->OWNADRCON = LL_IIC_OWNADRCON_OWN_ADR1_S(0x0);
        }
    }

    /* Open DMA tansfer done interrupt */
    hw->CON1 |= LL_IIC_CON1_DMA_IE_EN;

    /* Address for master mode is slave device that attempted to communicate 
       or itself slave address in slave mode */
    hw->OWNADRCON = (hw->OWNADRCON &~ LL_IIC_OWNADRCON_OWN_ADR1(0x3FF)) | LL_IIC_OWNADRCON_OWN_ADR1(addr);
    hw->CON1 |= LL_IIC_CON1_SSP_EN;

    dev->opened          = 1;
    dev->dsleep          = 0;
    dev->flag_rx_done    = 1;
    dev->flag_tx_done    = 1;
    dev->flag_table_done      = 1;
    dev->flag_table_config_en = 0;
    dev->irq_rx_done_en  = 0;
    dev->irq_tx_done_en  = 0;

    irq_enable(dev->irq_num);
    
    return RET_OK;
}

static int32 hgi2c_v1_close(struct i2c_device *i2c)
{
    struct hgi2c_v1 *dev = (struct hgi2c_v1 *)i2c;
    struct hgi2c_v1_hw *hw = (struct hgi2c_v1_hw *)dev->hw;
    
    if (!dev->opened) {
        return RET_OK;
    }
    
    /*
     * close IIC clk
     */
    if (SPI0_BASE == (uint32)hw) {
        sysctrl_spi0_clk_close();
    } else if (SPI1_BASE == (uint32)hw) {
        sysctrl_spi1_clk_close();
    } else if (SPI2_BASE == (uint32)hw) {
        sysctrl_spi2_clk_close();
    }


    irq_disable(dev->irq_num);

    hw->CON1 &= ~LL_IIC_CON1_SSP_EN;
    
    pin_func(i2c->dev.dev_id, 0);
    dev->irq_hdl         = NULL;
    dev->irq_data        = 0;
    dev->opened          = 0;
    dev->dsleep          = 0;
    dev->flag_rx_done    = 1;
    dev->flag_tx_done    = 1;
    dev->irq_rx_done_en  = 0;
    dev->irq_tx_done_en  = 0;

    
    return RET_OK;
}


static int32 hgi2c_v1_read(struct i2c_device *i2c, int8 *addr_buf, uint32 addr_len, int8 *buf, uint32 len) {

    struct hgi2c_v1 *dev = (struct hgi2c_v1 *)i2c;
    struct hgi2c_v1_hw *hw = (struct hgi2c_v1_hw *)dev->hw;
    uint32 slave_addr      = 0;
    uint32 len_temp        = len;
    uint32 addr_len_temp   = addr_len;
    uint32 len_offset      = 0;
    int32  ret_sema        = 0;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }


    /* Make sure the write done */
    if (!dev->flag_tx_done || !dev->flag_table_done) {
        return RET_ERR;
    }

    /* make sure that a least one buf is existent */
    if ((NULL == addr_buf) && (NULL == buf)) {
        return RET_ERR;
    }

    /* make sure that a least one len is existent */
    if ((0 == addr_len) && (0 == len)) {
        return RET_ERR;
    }

    os_mutex_lock(&dev->i2c_lock, osWaitForever);

    /* Indcate IIC module is reading */
    dev->flag_rx_done = 0;

    /* IIC master mode */
    if (!hgi2c_v1_is_slave_mode(hw)) {
        /* set IIC to send status */
        hgi2c_v1_set_dir_tx(hw);
        
        /* send the iic slave address configured by "open" function */
        slave_addr = hgi2c_v1_get_slave_address(hw);
        
        if(hgi2c_v1_get_slave_address_bitwidth(hw) == IIC_ADDR_10BIT) {
            hw->CMD_DATA = LL_IIC_CMD_DATA_START_BIT_EN | LL_IIC_CMD_DATA_WRITE(slave_addr >> 8 << 1) | 0xf0;
            /* It doesn't stop here when mode is IIC master */
            while (hgi2c_v1_get_busy(hw));
            hw->CMD_DATA = LL_IIC_CMD_DATA_WRITE(slave_addr);
        } else {
            hw->CMD_DATA = LL_IIC_CMD_DATA_START_BIT_EN | LL_IIC_CMD_DATA_WRITE(slave_addr << 1);
        }

        /* It doesn't stop here when mode is IIC master */
        while (hgi2c_v1_get_busy(hw));

        /* send the addr, may be register address */
        while(addr_len_temp > 0) {
            hw->CMD_DATA = LL_IIC_CMD_DATA_WRITE(*addr_buf++);
            addr_len_temp--;
            /* It doesn't stop here when mode is IIC master */
            while (hgi2c_v1_get_busy(hw));
        }

        if(hgi2c_v1_get_slave_address_bitwidth(hw) == IIC_ADDR_10BIT) {
            // 10bit read operate only need first byte
            hw->CMD_DATA = LL_IIC_CMD_DATA_START_BIT_EN | LL_IIC_CMD_DATA_WRITE(slave_addr >> 8 << 1) | 0xf1;
        } else {
            hw->CMD_DATA = LL_IIC_CMD_DATA_START_BIT_EN | LL_IIC_CMD_DATA_WRITE(slave_addr << 1) | 0x1;
        }
        while (hgi2c_v1_get_busy(hw));
    }

    /* set IIC to receive status */
    hgi2c_v1_set_dir_rx(hw);

    /* data length more than 4095 byte(limited by hardware) */
    len_offset = 0;
    if (len_temp >= 4095) {
        
        do {
            /* set ACK when receive data */
            hgi2c_v1_set_ack(hw);

            hw->RSTADR = (uint32)(buf + len_offset);
            hw->RDMALEN = 4095;
            hw->CON1 |= LL_IIC_CON1_DMA_EN; /* kick DMA */

            /* DMA interrupt to sema_up  */
            /* master use 6s for timeout */
            /* slave use wait forever    */
            if (!hgi2c_v1_is_slave_mode(hw)) {
                ret_sema = os_sema_down(&dev->i2c_done, 6*1000);
                if (!ret_sema) {
                    _os_printf("***IIC modoule info: M read err, 1 phase timeout!\r\n");
                    goto err_handle;
                }
            } else {
                os_sema_down(&dev->i2c_done, osWaitForever);
            }

            len_offset += 4095;
        }while ((len_temp -= 4095) >= 4095);
    }

    /* IIC master mode need to send the stop signal & NACK in last byte */
    if (!hgi2c_v1_is_slave_mode(hw)) {
        
        /* make sure the remain (len-1) more than 0 */
        if (len_temp - 1) {
            
            /* set ACK when receive data */
            hgi2c_v1_set_ack(hw);
                    
            hw->RSTADR = (uint32)(buf + len_offset);
            hw->RDMALEN = len_temp - 1;
            hw->CON1 |= LL_IIC_CON1_DMA_EN; /* kick DMA */
            
            /* DMA interrupt to sema_up  */
            /* master use 6s for timeout */
            ret_sema = os_sema_down(&dev->i2c_done, 6*1000);
            if (!ret_sema) {
                _os_printf("***IIC modoule info: M read err, 2 phase timeout!\r\n");
                goto err_handle;
            }

            len_offset += (len_temp - 1);
        }

        /* set NACK when receive the last byte */
        hgi2c_v1_set_nack(hw);
        
        //send the last byte with the stop signal
        while (hgi2c_v1_get_busy(hw));
        hw->RDMALEN  = 1;
        hw->CMD_DATA = LL_IIC_CMD_DATA_STOP_BIT_EN | LL_IIC_CMD_DATA_WRITE(0x00);
        while (hgi2c_v1_get_busy(hw));
        /* Read the last from hardware */
        *(buf+len_offset) = hw->CMD_DATA;
        
    } else {
        /* make sure the remain (len-1) more than 0 */
        if (len_temp - 1) {
            /* set ACK when receive data */
            hgi2c_v1_set_ack(hw);
            
            hw->RSTADR = (uint32)(buf + len_offset);
            hw->RDMALEN = len_temp - 1;
            hw->CON1 |= LL_IIC_CON1_DMA_EN; /* kick DMA */
            
            /* DMA interrupt to sema_up */
            /* slave use wait forever   */
            os_sema_down(&dev->i2c_done, osWaitForever);
            len_offset += (len_temp - 1);
         }

        /* set NACK when receive the last byte */
        hgi2c_v1_set_nack(hw);
        
        hw->RSTADR = (uint32)(buf + len_offset);
        hw->RDMALEN = 1;
        hw->CON1 |= LL_IIC_CON1_DMA_EN; /* kick DMA */
        
        /* DMA interrupt to sema_up */
        /* slave use wait forever   */
        os_sema_down(&dev->i2c_done, osWaitForever);
        len_offset += 1;
    }

    if (dev->irq_rx_done_en) {
        if (dev->irq_hdl) {
            dev->irq_hdl(I2C_IRQ_FLAG_RX_DONE, dev->irq_data, 0, 0);
        }
    }

err_handle:

    os_mutex_unlock(&dev->i2c_lock);

    /* Indcate IIC module has read done */
    dev->flag_rx_done = 1;

    return RET_OK;
}

static int32 hgi2c_v1_write(struct i2c_device *i2c, int8 *addr_buf, uint32 addr_len, int8 *buf, uint32 len) {

    struct hgi2c_v1 *dev = (struct hgi2c_v1 *)i2c;
    struct hgi2c_v1_hw *hw = (struct hgi2c_v1_hw *)dev->hw;
    uint32 slave_addr      = 0;
    uint32 len_temp        = len;
    uint32 addr_len_temp   = addr_len;
    uint32 len_offset      = 0;
    int32 ret_sema         = 0;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    /* Make sure the read done */
    if (!dev->flag_rx_done || !dev->flag_table_done) {
        return RET_ERR;
    }

    /* make sure that a least one buf is existent */
    if ((NULL == addr_buf) && (NULL == buf)) {
        return RET_ERR;
    }

    /* make sure that a least one len is existent */
    if ((0 == addr_len) && (0 == len)) {
        return RET_ERR;
    }

    os_mutex_lock(&dev->i2c_lock, osWaitForever);

    /* Indcate IIC module is writing */
    dev->flag_tx_done = 0;

    /* set IIC to send status */
    hgi2c_v1_set_dir_tx(hw);


    /* IIC master mode */
    if (!hgi2c_v1_is_slave_mode(hw)) {
        /* send the iic slave address configured by "open" function */
        slave_addr = hgi2c_v1_get_slave_address(hw);
        
        if(hgi2c_v1_get_slave_address_bitwidth(hw) == IIC_ADDR_10BIT) {
            hw->CMD_DATA = LL_IIC_CMD_DATA_START_BIT_EN | LL_IIC_CMD_DATA_WRITE(slave_addr >> 8 << 1) | 0xf0;
            /* It doesn't stop here when mode is IIC master */
            while (hgi2c_v1_get_busy(hw));
            hw->CMD_DATA = LL_IIC_CMD_DATA_WRITE(slave_addr);
        } else {
            hw->CMD_DATA = LL_IIC_CMD_DATA_START_BIT_EN | LL_IIC_CMD_DATA_WRITE(slave_addr << 1);
        }

        /* It doesn't stop here when mode is IIC master */
        while (hgi2c_v1_get_busy(hw));

        /* send the addr, may be register address */
        while(addr_len_temp > 0) {
            hw->CMD_DATA = LL_IIC_CMD_DATA_WRITE(*addr_buf++);
            addr_len_temp--;
            /* It doesn't stop here when mode is IIC master */
            while (hgi2c_v1_get_busy(hw));
        }
    }

    /* data length more than 4095 byte(limited by hardware) */
    len_offset = 0;
    if (len_temp >= 4095) {
        do {
            hw->TSTADR = (uint32)(buf + len_offset);
            hw->TDMALEN = 4095;
            hw->CON1 |= LL_IIC_CON1_DMA_EN; /* kick DMA */

            /* DMA interrupt to sema_up  */
            /* master use 6s for timeout */
            /* slave use wait forever    */
            if (!hgi2c_v1_is_slave_mode(hw)) {
                ret_sema = os_sema_down(&dev->i2c_done, 6*1000);
                if (!ret_sema) {
                    _os_printf("***IIC modoule info: M send err, 1 phase timeout!\r\n");
                    goto err_handle;
                }
            } else {
                os_sema_down(&dev->i2c_done, osWaitForever);
            }
            
            len_offset += 4095;
        }while ((len_temp -= 4095) >= 4095);
    }

    /* IIC master mode need to send the stop signal in last byte */
    if (!hgi2c_v1_is_slave_mode(hw)) {
        
        if(len_temp == 0) {
            goto err_handle;
        }

        /* make sure the remain (len-1) more than 0 */
        if (len_temp - 1) {
            hw->TSTADR = (uint32)(buf + len_offset);
            hw->TDMALEN = len_temp - 1;
            hw->CON1 |= LL_IIC_CON1_DMA_EN; /* kick DMA */

            /* DMA interrupt to sema_up  */
            /* master use 6s for timeout */
            ret_sema = os_sema_down(&dev->i2c_done, 6*1000);
            if (!ret_sema) {
                _os_printf("***IIC modoule info: M send err, 2 phase timeout!\r\n");
                goto err_handle;
            }

            len_offset += (len_temp - 1);
        }

        //send the last byte with the stop signal
        while (hgi2c_v1_get_busy(hw));
        hw->CMD_DATA = LL_IIC_CMD_DATA_STOP_BIT_EN | LL_IIC_CMD_DATA_WRITE(*(buf+len_offset));
        while (hgi2c_v1_get_busy(hw));
        
    } else {
        /* make sure the remain (len-1) more than 0 */
        if (len_temp) {
            hw->TSTADR = (uint32)(buf + len_offset);
            hw->TDMALEN = len_temp;
            hw->CON1 |= LL_IIC_CON1_DMA_EN; /* kick DMA */
            
            /* DMA interrupt to sema_up */
            /* slave use wait forever   */
            os_sema_down(&dev->i2c_done, osWaitForever);
            len_offset += (len_temp);
         }
    }

    if (dev->irq_tx_done_en) {
        if (dev->irq_hdl) {
            dev->irq_hdl(I2C_IRQ_FLAG_TX_DONE, dev->irq_data, 0, 0);
        }
    }

err_handle:

    os_mutex_unlock(&dev->i2c_lock);

    /* Indcate IIC module has written done */
    dev->flag_tx_done = 1;

    return RET_OK;

}

static int32 hgi2c_v1_write_table(struct i2c_device *i2c, uint8 cmd_len, uint8 slave_id, scatter_data *data)
{
    struct hgi2c_v1 *dev = (struct hgi2c_v1 *)i2c;
    struct hgi2c_v1_hw *hw = (struct hgi2c_v1_hw *)dev->hw; 
    int32 ret_sema         = 0;
    
    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    /* Make sure the read done */
    if (!dev->flag_rx_done || !dev->flag_tx_done || !dev->flag_table_config_en || !dev->flag_table_done) {
        if (!dev->flag_table_config_en)     os_printf("unenable iic write_table_mode!!!");
        if (!dev->flag_table_done)          os_printf("iic write table status busy ");
        return RET_ERR;
    }

    if ((cmd_len == 0) || (slave_id == 0)) {
        return RET_ERR;
    }
    
    if ((data->size == 0) || (data->addr == NULL)) {
        return RET_ERR;
    }

//    if (pin_func(i2c->dev.dev_id, 1) != RET_OK) {
//        return RET_ERR;
//    }
    
    os_mutex_lock(&dev->i2c_lock, osWaitForever);

    dev->flag_table_done = 0;

    hgi2c_v1_set_dir_tx(hw);
    hgi2c_v1_set_write_table_cmd_len(hw, cmd_len);
    hgi2c_v1_set_slave_addr(hw, (slave_id << 1));


    hw->TSTADR  = (uint32)data->addr;
    hw->TDMALEN = data->size;
    hw->CON1 |= LL_IIC_CON1_DMA_EN; /* kick DMA */

    ret_sema = os_sema_down(&dev->i2c_done, 6*1000);
    if (!ret_sema) {
        _os_printf("***IIC modoule info: M send err, write_talbe timeout!\r\n");
        goto err_handle;
    }

err_handle:

    os_mutex_unlock(&dev->i2c_lock);

    dev->flag_table_done = 1;
    return RET_OK;
}

#ifdef CONFIG_SLEEP
static int32 hgi2c_v1_suspend(struct dev_obj *obj)
{
    struct hgi2c_v1 *dev = (struct hgi2c_v1 *)obj;
    struct hgi2c_v1_hw *hw = (struct hgi2c_v1_hw *)dev->hw;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_OK;
    }

    if (0 > os_mutex_lock(&dev->bp_suspend_lock, 10000)) {
        return RET_ERR;
    }


    pin_func(dev->dev.dev.dev_id, 0);

    /*!
     * Close the IIC 
     */
    hw->CON1 &= ~ BIT(0);


    /*
     * close irq
     */
    irq_disable(dev->irq_num);

    /*
     * clear pending  
     */
    hw->STA1    = 0xFFFFFFFF;
    hw->STA2    = 0xFFFFFFFF;

    os_memset((void *)&dev->bp_regs, 0, sizeof(dev->bp_regs));

    /* 
     * save the reglist
     */
	dev->bp_regs.con0		  = hw->CON0;	
	dev->bp_regs.con1		  = hw->CON1;
	dev->bp_regs.timecon	  = hw->TIMECON;
	dev->bp_regs.tdmalen	  = hw->TDMALEN;
	dev->bp_regs.rdmalen	  = hw->RDMALEN;
	dev->bp_regs.tstadr 	  = hw->TSTADR;
	dev->bp_regs.rstadr 	  = hw->RSTADR;
	dev->bp_regs.slavesta	  = hw->SLAVESTA;
	dev->bp_regs.ownadrcon	  = hw->OWNADRCON;
	dev->bp_regs.timeoutcon   = hw->TIMEOUTCON;
	dev->bp_regs.rxtimeoutcon = hw->RXTIMEOUTCON;
	dev->bp_regs.rstadr1	  = hw->RSTADR1;
	dev->bp_regs.vsync_tcon   = hw->VSYNC_TCON;
	dev->bp_regs.hsync_tcon   = hw->HSYNC_TCON;


    /*
     * save the irq_hdl created by user
     */
    dev->bp_irq_hdl  = dev->irq_hdl;
    dev->bp_irq_data = dev->irq_data;

    /*
     * close IIC clk
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

static int32 hgi2c_v1_resume(struct dev_obj *obj)
{
    struct hgi2c_v1 *dev = (struct hgi2c_v1 *)obj;
    struct hgi2c_v1_hw *hw = (struct hgi2c_v1_hw *)dev->hw;

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
     * recovery the IIC clk
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
	hw->CON0		 = dev->bp_regs.con0;	   
	hw->CON1		 = dev->bp_regs.con1;  
	hw->TIMECON 	 = dev->bp_regs.timecon;   
	hw->TDMALEN 	 = dev->bp_regs.tdmalen;   
	hw->RDMALEN 	 = dev->bp_regs.rdmalen;   
	hw->TSTADR		 = dev->bp_regs.tstadr;    
	hw->RSTADR		 = dev->bp_regs.rstadr;    
	hw->SLAVESTA	 = dev->bp_regs.slavesta;  
	hw->OWNADRCON	 = dev->bp_regs.ownadrcon;	   
	hw->TIMEOUTCON	 = dev->bp_regs.timeoutcon;  
	hw->RXTIMEOUTCON = dev->bp_regs.rxtimeoutcon;
	hw->RSTADR1 	 = dev->bp_regs.rstadr1;
	hw->VSYNC_TCON	 = dev->bp_regs.vsync_tcon;  
	hw->HSYNC_TCON	 = dev->bp_regs.hsync_tcon; 


    /*
     * recovery the irq handle and data
     */ 
    dev->irq_hdl  = dev->bp_irq_hdl;
    dev->irq_data = dev->bp_irq_data;

    os_memset((void *)&dev->bp_regs, 0, sizeof(dev->bp_regs));

    /*!
     * Open the IIC 
     */
    hw->CON1 |= BIT(0);


    /*
     * open irq
     */
    irq_enable(dev->irq_num);

    dev->dsleep = 0;

    os_mutex_unlock(&dev->bp_resume_lock);

    return RET_OK;
}
#endif

static int32 hgi2c_v1_ioctl(struct i2c_device *i2c, int32 cmd, uint32 param)
{
    int32  ret_val = RET_OK;
    struct hgi2c_v1       *dev = (struct hgi2c_v1 *)i2c;
    struct hgi2c_v1_hw *hw  = (struct hgi2c_v1_hw*)( ( (struct hgi2c_v1 *)i2c )->hw );

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    switch (cmd) {
        case (IIC_SET_DEVICE_ADDR):
            ret_val = hgi2c_v1_set_slave_addr(hw, param);
            break;
        case (IIC_FILTERING):
            ret_val = hgi2c_v1_filtering(i2c, param);
            break;
        case (IIC_STRONG_OUTPUT):
            ret_val = hgi2c_v1_strong_output(i2c, param);
            break;
        case (IIC_SET_WRITE_TABLE_MODE) :
            hgi2c_v1_set_write_table_mode(hw);
            dev->flag_table_config_en = 1;
            break;
		case (IIC_GET_TX_STATUS):
			if(dev->flag_table_done == 1){
				ret_val = 1;
			}
			break;
        
        default:
            ret_val = -ENOTSUPP;
            break;
    }
    
    return ret_val;
}

/** 
  * @brief  IIC0 Interrupt handler
  * @param  None.
  * @retval None.
  */
static void hgi2c_v1_irq_handler(void *data)
{
     struct hgi2c_v1 *dev = (struct hgi2c_v1 *)data;
     struct hgi2c_v1_hw *hw = (struct hgi2c_v1_hw *)dev->hw;

    /* DMA Done interrupt */
    if ((hw->CON1 & LL_IIC_CON1_DMA_IE_EN) && (hw->STA1 & LL_IIC_STA1_DMA_PENDING)) {
        /* clear interrupt flag */
        hw->STA1 = LL_IIC_STA1_DMA_PENDING;
        os_sema_up(&dev->i2c_done);
    }

    /* I2C_IRQ_FLAG_RX_NACK */
    if ((hw->CON0 & LL_IIC_CON0_RX_NACK_IE_EN) && (hw->STA2 & LL_IIC_STA2_RX_NACK(1))) {
        /* clear interrupt flag */
        hw->STA2 = LL_IIC_STA2_RX_NACK(1);
        if (dev->irq_hdl) {
            dev->irq_hdl(I2C_IRQ_FLAG_RX_NACK, dev->irq_data, 0, 0);
        }
    }

    /* I2C_IRQ_FLAG_RX_ERROR */
    if ((hw->CON1 & LL_IIC_CON1_BUF_OV_IE_EN) && (hw->STA1 & LL_IIC_STA1_BUF_OV_PENDING)) {
        /* clear interrupt flag */
        hw->STA1 = LL_IIC_STA1_BUF_OV_PENDING;
        if (dev->irq_hdl) {
            dev->irq_hdl(I2C_IRQ_FLAG_RX_ERROR, dev->irq_data, 0, 0);
        }
    }

    /* I2C_IRQ_FLAG_DETECT_STOP */
    if ((hw->CON0 & LL_IIC_CON0_STOP_IE_EN) && (hw->STA2 & LL_IIC_STA2_STOP_PEND(1))) {
        /* clear interrupt flag */
        hw->STA2 = LL_IIC_STA2_STOP_PEND(1);
        if (dev->irq_hdl) {
            dev->irq_hdl(I2C_IRQ_FLAG_DETECT_STOP, dev->irq_data, 0, 0);
        }
    }
}

static int32 hgi2c_v1_request_irq(struct i2c_device *i2c, i2c_irq_hdl irqhdl, uint32 irq_data, uint32 irq_flag)
{
    struct hgi2c_v1       *dev = (struct hgi2c_v1 *)i2c;
    struct hgi2c_v1_hw *hw  = (struct hgi2c_v1_hw *)dev->hw;
    
    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    dev->irq_hdl = irqhdl;
    dev->irq_data = irq_data;

    if (irq_flag & I2C_IRQ_FLAG_TX_DONE) {
        dev->irq_tx_done_en = 1;
    }

    if (irq_flag & I2C_IRQ_FLAG_RX_DONE) {
        dev->irq_rx_done_en = 1;
    }

    if (irq_flag & I2C_IRQ_FLAG_DETECT_STOP) {
        hw->CON0 |= BIT(20);
    }

    if (irq_flag & I2C_IRQ_FLAG_RX_NACK) {
        hw->CON0 |= BIT(22);
    }

    if (irq_flag & I2C_IRQ_FLAG_RX_ERROR) {
        hw->CON1 |= BIT(8);
    }

    if (irq_flag & I2C_IRQ_FLAG_SLAVE_ADDRESSED) {
        hw->CON0 |= BIT(19);
    }	
    
    return RET_OK;
}

static int32 hgi2c_v1_release_irq(struct i2c_device *i2c, uint32 irq_flag) {

    struct hgi2c_v1       *dev = (struct hgi2c_v1 *)i2c;
    struct hgi2c_v1_hw *hw  = (struct hgi2c_v1_hw *)dev->hw;
    
    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    if (irq_flag & I2C_IRQ_FLAG_TX_DONE) {
        dev->irq_tx_done_en = 0;
    }

    if (irq_flag & I2C_IRQ_FLAG_RX_DONE) {
        dev->irq_rx_done_en = 0;
    }

    if (irq_flag & I2C_IRQ_FLAG_DETECT_STOP) {
        hw->CON0 &= ~ BIT(20);
    }

    if (irq_flag & I2C_IRQ_FLAG_RX_NACK) {
        hw->CON0 &= ~ BIT(22);
    }

    if (irq_flag & I2C_IRQ_FLAG_RX_ERROR) {
        hw->CON1 &= ~ BIT(8);
    }

	if (irq_flag & I2C_IRQ_FLAG_SLAVE_ADDRESSED) {
        hw->CON0 &= ~ BIT(19);
    }

    return RET_OK;
}

static const struct i2c_hal_ops i2c_ops = {

    .open           = hgi2c_v1_open,
    .close          = hgi2c_v1_close,
    .ioctl          = hgi2c_v1_ioctl,
    .baudrate       = hgi2c_v1_set_baudrate,
    .request_irq    = hgi2c_v1_request_irq,
    .read           = hgi2c_v1_read,
    .write          = hgi2c_v1_write,
    .write_table    = hgi2c_v1_write_table,
    .release_irq    = hgi2c_v1_release_irq,
#ifdef CONFIG_SLEEP
    .ops.suspend    = hgi2c_v1_suspend,
    .ops.resume     = hgi2c_v1_resume,
#endif
};

int32 hgi2c_v1_attach(uint32 dev_id, struct hgi2c_v1 *i2c)
{

    struct hgi2c_v1 *dev = (struct hgi2c_v1 *)i2c;

    i2c->dev.dev.ops        = (const struct devobj_ops *)&i2c_ops;
    i2c->irq_hdl            = NULL;
    i2c->irq_data           = 0;
    i2c->opened             = 0;
    i2c->dsleep             = 0;
    i2c->flag_rx_done       = 1;
    i2c->flag_tx_done       = 1;
    i2c->irq_rx_done_en     = 0;
    i2c->irq_tx_done_en     = 0;

#ifdef CONFIG_SLEEP
    os_mutex_init(&i2c->bp_suspend_lock);
    os_mutex_init(&i2c->bp_resume_lock);
#endif

    os_mutex_init(&i2c->i2c_lock);
    os_sema_init(&i2c->i2c_done, 0);

    request_irq(dev->irq_num, hgi2c_v1_irq_handler, dev);

    dev_register(dev_id, (struct dev_obj *)i2c);
    return RET_OK;
}


