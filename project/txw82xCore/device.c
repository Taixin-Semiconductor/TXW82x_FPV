#include "basic_include.h"
#include "lib/rpc/cpurpc.h"
#include "dev/uart/hguart_v2.h"
#include "dev/uart/hguart_v4.h"
#include "dev/dma/dw_dmac.h"
#include "dev/gpio/hggpio_v4.h"
#include "dev/dma/hg_m2m_dma_core1.h"
#include "dev/crc/hg_crc.h"
#include "dev/dma2d/hg_dma2d_v0.h"

extern struct dma_device *m2mdma;
struct mem_dma_dev mem_dma = {
    .hw      = (void *)M2M_DMA2_BASE,
    .irq_num = {M2M_DMA2_IRQn},
};

struct hgdma2d_v0 dma2d = {
    .hw      = DMA2D_BASE,
    .fgclut  = DMA2D_FGCLUT_BASE,
    .bgclut  = DMA2D_BGCLUT_BASE,
    .irq_num = DMA2D_IRQn,
};

void device_init(void)
{
    m2mdma = NULL;
    if (CoreSetting->m2m_dma_dev) {
        hg_m2m_dma_dev_attach(HG_M2MDMA_DEVID, &mem_dma);
        //m2mdma = (struct dma_device *)&mem_dma; 
    }

    hgdma2d_v0_attach(HG_DMA2D_DEVID, &dma2d);

    cpu_splock_init(CPU1_SPLCK_BASE, CPU_SPINLOCK_IRQn);
    os_printf(KERN_INFO"CPU1 init!\r\n");
}

