#include "basic_include.h"
#include "version.h"
#include "csi_config.h"
#include "soc.h"
#include "csi_core.h"
#include "csi_kernel.h"

extern int  main(void);
extern int  dev_init(void);
extern void device_init(void);

extern uint32_t g_intstackbase;
extern uint32_t g_top_irqstack;
extern uint32_t __heap_start;
extern uint32_t __heap_end;
extern uint32_t __psram_heap_start;
extern uint32_t __psram_heap_end;
extern struct os_workqueue main_wkq;
extern uint8_t assert_holdup;

uint32 srampool_start  = 0;
uint32 srampool_end    = 0;
uint32 psrampool_start = 0;
uint32 psrampool_end   = 0;
uint32 __sp_save[3];//add for cpu1 dsleep.S

__SYS_INIT void cache_open(void)
{
    uint32 csi_dcache_en = 0;
    sysctrl_cpu1_ibus_burst_set(CPU_BURST_SIZE_16B); 
    sysctrl_cpu1_dbus_burst_set(CPU_BURST_SIZE_0B); 

    sysctrl_cpu1_l2_dcache_en(1);
    sysctrl_cpu1_cache_en();
    if (!cld_cache_is_enable()) {
        cld_cache_enable();
        cld_cache_enable_profile();
    }

    csi_cache_reset_profile();
    csi_cache_set_range(0, 0, CACHE_CRCR_8M, 0x0);
    csi_cache_set_range(1, 0, CACHE_CRCR_8M, 0x0);
    csi_cache_set_range(2, 0, CACHE_CRCR_8M, 0x0);
    csi_cache_set_range(3, 0, CACHE_CRCR_8M, 0x0);
    if (!IS_SRAM_ADDR((uint32_t) & (__Vectors))) {
        csi_cache_set_range(0, ((uint32_t) & (__Vectors)) & 0xFFF00000, CACHE_CRCR_16M, 0x1);
    }
    
#if defined(PSRAM_HEAP)
    if (!sysctrl_get_cpu1_l2_dcache_en()) {
        sysctrl_cpu1_dbus_burst_set(CPU_BURST_SIZE_16B); 
        csi_cache_set_range(1, PSRAM_BASE, CACHE_CRCR_16M, 0x1);
        csi_dcache_en = 1;
    } 
#endif
    csi_cache_enable_profile();
    if (csi_dcache_en) {
        csi_dcache_enable();
    } else {
        csi_icache_enable();
    }
}

__SYS_INIT void system_clock_init(void)
{
    if (!sysctrl_cmu_sysclk_set(DEFAULT_SYS_CLK, 1)) {
        while (1);
    }
}

__SYS_INIT void SystemInit(void)
{
    cache_open();

    *((uint32_t *)(&g_intstackbase)) = 0xDEADBEEF;
    __set_VBR((uint32_t) & (__Vectors));

    void sysctrl_cmu_init_cpu(void);
    sysctrl_cmu_init_cpu();

#if defined(CONFIG_SEPARATE_IRQ_SP)
    /* 801 not supported */
    __set_Int_SP((uint32_t)&g_top_irqstack);
    __set_CHR(__get_CHR() | CHR_ISE_Msk);
#endif

    VIC->TSPR = 0xFF;
    /* Clear active and pending IRQ */
    csi_vic_disable_all_irq();
    csi_vic_clear_all_pending_irq();
    csi_vic_clear_all_active();
    /* All peripheral interrupt priority is set to lowest */
    /** All peripheral interrupt priority 
     * 0.CPU_SOFT_INT_IRQn (highest)
     * 1.cpu_splock & GFSK & CPU_DBG_ON_IRQn & cpu_MB
     */
    for (uint32 i = 0; i < IRQ_NUM; i++) {
        csi_vic_set_prio(i, 7);
    }

    csi_coret_config(system_clock_get() / 1000, CORET_IRQn);    //1ms
    csi_vic_enable_irq(CORET_IRQn);

    irq_enable(CPU_DBG_ON_IRQn);
    csi_vic_set_prio(CPU_DBG_ON_IRQn, 1);

    /* soft int highest priority, use for XIP mutex & CriticalText */
    irq_enable(CPU_SOFT_INT_IRQn);
    csi_vic_set_prio(CPU_SOFT_INT_IRQn, 0);
}

__init void malloc_init(void)
{
    sram_heap.name = "sram";
    sram_heap.ops  = &mmpool1_ops;
    sysheap_init(&sram_heap, (void *)CoreSetting->heap_addr, CoreSetting->heap_size, CoreSetting->heap_flag);
}

__init void pre_main(void)
{
    assert_holdup = ASSERT_HOLDUP;
    
    uint32 psram_rsv_addr;
    psram_rsv_addr = CoreSetting->psram_rsv_addr_core0;

    malloc_init();
#ifdef PSRAM_HEAP
    malloc_psram_init(); 
#endif    

    os_kernel_init();
    sys_sleepcb_init();

    dev_init();
    device_init();
    VERSION_SHOW();
    module_version_show();
    os_workqueue_init(&main_wkq, "MAIN", OS_TASK_PRIORITY_NORMAL, NULL, 2048);
    os_wkqmonitor_init();
    os_run_func((os_run_func_t)main, 0, 0, 0);
    os_kernel_start();
}

