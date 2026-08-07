/**
  ******************************************************************************
  * @file    sysctrl.h
  * @author  HUGE-IC Application Team
  * @version V1.0.0
  * @date    2021.01.14
  * @brief   This file contains all the PowerDomain firmware functions.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2021 HUGE-IC</center></h2>
  *
  *
  * Revision History
  * V1.0.0  2021.01.14  First Release
  *
  ******************************************************************************
  */

// Define to prevent recursive inclusion //////////////////////////////
#ifndef __SYSCTRL_H__
#define __SYSCTRL_H__


#include "typesdef.h"
#include "pmu.h"
#include "misc.h"
#include "cld_cache.h"
//#include "cpurpc.h"

#ifdef __cplusplus
extern "C" {
#endif


/** @defgroup SYSCTRL_Exported_Constants
  * @{
  */

#define CLK_RC128K             128000
#define CLK_HXOSC              40000000
#define CLK_SYSPLL             360000000
#define CLK_USBPLL             480000000
#define CLK_RC10M              10000000
#define CLK_RC8M               8000000
#define CLK_LXOSC32K           32000

#define SYS_CLK_213MHZ         213333333

struct __clock_cfg {
    uint8  clk_source_sel;
    uint8  clk_valid;
    uint16 reserved;
    uint32 exosc_clk_hz; /* read from efuse */
    uint32 hirc_clk_hz;  /* read from efuse */
    uint32 sys_clk;
    uint32 syspll_clk;
};

typedef struct
{
    __IO uint32 SYS_KEY;
    __IO uint32 SYS_CON0;
    __IO uint32 SYS_CON1;
    __IO uint32 SYS_CON2;
    __IO uint32 SYS_CON3;
    __IO uint32 SYS_CON4;
    __IO uint32 SYS_CON5;
    __IO uint32 SYS_CON6;
    __IO uint32 SYS_CON7;
    __IO uint32 SYS_CON8;
    __IO uint32 SYS_CON9;  //rev
    __IO uint32 SYS_CON10; //rev
    __IO uint32 SYS_CON11;
    __IO uint32 SYS_CON12;
    __IO uint32 SYS_CON13;
    __IO uint32 SYS_CON14;
    __IO uint32 SYS_CON15;
    __IO uint32 CLK_CON0;
    __IO uint32 CLK_CON1;
    __IO uint32 CLK_CON2;
    __IO uint32 CLK_CON3;
    __IO uint32 CLK_CON4;
    __IO uint32 CLK_CON5;
    __IO uint32 CHIP_ID;
    __IO uint32 CLK_CON6;
    __IO uint32 CLK_CON7;
    __IO uint32 SRAM0_PD_CON;
    __IO uint32 AIP_CON0; //rev
    __IO uint32 AIP_CON1; //rev
    __IO uint32 IO_MAP;
    __IO uint32 EFUSE_CON;
    __IO uint32 SYS_ERR0;
    __IO uint32 SYS_ERR1;
    __IO uint32 HOSC_MNT;
    __IO uint32 WK_CTRL;
    __IO uint32 LP_CTRL;
    __IO uint32 MBIST_CTRL;
    __IO uint32 MPE0;
    __IO uint32 MPE1;
    __IO uint32 MPE2;
    __IO uint32 MPE3; //rev
    __IO uint32 MPE4; //rev
    __IO uint32 MPE5; //rev
    __IO uint32 MPE0_PND;
    __IO uint32 MPE1_PND;
    __IO uint32 MPE2_PND;
    __IO uint32 MPE3_PND;//rev
    __IO uint32 MPE4_PND;//rev
    __IO uint32 MPE5_PND;//rev
    __IO uint32 MODE_REG;
    __IO uint32 MBIST_MISR;
    __IO uint32 MBIST_MISR1;//rev
    __IO uint32 MBIST_MISR2;//rev
    __IO uint32 MBIST_MISR3;//rev
    __IO uint32 MBIST_MISR4;//rev
    __IO uint32 MBIST_MISR5;//rev
    __IO uint32 MBIST_MISR6;//rev
    __IO uint32 MBIST_MISR7;//rev
    __IO uint32 USB20_PHY_CFG0;
    __IO uint32 USB20_PHY_CFG1;
    __IO uint32 USB20_PHY_CFG2;
    __IO uint32 USB20_PHY_CFG3;
    __IO uint32 USB20_PHY_DBG0;
    __IO uint32 USB20_PHY_DBG1;
         uint32 RESERVED0[16];
    __IO uint32 IOFUNCINCON0;
    __IO uint32 IOFUNCINCON1;
    __IO uint32 IOFUNCINCON2;
    __IO uint32 IOFUNCINCON3;
    __IO uint32 IOFUNCINCON4;
    __IO uint32 IOFUNCINCON5;
    __IO uint32 IOFUNCINCON6;
    __IO uint32 IOFUNCINCON7;
    __IO uint32 IOFUNCINCON8;
    __IO uint32 IOFUNCINCON9;
    __IO uint32 IOFUNCINCON10;
    __IO uint32 IOFUNCINCON11;
    __IO uint32 IOFUNCINCON12;
    __IO uint32 IOFUNCINCON13;
    __IO uint32 IOFUNCINCON14;
    __IO uint32 IOFUNCINCON15;
    __IO uint32 IOFUNCMASK0;
    __IO uint32 QSPI_MAP_CTL;
    __IO uint32 QSPI_ENCDEC_CON0;
    __IO uint32 QSPI_ENCDEC_CON1;
    __IO uint32 TRNG;
    __IO uint32 DCRC_TRIM;
    __IO uint32 CPU1_CON0;//rev
    __IO uint32 CPU1_CON1; //rev
    __IO uint32 IOFUNCMASK1;
    __IO uint32 PE16CON;
    __IO uint32 OSPI_MAP_CTL0;
    __IO uint32 OSPI_MAP_CTL1;
    __IO uint32 IOFUNCMASK2;
	__IO uint32 IOFUNCMASK3;
	__IO uint32 AHB2AHB_FIFO_CTRL;
	__IO uint32 QSPI_ENCDEC_CON2;
	__IO uint32 QSPI_ENCDEC_CON3;
    __IO uint32 OSPI_MAP_CTL2;
    __IO uint32 OSPI_MAP_CTL3;
	__IO uint32 OSPI_IO_MASK;
	__IO uint32 OSPI_DBS0;
	__IO uint32 OSPI_DBS1;
    __IO uint32 OSPI_DLY0 ;
    __IO uint32 OSPI_DLY1 ;	    
    __IO uint32 OSPI_IO_IN_DLY_SEL0 ;    //1E0
    __IO uint32 OSPI_IO_IN_DLY_SEL1 ;    //1E4
    __IO uint32 OSPI_DM_DLY_SEL     ;    //1E8
    __IO uint32 OSPI_IO_OUT_DLY_SEL0;    //1EC
    __IO uint32 OSPI_IO_OUT_DLY_SEL1;    //1F0
        uint32 RESERVED1[3];
    __IO uint32 IOFUNCMASK4;
    __IO uint32 IOFUNCMASK5;
    __IO uint32 IOFUNCMASK6;
    __IO uint32 IOFUNCMASK7;
	__IO uint32 IOFUNCMASK8;
	__IO uint32 EFF_CON;
	__IO uint32 EFF_TIM_PR;
	__IO uint32 EFF_RD_CNT;
	__IO uint32 EFF_WR_CNT; 
    __IO uint32 SYS_CON16;  
	__IO uint32 SPAREIO_CON;
	__IO uint32 SPARE_I_CON0;
	__IO uint32 SPARE_I_CON1;
	__IO uint32 SPARE_I_CON2;
} SYSCTRL_TypeDef;


#define SYSCTRL                 ((SYSCTRL_TypeDef    *) SYSCTRL_BASE)
#define ASYNC_PERIS_CLKCON      (*(volatile uint32*)0x400C1008)
#define ASYNC_PERIS_SFRST       (*(volatile uint32*)0x400C100C)


#if SYS_KEY_OPT_EN
#define sysctrl_unlock()                        do { SYSCTRL->SYS_KEY = 0x3fac87e4; } while(0)
#define sysctrl_lock()                          do { SYSCTRL->SYS_KEY = ~ 0x3fac87e4; } while(0)
#define SYSCTRL_REG_OPT_INIT()                  do { sysctrl_lock(); } while(0)
#define SYSCTRL_REG_OPT(expr)                   do { uint32 flag=disable_irq(); sysctrl_unlock(); expr; sysctrl_lock(); enable_irq(flag); } while(0)
#define EFUSE_REG_OPT(expr)                     do { uint32 flag=disable_irq(); SYSCTRL->SYS_KEY = 0xe0ac87e4; expr; __NOP();__NOP();__NOP(); SYSCTRL->SYS_KEY = ~ 0xe0ac87e4; sysctrl_lock(); enable_irq(flag); } while(0)
#else
#define sysctrl_unlock()
#define sysctrl_lock()
#define SYSCTRL_REG_OPT_INIT()                  do { SYSCTRL->SYS_KEY = 0x3fac87e4; } while(0)
#define SYSCTRL_REG_OPT(expr)                   do { expr; } while(0)
#define EFUSE_REG_OPT(expr)                     do { uint32 flag=disable_irq(); SYSCTRL->SYS_KEY = 0xe0ac87e4; expr; __NOP();__NOP();__NOP(); SYSCTRL->SYS_KEY = ~ 0xe0ac87e4; enable_irq(flag); } while(0)
#endif


#define SYSCTRL_REG_BITS_S0S1(reg, bits)       do { SYSCTRL_REG_OPT( reg &= ~(bits); __NOP();__NOP();__NOP(); (reg) |= (bits); ); } while(0)
#define SYSCTRL_REG_BITS_S1S0(reg, bits)       do { SYSCTRL_REG_OPT( reg |= (bits); __NOP();__NOP();__NOP(); (reg) &= ~(bits); ); } while(0)
#define SYSCTRL_REG_SET_BITS(reg, bits)        do { SYSCTRL_REG_OPT( reg |=  (bits); ); } while(0)
#define SYSCTRL_REG_CLR_BITS(reg, bits)        do { SYSCTRL_REG_OPT( reg &= ~(bits); ); } while(0)
#define SYSCTRL_REG_SET_VALUE(reg, bit_mask, val, bit_pos)  do { SYSCTRL_REG_OPT( reg = ((reg) & ~(bit_mask)) | (((val) << (bit_pos)) & (bit_mask)); ); } while(0)
#define SYSCTRL_REG_BIT_FUN(fun_name, reg, bits)\
__STATIC_INLINE void fun_name##_en(void) { SYSCTRL_REG_SET_BITS(reg, bits); }\
__STATIC_INLINE void fun_name##_dis(void) { SYSCTRL_REG_CLR_BITS(reg, bits); }





/*============================================================
                        Sort by register
  ============================================================*/

enum iis_clk_src {
    IIS_CLK_NONE, 
    IIS_CLK_SYSPLL, 
    IIS_CLK_USBPLL, 
    IIS_CLK_XOSC, 
};
/* SYS_CON0 */
#define sysctrl_iis1_clk_sel(iis_clk_src)         SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON0, BIT(31)|BIT(30), iis_clk_src, 30) 
#define sysctrl_rssi_ck_sel_rssi_done()           SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON0, BIT(29))
#define sysctrl_rssi_ck_sel_rssi_en()             SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON0, BIT(29))
#define sysctrl_rssi_sel_rssi_done()              SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON0, BIT(28))
#define sysctrl_rssi_sel_rssi_en()                SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON0, BIT(28))
#define sysctrl_iis_clk_open()                    SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON0, BIT(10))





#define sysctrl_iis0_clk_sel(iis_clk_src)         SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON0, BIT(11)|BIT(10), iis_clk_src, 10)
#define sysctrl_mac_pa_en()                       SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON0, BIT(9))
#define sysctrl_mac_pa_dis()                      SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON0, BIT(9))
//#define sysctrl_qspi_dlychain_cfg(n)              SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON0, 0x1F0, n, 4)
//#define sysctrl_qspi_clkin_dis()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON0, BIT(3))
//#define sysctrl_qspi_clkin_en()                   SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON0, BIT(3))
//#define sysctrl_mjpeg_reset()                     SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON0, BIT(2))
//#define sysctrl_iis1_reset()                      SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON0, BIT(1))
#define sysctrl_h264_master_sel(n)                SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON0, BIT(1), n, 1)
#define sysctrl_wdt_at_lp_gate_en()               SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON0, BIT(0))
#define sysctrl_wdt_at_lp_gate_dis()              SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON0, BIT(0))


/* SYS_CON1 */
#define sysctrl_qspi_reset()                      SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(31))
#define sysctrl_sha_reset()                       SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(30))
#define sysctrl_usb20sie_reset()                  SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(29))
#define sysctrl_dvp_reset()                       SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(28))
#define sysctrl_dma2ahb_sched_reset()             SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(27))
#define sysctrl_dbg_dma_reset()                   SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(26))
#define sysctrl_rf_dig_calib_reset()              SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(25))
#define sysctrl_saradc_sys_reset()                SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(24))
#define sysctrl_modem_reset()                     SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(22))
#define sysctrl_mac_reset()                       SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(21))
#define sysctrl_sdhc_reset()                      SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(20))
//#define sysctrl_sdio_reset()                      SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(19))
#define sysctrl_usb20_phy_reset()                 SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(18))
//#define sysctrl_pdm_reset()                       SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(17))
#define sysctrl_gmac_sys_rst()                    SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(16))
#define sysctrl_sysaes_reset()                    SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(15))
#define sysctrl_crc_reset()                       SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(14))
//#define sysctrl_iis0_reset()                      SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(13))
//#define sysctrl_timer_reset()                     SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(12))
//#define sysctrl_uart1_reset()                     SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(11))
//#define sysctrl_uart0_reset()                     SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(10))
#define sysctrl_qrc_soft_reset()                  SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(9))
//#define sysctrl_tk_reset()                        SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(8))
//#define sysctrl_spi2_reset()                      SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(7))
#define sysctrl_spi1_reset()                      SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(6))
#define sysctrl_spi0_reset()                      SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(5))
#define sysctrl_wdt_reset()                       SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(4))
#define sysctrl_dvp1_soft_rst()                   SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(3))
#define sysctrl_m2m_dma_reset()                   SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(2))
#define sysctrl_memory_reset()                    SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(1))
#define sysctrl_gpio_reset()                      SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON1, BIT(0))


/* SYS_CON2 */
#define sysctrl_pdm_mclk_div(n)                   SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON2, 0x0FF00000, n, 20) /* N > 1 */
enum pdm_clk_src {
    PDM_CLK_NONE, 
    PDM_CLK_SYSPLL, 
    PDM_CLK_USBPLL, 
    PDM_CLK_XOSC, 
};
#define sysctrl_pdm_clk_sel(pdm_clk_src)          SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON2, BIT(19)|BIT(18), pdm_clk_src, 18) 
#define sysctrl_xosc_loss_nmi_dis()               SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON2, BIT(17))
#define sysctrl_xosc_loss_nmi_en()                SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON2, BIT(17))
#define sysctrl_xosc_loss_swrc_dis()              SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON2, BIT(16))
#define sysctrl_xosc_loss_swrc_en()               SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON2, BIT(16))
#define sysctrl_cpu0_clk_en()                     SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON2, BIT(15))
#define sysctrl_cpu0_clk_dis()                    SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON2, BIT(15))
#define sysctrl_pll1_2x_div_en1_en                SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON2, BIT(14))
#define sysctrl_pll1_2x_div_en1_dis               SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON2, BIT(14))
//#define sysctrl_sdio_cmd_wk_en()                  SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON2, BIT(10))
//#define sysctrl_sdio_cmd_wk_dis()                 SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON2, BIT(10))
//#define sysctrl_cpu_clk_en()                      SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON2, BIT(9))
//#define sysctrl_cpu_clk_dis()                     SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON2, BIT(9))
#define sysctrl_pll1_2x_div1(n)                   SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON2, 0x00001E00, n, 9)
#define sysctrl_cpuclk_ram1bist_en                SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON2, BIT(8))
#define sysctrl_cpuclk_ram1bist_dis               SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON2, BIT(8))
#define sysctrl_gmac_dma_disable()                SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON2, BIT(7))
#define sysctrl_gmac_dma_enable()                 SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON2, BIT(7))
#define sysctrl_syserr_int_dis()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON2, BIT(6))
#define sysctrl_syserr_int_en()                   SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON2, BIT(6))
#define sysctrl_apb1_wr_optimize_dis()            SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON2, BIT(5)) /* sfr write optimze from 3 to 2 cycle */
#define sysctrl_apb1_wr_optimize_en()             SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON2, BIT(5))
#define sysctrl_apb0_wr_optimize_dis()            SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON2, BIT(4))
#define sysctrl_apb0_wr_optimize_en()             SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON2, BIT(4))
#define sysctrl_nmi_port_dis()                    SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON2, BIT(3)) /* PA4 */
#define sysctrl_nmi_port_en()                     SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON2, BIT(3))
#define sysctrl_iis_duplex_dis()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON2, BIT(0))
#define sysctrl_iis_duplex_en()                   SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON2, BIT(0))

/* SYS_CON3 */
//#define sysctrl_gmac_dbc_pll_sel(n)               SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON3, BIT(31), n, 31) 
//#define sysctrl_dvp_dbc_pll_sel(n)                SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON3, BIT(30), n, 30) 
#define sysctrl_iis1_mclk_sel(n)                  SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON3, BIT(29), n, 29) 
#define sysctrl_iis0_mclk_sel(n)                  SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON3, BIT(28), n, 28) 
#define sysctrl_iis01_mclk_sel(n)                 SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON3, BIT(27), n, 27) 
enum gpio_dbc_clk_src {
    GPIO_DEBUNCE_CLK_RC128K, 
    GPIO_DEBUNCE_CLK_XOSC, 
    GPIO_DEBUNCE_CLK_FCLK, 
    GPIO_DEBUNCE_CLK_RC32K,
};
#define sysctrl_gpioc_dbc_clk_sel(gpio_dbc_clk_src)   SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON3, BIT(25)|BIT(24), gpio_dbc_clk_src, 24) 
#define sysctrl_gpiob_dbc_clk_sel(gpio_dbc_clk_src)   SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON3, BIT(23)|BIT(22), gpio_dbc_clk_src, 22) 
#define sysctrl_gpioa_dbc_clk_sel(gpio_dbc_clk_src)   SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON3, BIT(21)|BIT(20), gpio_dbc_clk_src, 20) 
// #define sysctrl_trng_clk_sel(n)                       SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON3, BIT(19), n, 19) 
#define sysctrl_err_rsp_sdio_bus_en()                 SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON3, BIT(18))
#define sysctrl_err_rsp_sdio_bus_dis()                SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON3, BIT(18))


// #define sysctrl_tk_clk_en()                           SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON3, BIT(17))
// #define sysctrl_tk_clk_dis()                          SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON3, BIT(17))
#define sysctrl_err_rsp_dbus_en()                     SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON3, BIT(16))
#define sysctrl_err_rsp_dbus_dis()                    SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON3, BIT(16))
#define sysctrl_err_rsp_ibus_en()                     SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON3, BIT(15))
#define sysctrl_err_rsp_ibus_dis()                    SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON3, BIT(15))
#define sysctrl_audio_mem_pd_en()                     SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON3, BIT(14))
#define sysctrl_audio_mem_pd_dis()                    SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON3, BIT(14))
#define sysctrl_clk_source_en_bps()                   SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON3, BIT(13))
#define sysctrl_clk_source_dis_bps()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON3, BIT(13))
//#define sysctrl_pll1_clk_sel_120M()                   SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON3, BIT(12))
//#define sysctrl_pll1_clk_sel_div3()                   SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON3, BIT(12))
//#define sysctrl_pll_src_upll_div4()                   SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON3, BIT(12))
// #define sysctrl_pll_src_spll_div2()                   SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON3, BIT(11))
// #define sysctrl_pll_src_spll_div3()                   SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON3, BIT(11))
#define sysctrl_bb_clk_en()                           SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON3, BIT(5))
#define sysctrl_bb_clk_dis()                          SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON3, BIT(5))
#define sysctrl_usb_wk_en()                           SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON3, BIT(4))
#define sysctrl_usb_wk_dis()                          SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON3, BIT(4))



/* SYS_CON6 */
//#define sysctrl_dbg_seg3_sel(n)                       SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON6, 0xFF000000, n, 24)
//#define sysctrl_dbg_seg2_sel(n)                       SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON6, 0x00FF0000, n, 16)
//#define sysctrl_dbg_seg1_sel(n)                       SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON6, 0x0000FF00, n, 8)
//#define sysctrl_dbg_seg0_sel(n)                       SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON6, 0x000000FF, n, 0)

/* SYS_CON7 */
//#define sysctrl_dbg_seg7_sel(n)                       SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON7, 0xFF000000, n, 24)
//#define sysctrl_dbg_seg6_sel(n)                       SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON7, 0x00FF0000, n, 16)
//#define sysctrl_dbg_seg5_sel(n)                       SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON7, 0x0000FF00, n, 8)
//#define sysctrl_dbg_seg4_sel(n)                       SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON7, 0x000000FF, n, 0)
#define sysctrl_isp_reset()                           SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(30))
#define sysctrl_parallel_out_reset()                  SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(29))
#define sysctrl_parallel_in_reset()                   SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(28))
#define sysctrl_dsi_reset()                           SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(27))
#define sysctrl_vpp_reset()                           SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(26))
#define sysctrl_csi0_reset()                          SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(25))
#define sysctrl_prc_reset()                           SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(24))
#define sysctrl_csi1_reset()                          SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(23))
#define sysctrl_gen422_reset()                        SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(22))
#define sysctrl_gen420_reset()                        SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(21))
#define sysctrl_mjpeg1_reset()                        SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(20))
#define sysctrl_usb11_reset()                         SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(19))
#define sysctrl_vpp_acc_reset()                       SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(18))
#define sysctrl_dual_org_reset()                      SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(17))
#define sysctrl_csc_reset()                           SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(16))
#define sysctrl_osd_enc_reset()                       SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(15))
#define sysctrl_png_reset()                           SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(14))
#define sysctrl_dma2d_reset()                         SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(13))
#define sysctrl_npu_reset()                           SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(12))
#define sysctrl_h264_reset()                          SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(11))
#define sysctrl_mjpeg_reset()                         SYSCTRL_REG_BITS_S0S1(SYSCTRL->SYS_CON7, BIT(10))
enum pll1_xoscm_refclk_sel {
    XOSCM = 0,
    LXOSC32K = 1,
};
#define sysctrl_pll1_xoscm_refclk_sel(sel)            SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON7, BIT(9), sel, 9)


/* SYS_CON10 */
#define sysctrl_mem_end_16k_remap_sel(n)              SYSCTRL_REG_SET_VALUE(SYSCTRL->SYSCON11, BIT(31)|BIT(30), n, 30)
#define sysctrl_cpu1_swd_anymap_en                    SYSCTRL_REG_SET_BITS(SYSCTRL->SYSCON11, BIT(24))
/* SYS_CON14 */
#define sysctrl_image_isp_pll1_2x_clk_sel(n)          SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON14, BIT(27)|BIT(26)|BIT(25), n, 25)
#define sysctrl_image_isp_pll1_2x_clk_open()          SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON14, BIT(24))
#define sysctrl_image_isp_pll1_2x_clk_close()         SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON14, BIT(24))
/* SYS_CON15 */
#define sysctrl_gpiof_dbc_clk_sel(gpio_dbc_clk_src)   SYSCTRL_REG_SET_VALUE(SYSCTRL->SYSCON15, BIT(28)|BIT(27), gpio_dbc_clk_src, 27)
#define sysctrl_cpu1_cache_en(n)                      SYSCTRL_REG_CLR_BITS(SYSCTRL->SYS_CON15, BIT(22))
#define sysctrl_cpu1_cache_dis(n)                     SYSCTRL_REG_SET_BITS(SYSCTRL->SYS_CON15, BIT(22))
#define sysctrl_gpioe_dbc_clk_sel(gpio_dbc_clk_src)   SYSCTRL_REG_SET_VALUE(SYSCTRL->SYSCON15, BIT(20)|BIT(19), gpio_dbc_clk_src, 19)
#define sysctrl_gpiod_dbc_clk_sel(gpio_dbc_clk_src)   SYSCTRL_REG_SET_VALUE(SYSCTRL->SYSCON15, BIT(18)|BIT(17), gpio_dbc_clk_src, 17)


/* CLK_CON0 */


/* CLK_CON1 */
#define sysctrl_rf_dac_clk_n_posedge()                SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON1, BIT(31))
#define sysctrl_rf_dac_clk_n_negedge()                SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON1, BIT(31))
#define sysctrl_rf_adc_clk_n_posedge()                SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON1, BIT(30))
#define sysctrl_rf_adc_clk_n_negedge()                SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON1, BIT(30))
#define sysctrl_rf_adc_sample_clk_n_posedge()         SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON1, BIT(29))
#define sysctrl_rf_adc_sample_clk_n_negedge()         SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON1, BIT(29))

enum qspi_clk_src {
    QSPI_CLK_USBPLL2X_NP5, 
    QSPI_CLK_USBPLL, 
    QSPI_CLK_XOSC, 
    QSPI_CLK_USBPLL2X_NP5_1,
};


/* CKL_CON2 */
#define sysctrl_dvp_clk_open()             SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(31))
#define sysctrl_dvp_clk_close()            SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(31))
#define sysctrl_qspi_clk_open()            SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(30))
#define sysctrl_qspi_clk_close()           SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(30))
//#define sysctrl_tmr0_clk_open()            SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(29))
//#define sysctrl_tmr0_clk_close()           SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(29))
#define sysctrl_test_clk_open()            SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(28))
#define sysctrl_test_clk_close()           SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(28))
#define sysctrl_stmr_clk_open()            SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(27))
#define sysctrl_stmr_clk_close()           SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(27))
//#define sysctrl_tmr3_clk_open()            SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(26))
//#define sysctrl_tmr3_clk_close()           SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(26))
#define sysctrl_rfdac_clk_open()           SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(25))
#define sysctrl_rfdac_clk_close()          SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(25))
#define sysctrl_rfadda_clk_open()          SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(24))
#define sysctrl_rfadda_clk_close()         SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(24))
#define sysctrl_rf_pclk_open()             SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(23))
#define sysctrl_rf_pclk_close()            SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(23))
#define sysctrl_modem_clk_open()           SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(22))
#define sysctrl_modem_clk_close()          SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(22))
#define sysctrl_mac_clk_open()             SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(21))
#define sysctrl_mac_clk_close()            SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(21))
#define sysctrl_sdhc_clk_open()            SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(20))
#define sysctrl_sdhc_clk_close()           SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(20))
//#define sysctrl_sddev_clk_open()           SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(19))
//#define sysctrl_sddev_clk_close()          SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(19))
#define sysctrl_dsi_open()                 SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(19))
#define sysctrl_dsi_close()                SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(19))
#define sysctrl_usb20_clk_open()           SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(18))
#define sysctrl_usb20_clk_close()          SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(18))
//#define sysctrl_pdm_clk_open()             SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(17))
//#define sysctrl_pdm_clk_close()            SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(17))
//#define sysctrl_simtmr_clk_open()          SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(16))
//#define sysctrl_simtmr_clk_close()         SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(16))
#define sysctrl_sysaes_clk_open()          SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(15))
#define sysctrl_sysaes_clk_close()         SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(15))
#define sysctrl_crc_clk_open()             SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(14))
#define sysctrl_crc_clk_close()            SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(14))
//#define sysctrl_iis0_clk_open()            SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(13))
//#define sysctrl_iis0_clk_close()           SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(13))
//#define sysctrl_tmr1_clk_open()            SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(12))
//#define sysctrl_tmr1_clk_close()           SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(12))
//#define sysctrl_uart1_clk_open()           SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(11))
//#define sysctrl_uart1_clk_close()          SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(11))
//#define sysctrl_uart0_clk_open()           SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(10))
//#define sysctrl_uart0_clk_close()          SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(10))
//#define sysctrl_tmr2_clk_open()            SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(8))
//#define sysctrl_tmr2_clk_close()           SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(8))
//#define sysctrl_spi2_clk_open()            SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(7))
//#define sysctrl_spi2_clk_close()           SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(7))
#define sysctrl_qspi_xosc_sel_xosc         SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(9))
#define sysctrl_qspi_xosc_sel_rc10m        SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(9))

#define sysctrl_qspi_clk_src_sel(qspi_clk_src) SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON2, BIT(8)|BIT(7), qspi_clk_src, 7)
#define sysctrl_spi1_clk_open()            SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(6))
#define sysctrl_spi1_clk_close()           SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(6))
#define sysctrl_spi0_clk_open()            SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(5))
#define sysctrl_spi0_clk_close()           SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(5))
//#define sysctrl_wdt_clk_open()             SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(3))
//#define sysctrl_wdt_clk_close()            SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(3))
#define sysctrl_ahb1_clk_open()            SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(2))
#define sysctrl_ahb1_clk_close()           SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(2))
#define sysctrl_ahb0_clk_open()            SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(1))
#define sysctrl_ahb0_clk_close()           SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(1))
#define sysctrl_adkey_clk_open()           SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON2, BIT(0))
#define sysctrl_adkey_clk_close()          SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON2, BIT(0))


/* CLK_CON3 */
#define sysctrl_parallel_out_clk_div(n)    SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON3, 0xF8000000, n, 27)
//#define sysctrl_dvp_mclk_div(n)            SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON3, 0xFC000000, n, 26)
#define sysctrl_parallel_out_clk_en        SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(26))
#define sysctrl_parallel_out_clk_dis       SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(26))
#define sysctrl_cpu0_cache_en              SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(25))
#define sysctrl_cpu0_cache_dis             SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(25))
#define sysctrl_lmac_fifo_en()             SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(24))
#define sysctrl_lmac_fifo_dis()            SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(24))
#define sysctrl_lvd_dbc_clk_en()           SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(23))
#define sysctrl_lvd_dbc_clk_dis()          SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(23))
enum lvd_dbc_clk_src {
    LVD_DEBUNCE_CLK_RC128K, 
    LVD_DEBUNCE_CLK_XOSC, 
    LVD_DEBUNCE_CLK_FCLK, 
    LVD_DEBUNCE_CLK_RC32K,
};
#define sysctrl_lvd_dbg_clk_sel(lvd_dbc_clk_src)   SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON3, BIT(22)|BIT(21), lvd_dbc_clk_src, 21) 
#define sysctrl_sha_clk_open()                     SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(20))
#define sysctrl_sha_clk_close()                    SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(20))
#define sysctrl_m2mdma_clk_open()                  SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(19))
#define sysctrl_m2mdma_clk_close()                 SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(19))
//#define sysctrl_iis1_clk_open()                    SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(18))
//#define sysctrl_iis1_clk_close()                   SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(18))
#define sysctrl_gmac_clk_open()                    SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(17))
#define sysctrl_gmac_clk_close()                   SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(17))
#define sysctrl_rom_clk_open()                     SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(16))
#define sysctrl_rom_clk_close()                    SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(16))
#define sysctrl_apb1_clk_open()                    SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(15))
#define sysctrl_apb1_clk_close()                   SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(15))
#define sysctrl_mjpeg_clk_open()                   SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(14))
#define sysctrl_mjpeg_clk_close()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(14))
#define sysctrl_sdio_fifo_open()      		       SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(14))
#define sysctrl_sdio_fifo_close()                  SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(14))
#define sysctrl_qspi_pll_div(n)                    SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON3, BIT(13)|BIT(12)|BIT(11), n, 11)
#define sysctrl_sram10_clk_open()                  SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(10))
#define sysctrl_sram10_clk_close()                 SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(10))
#define sysctrl_sram9_clk_open()                   SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(9))
#define sysctrl_sram9_clk_close()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(9))
#define sysctrl_sram8_clk_open()                   SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(8))
#define sysctrl_sram8_clk_close()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(8))
#define sysctrl_sram7_clk_open()                   SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(7))
#define sysctrl_sram7_clk_close()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(7))
#define sysctrl_sram6_clk_open()                   SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(6))
#define sysctrl_sram6_clk_close()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(6))
#define sysctrl_sram5_clk_open()                   SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(5))
#define sysctrl_sram5_clk_close()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(5))
#define sysctrl_sram4_clk_open()                   SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(4))
#define sysctrl_sram4_clk_close()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(4))
#define sysctrl_sram3_clk_open()                   SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(3))
#define sysctrl_sram3_clk_close()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(3))
#define sysctrl_sram2_clk_open()                   SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(2))
#define sysctrl_sram2_clk_close()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(2))
#define sysctrl_sram1_clk_open()                   SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(1))
#define sysctrl_sram1_clk_close()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(1))
#define sysctrl_sram0_clk_open()                   SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON3, BIT(0))
#define sysctrl_sram0_clk_close()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON3, BIT(0))




 /* CLK_CON4 */
#define sysctrl_csi1_clk_open()                    SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON4, BIT(31))
#define sysctrl_csi1_clk_close()                   SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON4, BIT(31))
#define sysctrl_display_reset()                    SYSCTRL_REG_BITS_S0S1(SYSCTRL->CLK_CON4, BIT(30))
#define sysctrl_display_clk_open()                 SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON4, BIT(29))
#define sysctrl_display_clk_close()                SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON4, BIT(29))
#define sysctrl_ospi_set_clk_div(n)                SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON4, 0x1f000000, ((n)-1), 24)
#define sysctrl_ospi_clk_en()                      SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON4, BIT(23))
#define sysctrl_ospi_clk_dis()                     SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON4, BIT(23))
enum ospi_clk_src {
    OSPI_CLK_DCC_CLK, 
    OSPI_CLK_USBPLL, 
    OSPI_CLK_ADPLL, 
    OSPI_CLK_DOUBLE_CLK,
};
#define sysctrl_ospi_clk_src_sel(ospi_clk_src)     SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON4, BIT(22)|BIT(21), ospi_clk_src, 21)
#define sysctrl_ospi_reset()                       SYSCTRL_REG_BITS_S0S1(SYSCTRL->CLK_CON4, BIT(20))

#define sysctrl_qspi_lock()                        SYSCTRL_REG_OPT(SYSCTRL->SYS_KEY = ~0x1fac87e4)
#define sysctrl_qspi_unlock()                      SYSCTRL_REG_OPT(SYSCTRL->SYS_KEY = 0x1fac87e4)
#define sysctrl_ospi_lock()                        SYSCTRL_REG_OPT(SYSCTRL->SYS_KEY = ~0x01ac87e4)
#define sysctrl_ospi_unlock()                      SYSCTRL_REG_OPT(SYSCTRL->SYS_KEY = 0x01ac87e4)

#define sysctrl_audac_set_clk_div(n)               SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON4, 0x0000f800, (n), 11)
#define sysctrl_auadc_set_clk_div(n)               SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON4, 0x000007c0, (n), 6)
enum audac_pll_sel{
    AUDAC_CLK_NONE,
    AUDAC_CLK_USBPLL,
    AUDAC_XOSC,
    AUDAC_RC10M,
};
#define sysctrl_audac_pll_sel(audac_pll_sel)       SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON4, BIT(5)|BIT(4), audac_pll_sel, 4)
#define sysctrl_dcc_clk_sel_dly(n)                 SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON4, BIT(3)|BIT(2), n, 2)
#define sysctrl_usb11_clk_open()                   SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON4, BIT(1))
#define sysctrl_usb11_clk_close()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON4, BIT(1))
#define sysctrl_mjpeg1_clk_open()                  SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON4, BIT(0))
#define sysctrl_mjpeg1_clk_close()                 SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON4, BIT(0))


 /* CLK_CON5 */
#define sysctrl_cpu1_softrst_system_en()          SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON5, BIT(27))
#define sysctrl_cpu1_softrst_system_dis()         SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON5, BIT(27))
#define sysctrl_clk_to_io_set_div(n)               SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON5,0x00ff0000, n, 16)
#define sysctrl_osd_enc_clk_open()                 SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON5, BIT(15))
#define sysctrl_osd_enc_clk_close()                SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON5, BIT(15))
#define sysctrl_dma2ahb_sche_clk_open()            SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON5, BIT(14))
#define sysctrl_dma2ahb_sche_clk_close()           SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON5, BIT(14))
#define sysctrl_img_isp_clk_open()                 SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON5, BIT(13))
#define sysctrl_img_isp_clk_close()                SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON5, BIT(13))
#define sysctrl_gmac_clk_sel(n)                    SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON5,BIT(10)|BIT(9), n, 9)
#define sysctrl_mipi_cfgclk_div(n)                 SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON5,0x000001f0, n, 4)
#define sysctrl_vpp_clk_open()                     SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON5, BIT(2))
#define sysctrl_vpp_clk_close()                    SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON5, BIT(2))
#define sysctrl_csi0_clk_open()                    SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON5, BIT(1))
#define sysctrl_csi0_clk_close()                   SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON5, BIT(1))
#define sysctrl_prc_clk_open()                     SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON5, BIT(0))
#define sysctrl_prc_clk_close()                    SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON5, BIT(0))


 /* CLK_CON6 */
#define sysctrl_gen422_clk_open()                  SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON6, BIT(3))
#define sysctrl_gen422_clk_close()                 SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON6, BIT(3))
#define sysctrl_h264_clk_open()                    SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON6, BIT(6))
#define sysctrl_h264_clk_close()                   SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON6, BIT(6))


/* CLK_CON7 */
#define sysctrl_csc_clk_open()                     SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON7, BIT(19))
#define sysctrl_csc_clk_close()                    SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON7, BIT(19))
#define sysctrl_png_clk_open()                     SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON7, BIT(18))
#define sysctrl_png_clk_close()                    SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON7, BIT(18))
#define sysctrl_dvp1_clk_open()                    SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON7, BIT(17))
#define sysctrl_dvp1_clk_close()                   SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON7, BIT(17))
#define sysctrl_dual_org_clk_open()                SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON7, BIT(15))
#define sysctrl_dual_org_clk_close()               SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON7, BIT(15))
#define sysctrl_vpp_acc_clk_open()                 SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON7, BIT(14))
#define sysctrl_vpp_acc_clk_close()                SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON7, BIT(14))
#define sysctrl_parallel_in_clk_open()             SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON7, BIT(13))
#define sysctrl_parallel_in_clk_close()            SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON7, BIT(13))
#define sysctrl_gen420_clk_open()                  SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON7, BIT(12))
#define sysctrl_gen420_clk_close()                 SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON7, BIT(12))
#define sysctrl_image_isp_clk_sel(n)               SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON7,BIT(1)|BIT(2), n, 1)

 /* CLK_CON8 */
#define sysctrl_sysmonitor_clk_open()              //SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON8, BIT(23))
#define sysctrl_sysmonitor_clk_close()             //SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON8, BIT(23))


/* ASYNC_PERIS_CLKCON */
#define sysctrl_dsp_clk_open()                     SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(24))
#define sysctrl_dsp_clk_close()                    SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(24))
#define sysctrl_lin_clk_open()                     SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(23))
#define sysctrl_lin_clk_close()                    SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(23))
#define sysctrl_sam_clk_open()                     SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(22))
#define sysctrl_sam_clk_close()                    SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(22))
#define sysctrl_adkey1_clk_open()                  SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(21))
#define sysctrl_adkey1_clk_close()                 SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(21))
#define sysctrl_can_clk_open()                     SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(20))
#define sysctrl_can_clk_close()                    SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(20))
#define sysctrl_pdm_clk_open()                     //SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(19))
#define sysctrl_pdm_clk_close()                    //SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(19))
#define sysctrl_iis1_clk_open()                    SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(18))
#define sysctrl_iis1_clk_close()                   SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(18))
#define sysctrl_iis0_clk_open()                    SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(17))
#define sysctrl_iis0_clk_close()                   SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(17))
#define sysctrl_audac_clk_open()                   SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(16))
#define sysctrl_audac_clk_close()                  //SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(16))
#define sysctrl_auadc_clk_open()                   SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(15))
#define sysctrl_auadc_clk_close()                  //SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(15))
#define sysctrl_uart1_clk_open()                   SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(14))
#define sysctrl_uart1_clk_close()                  SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(14))
#define sysctrl_uart0_clk_open()                   SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(13))
#define sysctrl_uart0_clk_close()                  SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(13))
#define sysctrl_spi2_clk_open()                    SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(12))
#define sysctrl_spi2_clk_close()                   SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(12))
#define sysctrl_epwm3_clk_open()                   SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(9))
#define sysctrl_epwm3_clk_close()                  SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(9))
#define sysctrl_epwm2_clk_open()                   SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(8))
#define sysctrl_epwm2_clk_close()                  SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(8))
#define sysctrl_epwm1_clk_open()                   SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(7))
#define sysctrl_epwm1_clk_close()                  SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(7))
#define sysctrl_epwm0_clk_open()                   SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(6))
#define sysctrl_epwm0_clk_close()                  SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(6))
#define sysctrl_simtmr_clk_open()                  SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(5))
#define sysctrl_simtmr_clk_close()                 SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(5))
#define sysctrl_tmr3_clk_open()                    SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(4))
#define sysctrl_tmr3_clk_close()                   SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(4))
#define sysctrl_tmr2_clk_open()                    SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(3))
#define sysctrl_tmr2_clk_close()                   SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(3))
#define sysctrl_tmr1_clk_open()                    SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(2))
#define sysctrl_tmr1_clk_close()                   SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(2))
#define sysctrl_tmr0_clk_open()                    SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(1))
#define sysctrl_tmr0_clk_close()                   SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(1))
#define sysctrl_wdt_clk_open()                     SYSCTRL_REG_SET_BITS(ASYNC_PERIS_CLKCON, BIT(0))
#define sysctrl_wdt_clk_close()                    SYSCTRL_REG_CLR_BITS(ASYNC_PERIS_CLKCON, BIT(0))



/* ASYNC_PERIS_SFRST */
#define sysctrl_lin_reset()                        SYSCTRL_REG_BITS_S0S1(ASYNC_PERIS_SFRST, BIT(22))
#define sysctrl_sam_reset()                        SYSCTRL_REG_BITS_S0S1(ASYNC_PERIS_SFRST, BIT(21))
#define sysctrl_can_reset()                        SYSCTRL_REG_BITS_S0S1(ASYNC_PERIS_SFRST, BIT(20))
#define sysctrl_pdm_reset()                        SYSCTRL_REG_BITS_S0S1(ASYNC_PERIS_SFRST, BIT(19))
#define sysctrl_iis1_reset()                       SYSCTRL_REG_BITS_S0S1(ASYNC_PERIS_SFRST, BIT(18))
#define sysctrl_iis0_reset()                       SYSCTRL_REG_BITS_S0S1(ASYNC_PERIS_SFRST, BIT(17))
#define sysctrl_audac_reset()                      SYSCTRL_REG_BITS_S0S1(ASYNC_PERIS_SFRST, BIT(16))
#define sysctrl_auadc_reset()                      SYSCTRL_REG_BITS_S0S1(ASYNC_PERIS_SFRST, BIT(15))
#define sysctrl_uart1_reset()                      SYSCTRL_REG_BITS_S0S1(ASYNC_PERIS_SFRST, BIT(14))
#define sysctrl_uart0_reset()                      SYSCTRL_REG_BITS_S0S1(ASYNC_PERIS_SFRST, BIT(13))
#define sysctrl_spi2_reset()                       SYSCTRL_REG_BITS_S0S1(ASYNC_PERIS_SFRST, BIT(12))
#define sysctrl_epwm3_reset()                      SYSCTRL_REG_BITS_S0S1(ASYNC_PERIS_SFRST, BIT(9))
#define sysctrl_epwm2_reset()                      SYSCTRL_REG_BITS_S0S1(ASYNC_PERIS_SFRST, BIT(8))
#define sysctrl_epwm1_reset()                      SYSCTRL_REG_BITS_S0S1(ASYNC_PERIS_SFRST, BIT(7))
#define sysctrl_epwm0_reset()                      SYSCTRL_REG_BITS_S0S1(ASYNC_PERIS_SFRST, BIT(6))
#define sysctrl_timer_reset()                      SYSCTRL_REG_BITS_S0S1(ASYNC_PERIS_SFRST, BIT(1))
#define sysctrl_wdt_sys_reset()                    SYSCTRL_REG_BITS_S0S1(ASYNC_PERIS_SFRST, BIT(0))





/*============================================================
                        out-of-order
  ============================================================*/

 enum dvp_pll_src {
     DVP_PLL_SRC_XOSC, 
     DVP_PLL_SRC_SPLL, 
     DVP_PLL_SRC_UPLL,
 };
//#define sysctrl_dvp_pll_sel(dvp_pll_src) 		  SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON1, BIT(28)|BIT(27), dvp_pll_src, 27)
//#define sysctrl_qspi_pll_sel(n)          		  SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON1, BIT(26), n, 26)

//todo: check
#define sysctrl_qspi_clk_sel_spll()               do {  SYSCTRL_REG_OPT( SYSCTRL->CLK_CON0 &= ~(BIT(3));\
                                                         SYSCTRL->CLK_CON0 &= ~(BIT(26)); );\
                                                  } while(0)
