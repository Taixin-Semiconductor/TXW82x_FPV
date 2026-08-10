/**
 * @file hgpdm_v0.c
 * @author bxd
 * @brief pdm
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
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/string.h"
#include "hal/pdm.h"
#include "dev/pdm/hgpdm_v0.h"
#include "hgpdm_v0_hw.h"

/**********************************************************************************/
/*                           LOW LAYER FUNCTION                                   */
/**********************************************************************************/

static int32 hgpdm_v0_switch_hal_pdm_sample_freq(enum pdm_sample_freq param) {

    switch (param) {
#if defined(TXW80X) || defined(TXW82X)
        case (PDM_SAMPLE_FREQ_8K):
            return 8000;
            break;
#endif
        case (PDM_SAMPLE_FREQ_16K):
            return 16000;
            break;
        case (PDM_SAMPLE_FREQ_32K):
            return 32000;
            break;
        case (PDM_SAMPLE_FREQ_48K):
            return 48000;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hgpdm_v0_switch_hal_pdm_channel(enum pdm_channel param) {

    switch (param) {
        case (PDM_CHANNEL_LEFT):
            return 0;
            break;
        case (PDM_CHANNEL_RIGHT):
            return 1;
            break;
        case (PDM_CHANNEL_STEREO):
            return 2;
            break;
        default:
            return -1;
            break;
    }
}

static void hgpdm_v0_set_mclk(int32 freq) {

    uint32 temp;
    volatile uint32 div;

    sysctrl_unlock();
    SYSCTRL->SYS_CON2 |= 1<<18; //choose pll0
	
#ifdef TXW80X
#ifndef FPGA_SUPPORT
        div = (peripheral_clock_get(HG_APB0_PT_PDM)/ (freq*64*8));
#else
        float div_f;
        div_f = (96000000.0 / (freq*64*8));
        div = (uint32_t)(div_f+0.5);
#endif
#endif

#ifdef TXW81X
#ifndef FPGA_SUPPORT
        div = (peripheral_clock_get(HG_APB0_PT_PDM)/ (freq*100*4));
#else
        float div_f;
        div_f = (96000000.0 / (freq*64*8));
        div = (uint32_t)(div_f+0.5);
#endif
#endif

#ifdef TXW82X
    div = (peripheral_clock_get(HG_APB0_PT_PDM)/ (freq*100*4));
#endif

    temp  = SYSCTRL->SYS_CON2;
    temp &= ~0x07F00000;
    temp |= (div-1)<<20;            //select div

    SYSCTRL->SYS_CON2 = temp;
    sysctrl_lock();
}

static int32 hgpdm_v0_lr_channel_interchange(struct hgpdm_v0_hw *p_pdm, uint32 enable) {

    if (enable) {
        p_pdm->CON |=   LL_PDM_CON_LRSW(1);
    } else {
        p_pdm->CON &= ~ LL_PDM_CON_LRSW(1);
    }

    return RET_OK;
}

static void hgpdm_v0_enable(struct hgpdm_v0_hw *p_pdm) {

    p_pdm->CON |= LL_PDM_CON_ENABLE(1);
}

static void hgpdm_v0_disable(struct hgpdm_v0_hw *p_pdm) {

    p_pdm->CON &= ~ LL_PDM_CON_ENABLE(1);
}



/**********************************************************************************/
/*                             ATTCH FUNCTION                                     */
/**********************************************************************************/

static int32 hgpdm_v0_open(struct pdm_device *pdm, enum pdm_sample_freq freq, enum pdm_channel channel) {

    struct hgpdm_v0 *dev   = (struct hgpdm_v0 *)pdm;
    struct hgpdm_v0_hw *hw = (struct hgpdm_v0_hw *)dev->hw;
    int32  freq_to_reg     = 0;
    int32  channel_to_reg  = 0;

    if (dev->opened) {
        if (!dev->dsleep) {
            return -EBUSY;
        }
    }

    /* hal enum */
    freq_to_reg    = hgpdm_v0_switch_hal_pdm_sample_freq(freq);
    channel_to_reg = hgpdm_v0_switch_hal_pdm_channel(channel);

    if ((-1) == freq_to_reg ||
        (-1) == channel_to_reg) {
        return -EINVAL;
    }

    /* make sure the clk */
    #if 0
        if (0 != ((peripheral_clock_get(HG_APB0_PT_PDM)) % (freq_to_reg*512))) {
            return RET_ERR;
        }
    #endif

    /* pin config */
    if (pin_func(dev->dev.dev.dev_id , 1) != RET_OK) {
        return RET_ERR;
    }

    /*
     * open PDM clk
     */
    sysctrl_pdm_clk_open();
    /*
     * clear the regs
     */
     hw->CON      = 0;
     hw->DMACON   = 0;
     hw->DMALEN   = 0;
     hw->DMASTADR = 0;
     hw->STA      = 0xFFFF;

    /* reg config */
    hgpdm_v0_set_mclk(freq_to_reg);

    if(freq_to_reg == 8000) {
        hw->CON &= ~LL_PDM_CON_DECIM(3);
        hw->CON |= LL_PDM_CON_DECIM(1);
    }


#ifdef TXW80X
    hw->CON = LL_PDM_CON_WORKMODE(channel_to_reg) | LL_PDM_CON_DECIM(0);
#endif
#ifdef TXW81X
    hw->CON = LL_PDM_CON_WORKMODE(channel_to_reg) | LL_PDM_CON_DECIM(1);
#endif
#ifdef TXW82X
    hw->CON = LL_PDM_CON_WORKMODE(channel_to_reg) | LL_PDM_CON_DECIM(1);
#endif
    hw->STA = 3;

    dev->opened = 1;
    dev->dsleep = 0;
    return RET_OK;
}

static int32 hgpdm_v0_read(struct pdm_device *pdm, void* buf, uint32 len) {

    struct hgpdm_v0 *dev = (struct hgpdm_v0 *)pdm;
    struct hgpdm_v0_hw *hw = (struct hgpdm_v0_hw *)dev->hw;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }
    
    if(!buf || len == 0)
    {
        return RET_ERR;
    }
    
    hw->DMASTADR = (uint32)buf;
    hw->DMALEN   = len;

    hgpdm_v0_enable(hw);

    return RET_OK;
}

static int32 hgpdm_v0_close(struct pdm_device *pdm) {

    struct hgpdm_v0 *dev = (struct hgpdm_v0 *)pdm;
    struct hgpdm_v0_hw *hw = (struct hgpdm_v0_hw *)dev->hw;

    if (!dev->opened) {
        return RET_OK;
    }

    /*
     * close PDM clk
     */
    sysctrl_pdm_clk_close();

    hgpdm_v0_disable(hw            );
    irq_disable(dev->irq_num       );
    pin_func(dev->dev.dev.dev_id, 0);

    dev->opened = 0;
    dev->dsleep = 0;

    return RET_OK;
}

#ifdef CONFIG_SLEEP
static int32 hgpdm_v0_suspend(struct dev_obj *obj)
{
    struct hgpdm_v0 *dev = (struct hgpdm_v0 *)obj;
    struct hgpdm_v0_hw *hw = (struct hgpdm_v0_hw *)dev->hw;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_OK;
    }

    if (0 > os_mutex_lock(&dev->bp_suspend_lock, 10000)) {
        return RET_ERR;
    }


    /*!
     * Close the PDM 
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
    hw->STA = 0xFFFFFFFF;

    os_memset((void *)&dev->bp_regs, 0, sizeof(dev->bp_regs));

    /* 
     * save the reglist
     */
    dev->bp_regs.con       = hw->CON;
    dev->bp_regs.dmastadr  = hw->DMASTADR;
    dev->bp_regs.dmalen    = hw->DMALEN;
    dev->bp_regs.dmacon    = hw->DMACON;

    /*
     * save the irq_hdl created by user
     */
    dev->bp_irq_hdl  = dev->irq_hdl;
    dev->bp_irq_data = dev->irq_data;

    sysctrl_pdm_clk_close();
    
    dev->dsleep = 1;

    os_mutex_unlock(&dev->bp_suspend_lock);

    return RET_OK;
}

