/**
  ******************************************************************************
  * @file    Libraries/Driver/include/LL/tx_peg_ll_qspi.h
  * @author  HUGE-IC Application Team
  * @version V1.0.0
  * @date    01-07-2019
  * @brief   This file contains all the QSPI LL firmware functions.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2019 HUGE-IC</center></h2>
  *
  *
  *
  ******************************************************************************
  */ 
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _HG_XSPI_PSRAM_H_
#define _HG_XSPI_PSRAM_H_
#include "dev/xspi/hg_xspi.h"

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

     
/** @addtogroup TX_PEG_StdPeriph_Driver CanesVenatici Driver
  * @{
  */
     
/** @addtogroup qspi_interface_gr QSPI Driver
  * @ingroup  TX_PEG_StdPeriph_Driver
  * @{
  */ 

/** @addtogroup QSPI_LL_Driver QSPI LL Driver
  * @ingroup  qspi_interface_gr
  * @brief Mainly the driver part of the QSPI module, which includes \b QSPI \b Register 
  * \b Constants, \b QSPI \b Exported \b Constants, \b QSPI \b Exported \b Struct, \b QSPI
  * \b Data \b transfers \b functions, \b QSPI \b Initialization \b and \b QSPI \b Configuration 
  * \b And \b Interrupt \b Handle \b function.
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/


  
/***** DRIVER API *****/



/***** LL API *****/


  
/***** LL API AND DRIVER API *****/
/*
 * @usage: 
      clk_mhz == 0, use configuration makecode.ini
      clk_mhz == other, use parameter clk_mhz
 *
*/
int ll_xip_clock_init(int clk_mhz);




#ifdef __cplusplus
}
#endif

/**
  * @}
  */

/**
  * @}
  */

#endif 

/*************************** (C) COPYRIGHT 2023 HUGE-IC ***** END OF FILE *****/
