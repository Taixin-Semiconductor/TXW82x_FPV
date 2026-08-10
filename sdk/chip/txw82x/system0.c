#include "basic_include.h"
#include "version.h"
#include "csi_config.h"
#include "soc.h"
#include "csi_core.h"
#include "csi_kernel.h"
#include "sys_config.h"
#include "dev/dma/hg_m2m_dma.h"
#include "lib/ApplicationLoader/al_typedef.h"
#ifdef CONFIG_SLEEP
#include "lib/common/dsleepdata.h"
#endif
#include "lib/lmac/lmac.h"

#include "dev/xspi/hg_xspi_psram.h"
#include "dev/xspi/hg_xspi_flash.h"
#include "dev/adc/hgadc_v1.h"
#include "hal/adc.h"

extern int  main(void);
extern int32 dev_init(void);
extern void device_init(void);
extern int psram_auto_init(int extern_pt, uint32_t clk);
extern void psram_info(int isready);
void pmu_watchdog_init(void);

extern uint32_t g_intstackbase;
extern uint32_t g_top_irqstack;
extern uint32_t __heap_start;
extern uint32_t __heap_end;
extern uint32_t __psram_heap_start;
extern uint32_t __psram_heap_end;
extern unsigned int __al_user_sram_start;
extern struct os_workqueue main_wkq;
extern uint8_t assert_holdup;
extern uint32  psram_rsv_addr;

uint32 srampool_start  = 0;
uint32 srampool_end    = 0;
uint32 psrampool_start = 0;
uint32 psrampool_end   = 0;
uint32 __sp_save[3];


#ifndef SYS_FACTORY_PARAM_SIZE
#define SYS_FACTORY_PARAM_SIZE 20
#endif

//生成func_code=1的参数
#define FUNCCODE1_HEAD(fun_num,size) (((size) << 8) | ((fun_num)))
#define FUN_NUM (1)             //func_code=1的参数
#define FUN_NUM_1_SIZE (2+16)   //2是保留的2byte,16分别是iocfg、eqcfg、ispcfg, psramcfg


#ifdef PIN_FROM_PARAM
const uint16_t __used iocfg_psram[IOCFG_SIZE / 2] = {IOCFG_SIZE};
#else
#endif

#define PARAM_HEAD(size,head)  (SYS_FACTORY_PARAM_SIZE|head<<16)

__initconst const uint16_t __used isp_param[4096 / 2] = {4096, 0};
__initconst const uint16_t __used eq_param[1024 / 2] = {1024};
__initconst const uint16_t __used psram_param[4096 / 2] = {4096, 0};

const uint32_t __used sys_factory_param[SYS_FACTORY_PARAM_SIZE / 2] __at_section("SYS_PARAM") = 
    {
        PARAM_HEAD(SYS_FACTORY_PARAM_SIZE,0x2B1A),
        FUNCCODE1_HEAD(FUN_NUM,FUN_NUM_1_SIZE),
        (uint32_t)IOCFG_PARAM_ADDR, 
        (uint32_t)eq_param, 
        (uint32_t)isp_param, 
        (uint32_t)psram_param
    };


/** recommend usage :
  * 1、CSI (CPU separate) cache use for IBUS : SRAM(0x040000000) FLASH(0x10000000) PSRAM(0x08000000)
  *        32kB*2                       DBUS : SRAM(0x200000000) FLASH(0x30000000) PSRAM(0x28000000)
  *
  * 2、CLD (AHB share) cache only for   DBUS : PSRAM(0x28000000) can share for CPU0 & CPU1
  *        32KB
  */
__SYS_INIT void cache_open(void)
{
    sysctrl_cpu0_ibus_burst_set(CPU_BURST_SIZE_16B);

    sysctrl_cpu0_l2_dcache_en(1);
    sysctrl_cpu1_l2_dcache_en(0);
    cld_cache_disable();

    csi_cache_reset_profile();
    csi_cache_set_range(0, 0, CACHE_CRCR_8M, 0x0);
    csi_cache_set_range(1, 0, CACHE_CRCR_8M, 0x0);
    csi_cache_set_range(2, 0, CACHE_CRCR_8M, 0x0);
    csi_cache_set_range(3, 0, CACHE_CRCR_8M, 0x0);
    if (!IS_SRAM_ADDR((uint32_t) & (__Vectors))) {
        csi_cache_set_range(0, ((uint32_t) & (__Vectors)) & 0xFFF00000, CACHE_CRCR_8M, 0x1);
    }

    csi_cache_enable_profile();
    csi_icache_enable();
}