#define sysctrl_qspi_clk_sel_upll()               do {  SYSCTRL_REG_OPT( SYSCTRL->CLK_CON0 &= ~(BIT(3));\
                                                         SYSCTRL->CLK_CON1 |= (BIT(26)); );\
                                                  } while(0)
#define sysctrl_qspi_clk_sel_hxosc()              do {  SYSCTRL_REG_OPT( SYSCTRL->CLK_CON0 |= (BIT(3));\
                                                         SYSCTRL->CLK_CON1 &= ~(BIT(26)); );\
                                                  } while(0)
#define sysctrl_qspi_clk_sel_rc10m()              do {  SYSCTRL_REG_OPT( SYSCTRL->CLK_CON0 |= (BIT(3));\
                                                         SYSCTRL->CLK_CON1 |= (BIT(26)); );\
                                                  } while(0)

#define sysctrl_qspi_get_clk_div()              ((SYSCTRL->CLK_CON3 & 0x00003800) >> 11)

// enum qspi_clk_src {
//     QSPI_CLK_NONE, 
//     QSPI_CLK_USBPLL, 
//     QSPI_CLK_XOSC, 
//     QSPI_CLK_RC10M,
// };
//#define sysctrl_qspi_clk_src_sel(qspi_clk_src)\
//do {\
//    if (qspi_clk_src & BIT(1))    SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON0, BIT(3));\
//    else                          SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON0, BIT(3));\
//    if (qspi_clk_src & BIT(0))    SYSCTRL_REG_SET_BITS(SYSCTRL->CLK_CON1, BIT(26));\
//    else                          SYSCTRL_REG_CLR_BITS(SYSCTRL->CLK_CON1, BIT(26));\
//} while (0)

