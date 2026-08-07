#ifndef _HG_DMA2D_V0_H_
#define _HG_DMA2D_V0_H_

#include "hal/dma2d.h"
#include "osal/mutex.h"
#include "osal/semaphore.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    DMA2D_COLOR_SIZE_ARGB8888 = 4,
    DMA2D_COLOR_SIZE_RGB888   = 3,
    DMA2D_COLOR_SIZE_RGB565   = 2,
    DMA2D_COLOR_SIZE_ARGB1555 = 2,
    DMA2D_COLOR_SIZE_ARGB4444 = 2,
    DMA2D_COLOR_SIZE_L8       = 1,
    DMA2D_COLOR_SIZE_AL44     = 1,
    DMA2D_COLOR_SIZE_AL88     = 2,
    DMA2D_COLOR_SIZE_L4       = 0,
    DMA2D_COLOR_SIZE_A8       = 1,
    DMA2D_COLOR_SIZE_A4       = 0,
};

enum {
    DMA2D_MODE_MEMCPY         = 0,
    DMA2D_MODE_CONVERT        = 1,
    DMA2D_MODE_MIXTURE        = 2,
    DMA2D_MODE_MEMSET         = 3,
    DMA2D_MODE_MIXTURE_BG     = 4,
    DMA2D_MODE_MIXTURE_FG     = 5,
};

enum {
    DMA2D_CLUT_FLAG_FG = 0,
    DMA2D_CLUT_FLAG_BG,
};

struct hgdma2d_v0 {
    struct dma2d_device  dev;
    uint32               hw;
    uint32               fgclut;
    uint32               bgclut;
    uint32               irq_num;
    uint32               irq_data;
    uint32               result_status;
    dma2d_irq_hdl        irq_hdl;    
    struct os_mutex      done_lock;
    struct os_semaphore  done_sema;
    uint32               done         : 1,
                         dsleep       : 1,
                         suspend      : 1,
                         resume       : 1,
                         fg_clut_flag : 1,
                         bg_clut_flag : 1,
                         reserved     : 26;
};

int32 hgdma2d_v0_attach(uint32 dev_id, struct hgdma2d_v0  *dma2d);

#ifdef __cplusplus
}
#endif

#endif
