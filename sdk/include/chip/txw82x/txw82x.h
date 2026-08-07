#ifndef _TXW82X_DEF_H_
#define _TXW82X_DEF_H_
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////////
#define RETURN_ADDR()        __builtin_return_address(0)
#define cpu_dcache_disable() csi_dcache_clean_invalid(); csi_dcache_disable();
#define cpu_dcache_enable()  csi_dcache_enable()

typedef signed char          int8;
typedef signed short         int16;
typedef signed int           int32;
typedef signed long long     int64;

typedef unsigned char        uint8;
typedef unsigned short       uint16;
typedef unsigned int         uint32;
typedef unsigned long long   uint64;
typedef unsigned long ulong;

#ifndef _SIZE_T_DECLARED
typedef unsigned int    size_t;
#define _SIZE_T_DECLARED
#endif

#ifndef _SSIZE_T_DECLARED
#if defined(__INT_MAX__) && __INT_MAX__ == 2147483647
typedef int ssize_t;
#else
typedef long ssize_t;
#endif
#define _SSIZE_T_DECLARED
#endif
/////////////////////////////////////////////////////////////////////////////////

/* -------------------------  Interrupt Number Definition  ------------------------ */

enum IRQn {
    /******  Peripheral Interrupt Numbers *********************************************************/
    USB20DMA_IRQn               = 0,
    USB20MC_IRQn                = 1,
    UART0_IRQn                  = 2,
    UART1_IRQn                  = 3,
    LCD_IRQn                    = 4,
    QSPI_IRQn                   = 5,
    SPI0_IRQn                   = 6,
    SPI1_IRQn                   = 7,
    SPI2_IRQn                   = 8,
    OSPI_IRQn                   = 9,
    TIM0_IRQn                   = 10,
    TIM1_IRQn                   = 11,
    TIM2_IRQn                   = 12,
    TIM3_IRQn                   = 13,
    SCALE1_IRQn                 = 14,
    AUDIO_SUBSYS2_IRQn          = 15,//Swap the position of number 2 with number 0 ,
    AUDIO_SUBSYS1_IRQn          = 16,//To keep consist with rtl design
    AUDIO_SUBSYS0_IRQn          = 17,//
    SDIO_IRQn                   = 18,
    SDHOST1_IRQn                = 19,
    SDHOST_IRQn                 = 20,
    LMAC_IRQn                   = 21,
    GMAC_IRQn                   = 22,
    DUAL_ORG_IRQn               = 23,
    GEN422_IRQn                 = 24,
    CORET_IRQn                  = 25, //CPU TIMER
    SHA_IRQn                    = 26, //SHA/SYS_AES
    CRC_IRQn                    = 27,
    ADKEY0_IRQn                 = 28,
    PD_TMR_IRQn                 = 29,
    WKPND_IRQn                  = 30,
    PDWKPND_IRQn                = 31,
    LVD_IRQn                    = 32,
    WDT_IRQn                    = 33,
    SYS_ERR_IRQn                = 34,
    IIS0_IRQn                   = 35,
    IIS1_IRQn                   = 36,
    GPIOA_IRQn                  = 37,
    LO_MNT_IRQn                 = 38,
    DVP_IRQn                    = 39,
    MJPEG01_IRQn                = 40,
    H264ENC_IRQn                = 41,
    VPP_IRQn                    = 42,
    PRC_IRQn                    = 43,
    STMR5_IRQn                  = 44,
    PDM_IRQn                    = 45,
    LED_TMR_IRQn                = 46,
    SCALE2_IRQn                 = 47,
    GFSK_IRQn                   = 48,
    CMPOUT0_IRQn                = 49,
    UART6_IRQn                  = 50,
    SCALE3_IRQn                 = 51,
    IMG_ISP_IRQn                = 52,
    MIPI_CSI2_IRQn              = 53,
    RF_IRQn                     = 54,
    USB11_MC_IRQn               = 55,
    USB11_DMA_IRQn              = 56,
    RTC_VCCHVD_IRQn             = 57,
    RTC_IRQn                    = 58,
    MIPI_DSI_IRQn               = 59,
    PARA_OUT_IRQn               = 60,
    M2M_DMA0_IRQn               = 61,
    CAN_IRQn                    = 62,
    EXT_DCACHE_IRQn             = 63,
    CPU_SPINLOCK_IRQn           = 64,
    CPU_RECV_MAIL_IRQn          = 65,
    CPU_SEND_MAIL_IRQn          = 66,
    CPU_DBG_ON_IRQn             = 67,
    OSPI_FIFO_IRQn              = 68,
    OSPI_EFF_IRQn               = 69,
    GEN420_IRQn                 = 70,
    SYS_AES_IRQn                = 71,
    PKA_IRQn                    = 72,
    WDT1_IRQn                   = 73,
    GPIOB_IRQn                  = 74,
    GPIOC_IRQn                  = 75,
    GPIOD_IRQn                  = 76,
    GPIOE_IRQn                  = 77,
    GPIOF_IRQn                  = 78,
    DVP1_IRQn                   = 79,
    PNG_IRQn                    = 80,
    STMR4_IRQn                  = 81,
    STMR3_IRQn                  = 82,
    STMR2_IRQn                  = 83,
    STMR1_IRQn                  = 84,
    STMR0_IRQn                  = 85,
    CMPOUT1_IRQn                = 86,
    UART5_IRQn                  = 87,
    UART4_IRQn                  = 88,
    MIPI1_CSI2_IRQn             = 89,
    DMA2D_IRQn                  = 90,
    CSC_IRQn                    = 91,
    OSD_ENC_IRQn                = 92,
    USB20PHY_IRQn               = 93,
    PARA_IN_IRQn                = 94,
    M2M_DMA1_IRQn               = 95,
    WKUP_IRQn                   = 96,
    SAM_IRQn                    = 97,
    LIN_IRQn                    = 98,
    LIN1_IRQn                   = 99,
    CPU_SOFT_INT_IRQn           = 100,
    M2M_DMA2_IRQn               = 105, 