#define sysctrl_iis_clk_set(init) \
    SYSCTRL->CLK_CON0 = (SYSCTRL->CLK_CON0 & ~(0x7f << 16)) | (((init) ? 23 : 3) << 16)

#define sysctrl_iis_clk_reset() \
    SYSCTRL->CLK_CON0 = (SYSCTRL->CLK_CON0  & ~(0x7f << 16))  | (0<< 16)

#define sysctrl_qspi_clk_src_get() (SYSCTRL->CLK_CON0 & BIT(3)) ? ((SYSCTRL->CLK_CON1 & BIT(26)) ? 0x3 : 0x2) : ((SYSCTRL->CLK_CON1 & BIT(26)) ? 0x1 : 0x0);


#define sysctrl_ospi_clk_src_get()              ((SYSCTRL->CLK_CON4 & (BIT(22)|BIT(21))) >> 21)
 


#define sysctrl_ospi_get_clk_div()              ((SYSCTRL->CLK_CON4 & 0x1f000000) >> 24)


 enum rfadc_pll_sel {
     RF_ADC_PLL_80MHZ, 
     RF_ADC_PLL_40MHZ = 2,
     RF_ADC_PLL_20MHZ = 3,
 };
#define sysctrl_adda_cnt_pr(rfadc_pll_sel)   SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON1, BIT(25)|BIT(24), rfadc_pll_sel, 24)
#define sysctrl_apb1_clk_div(n)              SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON1, 0x00FF0000, n, 16)
#define sysctrl_apb0_clk_div(n)              SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON1, 0x0000FF00, n, 8)
#define sysctrl_mac_clk_sel(n)               SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON1, 0x000000C0, n, 6)
#define sysctrl_sys_clk_div(n)               SYSCTRL_REG_SET_VALUE(SYSCTRL->CLK_CON1, 0x0000003F, n, 0)







