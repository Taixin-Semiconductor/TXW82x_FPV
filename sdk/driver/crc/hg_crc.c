/**
 * @file hg_crc.c
 * @author LeonLeeV
 * @brief 
 * @version 
 * TXW80X; TXW81X; TXW82X
 * @date 2023-08-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "dev/crc/hg_crc.h"
#include "hg_crc_hw.h"

struct hgcrc_config {
    uint32 poly;
    uint32 poly_bits;
    uint32 init_val;
    uint32 xor_out;
    char   ref_in;
    char   ref_out;
};

static const struct hgcrc_config tcpip_chksum = {
    .init_val = 0x0,
    .xor_out = 0x0,
    .poly = 0x0,
    .poly_bits = 0,
    .ref_in = 1,
    .ref_out = 1,
};
static const struct hgcrc_config crc5_usb = {
    .init_val = 0x1f,
    .xor_out = 0x1f,
    .poly = 0x14,
    .poly_bits = 5,
    .ref_in = 1,
    .ref_out = 1,
};
static const struct hgcrc_config crc7_mmc = {
    .init_val = 0x0,
    .xor_out = 0x0,
    .poly = 0x48,
    .poly_bits = 7,
    .ref_in = 0,
    .ref_out = 0,
};
static const struct hgcrc_config crc8_maxim = {
    .init_val = 0x0,
    .xor_out = 0x0,
    .poly = 0x8C,
    .poly_bits = 8,
    .ref_in = 1,
    .ref_out = 1,
};
static const struct hgcrc_config crc8 = {
    .init_val = 0x0,
    .xor_out = 0x0,
    .poly = 0xE0,
    .poly_bits = 8,
    .ref_in = 0,
    .ref_out = 0,
};
static const struct hgcrc_config crc16 = {
    .init_val = 0x0,
    .xor_out = 0x0,
    .poly = 0xA001,
    .poly_bits = 16,
    .ref_in = 1,
    .ref_out = 1,
};
static const struct hgcrc_config crc16_ccitt = {
    .init_val = 0x0,
    .xor_out = 0x0,
    .poly = 0x8408,
    .poly_bits = 16,
    .ref_in = 1,
    .ref_out = 1,
};
static const struct hgcrc_config crc16_modbus = {
    .init_val = 0xFFFF,
    .xor_out = 0x0,
    .poly = 0xA001,
    .poly_bits = 16,
    .ref_in = 1,
    .ref_out = 1,
};
static const struct hgcrc_config crc32_winrar = {
    .init_val = 0xFFFFFFFF,
    .xor_out = 0xFFFFFFFF,
    .poly = 0xEDB88320,
    .poly_bits = 32,
    .ref_in = 1,
    .ref_out = 1,
};

static const struct hgcrc_config *hgcrc_cfg[CRC_TYPE_MAX] = {
    [CRC_TYPE_TCPIP_CHKSUM] = &tcpip_chksum,
    [CRC_TYPE_CRC5_USB] = &crc5_usb,
    [CRC_TYPE_CRC7_MMC] = &crc7_mmc,
    [CRC_TYPE_CRC8_MAXIM] = &crc8_maxim,
    [CRC_TYPE_CRC8] = &crc8,
    [CRC_TYPE_CRC16] = &crc16,
    [CRC_TYPE_CRC16_CCITT] = &crc16_ccitt,
    [CRC_TYPE_CRC16_MODBUS] = &crc16_modbus,
    [CRC_TYPE_CRC32_WINRAR] = &crc32_winrar,
};

static void hg_crc_irq_handler(void *data)
{
    struct hg_crc *crc = (struct hg_crc *)data;
    struct hg_crc_hw *hw = (struct hg_crc_hw *)crc->hw;

    hw->CRC_KST = LL_CRC_KST_DMA_PENDING_CLR;
    os_sema_up(&crc->done);
}

/**
 * CRC len 512KB for TXW81x/TXW82X» 64KB for TXW80x
 */
static int32 hg_crc_calc_continue(struct crc_dev *crc, struct crc_dev_req *req, uint32 *crc_value)
{
    int32 tmo = 2000;
    int32 ret = 0;
    struct hg_crc *dev = (struct hg_crc *)crc;
    struct hg_crc_hw *hw = (struct hg_crc_hw *)dev->hw;

    hw->CRC_INIT = hw->CRC_OUT ^ hw->CRC_INV;
    /* kick */
#if defined(TXW80X)
    hw->DMA_ADDR = (uint32)req->data & 0x00FFFFFF;
#else
    hw->DMA_ADDR = (uint32)req->data;
#endif
    hw->DMA_LEN  = req->len;

    ret = os_sema_down(&dev->done, tmo);
    *crc_value = hw->CRC_OUT;
    return ret > 0 ? RET_OK : RET_ERR;
}