    IRQ_NUM,
};

/* ================================================================================ */
/* ================      Processor and Core Peripheral Section     ================ */
/* ================================================================================ */

/* --------  Configuration of the CK804 Processor and Core Peripherals  ------- */
#define __CK803_REV               0x0000U   /* Core revision r0p0 */
#define __CK804_REV               0x0000U   /* Core revision r0p0 */
#define __MPU_PRESENT             0         /* MGU present or not */
#define __VIC_PRIO_BITS           3         /* Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig    0         /* Set to 1 if different SysTick Config is used */

#include "csi_core.h"                       /* Processor and core peripherals */
#include "stdint.h"


/** @addtogroup Peripheral_registers_structures
  * @{
  */

/**
  * @}
  */

/** @addtogroup Peripheral_memory_map
  * @{
  */
#define PSRAM_BASE_I            ((uint32_t)0x08000000) /*!< SRAM base address in the alias region */
#define PSRAM_END_ADDR_I        ((uint32_t)0x10000000) /*!< SRAM end address in the alias region */
#define FLASH_BASE              ((uint32_t)0x10000000) /*!< FLASH base address in the alias region */
#define FLASH_END_BASE          ((uint32_t)0x20000000) /*!< FLASH base address in the alias region */
#define SRAM_BASE               ((uint32_t)0x20000000) /*!< SRAM base address in the alias region */
#define PERIPH_BASE             ((uint32_t)0x40000000) /*!< Peripheral base address in the alias region */
#define DCACHE_CTRL_BASE        ((uint32_t)0x50000000)
#define PSRAM_BASE              ((uint32_t)0x28000000) /*!< SRAM base address in the alias region */
#define PSRAM_END_ADDR          ((uint32_t)0x30000000) /*!< SRAM end address in the alias region */

/*!< Peripheral memory map */
#define APB0_BASE                PERIPH_BASE
#define APB1_BASE               (PERIPH_BASE + 0x10000)
#define APB2_BASE               (PERIPH_BASE + 0xc0000)
#define AHB_BASE                (PERIPH_BASE + 0x20000)
#define GPIO_BASE               (PERIPH_BASE + 0xe0000)

#define SYSCTRL_BASE            (AHB_BASE + 0x0000)
#define CPUDBG_BASE             (AHB_BASE + 0x0280)

#define GPIOA_BASE              (GPIO_BASE + 0x0000)
#define GPIOB_BASE              (GPIO_BASE + 0x0100)
#define GPIOC_BASE              (GPIO_BASE + 0x0200)
#define GPIOD_BASE              (GPIO_BASE + 0x0300)
#define GPIOE_BASE              (GPIO_BASE + 0x0400)
#define GPIOF_BASE              (GPIO_BASE + 0x0500)

