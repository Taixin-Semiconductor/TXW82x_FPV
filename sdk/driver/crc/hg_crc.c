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

#define HG_CRC_TIMEOUT_MS              (2000)
#define HG_CRC_NONOS_TIMEOUT_LOOPS     (1000000UL)

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

static int32 hg_crc_req_check(struct crc_dev *crc, struct crc_dev_req *req,
                              uint32 *crc_value, uint32 flags)
{
    struct hg_crc *dev = (struct hg_crc *)crc;

    if (!dev || !dev->hw || !req || !crc_value || req->type >= CRC_TYPE_MAX) {
        return -EINVAL;
    }
    if ((flags & ~CRC_DEV_FLAGS_CONTINUE_CALC) ||
        (req->flag & ~CRC_REQ_FLAGS_SKIP_CACHE_SYNC)) {
        return -EINVAL;
    }
    if ((req->len > CRC_DEV_MAX_XFER_SIZE) || (req->len && !req->data)) {
        return -EINVAL;
    }
    if (!hgcrc_cfg[req->type]) {
        return -ENOTSUP;
    }
    if ((CRC_TYPE_TCPIP_CHKSUM == req->type) && req->len && (req->len < 4)) {
        return -ENOTSUP;
    }
    return RET_OK;
}

static uint32 hg_crc_empty_value(struct crc_dev_req *req, uint32 flags)
{
    const struct hgcrc_config *cfg = hgcrc_cfg[req->type];

    return (flags & CRC_DEV_FLAGS_CONTINUE_CALC) ? req->crc_last :
           (cfg->init_val ^ cfg->xor_out);
}

static void hg_crc_cache_sync(struct crc_dev_req *req)
{
    if (req->flag & CRC_REQ_FLAGS_SKIP_CACHE_SYNC) {
        return;
    }
#if defined(TXW82X)
    sys_dcache_clean_range_unaligned((uint32_t *)req->data, req->len);
#elif defined(PSRAM_HEAP)
    sys_dcache_clean_range_unaligned((uint32_t *)req->data, req->len);
#endif
}

static void hg_crc_irq_handler(void *data)
{
    struct hg_crc *crc = (struct hg_crc *)data;
    struct hg_crc_hw *hw = (struct hg_crc_hw *)crc->hw;

    if (hw->CRC_STA & LL_CRC_STA_DMA_PENDING) {
        hw->CRC_KST = LL_CRC_KST_DMA_PENDING_CLR;
        os_sema_up(&crc->done);
    }
}

static void hg_crc_abort(struct hg_crc *dev)
{
    struct hg_crc_hw *hw = (struct hg_crc_hw *)dev->hw;

    irq_disable(dev->irq_num);
    sysctrl_crc_reset();
    hw->CRC_KST = LL_CRC_KST_DMA_PENDING_CLR;
    csi_vic_clear_pending_irq(dev->irq_num);
    os_sema_eat(&dev->done);
    if (!(dev->flags & HGCRC_FLAGS_SUSPEND)) {
        irq_enable(dev->irq_num);
    }
}

