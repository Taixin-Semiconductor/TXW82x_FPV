#ifndef _HGPARA_IN_H
#define _HGPARA_IN_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "hal/para_in.h"

struct hgpara_in_hw {
    __IO uint32_t PARA_IN_CON;
    __IO uint32_t PARA_IN_STA;
    __IO uint32_t PARA_IN_ACT_SIZE;
    __IO uint32_t PARA_IN_BLK_SIZE;
    __IO uint32_t PARA_IN_TO_CON;
};

enum hgpara_in_flags {
    HGPARA_IN_FLAGS_OPENED  =    BIT(0),
    HGPARA_IN_FLAGS_READY   =    BIT(1),
    HGPARA_IN_FLAGS_SUSPEND =    BIT(2),
};

struct hgpara_in {
    struct para_in_device   dev;
    struct hgpara_in_hw     *hw;
    para_in_irq_hdl         irq_hdl;
    uint32                  irq_data;
    uint32                  irq_num;

    //状态位
    uint32                      flags;
};

#ifdef __cplusplus
}
#endif


#endif