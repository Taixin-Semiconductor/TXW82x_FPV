
/******************************************************************************
 * @file     isr.c
 * @brief    source file for the interrupt server route
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#include "sys_config.h"
#include <csi_config.h>

#include "soc.h"
#include <csi_kernel.h>
#include "typesdef.h"
#include "errno.h"
#include "osal/irq.h"
#include "osal/string.h"

extern void ck_usart_irqhandler(int32_t idx);
extern void dw_timer_irqhandler(int32_t idx);
extern void dw_gpio_irqhandler(int32_t idx);
extern void systick_handler(void);
extern void xPortSysTickHandler(void);
extern void OSTimeTick(void);

static struct sys_hwirq sys_irqs[IRQ_NUM];

#define  ATTRIBUTE_ISR __attribute__((isr))
#define readl(addr) ({ unsigned int __v = (*(volatile unsigned int *) (addr)); __v; })

#if defined(CSKY_OS)
void csi_intrpt_exit_do(uint32 irqn)
{
    csi_kernel_intrpt_exit(); 
    uint32_t in_disable_irq(void);
    if (in_disable_irq()) { 
        os_printf(KERN_ERR"!!! sys_irq%d disable irq\r\n", irqn); 
        enable_irq(0); 
    } 
}

#define  CSI_INTRPT_ENTER(irqn) csi_kernel_intrpt_enter()
#define  CSI_INTRPT_EXIT(irqn)  csi_intrpt_exit_do(irqn)
#else
#define  CSI_INTRPT_ENTER(irqn)
#define  CSI_INTRPT_EXIT(irqn)
#endif

#ifdef SYS_IRQ_STAT
#define SYS_IRQ_STATE_ST(irqn)  uint32 _t1_, _t2_; _t1_ = csi_coret_get_value();
#define SYS_IRQ_STATE_END(irqn) do{ \
        _t2_ = csi_coret_get_value();\
        _t1_ = ((_t1_>_t2_)?(_t1_-_t2_):(csi_coret_get_load()-_t2_+_t1_));\
        sys_irqs[irqn].tot_cycle += _t1_;\
        sys_irqs[irqn].trig_cnt++;\
        if (_t1_ > sys_irqs[irqn].max) {\
            sys_irqs[irqn].max = _t1_;\
        }\
        if (_t1_ < sys_irqs[irqn].min || sys_irqs[irqn].min == 0) {\
            sys_irqs[irqn].min = _t1_;\
        }\
    }while(0)
#else
#define SYS_IRQ_STATE_ST(irqn)
#define SYS_IRQ_STATE_END(irqn)
#endif

void irq_enable(uint32 irq)
{
    csi_vic_enable_irq(irq);
}

void irq_disable(uint32 irq)
{
    csi_vic_disable_irq(irq);
}

uint32 disable_irq(void)
{
    return __disable_irq();
}

void irq_priority(uint32 irq, uint32 prio)
{
    csi_vic_set_prio(irq, prio);
}

void enable_irq(uint32 flags)
{
    if (!flags) {
        __enable_irq();
    }
}

uint32_t in_disable_irq(void)
{
    return __in_disable_irq(__get_PSR());
}


#define SYSTEM_IRQ_HANDLE_FUNC(func_name,irqn)\
    ATTRIBUTE_ISR void func_name(void)\
    {\
        CSI_INTRPT_ENTER(irqn);\
        SYS_IRQ_STATE_ST(irqn);\
        if (sys_irqs[irqn].handle) {\
            sys_irqs[irqn].handle(sys_irqs[irqn].data);\
        }\
        SYS_IRQ_STATE_END(irqn);\
        CSI_INTRPT_EXIT(irqn);\
    }

int32 request_irq(uint32 irq_num, irq_handle handle, void *data)
{
    if (irq_num < IRQ_NUM) {
        sys_irqs[irq_num].data   = data;
        sys_irqs[irq_num].handle = handle;
        return RET_OK;
    } else {
        return -EINVAL;
    }
}

int32 release_irq(uint32 irq_num)
{
    if (irq_num < IRQ_NUM) {
        irq_disable(irq_num);
        sys_irqs[irq_num].handle = NULL;
        sys_irqs[irq_num].data   = NULL;
        return RET_OK;
    } else {
        return -EINVAL;
    }
}

uint32 sysirq_time(void)
{
    uint32 irq_time = 0;
#ifdef SYS_IRQ_STAT
    int i;
    uint32_t cnt;
    uint32_t total;
    uint16_t max, min;
    os_printf("SYS IRQ TIME:\r\n");
    for (i = 0; i < IRQ_NUM; i++) {
        if (sys_irqs[i].trig_cnt) {
            cnt   = sys_irqs[i].trig_cnt;
            total = sys_irqs[i].tot_cycle / (DEFAULT_SYS_CLK / 1000000); //us
            max   = sys_irqs[i].max / (DEFAULT_SYS_CLK / 1000000); //us
            min   = sys_irqs[i].min / (DEFAULT_SYS_CLK / 1000000); //us
            irq_time += total; //us
            sys_irqs[i].trig_cnt = 0;
            sys_irqs[i].tot_cycle = 0;
            sys_irqs[i].max = 0;
            sys_irqs[i].min = 0;
            os_printf("  IRQ%-2d: trig:%d,\t total:%dus, \t(max:%d, min:%d, avg:%d)\r\n", i, cnt, total, max, min, total/cnt);
        }
    }
#endif
    return irq_time;
}

__ram ATTRIBUTE_ISR void CPU_SOFT_INT_IRQHandler(void)
{
    /* only use for CPU1 */
    CSI_INTRPT_ENTER(CPU_SOFT_INT_IRQn);

    uint32 flags = disable_irq();
    uint32 wdt_feed_time = CoreSetting->cpu_clk/8; // ~0.5s
    uint32 wdt_feed_cnt = CoreSetting->wdt1_to * 4;
    sysctrl_cpu1_kick_cpu0_softint(); //ack
    /* cpu0 critical section : cpu0 cann't lock cpu1 */
    //os_printf("  IRQ 100: trig:%d %d\r\n", SYSCTRL_GET_CPU0_SOFTINT_PENDING, SYSCTRL_GET_CPU1_SOFTINT_PENDING);
    mcu_watchdog_feed();
    while (CoreSetting->soft_int_pending) {
        wdt_feed_time--;
        if (!wdt_feed_time) {
            if (wdt_feed_cnt) {
                wdt_feed_cnt--;
                mcu_watchdog_feed();
            }
            wdt_feed_time = CoreSetting->cpu_clk/8; // ~0.5s
        }
    }
    sysctrl_cpu1_kick_cpu0_softint();//ack

    enable_irq(flags);
    CSI_INTRPT_EXIT(CPU_SOFT_INT_IRQn);
}

