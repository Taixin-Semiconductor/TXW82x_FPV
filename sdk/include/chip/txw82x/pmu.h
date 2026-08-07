/**
  ******************************************************************************
  * @file    pmu.h
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
  * V1.0.0  2021.01.14  First Release, move from sysctrl
  *
  ******************************************************************************
  */

#ifndef __PMU_H__
#define __PMU_H__

#include "typesdef.h"

#ifdef __cplusplus
 extern "C" {
#endif

#define CORE_LVD_CON      (0x00>>0)
#define CORE_LVD_CON1     (0x04>>0)
#define CORE_LVD_CON2     (0x08>>0)
#define CORE_PMUCON0      (0x10>>0)
#define CORE_PMUCON1      (0x14>>0)
#define CORE_PMUCON2      (0x18>>0)
#define CORE_PMUCON3      (0x1c>>0)
#define CORE_PMUCON4      (0x20>>0)
#define CORE_PMUCON5      (0x24>>0)
#define CORE_RFCON0       (0x28>>0)
#define CORE_RFCON1       (0x2c>>0)
#define CORE_PMUCON6      (0x30>>0)
#define CORE_PMUCON7      (0x34>>0)
#define CORE_PMUCON8      (0x38>>0)
#define CORE_PMUCON9      (0x3c>>0)
#define CORE_PMUCON10     (0x40>>0)
#define CORE_PMUCON11     (0x44>>0)
#define CORE_PMUCON12 	(0x48>>0)
#define CORE_PMUCON13 	(0x4c>>0)
#define CORE_PNDCLR  	(0x50>>0)

//-------------RTC part1 start---------------
#define CORE_RTCWPR       (0x54>>0)	 //54
#define CORE_RTCCTR       (0x58>>0)  //58 
#define CORE_RTCPDR       (0x5c>>0)  //5c 
#define CORE_RTCISR       (0x60>>0)  //60 
#define CORE_RTCSSR       (0x64>>0)  //64 
#define CORE_RTCTMR       (0x68>>0)  //68 
#define CORE_RTCDAR       (0x6c>>0)  //6c 
#define CORE_RTCALR0R     (0x70>>0)  //70 
#define CORE_RTCWUTR      (0x74>>0)  //74 
#define CORE_RTCTSSSR     (0x78>>0)  //78 
#define CORE_RTCTSTMR     (0x7c>>0)  //7c
//-------------RTC part1 end---------------
#define CORE_TMRCON       (0x80>>0)
#define CORE_TMRPR        (0x84>>0)
#define CORE_TMRRPT       (0x88>>0)
#define CORE_TMRTRIM      (0x8c>>0)
#define CORE_TMRPR2       (0x90>>0)
//-------------RTC part2 start---------------
//#define CORE_TMRRTCC (0x94>>0)
#define CORE_RTCTSDAR     (0x94>>0)  //94 
#define CORE_RTCCALR      (0x98>>0)  //98 
#define CORE_RTCSSSR      (0x9c>>0)  //9c 
//-------------RTC part2 start---------------

#define CORE_WKCON   (0xa0>>0)
#define CORE_WKCON1  (0xa4>>0)
#define CORE_WKCON2  (0xa8>>0)
#define CORE_SECCON  (0xb0>>0)
#define CORE_SECDAT  (0xb4>>0)
#define CORE_SECADR  (0xb8>>0)
#define CORE_SECKEY  (0xbc>>0)
#define CORE_CMPCON  (0xc0>>0)
#define CORE_CMPCON1 (0xc4>>0)

#if 0 //vernus

#define CORE_TMR0CON      (0xd0>>0)
#define CORE_TMR1CON      (0xd4>>0)
#define CORE_TMR2CON      (0xd8>>0)
#define CORE_TMR3CON      (0xdc>>0)

#else //vernus200= eagle v2

#define CORE_PMUCON14  (0xc8>>0)
#define CORE_PMUCON15  (0xcc>>0)

#define CORE_TMR_CON  (0xd0>>0)
#define CORE_TMR0CON  (0xd4>>0)
#define CORE_TMR1CON  (0xd8>>0)
#define CORE_TMR2CON  (0xdc>>0)
#define CORE_TMR3CON  (0xe0>>0)

#define CORE_WDTCON   (0xe4>>0)
#define CORE_WDTKEY   (0xe8>>0)
#endif
typedef struct
{
    __IO uint32 LVD_CON;
    __IO uint32 LVD_CON1;
    __IO uint32 LVD_CON2;
    __IO uint32 TMRAOCNTL;
    __IO uint32 PMUCON0;
    __IO uint32 PMUCON1;
    __IO uint32 PMUCON2;
    __IO uint32 PMUCON3;
    __IO uint32 PMUCON4;
    __IO uint32 PMUCON5;
    __IO uint32 RFCON0;
    __IO uint32 RFCON1;
    __IO uint32 PMUCON6;
    __IO uint32 PMUCON7;
    __IO uint32 PMUCON8;
    __IO uint32 PMUCON9;  
    __IO uint32 PMUCON10;                          //40
    __IO uint32 PMUCON11;
    __IO uint32 PMUCON12;
    __IO uint32 PMUCON13;
    __IO uint32 PNDCLR;                            //50
    __IO uint32 RTCWPR;                 //54                     
    __IO uint32 RTCCTR;                 //58                        
    __IO uint32 RTCPDR;                 //5c                           
    __IO uint32 RTCISR;                 //60                       
    __IO uint32 RTCSSR;                 //64                      
    __IO uint32 RTCTMR;                 //68                      
    __IO uint32 RTCDAR;                 //6c                    
    __IO uint32 RTCALR0R;               //70                                           
    __IO uint32 RTCWUTR;                //74                     
    __IO uint32 RTCTSSSR;               //78                
    __IO uint32 RTCTSTMR;               //7c     
    __IO uint32 TMRCON;                 //80
    __IO uint32 TMRPR;
    __IO uint32 TMRRPT;
    __IO uint32 TMRTRIM;
    __IO uint32 TMRPR2;                 //90
    __IO uint32 RTCTSDAR;               //94                          
    __IO uint32 RTCCALR;                //98                      
    __IO uint32 RTCSSSR;                //9c
    __IO uint32 WKCON;                             //a0
    __IO uint32 WKCON1;
    __IO uint32 WKCON2;
    __IO uint32 TMRAOCNTH;
    __IO uint32 SECCON;                            //b0
    __IO uint32 SECDAT;
    __IO uint32 SECADDR;
    __IO uint32 SECKEY;
    __IO uint32 CMPCON;                            //c0
    __IO uint32 CMPCON1;                           //c4                 
    __IO uint32 PMUCON14;                          //c8                 
    __IO uint32 PMUCON15;                          //cc                                
    __IO uint32 TMR_CON;                           //d0               
    __IO uint32 TMR0CON0;                          //d4               
    __IO uint32 TMR1CON0;                          //d8           
    __IO uint32 TMR2CON0;                          //dc           
    __IO uint32 TMR3CON0;                          //e0
    __IO uint32 WDTCON;                            //e4
    __IO uint32 WDTKEY;                            //e8
    __IO uint32 PMUCON16;                          //ec
    __IO uint32 RTC_VCCHVD_CON;                    //f0
    __IO uint32 RTC_VCCHVD_PND;                    //f4
    
} PMU_TypeDef;

#define PMU               ((PMU_TypeDef *) PMU_BASE)

#define PMU_REG_SET_BITS(reg, bits)                     do { pmu_reg_write((uint32)&reg, (reg)|(bits)); } while(0)
#define PMU_REG_CLR_BITS(reg, bits)                     do { pmu_reg_write((uint32)&reg, (reg) & ~(bits)); } while(0)
#define PMU_REG_SET_VALUE(reg, bit_mask, val, bit_pos)  do { pmu_reg_write((uint32)&reg, ((reg) & ~(bit_mask)) | (((val) << (bit_pos)) & (bit_mask)) ); } while(0)
#define PMU_REG_BIT_FUN(fun_name, reg, bits)\
__STATIC_INLINE void fun_name##_en(void) { PMU_REG_SET_BITS(reg, bits); }\
__STATIC_INLINE void fun_name##_dis(void) { PMU_REG_CLR_BITS(reg, bits); }


/* lvdcon */
#define pmu_vcam_oc_int_en()        PMU_REG_CLR_BITS(PMU->LVD_CON, BIT(31))
#define pmu_vcam_oc_int_dis()       PMU_REG_SET_BITS(PMU->LVD_CON, BIT(31))
#define PMU_VCAM_OC_PENDING         (PMU->LVD_CON & BIT(30))
#define PMU_V15_OC_PENDING          (PMU->LVD_CON & BIT(29))
#define PMU_VDD_OC_PENDING          (PMU->LVD_CON & BIT(28))
#define PMU_VDD_LV_PENDING          (PMU->LVD_CON & BIT(27))
#define PMU_VCC_LV_PENDING          (PMU->LVD_CON & BIT(26))
#define PMU_VCC1_LV_PENDING         (PMU->LVD_CON & BIT(25)) //?
#define PMU_VCC15_LV_PENDING        (PMU->LVD_CON & BIT(24))
#define pmu_vcc1_lv_int_en()        PMU_REG_CLR_BITS(PMU->LVD_CON, BIT(23))
#define pmu_vcc1_lv_int_dis()       PMU_REG_SET_BITS(PMU->LVD_CON, BIT(23))
#define pmu_vcc15_oc_int_en()       PMU_REG_CLR_BITS(PMU->LVD_CON, BIT(22))
#define pmu_vcc15_oc_int_dis()      PMU_REG_SET_BITS(PMU->LVD_CON, BIT(22))
#define pmu_lvd_oe_en()             PMU_REG_SET_BITS(PMU->LVD_CON, BIT(21))
#define pmu_lvd_oe_dis()            PMU_REG_CLR_BITS(PMU->LVD_CON, BIT(21))
#define pmu_vdd_oc_int_en()         PMU_REG_CLR_BITS(PMU->LVD_CON, BIT(20))
#define pmu_vdd_oc_int_dis()        PMU_REG_SET_BITS(PMU->LVD_CON, BIT(20))
#define pmu_vdd_lv_reset_en()       PMU_REG_SET_BITS(PMU->LVD_CON, BIT(19))
#define pmu_vdd_lv_reset_dis()      PMU_REG_CLR_BITS(PMU->LVD_CON, BIT(19))
#define pmu_vcc_lv_reset_en()       PMU_REG_SET_BITS(PMU->LVD_CON, BIT(18))
#define pmu_vcc_lv_reset_dis()      PMU_REG_CLR_BITS(PMU->LVD_CON, BIT(18))
#define pmu_vcc15_lv_reset_en()     PMU_REG_SET_BITS(PMU->LVD_CON, BIT(17))
#define pmu_vcc15_lv_reset_dis()    PMU_REG_CLR_BITS(PMU->LVD_CON, BIT(17))

enum vcc15_oc_level {
    VCC15_OC_200MA = 0,
    VCC15_OC_90MA = 0x3,
    VCC15_OC_70MA,
    VCC15_OC_60MA,
    VCC15_OC_50MA,
};
#define pmu_vcc15_oc_level_set(vcc15_oc_level)      PMU_REG_SET_VALUE(PMU->LVD_CON, 0x0001c000, vcc15_oc_level, 14)

enum vdd_oc_level {
    VDD_OC_50MA,
    VDD_OC_100MA,
    VDD_OC_150MA,
    VDD_OC_200MA,
};
#define pmu_vdd_oc_level_set(vdd_oc_level)          PMU_REG_SET_VALUE(PMU->LVD_CON, 0x00003000, vdd_oc_level, 12)

enum vcc15_lv_level {
    VCC15_LV_0V86_T_0V90,
    VCC15_LV_1V00_T_1V05,
    VCC15_LV_1V14_T_1V19,
    VCC15_LV_1V29_T_1V35,

};
#define pmu_vcc15_lv_level_set(vcc15_lv_level)      PMU_REG_SET_VALUE(PMU->LVD_CON, 0x00000c00, vcc15_lv_level, 10)

enum vdd_lv_level {
    VDD_LV_0V64_T_0V68,
    VDD_LV_0V74_T_0V78,
    VDD_LV_0V84_T_0V88,
    VDD_LV_0V84_T_0V98,
};
#define pmu_vdd_lv_level_set(vdd_lv_level)          PMU_REG_SET_VALUE(PMU->LVD_CON, 0x00000300, vdd_lv_level, 8)

enum avcc_lv_level {
    AVCC_LV_1V90_T_1V99,
    AVCC_LV_2V06_T_2V16,
    AVCC_LV_2V21_T_2V31,
    AVCC_LV_2V33_T_2V44,
    AVCC_LV_2V46_T_2V58,
    AVCC_LV_2V62_T_2V74,
    AVCC_LV_2V75_T_2V87,
    AVCC_LV_2V90_T_3V04,
};
#define pmu_avcc_lv_level_set(avcc_lv_level)        PMU_REG_SET_VALUE(PMU->LVD_CON, 0x000000e0, avcc_lv_level, 5)

enum vcc1_lv_level {
    VCC1_LV_2V54_T_2V66,
    VCC1_LV_1V46_T_1V53,
};
#define pmu_vcc1_lv_level_set(vcc1_lv_level)        PMU_REG_SET_VALUE(PMU->LVD_CON, 0x00000010, vcc1_lv_level, 4)

#define pmu_vdd_lv_detect_en()                      PMU_REG_SET_BITS(PMU->LVD_CON, BIT(3))
#define pmu_vdd_lv_detect_dis()                     PMU_REG_CLR_BITS(PMU->LVD_CON, BIT(3))
#define pmu_avcc_lv_detect_en()                     PMU_REG_SET_BITS(PMU->LVD_CON, BIT(2))
#define pmu_avcc_lv_detect_dis()                    PMU_REG_CLR_BITS(PMU->LVD_CON, BIT(2))
#define pmu_vcc1_lv_detect_en()                     PMU_REG_SET_BITS(PMU->LVD_CON, BIT(1))
#define pmu_vcc1_lv_detect_dis()                    PMU_REG_CLR_BITS(PMU->LVD_CON, BIT(1))
#define pmu_vcc15_lv_detect_en()                    PMU_REG_SET_BITS(PMU->LVD_CON, BIT(0))
#define pmu_vcc15_lv_detect_dis()                   PMU_REG_CLR_BITS(PMU->LVD_CON, BIT(0))

/* lvdcon1 */
#define PMU_VDD18_OC_PENDING                        (PMU->LVD_CON1 & BIT(31))
#define PMU_VDD12_OC_PENDING                        (PMU->LVD_CON1 & BIT(30))
#define pmu_vcc_lv_detect_sync_en()                 PMU_REG_CLR_BITS(PMU->LVD_CON1, BIT(30))
#define pmu_vcc_lv_detect_sync_dis()                PMU_REG_SET_BITS(PMU->LVD_CON1, BIT(30))
#define pmu_vdd_lv_detect_filter_en()               PMU_REG_CLR_BITS(PMU->LVD_CON1, BIT(29))
#define pmu_vdd_lv_detect_filter_dis()              PMU_REG_SET_BITS(PMU->LVD_CON1, BIT(29))
#define pmu_vcc_lv_detect_filter_en()               PMU_REG_CLR_BITS(PMU->LVD_CON1, BIT(28))
#define pmu_vcc_lv_detect_filter_dis()              PMU_REG_SET_BITS(PMU->LVD_CON1, BIT(28))
#define pmu_vdd_oc_lv_ldetect_filter_set(level)     PMU_REG_SET_VALUE(PMU->LVD_CON1, 0x0FE00000, level, 21)
#define pmu_vdd_oc_lv_ldetect_filter_set(level)     PMU_REG_SET_VALUE(PMU->LVD_CON1, 0x0FE00000, level, 21)
#define pmu_vdd_oc_lv_hdetect_filter_set(level)     PMU_REG_SET_VALUE(PMU->LVD_CON1, 0x01FC000, level, 14)
#define pmu_vdd_oc_lv_hdetect_filter_set(level)     PMU_REG_SET_VALUE(PMU->LVD_CON1, 0x01FC000, level, 14)
#define pmu_vcc_oc_lv_ldetect_filter_set(level)     PMU_REG_SET_VALUE(PMU->LVD_CON1, 0x00003f80, level, 7)
#define pmu_vcc_oc_lv_ldetect_filter_set(level)     PMU_REG_SET_VALUE(PMU->LVD_CON1, 0x00003f80, level, 7)
#define pmu_vcc_oc_lv_hdetect_filter_set(level)     PMU_REG_SET_VALUE(PMU->LVD_CON1, 0x0000007f, level, 0)
#define pmu_vcc_oc_lv_hdetect_filter_set(level)     PMU_REG_SET_VALUE(PMU->LVD_CON1, 0x0000007f, level, 0)

/* lvdcon2 */
#define pmu_vdd12_oc_int_en()                       PMU_REG_CLR_BITS(PMU->LVD_CON2, BIT(31))
#define pmu_vdd12_oc_int_dis()                      PMU_REG_SET_BITS(PMU->LVD_CON2, BIT(31))
#define pmu_vdd18_oc_int_en()                       PMU_REG_CLR_BITS(PMU->LVD_CON2, BIT(30))
#define pmu_vdd18_oc_int_dis()                      PMU_REG_SET_BITS(PMU->LVD_CON2, BIT(30))
#define pmu_vcc1_lv_detect_filter_en()              PMU_REG_CLR_BITS(PMU->LVD_CON2, BIT(29))
#define pmu_vcc1_lv_detect_filter_dis()             PMU_REG_SET_BITS(PMU->LVD_CON2, BIT(29))
#define pmu_vcc15_lv_detect_filter_en()             PMU_REG_CLR_BITS(PMU->LVD_CON2, BIT(28))
#define pmu_vcc15_lv_detect_filter_dis()            PMU_REG_SET_BITS(PMU->LVD_CON2, BIT(28))
#define pmu_vcc1_oc_lv_ldetect_filter_set(level)    PMU_REG_SET_VALUE(PMU->LVD_CON2, 0x0FE00000, level, 21)
#define pmu_vcc1_oc_lv_ldetect_filter_set(level)    PMU_REG_SET_VALUE(PMU->LVD_CON2, 0x0FE00000, level, 21)
#define pmu_vcc1_oc_lv_hdetect_filter_set(level)    PMU_REG_SET_VALUE(PMU->LVD_CON2, 0x001FC000, level, 14)
#define pmu_vcc1_oc_lv_hdetect_filter_set(level)    PMU_REG_SET_VALUE(PMU->LVD_CON2, 0x001FC000, level, 14)
#define pmu_vcc15_oc_lv_ldetect_filter_set(level)   PMU_REG_SET_VALUE(PMU->LVD_CON2, 0x00003f80, level, 7)
#define pmu_vcc15_oc_lv_ldetect_filter_set(level)   PMU_REG_SET_VALUE(PMU->LVD_CON2, 0x00003f80, level, 7)
#define pmu_vcc15_oc_lv_hdetect_filter_set(level)   PMU_REG_SET_VALUE(PMU->LVD_CON2, 0x0000007f, level, 0)
#define pmu_vcc15_oc_lv_hdetect_filter_set(level)   PMU_REG_SET_VALUE(PMU->LVD_CON2, 0x0000007f, level, 0)

/* CORE_PMUCON0 */
#define pmu_ftmclr_en()                             PMU_REG_SET_BITS(PMU->PMUCON0, BIT(31))
#define pmu_ftmclr_dis()                            PMU_REG_CLR_BITS(PMU->PMUCON0, BIT(31))
#define pmu_port_wk_auto_dis_en()                   PMU_REG_SET_BITS(PMU->PMUCON0, BIT(29))
#define pmu_port_wk_auto_dis_dis()                  PMU_REG_CLR_BITS(PMU->PMUCON0, BIT(29))
#define pmu_mclr_soft_en()                          PMU_REG_SET_BITS(PMU->PMUCON0, BIT(28))
#define pmu_mclr_soft_dis()                         PMU_REG_CLR_BITS(PMU->PMUCON0, BIT(28))
#define pmu_led_io_mux_en()                         PMU_REG_CLR_BITS(PMU->PMUCON0, BIT(27))
#define pmu_led_io_mux_dis()                        PMU_REG_SET_BITS(PMU->PMUCON0, BIT(27))
enum pd_tmr1_clk_sel {
    PD_TMR1_CLK_RC128K,
    PD_TMR1_CLK_XOSC,
    PD_TMR1_CLK_RC8M,
    PD_TMR1_CLK_LX32K,
};
#define pmu_pd_tmr1_clk_sel(pd_tmr1_clk_sel)        PMU_REG_SET_VALUE(PMU->PMUCON0, 0x06000000, pd_tmr1_clk_sel, 25)

#define pmu_pd_mem2_en()                            PMU_REG_SET_BITS(PMU->PMUCON0, BIT(24))
#define pmu_pd_mem2_dis()                           PMU_REG_CLR_BITS(PMU->PMUCON0, BIT(24))
#define pmu_fls_pg_en()                             PMU_REG_SET_BITS(PMU->PMUCON0, BIT(23))
#define pmu_fls_pg_dis()                            PMU_REG_CLR_BITS(PMU->PMUCON0, BIT(23))
#define pmu_pd_mem32k_en()                          PMU_REG_SET_BITS(PMU->PMUCON0, BIT(22))
#define pmu_pd_mem32k_dis()                         PMU_REG_CLR_BITS(PMU->PMUCON0, BIT(22))
#define pmu_pd_ulp_en()                             PMU_REG_SET_BITS(PMU->PMUCON0, BIT(21))
#define pmu_pd_ulp_dis()                            PMU_REG_CLR_BITS(PMU->PMUCON0, BIT(21))
#define pmu_pd_io_latch_en()                        PMU_REG_SET_BITS(PMU->PMUCON0, BIT(20))
#define pmu_pd_io_latch_dis()                       PMU_REG_CLR_BITS(PMU->PMUCON0, BIT(20))
#define pmu_pd_rf_latch_en()                        PMU_REG_CLR_BITS(PMU->PMUCON0, BIT(19))
#define pmu_pd_rf_latch_dis()                       PMU_REG_SET_BITS(PMU->PMUCON0, BIT(19))
#define pmu_pd_osc_en()                             PMU_REG_SET_BITS(PMU->PMUCON0, BIT(18))
#define pmu_pd_osc_dis()                            PMU_REG_CLR_BITS(PMU->PMUCON0, BIT(18))
#define pmu_pd_osc_hw_trim_en()                     PMU_REG_SET_BITS(PMU->PMUCON0, BIT(17))
#define pmu_pd_osc_hw_trim_dis()                    PMU_REG_CLR_BITS(PMU->PMUCON0, BIT(17))
enum pd_tmr_clk_sel {
    PD_TMR_CLK_RC128K,
    PD_TMR_CLK_XOSC,
    PD_TMR_CLK_RC32K,
    PD_TMR_CLK_LX32K,
};
#define pmu_pd_tmr_clk_sel(pd_tmr_clk_sel)          PMU_REG_SET_VALUE(PMU->PMUCON0, 0x00018000, pd_tmr_clk_sel, 15)
#define pmu_io_clk_en()                             PMU_REG_SET_BITS(PMU->PMUCON0, BIT(14))
#define pmu_io_clk_dis()                            PMU_REG_CLR_BITS(PMU->PMUCON0, BIT(14))
#define pmu_pd_wdt_en()                             PMU_REG_SET_BITS(PMU->PMUCON0, BIT(11))
#define pmu_pd_wdt_dis()                            PMU_REG_CLR_BITS(PMU->PMUCON0, BIT(11))

enum lpvdd_vol_level {
    LPVDD_VOL_0V74,
    LPVDD_VOL_0V81,
    LPVDD_VOL_0V88,
    LPVDD_VOL_0V95,
    LPVDD_VOL_1V02,
    LPVDD_VOL_1V09,
    LPVDD_VOL_1V16,
    LPVDD_VOL_1V23,
};
#define pmu_lpvdd_vol_set(lpvdd_vol_level)          PMU_REG_SET_VALUE(PMU->PMUCON0, 0x00000700, lpvdd_vol_level, 8)


/* CORE_PMUCON1 */
/* CORE_PMUCON2 */
/* CORE_PMUCON3 */
/* CORE_PMUCON4 */
#define pmu_vdd_en()                                    PMU_REG_SET_BITS(PMU->PMUCON4, BIT(9))
#define pmu_vdd_dis()                                   PMU_REG_CLR_BITS(PMU->PMUCON4, BIT(9))

/* CORE_PMUCON5 */
enum vdd15_vol_level {
    VDD15_VOL_1V2,
    VDD15_VOL_1V3,
    VDD15_VOL_1V4,
    VDD15_VOL_1V5,
    VDD15_VOL_1V6,
    VDD15_VOL_1V7,
    VDD15_VOL_1V8,
    VDD15_VOL_1V9,
    VDD15_VOL_2V0,
    VDD15_VOL_2V1,
    VDD15_VOL_2V2,
    VDD15_VOL_2V3,
    VDD15_VOL_2V4,
    VDD15_VOL_2V5,
};
#define pmu_vdd15_vol_set(vdd15_vol_level)          PMU_REG_SET_VALUE(PMU->PMUCON5, 0xF, vdd15_vol_level, 0)


#define pmu_hirc_8mhz_en()                         PMU_REG_SET_BITS(PMU->PMUCON5, BIT(24))
#define pmu_hirc_8mhz_dis()                        PMU_REG_CLR_BITS(PMU->PMUCON5, BIT(24))


/* CORE_PMUCON7 */
enum pmu_con7_bits_funcs {
    PMU_BOOT_DIRECT_RUN,            //0
    PMU_APP_DEADCODE_PENDING,
    PMU_CP_FAIL_PENDING,
    PMU_BOOT_CODE_POS,
    
    PMU_DIRECT_RUN_DISABLE_SIGN,    //4
    PMU_SKIP_MASTER_BOOT_SIGN,
    PMU_SKIP_TEST_MODE_SIGN,
    PMU_SKIP_PSRAM_SIGN,
    
    PMU_RSV8,                       //8
    PMU_RSV9,
    PMU_RSV10,
    PMU_RSV11,
    PMU_RSV12,
    PMU_RSV13,
    PMU_RSV14,
    PMU_RSV15,
    
    PMU_DSLEEP_INIT_SIGN,           //16
    PMU_LPWDT_LOCK_SIGN,
    PMU_WDT1_LOCK_SIGN,
    PMU_WDT_LOCK_SIGN,
    
    PMU_APP_SOFTRESET_SIGN,         //20
    PMU_WIFI_TEST_MODE_SIGN,
    PMU_RSV22,
    PMU_BOOT_DIRECT_RUN_PENDING2, 
};

#define pmu_boot_direct_run_en()                    PMU_REG_SET_BITS(PMU->PMUCON7, BIT(0))
#define pmu_boot_direct_run_dis()                   PMU_REG_CLR_BITS(PMU->PMUCON7, BIT(0))
#define pmu_get_boot_direct_run_pending()           ((PMU->PMUCON7 & BIT(0)))
#define pmu_get_deadcode_pending()                  ((PMU->PMUCON7 & BIT(1)) ? 1 : 0)
#define pmu_clr_deadcode_pending()                  PMU_REG_CLR_BITS(PMU->PMUCON7, BIT(1))
#define pmu_get_cp_fail_pending()                   ((PMU->PMUCON7 & BIT(2)) ? 1 : 0)
#define pmu_clr_cp_fail_pending()                   PMU_REG_CLR_BITS(PMU->PMUCON7, BIT(2))
#define pmu_get_boot_code_pos()                     ((PMU->PMUCON7 & BIT(3)) ? 1 : 0)

#define GET_DIRECT_RUN_DISABLE_SIGN()               ((PMU->PMUCON7 & BIT(4)) ? 1 : 0)
#define SET_DIRECT_RUN_DISABLE_SIGN(n)\
    do {\
        if (n)  PMU_REG_SET_BITS(PMU->PMUCON7, BIT(4));\
        else    PMU_REG_CLR_BITS(PMU->PMUCON7, BIT(4));\
    } while(0)

#define GET_SKIP_MASTER_BOOT_SIGN()     ((PMU->PMUCON7 & BIT(5)) ? 1 : 0)
#define SET_SKIP_MASTER_BOOT_SIGN(n)\
    do {\
        if (n)  PMU_REG_SET_BITS(PMU->PMUCON7, BIT(5));\
        else    PMU_REG_CLR_BITS(PMU->PMUCON7, BIT(5));\
    } while(0)

#define GET_SKIP_TEST_MODE_SIGN()     ((PMU->PMUCON7 & BIT(6)))
#define SET_SKIP_TEST_MODE_SIGN(n)\
    do {\
        if (n)  PMU_REG_SET_BITS(PMU->PMUCON7, BIT(6));\
        else    PMU_REG_CLR_BITS(PMU->PMUCON7, BIT(6));\
    } while(0)

#define GET_SKIP_PSRAM_SIGN()     ((PMU->PMUCON7 & BIT(7)))
#define SET_SKIP_PSRAM_SIGN(n)\
    do {\
        if (n)  PMU_REG_SET_BITS(PMU->PMUCON7, BIT(7));\
        else    PMU_REG_CLR_BITS(PMU->PMUCON7, BIT(7));\
    } while(0)

/* PMU->PMUCON7[8:19] : not used */


/* PMU->PMUCON7[20] */
#define GET_APP_SOFTRESET_SIGN()     ((PMU->PMUCON7 & BIT(20)))
#define SET_APP_SOFTRESET_SIGN(n)\
    do {\
        if (n)  PMU_REG_SET_BITS(PMU->PMUCON7, BIT(20));\
        else    PMU_REG_CLR_BITS(PMU->PMUCON7, BIT(20));\
    } while(0)

/* PMU->PMUCON7[22] */
#define GET_WIFI_TEST_MODE_SIGN()     ((PMU->PMUCON7 & BIT(21)))
#define SET_WIFI_TEST_MODE_SIGN(n)\
    do {\
        if (n)  PMU_REG_SET_BITS(PMU->PMUCON7, BIT(21));\
        else    PMU_REG_CLR_BITS(PMU->PMUCON7, BIT(21));\
    } while(0)
    
#define pmu_get_boot_direct_run_pending2()           ((PMU->PMUCON7 & BIT(23)))
#define pmu_set_direct_run_pengding2()              PMU_REG_SET_BITS(PMU->PMUCON7, BIT(23))
#define pmu_clr_direct_run_pengding2()              PMU_REG_CLR_BITS(PMU->PMUCON7, BIT(23))

#define pmu_set_direct_run_addr(addr)               PMU_REG_SET_VALUE(PMU->PMUCON7, 0xFF000000, ((addr >> 12) & 0xFF), 24)

/* CORE_PMUCON8 */
#define pmu_jtag_io_en()                            PMU_REG_SET_BITS(PMU->PMUCON8, BIT(0))
#define pmu_jtag_io_dis()                           PMU_REG_CLR_BITS(PMU->PMUCON8, BIT(0))


/* CORE_PMUCON11 */
#define VCCFLS_PG_DISABLE()\
    do {\
         pmu_reg_write((uint32)&PMU->PMUCON0,  PMU->PMUCON0 & (~ BIT(23))); \
         pmu_reg_write((uint32)&PMU->PMUCON11, PMU->PMUCON11 | (BIT(17))); \
    } while(0)
    
#define VCCFLS_PG_ENABLE()\
    do {\
         pmu_reg_write((uint32)&PMU->PMUCON11, PMU->PMUCON11 & (~ BIT(17)));\
         pmu_reg_write((uint32)&PMU->PMUCON0,  PMU->PMUCON0 | BIT(23)); \
    } while(0)
    
#define pmu_efuse_program_dis()                     PMU_REG_CLR_BITS(PMU->PMUCON11, BIT(31))
#define pmu_efuse_program_en()                      PMU_REG_SET_BITS(PMU->PMUCON11, BIT(31))

#define pmu_vcam_lc_dis()                           PMU_REG_CLR_BITS(PMU->PMUCON11, BIT(29))
#define pmu_vcam_lc_en()                            PMU_REG_SET_BITS(PMU->PMUCON11, BIT(29))
#define pmu_vcam_pg_en()                            PMU_REG_SET_BITS(PMU->PMUCON11, BIT(16))
#define pmu_vcam_pg_dis()                           PMU_REG_CLR_BITS(PMU->PMUCON11, BIT(16))
#define pmu_vcam_oc_detect_en()                     PMU_REG_SET_BITS(PMU->PMUCON11, BIT(15))
#define pmu_vcam_oc_detect_dis()                    PMU_REG_CLR_BITS(PMU->PMUCON11, BIT(15))
#define pmu_vcam_en()                               PMU_REG_SET_BITS(PMU->PMUCON11, BIT(14))
#define pmu_vcam_dis()                              PMU_REG_CLR_BITS(PMU->PMUCON11, BIT(14))
#define pmu_vcam_discharge_en()                     PMU_REG_SET_BITS(PMU->PMUCON11, BIT(13))
#define pmu_vcam_discharge_dis()                    PMU_REG_CLR_BITS(PMU->PMUCON11, BIT(13))

/* CORE_PNDCLR */
#define pmu_vcam2_oc_pending_clr()                  PMU_REG_SET_BITS(PMU->PNDCLR, BIT(30))
#define pmu_vcc18_oc_pending_clr()                  PMU_REG_SET_BITS(PMU->PNDCLR, BIT(29))
#define pmu_vcam_oc_pending_clr()                   PMU_REG_SET_BITS(PMU->PNDCLR, BIT(22))
#define pmu_vcc15_oc_pending_clr()                  PMU_REG_SET_BITS(PMU->PNDCLR, BIT(13))
#define pmu_vdd_oc_pending_clr()                    PMU_REG_SET_BITS(PMU->PNDCLR, BIT(12))

enum vcam_vol_level {
    VCAM_VOL_2V50,
    VCAM_VOL_2V55,
    VCAM_VOL_2V60,
    VCAM_VOL_2V65,
    VCAM_VOL_2V70,
    VCAM_VOL_2V75,
    VCAM_VOL_2V80,
    VCAM_VOL_2V85,
    VCAM_VOL_2V90,
    VCAM_VOL_2V95,
    VCAM_VOL_3V00,
    VCAM_VOL_3V05,
    VCAM_VOL_3V10,
    VCAM_VOL_3V15,
    VCAM_VOL_3V20,
    VCAM_VOL_3V25,
};
#define pmu_set_vcam_vol(vcam_vol_level)            PMU_REG_SET_VALUE(PMU->PMUCON11, 0x01e00000, vcam_vol_level, 21)
enum vcam_oc_level {
    VCAM_OC_750MA,
    VCAM_OC_300MA,
    VCAM_OC_200MA,
    VCAM_OC_100MA,
};
#define pmu_vcam_oc_set(vcam_oc_level)              PMU_REG_SET_VALUE(PMU->PMUCON11, 0x00000600, vcam_oc_level, 9)


/* CORE_RFCON0 */
#define pmu_xo_cs_set(xo_cs_vdd)                    PMU_REG_SET_VALUE(PMU->RFCON0, 0x03f80000, xo_cs_vdd, 19)
#define xo_set_cs_sec(xo_cs_vdd)                    pmu_xo_cs_set(xo_cs_vdd)

#define pmu_xosc_en()                               PMU_REG_SET_BITS(PMU->RFCON0, BIT(0))
#define pmu_xosc_dis()                              PMU_REG_CLR_BITS(PMU->RFCON0, BIT(0))

enum vcc_ldo_vol_level {
    VCC_LDO_VOL_1V00,
    VCC_LDO_VOL_1V05,
    VCC_LDO_VOL_1V10,
    VCC_LDO_VOL_1V15,
    VCC_LDO_VOL_1V20,
    VCC_LDO_VOL_1V25,
    VCC_LDO_VOL_1V30,
    VCC_LDO_VOL_1V35,
    VCC_LDO_VOL_1V40,
    VCC_LDO_VOL_1V45,
    VCC_LDO_VOL_1V50,
    VCC_LDO_VOL_1V55,
    VCC_LDO_VOL_1V60,
    VCC_LDO_VOL_1V65,
    VCC_LDO_VOL_1V70,
    VCC_LDO_VOL_1V75,
    VCC_LDO_VOL_1V80,
    VCC_LDO_VOL_1V85,
    VCC_LDO_VOL_1V90,
    VCC_LDO_VOL_1V95,
    VCC_LDO_VOL_2V00,
    VCC_LDO_VOL_2V05,
    VCC_LDO_VOL_2V10,
    VCC_LDO_VOL_2V15,
    VCC_LDO_VOL_2V20,
    VCC_LDO_VOL_2V25,
    VCC_LDO_VOL_2V30,
    VCC_LDO_VOL_2V35,
    VCC_LDO_VOL_2V40,
    VCC_LDO_VOL_2V45,
    VCC_LDO_VOL_2V50,
    VCC_LDO_VOL_2V55,
};

struct system_reset_pending_bits {
  uint32  srp_mclr : 1,           /* mclr pin reset */
          srp_soft : 1,           /* cpu0 mcu reset */
          srp_wdt : 1,            /* cpu0 watchdog reset */
          srp_lp_wdt : 1,         /* lowpower watchdog reset */
          srp_wdt1 : 1,           /* cpu1 watchdog reset */
          srp_soft1 : 1,          /* cpu1 mcu reset */
          Reserved0 : 2,
  
          srp_lvd : 1,            /* vcc lower voltage reset */
          srp_lvd_vcam_cur_ov : 1,/* vcam over-current */
          srp_lvd_vcc15_cur_ov : 1,/* vcc15 over-current */
          srp_lvd_vdd_cur_ov : 1,  /* vdd over-current */
          srp_lvd_vdd_vol_low : 1, /* vdd voltage low */
          srp_lvd_vcc_vol_low : 1, /* vcc voltage low */
          srp_lvd_vcc1_vol_low : 1, /* vcc1 voltage low */
          srp_lvd_vcc15_vol_low : 1, /* vcc15 voltage low */
          
          srp_deep_sleep : 1,
          srp_lvd_vdd18_cur_ov : 1, /* vdd18 over-current */
          srp_lvd_vcam2_cur_ov : 1, /* vcam2 over-current */
          
          Reserved : 13;
};

struct system_reset_info {
    union {
        uint32 system_reset_pending;
        struct system_reset_pending_bits  system_reset_pending_bits_s;
    };
}  __attribute__((packed));


extern struct system_reset_info sri;


void pmu_reg_write(uint32 reg_addr, uint32 data) ;

void pmu_dcdc_open(void);

void pmu_vccfls_power_set(uint32 power_on, uint32 suply_3p3v);
/* PC[8:13] power domain : VCC or VCC18 */
void pmu_vccsd_power_set(uint32 power_on, uint32 suply_3p3v);
int32 pmu_vdd18_ldo_en(uint32 en, enum vcc_ldo_vol_level vol);
int pmu_is_vdd18_ldo_en(void);
int pmu_get_vdd18_ldo_vol(void);

/**
 * strongly recommend vol same with vcc18 when power for mipi sensor
 */
int32 pmu_vcam2_ldo_en(uint32 en, enum vcc_ldo_vol_level vol);
int pmu_is_vcam2_ldo_en(void);
int pmu_get_vcam2_ldo_vol(void);


void pmu_tsensor_chan_sel_sec(uint8 chan_idx);
void pmu_pd_set_anatop_en_sec(void);
int jtag_map_set(uint8 val);

void tsensor_open(void);
void tsensor_close(void);

void lvd_irq_handler(void *data);
void sys_reset_detect(void);
void sys_reset_pending_clr(void);
void sys_reset_show(void);
void pmu_vdd_core_set(uint32 vddcore);



#ifdef __cplusplus
}
#endif

#endif //__PMU_H__



/******************* (C) COPYRIGHT 2021 HUGE-IC *****END OF FILE****/


