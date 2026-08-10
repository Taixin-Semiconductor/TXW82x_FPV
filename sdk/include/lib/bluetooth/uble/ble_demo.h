/**
  ******************************************************************************
  * @file    sdk\include\lib\bluetooth\uble
  * @author  TAIXIN-SEMI Application Team
  * @version V2.0.0
  * @date    03-07-2025
  * @brief   This file contains three BLE configuration network features.
  ******************************************************************************
  * @attention
  *
  * COPYRIGHT 2023 TAIXIN-SEMI
  *
  ******************************************************************************
  */ 
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _BLE_DEMO_H_
#define _BLE_DEMO_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "lib/bluetooth/hci/hci_host.h"
#include "lib/bluetooth/uble/uble.h"
#include "lib/bluetooth/uble/ble_adv.h"

enum switch_type
{
    WIRELESS_WIFI,
    WIRELESS_BLE,
};

void  ble_adv_parse_param(uint8 *data, uint32 len);
int32 ble_demo_init(void);

int32 ble_set_mode(uint8 mode, uint8 chan);
int32 ble_set_coexist_en(uint8 coexist, uint8 dec_duty);

#ifdef __cplusplus
}
#endif
#endif
/*************************** (C) COPYRIGHT 2023 TAIXIN-SEMI ***** END OF FILE *****/

