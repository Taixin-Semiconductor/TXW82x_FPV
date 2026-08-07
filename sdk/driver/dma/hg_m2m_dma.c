#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "osal/irq.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/string.h"
#include "hal/dma.h"
#include "dev/dma/hg_m2m_dma.h"
#include "osal/sleep.h"

#define DMA_LEN_THRESHOLD       (2048)
uint32_t in_disable_irq(void);

static void hg_m2m0_dma_irq_handler(void *data)
{
    struct mem_dma_dev *dma = (struct mem_dma_dev *)data;
    struct mem_dma_hw  *hw  = (struct mem_dma_hw  *)dma->hw;

    hw->dma_ch[0].DMA_SAIE = 1;
    os_sema_up(&dma->done[0]);
}

static void hg_m2m1_dma_irq_handler(void *data)
{
    struct mem_dma_dev *dma = (struct mem_dma_dev *)data;
    struct mem_dma_hw  *hw  = (struct mem_dma_hw  *)dma->hw;

    hw->dma_ch[1].DMA_SAIE = 1;
    os_sema_up(&dma->done[1]);
}

#ifdef TXW82X
static void hg_m2m2_dma_irq_handler(void *data)
{
    struct mem_dma_dev *dma = (struct mem_dma_dev *)data;
    struct mem_dma_hw  *hw  = (struct mem_dma_hw  *)dma->hw;

    hw->dma_ch[2].DMA_SAIE = 1;
    os_sema_up(&dma->done[2]);
}
#endif

static inline int32 hg_m2m_dma_get_free_ch(struct mem_dma_dev *dev, uint8 ch_fix)
{
    int32  ch = (ch_fix >= HG_M2M_DMA_NUM) ? (HG_M2M_DMA_NUM-1) : ch_fix;
    uint32 flags = disable_irq();
    for (; ch>=0; ) {
        if (!(dev->busy_flag & BIT(ch))){
            dev->busy_flag |= BIT(ch);
            break;
        }
        if (ch_fix < HG_M2M_DMA_NUM) {
            ch = -1;
            break;
        } else {
            ch--;
        }
    }
    
    enable_irq(flags);
    return ch;
}

static inline void hg_m2m_dma_free_ch(struct mem_dma_dev *dev, int32 ch)
{    
    uint32 flags = disable_irq();
    dev->busy_flag &= ~ BIT(ch);
    enable_irq(flags);
}

#define is_memset_same_val(n)   ((((n) >> 0) & 0xff) == (((n) >> 8) & 0xff))
static void hg_soft_memset(uint32 *dest, uint32 val, uint32 size)
{
    uint32 i         = 0;
    uint8  head_size = (uint32)dest % 4;
    uint8  tail_size = ((uint32)dest+size) % 4;
    uint32 c_size    = ALIGN(size, 4) + ALIGN(head_size, 4);
    uint32 *p_dst    = (uint32 *)((uint32)dest - head_size);
    uint8  *p_dst_bt = (uint8  *)dest;
    if (is_memset_same_val(val))
    {
        os_memset((void*)dest, (val & 0xff), size);
    } else {
        for (; i < c_size>>2; i++)
            *p_dst++ = val;
        for (i = head_size; i < (head_size << 1); i++)
            *p_dst_bt++ = val>>((i % 4)<<3);
        p_dst_bt = (uint8 *)((uint32)dest + size - tail_size);
        for (i = tail_size; i < (tail_size << 1); i++)
            *p_dst_bt++ = val>>((i % 4)<<3);
    }
}

