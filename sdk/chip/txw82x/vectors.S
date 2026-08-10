/*
 * Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 /******************************************************************************
 * @file     vectors.S
 * @brief    define default vector handlers. Should use with
 *           GCC for CSKY Embedded Processors
 * @version  V1.0
 * @date     28. Nove 2017
 ******************************************************************************/

#include <csi_config.h>

.import trap_c

.section .trap_stack
    .align  2
    .globl  g_trapstackalloc
    .global g_trapstackbase
    .global g_top_trapstack
g_trapstackalloc:
g_trapstackbase:
#if defined(__CK803__) || defined(__E803__) || defined(__E803T__) || defined(__S803__) || defined(__S803T__)
    .space 512
#else
    .space 768
#endif
g_top_trapstack:

.text

/******************************************************************************
 * Functions:
 *     void trap(void);
 * default exception handler
 ******************************************************************************/
    .global trap
    .type   trap, %function
trap:
    psrset  ee
    subi    sp, 4
    stw     r0, (sp)

    lrw     r0, g_top_trapstack
#if defined(__E804DF__) /* CPU0 */
    subi    r0, 200
    stm     r0-r31, (r0)

    ldw     r1, (sp)
    addi    sp, 4
    stw     sp, (r0, 56) /* save r14 */
    mov     sp, r0
    stw     r1, (sp)     /* save r0 */

    addi    r0, 128
    fstms   vr0-vr15, (r0)
    mov     r0, sp

    mfcr    r1, epsr
    stw     r1, (sp, 192)
    mfcr    r1, epc
    stw     r1, (sp, 196)
#else /* CPU1 */
    subi    r0, 76
    stm     r0-r15, (r0)
    stw     r28, (r0, 64)

    ldw     r1, (sp)
    addi    sp, 4
    stw     sp, (r0, 56) /* save r14 */
    mov     sp, r0
    stw     r1, (sp)     /* save r0 */

    mfcr    r1, epsr
    stw     r1, (sp, 68)
    mfcr    r1, epc
    stw     r1, (sp, 72)
#endif


    jbsr    trap_c

    .align  2
    .weak   Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    br      trap
    .size   Default_Handler, . - Default_Handler