#define SYS_MNT_BASE            (AHB_BASE + 0x023c)
#define SDHOST_BASE             (0x40040048)
#define SDHOST1_BASE            (0x40040068)
#define SDIO_HOST_BASE          (AHB_BASE + 0x3000) //not used
#define SDIO_SLAVE_BASE         (AHB_BASE + 0x20000)
#define LMAC_BASE               (AHB_BASE + 0x42000)
#define USB11S_BASE             (PERIPH_BASE + 0x70000)
#define GMAC_BASE               (PERIPH_BASE + 0x71000)
#define MODEM_BASE              (PERIPH_BASE + 0xd0000)
#define USB20_BASE              (PERIPH_BASE + 0xf0000)
//#define DMAC_BASE               (AHB_BASE + 0x0000)
//#define GPIOA_BASE              (AHB_BASE + 0x2000)
//#define GPIOB_BASE              (AHB_BASE + 0x5000)
//#define SYS_MNT_BASE            (AHB_BASE + 0x9000)

//-------------------------------------------------------
// APB0 SYNC CLK DOMAIN PERIS
//-------------------------------------------------------
#define QSPI_BASE               (APB0_BASE + 0x0000)
#define OSPI_BASE               (APB0_BASE + 0x1000)
#define SCHED_BASE              (APB0_BASE + 0x2000)
#define UART0_BASE              (APB2_BASE + 0x0000)
#define UART1_BASE              (APB2_BASE + 0x0100)
#define UART2_BASE              (APB2_BASE + 0x0200)
#define UART3_BASE              (APB2_BASE + 0x0300)
#define UART4_BASE              (APB2_BASE + 0x0b70)
#define UART5_BASE              (APB2_BASE + 0x0b90)
#define UART6_BASE              (APB2_BASE + 0x0bb0)
#define SPI0_BASE               (APB0_BASE + 0x7400)
#define SPI1_BASE               (APB0_BASE + 0x7500)
#define SPI2_BASE               (APB2_BASE + 0x0600)
#define SPI3_BASE               (APB2_BASE + 0x0700)
#define SPI4_BASE               (APB2_BASE + 0x0800)
#define SPI5_BASE               (APB2_BASE + 0x0b20)
#define SPI6_BASE               (APB2_BASE + 0x0b38)
#define LED_BASE                (APB0_BASE + 0x4b50)
#define IIC0_BASE               (APB0_BASE + 0x7400)
#define IIC1_BASE               (APB0_BASE + 0x7500)
#define IIC2_BASE               (APB2_BASE + 0x0600)
#define IIC3_BASE               (APB2_BASE + 0x0700)
#define IIC4_BASE               (APB2_BASE + 0x0800)
#define IIS0_BASE               (APB2_BASE + 0x3400)
#define IIS1_BASE               (APB2_BASE + 0x3440)
#define PDM_BASE                (APB2_BASE + 0x3480)
#define PRC_BASE                (APB0_BASE + 0x5000)
#define OF_BASE                 (APB0_BASE + 0x5020)
#define FT_BASE                 (APB0_BASE + 0x5040)
#define PALE_BASE               (APB0_BASE + 0x5060)
#define VPP_BASE                (APB0_BASE + 0x5100)
#define MJPEG0_BASE             (APB0_BASE + 0x5200)
#define MJPEG0_DQT_BASE         (APB0_BASE + 0x5280)
#define MJPEG1_BASE             (APB0_BASE + 0x5300)
#define CSI2_HOST_BASE          (APB0_BASE + 0x5700)
#define MJPEG0_TAB_BASE         (APB0_BASE + 0x5280)
#define MJPEG1_TAB_BASE         (APB0_BASE + 0x5380)
#define MJPEG_HUFF_BASE         (APB0_BASE + 0x5400)
#define PARA_OUT_BASE           (APB0_BASE + 0x6000)
#define PARA_IN_BASE            (APB0_BASE + 0x6040)
#define DVP_BASE                (APB0_BASE + 0x6070)
#define DVP1_BASE               (APB0_BASE + 0x6088)
#define GEN422_BASE             (APB0_BASE + 0x60a0)
#define GEN420_BASE             (APB0_BASE + 0x60e0)
#define DUAL_ORG_BASE           (APB0_BASE + 0x6110)
#define MIPI_DSI_BASE           (APB0_BASE + 0x6200)
#define MIPI_CSI0_BASE          (APB0_BASE + 0x6400)
#define MIPI_CSI1_BASE          (APB0_BASE + 0x6600)
#define SCALE1_BASE             (APB0_BASE + 0x7100)
#define LCDC_BASE               (APB0_BASE + 0x7000)
#define ROT_BASE                (APB1_BASE + 0xc140)
#define FILL_BASE               (APB1_BASE + 0xc180)
#define CSC_BASE                (APB1_BASE + 0xc200)
#define OSD_ENC_BASE            (APB1_BASE + 0xc240)
#define SHADOW_BASE             (APB1_BASE + 0xc280)
#define TRANSFORM_BASE          (APB1_BASE + 0xc300)
#define SCALE2_BASE             (APB0_BASE + 0x71A0)
#define SCALE3_BASE             (APB0_BASE + 0x7200)
#define AUDIO_BASE              (APB2_BASE + 0x3000)


