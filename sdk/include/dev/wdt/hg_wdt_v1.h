/**
 * @file 
 * @author LeonLeeV
 * @brief 
 * @version 
 * TXW80X; TXW81X
 * @date 2023-08-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */


#ifndef _HGWDT_V1_H
#define _HGWDT_V1_H

#include "hal/watchdog.h"

#ifdef __cplusplus
extern "C" {
#endif


struct hg_wdt_v1 {
    struct watchdog_device  dev;
    void                   *hw;
    wdt_irq_hdl             irq_hdl;
    uint32                  irq_num;
    uint32                  flags;
};

int32 hg_wdt_v1_attach(uint32 dev_id, struct hg_wdt_v1 *watchdog);


#ifdef __cplusplus
}
#endif


#endif