__SYS_INIT void cache_open_psram(void)
{
    sysctrl_cpu0_dbus_burst_set(CPU_BURST_SIZE_0B);

    sysctrl_cpu0_l2_dcache_en(1);
    if (!cld_cache_is_enable()) {
        sysctrl_force_cld_allocate_en();
        cld_cache_invalidate_all();
        cld_cache_enable();
        cld_cache_enable_profile();
    }

#if defined(PSRAM_HEAP)
    if (!SYSCTRL_GET_CPU0_L2_DCACHE_EN) {
        sysctrl_cpu0_dbus_burst_set(CPU_BURST_SIZE_16B);
        csi_cache_set_range(3, PSRAM_BASE, CACHE_CRCR_16M, 0x1);
        csi_dcache_enable();
    } 
#endif
}


__SYS_INIT void system_clock_init(void)
{
   if (!sysctrl_cmu_sysclk_set(DEFAULT_SYS_CLK, 1)) {
       while (1);
   }
}

__SYS_INIT void system_set_ace_peris(void)
{
    uint32_t ie = disable_irq();
#ifndef SINGLE_CORE
    sysctrl_ace_peris_access_cpu0(ACE_USB20|ACE_USB11|ACE_GMAC|ACE_DISPLAY_TOP|ACE_VIDEO_INTF_TOP|ACE_VIDEO_SUBSYS_TOP|ACE_OSPI_CTRL|ACE_QSPI_CTRL|ACE_H264_CODEC_TOP|ACE_IMAGE_ISP_TOP|ACE_VIDEO_GPU_TOP);
    sysctrl_ace_peris_access_cpu1(ACE_BASEBAND1|ACE_BASEBAND|ACE_RFDIGITAL|ACE_RFDIGCAL_TOP);
    sysctrl_ace_peris_access_cpu_all(ACE_MIX_TOP|ACE_GPIO_TOP|ACE_EFUSE_CTRL|ACE_SYS_SEC_TOP|ACE_PMU);
#else
    sysctrl_ace_peris_access_cpu0(0xFFFFFFFF);
    sysctrl_ace_peris_access_cpu_all(ACE_BASEBAND1|ACE_BASEBAND|ACE_RFDIGITAL|ACE_RFDIGCAL_TOP);
#endif
    enable_irq(ie);
}