/* CHIP_ID */
#define sysctrl_get_chip_dcn()                     ((uint16)(SYSCTRL->CHIP_ID >> 16))
//#define sysctrl_get_chip_id()                      ((uint16)(SYSCTRL->CHIP_ID))
extern uint16 sysctrl_get_chip_id();

/* SRAM0_PD_CON */         
#define sysctrl_sram0_pd_exit_cnt(n)               SYSCTRL_REG_SET_VALUE(SYSCTRL->SRAM0_PD_CON, 0x001FF800, n, 11)
#define sysctrl_sram0_pd_enter_cnt(n)              SYSCTRL_REG_SET_VALUE(SYSCTRL->SRAM0_PD_CON, 0x000007Fe, n, 1)
#define sysctrl_sram0_hw_pd_en()                   SYSCTRL_REG_SET_BITS(SYSCTRL->SRAM0_PD_CON, BIT(0))
#define sysctrl_sram0_hw_pd_dis()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->SRAM0_PD_CON, BIT(0))


/* IO_MAP */

/* MBIST_CTRL */
#define sysctrl_get_softreset_pending()        ((SYSCTRL->MBIST_CTRL & BIT(31)))
#define sysctrl_clr_softreset_pending()        ((SYSCTRL->MBIST_CTRL = 0x80000000))


