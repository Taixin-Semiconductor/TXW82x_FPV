#ifndef _HG_M2M_DMA_H_
#define _HG_M2M_DMA_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TXW82X
#define HG_M2M_DMA_NUM              (CONFI_CORE_M2M_DMA ? 2 : 3)
#define HG_M2M_DMA_MAX_LEN          (1<<20)
#else
#define HG_M2M_DMA_NUM              2
#define HG_M2M_DMA_MAX_LEN          (64<<10)
#endif
    
/** 
  * @brief  M2M_DMA
  */
struct mem_dma_ch {
    __IO uint32 DMA_CON;
    __IO uint32 DMA_SAIE;
    __IO uint32 DMA_DATA;
    __IO uint32 DMA_SADR;
    __IO uint32 DMA_TADR;
    __IO uint32 DMA_DLEN;
#ifdef TXW81X
    __IO uint32 DMA_ISIZE;
#endif
};

struct mem_dma_hw {
    struct mem_dma_ch dma_ch[HG_M2M_DMA_NUM];
};

struct mem_dma_dev {
    struct dma_device  dev;
    struct mem_dma_hw  *hw;
    uint32             irq_num[HG_M2M_DMA_NUM];
    uint32             busy_flag   : 8,
                       dma1_status : 1,
                       dma1_mutex  : 1,
                       suspend    : 1,
                       reserved    : 21;
    struct os_semaphore done[HG_M2M_DMA_NUM];
};


/** @brief M2M DMA register constant table definition
  * @{
  */

/***** DMA_CON *****/
#define HG_M2M_DMA_CON_ENDIAN_SET(n)        ((n) << 3)
#define HG_M2M_DMA_CON_ENDIAN_RES           (3UL << 3)
#define HG_M2M_DMA_CON_BLKCPY               (2UL << 1)
#define HG_M2M_DMA_CON_MEMSET               (1UL << 1)
#define HG_M2M_DMA_CON_MEMCPY               (0UL << 1)
#define HG_M2M_DMA_CON_DTE                  (1UL << 0)

/***** DMA_SAIE *****/
#define HG_M2M_DMA_SAIE_TCIE                (1UL << 16)
#define HG_M2M_DMA_SAIE_TCP_PENDING         (1UL << 0)

/***** DMA_ISIZE *****/
#define HG_M2M_DMA_ISZIE_DST_WIDTH(n)       ((n) << 16)
#define HG_M2M_DMA_ISZIE_SRC_WIDTH(n)       ((n) << 0)

/***** DMA_DLEN *****/
#define HG_M2M_DMA_DLEN_BLK_HEIGHT(n)       ((n) << 20)
#define HG_M2M_DMA_DLEN_BLK_WIDTH(n)        ((n) << 0)

/**
  * @}
  */

int32 hg_m2m_dma_dev_attach(uint32 dev_id, struct mem_dma_dev *p_dma);

#ifdef __cplusplus
}
#endif

#endif
