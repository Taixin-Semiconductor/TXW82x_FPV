/**
  ******************************************************************************
  * @file    misc.h
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

#ifndef __MISC_H__
#define __MISC_H__

#include "typesdef.h"
#include "cld_cache.h"
#include "lib/common/rbuffer.h"

#ifdef __cplusplus
 extern "C" {
#endif



#ifdef VENUS_END
/*******************************************************************/
/*    Venus to be continue */
/*******************************************************************/
#endif


/**
  * @}
  */

///** @defgroup SYSMONITOR MACRO-DEF & Exported_Functions
//  * @{
//  */
typedef struct
{
    __IO uint32 CHx_LMT_L;
    __IO uint32 CHx_LMT_H;
    __IO uint32 CHx_ERR_ADR;
//         uint32 RESERVEDx;
} SYS_MNT_CH_TypeDef;

typedef struct
{
    __IO uint32 CTRL;
    __IO uint32 PND;
    __IO uint32 CLR;
    __IO uint32 RD;
    __IO uint32 WR;
    __IO uint32 rev[4];  
    SYS_MNT_CH_TypeDef SYS_MNT_CH[2];
} SYS_MNT_TypeDef;

#define SYSMNT                  ((SYS_MNT_TypeDef    *) SYS_MNT_BASE)

/**
  * @}
  */
/**
  * @brief CPU1_MNT
  */

typedef struct
{
    __IO uint32 R0;
    __IO uint32 R1;
    __IO uint32 R2; 
    __IO uint32 R3;
    __IO uint32 R4;
    __IO uint32 R5; 
    __IO uint32 R6;
    __IO uint32 R7;
    __IO uint32 R8; 
    __IO uint32 R9;
    __IO uint32 R10;
    __IO uint32 R11; 
    __IO uint32 R12; 
    __IO uint32 R13;
    __IO uint32 R14;
    __IO uint32 R15; //0x40
         uint32 reserved_0x44[12];
    __IO uint32 R28; 
         uint32 reserved_0x70[19];
    __IO uint32 EXPC;  //0xc0
    __IO uint32 PSR;
    __IO uint32 EPC; 
    __IO uint32 EPSR;
    __IO uint32 FCR;
    __IO uint32 FESR; 
    __IO uint32 CPU1_DBGON; 
    __IO uint32 CPU0_DBGON; 
    __IO uint32 CPU1_SFTPND; 
} CPU1_MNT_TypeDef;

#define CPU1_MNT                ((CPU1_MNT_TypeDef  *) 0x4000A100    )  


/** @addtogroup Template_Project
  * @{
  */
#define LL_SYSMONITOR_CHN_EN(n)                 (BIT(n))
#define LL_SYSMONITOR_CHN_CAP_RANGE_INTER(n)    (BIT(n+4))
#define LL_SYSMONITOR_CHN_CAP_RANGE_OUTER(n)    (0)
#define LL_SYSMONITOR_CAP_PC_EN                 (1UL << 6)
#define LL_SYSMONITOR_INT_EN                    (1UL << 7)

#define LL_SYSMONITOR_CH0_SEL(n)                (((n) & 0x1F) << 8)
#define LL_SYSMONITOR_CH0_SEL_MSK               ((0x1F) << 8)
#define LL_SYSMONITOR_CH0_CAP_READ              ((1UL) << 18)
#define LL_SYSMONITOR_CH0_CAP_WRITE             ((1UL) << 19)
#define LL_SYSMONITOR_CH0_CAP_MODE(n)           ((((n) & 0x3)) << 18)

#define LL_SYSMONITOR_CH1_SEL(n)                (((n) & 0x1F) << 13)
#define LL_SYSMONITOR_CH1_SEL_MSK               ((0x1F) << 13)
#define LL_SYSMONITOR_CH1_CAP_READ              ((1UL) << 20)
#define LL_SYSMONITOR_CH1_CAP_WRITE             ((1UL) << 21)
#define LL_SYSMONITOR_CH1_CAP_MODE(n)           ((((n) & 0x3)) << 20)