/* EFUSE_CON */
#define sysctrl_efuse_pwron_init()                 SYSCTRL_REG_SET_BITS(SYSCTRL->EFUSE_CON, BIT(1)|BIT(0))


/* SYS_ERR0 */


/* SYS_ERR1 */


/* HOSC_MNT */
#define sysctrl_hosc_mnt_high_limit(n)             SYSCTRL_REG_SET_VALUE(SYSCTRL->HOSC_MNT, 0xFFFF0000, n, 16)
#define sysctrl_hosc_mnt_en()                      SYSCTRL_REG_SET_BITS(SYSCTRL->HOSC_MNT, BIT(15))
#define sysctrl_hosc_mnt_dis()                     SYSCTRL_REG_CLR_BITS(SYSCTRL->HOSC_MNT, BIT(15))
#define sysctrl_hosc_lost_pend_clr()               SYSCTRL_REG_SET_BITS(SYSCTRL->HOSC_MNT, BIT(14))
#define sysctrl_hosc_mnt_low_limit(n)              SYSCTRL_REG_SET_VALUE(SYSCTRL->HOSC_MNT, 0x00003FFF, n, 0)

/* WK_CTRL */
/* LP_CTRL */

/* MPE0~5 */
void sysctrl_memory_protect_config(uint32 start_addr, uint32 size, uint8 enable);
#define sysctrl_get_mem_protect_pending()   ((SYSCTRL->MPE_PND[0] | SYSCTRL->MPE_PND[1] | SYSCTRL->MPE_PND[2]))

