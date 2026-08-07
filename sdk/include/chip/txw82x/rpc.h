#ifndef _TXW82X_RPC_FUNCID_H_
#define _TXW82X_RPC_FUNCID_H_

#define RPC_FUNC_ID(f)        RPC_FUNCID_##f

enum CPU0_RPC_FUNCID {
    RPC_FUNC_ID(sys_wifi_register) = 2, /*ID 1 空出来*/
    RPC_FUNC_ID(sys_wifi_recv),
    RPC_FUNC_ID(sys_event_new),
    RPC_FUNC_ID(sys_wifi_event_cb),
    RPC_FUNC_ID(atcmd_recv),

    //BT
    RPC_FUNC_ID(hci_host_recv),

	//lvgl
	RPC_FUNC_ID(lvgl_frame_rotate_rpc_sync),

    /////////////////////////////////
    CPU0_RPC_FUNCID_NUM,
};

enum CPU1_RPC_FUNCID {
	RPC_FUNC_ID(sys_enter_sleep), //SLEEP: 固定为 ID 1
    RPC_FUNC_ID(cpu1_run_func),
    RPC_FUNC_ID(cpu1_atcmd_recv),
    RPC_FUNC_ID(lmac_ioctl_rpc),
    RPC_FUNC_ID(skbpool_add_region),
    RPC_FUNC_ID(ieee80211_deliver_init),
    RPC_FUNC_ID(ieee80211_iface_create_ap),
    RPC_FUNC_ID(ieee80211_iface_create_sta),
    RPC_FUNC_ID(ieee80211_iface_start),
    RPC_FUNC_ID(ieee80211_iface_stop),
    RPC_FUNC_ID(ieee80211_pairing),
    RPC_FUNC_ID(ieee80211_unpair),
    RPC_FUNC_ID(ieee80211_scan),
    RPC_FUNC_ID(ieee80211_scatter_tx),
    RPC_FUNC_ID(ieee80211_tx),
    RPC_FUNC_ID(ieee80211_input),
    RPC_FUNC_ID(ieee80211_tx_mgmt),
    RPC_FUNC_ID(ieee80211_tx_custmgmt),
    RPC_FUNC_ID(ieee80211_tx_ether),
    RPC_FUNC_ID(ieee80211_disassoc),
    RPC_FUNC_ID(ieee80211_disassoc_all),
    RPC_FUNC_ID(ieee80211_hook_ext_data),
    RPC_FUNC_ID(ieee80211_cleanup_bsslist),
    RPC_FUNC_ID(ieee80211_get_bsslist),
    RPC_FUNC_ID(ieee80211_get_stalist),
    RPC_FUNC_ID(ieee80211_conf_set_bssbw),
    RPC_FUNC_ID(ieee80211_conf_get_bssbw),
    RPC_FUNC_ID(ieee80211_conf_set_chanlist),
    RPC_FUNC_ID(ieee80211_conf_set_ssid),
    RPC_FUNC_ID(ieee80211_conf_get_ssid),
    RPC_FUNC_ID(ieee80211_conf_set_keymgmt),
    RPC_FUNC_ID(ieee80211_conf_get_keymgmt),
    RPC_FUNC_ID(ieee80211_conf_set_psk),
    RPC_FUNC_ID(ieee80211_conf_set_passwd),
    RPC_FUNC_ID(ieee80211_conf_get_psk),
    RPC_FUNC_ID(ieee80211_conf_set_beacon_int),
    RPC_FUNC_ID(ieee80211_conf_set_dtim_int),
    RPC_FUNC_ID(ieee80211_conf_set_bssid),
    RPC_FUNC_ID(ieee80211_conf_get_bssid),
    RPC_FUNC_ID(ieee80211_conf_set_channel),
    RPC_FUNC_ID(ieee80211_conf_get_channel),
    RPC_FUNC_ID(ieee80211_conf_get_mac),
    RPC_FUNC_ID(ieee80211_conf_set_mac),
    RPC_FUNC_ID(ieee80211_conf_set_bss_max_idle),
    RPC_FUNC_ID(ieee80211_conf_get_connstate),
    RPC_FUNC_ID(ieee80211_conf_get_wkreason),
    RPC_FUNC_ID(ieee80211_conf_wakeup_sta),
    RPC_FUNC_ID(ieee80211_conf_get_txpower),
    RPC_FUNC_ID(ieee80211_conf_set_heartbeat_int),
    RPC_FUNC_ID(ieee80211_conf_set_aplost_time),
    RPC_FUNC_ID(ieee80211_conf_set_acs),
    RPC_FUNC_ID(ieee80211_conf_set_wmm_enable),
    RPC_FUNC_ID(ieee80211_conf_set_use4addr),
    RPC_FUNC_ID(ieee80211_conf_get_stainfo),
    RPC_FUNC_ID(ieee80211_conf_get_stalist),
    RPC_FUNC_ID(ieee80211_conf_get_sta_snr),
    RPC_FUNC_ID(ieee80211_conf_get_stacnt),
    RPC_FUNC_ID(ieee80211_conf_set_aphide),
    RPC_FUNC_ID(ieee80211_conf_get_bgrssi),
    RPC_FUNC_ID(ieee80211_conf_set_mcast_txrate),
    RPC_FUNC_ID(ieee80211_conf_set_hwmode),
    RPC_FUNC_ID(ieee80211_conf_set_psdata_cnt),
    RPC_FUNC_ID(ieee80211_conf_set_wmm_param),
    RPC_FUNC_ID(ieee80211_conf_get_wmm_param),
    RPC_FUNC_ID(ieee80211_conf_set_wpa_group_rekey),
    RPC_FUNC_ID(ieee80211_conf_set_datatag),
    RPC_FUNC_ID(ieee80211_conf_get_ant_sel),
    RPC_FUNC_ID(ieee80211_conf_get_reason_code),
    RPC_FUNC_ID(ieee80211_conf_get_status_code),
    RPC_FUNC_ID(ieee80211_conf_get_rtc),
    RPC_FUNC_ID(ieee80211_conf_get_acs_result),
    RPC_FUNC_ID(ieee80211_conf_set_rtc),
    RPC_FUNC_ID(ieee80211_conf_set_radio_onoff),
    RPC_FUNC_ID(ieee80211_conf_set_isolate),
    RPC_FUNC_ID(ieee80211_conf_set_ft),
    RPC_FUNC_ID(ieee80211_pair_enable),

    //BT
    RPC_FUNC_ID(hci_controller_recv),

    //audio
    RPC_FUNC_ID(audio_coder_run),

    //lvgl
    RPC_FUNC_ID(lvgl_frame_rotate_rpc_init),
    RPC_FUNC_ID(lvgl_frame_rotate_rpc),

    /////////////////////////////////
    CPU1_RPC_FUNCID_NUM,
};

#endif