#define COMP_BASE               (APB0_BASE + 0x9000)
#define EFUSE_BASE              (APB0_BASE + 0xa000)
#define LO_MNT_BASE             (APB2_BASE + 0x3740)
#define AUDIO_DSP               (APB2_BASE + 0x3600)
#define CAN_RBUF_BASE           (APB2_BASE + 0x4000)
#define CAN_TBUF_BASE           (APB2_BASE + 0x4050)
#define CAN_CTRL_BASE           (APB2_BASE + 0x4098)
#define CAN_ACF_BASE            (APB2_BASE + 0x40b8)

#define FPGA_DDR_BASE           (APB0_BASE + 0xf000)
#define H264_REG_BASE           (APB1_BASE + 0x1000)
#define RFADCDIG_BASE           (APB1_BASE + 0x4000)
#define WDT_BASE                (APB2_BASE + 0x1000)
#define WDT1_BASE               (APB2_BASE + 0x1010)
#define TIMER_ALL_BASE          (APB2_BASE + 0x1128)
#define TIMER0_BASE             (APB2_BASE + 0x1100)
#define TIMER1_BASE             (APB2_BASE + 0x1200)
#define TIMER2_BASE             (APB2_BASE + 0x1300)
#define TIMER3_BASE             (APB2_BASE + 0x1400)
#define TIMER4_BASE             (APB2_BASE + 0x1500)
#define SUPTMR_BASE             (APB2_BASE + 0x1600)
#define SIMPLE_TIMER0_BASE      (APB2_BASE + 0x1b00)
#define SIMPLE_TIMER1_BASE      (APB2_BASE + 0x1b10)
#define SIMPLE_TIMER2_BASE      (APB2_BASE + 0x1b20)
#define SIMPLE_TIMER3_BASE      (APB2_BASE + 0x1b30)
#define SIMPLE_TIMER4_BASE      (APB2_BASE + 0x1b40)
#define SIMPLE_TIMER5_BASE      (APB2_BASE + 0x1b50)

#define CRC_BASE                (APB1_BASE + 0x2000)
#define SYS_AES_BASE            (APB1_BASE + 0x2100)
#define SHA_BASE                (APB1_BASE + 0X2200)

#define IMAGE_ISP_BASE          (APB1_BASE + 0x6000)
#define ISP_LSC_BASE            (APB1_BASE + 0x6300)
#define ISP_R_GAMMA_BASE        (APB1_BASE + 0x6400)
#define ISP_G_GAMMA_BASE        (APB1_BASE + 0x6500)
#define ISP_B_GAMMA_BASE        (APB1_BASE + 0x6600)
#define ISP_Y_GAMMA_BASE        (APB1_BASE + 0x6700)
#define ISP_R_HIST_BASE         (APB1_BASE + 0x6800)
#define ISP_G_HIST_BASE         (APB1_BASE + 0x6900)
#define ISP_B_HIST_BASE         (APB1_BASE + 0x6a00)
#define ISP_Y_HIST_BASE         (APB1_BASE + 0x6b00)