const uint8 dma_element_size[4] = {1,2,4,8};
static int32 hg_m2m_dma_xfer(struct dma_device *dma, struct dma_xfer_data *data)
{
    int32  ch    = -1;
    uint32 val   = (data->src_addr_mode == DMA_XFER_MODE_RECYCLE) ? (*((uint32 *)data->src)) : (0);
    uint32 count = data->element_num * dma_element_size[data->element_per_width];
    static uint8 ch0_lock = 0;
    uint32 retry;
    uint32 addr_offset = 0;
    uint32 dma_cnt     = 0;
    int32  ret         = 0;
    
    struct mem_dma_dev *dev = (struct mem_dma_dev *)dma;
    struct mem_dma_hw *hw   = (struct mem_dma_hw *)dev->hw;

    retry = (__in_interrupt() || in_disable_irq()) ? 1 : (count >> (8+(2*dev->dma1_status)));
    if (!retry) retry = 1;
    
    if (data->element_per_width >= DMA_SLAVE_BUSWIDTH_UNDEFINED) {
        return -EBUSY;
    }
    
    /* get free channel */
    for ( ; (!dev->suspend) && (retry--); ) {
        if (data->dir != DMA_XFER_DIR_M2M) {
            uint32 flags = disable_irq();
            ch = hg_m2m_dma_get_free_ch(dev, 0);
            if (0 == ch)
                ch0_lock = 1;
            enable_irq(flags);
        } else {
           ch = hg_m2m_dma_get_free_ch(dev, ch0_lock ? (HG_M2M_DMA_NUM - 1) : HG_M2M_DMA_NUM);
        }
        if (ch >= 0) {
            break;
        }
    } 

    if (ch < 0) {
        if (data->src_addr_mode == DMA_XFER_MODE_RECYCLE) {
            hg_soft_memset((void*)data->dest, val, count);
        } else if(data->src_addr_mode == DMA_XFER_MODE_INCREASE){
            os_memcpy((void*)data->dest, (void*)data->src, count);
        }
        return ch;
    }

#ifdef TXW81X
    uint32 dst_addr = data->dest>>24;
    if (dst_addr == 0x38 || dst_addr == 0x08) {
        while(ll_sysctrl_dma2ahb_is_busy((ch) ? (DMA2AHB_BURST_CH_M2M1_WR) : (DMA2AHB_BURST_CH_M2M0_WR)));
    }
#endif
        
    //sysctrl_m2m_dma_reset();
#if defined(TXW81X)
    hw->dma_ch[ch].DMA_CON  &= HG_M2M_DMA_CON_ENDIAN_RES;
    hw->dma_ch[ch].DMA_ISIZE = 0;
#elif defined(TXW82X)
    hw->dma_ch[ch].DMA_CON  = 0x00;
    hw->dma_ch[ch].DMA_CON  |= HG_M2M_DMA_CON_ENDIAN_SET(data->endian);
#else 
    hw->dma_ch[ch].DMA_CON  = 0x00;
#endif
    hw->dma_ch[ch].DMA_DATA = val;
    
    while(count)
    {
        hw->dma_ch[ch].DMA_TADR = (uint32)data->dest + addr_offset;
        hw->dma_ch[ch].DMA_SADR = (uint32)data->src  + addr_offset;
        dma_cnt = (count > (HG_M2M_DMA_MAX_LEN)) ? (HG_M2M_DMA_MAX_LEN) : (count);
        if ((!__in_interrupt()) && (!in_disable_irq()) && (dma_cnt > DMA_LEN_THRESHOLD)) {
            hw->dma_ch[ch].DMA_SAIE = 0x10001;
            os_sema_eat(&dev->done[ch]);
        } else {
            hw->dma_ch[ch].DMA_SAIE = 1;
        }
        hw->dma_ch[ch].DMA_DLEN = dma_cnt - 1;
        count       -= dma_cnt;
        addr_offset += dma_cnt;

        if (data->src_addr_mode == DMA_XFER_MODE_RECYCLE) {
            hw->dma_ch[ch].DMA_CON  |= (HG_M2M_DMA_CON_MEMSET | HG_M2M_DMA_CON_DTE);
        } else if(data->src_addr_mode == DMA_XFER_MODE_INCREASE){
            hw->dma_ch[ch].DMA_CON  |= (HG_M2M_DMA_CON_MEMCPY | HG_M2M_DMA_CON_DTE);
        }
    
        if (hw->dma_ch[ch].DMA_SAIE & 0x10000) {
            ret = os_sema_down(&dev->done[ch], 50);
            if (!ret) {
                os_printf(KERN_ERR"hw_dma err: {%08x <--- %08x} len=%d\r\n", hw->dma_ch[ch].DMA_TADR, hw->dma_ch[ch].DMA_SADR, dma_cnt);
            }
        } else {
            while (hw->dma_ch[ch].DMA_CON & HG_M2M_DMA_CON_DTE) {
            }
        }
    }
    hw->dma_ch[ch].DMA_CON  = 0x00;
    hg_m2m_dma_free_ch(dev, ch);
    if(ch && dev->dma1_mutex && !dev->dma1_status)
    {
        dev->dma1_status = true;
        hg_m2m_dma_get_free_ch(dev, ch);
    }
    return ch;
}