static int32 hg_crc_calc_nonos(struct crc_dev *crc, struct crc_dev_req *req,
                               uint32 *crc_value, uint32 flags)
{
    int32 ret;
    uint32 cfg_reg = 0;
    uint32 old_cfg;
    uint32 irq_flags;
    uint32 timeout = HG_CRC_NONOS_TIMEOUT_LOOPS;
    struct hg_crc    *dev = (struct hg_crc *)crc;
    struct hg_crc_hw *hw;
    const struct hgcrc_config *p_cfg;

    ret = hg_crc_req_check(crc, req, crc_value, flags);
    if (ret != RET_OK) {
        return ret;
    }
    if (req->len == 0) {
        *crc_value = hg_crc_empty_value(req, flags);
        return RET_OK;
    }
    hw = (struct hg_crc_hw *)dev->hw;

    irq_flags = disable_irq();
    if (dev->flags & (HGCRC_FLAGS_SUSPEND | HGCRC_FLAGS_HOLD)) {
        enable_irq(irq_flags);
        return -EBUSY;
    }
    dev->flags |= HGCRC_FLAGS_HOLD;
    old_cfg = hw->CRC_CFG;
    p_cfg = hgcrc_cfg[req->type];
    if (p_cfg->poly_bits == 0) {
        cfg_reg = LL_CRC_CFG_TCP_MODE_EN | LL_CRC_CFG_DMAWAIT_CLOCK(5);
    } else {
        cfg_reg = LL_CRC_CFG_POLY_BITS(p_cfg->poly_bits) | LL_CRC_CFG_DMAWAIT_CLOCK(5);
        cfg_reg |= p_cfg->ref_in ? LL_CRC_CFG_BIT_ORDER_RIGHT : LL_CRC_CFG_BIT_ORDER_LEFT;
    }
    hw->CRC_KST = LL_CRC_KST_DMA_PENDING_CLR;
    hw->CRC_INV = p_cfg->xor_out;
    hw->CRC_INIT = (flags & CRC_DEV_FLAGS_CONTINUE_CALC) ?
                   (p_cfg->xor_out ^ req->crc_last) : p_cfg->init_val;
    hw->CRC_POLY = p_cfg->poly;
    hw->CRC_CFG = cfg_reg;
    hg_crc_cache_sync(req);
#if defined(TXW80X)
    hw->DMA_ADDR = (uint32)req->data & 0x00FFFFFF;
#else
    hw->DMA_ADDR = (uint32)req->data;
#endif
    hw->DMA_LEN  = req->len;

    while (timeout-- && !(hw->CRC_STA & LL_CRC_STA_DMA_PENDING));
    if (hw->CRC_STA & LL_CRC_STA_DMA_PENDING) {
        hw->CRC_KST = LL_CRC_KST_DMA_PENDING_CLR;
        *crc_value = hw->CRC_OUT;
        ret = RET_OK;
    } else {
        sysctrl_crc_reset();
        hw->CRC_KST = LL_CRC_KST_DMA_PENDING_CLR;
        ret = -ETIMEDOUT;
    }
    hw->CRC_CFG = old_cfg;
    dev->flags &= ~HGCRC_FLAGS_HOLD;
    enable_irq(irq_flags);
    return ret;
}

int32 hg_crc5_usb_calc_nonos(struct crc_dev *crc, struct crc_dev_req *req,
                             uint32 *crc_value, uint32 flags)
{
    return hg_crc_calc_nonos(crc, req, crc_value, flags);
}

int32 hg_crc8_calc_nonos(struct crc_dev *crc, struct crc_dev_req *req,
                         uint32 *crc_value, uint32 flags)
{
    return hg_crc_calc_nonos(crc, req, crc_value, flags);
}

/**
 * CRC len 512KB for TXW81x/TXW82X£» 64KB for TXW80x
 */
static int32 hg_crc_calc(struct crc_dev *crc, struct crc_dev_req *req, uint32 *crc_value, uint32 flags)
{
    int32  ret;
    uint32 cfg_reg = 0;
    struct hg_crc    *dev = (struct hg_crc *)crc;
    struct hg_crc_hw *hw;
    const struct hgcrc_config *p_cfg;

    ret = hg_crc_req_check(crc, req, crc_value, flags);
    if (ret != RET_OK) {
        os_printf(KERN_ERR"%s ARG err\r\n", __FUNCTION__);
        return ret;
    }
    if (req->len == 0) {
        *crc_value = hg_crc_empty_value(req, flags);
        return RET_OK;
    }

    ret = os_mutex_lock(&dev->lock, osWaitForever);
    if (ret < 0) {
        return ret;
    }
    if (dev->flags & HGCRC_FLAGS_SUSPEND) {
        os_mutex_unlock(&dev->lock);
        return -EBUSY;
    }

    dev->flags |= HGCRC_FLAGS_HOLD;
    hw = (struct hg_crc_hw *)dev->hw;
    p_cfg = hgcrc_cfg[req->type];
    sysctrl_crc_reset();
    hw->CRC_KST = LL_CRC_KST_DMA_PENDING_CLR;
    csi_vic_clear_pending_irq(dev->irq_num);
    os_sema_eat(&dev->done);
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
    hg_crc_cache_sync(req);

#if defined(TXW80X)
    hw->DMA_ADDR = (uint32)req->data & 0x00FFFFFF;
#else
    hw->DMA_ADDR = (uint32)req->data;
#endif
    hw->DMA_LEN  = req->len;

    ret = os_sema_down(&dev->done, HG_CRC_TIMEOUT_MS);
    
    if (ret > 0) {
        *crc_value = hw->CRC_OUT;
        ret = RET_OK;
    } else {
        os_printf(KERN_ERR"%s timeout\r\n", __FUNCTION__);
        hg_crc_abort(dev);
        ret = -ETIMEDOUT;
    }
    dev->flags &= ~HGCRC_FLAGS_HOLD;
    os_mutex_unlock(&dev->lock);
    return ret;
}