/* MODE_REG */
#define sysctrl_get_buck_cmpfb_sta()              ((SYSCTRL->MODE_REG & BIT(31)))
#define sysctrl_get_buck_dtmod_index_sta()        ((SYSCTRL->MODE_REG & BIT(30)))
#define sysctrl_get_buck_ipeaktrimout_sta()       ((SYSCTRL->MODE_REG & BIT(29)))
#define sysctrl_get_buck_irptrimout_sta()         ((SYSCTRL->MODE_REG & BIT(28)))
#define sysctrl_get_ft_mode()                     ((SYSCTRL->MODE_REG & BIT(1)))
#define sysctrl_get_cp_mode()                     ((SYSCTRL->MODE_REG & BIT(0)))


/* QSPI_MAP_CTL */    
#define sysctrl_set_qspi_iomap(iomap)             SYSCTRL_REG_OPT(SYSCTRL->QSPI_MAP_CTL = iomap;);
#define sysctrl_get_qspi_iomap()                  ((SYSCTRL->QSPI_MAP_CTL))

/* QSPI_ENCDEC_CON0 */  
#define sysctrl_get_xip_scramble_sta()            (SYSCTRL->QSPI_ENCDEC_CON0 & (BIT(31)))
#define sysctrl_xip_scramble_disable()            SYSCTRL_REG_OPT( SYSCTRL->QSPI_ENCDEC_CON0 &= ~(BIT(31)); );
#define sysctrl_xip_scramble_enable()             SYSCTRL_REG_OPT( SYSCTRL->QSPI_ENCDEC_CON0 |= (BIT(31)); );
#define sysctrl_xip_scramble_toggle()             SYSCTRL_REG_OPT( SYSCTRL->QSPI_ENCDEC_CON0 ^= (BIT(31)); );

/* TRNG */    
#define sysctrl_get_trng()                        ((SYSCTRL->TRNG))

/* DCRC_TRIM */   
#define sysctrl_set_dc_trim_init_val(n)           SYSCTRL_REG_SET_VALUE(SYSCTRL->DCRC_TRIM, 0x00fc0000, n, 18)
#define sysctrl_set_dc_trim_range(n)              SYSCTRL_REG_SET_VALUE(SYSCTRL->DCRC_TRIM, 0x0003c000, n, 14)
#define sysctrl_set_dc_trim_step(n)               SYSCTRL_REG_SET_VALUE(SYSCTRL->DCRC_TRIM, 0x00003e00, n, 9)
#define sysctrl_set_dc_trim_1us_prd(n)            SYSCTRL_REG_SET_VALUE(SYSCTRL->DCRC_TRIM, 0x000001fe, n, 1)
#define sysctrl_dc_trim_en()                      SYSCTRL_REG_SET_BITS(SYSCTRL->DCRC_TRIM, BIT(0))
#define sysctrl_dc_trim_dis()                     SYSCTRL_REG_CLR_BITS(SYSCTRL->DCRC_TRIM, BIT(0))

/* CPU1_CON0 */   
#define sysctrl_set_cpu1_pc_rst_addr(n)            SYSCTRL_REG_SET_VALUE(SYSCTRL->CPU1_CON0, 0xfffffc00, n, 10)
#define sysctrl_hclk1_pllclk_en()                  SYSCTRL_REG_SET_BITS(SYSCTRL->CPU1_CON0, BIT(7))
#define sysctrl_hclk1_pllclk_dis()                 SYSCTRL_REG_CLR_BITS(SYSCTRL->CPU1_CON0, BIT(7))
#define sysctrl_usb11_pllclk_en()                  SYSCTRL_REG_SET_BITS(SYSCTRL->CPU1_CON0, BIT(6))
#define sysctrl_usb11_pllclk_dis()                 SYSCTRL_REG_CLR_BITS(SYSCTRL->CPU1_CON0, BIT(6))
#define sysctrl_auadc_pllclk_en()                  SYSCTRL_REG_SET_BITS(SYSCTRL->CPU1_CON0, BIT(5))
#define sysctrl_auadc_pllclk_dis()                 SYSCTRL_REG_CLR_BITS(SYSCTRL->CPU1_CON0, BIT(5))
#define sysctrl_audac_pllclk_en()                  SYSCTRL_REG_SET_BITS(SYSCTRL->CPU1_CON0, BIT(4))
#define sysctrl_audac_pllclk_dis()                 SYSCTRL_REG_CLR_BITS(SYSCTRL->CPU1_CON0, BIT(4))
#define sysctrl_isp_pllclk_en()                    SYSCTRL_REG_SET_BITS(SYSCTRL->CPU1_CON0, BIT(3))
#define sysctrl_isp_pllclk_dis()                   SYSCTRL_REG_CLR_BITS(SYSCTRL->CPU1_CON0, BIT(3))
#define sysctrl_ospi_pllclk_en()                   SYSCTRL_REG_SET_BITS(SYSCTRL->CPU1_CON0, BIT(2))
#define sysctrl_ospi_pllclk_dis()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->CPU1_CON0, BIT(2))
#define sysctrl_qspi_pllclk_en()                   SYSCTRL_REG_SET_BITS(SYSCTRL->CPU1_CON0, BIT(1))
#define sysctrl_qspi_pllclk_dis()                  SYSCTRL_REG_CLR_BITS(SYSCTRL->CPU1_CON0, BIT(1))
#define sysctrl_mjpeg_pllclk_en()                  SYSCTRL_REG_SET_BITS(SYSCTRL->CPU1_CON0, BIT(0))
#define sysctrl_mjpeg_pllclk_dis()                 SYSCTRL_REG_CLR_BITS(SYSCTRL->CPU1_CON0, BIT(0))

/* CPU1_CON1 */   
#define sysctrl_dbg_stop_wdt_en()                 SYSCTRL_REG_SET_BITS(SYSCTRL->CPU1_CON1, BIT(21))
#define sysctrl_dbg_stop_wdt_dis()                SYSCTRL_REG_CLR_BITS(SYSCTRL->CPU1_CON1, BIT(21))
#define sysctrl_dbg_stop_tmr3_en()                SYSCTRL_REG_SET_BITS(SYSCTRL->CPU1_CON1, BIT(20))
#define sysctrl_dbg_stop_tmr3_dis()               SYSCTRL_REG_CLR_BITS(SYSCTRL->CPU1_CON1, BIT(20))
#define sysctrl_dbg_stop_tmr2_en()                SYSCTRL_REG_SET_BITS(SYSCTRL->CPU1_CON1, BIT(19))
#define sysctrl_dbg_stop_tmr2_dis()               SYSCTRL_REG_CLR_BITS(SYSCTRL->CPU1_CON1, BIT(19))
#define sysctrl_dbg_stop_tmr1_en()                SYSCTRL_REG_SET_BITS(SYSCTRL->CPU1_CON1, BIT(18))
#define sysctrl_dbg_stop_tmr1_dis()               SYSCTRL_REG_CLR_BITS(SYSCTRL->CPU1_CON1, BIT(18))
#define sysctrl_dbg_stop_tmr0_en()                SYSCTRL_REG_SET_BITS(SYSCTRL->CPU1_CON1, BIT(17))
#define sysctrl_dbg_stop_tmr0_dis()               SYSCTRL_REG_CLR_BITS(SYSCTRL->CPU1_CON1, BIT(17))
#define sysctrl_dbg_stop_stmr_en()                SYSCTRL_REG_SET_BITS(SYSCTRL->CPU1_CON1, BIT(16))
#define sysctrl_dbg_stop_stmr_dis()               SYSCTRL_REG_CLR_BITS(SYSCTRL->CPU1_CON1, BIT(16))
#define sysctrl_force_cld_allocate_en()           SYSCTRL_REG_SET_BITS(SYSCTRL->CPU1_CON1, BIT(8))
#define sysctrl_force_cld_allocate_dis()          SYSCTRL_REG_CLR_BITS(SYSCTRL->CPU1_CON1, BIT(8))
#define sysctrl_cpu1_softrst_self_en()            SYSCTRL_REG_CLR_BITS(SYSCTRL->CPU1_CON1, BIT(7))
#define sysctrl_cpu1_softrst_self_dis()           SYSCTRL_REG_SET_BITS(SYSCTRL->CPU1_CON1, BIT(7))
#define sysctrl_cpu1_softrst_en()                 do { SYSCTRL_REG_CLR_BITS(SYSCTRL->CPU1_CON1, BIT(2)); *(volatile unsigned int *)(WDT1_BASE + 0x4) = 0xDDDD; } while(0)
#define sysctrl_cpu1_softrst_dis()                do { SYSCTRL_REG_SET_BITS(SYSCTRL->CPU1_CON1, BIT(2)); *(volatile unsigned int *)(WDT1_BASE + 0x4) = 0xCCCC; } while(0)
#define sysctrl_cpu1_clk_en()                     SYSCTRL_REG_SET_BITS(SYSCTRL->CPU1_CON1, BIT(1))
#define sysctrl_cpu1_clk_dis()                    SYSCTRL_REG_CLR_BITS(SYSCTRL->CPU1_CON1, BIT(1))
enum cpu1_jtag_sel {
    CPU1_JTAG_DISABLE = 0,
    CPU1_JTAG_MAP1_PD12_13 = BIT(3), 
    CPU1_JTAG_MAP0_PC0_1 = BIT(0),
};
#define sysctrl_set_cpu1_jtag_map(cpu1_jtag_sel)            SYSCTRL_REG_SET_VALUE(SYSCTRL->CPU1_CON1, 0x00000009, cpu1_jtag_sel, 0)


typedef struct 
{
    __IO  uint32 CTRL_CON;     //0x00
    __IO  uint32 BURST_EN0;    //0x04
    __IO  uint32 BURST_EN1;    //0x08	
    __IO  uint32 BURST_CON0;   //0x0c
    __IO  uint32 BURST_CON1;   //0x10
    __IO  uint32 BURST_CON2;   //0x14
    __IO  uint32 BURST_CON3;   //0x18    
    __IO  uint32 BASE_CON0;    //0x1c
    __IO  uint32 BASE_CON1;    //0x20
    __IO  uint32 BASE_CON2;    //0x24
    __IO  uint32 BASE_CON3;    //0x28
    __IO  uint32 BASE_CON4;    //0x2c
    __IO  uint32 BASE_CON5;    //0x30
    __IO  uint32 BASE_CON6;    //0x34
    __IO  uint32 BASE_CON7;    //0x38
    __IO  uint32 BASE_CON8;    //0x3c
    __IO  uint32 BASE_CON9;    //0x40
    __IO  uint32 BASE_CON10;   //0x44
    __IO  uint32 BASE_CON11;   //0x48
    __IO  uint32 BASE_CON12;   //0x4c
    __IO  uint32 BASE_CON13;   //0x50
    __IO  uint32 BASE_CON14;   //0x54
    __IO  uint32 BASE_CON15;   //0x58     
    __IO  uint32 FIFO_EN0;     //0x5c
    __IO  uint32 FIFO_EN1;     //0x60    
    __IO  uint32 BW_STA_CYCLE; //0x64
    __IO  uint32 BW_STA_CNT;   //0x68
    __IO  uint32 FLASH_RD_EN0; //0x6c
    __IO  uint32 FLASH_RD_EN1; //0x70 

} SCHED_TypeDef;