ATTRIBUTE_ISR void CORET_IRQHandler(void)
{
    SYS_IRQ_STATE_ST(CORET_IRQn);
    CSI_INTRPT_ENTER(CORET_IRQn);
    readl(0xE000E010);
    systick_handler();
    CSI_INTRPT_EXIT(CORET_IRQn);
    SYS_IRQ_STATE_END(CORET_IRQn);
}

ATTRIBUTE_ISR void CPU_DBG_ON_IRQHandler(void)
{
    //BIT0: CPU1 SOFTRESET PENDING , W1C
    volatile uint32 *dbg = (volatile uint32 *)CPUDBG_BASE;
    CSI_INTRPT_ENTER(CPU_DBG_ON_IRQn);
    
    uint32_t sys_core1_static(int32 printf_en);
    sys_core1_static(1);
    _os_printf("CPU debug0 ...MB_IRQ_EN=%08x MB_IRQ_STA=%08x\r\n", *(volatile int *)0x80001000, *(volatile int *)0x80001004);
    _os_printf("CPU debug1 ...MB_IRQ_EN=%08x MB_IRQ_STA=%08x\r\n", *(volatile int *)0x80000000, *(volatile int *)0x80000004);
    while (*dbg & (BIT(1) | BIT(2))) {
//        delay_us(2000*100);
//        _os_printf("CPU1 debug ...RD_IRQ_EN=%08x RD_IRQ_STA=%08x\r\n", *(volatile int *)0x80001000, *(volatile int *)0x80001004);
//        _os_printf("CPU0/1_SOFTINT=%08x %08x %08x\r\n", *(volatile uint32 *)(0x80001200), *(volatile uint32 *)(0x80001200), sys_soft_int_cnt, sys_soft_int_wait_cnt);
    }
    CSI_INTRPT_EXIT(CPU_DBG_ON_IRQn);
}