void hg_crc_test_printf(struct crc_dev *crc)
{
    uint8_t src[64];
    for (int i = 0;i<64;i++)
        src[i] = 99+i;
    struct crc_dev_req req = {
        .type = CRC_TYPE_CRC32_WINRAR,
        .data = src,
        .len = 64,
    };
    uint32 crc_val = 0;
    hg_crc_calc(crc, &req, &crc_val, 0);
    _os_printf("crc r: 0x%08x\r\n", crc_val);
    if (crc_val != 0x81efdc25) {
        _os_printf("crc lp err\r\n");
    }
}

#ifdef CONFIG_SLEEP
//#define HGCRC_SLEEP_TEST(dev) hg_crc_test_printf(dev)
 #define HGCRC_SLEEP_TEST(dev)
int32 hg_crc_suspend(struct dev_obj *dev)
{
    int32 ret;
    struct hg_crc *crc = (struct hg_crc *)dev;

    HGCRC_SLEEP_TEST(dev);
    ret = os_mutex_lock(&crc->lock, osWaitForever);
    if (ret < 0) {
        return ret;
    }
    if (crc->flags & HGCRC_FLAGS_SUSPEND) {
        os_mutex_unlock(&crc->lock);
        return RET_OK;
    }
    irq_disable(crc->irq_num);
    crc->flags |= HGCRC_FLAGS_SUSPEND;
    sysctrl_crc_clk_close();
    os_mutex_unlock(&crc->lock);
    return RET_OK;
}

int32 hg_crc_resume(struct dev_obj *dev)
{
    int32 ret;
    struct hg_crc *crc = (struct hg_crc *)dev;
    struct hg_crc_hw *hw = (struct hg_crc_hw *)crc->hw;
    
    ret = os_mutex_lock(&crc->lock, osWaitForever);
    if (ret < 0) {
        return ret;
    }
    if (crc->flags & HGCRC_FLAGS_SUSPEND) {
        sysctrl_crc_clk_open();
        sysctrl_crc_reset();
        hw->CRC_KST = LL_CRC_KST_DMA_PENDING_CLR;
        csi_vic_clear_pending_irq(crc->irq_num);
        os_sema_eat(&crc->done);
        hw->CRC_CFG |= LL_CRC_CFG_INT_EN;
        crc->flags &= ~HGCRC_FLAGS_SUSPEND;
        irq_enable(crc->irq_num);
        HGCRC_SLEEP_TEST(dev);
    }
    os_mutex_unlock(&crc->lock);
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
    int32 ret;
    struct hg_crc_hw *hw;

    if (!crc || !crc->hw) {
        return -EINVAL;
    }
    hw = (struct hg_crc_hw *)crc->hw;

    crc->dev.dev.ops = (const struct devobj_ops *)&crc_ops;
    crc->flags = 0;

    ret = os_mutex_init(&crc->lock);
    if (ret != RET_OK) {
        return ret;
    }
    ret = os_sema_init(&crc->done, 0);
    if (ret != RET_OK) {
        os_mutex_del(&crc->lock);
        return ret;
    }
    sysctrl_crc_clk_open();
    sysctrl_crc_reset();

    ret = request_irq(crc->irq_num, hg_crc_irq_handler, crc);
    if (ret != RET_OK) {
        sysctrl_crc_clk_close();
        os_sema_del(&crc->done);
        os_mutex_del(&crc->lock);
        return ret;
    }
    hw->CRC_CFG |= LL_CRC_CFG_INT_EN;
    irq_enable(crc->irq_num);
    ret = dev_register(dev_id, (struct dev_obj *)crc);
    if (ret != RET_OK) {
        irq_disable(crc->irq_num);
        release_irq(crc->irq_num);
        sysctrl_crc_clk_close();
        os_sema_del(&crc->done);
        os_mutex_del(&crc->lock);
    }
    return ret;
}




