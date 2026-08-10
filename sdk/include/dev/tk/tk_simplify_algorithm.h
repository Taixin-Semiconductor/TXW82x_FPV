/**
  ******************************************************************************
  * @file    tk_simplify_algorithm.h
  * @author  TAIXIN-IC Application Team
  * @version V1.0.0
  * @date    01-05-2021
  * @brief   This file contains all the GPIO LL firmware functions.
  ******************************************************************************
  * @attention
  *
  * 用户不允许修改
  *
  *
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LIB_TK_ALGORITHM_H
#define __LIB_TK_ALGORITHM_H
#ifdef __cplusplus
extern "C" {
#endif
#include "dev/tk/hg_touchkey_v2.h"
/* Includes ------------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
// 用户不允许修改
extern void                     __tk_lib_init(struct hgtouchkey_v2 *tk);
extern int32                     __tk_scan(struct os_work *work);
extern int32                     __tk_ms_handler(struct os_work *work);

#define __CPU_NOP()  {__NOP(); __NOP(); __NOP(); __NOP();}

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif // __LIB_TK_ALGORITHM_H__

/**
  * @}
  */

/**
  * @}
  */

/*************************** (C) COPYRIGHT 2021 TAIXIN-IC ***** END OF FILE ****/
