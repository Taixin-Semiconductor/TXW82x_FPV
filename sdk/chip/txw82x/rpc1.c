#include "basic_include.h"
#include "lib/rpc/cpurpc.h"
#include "lib/common/rbuffer.h"
#include "lib/common/sysevt.h"
#include "lib/skb/skbpool.h"
#include "lib/umac/ieee80211.h"
#include "lib/audio/audio_code/audio_code.h"
#include "lib/lvgl_rotate_rpc/lvgl_rotate_rpc.h"

extern int32 lmac_ioctl_rpc(void * lops, uint32 cmd, uint32 param1, uint32 param2);
extern int32 hci_controller_recv(uint32 data, uint32 len);
extern int32 atcmd_recv(uint8 *data, int32 len);
extern void sys_enter_sleep(void);

static int32 cpu1_run_func(void *func, uint32 p1, uint32 p2, uint32 p3)
{
    if(func){
        return ((uint32(*)(uint32, uint32, uint32))func)(p1, p2, p3);
    }
    return -EINVAL;
}
static void *cpu1_new_task(const char * name, os_task_func_t func, void * args, uint32 prio, uint32 time, void * stack, uint32 stack_size)
{
    return os_task_create(name, func, args, prio, time, stack, stack_size);
}
static int32 cpu1_atcmd_recv(char *data, uint32 len)
{
    return atcmd_recv((uint8 *)data, len);
}