int32 hg_crc5_usb_calc_nonos(struct crc_dev *crc, struct crc_dev_req *req, uint32 *crc_value, uint32 flags)
{
    uint32 cfg_reg;
    
    struct hg_crc    *dev = (struct hg_crc *)crc;
    struct hg_crc_hw *hw  = (struct hg_crc_hw *)dev->hw;
    hw->CRC_CFG &= ~ LL_CRC_CFG_INT_EN;

    cfg_reg = LL_CRC_CFG_POLY_BITS(5) | LL_CRC_CFG_DMAWAIT_CLOCK(5);
    cfg_reg |= LL_CRC_CFG_BIT_ORDER_RIGHT;

    /* config */
    hw->CRC_INIT = 0x1f;
    hw->CRC_INV  = 0x1f;
    hw->CRC_POLY = 0x14;
    hw->CRC_CFG  = cfg_reg;

    /* kick */
#if defined(TXW80X)
    hw->DMA_ADDR = (uint32)req->data & 0x00FFFFFF;
#else
    hw->DMA_ADDR = (uint32)req->data;
#endif
    hw->DMA_LEN  = req->len;

    while(!(hw->CRC_STA & LL_CRC_STA_DMA_PENDING));
    hw->CRC_KST = LL_CRC_KST_DMA_PENDING_CLR;

    *crc_value = hw->CRC_OUT;

    hw->CRC_CFG |= LL_CRC_CFG_INT_EN;

    return 0;
}

int32 hg_crc8_calc_nonos(struct crc_dev *crc, struct crc_dev_req *req, uint32 *crc_value, uint32 flags)
{
    uint32 cfg_reg;
    
    struct hg_crc    *dev = (struct hg_crc *)crc;
    struct hg_crc_hw *hw  = (struct hg_crc_hw *)dev->hw;
    hw->CRC_CFG &= ~ LL_CRC_CFG_INT_EN;

    cfg_reg = LL_CRC_CFG_POLY_BITS(8) | LL_CRC_CFG_DMAWAIT_CLOCK(5);
    cfg_reg |= LL_CRC_CFG_BIT_ORDER_LEFT;

    /* config */
    hw->CRC_INIT = 0x0;
    hw->CRC_INV  = 0x0;
    hw->CRC_POLY = 0xE0;
    hw->CRC_CFG  = cfg_reg;

    /* kick */
#if defined(TXW80X)
    hw->DMA_ADDR = (uint32)req->data & 0x00FFFFFF;
#else
    hw->DMA_ADDR = (uint32)req->data;
#endif
    hw->DMA_LEN  = req->len;

    while(!(hw->CRC_STA & LL_CRC_STA_DMA_PENDING));
    hw->CRC_KST = LL_CRC_KST_DMA_PENDING_CLR;

    *crc_value = hw->CRC_OUT;

    hw->CRC_CFG |= LL_CRC_CFG_INT_EN;

    return 0;
}

/**
 * CRC len 512KB for TXW81x/TXW82X£» 64KB for TXW80x
 */
static int32 hg_crc_calc(struct crc_dev *crc, struct crc_dev_req *req, uint32 *crc_value, uint32 flags)
{
    int32  ret  = 0;
    uint32 cfg_reg = 0;
    struct hg_crc    *dev = (struct hg_crc *)crc;
    struct hg_crc_hw *hw  = (struct hg_crc_hw *)dev->hw;
    const struct hgcrc_config *p_cfg;

    if (req == NULL || req->data == NULL || req->len == 0 || req->type >= CRC_TYPE_MAX) {
        os_printf(KERN_ERR"%s ARG err\r\n", __FUNCTION__);
        return -EINVAL;
    }

    if ((CRC_TYPE_TCPIP_CHKSUM == req->type) && (req->len < 4)) {
        os_printf(KERN_ERR"%s not support\r\n", __FUNCTION__);        
        return -ENOTSUP;
    }

    if ((dev->flags & BIT(HGCRC_FLAGS_SUSPEND))) {
        os_printf(KERN_ERR"%s at suspend\r\n", __FUNCTION__);        
        return -ENOTSUP;
    }
    p_cfg = hgcrc_cfg[req->type];
    if (p_cfg == NULL) {
        os_printf(KERN_ERR"%s not support\r\n", __FUNCTION__);        
        return -ENOTSUP;
    }

    os_mutex_lock(&dev->lock, osWaitForever);
    os_sema_eat(&dev->done);            
    sysctrl_crc_reset();
    if (0 == p_cfg->poly_bits) {
        cfg_reg = LL_CRC_CFG_INT_EN | LL_CRC_CFG_TCP_MODE_EN | LL_CRC_CFG_DMAWAIT_CLOCK(5);
    } else {
        cfg_reg = LL_CRC_CFG_INT_EN | LL_CRC_CFG_POLY_BITS(p_cfg->poly_bits) | LL_CRC_CFG_DMAWAIT_CLOCK(5);
        if (p_cfg->ref_in) {
            cfg_reg |= LL_CRC_CFG_BIT_ORDER_RIGHT;
        } else {
            cfg_reg |= LL_CRC_CFG_BIT_ORDER_LEFT;
        }
    }

    /* config */
    hw->CRC_INV  = p_cfg->xor_out;
	hw->CRC_INIT = p_cfg->init_val;	
    if (flags & CRC_DEV_FLAGS_CONTINUE_CALC) {
        hw->CRC_INIT = p_cfg->xor_out ^ req->crc_last;
    }    
	hw->CRC_POLY = p_cfg->poly;
    hw->CRC_CFG  = cfg_reg;

    /* kick */

#ifdef PSRAM_HEAP
    if (req->flag) {    
        ;
    } else if (CRC_TYPE_TCPIP_CHKSUM == req->type) {
        sys_dcache_clean_range_unaligned((uint32_t *)req->data, req->len);
    } else {
        sys_dcache_clean_range((uint32_t *)req->data, req->len);
    }
#endif

#if defined(TXW80X)
    hw->DMA_ADDR = (uint32)req->data & 0x00FFFFFF;
#else
    hw->DMA_ADDR = (uint32)req->data;
#endif
    hw->DMA_LEN  = req->len;

    ret = os_sema_down(&dev->done, 2000);
    
    if (!ret) {
        os_printf(KERN_ERR"%s timeout\r\n", __FUNCTION__);
    }
    *crc_value = hw->CRC_OUT;
    
    os_mutex_unlock(&dev->lock);
    
    return ret > 0 ? RET_OK : RET_ERR;
}