#ifndef SINGLE_CORE
__SYS_INIT void sys_start_cpu1(uint32_t run_addr)
{
    os_printf(KERN_NOTICE"kick start cpu1 at %p!\n", run_addr);

    CoreSetting->soft_int_pending = 0;
    CoreSetting->print_level = 0;
    CoreSetting->disable_print = 0;
    CoreSetting->dcache_maint_en = CONFI_CORE_DCACHE_MAINT_EN;
    CoreSetting->dbg_uart_dev = CONFI_CORE_UARTDEV;
    CoreSetting->m2m_dma_dev = (HG_M2M_DMA_NUM > 2) ? 0 : 1; // modify HG_M2M_DMA_NUM CORE0 2 or 3 （core1 use DMA2）
    CoreSetting->adc_dev = 0;
    CoreSetting->wdt1_to = 4;
    CoreSetting->wdt1_irq_hdl = 0;
    CoreSetting->cpu_clk = CONFIG_CORE_CPU_CLK;
    CoreSetting->rxbuf_addr = (uint32_t)(os_malloc(CONFIG_CORE_RXBUF_SIZE));;
    CoreSetting->rxbuf_size = CONFIG_CORE_RXBUF_SIZE;
    CoreSetting->heap_addr = (uint32_t)(os_malloc(CONFIG_CORE_HEAP_SIZE));
    CoreSetting->heap_size = CONFIG_CORE_HEAP_SIZE;
    CoreSetting->skbpool_addr = (uint32_t)(os_malloc_psram(CONFIG_CORE_SKB_POOL_SIZE));;
    CoreSetting->skbpool_size = CONFIG_CORE_SKB_POOL_SIZE;
    CoreSetting->skbpool_flag = BIT(4) | BIT(7); //SKBPOOL_FLAGS_ALIGN_32|SKBPOOL_FLAGS_TAIL_ALIGN_32
    CoreSetting->soft_int_pending = 0;
    CoreSetting->vif_maxcnt = 8;
    CoreSetting->bss_maxcnt = 16;
    CoreSetting->sta_maxcnt = 4;
    CoreSetting->bss_lifetime = 30;
    CoreSetting->heap_flag = 0;//SYSHEAP_FLAGS_MEM_LEAK_TRACE | SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK;
    CoreSetting->afh_en = 0;
    CoreSetting->afh_chan_mask = 0x00;
    #ifdef LMAC_BGN_PCF
    CoreSetting->afh_en = 1;
    #endif
    #if defined(PSRAM_HEAP)
    CoreSetting->psram_rsv_addr_core0 = (uint32_t)os_malloc_psram(32);
    #else
    CoreSetting->psram_rsv_addr_core0 = 0;
    #endif
    CoreSetting->lmac_module_init_mask = (WIFI_MODULE_MULTI_MAC_EN ? LMAC_MODULE_INIT_BIT_MULTI_MAC : 0)        |\
                                         (WIFI_MODULE_RX_REORDER_EN ? LMAC_MODULE_INIT_BIT_RX_REORDER : 0);
    sysctrl_cpu1_softrst_en();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    sysctrl_cpu1_softrst_system_en();
    sysctrl_cpu1_softrst_self_dis();
    sysctrl_set_cpu1_pc_rst_addr(run_addr >> 10);
    sysctrl_cpu1_clk_en();
    sysctrl_set_cpu1_jtag_map(CPU1_JTAG_DISABLE);
    sysctrl_cpu1_softrst_dis();
    while(!CoreSetting->cpu1_ready);
    os_printf(KERN_INFO"CPU1 ready!\r\n");
}
#endif

void cld_cache_irq_handler(void *data)
{
    os_printf("%s %d\r\n",__func__, __LINE__);
    os_printf("CTRL=%08x\r\n", DCACHE_CTRL->CTRL);
    os_printf("MAINT_STATUS=%08x\r\n", DCACHE_CTRL->MAINT_STATUS);
    os_printf("SECIRQSTAT=%08x\r\n", DCACHE_CTRL->SECIRQSTAT);
    os_printf("NSECIRQSTAT=%08x\r\n", DCACHE_CTRL->NSECIRQSTAT);
     __disable_irq();
    while(1);
}

__SYS_INIT void SystemInit(void)
{

    SYSCTRL_REG_OPT_INIT();
    sysctrl_cpu1_clk_dis();

    srampool_start  = (uint32)&__heap_start;
    srampool_end    = (uint32)&__heap_end;
#if defined(PSRAM_HEAP)
    psrampool_start = (uint32)&__psram_heap_start;
    psrampool_end   = (uint32)&__psram_heap_end;
#endif

    sysctrl_qspi_lock();
    
    system_set_ace_peris();
    cache_open();
    
    *((uint32_t *)(&g_intstackbase)) = 0xDEADBEEF;
    __set_VBR((uint32_t) & (__Vectors));

    sys_reset_pending_clr();

    PMU_REG_CLR_BITS(PMU->PMUCON7, BIT(PMU_WDT_LOCK_SIGN)|BIT(PMU_WDT1_LOCK_SIGN)|BIT(PMU_LPWDT_LOCK_SIGN)|BIT(PMU_BOOT_DIRECT_RUN));
    sysctrl_efuse_pwron_init();

    if (sysctrl_get_softreset_pending()) {
        sysctrl_clr_softreset_pending();
        pmu_set_direct_run_pengding2();
    } else {
        pmu_clr_direct_run_pengding2();
    }

    firmware_info_t info_in_fls = (firmware_info_t)&__al_user_sram_start;
    user_info_t user_data   = (user_info_t)info_in_fls->user.user_data;
    if (info_in_fls->user.user_data) {
        if (user_data->fw_magic == (0xC791B319)) {
            
        } else {
            ll_xip_clock_init(0);
        }
    } else {
        ll_xip_clock_init(0);
    }
    
    sysctrl_cmu_init();
    
#ifndef FPGA_SUPPORT
    system_clock_init();
#endif
    if (DEFAULT_SYS_CLK > (192*1000000) ) {
        void ll_clock_set_apb0_div(uint8 apb0_div);
        ll_clock_set_apb0_div(2);
    }

    //pmu_vdd_core_set(5);
    pmu_vdd_dis();

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
    for (uint32 i = 0; i < IRQ_NUM; i++) {
        csi_vic_set_prio(i, 7);
    }

    csi_coret_config(system_clock_get() / CONFIG_SYSTICK_HZ, CORET_IRQn);    //1ms
    csi_vic_enable_irq(CORET_IRQn);
    request_irq(LVD_IRQn, lvd_irq_handler, 0);
    irq_enable(LVD_IRQn);

    cld_cache_irq_enable();
    request_irq(EXT_DCACHE_IRQn, cld_cache_irq_handler, 0);
    irq_enable(EXT_DCACHE_IRQn);
    
    pmu_clr_deadcode_pending();
}