//0
SYSTEM_IRQ_HANDLE_FUNC(USB20DMA_IRQHandler, USB20DMA_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(USB20MC_IRQHandler, USB20MC_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(UART0_IRQHandler, UART0_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(UART1_IRQHandler, UART1_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(LCD_IRQHandler, LCD_IRQn)


//5
SYSTEM_IRQ_HANDLE_FUNC(QSPI_IRQHandler, QSPI_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SPI0_IRQHandler, SPI0_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SPI1_IRQHandler, SPI1_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SPI2_IRQHandler, SPI2_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(OSPI_IRQHandler, OSPI_IRQn)


//10
SYSTEM_IRQ_HANDLE_FUNC(TIM0_IRQHandler, TIM0_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(TIM1_IRQHandler, TIM1_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(TIM2_IRQHandler, TIM2_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(TIM3_IRQHandler, TIM3_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SCALE1_IRQHandler, SCALE1_IRQn)


//15
//SYSTEM_IRQ_HANDLE_FUNC(AUDIO_VAD_HS_ALAW_IRQHandler, AUDIO_SUBSYS2_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(AUDIO_ADC_IRQHandler, AUDIO_SUBSYS1_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(AUDIO_DAC_IRQHandler, AUDIO_SUBSYS0_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SDIO_IRQHandler, SDIO_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SDHOST1_IRQHandler, SDHOST1_IRQn)


//20
SYSTEM_IRQ_HANDLE_FUNC(SDHOST_IRQHandler, SDHOST_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(LMAC_IRQHandler, LMAC_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(GMAC_IRQHandler, GMAC_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(DUAL_ORG_IRQHandler, DUAL_ORG_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(GEN422_IRQHandler, GEN422_IRQn)


//25
//SYSTEM_IRQ_HANDLE_FUNC(CORET_IRQHandler, CORET_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SHA_IRQHandler, SHA_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(CRC_IRQHandler, CRC_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(ADKEY0_IRQHandler, ADKEY0_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(PD_TMR_IRQHandler, PD_TMR_IRQn)


//30
SYSTEM_IRQ_HANDLE_FUNC(WKPND_IRQHandler, WKPND_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(PDWKPND_IRQHandler, PDWKPND_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(LVD_IRQHandler, LVD_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(WDT_IRQHandler, WDT_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SYS_ERR_IRQHandler, SYS_ERR_IRQn)


//35
SYSTEM_IRQ_HANDLE_FUNC(IIS0_IRQHandler, IIS0_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(IIS1_IRQHandler, IIS1_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(GPIOA_IRQHandler, GPIOA_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(LO_MNT_IRQHandler, LO_MNT_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(DVP_IRQHandler, DVP_IRQn)


//40
SYSTEM_IRQ_HANDLE_FUNC(MJPEG01_IRQHandler, MJPEG01_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(H264ENC_IRQHandler, H264ENC_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(VPP_IRQHandler, VPP_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(PRC_IRQHandler, PRC_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(STMR5_IRQHandler, STMR5_IRQn)


//45
SYSTEM_IRQ_HANDLE_FUNC(PDM_IRQHandler, PDM_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(LED_TMR_IRQHandler, LED_TMR_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SCALE2_IRQHandler, SCALE2_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(GFSK_IRQHandler, GFSK_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(CMPOUT0_IRQHandler, CMPOUT0_IRQn)


//50
SYSTEM_IRQ_HANDLE_FUNC(UART6_IRQHandler, UART6_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SCALE3_IRQHandler, SCALE3_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(IMG_ISP_IRQHandler, IMG_ISP_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(MIPI_CSI2_IRQHandler, MIPI_CSI2_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(RF_IRQHandler, RF_IRQn)


//55
SYSTEM_IRQ_HANDLE_FUNC(USB11MC_IRQHandler, USB11_MC_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(USB11DMA_IRQHandler, USB11_DMA_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(RTC_VCCHVD_IRQHandler, RTC_VCCHVD_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(RTC_IRQHandler, RTC_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(MIPI_DSI_IRQHandler, MIPI_DSI_IRQn)


//60
SYSTEM_IRQ_HANDLE_FUNC(PARA_OUT_IRQHandler, PARA_OUT_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(M2M0_IRQHandler, M2M_DMA0_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(CAN_IRQHandler, CAN_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(EXT_DCACHE_IRQHandler, EXT_DCACHE_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(CPU_SPINLOCK_IRQHandler, CPU_SPINLOCK_IRQn)


//65
SYSTEM_IRQ_HANDLE_FUNC(CPU_BOX_RECV_IRQHandler, CPU_RECV_MAIL_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(CPU_BOX_SEND_IRQHandler, CPU_SEND_MAIL_IRQn)
//SYSTEM_IRQ_HANDLE_FUNC(CPU_DBG_ON_IRQHandler, CPU_DBG_ON_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(OSPI_FIFO_IRQHandler, OSPI_FIFO_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(OSPI_EFF_IRQHandler,  OSPI_EFF_IRQn)


//70
SYSTEM_IRQ_HANDLE_FUNC(GEN420_IRQHandler,  GEN420_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SYS_AES_IRQHandler, SYS_AES_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(PKA_IRQHandler,     PKA_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(WDT1_IRQHandler,    WDT1_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(GPIOB_IRQHandler,   GPIOB_IRQn)


//75
SYSTEM_IRQ_HANDLE_FUNC(GPIOC_IRQHandler, GPIOC_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(GPIOD_IRQHandler, GPIOD_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(GPIOE_IRQHandler, GPIOE_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(GPIOF_IRQHandler, GPIOF_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(DVP1_IRQHandler,  DVP1_IRQn)


//80
SYSTEM_IRQ_HANDLE_FUNC(PNG_IRQHandler,   PNG_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(STMR4_IRQHandler, STMR4_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(STMR3_IRQHandler, STMR3_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(STMR2_IRQHandler, STMR2_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(STMR1_IRQHandler, STMR1_IRQn)


//85
SYSTEM_IRQ_HANDLE_FUNC(STMR0_IRQHandler, STMR0_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(CMPOUT1_IRQHandler, CMPOUT1_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(UART5_IRQHandler, UART5_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(UART4_IRQHandler, UART4_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(MIPI1_CSI2_IRQHandler, MIPI1_CSI2_IRQn)


//90
SYSTEM_IRQ_HANDLE_FUNC(DMA2D_IRQHandler, DMA2D_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(CSC_IRQHandler, CSC_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(OSD_ENC_IRQHandler, OSD_ENC_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(USB20PHY_IRQHandler, USB20PHY_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(PARA_IN_IRQHandler, PARA_IN_IRQn)


//95
SYSTEM_IRQ_HANDLE_FUNC(M2M1_IRQHandler, M2M_DMA1_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(WKUP_IRQHandler, WKUP_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(SAM_IRQHandler,  SAM_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(LIN_IRQHandler,  LIN_IRQn)
SYSTEM_IRQ_HANDLE_FUNC(LIN1_IRQHandler, LIN1_IRQn)


//100
//RES: default_handler
//RES: default_handler
//RES: default_handler
//RES: default_handler
//RES: default_handler


//105
SYSTEM_IRQ_HANDLE_FUNC(M2M2_IRQHandler, M2M_DMA2_IRQn)


