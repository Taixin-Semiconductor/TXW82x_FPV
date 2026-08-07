#include "sys_config.h"
#include "basic_include.h"
#include "hal/adc.h"
#include "lib/rpc/cpurpc.h"
#include "lib/atcmd/libatcmd.h"
#include "lib/common/atcmd.h"
#include "lib/umac/ieee80211.h"
#include "lib/lmac/lmac.h"
#include "syscfg.h"

__init static void sys_wifi_start_acs(void)
{
    void *ops = NULL;   //另外一个CPU处理
    struct lmac_acs_ctl acs_ctl;

    if (sys_cfgs.wifi_mode == WIFI_MODE_AP) {
        if (sys_cfgs.channel == 0) {
            acs_ctl.enable     = 1;
            acs_ctl.scan_ms    = WIFI_ACS_SCAN_TIME;;
            acs_ctl.chn_bitmap = WIFI_ACS_CHAN_LISTS;
            lmac_start_async_acs(ops, &acs_ctl);
        }
    }
}

__init static void sys_wifi_parameter_init(void)
{
    void *ops = NULL;   //另外一个CPU处理
    lmac_set_rf_pwr_level(ops, WIFI_RF_PWR_LEVEL);
#if WIFI_FEM_CHIP != LMAC_FEM_NONE
    lmac_fem_pin_func(1);
    lmac_set_fem(ops, WIFI_FEM_CHIP);   //初始化FEM之后，不能进行RF档位选择！
#endif
    lmac_set_rts(ops, WIFI_RTS_THRESHOLD);
    lmac_set_retry_cnt(ops, WIFI_TX_MAX_RETRY, WIFI_RTS_MAX_RETRY);
    lmac_set_supp_rate(ops, WIFI_TX_SUPP_RATE);
    lmac_set_mcast_dup_txcnt(ops, WIFI_MULICAST_RETRY);
    lmac_set_tx_duty_cycle(ops, WIFI_TX_DUTY_CYCLE);
#if WIFI_SSID_FILTER_EN
    lmac_set_ssid_filter(ops, sys_cfgs.ssid, strlen(sys_cfgs.ssid));
#endif
#if WIFI_PREVENT_PS_MODE_EN
    lmac_set_prevent_ps_mode(ops, WIFI_PREVENT_PS_MODE_EN);
#endif
    lmac_set_ps_no_frm_loss(ops, WIFI_PS_NO_FRM_LOSS_EN);

#ifdef RATE_CONTROL_SELECT
    lmac_rate_ctrl_type(ops, RATE_CONTROL_SELECT);
#endif
#if !WIFI_METER_TEST_EN
    uint8 gain_table[] = {
        125, 125, 100, 100, 80, 80, 56, 56,     // NON HT OFDM
        125, 100, 100, 80,  80, 56, 56, 56,     // HT
        80,  80,  80,  80,                      // DSSS
    };
    lmac_set_tx_modulation_gain(ops, gain_table, sizeof(gain_table));
    lmac_set_temperature_compesate_en(ops, WIFI_TEMPERATURE_COMPESATE_EN);
#endif
    lmac_set_freq_offset_track_mode(ops, WIFI_FREQ_OFFSET_TRACK_MODE);

#if WIFI_HIGH_PRIORITY_TX_MODE_EN
    struct lmac_txq_param txq_max;
    struct lmac_txq_param txq_min;

    txq_max.aifs   = 0xFF;                  //不限制aifs
    txq_max.txop   = 0xFFFF;                //不限制txop
    txq_max.cw_min = 1;                     //cwmin最大值。如果觉得冲突太厉害，可以改成3
    txq_max.cw_max = 3;                     //cwmax最大值。如果觉得冲突太厉害，可以改成7
    lmac_set_edca_max(ops, &txq_max);
    lmac_set_tx_edca_slot_time(ops, 6);     //6us是其他客户推荐的值
    lmac_set_nav_max(ops, 0);               //完全关闭NAV功能

    txq_min.aifs   = 0;
    txq_min.txop   = 100;                   //txop最小值限制为100
    txq_min.cw_min = 0;
    txq_min.cw_max = 0;
    lmac_set_edca_min(ops, &txq_min);
#endif

    lmac_set_aggcnt(ops, 0);
    lmac_set_rx_aggcnt(ops, 0);

#ifdef CONFIG_SLEEP
    void *bgn_dsleep_init(void *ops);
    bgn_dsleep_init(ops);
#endif
}

