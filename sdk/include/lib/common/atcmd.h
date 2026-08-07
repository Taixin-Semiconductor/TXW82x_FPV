
#ifndef _COMMON_ATCMD_H_
#define _COMMON_ATCMD_H_
#ifdef __cplusplus
extern "C" {
#endif

int32 sys_atcmd_loaddef(const char *cmd, char *argv[], uint32 argc);
int32 sys_atcmd_reset(const char *cmd, char *argv[], uint32 argc);
int32 sys_atcmd_jtag(const char *cmd, char *argv[], uint32 argc);
int32 sys_syscfg_dump_hdl(const char *cmd, char *argv[], uint32 argc);
int32 sys_atcmd_errlog(const char *cmd, char *argv[], uint32 argc);
int32 sys_atcmd_sysdbg(const char *cmd, char *argv[], uint32 argc);
int32 sys_atcmd_disprintf(const char *cmd, char *argv[], uint32 argc);
int32 xmodem_fwupgrade_hdl(const char *cmd, char *argv[], uint32 count);
int32 sys_heap_dump_hdl(const char *cmd, char *argv[], uint32 argc);
int32 sys_atcmd_watchdog(const char *cmd, char *argv[], uint32 argc);

int32 sys_wifi_atcmd_set_channel(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_set_bssid(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_set_encrypt(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_set_ssid(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_set_key(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_set_rssid(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_set_rkey(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_set_rmode(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_set_wifimode(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_loaddef(const char *cmd, char *argv[], uint32 argc);

int32 sys_wifi_atcmd_scan(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_pair(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_aphide(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_hwmode(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_ft(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_passwd(const char *cmd, char *argv[], uint32 argc);

int32 sys_ble_atcmd_blenc(const char *cmd, char *argv[], uint32 argc);
int32 sys_ble_atcmd_set_coexist_en(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_ble_start_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_ble_tx_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_ble_rx_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_ble_rx_cnt_clr_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_ble_rx_cnt_get_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_ble_tx_delay_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_ble_chan_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_ble_rx_timeout_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_ble_tx_gain_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_write_ble_tx_gain_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_io_test_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_io_test_res_get_hdl(const char *cmd, char *argv[], uint32 argc);

int32 sys_atcmd_ping(const char *cmd, char *argv[], uint32 argc);
int32 sys_atcmd_icmp_mntr(const char *cmd, char *argv[], uint32 argc);
int32 sys_atcmd_iperf2(const char *cmd, char *argv[], uint32 argc);
int32 sys_atcmd_goto_boot(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_reboot_test_mode(const char *cmd, char *argv[], uint32 argc);

int32 sys_wifi_atcmd_pcap(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_wificsa(const char *cmd, char *argv[], uint32 argc);

int32 udp_test_atcmd_hdl(const char *cmd, char *argv[], uint32 argc);
int32 tcp_test_atcmd_hdl(const char *cmd, char *argv[], uint32 argc);

int32 sys_get_gpio_imap(const char *cmd, char *argv[], uint32 argc);
int32 sys_get_gpio_omap(const char *cmd, char *argv[], uint32 argc);

#ifdef __cplusplus
}
#endif
#endif