#define SCHED                 ((SCHED_TypeDef    *) SCHED_BASE)


/* DMA2AHB */
enum sysctrl_dma2ahb_burst_obj {
    DMA2AHB_BURST_OBJ_QSPI, 
    DMA2AHB_BURST_OBJ_OSPI,
};

/* unit: byte, total memery is pingpong, need x2 */
enum sysctrl_dma2ahb_burst_size {
    DMA2AHB_BURST_SIZE_32, 
    DMA2AHB_BURST_SIZE_64,
    DMA2AHB_BURST_SIZE_128,
    DMA2AHB_BURST_SIZE_256,
    DMA2AHB_BURST_SIZE_0, 
};

enum sysctrl_dma2ahb_burst_chanel {
	//ch0
    DMA2AHB_BURST_CH_ROTATE_IN_RD = 0x00, 
    DMA2AHB_BURST_CH_VPP_IPF_RD ,  
    DMA2AHB_BURST_CH_OSD0_RD    , 
    DMA2AHB_BURST_CH_LMAC_RX_WR , 
    DMA2AHB_BURST_CH_LMAC_TX_RD , 
    DMA2AHB_BURST_CH_LMAC_WAVE_RD  , 
    DMA2AHB_BURST_CH_LMAC_WAVE_WR  , 
    DMA2AHB_BURST_CH_GMAC_TX_RD    , 
	
	//ch0+32
    DMA2AHB_BURST_CH_LMAC_TXHDR_REQ = 0x20+0x00,
    DMA2AHB_BURST_CH_MJPEG1_DEC_RD ,
    DMA2AHB_BURST_CH_OSD_ENC_WR    , 
    DMA2AHB_BURST_CH_M2M1_RD    ,      
    DMA2AHB_BURST_CH_AUDIO_RD    ,      
    DMA2AHB_BURST_CH_CSC_CH0_RD    ,      
	DMA2AHB_BURST_CH_CSC_CH2_WR    , 
	DMA2AHB_BURST_CH_FILL_MASK_RD  ,

	//ch8
    DMA2AHB_BURST_CH_DUAL_ORG0_WR = 0x08,
    DMA2AHB_BURST_CH_DUAL_ORG1_WR,
    DMA2AHB_BURST_CH_DUAL_ORG_RD,
    DMA2AHB_BURST_CH_GEN422_RD,
	DMA2AHB_BURST_CH_OSD_ENC_RD , 
    DMA2AHB_BURST_CH_SCALE3_Y_WR    , 
    DMA2AHB_BURST_CH_SCALE3_U_WR,
    DMA2AHB_BURST_CH_SCALE3_V_WR ,
	
	//ch8+32
    DMA2AHB_BURST_CH_MJPEG0_ENC_WR = 0x20+0x08, 
    DMA2AHB_BURST_CH_MJPEG1_ENC_SCALE2_YUV_WR  , 
    DMA2AHB_BURST_CH_M2M1_WR, 
    DMA2AHB_BURST_CH_CODEC_RD,      
    DMA2AHB_BURST_CH_CSC_CH1_RD,      
    DMA2AHB_BURST_CH_CSC_CH2_RD,     
    DMA2AHB_BURST_CH_CSC_CH0_WR,
	DMA2AHB_BURST_CH_SCALE2_IL2_RD,


	//ch16
    DMA2AHB_BURST_CH_IMX_YN_RD_SCALE3_R0_RD = 0x10,   //PARA_OY_RD
    DMA2AHB_BURST_CH_IMX_VN_RD_SCALE3_R1_RD,		  //PARA_OV_RD
    DMA2AHB_BURST_CH_VEDIOU_RD_SCALE3_B0_RD,  
    DMA2AHB_BURST_CH_IMX_YN_WR_SCREEN_Y_WR, 
    DMA2AHB_BURST_CH_VPP_Y_WR,
    DMA2AHB_BURST_CH_VPP_U_WR,
    DMA2AHB_BURST_CH_DMA2D_FG_RD_FILL_SRC_RD, 
    DMA2AHB_BURST_CH_VPP_IWM_RD,        /* FLASH & PSRAM */
	
	//ch16+32
    DMA2AHB_BURST_CH_M2M0_RD = 0x20+0x10,           /* FLASH & PSRAM */
    DMA2AHB_BURST_CH_SDHOST0_WR,
    DMA2AHB_BURST_CH_SDHOST1_WR,      //M2M2_WR
    DMA2AHB_BURST_CH_SHA_RD,      /* FLASH & PSRAM */
    DMA2AHB_BURST_CH_SYSAES_RD,      /* FLASH & PSRAM */
    DMA2AHB_BURST_CH_CRC_RD,      /* FLASH & PSRAM */
    DMA2AHB_BURST_CH_CSC_CH1_WR,
	DMA2AHB_BURST_CH_DPC_RD,
    
	//ch24
    DMA2AHB_BURST_CH_IMX_UN_RD_SCALE3_G0_RD = 0x18, //PARA_OU_RD
    DMA2AHB_BURST_CH_VEDIOY_RD_SCALE3_G1_RD,
    DMA2AHB_BURST_CH_VEDIOV_RD_SCALE3_B1_RD,
    DMA2AHB_BURST_CH_IMX_UN_WR_SCREEN_U_WR , 
    DMA2AHB_BURST_CH_IMX_VN_WR_SCREEN_V_WR  , 
    DMA2AHB_BURST_CH_GMAC_RX_WR,    
    DMA2AHB_BURST_CH_DMA2D_BG_RD_FILL_DEST_RD , 
    DMA2AHB_BURST_CH_DMA2D_OP_WR_FILL_DEST_WR  , 
	
	//ch24 +32
    DMA2AHB_BURST_CH_M2M0_WR = 0x20+0x18,
    DMA2AHB_BURST_CH_SDHOST0_RD ,
    DMA2AHB_BURST_CH_SDHOST1_RD,  //M2M2_RD
	DMA2AHB_BURST_CH_M2M2_RD = 58,
    DMA2AHB_BURST_CH_SYSAES_WR    ,      
    DMA2AHB_BURST_CH_CODEC_DMA_WR,      
    DMA2AHB_BURST_CH_GEN420_RD ,      
    DMA2AHB_BURST_CH_VPP_VDMA_WR,
	DMA2AHB_BURST_CH_SCALE2_IL1_RD,
};

/**
  * @brief CPU_ACE_FLG
  */

typedef struct
{
    __IO uint32_t CPU_ACE_KEY;
    __IO uint32_t CPU_ACE_FLG0;
    __IO uint32_t CPU_ACE_FLG1;    
} CPU_ACE_FLG_TypeDef;
#define CPU0_ACE                ((CPU_ACE_FLG_TypeDef *) 0x80001100    )  //JUST ONLY FOR CPU0 ACESS



/* CTRLCON */
int32 ll_sysctrl_dma2ahb_burst_set(enum sysctrl_dma2ahb_burst_chanel ch, enum sysctrl_dma2ahb_burst_size size) ;

__STATIC_INLINE void ll_sysctrl_dma2ahb_select(enum sysctrl_dma2ahb_burst_obj obj) {
    SCHED->CTRL_CON = (SCHED->CTRL_CON & ~(0x1UL << 0)) | ((obj & 0x1) << 0);  
}
__STATIC_INLINE void ll_sysctrl_dma2ahb_osd_enc_rd_lmac_wave_rd_priority_switch(uint8 enable) {
    if (enable) {
        SCHED->CTRL_CON |= (BIT(4));
    } else {
        SCHED->CTRL_CON &= ~(BIT(4));
    }
}

__STATIC_INLINE void ll_sysctrl_dma2ahb_pri_switch(uint32 pri) {
		SCHED->CTRL_CON &= ~(0xfff<<8);
        SCHED->CTRL_CON |= (pri<<8);
}
__STATIC_INLINE void ll_sysctrl_dma2ahb_m2m1_wr_lmac_wave_wr_priority_switch(uint8 enable) {
    if (enable) {
        SCHED->CTRL_CON |= (BIT(5));
    } else {
        SCHED->CTRL_CON &= ~(BIT(5));
    }
}

__STATIC_INLINE void ll_sysctrl_dma2ahb_m2m1_rd_gmac_tx_rd_priority_switch(uint8 enable) {
    if (enable) {
        SCHED->CTRL_CON |= (BIT(6));
    } else {
        SCHED->CTRL_CON &= ~(BIT(6));
    }
}

__STATIC_INLINE int32 ll_sysctrl_dma2ahb_is_busy(enum sysctrl_dma2ahb_burst_chanel ch) {
    return (SCHED->FIFO_EN0 & BIT(ch));
}

#define sysctrl_dma2ahb_bw_io_freq_set(hz)  do {SCHED->BW_STA_CYCLE = hz;} while (0)
#define sysctrl_dma2ahb_bw_calc_en()        do {SCHED->CTRL_CON |= BIT(1);} while (0)
#define sysctrl_dma2ahb_bw_calc_dis()       do {SCHED->CTRL_CON &= ~ BIT(1);} while (0)
#define sysctrl_get_dma2ahb_bw_byteps()     (SCHED->BW_STA_CNT)


__STATIC_INLINE void sysctrl_cpu0_l2_dcache_en(uint32 enable)
{
    SYSCTRL->SYS_KEY = enable ? 0x0fac87e4 : (~0x0fac87e4);
}

__STATIC_INLINE void sysctrl_cpu1_l2_dcache_en(uint32 enable)
{
    SYSCTRL->SYS_KEY = enable ? 0xf1ac87e4 : (~0xf1ac87e4);
}

#define sysctrl_get_cpu0_l2_dcache_en()                     ((uint16)(SYSCTRL->SYS_KEY & BIT(8)))
#define sysctrl_get_cpu1_l2_dcache_en()                     ((uint16)(SYSCTRL->SYS_KEY & BIT(9)))