#define PMU_BASE                (APB1_BASE + 0x8000)
#define LVD_BASE                (APB1_BASE + 0x8000)
#define RTC_BASE                (APB1_BASE + 0x8000)
#define RFDIG_BASE              (APB1_BASE + 0x9000)
#define ADKEY_BASE              (APB2_BASE + 0x2000)
#define CMP_BASE                (APB1_BASE + 0xa020)
#define ADKEY1_BASE             (APB1_BASE + 0xa024)
#define USB11_BASE              (APB1_BASE + 0xb000)
#define DMA2D_BASE              (APB1_BASE + 0xc000)
#define DMA2D_FGCLUT_BASE       (APB1_BASE + 0xc800)
#define DMA2D_BGCLUT_BASE       (APB1_BASE + 0xcc00)
#define RFDIGCAL_BASE           (APB1_BASE + 0xd000)
#define NPU_BASE                (APB1_BASE + 0xe000)

#define SYSCTRL_BASE            (AHB_BASE + 0x0000)
#define GPIOA_BASE              (GPIO_BASE + 0x0000)
#define GPIOB_BASE              (GPIO_BASE + 0x0100)
#define GPIOC_BASE              (GPIO_BASE + 0x0200)
#define GPIOD_BASE              (GPIO_BASE + 0x0300)
#define GPIOE_BASE              (GPIO_BASE + 0x0400)
#define GPIOF_BASE              (GPIO_BASE + 0x0500)

#define AUDIO_CORDIC_BASE       (AUDIO_BASE+ 0x500)
#define AUDIO_DSP_FLT_BASE      (AUDIO_BASE+ 0x600)
#define AUDIO_EQ_BASE           (AUDIO_BASE+ 0x700)
#define AUDIO_ASRC_BASE         (AUDIO_BASE+ 0x710)
#define AUDIO_DRC_BASE          (AUDIO_BASE+ 0x720)
#define M2M_DMA_BASE            (0x40040000)
#define M2M_DMA0_BASE           (M2M_DMA_BASE + 0x00)
#define M2M_DMA1_BASE           (M2M_DMA_BASE + 0x18)
#define M2M_DMA2_BASE           (M2M_DMA_BASE + 0x30)

#define SYS_MNT_BASE            (AHB_BASE + 0x023c)
#define SDHOST_BASE             (0x40040048)
#define SDHOST1_BASE            (0x40040068)
#define SDIO_SLAVE_BASE         (AHB_BASE + 0x20000)
#define MODEM_BASE              (PERIPH_BASE + 0xd0000)
#define LMAC_BASE               (AHB_BASE + 0x42000)
#define GMAC_BASE               (PERIPH_BASE + 0x71000)
#define USB20_BASE              (PERIPH_BASE + 0xf0000)
#define FT_USB3_BASE            (APB0_BASE + 0x5040)
#define LED_TIMER0_BASE         (PMU_BASE + 0x000d0)
#define LED_TIMER1_BASE         (PMU_BASE + 0x000d4)
#define LED_TIMER2_BASE         (PMU_BASE + 0x000d8)
#define LED_TIMER3_BASE         (PMU_BASE + 0x000dc)
#define CPU0_MSGBOX_BASE        (0x80001000)
#define CPU0_SPLCK_BASE         (0x80001060)
#define CPU1_MSGBOX_BASE        (0x80000000)
#define CPU1_SPLCK_BASE         (0x80000060)
#define CORESETTING_BASE        (0x2006bf00)

