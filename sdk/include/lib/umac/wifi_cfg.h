#ifndef _HGUC_WIFI_CFG_H_
#define _HGUC_WIFI_CFG_H_
#ifdef __cplusplus
extern "C" {
#endif

int32 wificfg_save(int8 force);

void syscfg_dump(void);

void wificfg_set_mode(uint8 mode);

int32 wificfg_get_mode();

int32 wificfg_set_bss_bw(uint8 ifidx, uint8 bss_bw);

int32 wificfg_get_bss_bw(uint8 ifidx);

int32 wificfg_set_tx_mcs(uint8 tx_mcs);

int32 wificfg_set_channel(uint8 ifidx, uint8 channel);

int32 wificfg_set_chan_list(uint8 ifidx, uint16 *chan_list, uint8 chan_cnt);

int32 wificfg_set_macaddr(uint8 ifidx, uint8 *addr);

int32 wificfg_set_ssid(uint8 ifidx, uint8 *ssid);

int32 wificfg_set_bssid(uint8 ifidx, uint8 *bssid);

int32 wificfg_set_mcast_key(uint8 ifidx, uint8 *key);

int32 wificfg_set_wpa_psk(uint8 ifidx, uint8 *key);

int32 wificfg_get_psk(uint8 *key);

int32 wificfg_set_key_mgmt(uint8 ifidx, uint32 key_mgmt);

int32 wificfg_get_ssid(uint8 ifidx, uint8 ssid[32]);

int32 wificfg_get_bssid(uint8 ifidx, uint8 *bssid);

int32 wificfg_get_wpa_psk(uint8 ifidx, uint8 psk[32]);

int32 wificfg_get_chan_list(uint8 ifidx, uint16 *chan_list, uint8 chan_cnt);

int32 wificfg_set_acs(uint8 ifidx, uint8 acs, uint8 tmo);

int32 wificfg_set_pri_chan(uint8 pri_chan);

int32 wificfg_set_tx_power(uint8 txpower);

int32 wificfg_get_txpower(uint8 ifidx);

int32 wificfg_set_beacon_int(uint8 ifidx, uint32 beacon_int);

int32 wificfg_set_heartbeat_int(uint8 ifidx, uint32 heartbeat_int);

int32 wificfg_set_bss_maxidle(uint8 ifidx, uint32 max_idle);

int32 wificfg_set_wkio_mode(uint8 ifidx, uint8 mode);

int32 wificfg_set_dtim_period(uint8 ifidx, uint32 dtim_period);

int32 wificfg_set_psmode(uint8 ifidx, uint8 psmode);

int32 wificfg_set_wkdata_save(uint8 ifidx, uint8 wkdata_save);

int32 wificfg_set_autosleep(uint8 ifidx, uint8 autosleep);

int32 wificfg_set_psconnect(uint8 period, uint8 roundup);

int32 wificfg_load_def(uint8 reset);

int32 wificfg_set_sleep_aplost_time(uint8 ifidx, uint32 time);

int32 wificfg_set_agg_cnt(uint8 agg_cnt);

int32 wificfg_get_agg_cnt(uint8 ifidx);

int32 wificfg_set_auto_chan_switch(uint8 disable);

int32 wificfg_set_reassoc_wkhost(uint8 ifidx, uint8 enable);

int32 wificfg_set_wakeup_io(uint8 io, uint8 edge);

const char *wificfg_keymgmt_str(uint8 ifidx);

const char *wificfg_wifimode_str(uint8 ifidx);

int32 wificfg_set_auto_sleep_time(uint8 ifidx, int32 time);

int32 wificfg_set_pair_autostop(uint8 ifidx, uint8 en);

int32 wificfg_set_super_pwr_off(uint8 off);

int32 wificfg_set_repeater_ssid(uint8 ifidx, uint8 *ssid);

int32 wificfg_set_repeater_psk(uint8 ifidx, uint8 *data, uint32 data_len);

int32 wificfg_set_auto_save(uint8 ifidx, uint8 off);

int32 wificfg_set_dcdc13(uint8 en);

int32 wificfg_set_acktmo(uint16 tmo);

int32 wificfg_set_pa_pwrctrl_dis(uint8 dis);

int32 wificfg_set_dhcpc_en(uint8 ifidx, uint8 en);

int32 wificfg_set_mcast_txparam(uint8 ifidx, uint8 *data);

int32 wificfg_set_ant_auto_off(uint8 off);

int32 wificfg_set_ant_sel(uint8 sel);

int32 wificfg_set_wkhost_reasons(uint8 ifidx, uint8 *reasons, uint8 count);

int32 wificfg_set_mac_filter_en(uint8 ifidx, uint8 en);

int32 wificfg_get_use2addr(void);

int32 wificfg_set_ap_hide(uint8 ifidx, uint8 en);

int32 wificfg_set_dual_ant(uint8 en);

int32 wificfg_set_max_txcnt(uint8 frm_max, uint8 rts_max);

int32 wificfg_set_dup_filter_en(uint8 en);

int32 wificfg_set_dis_1v1_m2u(uint8 dis);

int32 wificfg_set_dis_psconnect(uint8 dis);

int32 wificfg_get_freq_range(uint8 ifidx, uint16 *freq_start, uint16 *freq_end, uint8 *chan_bw);

int32 wificfg_get_freq_range(uint8 ifidx, uint16 *freq_start, uint16 *freq_end, uint8 *chan_bw);

int32 wificfg_set_cca_for_ce(uint8 en);

int32 wificfg_set_ap_psmode(uint8 en);

int32 wificfg_set_tx_bw(uint8 tx_bw);

int32 wificfg_set_ap_chan_switch(uint8 ifidx, uint8 chan, uint8 counter);

int32 wificfg_get_curr_freq(uint8 ifidx);

int32 wificfg_set_connect_paironly(uint8 en);

int32 wificfg_set_paired_stas(uint8 ifidx, uint8 *mac_list, uint8 cnt);

int32 wificfg_set_wait_psmode(uint8 mode);

int32 wificfg_set_standby(uint8 channel, uint32 period_ms);

int32 wificfg_set_wkupdata_mask(uint8 ifidx, uint8 offset, uint8 *mask, uint32 mask_len);

int32 wificfg_set_wakeup_data(uint8 ifidx, uint8 *data, uint32 data_len);

int32 wificfg_set_wkdata_svr_ip(uint8 ifidx, uint32 svr_ip);

int32 wificfg_set_passwd(uint8 ifidx, uint8 *passwd);

int32 wificfg_set_rpasswd(uint8 ifidx, uint8 *passwd);

int32 wificfg_set_signal_threshold(uint8 ifidx, uint8 rssi_thres, uint8 evm_thres);



#ifdef __cplusplus
}
#endif
#endif
