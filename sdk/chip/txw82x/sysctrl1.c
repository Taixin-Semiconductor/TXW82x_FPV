/**
  ******************************************************************************
  * @file    sysctrl.c
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

#include "sys_config.h"
#include "typesdef.h"
#include "version.h"
#include "errno.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/sleep.h"
#include "osal/irq.h"
#include "osal/string.h"

#include "hal/gpio.h"
#include "hal/spi.h"
#include "hal/spi_nor.h"

#include "lib/common/ticker_api.h"
#ifdef CONFIG_SLEEP
    #include "lib/common/dsleepdata.h"
#endif

MODULE_VERSION(core1);

struct __clock_cfg sys_clock_cfg __attribute__((aligned(4))) = {0};

uint16 sysctrl_get_chip_id()
{
    return ((uint16)(SYSCTRL->CHIP_ID));
}

uint32 sysctrl_get_cpu_id(void) 
{
    return !(CPU0_ACE->CPU_ACE_FLG1);
}

void sysctrl_cmu_init_cpu(void)
{
    /* set default osc clk, */
    sys_clock_cfg.exosc_clk_hz = CLK_HXOSC;
    sys_clock_cfg.hirc_clk_hz = CLK_RC8M;
//    sys_clock_cfg.syspll_clk
    sys_clock_cfg.sys_clk = DEFAULT_SYS_CLK;//system_clock_get_refresh();
    
}

/**
 * @brief   system_clock_get
 * @retval  system clk_hz
 * @note    this function will get the clock form sotfware temp storage
 */
uint32 system_clock_get(void)
{
    return sys_clock_cfg.sys_clk;
}
int32 sys_get_sysclk(void)
{
    return system_clock_get();
}


uint32 mcu_watchdog_static(uint8 pinrtf_en)
{
    uint8  tmo_cnt;
    uint32 tmo_ms = 0;
    if (*((volatile unsigned int *)(WDT1_BASE)) & BIT(4)) {
        tmo_ms = 8;
        tmo_cnt = *(volatile unsigned int *)(WDT1_BASE) & 0xF;
        for ( ; tmo_cnt; tmo_cnt--) {
            tmo_ms *= 2;
        } 
    }
    if (pinrtf_en) {
        os_printf(KERN_WARNING"WTD lock=%d TO=%d ms, REG=%08x %08x %08x %08x\r\n", !!(PMU->PMUCON7 & BIT(PMU_WDT1_LOCK_SIGN)), tmo_ms, 
            *((volatile unsigned int *)(WDT1_BASE+0)), *((volatile unsigned int *)(WDT1_BASE+4)),
            *((volatile unsigned int *)(WDT1_BASE+8)), *((volatile unsigned int *)(WDT1_BASE+12))
            );
    }
    return tmo_ms;
}

int32 mcu_watchdog_timeout(uint8 tmo_s)
{
    if ((PMU->PMUCON7 & BIT(PMU_WDT1_LOCK_SIGN))) {
        return -ENOLCK;
    }

    if (tmo_s) {
        int time_cycle = 6;     /* 7 for 1S, 0 for 8ms  */
                    
        *(volatile unsigned int *)(WDT1_BASE + 0x4) = 0xDDDC + (!(PMU->PMUCON7 & BIT(PMU_WDT1_LOCK_SIGN)));;
        *(volatile unsigned int *)(WDT1_BASE + 0x4) = 0xAA55;
    
         while (tmo_s) {
             time_cycle++;
             tmo_s >>= 1;
         }

         do {
            *(volatile unsigned int *)(WDT1_BASE + 0x4) = 0x5554 + (!(PMU->PMUCON7 & BIT(PMU_WDT1_LOCK_SIGN)));;
            *(volatile unsigned int *)(WDT1_BASE) = ((*(volatile unsigned int *)WDT1_BASE) &~ (0xF << 0)) | (time_cycle&0xF << 0);
         } while(0);

        *(volatile unsigned int *)(WDT1_BASE + 0x4) = 0xAAAA;
        *(volatile unsigned int *)(WDT1_BASE + 0x4) = 0xCCCC;
        PMU_REG_SET_BITS(PMU->PMUCON7, BIT(PMU_WDT1_LOCK_SIGN));
    }else { //disable
        *(volatile unsigned int *)(WDT1_BASE + 0x4) = 0xDDDC + (!(PMU->PMUCON7 & BIT(PMU_WDT1_LOCK_SIGN)));;
    }
    return 0;
}

void mcu_watchdog_timeout_level(uint8 level)
{
    if (level) {
        /* 7 for 1S  */
        *(volatile unsigned int *)(WDT1_BASE + 0x4) = 0xDDDD;
        *(volatile unsigned int *)(WDT1_BASE + 0x4) = 0xAA55;
    
         do {
            *(volatile unsigned int *)(WDT1_BASE + 0x4) = 0x5555;
            *(volatile unsigned int *)(WDT1_BASE) = ((*(unsigned int *)WDT1_BASE) &~ (0xF << 0)) | (level&0xF << 0);
         } while(0);

        *(volatile unsigned int *)(WDT1_BASE + 0x4) = 0xAAAA;
        *(volatile unsigned int *)(WDT1_BASE + 0x4) = 0xCCCC;
    }
}

void mcu_watchdog_irq_request(void *hdl)
{
    if(hdl) {
        *(volatile unsigned int *)(WDT1_BASE + 0x4) = 0x55AA;
        irq_enable(WDT1_IRQn);
        request_irq(WDT1_IRQn, (void*)hdl, NULL);
    } else {
        *(volatile unsigned int *)(WDT1_BASE + 0x4) = 0xAA55;
        irq_disable(WDT1_IRQn);
        release_irq(WDT1_IRQn);
    }
}

void mcu_watchdog_feed(void)
{
    /* Enable watchdog when feed watchdog everytime and wdt1 is open */
    *(volatile unsigned int *)(WDT1_BASE + 0x4) = 0xCCCB + (!!(PMU->PMUCON7 & BIT(PMU_WDT1_LOCK_SIGN)));
    *(volatile unsigned int *)(WDT1_BASE + 0x4) = 0xAAAA;
}

void mcu_reset(void)
{
    __disable_irq();
    mcu_watchdog_irq_request(NULL);
    mcu_watchdog_timeout_level(1);
    while(1);
}


/******************* (C) COPYRIGHT 2025 HUGE-IC *****END OF FILE****/