#define LL_SYSMONITOR_CLEAR_REGISTER()   do { SYSMNT->CLR = 1 | 2; } while(0)

/** @addtogroup XXX
  * @{
  */ 
typedef enum {
	//CHANNEL 1
    LL_SYSMONITOR_CHN_M2M0_RD       = (0) | (1<<8),
    LL_SYSMONITOR_CHN_M2M0_WR       = (1) | (1<<8),
    LL_SYSMONITOR_CHN_M2M1_RD       = (2) | (1<<8),
    LL_SYSMONITOR_CHN_M2M1_WR       = (3) | (1<<8),
    LL_SYSMONITOR_CHN_CPU0_ICODE    = (4) | (1<<8),
	  LL_SYSMONITOR_CHN_CPU0_DCODE	  = (5) | (1<<8),
    LL_SYSMONITOR_CHN_USB_20        = (6) | (1<<8),
    LL_SYSMONITOR_CHN_USB_11        = (7) | (1<<8),
	//CHANNEL 0
    LL_SYSMONITOR_CHN_CPU1_ICODE    = (6) | (0<<8),
	  LL_SYSMONITOR_CHN_CPU1_DCODE    = (7) | (0<<8),
    LL_SYSMONITOR_CHN_OSPI          = (8) | (0<<8),
    LL_SYSMONITOR_CHN_QSPI          = (9) | (0<<8),
    LL_SYSMONITOR_CHN_GMAC_RX       = (10) | (0<<8),
    LL_SYSMONITOR_CHN_GMAC_TX       = (11) | (0<<8),
} TYPE_ENUM_LL_SYSMONITOR_CH;

/** @addtogroup XXX
  * @{
  */ 
typedef enum {
    LL_SYSMONITOR_CAP_RANGE_INTER           = (0),
    LL_SYSMONITOR_CAP_RANGE_OUTER           = (1),
} TYPE_ENUM_LL_SYSMONITOR_CAP_RANGE;

/** @addtogroup XXX
  * @{
  */ 
typedef enum {
    LL_SYSMONITOR_CAP_MODE_READ             = (1),
    LL_SYSMONITOR_CAP_MODE_WRITE            = (2),
    LL_SYSMONITOR_CAP_MODE_READ_OR_WRITE    = (3),
} TYPE_ENUM_LL_SYSMONITOR_CAP_MODE;

typedef struct __ll_sysmonitor_cfg {
    /*! System monitor interrupt enable
     */
    bool                                int_en;
    /*! System monitor sub channel :  [0, 20]
     */
    TYPE_ENUM_LL_SYSMONITOR_CH          ch;
    /*! capture mode
    */
    TYPE_ENUM_LL_SYSMONITOR_CAP_MODE    cap_mode;
    /*! capture range
    */
    TYPE_ENUM_LL_SYSMONITOR_CAP_RANGE   cap_range;
    /*! Chx low limited address 
     */
    uint32                                 chx_limit_low;
    /*! Chx high limited address
     */
    uint32                                 chx_limit_high;
    /*  IRQ handle
    */
    void                                (*cb)(void *);
} TYPE_SYSMONITOR_CFG;

void sys_monitor_config(TYPE_SYSMONITOR_CFG *p_cfg);
void sys_monitor_reset(void);

/*******************************************************************/
/*             OSPI_EFF                                            */
/*******************************************************************/
struct HW_EFF {
    volatile uint32_t CON;
    volatile uint32_t PERIOD;
    volatile uint32_t RD_CNT;
    volatile uint32_t WR_CNT;
};

#define OSPI_EFF                ((struct HW_EFF *) 0x40020214)  
#define EFF_CNT_CLEAR (1<<6)
#define EFF_INTR_PEND (1<<5)
#define EFF_CH_SHIFT  (2)
#define EFF_CH_MASK   (7<<2)
#define EFF_INTR_EN   (1<<1)
#define EFF_MODU_EN   (1<<0)

#define OSPI_EFF_MSG_SIZE (5)