__init static void malloc_init(void)
{
    uint32 flags = 0;
#ifdef MEM_TRACE
    flags |= SYSHEAP_FLAGS_MEM_LEAK_TRACE | SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK;
#endif
    sram_heap.name = "sram";
    sram_heap.ops  = &mmpool1_ops;
    sysheap_init(&sram_heap, (void *)SYS_HEAP_START, SYS_HEAP_SIZE, flags);
}

__init static void malloc_psram_init(void)
{
#ifdef PSRAM_HEAP
    uint32 flags = SYSHEAP_FLAGS_MEM_ALIGN_32;
#ifdef MEM_TRACE
    flags |= SYSHEAP_FLAGS_MEM_LEAK_TRACE | SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK;
#endif
    psram_heap.name = "psram";
    psram_heap.ops  = &mmpool1_ops;
    sysheap_init(&psram_heap, (void *)SYS_PSRAM_HEAP_START, SYS_PSRAM_HEAP_SIZE, flags);
#endif
}


__init static int system_psram_init(void)
{
	extern void add_psram_cfg();
	add_psram_cfg();
    
    //  240M， 320M, 274M, 160M...
    //  你可以选择你喜欢的频率， 但内部只有几个挡位可以选择， 匹配最接近的配置
    int psram_heap_size = psram_auto_init(0, 320 * 1000000);
    cache_open_psram();
    
#ifdef PSRAM_HEAP
    if(psram_heap_size){
        psrampool_end = PSRAM_BASE + psram_heap_size*1024*1024;
        malloc_psram_init(); 
        psram_rsv_addr = (uint32)os_malloc_psram(32);
    }

    if(!psram_heap_size){
        os_printf("psram not ready\n");
    }
	return psram_heap_size;
#endif
}



__init void pre_main(void)
{
    CoreSetting->cpu1_ready = 0;
    assert_holdup = ASSERT_HOLDUP;

    save_boot_loader_addr();
    malloc_init();
    
    int psram_size = system_psram_init();
    
    os_kernel_init();
#ifdef CONFIG_SLEEP
    sys_sleepcb_init();
#endif    
    dev_init();
    device_init();
    
    psram_info(psram_size);
    
    sysctrl_efuse_validity_handle(1);
    
#ifndef SINGLE_CORE
    adc_open((struct adc_device *)dev_get(HG_ADC0_DEVID));
    irq_enable(CPU_DBG_ON_IRQn);
    csi_vic_set_prio(CPU_DBG_ON_IRQn, 1);
    sys_start_cpu1(0x10001000);
#endif
#ifdef CONFIG_SLEEP
    sys_sleepdata_reset();
    sys_sleepdata_init();
#endif    
    pmu_watchdog_init();
    
    VERSION_SHOW();
    module_version_show();
    sys_reset_show();
    os_workqueue_init(&main_wkq, "MAIN", OS_TASK_PRIORITY_NORMAL, NULL, 2048);
    os_wkqmonitor_init();
    os_run_func((os_run_func_t)main, 0, 0, 0);

    os_kernel_start();
}