static int32 hg_m2m_dma_only_hw_xfer(struct dma_device *dma, struct dma_xfer_data *data)
{
    int32  ch    = -1;
    uint32 val   = (data->src_addr_mode == DMA_XFER_MODE_RECYCLE) ? (*((uint32 *)data->src)) : (0);
    uint32 count = data->element_num * dma_element_size[data->element_per_width];
    static uint8 ch0_lock = 0;
    uint32 retry;
    uint32 addr_offset = 0;
    uint32 dma_cnt     = 0;
    int32  ret         = 0;
    
    struct mem_dma_dev *dev = (struct mem_dma_dev *)dma;
    struct mem_dma_hw *hw   = (struct mem_dma_hw *)dev->hw;

    retry = (__in_interrupt() || in_disable_irq()) ? 1 : (count >> (8+(2*dev->dma1_status)));
    if (!retry) retry = 1;
    
    if (data->element_per_width >= DMA_SLAVE_BUSWIDTH_UNDEFINED) {
        return -EBUSY;
    }
    
    /* get free channel */
    for ( ; (!dev->suspend) && (retry); ) {
        if (data->dir != DMA_XFER_DIR_M2M) {
            uint32 flags = disable_irq();
            ch = hg_m2m_dma_get_free_ch(dev, 0);
            if (0 == ch)
                ch0_lock = 1;
            enable_irq(flags);
        } else {
           ch = hg_m2m_dma_get_free_ch(dev, ch0_lock ? (HG_M2M_DMA_NUM - 1) : HG_M2M_DMA_NUM);
        }
        if (ch >= 0) {
            break;
        }
        os_sleep_ms(1);
    } 

#ifdef TXW81X
    uint32 dst_addr = data->dest>>24;
    if (dst_addr == 0x38 || dst_addr == 0x08) {
        while(ll_sysctrl_dma2ahb_is_busy((ch) ? (DMA2AHB_BURST_CH_M2M1_WR) : (DMA2AHB_BURST_CH_M2M0_WR)));
    }
#endif
        
    //sysctrl_m2m_dma_reset();
#if defined(TXW81X)
    hw->dma_ch[ch].DMA_CON  &= HG_M2M_DMA_CON_ENDIAN_RES;
    hw->dma_ch[ch].DMA_ISIZE = 0;
#elif defined(TXW82X)
    hw->dma_ch[ch].DMA_CON  = 0x00;
    hw->dma_ch[ch].DMA_CON  |= HG_M2M_DMA_CON_ENDIAN_SET(data->endian);
#else 
    hw->dma_ch[ch].DMA_CON  = 0x00;
#endif
    hw->dma_ch[ch].DMA_DATA = val;
    
    while(count)
    {
        hw->dma_ch[ch].DMA_TADR = (uint32)data->dest + addr_offset;
        hw->dma_ch[ch].DMA_SADR = (uint32)data->src  + addr_offset;
        dma_cnt = (count > (HG_M2M_DMA_MAX_LEN)) ? (HG_M2M_DMA_MAX_LEN) : (count);
        if ((!__in_interrupt()) && (!in_disable_irq()) && (dma_cnt > DMA_LEN_THRESHOLD)) {
            hw->dma_ch[ch].DMA_SAIE = 0x10001;
            os_sema_eat(&dev->done[ch]);
        } else {
            hw->dma_ch[ch].DMA_SAIE = 1;
        }
        hw->dma_ch[ch].DMA_DLEN = dma_cnt - 1;
        count       -= dma_cnt;
        addr_offset += dma_cnt;

        if (data->src_addr_mode == DMA_XFER_MODE_RECYCLE) {
            hw->dma_ch[ch].DMA_CON  |= (HG_M2M_DMA_CON_MEMSET | HG_M2M_DMA_CON_DTE);
        } else if(data->src_addr_mode == DMA_XFER_MODE_INCREASE){
            hw->dma_ch[ch].DMA_CON  |= (HG_M2M_DMA_CON_MEMCPY | HG_M2M_DMA_CON_DTE);
        }
    
        if (hw->dma_ch[ch].DMA_SAIE & 0x10000) {
            ret = os_sema_down(&dev->done[ch], 50);
            if (!ret) {
                os_printf(KERN_ERR"hw_dma err: {%08x <--- %08x} len=%d\r\n", hw->dma_ch[ch].DMA_TADR, hw->dma_ch[ch].DMA_SADR, dma_cnt);
            }
        } else {
            while (hw->dma_ch[ch].DMA_CON & HG_M2M_DMA_CON_DTE) {
            }
        }
    }
    hw->dma_ch[ch].DMA_CON  = 0x00;
    hg_m2m_dma_free_ch(dev, ch);
    if(ch && dev->dma1_mutex && !dev->dma1_status)
    {
        dev->dma1_status = true;
        hg_m2m_dma_get_free_ch(dev, ch);
    }
    return ch;
}

static int32 hg_m2m_dma_get_status(struct dma_device *dma, uint32 chn)
{
    struct mem_dma_dev *dev = (struct mem_dma_dev *)dma;
    struct mem_dma_hw *hw  = (struct mem_dma_hw *)dev->hw;
    
    if (hw->dma_ch[chn].DMA_CON & HG_M2M_DMA_CON_DTE) {
        return DMA_IN_PROGRESS;
    } else {
        return DMA_SUCCESS;
    }
}