static int32 hgpdm_v0_resume(struct dev_obj *obj)
{
    struct hgpdm_v0 *dev = (struct hgpdm_v0 *)obj;
    struct hgpdm_v0_hw *hw = (struct hgpdm_v0_hw *)dev->hw;

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
     * recovery the PDM clk
     */
    sysctrl_pdm_clk_open();


    /*
     * recovery the reglist from sram
     */
    hw->DMASTADR  = dev->bp_regs.dmastadr;
    hw->DMALEN    = dev->bp_regs.dmalen;
    hw->DMACON    = dev->bp_regs.dmacon;
    hw->CON       = dev->bp_regs.con;


    /*
     * recovery the irq handle and data
     */ 
    dev->irq_hdl  = dev->bp_irq_hdl;
    dev->irq_data = dev->bp_irq_data;

    os_memset((void *)&dev->bp_regs, 0, sizeof(dev->bp_regs));

    /*!
     * Open the PDM
     */
    hw->CON |= BIT(0);

    /*
     * open irq
     */
    irq_enable(dev->irq_num);

    dev->dsleep = 0;

    os_mutex_unlock(&dev->bp_resume_lock);

    return RET_OK;
}
#endif

static void hgpdm_v0_irq_handler(void *data) {

    struct hgpdm_v0 *dev = (struct hgpdm_v0 *)data;
    struct hgpdm_v0_hw *hw = (struct hgpdm_v0_hw *)dev->hw;
    if ((hw->DMACON & LL_PDM_DMACON_HF_IE_EN(1)) && 
        (hw->STA     & LL_PDM_STA_HF_PENDING(1))) {
        hw->STA |= LL_PDM_STA_HF_PENDING(1);
        if (dev->irq_hdl) {
            dev->irq_hdl(PDM_IRQ_FLAG_DMA_HF, dev->irq_data);
        }
    }

    if ((hw->DMACON & LL_PDM_DMACON_OV_IE_EN(1)) && 
        (hw->STA     & LL_PDM_STA_OV_PENDING(1))) {
        hw->STA |= LL_PDM_STA_OV_PENDING(1);
        if (dev->irq_hdl) {
            dev->irq_hdl(PDM_IRQ_FLAG_DMA_OV, dev->irq_data);
        }
    }
}