static const void *rpc_funcs[CPU1_RPC_FUNCID_NUM] = {
    RPC_FUNC_DEF(sys_enter_sleep),
    RPC_FUNC_DEF(cpu1_run_func),
    RPC_FUNC_DEF(cpu1_atcmd_recv),
    RPC_FUNC_DEF(lmac_ioctl_rpc),
    RPC_FUNC_DEF(skbpool_add_region),
    RPC_FUNC_DEF(ieee80211_deliver_init),
    RPC_FUNC_DEF(ieee80211_iface_create_ap),
    RPC_FUNC_DEF(ieee80211_iface_create_sta),
    RPC_FUNC_DEF(ieee80211_iface_start),
    RPC_FUNC_DEF(ieee80211_iface_stop),
    RPC_FUNC_DEF(ieee80211_pairing),
    RPC_FUNC_DEF(ieee80211_unpair),
    RPC_FUNC_DEF(ieee80211_scan),
    RPC_FUNC_DEF(ieee80211_scatter_tx),
    RPC_FUNC_DEF(ieee80211_tx),
    RPC_FUNC_DEF(ieee80211_input),
    RPC_FUNC_DEF(ieee80211_tx_mgmt),
    RPC_FUNC_DEF(ieee80211_tx_custmgmt),
    RPC_FUNC_DEF(ieee80211_tx_ether),
    RPC_FUNC_DEF(ieee80211_disassoc),
    RPC_FUNC_DEF(ieee80211_disassoc_all),
    RPC_FUNC_DEF(ieee80211_hook_ext_data),
    RPC_FUNC_DEF(ieee80211_cleanup_bsslist),
    RPC_FUNC_DEF(ieee80211_get_bsslist),
    RPC_FUNC_DEF(ieee80211_get_stalist),
    RPC_FUNC_DEF(ieee80211_conf_set_bssbw),
    RPC_FUNC_DEF(ieee80211_conf_get_bssbw),
    RPC_FUNC_DEF(ieee80211_conf_set_chanlist),
    RPC_FUNC_DEF(ieee80211_conf_set_ssid),
    RPC_FUNC_DEF(ieee80211_conf_get_ssid),
    RPC_FUNC_DEF(ieee80211_conf_set_keymgmt),
    RPC_FUNC_DEF(ieee80211_conf_get_keymgmt),
    RPC_FUNC_DEF(ieee80211_conf_set_psk),
    RPC_FUNC_DEF(ieee80211_conf_set_passwd),
    RPC_FUNC_DEF(ieee80211_conf_get_psk),
    RPC_FUNC_DEF(ieee80211_conf_set_beacon_int),
    RPC_FUNC_DEF(ieee80211_conf_set_dtim_int),
    RPC_FUNC_DEF(ieee80211_conf_set_bssid),
    RPC_FUNC_DEF(ieee80211_conf_get_bssid),
    RPC_FUNC_DEF(ieee80211_conf_set_channel),
    RPC_FUNC_DEF(ieee80211_conf_get_channel),
    RPC_FUNC_DEF(ieee80211_conf_get_mac),
    RPC_FUNC_DEF(ieee80211_conf_set_mac),
    RPC_FUNC_DEF(ieee80211_conf_set_bss_max_idle),
    RPC_FUNC_DEF(ieee80211_conf_get_connstate),
    RPC_FUNC_DEF(ieee80211_conf_get_wkreason),
    RPC_FUNC_DEF(ieee80211_conf_wakeup_sta),
    RPC_FUNC_DEF(ieee80211_conf_get_txpower),
    RPC_FUNC_DEF(ieee80211_conf_set_heartbeat_int),
    RPC_FUNC_DEF(ieee80211_conf_set_aplost_time),
    RPC_FUNC_DEF(ieee80211_conf_set_acs),
    RPC_FUNC_DEF(ieee80211_conf_set_wmm_enable),
    RPC_FUNC_DEF(ieee80211_conf_set_use4addr),
    RPC_FUNC_DEF(ieee80211_conf_get_stainfo),
    RPC_FUNC_DEF(ieee80211_conf_get_stalist),
    RPC_FUNC_DEF(ieee80211_conf_get_sta_snr),
    RPC_FUNC_DEF(ieee80211_conf_get_stacnt),
    RPC_FUNC_DEF(ieee80211_conf_set_aphide),
    RPC_FUNC_DEF(ieee80211_conf_get_bgrssi),
    RPC_FUNC_DEF(ieee80211_conf_set_mcast_txrate),
    RPC_FUNC_DEF(ieee80211_conf_set_hwmode),
    RPC_FUNC_DEF(ieee80211_conf_set_psdata_cnt),
    RPC_FUNC_DEF(ieee80211_conf_set_wmm_param),
    RPC_FUNC_DEF(ieee80211_conf_get_wmm_param),
    RPC_FUNC_DEF(ieee80211_conf_set_wpa_group_rekey),
    RPC_FUNC_DEF(ieee80211_conf_set_datatag),
    RPC_FUNC_DEF(ieee80211_conf_get_ant_sel),
    RPC_FUNC_DEF(ieee80211_conf_get_reason_code),
    RPC_FUNC_DEF(ieee80211_conf_get_status_code),
    RPC_FUNC_DEF(ieee80211_conf_get_rtc),
    RPC_FUNC_DEF(ieee80211_conf_get_acs_result),
    RPC_FUNC_DEF(ieee80211_conf_set_rtc),
    RPC_FUNC_DEF(ieee80211_conf_set_radio_onoff),
    RPC_FUNC_DEF(ieee80211_conf_set_isolate),
    RPC_FUNC_DEF(ieee80211_conf_set_ft),
    RPC_FUNC_DEF(ieee80211_pair_enable),
    RPC_FUNC_DEF(hci_controller_recv),
    RPC_FUNC_DEF(audio_coder_run),
    RPC_FUNC_DEF(lvgl_frame_rotate_rpc_init),
    RPC_FUNC_DEF(lvgl_frame_rotate_rpc),
    RPC_FUNC_DEF(ieee80211_conf_set_pair_ngo),
    RPC_FUNC_DEF(ieee80211_conf_set_mutl_pair),
    RPC_FUNC_DEF(ieee80211_conf_set_linkcost_param),
    RPC_FUNC_DEF(ieee80211_conf_set_pair_channel),
    RPC_FUNC_DEF(ieee80211_conf_get_tx_mcs),
    RPC_FUNC_DEF(ieee80211_conf_set_tx_mcs),
    RPC_FUNC_DEF(ieee80211_iface_create_rmesh),
    RPC_FUNC_DEF(ieee80211_conf_set_rmesh_devmax),
    RPC_FUNC_DEF(ieee80211_conf_set_rmesh_aid),
    RPC_FUNC_DEF(ieee80211_conf_set_rmesh_device),
    RPC_FUNC_DEF(ieee80211_conf_set_bss_disable),
    RPC_FUNC_DEF(ieee80211_conf_set_rmesh_notfw),
    RPC_FUNC_DEF(ieee80211_conf_set_rmesh_rssimin),
    RPC_FUNC_DEF(ieee80211_conf_get_rmesh_device),
    RPC_FUNC_DEF(cpu1_new_task),
    RPC_FUNC_DEF(audio_code_frame),
};

const void *cpu_rpc_func(uint32 func_id)
{
    if (func_id < ARRAY_SIZE(rpc_funcs)) {
        return rpc_funcs[func_id];
    }
    return NULL;
}