static int32 hg_m2m_dma_ioctl(struct dma_device *dma, uint32 cmd, int32 param1, int32 param2)
{
    int32 ret_val = RET_OK;
   
    switch (cmd)
    {
#if (defined(TX81X) || defined(TXW82X))
        case DMA_IOCTL_CMD_ENDIAN:{
			struct mem_dma_dev *dev = (struct mem_dma_dev *)dma;
			struct mem_dma_hw  *hw  = (struct mem_dma_hw  *)dev->hw;
            for (int i = 0; i < HG_M2M_DMA_NUM; i++)
                hw->dma_ch[i].DMA_CON = ((hw->dma_ch[i].DMA_CON & (~HG_M2M_DMA_CON_ENDIAN_RES)) | HG_M2M_DMA_CON_ENDIAN_SET(param1));
			break;
		}

        case DMA_IOCTL_CMD_CHECK_DMA1_STATUS:{
            struct mem_dma_dev *dev = (struct mem_dma_dev *)dma;
            ret_val = dev->dma1_status;
            break;
        }

        case DMA_IOCTL_CMD_DMA1_LOCK:{
            struct mem_dma_dev *dev = (struct mem_dma_dev *)dma;
            int32 ch = hg_m2m_dma_get_free_ch(dev, 1);
            if(ch)
            {
                dev->dma1_status = true;
            }else{
                hg_m2m_dma_free_ch(dev, ch);
                dev->dma1_status = false;
            }
            dev->dma1_mutex = true;
            break;
        };

        case DMA_IOCTL_CMD_DMA1_UNLOCK:{
            struct mem_dma_dev *dev = (struct mem_dma_dev *)dma;
            hg_m2m_dma_free_ch(dev, 1);
            dev->dma1_mutex  = false;
            dev->dma1_status = false;
            break;
        };
#endif
        
        default:
            ret_val = -ENOTSUPP;
            break;
    }
    return ret_val;
}

#ifdef CONFIG_SLEEP
int32 hg_m2m_dma_suspend(struct dev_obj *dev)
{
    struct mem_dma_dev *dma = (struct mem_dma_dev *)dev;

    if (dma->suspend) {
        return -ENOTSUP;
    }

    /* force all dma busy */
    dma->suspend = 1;

    for (int i = 0; i < HG_M2M_DMA_NUM; i++)
        while (0 != hg_m2m_dma_get_free_ch(dma, i)) { os_sleep_ms(1); }
     
    return RET_OK;
}

int32 hg_m2m_dma_resume(struct dev_obj *dev)
{
    struct mem_dma_dev *dma = (struct mem_dma_dev *)dev;
    
    if (!dma->suspend) {
        return -ENOTSUP;
    }
    hg_m2m_dma_free_ch(dma, 0);
    hg_m2m_dma_free_ch(dma, 1); 
    irq_enable(dma->irq_num);
    irq_enable(dma->irq_num+1);
    dma->suspend = 0;

    return RET_OK;
}
#endif


static const struct dma_hal_ops m2m_ops = {
    .xfer                 = hg_m2m_dma_xfer,
    .only_hw_xfer         = hg_m2m_dma_only_hw_xfer,
    .get_status           = hg_m2m_dma_get_status,
    .ioctl                = hg_m2m_dma_ioctl,
#ifdef CONFIG_SLEEP
    .ops.suspend = hg_m2m_dma_suspend,
    .ops.resume  = hg_m2m_dma_resume,
#endif    
};

__init int32 hg_m2m_dma_dev_attach(uint32 dev_id, struct mem_dma_dev *p_dma)
{
#ifdef TXW82X
    void *irq_handler[] = {hg_m2m0_dma_irq_handler, hg_m2m1_dma_irq_handler, hg_m2m2_dma_irq_handler};
#else
    void *irq_handler[] = {hg_m2m0_dma_irq_handler, hg_m2m1_dma_irq_handler};
#endif
    p_dma->dev.dev.ops = (const struct devobj_ops *)&m2m_ops;
    p_dma->busy_flag = 0;
    p_dma->suspend = 0;
    
    for (uint8 i = 0; i < HG_M2M_DMA_NUM; i++)
    {
        os_sema_init(&p_dma->done[i], 0);
        p_dma->hw->dma_ch[i].DMA_CON  = 0x00;
        p_dma->hw->dma_ch[i].DMA_SAIE = HG_M2M_DMA_SAIE_TCP_PENDING;
        irq_enable(p_dma->irq_num[i]);
        request_irq(p_dma->irq_num[i], irq_handler[i], p_dma);
    }
    
    dev_register(dev_id, (struct dev_obj *)p_dma);
    return RET_OK;
}

