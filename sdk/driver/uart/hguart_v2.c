/**
 * @file hguart_v2.c
 * @author bxd
 * @brief normal uart
 * @version 
 * TXW80X; TXW81X
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
#include "hal/uart.h"
#include "dev/uart/hguart_v2.h"
#include "hguart_v2_hw.h"

#define UART_FLAG_TDMA_BUSY (0x1)


/**********************************************************************************/
/*                           LOW LAYER FUNCTION                                   */
/**********************************************************************************/

static int32 hguart_v2_switch_hal_uart_parity(enum uart_parity param)
{

    switch (param) {
        case (UART_PARITY_NONE):
            return 0;
            break;
        case (UART_PARITY_ODD):
            return 1;
            break;
        case (UART_PARITY_EVEN):
            return 2;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hguart_v2_switch_hal_uart_stop_bit(enum uart_stop_bit param)
{

    switch (param) {
        case (UART_STOP_BIT_1):
            return 0;
            break;
        case (UART_STOP_BIT_2):
            return 1;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hguart_v2_switch_hal_uart_data_bit(enum uart_data_bit param)
{

    switch (param) {
        case (UART_DATA_BIT_8):
            return 0;
            break;
        case (UART_DATA_BIT_9):
            return 1;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hguart_v2_set_data_bit(struct hguart_v2_hw *p_uart, enum uart_data_bit data_bit)
{

    int32 data_bit_to_reg = 0;

    data_bit_to_reg = hguart_v2_switch_hal_uart_data_bit(data_bit);

    if (((-1) == data_bit_to_reg)) {
        return RET_ERR;
    }

    if (data_bit_to_reg) {
        p_uart->CON |=   LL_UART_CON_BIT9_EN;
    } else {
        p_uart->CON &= ~ LL_UART_CON_BIT9_EN;
    }

    return RET_OK;
}

static int32 hguart_v2_set_parity(struct hguart_v2_hw *p_uart, enum uart_parity parity)
{

    int32 parity_to_reg = 0;

    parity_to_reg = hguart_v2_switch_hal_uart_parity(parity);

    if (((-1) == parity_to_reg)) {
        return RET_ERR;
    }

    switch (parity_to_reg) {
        case (0):
            p_uart->CON &= ~(LL_UART_CON_PARITY_EN | LL_UART_CON_ODD_EN);
            break;
        case (1):
            p_uart->CON |= LL_UART_CON_PARITY_EN | LL_UART_CON_ODD_EN;
            break;
        case (2):
            p_uart->CON = (p_uart->CON & ~(LL_UART_CON_ODD_EN)) | LL_UART_CON_PARITY_EN;
            break;
    }

    return RET_OK;
}

static int32 hguart_v2_set_stop_bit(struct hguart_v2_hw *p_uart, enum uart_stop_bit stop_bit)
{

    int32 stop_bit_to_reg = 0;

    stop_bit_to_reg = hguart_v2_switch_hal_uart_stop_bit(stop_bit);

    if (((-1) == stop_bit_to_reg)) {
        return RET_ERR;
    }

    if (stop_bit_to_reg) {
        p_uart->CON |=   LL_UART_CON_STOP_BIT(1);
    } else {
        p_uart->CON &= ~ LL_UART_CON_STOP_BIT(1);
    }

    return RET_OK;
}

static int32 hguart_v2_set_time_out(struct hguart_v2_hw *p_uart, uint32 time_bit, uint32 enable)
{

    if (enable) {
        p_uart->TOCON = (p_uart->TOCON & ~ LL_UART_TOCON_TO_BIT_LEN(0xFFFF)) | LL_UART_TOCON_TO_BIT_LEN(time_bit) | LL_UART_TOCON_TO_EN;
    } else {
        p_uart->TOCON = 0;
    }


    return RET_OK;
}

static int32 hguart_v2_set_dma(struct hguart_v2 *dev, uint32 enable)
{

    if (enable) {
        dev->use_dma = 1;
    } else {
        dev->use_dma = 0;
    }

    return RET_OK;
}

static int32 hguart_v2_set_baudrate(struct hguart_v2_hw *p_uart, uint32 baudrate)
{

    p_uart->BAUD = (peripheral_clock_get(HG_APB0_PT_UART0) / baudrate - 1);

    return RET_OK;
}

static int32 hguart_v2_set_mode(struct hguart_v2_hw *p_uart, uint32 mode)
{
    switch (mode) {
        case UART_MODE_DUPLEX:
            p_uart->CON &= ~ LL_UART_CON_WORK_MODE(0x3);
            p_uart->CON |=   LL_UART_CON_WORK_MODE(0);
            break;
        case UART_MODE_SIMPLEX_TX:
            p_uart->CON &= ~ LL_UART_CON_WORK_MODE(0x3);
            p_uart->CON |=   LL_UART_CON_WORK_MODE(1);
            break;
        case UART_MODE_SIMPLEX_RX:
            p_uart->CON &= ~ LL_UART_CON_WORK_MODE(0x3);
            p_uart->CON |=   LL_UART_CON_WORK_MODE(2);
            break;
        default:
            return -ENOTSUPP;
    }
    return RET_OK;
}

static void hguart_v2_dma_tx_config(struct hguart_v2_hw *p_uart, uint8 status)
{
    uint32 _dmacon = p_uart->DMACON;

    if (status) {
        _dmacon |= LL_UART_DMACON_TX_DMA_EN;
    } else {
        _dmacon &= ~ BIT(0);
        _dmacon |= BIT(8);
    }

    p_uart->DMACON = _dmacon;
}

static int32 hguart_v2_dma_rx_config(struct hguart_v2_hw *p_uart, uint8 status)
{
    uint32 _dmacon = p_uart->DMACON;

    if (status) {
        _dmacon |= LL_UART_DMACON_RX_DMA_EN;
    } else {
        _dmacon &= ~ BIT(1);
        _dmacon |= BIT(9);
    }

    p_uart->DMACON = _dmacon;

    return 0;
}

// static int32 hguart_v2_rs485_config(struct hguart_v2_hw *p_uart,  uint8 enable,
//                                                uint8 re_sig_active_level,    uint8 de_sig_active_level,
//                                                uint32 de_deassertion_time,   uint32 de_assertion_time,
//                                                uint32 de2re_turnaround_time, uint32 re2de_turnaround_time)
// {
//     struct hguart_v2    *dev = (struct hguart_v2 *)p_uart;
//     struct hguart_v2_hw *hw  = (struct hguart_v2_hw *)dev->hw;

//     if (enable) {
//         hw->RS485_DET = LL_UART_RS485_DET_DE_AT(de_assertion_time)    | LL_UART_RS485_DET_DE_DAT(de_deassertion_time);
//         hw->RS485_TAT = LL_UART_RS485_TAT_DE2RE_T(de2re_turnaround_time)  | LL_UART_RS485_TAT_RE2DE_T(re2de_turnaround_time);
          
//         hw->RS485_CON = LL_UART_RS485_CON_DE_EN | LL_UART_RS485_CON_RE_EN | \
//                           LL_UART_RS485_CON_RS485_MODE(1) |                 \
//                           LL_UART_RS485_CON_RE_POL(re_sig_active_level)|    \
//                           LL_UART_RS485_CON_DE_POL(de_sig_active_level)|    \
//                           LL_UART_RS485_CON_RS485_EN;

//     } else {
//         hw->RS485_CON = 0;
//         hw->RS485_DET = 0;
//         hw->RS485_TAT = 0;
//     }

// //    hw->RS485_DET = LL_UART_RS485_DET_DE_AT(200)    | LL_UART_RS485_DET_DE_DAT(100);
// //    hw->RS485_TAT = LL_UART_RS485_TAT_DE2RE_T(400)  | LL_UART_RS485_TAT_RE2DE_T(300);
// //      
// //    hw->RS485_CON = LL_UART_RS485_CON_DE_EN | LL_UART_RS485_CON_RE_EN | \
// //                      LL_UART_RS485_CON_RS485_MODE(1) |\
// //                      LL_UART_RS485_CON_RE_POL(1)|\
// //                      LL_UART_RS485_CON_DE_POL(0)|\
// //                      LL_UART_RS485_CON_RS485_EN;

//     return RET_OK;
// }

static int32 hguart_v2_rs485det_set(struct hguart_v2_hw *p_uart, uint32 de_deassertion_time, uint32 de_assertion_time)
{
    struct hguart_v2_hw *hw = p_uart;

    hw->RS485_DET = LL_UART_RS485_DET_DE_AT(de_assertion_time) | LL_UART_RS485_DET_DE_DAT(de_deassertion_time);
    
    return RET_OK;
}

static int32 hguart_v2_rs485tat_set(struct hguart_v2_hw *p_uart, uint32 de2re_turnaround_time, uint32 re2de_turnaround_time)
{
    struct hguart_v2_hw *hw = p_uart;

    hw->RS485_TAT = LL_UART_RS485_TAT_DE2RE_T(de2re_turnaround_time) | LL_UART_RS485_TAT_RE2DE_T(re2de_turnaround_time);
    
    return RET_OK;
}

static int32 hguart_v2_rs485dre_pol_set(struct hguart_v2_hw *p_uart, uint8 re_sig_active_level,    uint8 de_sig_active_level)
{
    struct hguart_v2_hw *hw  = p_uart;

    hw->RS485_CON &= ~(LL_UART_RS485_CON_RE_POL(1)|LL_UART_RS485_CON_DE_POL(1));

    hw->RS485_CON |= LL_UART_RS485_CON_RE_POL(re_sig_active_level)|    \
                     LL_UART_RS485_CON_DE_POL(de_sig_active_level);
    
    return RET_OK;
}

static int32 hguart_v2_rs485_en(struct hguart_v2_hw *p_uart, uint8 enable)
{
    struct hguart_v2_hw *hw  = p_uart;

    if (enable) {
        hw->RS485_DET = LL_UART_RS485_DET_DE_AT(200)    | LL_UART_RS485_DET_DE_DAT(100);
        hw->RS485_TAT = LL_UART_RS485_TAT_DE2RE_T(400)  | LL_UART_RS485_TAT_RE2DE_T(300);
        hw->RS485_CON = LL_UART_RS485_CON_DE_EN | LL_UART_RS485_CON_RE_EN | \
                        LL_UART_RS485_CON_RS485_MODE(1) |                   \
                        LL_UART_RS485_CON_RE_POL(1)|    \
                        LL_UART_RS485_CON_DE_POL(0)|    \
                        LL_UART_RS485_CON_RS485_EN;
    } else {
        hw->RS485_CON = 0;
        hw->RS485_DET = 0;
        hw->RS485_TAT = 0;
    }
    return RET_OK;
}

static int32 hguart_v2_hw_control_config(struct hguart_v2_hw *p_uart,  uint8 enable)
{
    struct hguart_v2    *dev = (struct hguart_v2 *)p_uart;
    struct hguart_v2_hw *hw  = (struct hguart_v2_hw *)dev->hw;

    if (enable) {
        hw->CON |= LL_UART_CON_CTS_EN | LL_UART_CON_RTS_EN;
        hw->RS485_CON = 0;
        hw->RS485_DET = 0;
        hw->RS485_TAT = 0;
    } else {
        hw->CON &= ~(LL_UART_CON_CTS_EN | LL_UART_CON_RTS_EN);
        hw->RS485_CON = 0;
        hw->RS485_DET = 0;
        hw->RS485_TAT = 0;
    }

    return RET_OK;
}

/**********************************************************************************/
/*                             ATTCH FUNCTION                                     */
/**********************************************************************************/

static int32 hguart_v2_open(struct uart_device *uart, uint32 baudrate)
{

    struct hguart_v2    *dev = (struct hguart_v2 *)uart;
    struct hguart_v2_hw *hw  = (struct hguart_v2_hw *)dev->hw;

    if (dev->opened) {
        if (!dev->dsleep) {
            return -EBUSY;
        }
    }

    if ((baudrate < 0) || ((peripheral_clock_get(HG_APB0_PT_UART0) / baudrate) > 0x0003ffff)) {
        return RET_ERR;
    }

    /* pin config */
    if (pin_func(dev->dev.dev.dev_id, 1) != RET_OK) {
        return RET_ERR;
    }


    /*
     * open UART clk
     */
    if (UART0_BASE == (uint32)hw) {
        sysctrl_uart0_clk_open();
    } else if (UART1_BASE == (uint32)hw) {
        sysctrl_uart1_clk_open();
    }

    /* clear reg */
    hw->CON       = 0;
    hw->BAUD      = 0;
    hw->STA       = 0xFFFFFFFF;
    hw->DMACON    = 0;
    hw->DMASTA    = 0xFFFFFFFF;
    hw->RSTADR    = 0;
    hw->TSTADR    = 0;
    hw->RDMALEN   = 0;
    hw->TDMALEN   = 0;
    hw->TOCON     = 0;
    hw->RS485_CON = 0;
    hw->RS485_DET = 0;
    hw->RS485_TAT = 0;


    /* reg config */
#ifndef FPGA_SUPPORT
    hw->BAUD = (peripheral_clock_get(HG_APB0_PT_UART0) / baudrate) - 1;
#else
    hw->BAUD = (96000000 / baudrate) - 1;
#endif
    hw->CON  = 0x1;      /* Default config: 8bit + 1 stop_bit + no parity + duplex mode */

    dev->opened = 1;
    dev->dsleep = 0;

    return RET_OK;
}

static int32 hguart_v2_close(struct uart_device *uart)
{

    struct hguart_v2    *dev = (struct hguart_v2 *)uart;
    struct hguart_v2_hw *hw  = (struct hguart_v2_hw *)dev->hw;

    if (!dev->opened) {
        return RET_OK;
    }

    /*
     * close UART clk
     */
    if (UART0_BASE == (uint32)hw) {
        sysctrl_uart0_clk_close();
    } else if (UART1_BASE == (uint32)hw) {
        sysctrl_uart1_clk_close();
    }

    hw->CON &= ~0x1;
    irq_disable(dev->irq_num);
    pin_func(dev->dev.dev.dev_id, 0);

    dev->opened = 0;
    dev->dsleep = 0;

    return RET_OK;
}

static int32 hguart_v2_rs485_de_set(struct hguart_v2_hw *p_uart)
{
    if(PIN_UART0_DE != 255) {
        ll_uart485_de_enable(p_uart);
        return RET_OK;
    }

    return RET_ERR;
}

static int32 hguart_v2_rs485_de_reset(struct hguart_v2_hw *p_uart)
{
    if(PIN_UART0_DE != 255) {
        ll_uart485_de_disable(p_uart);
        return RET_OK;
        // p_uart->RS485_CON &= (~LL_UART_RS485_CON_DE_EN);
    }
    // os_printf("%s:%d\n", __FUNCTION__, __LINE__);
    return RET_ERR;
}

static int32 hguart_v2_rs485_re_set(struct hguart_v2_hw *p_uart)
{
    if(PIN_UART4_RE != 255) {
        ll_uart485_re_enable(p_uart);
        return RET_OK;
    }

    return RET_ERR;
}

static int32 hguart_v2_rs485_re_reset(struct hguart_v2_hw *p_uart)
{
    if(PIN_UART4_RE != 255) {
        ll_uart485_re_disable(p_uart);
        return RET_OK;
    }

    return RET_ERR;
}

static int32 hguart_v2_putc(struct uart_device *uart, int8 value)
{
    uint8 rs485_en_flag = 0;
    struct hguart_v2    *dev = (struct hguart_v2 *)uart;
    struct hguart_v2_hw *hw  = (struct hguart_v2_hw *)dev->hw;

    if (dev->opened && (hw->CON & LL_UART_CON_UART_EN) && (!dev->dsleep)) {
        if (hw->RS485_CON & LL_UART_RS485_CON_RS485_EN) {
            hguart_v2_rs485_en(hw, 0);
            rs485_en_flag = 1;
        }   
        while (!(hw->STA & LL_UART_STA_TX_BUF_EMPTY));
        hw->DATA = value;
        if(rs485_en_flag) {
            rs485_en_flag = 0;
            hguart_v2_rs485_en(hw, 1);
        }
        return RET_OK;
    } else {
        return -EIO;
    }
}

static uint8 hguart_v2_getc(struct uart_device *uart)
{

    struct hguart_v2    *dev = (struct hguart_v2 *)uart;
    struct hguart_v2_hw *hw  = (struct hguart_v2_hw *)dev->hw;

    if (dev->opened && (hw->CON & LL_UART_CON_UART_EN) && (!dev->dsleep)) {
        while (!(hw->STA & LL_UART_STA_RX_BUF_NOT_EMPTY));
    }
    return hw->DATA;
}

static int32 hguart_v2_puts(struct uart_device *uart, uint8 *buf, uint32 len)
{

    struct hguart_v2    *dev = (struct hguart_v2 *)uart;
    struct hguart_v2_hw *hw  = (struct hguart_v2_hw *)dev->hw;
    int32  i = 0;
    uint32_t irq_flag;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    if(hw->RS485_CON & LL_UART_RS485_CON_RS485_EN) {
        hguart_v2_rs485_de_set(hw);
        hguart_v2_rs485_re_set(hw);
    }

    if (((PSRAM_BASE <= (uint32)buf)) && (((uint32)buf) <= PSRAM_END_ADDR)) {
        for (i = 0; i < len; i++) {
            hguart_v2_putc(uart, buf[i]);
        }
    } else {
        if (dev->use_dma) {
            irq_flag = disable_irq();
            /* Clear the former configuration */
            hguart_v2_dma_tx_config(hw, 0);

            /* clear the tx dma done pending before tx kick */
            hw->DMASTA  = BIT(0);

            hw->TSTADR  = (uint32)buf;
            hw->TDMALEN = len;

            hguart_v2_dma_tx_config(hw, 1);
            dev->flag |= UART_FLAG_TDMA_BUSY;
            enable_irq(irq_flag);

        
            //TDMA_IE_EN会造成两种情况
            if ((hw->DMACON & LL_UART_DMACON_TX_DMA_IE_EN) 
                && (!irq_flag)) {
                while(dev->flag & UART_FLAG_TDMA_BUSY);
            } else {
                while (!(hw->DMASTA & LL_UART_DMASTA_TX_DMA_PEND)) {
                    //休眠唤醒后pending位被清零了， 防卡死
                    if(!hw->TDMALEN) {
                        break;
                    }
                }
                irq_flag = disable_irq();
                dev->flag &= ~UART_FLAG_TDMA_BUSY;
                enable_irq(irq_flag);
            }
        } else {
            for (i = 0; i < len; i++) {
                hguart_v2_putc(uart, buf[i]);
            }
        }
    }

    if(hw->RS485_CON & LL_UART_RS485_CON_RS485_EN) {
        hguart_v2_rs485_de_reset(hw);
        hguart_v2_rs485_re_reset(hw);
    }

    return RET_OK;
}

static int32 hguart_v2_gets(struct uart_device *uart, uint8 *buf, uint32 len)
{

    struct hguart_v2    *dev = (struct hguart_v2 *)uart;
    struct hguart_v2_hw *hw  = (struct hguart_v2_hw *)dev->hw;
    int32  i = 0;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }


    if (((__PSRAM_ADDR_START <= (uint32)buf)) && (((uint32)buf) <= __PSRAM_ADDR_END)) {
        for (i = 0; i < len; i++) {
            buf[i] = hguart_v2_getc(uart);
        }

        return i;
    } else {
        if (dev->use_dma) {
            /* Clear the former configuration */
            hguart_v2_dma_rx_config(hw, 0);

            hw->STA     = BIT(1);
            hw->RSTADR  = (uint32)buf;
            hw->RDMALEN = len;

            hguart_v2_dma_rx_config(hw, 1);
            return RET_OK;
        } else {
            for (i = 0; i < len; i++) {
                buf[i] = hguart_v2_getc(uart);
            }

            return i;
        }
    }

}

#ifdef CONFIG_SLEEP
int32 hguart_v2_suspend(struct dev_obj *obj)
{
    struct hguart_v2    *dev = (struct hguart_v2 *)obj;
    struct hguart_v2_hw *hw  = (struct hguart_v2_hw *)dev->hw;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_OK;
    }

 
    uint32_t disable = disable_irq();
    enable_irq(disable);


    while(dev->flag & UART_FLAG_TDMA_BUSY);
    hw->TDMALEN = 0;

    /*!
     * Close the UART
     */
    hw->CON &= ~ BIT(0);


    pin_func(dev->dev.dev.dev_id, 0);


    /*
     * close irq
     */
    irq_disable(dev->irq_num);

    /*
     * clear pending
     */
    hw->DMASTA = 0xFFFFFFFF;
    hw->STA    = 0xFFFFFFFF;

    /*
     * clear the data in the UART fifo
     */
    do {
        hw->DATA;
    } while (hw->STA & LL_UART_STA_RX_CNT(0x7));


    os_memset((void *)&dev->bp_regs, 0, sizeof(dev->bp_regs));

    /*
     * save the reglist
     */
    dev->bp_regs.con       = hw->CON;
    dev->bp_regs.baud      = hw->BAUD;
    dev->bp_regs.tstadr    = hw->TSTADR;
    dev->bp_regs.rstadr    = hw->RSTADR;
    dev->bp_regs.tdmalen   = hw->TDMALEN;
    dev->bp_regs.rdmalen   = hw->RDMALEN;
    dev->bp_regs.dmacon    = hw->DMACON;
    dev->bp_regs.rs485_con = hw->RS485_CON;
    dev->bp_regs.rs485_det = hw->RS485_DET;
    dev->bp_regs.rs485_tat = hw->RS485_TAT;
    dev->bp_regs.tocon     = hw->TOCON;

    /*
     * save the irq_hdl created by user
     */
    dev->bp_irq_hdl  = dev->irq_hdl;
    dev->bp_irq_data = dev->irq_data;

    if (UART0_BASE == (uint32)hw) {
        sysctrl_uart0_clk_close();
    } else if (UART1_BASE == (uint32)hw) {
        sysctrl_uart1_clk_close();
    }

    dev->dsleep = 1;

    //os_mutex_unlock(&dev->bp_suspend_lock);

    return RET_OK;
}

int32 hguart_v2_resume(struct dev_obj *obj)
{
    struct hguart_v2    *dev = (struct hguart_v2 *)obj;
    struct hguart_v2_hw *hw  = (struct hguart_v2_hw *)dev->hw;
    if ((!dev->opened) || (!dev->dsleep)) {
        return RET_OK;
    }

    //if (0 > os_mutex_lock(&dev->bp_resume_lock, 10000)) {
    //    return RET_ERR;
    //}

    /* pin config */
    if (pin_func(dev->dev.dev.dev_id, 1) != RET_OK) {
        return RET_ERR;
    }


    /*
     * recovery the UART clk
     */
    if (UART0_BASE == (uint32)hw) {
        sysctrl_uart0_clk_open();
    } else if (UART1_BASE == (uint32)hw) {
        sysctrl_uart1_clk_open();
    }

    /*
     * recovery the reglist from sram
     */
    hw->CON       = dev->bp_regs.con;
    hw->BAUD      = dev->bp_regs.baud;
    hw->TSTADR    = dev->bp_regs.tstadr;
    hw->RSTADR    = dev->bp_regs.rstadr;
    hw->TDMALEN   = dev->bp_regs.tdmalen;
    hw->RDMALEN   = dev->bp_regs.rdmalen;
    hw->DMACON    = dev->bp_regs.dmacon | BIT(8) | BIT(9);
    hw->RS485_CON = dev->bp_regs.rs485_con;
    hw->RS485_DET = dev->bp_regs.rs485_det;
    hw->RS485_TAT = dev->bp_regs.rs485_tat;
    hw->TOCON     = dev->bp_regs.tocon;

    /*
     * recovery the irq handle and data
     */
    dev->irq_hdl  = dev->bp_irq_hdl;
    dev->irq_data = dev->bp_irq_data;

    os_memset((void *)&dev->bp_regs, 0, sizeof(dev->bp_regs));

    /*!
     * Open the UART
     */
    hw->CON |= BIT(0);

    /*
     * open irq
     */
    irq_enable(dev->irq_num);

    dev->dsleep = 0;

    //os_mutex_unlock(&dev->bp_resume_lock);

    return RET_OK;
}
#else
int32 hguart_v2_suspend(struct dev_obj *obj)
{
    return RET_ERR;
}
int32 hguart_v2_resume(struct dev_obj *obj)
{
    return RET_ERR;
}
#endif

static int32 hguart_v2_ioctl(struct uart_device *uart, enum uart_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2)
{

    int32  ret_val = RET_OK;
    struct hguart_v2    *dev = (struct hguart_v2 *)uart;
    struct hguart_v2_hw *hw  = (struct hguart_v2_hw *)dev->hw;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    switch (ioctl_cmd) {
        case (UART_IOCTL_CMD_SET_BAUDRATE):
            ret_val = hguart_v2_set_baudrate(hw, param1);
            break;
        case (UART_IOCTL_CMD_SET_DATA_BIT):
            ret_val = hguart_v2_set_data_bit(hw, param1);
            break;
        case (UART_IOCTL_CMD_SET_PARITY):
            ret_val = hguart_v2_set_parity(hw, param1);
            break;
        case (UART_IOCTL_CMD_SET_STOP_BIT):
            ret_val = hguart_v2_set_stop_bit(hw, param1);
            break;
        case (UART_IOCTL_CMD_SET_TIME_OUT):
            ret_val = hguart_v2_set_time_out(hw, param1, param2);
            break;
        case (UART_IOCTL_CMD_USE_DMA):
            ret_val = hguart_v2_set_dma(dev, param1);
            break;
        case (UART_IOCTL_CMD_SET_WORK_MODE):
            ret_val = hguart_v2_set_mode(hw, param1);
            break;
        case (UART_IOCTL_CMD_SET_RS485_EN):
            ret_val = hguart_v2_rs485_en(hw, param1);
            break;
        case (UART_IOCTL_CMD_SET_RS485_DET):
            ret_val = hguart_v2_rs485det_set(hw, param1, param2);
            break;
        case (UART_IOCTL_CMD_SET_RS485_TAT):
            ret_val = hguart_v2_rs485tat_set(hw, param1, param2);
            break;
        case (UART_IOCTL_CMD_SET_RS485DRE_POL):
            ret_val = hguart_v2_rs485dre_pol_set(hw, param1, param2);
            break;
        default:
            ret_val = -ENOTSUPP;
            break;
    }

    return ret_val;
}

static void hguart_v2_irq_handler(void *data)
{

    struct hguart_v2    *dev = (struct hguart_v2 *)data;
    struct hguart_v2_hw *hw  = (struct hguart_v2_hw *)dev->hw;
    static uint8  rec_data_len[2]   = {0};

    /*------Time out interrupt-----*/
    if ((hw->TOCON & LL_UART_TOCON_TO_IE_EN) &&
        (hw->STA   & LL_UART_STA_TO_PEND)) {
        hw->STA = LL_UART_STA_TO_PEND;

        if (dev->irq_hdl) {
            rec_data_len[1] = hw->RDMACNT >> 8;
            rec_data_len[0] = hw->RDMACNT;
            dev->irq_hdl(UART_IRQ_FLAG_TIME_OUT, dev->irq_data, hw->RDMACNT, 0);
            if((PIN_UART0_DE != 255) && (PIN_UART0_RE != 255)) {
                uart_puts((struct uart_device *)dev, rec_data_len, sizeof(rec_data_len));
            }
        }
    }

    /*------Frame erro interrupt-----*/
    if ((hw->CON & LL_UART_CON_FERR_IE_EN) &&
        (hw->STA & LL_UART_STA_FERR_PENDING)) {
        hw->STA = LL_UART_STA_FERR_PENDING;

        if (dev->irq_hdl) {
            dev->irq_hdl(UART_IRQ_FLAG_FRAME_ERR, dev->irq_data, 0, 0);
        }
    }

    /*------TX complete interrupt-----*/
    if ((hw->CON & LL_UART_CON_TCIE_EN) &&
        (hw->STA & LL_UART_STA_TC_PENDING)) {
        hw->STA = LL_UART_STA_TC_PENDING;

        if (dev->irq_hdl) {
            dev->irq_hdl(UART_IRQ_FLAG_TX_BYTE, dev->irq_data, 0, 0);
        }
    }

    /*------RX_DMA_INTERRUPT-----*/
    if ((hw->DMACON & LL_UART_DMACON_RX_DMA_IE_EN) &&
        (hw->DMASTA & LL_UART_DMASTA_RX_DMA_PEND)) {
        hw->DMASTA = LL_UART_DMASTA_RX_DMA_PEND;

        if (dev->irq_hdl) {
            dev->irq_hdl(UART_IRQ_FLAG_DMA_RX_DONE, dev->irq_data, hw->RDMACNT, 0);
        }
    }

    /*------TX_DMA_INTERRUPT-----*/
    if ((hw->DMACON & LL_UART_DMACON_TX_DMA_IE_EN) &&
        (hw->DMASTA & LL_UART_DMASTA_TX_DMA_PEND)) {
        hw->DMASTA = LL_UART_DMASTA_TX_DMA_PEND;
        dev->flag &= ~UART_FLAG_TDMA_BUSY;

        if (dev->irq_hdl) {
            dev->irq_hdl(UART_IRQ_FLAG_DMA_TX_DONE, dev->irq_data, hw->TDMACNT, 0);
        }
    }

    /*------RX buf not empty interrupt-----*/
    if ((hw->CON & LL_UART_CON_RXBUF_NEMPTY_IE_EN) &&
        (hw->STA & LL_UART_STA_RX_BUF_NOT_EMPTY)) {

        if (dev->irq_hdl) {
            dev->irq_hdl(UART_IRQ_FLAG_RX_BYTE, dev->irq_data, hw->DATA, 0);
        }
    }
}

static int32 hguart_v2_request_irq(struct uart_device *uart, uart_irq_hdl irq_hdl, uint32 irq_flag, uint32 irq_data)
{

    struct hguart_v2 *dev = (struct hguart_v2 *)uart;
    struct hguart_v2_hw *hw  = (struct hguart_v2_hw *)dev->hw;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    dev->irq_hdl  = irq_hdl;
    dev->irq_data = irq_data;
    request_irq(dev->irq_num, hguart_v2_irq_handler, dev);


    if (irq_flag & UART_IRQ_FLAG_TX_BYTE) {
        hw->CON    |= LL_UART_CON_TCIE_EN;
    }

    if (irq_flag & UART_IRQ_FLAG_TIME_OUT) {
        hw->TOCON  |= LL_UART_TOCON_TO_IE_EN;
    }

    if (irq_flag & UART_IRQ_FLAG_DMA_TX_DONE) {
        hw->DMACON |= LL_UART_DMACON_TX_DMA_IE_EN;
    }

    if (irq_flag & UART_IRQ_FLAG_DMA_RX_DONE) {
        hw->DMACON |= LL_UART_DMACON_RX_DMA_IE_EN;
    }

    if (irq_flag & UART_IRQ_FLAG_FRAME_ERR) {
        hw->CON    |= LL_UART_CON_FERR_IE_EN;
    }

    if (irq_flag & UART_IRQ_FLAG_RX_BYTE) {
        hw->CON    |= LL_UART_CON_RXBUF_NEMPTY_IE_EN;
    }
    irq_enable(dev->irq_num);

    return RET_OK;
}

static int32 hguart_v2_release_irq(struct uart_device *uart, uint32 irq_flag)
{

    struct hguart_v2    *dev = (struct hguart_v2 *)uart;
    struct hguart_v2_hw *hw  = (struct hguart_v2_hw *)dev->hw;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    if (irq_flag & UART_IRQ_FLAG_TX_BYTE) {
        hw->CON    &= ~ LL_UART_CON_TCIE_EN;
    }

    if (irq_flag & UART_IRQ_FLAG_TIME_OUT) {
        hw->TOCON  &= ~ LL_UART_TOCON_TO_IE_EN;
    }

    if (irq_flag & UART_IRQ_FLAG_DMA_TX_DONE) {
        hw->DMACON &= ~ LL_UART_DMACON_TX_DMA_IE_EN;
    }

    if (irq_flag & UART_IRQ_FLAG_DMA_RX_DONE) {
        hw->DMACON &= ~ LL_UART_DMACON_RX_DMA_IE_EN;
    }

    if (irq_flag & UART_IRQ_FLAG_FRAME_ERR) {
        hw->CON    &= ~ LL_UART_CON_FERR_IE_EN;
    }

    if (irq_flag & UART_IRQ_FLAG_RX_BYTE) {
        hw->CON    &= ~ LL_UART_CON_RXBUF_NEMPTY_IE_EN;
    }

    return RET_OK;
}


static const struct uart_hal_ops uart_v2_ops = {
    .open        = hguart_v2_open,
    .close       = hguart_v2_close,
    .putc        = hguart_v2_putc,
    .getc        = hguart_v2_getc,
    .puts        = hguart_v2_puts,
    .gets        = hguart_v2_gets,
    .ioctl       = hguart_v2_ioctl,
    .request_irq = hguart_v2_request_irq,
    .release_irq = hguart_v2_release_irq,
#ifdef CONFIG_SLEEP
    .ops.suspend = NULL,//hguart_v2_suspend,
    .ops.resume  = NULL,//hguart_v2_resume,
#endif
};

int32 hguart_v2_attach(uint32 dev_id, struct hguart_v2 *uart)
{
    uart->opened          = 0;
    uart->dsleep          = 0;
    uart->use_dma         = 0;
    uart->irq_hdl         = NULL;
    uart->irq_data        = 0;
	uart->flag  = 0;
    uart->dev.dev.ops     = (const struct devobj_ops *)&uart_v2_ops;
#ifdef CONFIG_SLEEP
	
	
#endif
    irq_disable(uart->irq_num);
    dev_register(dev_id, (struct dev_obj *)uart);
    return RET_OK;
}

