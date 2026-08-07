#include "sys_config.h"
#include "basic_include.h"
#include "hal/adc.h"
#include "lib/rpc/cpurpc.h"
#include "lib/atcmd/libatcmd.h"
#include "lib/common/atcmd.h"
#include "lib/umac/ieee80211.h"
#include "lib/lmac/lmac.h"
#include "lib/bluetooth/uble/ble_demo.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/tcpip.h"
#include "netif/ethernetif.h"

#include "lib/net/dhcpd/dhcpd.h"
#include "lib/net/utils.h"
#include "lib/net/skmonitor/skmonitor.h"
#include "syscfg.h"
#include "app/user_app.h"

extern uint32 srampool_start;
extern uint32 srampool_end;
extern uint32 psrampool_start;
extern uint32 psrampool_end;

static struct os_work main_wk;
struct system_status  sys_status;

extern sysevt_hdl_res sys_event_hdl(uint32 event_id, uint32 data, uint32 priv);
extern void wifi_dev_status(uint32 dev_id);
extern void sys_atcmd_init(void);
extern void sys_wifi_test_mode_init(void);
extern void sys_wifi_init(void);
extern void sys_network_init(void);
extern void sys_dhcpd_start(void);
extern void sys_dhcpc_check(void);

static __init void sys_cpurpc_init()
{
    cpu_rpc_init(CPU0_MSGBOX_BASE, CPU_RECV_MAIL_IRQn, CPU_SEND_MAIL_IRQn);
}

__init static void sys_cfg_load(void)
{
    if (syscfg_init("syscfg", &sys_cfgs, sizeof(sys_cfgs)) == RET_OK) {
        return;
    }

    os_printf("use default params.\r\n");
    syscfg_default();
    syscfg_save();
}

static void sys_print_dbgtime(uint32 *buff, uint32 size)
{
#if SYS_APP_SNTP
    struct timeval tv;
    gettimeofday(&tv, 0);
    tv.tv_sec  += 8 * 3600; //时区
    os_printf("system time: %s\r\n", ctime((const time_t *)&tv.tv_sec));
#endif
}

static void sys_print_dbgcache(uint32 *buff, uint32 size)
{
    if (sys_status.dbg_cache) { //打印Cache命中率
        uint32 csi_miss = sys_csi_cache_static(sysctrl_get_cpu_id(), 1);
        uint32 cld_miss = sys_cld_cache_static(1);
        sys_psram_eff_static((cld_miss > 20) || (csi_miss > 30));
    }
}

static void sys_print_dbgtop(uint32 *buff, uint32 size)
{
    if (sys_status.dbg_top) {  //打印CPU使用率
        cpu_loading_print(sys_status.dbg_top == 2, (struct os_task_info *)buff, size / sizeof(struct os_task_info));
    }
}

static void sys_print_dbgheap(uint32 *buff, uint32 size)
{
    if (sys_status.dbg_heap) { //打印Heap使用情况
        sysheap_status(&sram_heap, buff, size / 4, 0);
#ifdef PSRAM_HEAP
        sysheap_status(&psram_heap, buff, size / 4, 0);
#endif
    }
}

static void sys_print_dbgumac(uint32 *buff, uint32 size)
{
    if (sys_status.dbg_umac) { //打印WIFI调试信息
        wifi_dev_status(HG_WIFI0_DEVID);
    }
}

static void sys_print_dbgnet(uint32 *buff, uint32 size)
{
    struct netif *nif;

    if (sys_status.dbg_net){
        os_printf("-----------------------------------------------------\r\n");
        os_printf("Network Info:\r\n");
        nif = netif_find("w0");
        if(nif){
            os_printf("  w0: (%s) "IPSTR"/"IPSTR"/"IPSTR"\r\n", 
                (sys_cfgs.dhcpc_en && sys_status.dhcpc_done)?"DHCP":"Static",
                IP2STR_N(nif->ip_addr.addr), IP2STR_N(nif->netmask.addr), IP2STR_N(nif->gw.addr));
        }

        nif = netif_find("e0");
        if(nif){
            os_printf("  e0: (%s) "IPSTR"/"IPSTR"/"IPSTR"\r\n", sys_cfgs.dhcpc_en?"DHCP":"Static",
                IP2STR_N(nif->ip_addr.addr), IP2STR_N(nif->netmask.addr), IP2STR_N(nif->gw.addr));
        }

#if IP_NAT
        os_printf("-----------------------------------------------------\r\n");
        ip4_nat_status();
#endif

        if(sys_cfgs.dhcpd_en){
            os_printf("-----------------------------------------------------\r\n");
            dhcpd_dump_ippool();
        }
        
    }
}