enum EFF_CH {
	EFF_CPU0_DBUS,
	EFF_CPU0_IBUS,
	EFF_CPU1_DBUS,
	EFF_CPU1_IBUS,
	EFF_H264,
	EFF_SCHED,
	EFF_CH_OTHER,
	EFF_CH_END,
};

struct eff_msg {
	uint32_t rd_cnt[EFF_CH_END];
	uint32_t wr_cnt[EFF_CH_END];
};

struct eff_mgr {
	RBUFFER_DEF(rb, struct eff_msg, OSPI_EFF_MSG_SIZE);
};


/*统计period_ms时间内PSRAM的请求数， MAX值是0xffffffff / CPU_CLK * 1000*/
int eff_start(uint32_t period_ms);
void eff_stop();

/*输出最近一次统计到的信息*/
int eff_printf();
int eff_read(struct eff_msg *msg);

/*******************************************************************/
/*             EFUSE                                               */
/*******************************************************************/
/**
  * @}
  */
#define CHIP_PACKID_KL928           0x01
#define CHIP_PACKID_TXW825_824      0x02

#define CHIP_PACKID_TXW826_824      0x10
#define CHIP_PACKID_TXW826_824C     0x11

#define CHIP_PACKID_TXW827_C08      0x20

#define CHIP_PACKID_TXW828_C08FL    0x30
#define CHIP_PACKID_TXW828_E08FL    0x31
#define CHIP_PACKID_TXW828_E016FL   0x32
#define CHIP_PACKID_TXW828_J016FL   0x33

#define CHIP_PACKID_TXM609_800      0x40
#define CHIP_PACKID_TXM609_804      0x41
#define CHIP_PACKID_TXM609_H016     0x42



/** @defgroup EFUSE MACRO-DEF & Exported_Functions
  * @{
  */

typedef struct
{
    __IO uint32 EFUSE_CON;
    __IO uint32 EFUSE_TIME_CON0;
    __IO uint32 EFUSE_TIME_CON1;
    __IO uint32 EFUSE_TIME_CON2;
    __IO uint32 EFUSE_STATUS;
    __IO uint32 EFUSE_ADDR_CNT;
    __IO uint32 EFUSE_DATA;
} EFUSE_TypeDef;

#define EFUSE                   ((EFUSE_TypeDef *) EFUSE_BASE)
/* efuse timing define */
#define LL_EFUSE_POWER_FREQ                 100000          /* 10us */
#define LL_EFUSE_PROGRAM_FREQ               100000          /* 10us (9~11uS) */
#define LL_EFUSE_READ_FREQ                  10000000       /* 100ns ( > 36ns)  */

uint16 sysctrl_efuse_config_and_read(uint32 addr, uint8 *p_buf, uint16 len);
void rf_para_efuse_check_valid(void);
void sysctrl_efuse_mac_addr_calc(uint8 *addr_buf);
uint16 sysctrl_efuse_get_customer_id(void);
uint8 sysctrl_efuse_get_chip_package(void);
uint32 sysctrl_efuse_get_smt_dat(void);
int32 tsensor_meas(uint8 sensor_idx);

/*******************************************************************/
/*             RFADC                                               */
/*******************************************************************/
typedef struct {
    __IO uint32 RFADCDIGCON;
} RFADCDIG_TypeDef;
    
#define RFADCDIG                ((RFADCDIG_TypeDef *) RFADCDIG_BASE)
    
#define RFADCDIGCON_ACS_ON_MSK                   BIT(0)
#define RFADCDIGCON_ACS_RW_MSK                   BIT(1)
#define RFADCDIGCON_ACS_ADDR_SHIFT               2
#define RFADCDIGCON_ACS_ADDR_MSK                (0x1f<<RFADCDIGCON_ACS_ADDR_SHIFT)
#define RFADCDIGCON_ACS_DATA_SHIFT               7
#define RFADCDIGCON_ACS_DATA_MSK                (0x3fffff<<RFADCDIGCON_ACS_DATA_SHIFT)
    