typedef enum {
    HG_AHB_PT_ALL,
    HG_AHB_PT_GPIOA_DEBUNCE,
    HG_AHB_PT_GPIOB_DEBUNCE,
    HG_AHB_PT_GPIOC_DEBUNCE,
    HG_AHB_PT_M2M_DMA,
    HG_AHB_PT_SYS_MNT,
    HG_AHB_PT_SDMMC,
    HG_AHB_PT_SDIO,
    HG_AHB_PT_MODEM,
    HG_AHB_PT_LMAC,
    HG_AHB_PT_GMAC,
    HG_AHB0_PT_DISPLAY, //scale0/2/3,rotate,screenshot,osd0,gama,dither,lcd_if,ccm_constrast_saturation,alpha_blend,video
    HG_AHB0_PT_VEDIO_IF, //dual_org,dvp0/1,gen420,gen422,csi0/1,dsi,para_in.

    HG_APB0_PT_ALL,
    HG_AHB0_PT_H264, //dual_org,dvp0/1,gen420,gen422,csi0/1,dsi,para_in.
    HG_APB0_PT_QSPI,
    HG_APB0_PT_OSPI,
    HG_APB0_PT_UART0,
    HG_APB0_PT_UART1,
    HG_APB0_PT_UART4,
    HG_APB0_PT_UART5,
    HG_APB0_PT_UART6,
    HG_APB0_PT_SPI0,
    HG_APB0_PT_SPI1,
    HG_APB0_PT_SPI2,
    HG_APB0_PT_SPI3,
    HG_APB0_PT_SPI5,
    HG_APB0_PT_SPI6,
    HG_APB0_PT_LED,
    HG_APB0_PT_IIC0,
    HG_APB0_PT_IIC1,
    HG_APB0_PT_IIC2,
    HG_APB0_PT_IIC3,
    HG_APB0_PT_IIS0,
    HG_APB0_PT_IIS1,
    HG_APB0_PT_PDM,
    HG_APB0_PT_JPEG,
    HG_APB0_PT_DVP,
    HG_APB0_PT_CMP,
    HG_APB0_PT_EFUSE,

    HG_APB1_PT_ALL,
    HG_APB1_PT_AES,
    HG_APB1_PT_ISP,
    HG_APB1_PT_TMR0,
    HG_APB1_PT_TMR1,
    HG_APB1_PT_TMR2,
    HG_APB1_PT_TMR3,
    HG_APB1_PT_STMR,
    HG_APB1_PT_SUPTMR,
    HG_APB1_PT_CRC,
    HG_APB1_PT_PMU,
    HG_APB1_PT_LVD,
    HG_APB1_PT_ADKEY,
    HG_ASYNC_APB1_PT_AUDIO_SUB, //adc,dac,iis0/1,pdm,cordic,hs,sync,vad
} HG_Peripheral_Type;

typedef enum {
    CPU_SPLOCK_ID_0,
    CPU_SPLOCK_ID_1,
    CPU_SPLOCK_ID_2,
    CPU_SPLOCK_ID_3,
    CPU_SPLOCK_ID_4,
    CPU_SPLOCK_ID_5,
    CPU_SPLOCK_ID_6,
    CPU_SPLOCK_ID_7,
    CPU_SPLOCK_ID_8,
    CPU_SPLOCK_ID_9,
    CPU_SPLOCK_ID_10,
    CPU_SPLOCK_ID_11,
    CPU_SPLOCK_ID_12,
    CPU_SPLOCK_ID_13_DMA2D,
    CPU_SPLOCK_ID_14_EFUSE,
    CPU_SPLOCK_ID_15_DCACHE,
    CPU_SPLOCK_ID_MAX,
} CPU_SPLOCK_ID;

#define EFUSE_OFFSET 3
#define JPG_NUM      2
#define OSC_CLK      32000000UL

#define JPG_NUM      2

#define CPU_CYCLE_VALUE()    csi_coret_get_value()
#define IS_DCACHE_ADDR(addr) ((uint32_t)(addr) >= PSRAM_BASE && (uint32_t)(addr) < PSRAM_END_ADDR)
#define IS_PSRAM_ADDR(addr)  ((uint32_t)(addr) >= PSRAM_BASE && (uint32_t)(addr) < PSRAM_END_ADDR)
#define IS_FLASH_ADDR(addr)  ((uint32_t)(addr) >= FLASH_BASE && (uint32_t)(addr) < FLASH_END_BASE)
#define IS_SRAM_ADDR(addr)   ((uint32_t)(addr) >= SRAM_BASE && (uint32_t)(addr) < PSRAM_BASE)

typedef struct {
    uint32_t crc32;
    volatile uint8 cpu1_ready;
    volatile uint8 soft_int_pending;
    uint8_t  print_level;
    uint8_t  disable_print:1, dcache_maint_en:1, rev:6;
    uint8_t  vif_maxcnt;
    uint8_t  bss_maxcnt;
    uint16_t dbg_flags;
    uint16_t sta_maxcnt;
    uint16_t bss_lifetime;
    uint32_t dbg_uart_dev;
    uint32_t m2m_dma_dev;
    uint32_t wdt1_to;
    void *   wdt1_irq_hdl;
    uint32_t adc_dev;
    uint32_t cpu_clk;
    uint32_t rxbuf_addr;
    uint32_t rxbuf_size;
    uint32_t heap_addr;
    uint32_t heap_size;
    uint32_t skbpool_addr;
    uint32_t skbpool_size;
    uint32_t skbpool_flag;
    uint32_t heap_flag;
} txw82x_CoreSetting;
#define CoreSetting   ((txw82x_CoreSetting *) CORESETTING_BASE)

#ifdef __cplusplus
}
#endif

#endif