/*    Macro to define default handlers. Default handler
 *    will be weak symbol and just dead loops. They can be
 *    overwritten by other handlers */
    .macro  def_weak_handler handler_name Default_Handler_name
    .weak   \handler_name
    .globl  \handler_name
    .set    \handler_name, \Default_Handler_name
    .endm

 //weak function
    def_weak_handler   USB20DMA_IRQHandler,                 Default_Handler
    def_weak_handler   USB20MC_IRQHandler,                  Default_Handler
    def_weak_handler   UART0_IRQHandler,                    Default_Handler
    def_weak_handler   UART1_IRQHandler,                    Default_Handler
    def_weak_handler   LCD_IRQHandler,                      Default_Handler
    def_weak_handler   QSPI_IRQHandler,                     Default_Handler
    def_weak_handler   SPI0_IRQHandler,                     Default_Handler
    def_weak_handler   SPI1_IRQHandler,                     Default_Handler
    def_weak_handler   SPI2_IRQHandler,                     Default_Handler
    def_weak_handler   OSPI_IRQHandler,                     Default_Handler
    def_weak_handler   TIM0_IRQHandler,                     Default_Handler
    def_weak_handler   TIM1_IRQHandler,                     Default_Handler
    def_weak_handler   TIM2_IRQHandler,                     Default_Handler
    def_weak_handler   TIM3_IRQHandler,                     Default_Handler
    def_weak_handler   SCALE1_IRQHandler,                   Default_Handler
    def_weak_handler   AUDIO_DAC_IRQHandler,                Default_Handler
    def_weak_handler   AUDIO_ADC_IRQHandler,                Default_Handler
    def_weak_handler   AUDIO_VAD_HS_ALAW_IRQHandler,        Default_Handler
    def_weak_handler   SDIO_IRQHandler,                     Default_Handler
    def_weak_handler   SDHOST1_IRQHandler,                  Default_Handler
    def_weak_handler   SDHOST_IRQHandler,                   Default_Handler
    def_weak_handler   LMAC_IRQHandler,                     Default_Handler
    def_weak_handler   GMAC_IRQHandler,                     Default_Handler
    def_weak_handler   DUAL_ORG_IRQHandler,                 Default_Handler
    def_weak_handler   GEN422_IRQHandler,                   Default_Handler
    def_weak_handler   CORET_IRQHandler,                    Default_Handler
    def_weak_handler   SHA_IRQHandler,                      Default_Handler
    def_weak_handler   CRC_IRQHandler,                      Default_Handler
    def_weak_handler   ADKEY0_IRQHandler,                   Default_Handler
    def_weak_handler   PD_TMR_IRQHandler,                   Default_Handler
    def_weak_handler   WKPND_IRQHandler,                    Default_Handler
    def_weak_handler   PDWKPND_IRQHandler,                  Default_Handler
    def_weak_handler   LVD_IRQHandler,                      Default_Handler
    def_weak_handler   WDT_IRQHandler,                      Default_Handler
    def_weak_handler   SYS_ERR_IRQHandler,                  Default_Handler
    def_weak_handler   IIS0_IRQHandler,                     Default_Handler
    def_weak_handler   IIS1_IRQHandler,                     Default_Handler
    def_weak_handler   GPIOA_IRQHandler,                    Default_Handler
    def_weak_handler   LO_MNT_IRQHandler,                   Default_Handler
    def_weak_handler   DVP_IRQHandler,                      Default_Handler
    def_weak_handler   MJPEG01_IRQHandler,                  Default_Handler
    def_weak_handler   H264ENC_IRQHandler,                  Default_Handler
    def_weak_handler   VPP_IRQHandler,                      Default_Handler
    def_weak_handler   PRC_IRQHandler,                      Default_Handler
    def_weak_handler   STMR5_IRQHandler,                    Default_Handler
    def_weak_handler   PDM_IRQHandler,                      Default_Handler
    def_weak_handler   LED_TMR_IRQHandler,                  Default_Handler
    def_weak_handler   SCALE2_IRQHandler,                   Default_Handler
    def_weak_handler   GFSK_IRQHandler,                     Default_Handler
    def_weak_handler   CMPOUT0_IRQHandler,                  Default_Handler
    def_weak_handler   UART6_IRQHandler,                    Default_Handler
    def_weak_handler   SCALE3_IRQHandler,                   Default_Handler
    def_weak_handler   IMG_ISP_IRQHandler,                  Default_Handler
    def_weak_handler   MIPI_CSI2_IRQHandler,                Default_Handler
    def_weak_handler   RF_IRQHandler,                       Default_Handler
    def_weak_handler   USB11MC_IRQHandler,                  Default_Handler
    def_weak_handler   USB11DMA_IRQHandler,                 Default_Handler
    def_weak_handler   RTC_VCCHVD_IRQHandler,               Default_Handler
    def_weak_handler   RTC_IRQHandler,                      Default_Handler
    def_weak_handler   MIPI_DSI_IRQHandler,                 Default_Handler
    def_weak_handler   PARA_OUT_IRQHandler,                 Default_Handler
    def_weak_handler   M2M0_IRQHandler,                     Default_Handler
    def_weak_handler   CAN_IRQHandler,                      Default_Handler
    def_weak_handler   EXT_DCACHE_IRQHandler,               Default_Handler
    def_weak_handler   CPU_SPINLOCK_IRQHandler,             Default_Handler
    def_weak_handler   CPU_SEND_MAIL_IRQHandler,            Default_Handler
    def_weak_handler   CPU_RECV_MAIL_IRQHandler,            Default_Handler
    def_weak_handler   CPU_DBG_ON_IRQHandler,               Default_Handler
    def_weak_handler   CPU_SOFT_INT_IRQHandler,             Default_Handler
    def_weak_handler   OSPI_FIFO_IRQHandler,                Default_Handler
    def_weak_handler   OSPI_EFF_IRQHandler,                 Default_Handler
    def_weak_handler   GEN420_IRQHandler,                   Default_Handler
    def_weak_handler   SYS_AES_IRQHandler,                  Default_Handler
    def_weak_handler   PKA_IRQHandler,                      Default_Handler
    def_weak_handler   WDT1_IRQHandler,                     Default_Handler
    def_weak_handler   GPIOB_IRQHandler,                    Default_Handler
    def_weak_handler   GPIOC_IRQHandler,                    Default_Handler
    def_weak_handler   GPIOD_IRQHandler,                    Default_Handler
    def_weak_handler   GPIOE_IRQHandler,                    Default_Handler
    def_weak_handler   GPIOF_IRQHandler,                    Default_Handler
    def_weak_handler   DVP1_IRQHandler,                     Default_Handler
    def_weak_handler   PNG_IRQHandler,                      Default_Handler
    def_weak_handler   STMR4_IRQHandler,                    Default_Handler
    def_weak_handler   STMR3_IRQHandler,                    Default_Handler
    def_weak_handler   STMR2_IRQHandler,                    Default_Handler
    def_weak_handler   STMR1_IRQHandler,                    Default_Handler
    def_weak_handler   STMR0_IRQHandler,                    Default_Handler
    def_weak_handler   CMPOUT1_IRQHandler,                  Default_Handler
    def_weak_handler   UART5_IRQHandler,                    Default_Handler
    def_weak_handler   UART4_IRQHandler,                    Default_Handler
    def_weak_handler   MIPI1_CSI2_IRQHandler,               Default_Handler
    def_weak_handler   DMA2D_IRQHandler,                    Default_Handler
    def_weak_handler   CSC_IRQHandler,                      Default_Handler
    def_weak_handler   OSD_ENC_IRQHandler,                  Default_Handler
    def_weak_handler   USB20PHY_IRQHandler,                 Default_Handler
    def_weak_handler   PARA_IN_IRQHandler,                  Default_Handler
    def_weak_handler   M2M1_IRQHandler,                     Default_Handler
    def_weak_handler   WKUP_IRQHandler,                     Default_Handler
    def_weak_handler   SAM_IRQHandler,                      Default_Handler
    def_weak_handler   LIN_IRQHandler,                      Default_Handler
    def_weak_handler   LIN1_IRQHandler,                     Default_Handler

	def_weak_handler   rev0_IRQHandler, Default_Handler
	def_weak_handler   rev1_IRQHandler, Default_Handler
	def_weak_handler   rev2_IRQHandler, Default_Handler
	def_weak_handler   rev3_IRQHandler, Default_Handler
	def_weak_handler   rev4_IRQHandler, Default_Handler

	def_weak_handler   M2M2_IRQHandler, 				  Default_Handler

    def_weak_handler   Reset_Handler,                     Default_Handler
	def_weak_handler   trap0_handler,                     Default_Handler
    def_weak_handler   tspend_handler,                    Default_Handler