#ifdef CONFIG_SLEEP
int32 hg_crc_suspend(struct dev_obj *dev)
{
    int32 ret = 0;
    struct hg_crc *crc = (struct hg_crc *)dev;
    struct hg_crc_hw *hw  = (struct hg_crc_hw *)crc->hw;

    if ((crc->flags & BIT(HGCRC_FLAGS_SUSPEND))) {
        return RET_OK;
    }	
    ret = os_mutex_lock(&crc->lock, osWaitForever);
    if (ret < 0) {
        return ret;
    }
    irq_disable(crc->irq_num);
    crc->flags |= BIT(HGCRC_FLAGS_SUSPEND);
    
    crc->regs = (uint32 *)os_malloc(sizeof(struct hg_crc_hw));
    if (NULL == crc->regs) {
        return RET_ERR;
    }
    /* register backup */
    crc->regs[0] = hw->CRC_CFG;
    crc->regs[1] = hw->CRC_INIT;
    crc->regs[2] = hw->CRC_INV;
    crc->regs[3] = hw->CRC_POLY;
    crc->regs[4] = hw->DMA_ADDR;
    
    sysctrl_crc_clk_close();

    return RET_OK;
}

int32 hg_crc_resume(struct dev_obj *dev)
{
    int32 ret = 0;
    struct hg_crc *crc = (struct hg_crc *)dev;
    struct hg_crc_hw *hw  = (struct hg_crc_hw *)crc->hw;
    
    if ((crc->flags & BIT(HGCRC_FLAGS_SUSPEND) && crc->regs)) {
        ret = os_mutex_unlock(&crc->lock);
        if (ret < 0) {
            return ret;
        }
        sysctrl_crc_clk_open();
        /* register recovery */
        hw->CRC_INIT = crc->regs[1];
        hw->CRC_INV  = crc->regs[2];
        hw->CRC_POLY = crc->regs[3];
        hw->DMA_ADDR = crc->regs[4];
        hw->CRC_CFG  = crc->regs[0];
        crc->flags &= ~ BIT(HGCRC_FLAGS_SUSPEND);
        irq_enable(crc->irq_num);
    }

    if (crc->regs) {
        os_free(crc->regs);
    }
    return RET_OK;
}
#endif

static const struct crc_hal_ops crc_ops = {
    .calc        = hg_crc_calc,
#ifdef CONFIG_SLEEP
    .ops.suspend = hg_crc_suspend,
    .ops.resume  = hg_crc_resume,
#endif
};

__init int32 hg_crc_attach(uint32 dev_id, struct hg_crc *crc)
{
    struct hg_crc_hw *hw = (struct hg_crc_hw *)crc->hw;

    crc->dev.dev.ops = (const struct devobj_ops *)&crc_ops;
    crc->flags = 0;

    os_mutex_init(&crc->lock);
    os_sema_init(&crc->done, 0);
    sysctrl_crc_clk_open();
    sysctrl_crc_reset();

    request_irq(crc->irq_num, hg_crc_irq_handler, crc);
    hw->CRC_CFG |= LL_CRC_CFG_INT_EN;
    irq_enable(crc->irq_num);
    dev_register(dev_id, (struct dev_obj *)crc);
    return RET_OK;
}