#define ADCOUTSEL                               0 // 0: output data after calibration; 1: output data before calibration

/*******************************************************************/
/*             RFADC                                               */
/*******************************************************************/

enum lf_clk_src {
    FROM_HOSC  = 0,
    FROM_RC32K = 2,
    FROM_PB0   = 3,
};

enum lf_clk_count {
    _1024_CYCLE = 0,
    _512_CYCLE  = 1,
    _256_CYCLE  = 2,
    _128_CYCLE  = 3,
    _64_CYCLE   = 4,
    _32_CYCLE   = 5,
    _16_CYCLE   = 6,
    _8_CYCLE    = 7,
};

enum dac_default_state {
    POWER_OFF = 0,
    SLEEP     = 1,
    STAND_BY  = 2,
    POWER_ON  = 3
};

enum rf_bw {
    BW_1M = 0,
    BW_2M = 1,
    BW_4M = 2,
    BW_8M = 3
};

enum rf_parameter_type {
	RF_EFUSE_ALL_SIM = 0,
    ADC_CALIB_VAL    = 1,
	ADDA_TIM_OFFSET  = 2,
    TXDCOC_CALIB_VAL = 3,
    TXIMB_CALIB_VAL  = 4,
    RXDCOC_CALIB_VAL = 5,
    RXIMB_CALIB_VAL  = 6
};

int32 tx_digi_gain_store(uint16 tx_digi_gain);
int32 sysctrl_internal_rf_sel_lo_freq(uint8 lo_freq_idx);
int32 is_xo_cs_calibed(void);
int32 is_xo_cto_calibed(void);
int32 xo_cs_store(uint8 xo_cs_val);
int32 sysctrl_get_xo_drcs(void);
int32 sysctrl_set_xo_drcs(uint16 xo_drcs);
void adc_cfg_reg_wr(uint8 addr, uint32 data);
uint32 adc_cfg_reg_rd(uint8 addr);
int32 is_adc_calibed(void);
void adc_init(void);
int32 adc_calib_res_store(void);
void adc_calib_res_restore(void);
void rfadc_calib_mode_soft_test(void);

int32 sysctrl_err_resp_icode_bus(uint8 enable);
int32 sysctrl_err_resp_dcode_bus(uint8 enable);
int32 sysctrl_err_resp_ahb2dma_bus(uint8 enable);
int32 sysctrl_err_resp_sdiodevice_bus(uint8 enable);
int32 sysctrl_err_resp_disable(void);

int32 sysctrl_get_chip_flash_size(void);

int32 sysctrl_is_chipdcn_compid(void);


/* 速率：x Bps */
uint32_t sys_dma2ahb_bw_static(int32 printf_en);

/* miss rate :  万分之x */
uint32_t sys_csi_cache_static(int32 core_id, int32 printf_en);

uint32_t sys_cld_cache_static(int32 printf_en);
uint32_t sys_psram_eff_static(int32 printf_en);

int32 sys_get_cpu_dcache_maint_both(void);
int32 sys_get_cpu_dcache_enable(void);

/** use when DMA source is psram 
 *  usage( this -> DMA kick) 
 */
void sys_dcache_clean_range (uint32_t *addr, int32_t dsize);
/** use when DMA destination is psram 
 *  usage : (  this -> DMA) ; 
 */
void sys_dcache_clean_invalid_range (uint32_t *addr, int32_t dsize);
/** use when DMA destination is psram 
 *  1.usage : ( DMA done-> this) ; 
 *  2.Precondition: dma buffer addr & size is 16/32byte aligned 
 *  3.DMA buffer size is not checked
 */
void sys_dcache_invalid_range (uint32_t *addr, int32_t dsize);

/* usefor TCPIP CHKSUM */
void sys_dcache_clean_range_unaligned (uint32_t *addr, int32_t dsize);

#ifdef __cplusplus
}
#endif

#endif //__MISC_H__



/******************* (C) COPYRIGHT 2021 HUGE-IC *****END OF FILE****/


