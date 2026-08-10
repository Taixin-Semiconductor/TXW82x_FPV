#include "sys_config.h"
#include "basic_include.h"
#include "lib/lmac/lmac.h"
#include "lib/bluetooth/hci/hci_controller.h"
#include "lib/skb/skbpool.h"
#include "lib/rpc/cpurpc.h"
#include "lib/umac/ieee80211.h"
#include "syscfg.h"

static struct os_work main_wk;
struct system_status  sys_status;
extern uint32         srampool_start;
extern uint32         srampool_end;
static void          *lmac_ops;
static void          *bt_ops;

extern void sys_atcmd_init(void);
extern int32 sys_ieee80211_event_rpc(uint8 ifidx, uint16 evt, uint32 param1, uint32 param2);

static int32 sys_ieee80211_event_cb(uint8 ifidx, uint16 evt, uint32 param1, uint32 param2)
{
    int32 ret = RET_OK;

    switch (evt) {
        case IEEE80211_EVENT_RX_FRAME:
        case IEEE80211_EVENT_TX_FRAME:
        case IEEE80211_EVENT_UPDATE_BSS:
        case IEEE80211_EVENT_ADD_CUSTOMER_IE:
        case IEEE80211_EVENT_BEACON_IDLE:
        case IEEE80211_EVENT_TX_STATUS:
            return RET_OK; // 这些event拦截下来，不传递给 CPU0: 过于频繁
        default:
            break;
    }

    //////////////////////////////////////////////////////////////////////
    ret |= sys_ieee80211_event_rpc(ifidx, evt, param1, param2);
    return ret;
}

int32 lmac_ioctl_rpc(void *lops, uint32 cmd, uint32 param1, uint32 param2)
{
    if (lmac_ops) {
        return lmac_ioctl(lmac_ops, cmd, param1, param2);
    } else {
        return -ENODEV;
    }
}

static __init void sys_cpurpc_init()
{
    cpu_rpc_init(CPU1_MSGBOX_BASE, CPU_RECV_MAIL_IRQn, CPU_SEND_MAIL_IRQn);
}

__init static void sys_lmac_init(void *ops)
{
    lmac_bgn_module_80211w_init(ops);       //默认打开802.11w支持
    //802.11w之外的模块，都是按需初始化
    if(CoreSetting->lmac_module_init_mask) {
        os_printf("lmac module init:\r\n");
    }
    if(CoreSetting->lmac_module_init_mask & LMAC_MODULE_INIT_BIT_MULTI_MAC) {
        lmac_bgn_module_multi_mac_init(ops);
        _os_printf("    multi mac\r\n");
    }
    if(CoreSetting->lmac_module_init_mask & LMAC_MODULE_INIT_BIT_RX_REORDER) {
        lmac_bgn_module_rx_reorder_init(ops);
        _os_printf("    rx reorder\r\n");
    }
}

__init static void sys_wifi_init(void)
{
    void *ops;
    struct lmac_init_param lparam;
    struct ieee80211_initparam param;

    skbpool_init(CoreSetting->skbpool_addr, CoreSetting->skbpool_addr + CoreSetting->skbpool_size, 80, CoreSetting->skbpool_flag);

    lparam.rxbuf      = CoreSetting->rxbuf_addr;
    lparam.rxbuf_size = CoreSetting->rxbuf_size;
    ops = lmac_bgn_init(&lparam);
    if (ops == NULL) {
        os_printf("LMAC init error\r\n");
        return;
    }

    sys_lmac_init(ops);
    lmac_ops = ops;
    bt_ops = ble_ll_init(ops);
    bt_hci_init(bt_ops);

    // enter wifi test mode
    if (system_is_wifi_test_mode()) {
        return;
    }

#if WPA_CRYPTO_OPS
#ifdef CONFIG_SAE
    ieee80211_crypto_bignum_support();
    ieee80211_crypto_ec_support();
#endif

#ifdef CONFIG_OWE
    ieee80211_crypto_ecdh_support();
#endif

#ifdef CONFIG_WPS
    ieee80211_crypto_aes_support();
#endif
#endif

    os_memset(&param, 0, sizeof(param));
    param.vif_maxcnt = CoreSetting->vif_maxcnt;
    param.sta_maxcnt = CoreSetting->sta_maxcnt;
    param.bss_maxcnt = CoreSetting->bss_maxcnt;
    param.bss_lifetime = CoreSetting->bss_lifetime;
    param.evt_cb = sys_ieee80211_event_cb;
    ieee80211_init(&param);
    ieee80211_support_txw81x(ops);
#ifdef CONFIG_SLEEP
    void dsleep1_init(void *ops);
    dsleep1_init(ops);
#endif
    lmac_afh_init(ops);
}

void sys_print_dbgcache(uint32 *buff, uint32 size)
{
    if (sys_status.dbg_cache) { //打印Cache命中率
        uint32 csi_miss = sys_csi_cache_static(sysctrl_get_cpu_id(), 1);
        uint32 cld_miss = sys_cld_cache_static(1);
        sys_psram_eff_static((cld_miss > 20) || (csi_miss > 30));
    }
}

void sys_print_dbgtop(uint32 *buff, uint32 size)
{
    if (sys_status.dbg_top) {  //打印CPU使用率
        cpu_loading_print(sys_status.dbg_top == 2, (struct os_task_info *)buff, size / sizeof(struct os_task_info));
    }
}

void sys_print_dbgheap(uint32 *buff, uint32 size)
{
    if (sys_status.dbg_heap) { //打印Heap使用情况
        sysheap_status(&sram_heap, buff, size / 4, 0);
        skbpool_status(buff, size / 4, 0);
    }
}

void sys_print_dbgumac(uint32 *buff, uint32 size)
{
    if (sys_status.dbg_umac && !system_is_wifi_test_mode()) { //打印WIFI调试信息
        ieee80211_status((uint8 *)buff, size);
    }
}

static void sys_dbginfo_print(void)
{
    uint32 _print_buf[256];
    static int8   print_interval = 0;

    if (print_interval++ >= 5) { // 5秒打印一次
        void sysctrl_chip_package_permission_handle(void);
        sysctrl_chip_package_permission_handle();
        sys_print_dbgcache(_print_buf, sizeof(_print_buf));
        sys_print_dbgtop(_print_buf, sizeof(_print_buf));
        sys_print_dbgheap(_print_buf, sizeof(_print_buf));
        sys_print_dbgumac(_print_buf, sizeof(_print_buf));
        print_interval = 0;
    }
}

static int32 sys_main_loop(struct os_work *work)
{
    print_level(CoreSetting->print_level);
    disable_print(CoreSetting->disable_print);
    sys_dbginfo_print();
    mcu_watchdog_feed();
    os_run_work_delay(&main_wk, 1000);
    return 0;
}

int main(void)
{
    mcu_watchdog_timeout(CoreSetting->wdt1_to);
    mcu_watchdog_irq_request(CoreSetting->wdt1_irq_hdl);
    sys_cpurpc_init();
    sys_atcmd_init();
    sys_wifi_init();
    OS_WORK_INIT(&main_wk, sys_main_loop, 0);
    os_run_work_delay(&main_wk, 1000);
    CoreSetting->cpu1_ready = 1;//CPU1 ready!!!
    os_printf(KERN_INFO"CPU1 ready!\r\n");
}