static int32 hgpdm_v0_request_irq(struct pdm_device *pdm, enum pdm_irq_flag irq_flag, pdm_irq_hdl irq_hdl, uint32 data) {

    struct hgpdm_v0 *dev = (struct hgpdm_v0 *)pdm;
    struct hgpdm_v0_hw *hw = (struct hgpdm_v0_hw *)dev->hw;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    dev->irq_hdl  = irq_hdl;
    dev->irq_data = data;

    request_irq(dev->irq_num, hgpdm_v0_irq_handler, dev);

    if (irq_flag & PDM_IRQ_FLAG_DMA_HF) {
        hw->DMACON |= LL_PDM_DMACON_HF_IE_EN(1);
    }

    if (irq_flag & PDM_IRQ_FLAG_DMA_OV) {
        hw->DMACON |= LL_PDM_DMACON_OV_IE_EN(1);
    }

    irq_enable(dev->irq_num);

    return RET_OK;
}

static int32 hgpdm_v0_release_irq(struct pdm_device *pdm, enum pdm_irq_flag irq_flag) {

    struct hgpdm_v0 *dev = (struct hgpdm_v0 *)pdm;
    struct hgpdm_v0_hw *hw = (struct hgpdm_v0_hw *)dev->hw;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    if (irq_flag & PDM_IRQ_FLAG_DMA_HF) {
        hw->DMACON &= ~ LL_PDM_DMACON_HF_IE_EN(1);
    }

    if (irq_flag & PDM_IRQ_FLAG_DMA_OV) {
        hw->DMACON &= ~ LL_PDM_DMACON_OV_IE_EN(1);
    }

    return RET_OK;
}

static int32 hgpdm_v0_ioctl(struct pdm_device *pdm, enum pdm_ioctl_cmd ioctl_cmd, uint32 param) {

    int32 ret_val = RET_OK;
    struct hgpdm_v0 *dev = (struct hgpdm_v0 *)pdm;
    struct hgpdm_v0_hw *hw = (struct hgpdm_v0_hw *)dev->hw;

    if ((!dev->opened) || (dev->dsleep)) {
        return RET_ERR;
    }

    switch (ioctl_cmd) {
        case (PDM_IOCTL_CMD_LR_CHANNEL_INTERCHANGE):
            ret_val = hgpdm_v0_lr_channel_interchange(hw, param);
            break;
        default:
            ret_val = -ENOTSUPP;
            break;
    }

    return ret_val;
}

static const struct pdm_hal_ops pdm_ops = {
    .open        = hgpdm_v0_open,
    .read        = hgpdm_v0_read,
    .close       = hgpdm_v0_close,
    .ioctl       = hgpdm_v0_ioctl,
    .request_irq = hgpdm_v0_request_irq,
    .release_irq = hgpdm_v0_release_irq,
#ifdef CONFIG_SLEEP
    .ops.suspend = hgpdm_v0_suspend,
    .ops.resume  = hgpdm_v0_resume,
#endif
};

int32 hgpdm_v0_attach(uint32 dev_id, struct hgpdm_v0 *pdm) {
    pdm->opened          = 0;
    pdm->dsleep          = 0;
    pdm->irq_hdl         = NULL;
    pdm->irq_data        = 0;
    pdm->dev.dev.ops     =(const struct devobj_ops *)&pdm_ops;
#ifdef CONFIG_SLEEP
    os_mutex_init(&pdm->bp_suspend_lock);
    os_mutex_init(&pdm->bp_resume_lock);
#endif
    irq_disable(pdm->irq_num);
    dev_register(dev_id, (struct dev_obj *)pdm);
    return RET_OK;
}