static void sys_dbginfo_print(void)
{
    static int8 print_interval = 0;
#if 0
    static uint32 _print_buf[256];
#else
    uint32 _print_buf[256];
    ASSERT(sizeof(_print_buf) < 1600); //使用task堆栈，避免堆栈溢出
#endif

    if (print_interval++ >= 5) { // 5秒打印一次
        sys_print_dbgcache(_print_buf, sizeof(_print_buf));
        sys_print_dbgtop(_print_buf, sizeof(_print_buf));
        sys_print_dbgheap(_print_buf, sizeof(_print_buf));
        sys_print_dbgumac(_print_buf, sizeof(_print_buf));
        sys_print_dbgtime(_print_buf, sizeof(_print_buf));
        sys_print_dbgnet(_print_buf, sizeof(_print_buf));
        print_interval = 0;
    }
}

__init static void sys_heap_info()
{
    /*打印各个heap区间信息*/
    os_printf("------------------------------------------------------------\r\n");
    os_printf("System Heaps Info, Total: %d (%p ~ %p)\r\n", SRAM_POOL_SIZE, SRAM_POOL_START, SRAM_POOL_START+SRAM_POOL_SIZE);
    os_printf("| CPU1 HEAP   : %p ~ %p, Size:%-8d     |\r\n", CONFIG_CORE_HEAP_START, CONFIG_CORE_HEAP_START+CONFIG_CORE_HEAP_SIZE, CONFIG_CORE_HEAP_SIZE);
    os_printf("| CPU1 RXBUF  : %p ~ %p, Size:%-8d     |\r\n", CONFIG_CORE_RXBUF_ADDR, CONFIG_CORE_RXBUF_ADDR+CONFIG_CORE_RXBUF_SIZE, CONFIG_CORE_RXBUF_SIZE);
    os_printf("| CPU1 SKBPOOL: %p ~ %p, Size:%-8d     |\r\n", CONFIG_CORE_SKB_POOL_ADDR, CONFIG_CORE_SKB_POOL_ADDR+CONFIG_CORE_SKB_POOL_SIZE, CONFIG_CORE_SKB_POOL_SIZE);
    os_printf("| CPU0 AVHEAP : %p ~ %p, Size:%-8d     |\r\n", CONFIG_AVHEAP_START, CONFIG_AVHEAP_START+CONFIG_AVHEAP_SIZE, CONFIG_AVHEAP_SIZE);
    os_printf("| CPU0 HEAP   : %p ~ %p, Size:%-8d     |\r\n", SYS_HEAP_START, SYS_HEAP_START+SYS_HEAP_SIZE, SYS_HEAP_SIZE);
    os_printf("------------------------------------------------------------\r\n");
}

static int32 sys_main_loop(struct os_work *work)
{
    mcu_watchdog_feed();
    sys_dbginfo_print();

#if SYS_NETWORK_SUPPORT
    sys_dhcpc_check();
#endif

    /*run again after 1000 ms.*/
    os_run_work_delay(&main_wk, 1000);
    return 0;
}

__init static void sys_app_init(void)
{
#if SYS_APP_DHCPD && SYS_NETWORK_SUPPORT
    sys_dhcpd_start();
#endif

#if SYS_APP_SNTP && SYS_NETWORK_SUPPORT
    sntp_client_init("ntp.aliyun.com", 2);
#endif

#ifdef SYS_APP_FPV
    sys_app_fpv_init();
#endif

#ifdef SYS_APP_DEMO
    sys_app_demo_init();
#endif

#ifdef SYS_APP_IPC
    sys_app_ipc_init();
#endif

#ifdef SYS_APP_BBM
    sys_app_bbm_init();
#endif

#if SYS_APP_BLENC
    sys_ble_netconfig_init();
#endif

#if SYS_WIFI_PAIR
    sys_wifi_pair_init();
#endif

#if SYS_WIFI_PAIR_LED
    sys_wifi_pair_led_init();
#endif

#ifdef SYS_APP_ISP_TUNNING
    sys_app_isp_tunning_init();
#endif

}

__init static void usr_app_init(void)
{
    /*
       添加用户App代码初始化
    */
}

static int32 watchdog_loop(struct os_work *work)
{
    mcu_watchdog_feed();
    os_run_work_delay(&main_wk, 1000);
    return 0;
}

int main(void)
{
    mcu_watchdog_timeout(0); //打开或关闭MCU看门狗
    sys_cpurpc_init();
    sys_heap_info();
    sys_cfg_load();
    sys_event_init(32);
    sys_atcmd_init();

    if (system_is_wifi_test_mode()) { // enter wifi test mode
        sys_wifi_test_mode_init();
        system_reboot_normal_mode();
        OS_WORK_INIT(&main_wk, watchdog_loop, 0);
        os_run_work_delay(&main_wk, 1000);
    }else{ // normal mode
        sys_wifi_init();
        sys_network_init();
        do_global_ctors();
        sys_app_init();
        usr_app_init();
        sys_event_take(0xffffffff, sys_event_hdl, 0);
        OS_WORK_INIT(&main_wk, sys_main_loop, 0);
        os_run_work_delay(&main_wk, 1000);
    }
    pmu_watchdog_timeout(0); //打开或关闭PMU看门狗, 放在sys_wifi_init之后！！
    return 0;
}