__init void sys_wifi_test_mode_init(void)
{
    uint8 default_gain_table[] = {          //gain默认值
        64, 64, 64, 64, 64, 64, 64, 64,     //OFDM
        64, 64, 64, 64, 64, 64, 64, 64,     //HT
        64, 64, 64, 64,                     //DSSS
    };

    lmac_set_mac_addr(NULL, 0, sys_cfgs.mac);
    lmac_set_tx_modulation_gain(NULL, default_gain_table, sizeof(default_gain_table));
#if WIFI_FEM_CHIP != LMAC_FEM_NONE
    lmac_set_fem(NULL, WIFI_FEM_CHIP);   //初始化FEM之后，不能进行RF档位选择！
#endif
}

__init static void sys_wifi_ap_init(void)
{
    ieee80211_iface_create_ap(WIFI_MODE_AP, IEEE80211_BAND_2GHZ);
    if (sys_cfgs.wifi_mode == WIFI_MODE_AP) {
        wificfg_flush(WIFI_MODE_AP);
        ieee80211_conf_set_psdata_cnt(WIFI_MODE_AP, 100);
        ieee80211_iface_start(WIFI_MODE_AP);
    }
}

__init static void sys_wifi_sta_init(void)
{
    ieee80211_iface_create_sta(WIFI_MODE_STA, IEEE80211_BAND_2GHZ);
    if (sys_cfgs.wifi_mode == WIFI_MODE_STA) {
        wificfg_flush(WIFI_MODE_STA);
        ieee80211_iface_start(WIFI_MODE_STA);
    }
}

/* p2p功能需要使用 AP/STA 功能，p2p默认复用了AP/STA接口。
   如果需要同时使用AP/STA功能和p2p功能，则需要为 p2p 创建专用的ap/sta接口，WIFI_WORK_MODE为p2p添加定义
*/
__init static void sys_wifi_p2p_init(void)
{
#if WIFI_P2P_SUPPORT
    ieee80211_iface_create_ap(WIFI_MODE_AP, IEEE80211_BAND_2GHZ);
    //wificfg_flush(WIFI_MODE_AP);
    ieee80211_conf_set_mac(WIFI_MODE_AP, sys_cfgs.mac);
    ieee80211_iface_create_sta(WIFI_MODE_STA, IEEE80211_BAND_2GHZ);
    ieee80211_conf_stabr_table(WIFI_MODE_STA, 128, 10 * 60);
    //wificfg_flush(WIFI_MODE_STA);
    ieee80211_conf_set_mac(WIFI_MODE_STA, sys_cfgs.mac);
    ieee80211_iface_create_p2pdev(WIFI_MODE_P2P, IEEE80211_BAND_2GHZ, WIFI_MODE_AP, WIFI_MODE_STA);
    //wificfg_flush(WIFI_MODE_P2P);
    ieee80211_conf_set_mac(WIFI_MODE_P2P, sys_cfgs.mac);
    ieee80211_conf_set_ssid(WIFI_MODE_P2P, sys_cfgs.ssid);
    ieee80211_conf_set_keymgmt(WIFI_MODE_P2P, sys_cfgs.key_mgmt);
    ieee80211_conf_set_psk(WIFI_MODE_P2P, sys_cfgs.psk);
    ieee80211_iface_start(WIFI_MODE_P2P);
#endif
}

__init void sys_wifi_init(void)
{
    sys_wifi_parameter_init();

    ieee80211_deliver_init(16, 60);

#if WIFI_AP_SUPPORT
    sys_wifi_ap_init();
#endif

#if WIFI_STA_SUPPORT
    sys_wifi_sta_init();
#endif

#if WIFI_P2P_SUPPORT
    sys_wifi_p2p_init();
#endif

    sys_wifi_start_acs();
}

