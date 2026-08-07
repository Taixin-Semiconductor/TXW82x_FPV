/**
  ******************************************************************************
  * @file    hg_sysaes_v3.c
  * @author  HUGE-IC Application Team
  * @version TXW81X
  * @date    
  * @brief   standard aes support aes 128/192/256
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2019 HUGE-IC</center></h2>
  *
  *
  *
  ******************************************************************************
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
#include "dev/sysaes/hg_sysaes_v3.h"
#include "hg_sysaes_v3_hw.h"
#include "hal/sysaes.h"

#define SYS_AES_DEBUG_ERR_ENABLE           1


#if SYS_AES_DEBUG_ERR_ENABLE
#define SYS_AES_ERR_PRINTF(fmt, args...)               os_printf(fmt, ##args)
#else
#define SYS_AES_ERR_PRINTF(fmt, args...)
#endif



static void hg_sysaes_v3_irq_handler(void *data)
{
    struct hg_sysaes_v3 *sysaes = (struct hg_sysaes_v3 *)data;
    struct hg_sysaes_v3_hw *hw = (struct hg_sysaes_v3_hw *)sysaes->hw;
    hw->AES_STAT = AES_STAT_COMP_PD_MSK;
    os_sema_up(&sysaes->done);
}

static void hg_sysaes_v3_fill_key(struct sysaes_dev *dev, struct sysaes_para *para)
{
    struct hg_sysaes_v3 *sysaes = (struct hg_sysaes_v3 *)dev;
    struct hg_sysaes_v3_hw *hw = (struct hg_sysaes_v3_hw *)sysaes->hw;
    for (uint8 i = 0; i < 8; ++i) {
        hw->KEY[i] = get_unaligned_le32((const void*)&para->key[i*4]);
    }
}

static int32 hg_sysaes_v3_hdl(struct sysaes_dev *dev, struct sysaes_para *para, uint32 flags)
{
    int32 ret;
    uint32 aes_key_len = 0;
    uint32 aes_mode = 0;
    struct hg_sysaes_v3 *sysaes = (struct hg_sysaes_v3 *)dev;
    struct hg_sysaes_v3_hw *hw = (struct hg_sysaes_v3_hw *)sysaes->hw;

    if ((para->key_len > AES_KEY_LEN_BIT_256)) {
        return RET_ERR;
    }
    ret = os_mutex_lock(&sysaes->lock, osWaitForever);
    if (ret) {
        if (flags == ENCRYPT) {
            SYS_AES_ERR_PRINTF("sysaes encrypt lock timeout!\r\n");
        } else {
            SYS_AES_ERR_PRINTF("sysaes decrypt lock timeout!\r\n");
        }
        os_mutex_unlock(&sysaes->lock);
        return RET_ERR;
    }
    os_sema_eat(&sysaes->done);
    if (flags == ENCRYPT) {
        hw->AES_CTRL &= ~AES_CTRL_EOD_MSK;
    } else {
        hw->AES_CTRL |= AES_CTRL_EOD_MSK;
    }
    switch (para->mode) {
        case AES_MODE_ECB:
            aes_mode = AES_CTRL_AES_ECB;
            break;
        case AES_MODE_CBC:
            aes_mode = AES_CTRL_AES_CBC;
            break;
        case AES_MODE_CTR:
            aes_mode = AES_CTRL_AES_CTR;
            break;
        default:
            break;
    }
    hw->AES_CTRL = (hw->AES_CTRL & ~ AES_CTRL_AES_MODE_MSK) | (aes_mode & AES_CTRL_AES_MODE_MSK);
    switch (para->key_len) {
        case AES_KEY_LEN_BIT_128:
            aes_key_len = AES_CTRL_AES_128;
            break;
        case AES_KEY_LEN_BIT_192:
            aes_key_len = AES_CTRL_AES_192;
            break;
        case AES_KEY_LEN_BIT_256:
            aes_key_len = AES_CTRL_AES_256;
            break;
        default:
            break;
    }
    hw->AES_CTRL = (hw->AES_CTRL & ~ AES_CTRL_AES_KEYLEN_MSK) | 
                   (aes_key_len & AES_CTRL_AES_KEYLEN_MSK)    | 
                    AES_CTRL_IRQ_EN_MSK; 

    // READ_ADDR
    sys_dcache_clean_range_unaligned((uint32 *)para->src, para->aes_len);
    // WRITE_ADDR
    sys_dcache_clean_invalid_range((uint32 *)para->dest, para->aes_len);

    hw->SADDR = (uint32)para->src;
    hw->DADDR = (uint32)para->dest;
    hw->BLOCK_NUM = para->aes_len;
    hw->AES_CTRL &= ~AES_CTRL_MOD_MSK;
    for (uint8 i = 0; i < 8; ++i) {
        hw->KEY[i] = get_unaligned_le32((const void*)&para->key[i*4]);
    }
    for (uint8 i = 0; i < 4; ++i) {
        hw->IV[i] = get_unaligned_le32((const void*)&para->iv[i*4]);
    }
    if(para->mode == AES_MODE_CTR) {
        hw->IV[3] = (para->iv[12]) | (para->iv[13] << 8) | (para->iv[15] << 16) | (para->iv[14] << 24); 
    } 
    
    hw->AES_STAT = AES_STAT_COMP_PD_MSK;
    while(ll_sysctrl_dma2ahb_is_busy(DMA2AHB_BURST_CH_SYSAES_WR));
    hw->AES_CTRL |= AES_CTRL_START_MSK;
    ret = os_sema_down(&sysaes->done, 2000);

    if (!ret) {
        sysctrl_sysaes_reset();
        if (flags == ENCRYPT) {
            SYS_AES_ERR_PRINTF("sysaes encrypt wait irq timeout!\r\n");
        } else {
            SYS_AES_ERR_PRINTF("sysaes decrypt wait irq timeout!\r\n");
        }
        os_mutex_unlock(&sysaes->lock);
        return RET_ERR;
    }
    os_mutex_unlock(&sysaes->lock);
    return RET_OK;
}

static int32 hg_sysaes_v3_encrypt(struct sysaes_dev *dev, struct sysaes_para *para)
{
    if (para->mode > AES_MODE_CTR) {
        return RET_ERR;
    }
    //hg_sysaes_v3_fill_key(dev, para);
    return hg_sysaes_v3_hdl(dev, para, ENCRYPT);
}

static int32 hg_sysaes_v3_decrypt(struct sysaes_dev *dev, struct sysaes_para *para)
{
    if (para->mode > AES_MODE_CTR) {
        return RET_ERR;
    }
    //hg_sysaes_v3_fill_key(dev, para);
    if (para->mode == AES_MODE_CTR) {
        return hg_sysaes_v3_hdl(dev, para, ENCRYPT);
    } else {
        return hg_sysaes_v3_hdl(dev, para, DECRYPT);
    }
}

#ifdef CONFIG_SLEEP

int32 hg_sysaes_v3_suspend(struct dev_obj *dev)
{
    int32 ret = 0;
    struct hg_sysaes_v3 *sysaes = (struct hg_sysaes_v3 *)dev;
    struct hg_sysaes_v3_hw *hw = (struct hg_sysaes_v3_hw *)sysaes->hw;
    uint32 *p_hw = (uint32 *)hw;
    
    ret = os_mutex_lock(&sysaes->lock, osWaitForever);
    if (ret < 0)
        return ret;
    irq_disable(sysaes->irq_num);

    /* register backup */
    for (int i=0; i<sizeof(struct hg_sysaes_v3_hw)/sizeof(uint32); i++) {
        sysaes->regs[i] = *p_hw++;
    }
    
    sysctrl_sysaes_clk_close();
    sysaes->flags |= BIT(HG_SYSAES_FLAGS_SUSPEND);
    return RET_OK;
}