#define SYSCTRL_GET_CPU0_L2_DCACHE_EN                     (SYSCTRL->SYS_KEY & BIT(8))
#define SYSCTRL_GET_CPU1_L2_DCACHE_EN                     (SYSCTRL->SYS_KEY & BIT(9))
#define SYSCTRL_GET_CPU_DCACHE_SHARE_EN                   ((SYSCTRL->SYS_KEY & 0x300) == 0x300)

#define SYSCTRL_GET_DCACHE_LINE_SIZE                      (32)

/* soft int not use */
#define sysctrl_cpu0_kick_cpu1_softint()                  do { *((volatile uint32 *)(0x80001200)) = 1; } while (0)
#define sysctrl_cpu1_kick_cpu0_softint()                  do { *((volatile uint32 *)(0x80000204)) = 1; } while (0)
#define sysctrl_clr_cpu0_softint_pending()                do { *((volatile uint32 *)(0x80000204)) = 1; } while (0)
#define sysctrl_clr_cpu1_softint_pending()                do { *((volatile uint32 *)(0x80001200)) = 1; } while (0)
#define SYSCTRL_GET_CPU0_SOFTINT_PENDING                  (*(volatile uint32 *)(0x80000204) & 1)
#define SYSCTRL_GET_CPU1_SOFTINT_PENDING                  (*(volatile uint32 *)(0x80001200) & 1)


enum cpu_burst_size_def {
    CPU_BURST_SIZE_0B, 
    CPU_BURST_SIZE_16B, 
    CPU_BURST_SIZE_32B, // only for dbus, not use this
    CPU_BURST_SIZE_00B,
};
#define sysctrl_cpu0_ibus_burst_set(cpu_burst_size_def)  SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON9, BIT(17)|BIT(16), cpu_burst_size_def, 16);
#define sysctrl_cpu0_dbus_burst_set(cpu_burst_size_def)  SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON9, BIT(19)|BIT(18), cpu_burst_size_def, 18);
#define sysctrl_cpu1_ibus_burst_set(cpu_burst_size_def)  SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON10, BIT(17)|BIT(16), cpu_burst_size_def, 16);
#define sysctrl_cpu1_dbus_burst_set(cpu_burst_size_def)  SYSCTRL_REG_SET_VALUE(SYSCTRL->SYS_CON10, BIT(19)|BIT(18), cpu_burst_size_def, 18);
#define sysctrl_cpu0_ibus_burst_get()  (SYSCTRL->SYS_CON9 & (BIT(17)|BIT(16)) >> 16)
#define sysctrl_cpu0_dbus_burst_get()  (SYSCTRL->SYS_CON9 & (BIT(19)|BIT(18)) >> 18)
#define sysctrl_cpu1_ibus_burst_get()  (SYSCTRL->SYS_CON10 & (BIT(17)|BIT(16)) >> 16)
#define sysctrl_cpu1_dbus_burst_get()  (SYSCTRL->SYS_CON10 & (BIT(19)|BIT(18)) >> 18)


enum peris_access_ace_def {
    ACE_BASEBAND1       = BIT(0),
    ACE_BASEBAND        = BIT(1),
    ACE_USB20           = BIT(2),
    ACE_GMAC            = BIT(3),
    ACE_MIX_TOP         = BIT(4),  //SDHOST0,SDHOST1,SYS_CTRL,M2M_DMA0,M2M_DMA1,SYS_MONITOR
    ACE_GPIO_TOP        = BIT(5),
    ACE_EFUSE_CTRL      = BIT(6),
    ACE_DISPLAY_TOP     = BIT(7), //[SPI0,SPI1,SCALE0,SCALE2,SCALE3,LCDC]
    ACE_VIDEO_INTF_TOP  = BIT(8), //[DUAL_ORG,DVP0,DVP1,GEN420,GEN422,MIPI_CSI0,MIPI_CSI1,MIPI_DSI,PARA_IN,PARA_OUT]
    ACE_VIDEO_SUBSYS_TOP= BIT(9), //[MJPEG0,MJPEG1,OF,PRC,VPP]
    ACE_OSPI_CTRL       = BIT(10),
    ACE_QSPI_CTRL       = BIT(11),
    ACE_H264_CODEC_TOP  = BIT(12),
    ACE_SYS_SEC_TOP     = BIT(13),//[CRC,SHA,SYS_AES] 
    ACE_IMAGE_ISP_TOP   = BIT(14),
    ACE_PMU             = BIT(15),
    ACE_RFDIGITAL       = BIT(16),
    ACE_USB11           = BIT(17),
    ACE_VIDEO_GPU_TOP   = BIT(18),//[CSC,DMA2D,GPU2D,OSDENC,PALETTE_ACC]
    ACE_RFDIGCAL_TOP    = BIT(19),
    ACE_NPU_TOP         = BIT(20),
};
void sysctrl_ace_peris_access_cpu0(uint32 peris_access_ace_def) ;
void sysctrl_ace_peris_access_cpu1(uint32 peris_access_ace_def) ;
void sysctrl_ace_peris_access_cpu_all(uint32 peris_access_ace_def) ;
uint32 sysctrl_get_cpu_id(void) ;


enum cpu1_debug_io_def {
    CPU1_DEBUG_IO_NONE,
    CPU1_DEBUG_IO_PA5_PA4 = BIT(0), 
    CPU1_DEBUG_IO_PD12_PD13= BIT(3),
};
#define sysctrl_cpu1_debug_io_set(cpu1_debug_io_def)    SYSCTRL_REG_OPT (SYSCTRL->CPU1_CFG1 = (SYSCTRL->CPU1_CFG1 & ~ (BIT(0)|BIT(3))) | cpu1_debug_io_def);




/* ic SIM USE */
#define IC_SIM_EN           0
#if IC_SIM_EN
//#define IC_SIM_START()      do {CRC-> CRC_INIT = 0x11223344; CRC-> CRC_INIT = 0x55667788;} while(0)
#define IC_SIM_START()      do { *((int volatile*)0x40012004) = 0x11223344; *((int volatile*)0x40012004) = 0x55667788;} while(0);
#define IC_SIM_END_OK()     do { *((int32*)0x400FFFFC) = 0x88888888; } while(0)
#define IC_SIM_END_NG()     do { *((int32*)0x400FFFFC) = 0xDEADDEAD; } while(0)
#else
#define IC_SIM_START()      
#define IC_SIM_END_OK()     
#define IC_SIM_END_NG()     
#endif


/**
  * @}
  */



int32 sys_get_apbclk(void);
int32 sys_get_sysclk(void);
int32 sys_set_sysclk(uint32 system_clk);

void mcu_reset(void);
int32 mcu_watchdog_timeout(uint8 tmo_sec);
void mcu_watchdog_timeout_level(uint8 level);
void mcu_watchdog_feed(void);
uint32 mcu_watchdog_static(uint8 pinrtf_en);
void mcu_watchdog_irq_request(void *hdl);

int32 pmu_watchdog_timeout(uint8 tmo_sec);
uint32 pmu_watchdog_static(uint8 pinrtf_en);
void pmu_watchdog_feed(void);

#define FUNCTION_MAYBE_COST_TIME(func, time_s) \
    do { uint8 tmo_bak = *(volatile unsigned int *)(WDT_BASE) & 0xF; \
        mcu_watchdog_timeout(time_s << 1); \
        func; \
        mcu_watchdog_timeout_level(tmo_bak); } while (0)

void system_enter_sleep(void);
void system_exit_sleep(void);

void sysctrl_cmu_init(void);
bool sysctrl_cmu_upll_init(void);
bool sysctrl_cmu_sysclk_set(uint32 clk, bool is_upll);
#define sysctrl_gmac_clk_sel_clk_from_io()


/**
 * @brief   system_clock_set
 * @param   clk_hz : clk wish to be set
 * @retval  OK or ERR
 * @note    
 */
int32 system_clock_set(uint32 clk_hz);
/**
* @brief   system_clock_get
* @retval  system clk_hz
* @note    this function will get the clock form sotfware temp storage
*/
uint32 system_clock_get(void);

/**
 * @brief   system_clock_get_refresh
 * @retval  system clk_hz
 * @note    this function will get the clock form hardware
 */
uint32 system_clock_get_refresh(void);

/**
 * @brief   peripheral_clock_set
 * @param   peripheral  : HG_Peripheral_Type  
 * @param   clk_hz      : Peripheral clk_hz
 * @retval  ok or err
 * @note    peripheral on same bus apb0/1 clk must be same
 */
int32 peripheral_clock_set(HG_Peripheral_Type peripheral, uint32 clk_hz);

/**
 * @brief   peripheral_clock_get
 * @param   peripheral  : HG_Peripheral_Type  
 * @retval  clk_hz
 */
uint32 peripheral_clock_get(HG_Peripheral_Type peripheral);

uint32 sysctrl_efuse_get_chip_uuid(uint8* pbuf, uint32 len);

uint32 sysctrl_efuse_validity_get(void);

void sysctrl_efuse_validity_handle(void);
void sysctrl_efuse_info_show(void);

uint32 sysctrl_efuse_vddi_get(void);

uint32 sysctrl_efuse_mipi_csi_get(uint8 need_check_crc);

uint32 sysctrl_efuse_tsensor_get(void);

uint32 sysctrl_efuse_pmu_tsensor_get(void);

uint32 sysctrl_efuse_vddi_gears_get(void);

uint32 sysctrl_efuse_adda_vref_get(void);

uint32 sysctrl_efuse_saradc_gain(void);

uint32 sysctrl_efuse_saradc_offset(void);

uint32 sysctrl_efuse_aubias_sel_vcc27au_vdd(void);

void sysctrl_rst_lmac_phy(void);

uint32 sysctrl_efuse_audio_en(void);

int32 sysctrl_err_resp_disable(void);
uint8 sysctrl_efuse_get_bios_id(void);
uint8 sysctrl_efuse_get_module_type(void);
void system_clocks_show(void);
int ll_qspi_clock_check();

void system_goto_boot(void);
void system_reboot_test_mode(void);
void system_reboot_normal_mode(void);
void system_mclr_soft_en(void);

__STATIC_INLINE int32 system_is_wifi_test_mode(void)
{
    return !!(PMU->PMUCON7 & BIT(21));
}

#define sysctrl_dma_bridge_reset()


#ifdef __cplusplus
}
#endif

#endif //__SYSCTRL_H__

/**
  * @}
  */

/**
  * @}
  */

/******************* (C) COPYRIGHT 2019 HUGE-IC *****END OF FILE****/
