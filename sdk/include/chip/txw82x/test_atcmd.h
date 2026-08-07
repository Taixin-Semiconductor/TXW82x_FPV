#ifndef _TXW82X_TEST_ATCMD_H_
#define _TXW82X_TEST_ATCMD_H_

#ifdef __cplusplus
 extern "C" {
#endif

int32 atcmd_reg_rd_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_reg_wt_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_test_start_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_test_addr_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_temp_en_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_tx_type_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_tx_rf_pwr_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_tx_pha_amp_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_tx_step_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_tx_cont_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_tx_start_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_tx_trig_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_tx_frm_type_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_tx_gain_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_tx_mcs_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_lo_freq_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_mac_addr_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_edca_cw_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_edca_aifs_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_edca_txop_hdl(const char *cmd, char *argv[], uint32 argc);
int32 at_cmd_cfg_rx_agc_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_lmac_print_period_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_bss_bw_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_set_xosc_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_set_tx_dpd_gain_hdl(const char *cmd, char *argv[], uint32 argc);
int32 at_cmd_cca_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_rx_cnt_clr_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_rx_cnt_get_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_tx_delay_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_wave_dump_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_write_mac_addr_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_write_tx_dpd_gain_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_write_xosc_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_set_cca_cert_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_set_srrc_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_dbg_level_hdl(const char *cmd, char *argv[], uint32 argc);
int32 sys_atcmd_reset(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_sleep_dbg_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_sleep_alg_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_sleep_hdl(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_dtim_hdl(const char *cmd, char *argv[], uint32 argc);


#ifdef __cplusplus
}
#endif

#endif

