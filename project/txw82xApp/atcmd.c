#include "sys_config.h"
#include "basic_include.h"
#include "lib/atcmd/libatcmd.h"
#include "lib/common/atcmd.h"
#include "syscfg.h"

// extern int32 app_atcmd_check_heap(const char *cmd, char *argv[], uint32 argc);
// int32 app_atcmd_dbg(const char *cmd, char *argv[], uint32 argc);
extern int32 cpu1_atcmd_recv(char *data, uint32 len);

int32 sys_atcmd_uf_test(const char *cmd, char *argv[], uint32 argc);
int32 txmplayer_test_atcmd(const char *cmd, char *argv[], uint32 argc);
int32 atcmd_record_audio(const char *cmd, char *argv[], uint32 argc);

int32 sys_empty_atcmd(const char *cmd, char *argv[], uint32 argc)
{
    char *str = argv[0];
    int32 len = argc;
    if(os_strncmp("xxx", str, 3) == 0){ //自定义cmd
        //执行自定义AT命令
    }else{ //默认传递给cpu1
        if(os_strncasecmp(str, "AT1+", 4) == 0){
            str[1]='A'; str[2]='T'; 
            str++; len--;
        }
        cpu1_atcmd_recv(str, len);
    }
    return ATCMD_RESULT_DONE;
}

static const struct hgic_atcmd static_atcmds[] = {
    ///////////////////////////////////////////////////
    /* 常用调试 AT指令          */
    { "AT+RST", sys_atcmd_reset },
    { "AT+JTAG", sys_atcmd_jtag },
    { "AT+SYSDBG", sys_atcmd_sysdbg },
    { "AT+SYSCFG", sys_syscfg_dump_hdl },
    { "AT+FWUPG", xmodem_fwupgrade_hdl },
    { "AT+ERRLOG", sys_atcmd_errlog },
    { "AT+LOADDEF", sys_atcmd_loaddef },
    { "AT+HEAP", sys_heap_dump_hdl },
	{ "AT+INMAP", sys_get_gpio_imap},
	{ "AT+OUTMAP", sys_get_gpio_omap},
    { "AT+VCAM2", sys_vcam2_conflict_detect},
	{ NULL, sys_empty_atcmd},  //用于调用cpu1的atcmd

    /* WiFi参数设置 AT指令          */
    { "AT+CHANNEL", sys_wifi_atcmd_set_channel },
    { "AT+BSSID", sys_wifi_atcmd_set_bssid },
    { "AT+ENCRYPT", sys_wifi_atcmd_set_encrypt },
    { "AT+SSID", sys_wifi_atcmd_set_ssid },
    { "AT+KEY", sys_wifi_atcmd_set_key },
    { "AT+WIFIMODE", sys_wifi_atcmd_set_wifimode },
    { "AT+SCAN", sys_wifi_atcmd_scan },
#if SYS_WIFI_PAIR
    { "AT+PAIR", sys_wifi_atcmd_pair },
#endif
    { "AT+APHIDE", sys_wifi_atcmd_aphide },
    { "AT+HWMODE", sys_wifi_atcmd_hwmode },
#if WIFI_RMESH_SUPPORT
    { "AT+RMESH", sys_wifi_atcmd_set_rmesh },
#endif

#if BLE_SUPPORT  
    { "AT+BLENC", sys_ble_atcmd_blenc },
    { "AT+BLE_COEXIST", sys_ble_atcmd_set_coexist_en },
#endif    
//    { "AT+WIFICSA", sys_wifi_atcmd_wificsa },

    ///////////////////////////////////////////////////
    /* 应用/测试 AT指令          */
    { "AT+PING", sys_atcmd_ping },
    { "AT+IPERF2", sys_atcmd_iperf2},
    //{ "AT+TCPTEST", tcp_test_atcmd_hdl },
    { "AT+ICMP_MNTR", sys_atcmd_icmp_mntr },

//    { "AT+PCAP", sys_wifi_atcmd_pcap },

#if COZE_DEMO
    {"AT+UPLOAD_PHOTO",sys_atcmd_coze_upload_photo},
#endif
    /*
        继续添加其他AT命令
        ....
    */
    ///////////////////////////////////////////////////
#ifdef CONFIG_SLEEP
    /* 休眠相关AT指令          */
    { "AT+SLEEP_DBG", atcmd_sleep_dbg_hdl },
    { "AT+SLEEP", atcmd_sleep_hdl },
//    { "AT+DTIM", atcmd_dtim_hdl },
#endif

    // { "AT+APP_HEAP", app_atcmd_check_heap },
    // { "AT+APP_DBG", app_atcmd_dbg },

	//{ "AT+UF_TEST", sys_atcmd_uf_test},

	{ "AT+TXMPLAYER", txmplayer_test_atcmd},
	{ "AT+MSI", atcmd_dump_msi_hdl},
    { "AT+RECORD_AUDIO", atcmd_record_audio },
};

__init void sys_atcmd_init(void)
{
    struct atcmd_settings setting;
    os_memset(&setting, 0, sizeof(setting));
    setting.args_count = ATCMD_ARGS_COUNT;
    setting.printbuf_size = ATCMD_PRINT_BUF_SIZE;
    setting.static_atcmds = static_atcmds;
    setting.static_cmdcnt = ARRAY_SIZE(static_atcmds);
    atcmd_uart_init(ATCMD_UARTDEV, 921600, 5, &setting);
}