int32 hg_sysaes_v3_resume(struct dev_obj *dev)
{
    int32 ret = 0;
    struct hg_sysaes_v3 *sysaes = (struct hg_sysaes_v3 *)dev;
    struct hg_sysaes_v3_hw *hw = (struct hg_sysaes_v3_hw *)sysaes->hw;
    uint32 *p_hw = (uint32 *)hw;
    
    if (sysaes->flags & BIT(HG_SYSAES_FLAGS_SUSPEND)) {
        ret = os_mutex_unlock(&sysaes->lock);
        if (ret < 0)
            return ret;
        
        sysctrl_sysaes_clk_open();
        
        /* register recovery */
        for (int i=0; i<sizeof(struct hg_sysaes_v3_hw)/sizeof(uint32); i++) {
            *p_hw++ = sysaes->regs[i]; 
        }
        
        sysaes->flags &= ~ BIT(HG_SYSAES_FLAGS_SUSPEND);
        irq_enable(sysaes->irq_num);
    }
    return RET_OK;
}
#endif

static const struct aes_hal_ops aes_v3_ops = {
    .encrypt     = hg_sysaes_v3_encrypt,
    .decrypt     = hg_sysaes_v3_decrypt,
#ifdef CONFIG_SLEEP
    .ops.suspend = hg_sysaes_v3_suspend,
    .ops.resume  = hg_sysaes_v3_resume,
#endif
};

__init int32 hg_sysaes_v3_attach(uint32 dev_id, struct hg_sysaes_v3 *sysaes)
{
    sysaes->dev.dev.ops = (const struct devobj_ops *)&aes_v3_ops;
    os_mutex_init(&sysaes->lock);
    os_sema_init(&sysaes->done, 0);
    sysctrl_sysaes_clk_open();
    sysctrl_sysaes_reset();
    request_irq(sysaes->irq_num, hg_sysaes_v3_irq_handler, sysaes);
    irq_enable(sysaes->irq_num);
    dev_register(dev_id, (struct dev_obj *)sysaes);
    return RET_OK;
}


