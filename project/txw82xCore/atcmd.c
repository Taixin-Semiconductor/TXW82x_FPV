#include "sys_config.h"
#include "basic_include.h"
#include "lib/atcmd/libatcmd.h"
#include "lib/common/atcmd.h"
#include "syscfg.h"

static const struct hgic_atcmd static_atcmds[] = {
    /* 常用调试 AT指令          */
    { "AT+RST", sys_atcmd_reset },
    { "AT+JTAG", sys_atcmd_jtag },
    { "AT+SYSDBG", sys_atcmd_sysdbg },

    ///////////////////////////////////////////////////
    /* WiFi测试模式 AT指令          */
    { "AT+BSS_BW", atcmd_bss_bw_hdl },
    { "AT+CCA", at_cmd_cca_hdl },
    { "AT+CFG_RX_AGC", at_cmd_cfg_rx_agc_hdl },
    { "AT+CLR_RX_CNT", atcmd_rx_cnt_clr_hdl },
    { "AT+DBG_LEVEL", atcmd_dbg_level_hdl },
    { "AT+EDCA_AIFS", atcmd_edca_aifs_hdl },
    { "AT+EDCA_CW", atcmd_edca_cw_hdl },
    { "AT+EDCA_TXOP", atcmd_edca_txop_hdl },
    { "AT+TX_FRM_TYPE", atcmd_tx_frm_type_hdl },
    { "AT+TX_GAIN", atcmd_tx_gain_hdl },
    { "AT+GET_RX_CNT", atcmd_rx_cnt_get_hdl },
    { "AT+LO_FREQ", atcmd_lo_freq_hdl },
    { "AT+MAC_ADDR", atcmd_mac_addr_hdl },
    { "AT+PRINT_PERIOD", atcmd_lmac_print_period_hdl },
    { "AT+REG_RD", atcmd_reg_rd_hdl },
    { "AT+REG_WT", atcmd_reg_wt_hdl },
    { "AT+SET_XOSC", atcmd_set_xosc_hdl },
    { "AT+SET_TX_DPD_GAIN", atcmd_set_tx_dpd_gain_hdl },
    { "AT+TEST_START", atcmd_test_start_hdl },
    { "AT+TEST_ADDR", atcmd_test_addr_hdl },
    { "AT+TEMP_EN", atcmd_temp_en_hdl },
    { "AT+TX_CONT", atcmd_tx_cont_hdl },
    { "AT+TX_DELAY", atcmd_tx_delay_hdl },
    { "AT+TX_MCS", atcmd_tx_mcs_hdl },

    { "AT+TX_PHA_AMP", atcmd_tx_pha_amp_hdl },
    { "AT+TX_START", atcmd_tx_start_hdl },
    { "AT+TX_STEP", atcmd_tx_step_hdl },
    { "AT+TX_TRIG", atcmd_tx_trig_hdl },
    { "AT+TX_TYPE", atcmd_tx_type_hdl },
    { "AT+WAVE_DUMP", atcmd_wave_dump_hdl },
    { "AT+WRITE_MAC_ADDR", atcmd_write_mac_addr_hdl },
    { "AT+WRITE_TX_DPD_GAIN", atcmd_write_tx_dpd_gain_hdl },
    { "AT+WRITE_XOSC", atcmd_write_xosc_hdl },
    { "AT+CCA_CERT", atcmd_set_cca_cert_hdl },
    { "AT+SRRC", atcmd_set_srrc_hdl },

    { "AT+REBOOT_TEST", sys_atcmd_reboot_test_mode },

#if BLE_SUPPORT  
    {"AT+BLE_START", atcmd_ble_start_hdl},
    {"AT+BLE_TX", atcmd_ble_tx_hdl},
    {"AT+BLE_RX", atcmd_ble_rx_hdl},
    {"AT+BLE_TX_DELAY", atcmd_ble_tx_delay_hdl},
    {"AT+BLE_CLR_RX_CNT", atcmd_ble_rx_cnt_clr_hdl},
    {"AT+BLE_GET_RX_CNT", atcmd_ble_rx_cnt_get_hdl},
    {"AT+BLE_RX_TIMEOUT", atcmd_ble_rx_timeout_hdl},
    {"AT+BLE_CHAN", atcmd_ble_chan_hdl},
    {"AT+BLE_TX_GAIN", atcmd_ble_tx_gain_hdl},
    {"AT+WRITE_BLE_TX_GAIN", atcmd_write_ble_tx_gain_hdl},
    {"AT+IO_TEST", atcmd_io_test_hdl},
    {"AT+GET_IO_TEST_RES", atcmd_io_test_res_get_hdl},
#endif
};

static int32 atcmd_null_write(struct atcmd_dataif *dataif, uint8 *buf, int32 len)
{
    hgprintf_out((char *)buf, len, 0);
	return len;
}
static int32 atcmd_null_read(struct atcmd_dataif *dataif, uint8 *buf, int32 len)
{
    return 0;
}
static const struct atcmd_dataif at_dataif = {
    atcmd_null_write, atcmd_null_read
};

__init void sys_atcmd_init(uint32 uart_dev, uint32 bautrate)
{
    struct atcmd_settings setting;
    os_memset(&setting, 0, sizeof(setting));
    setting.args_count = ATCMD_ARGS_COUNT;
    setting.printbuf_size = ATCMD_PRINT_BUF_SIZE;
    setting.static_atcmds = static_atcmds;
    setting.static_cmdcnt = ARRAY_SIZE(static_atcmds);
    atcmd_init((struct atcmd_dataif *)&at_dataif, &setting);
}

