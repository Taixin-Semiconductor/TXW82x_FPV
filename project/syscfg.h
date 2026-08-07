#ifndef _PROJECT_SYSCFG_H_
#define _PROJECT_SYSCFG_H_

enum WIFI_WORK_MODE {
    WIFI_MODE_NONE = 0,
    WIFI_MODE_STA,
    WIFI_MODE_AP,
    WIFI_MODE_APSTA,
};

/* 系统状态信息 */
struct system_status {
    uint32 dhcpc_done: 1,
           wifi_connected: 1,
           dbg_heap: 1,
           dbg_top: 2,
           dbg_lmac: 1,
           dbg_umac: 1,
           dbg_irq: 1,
           upgrading: 1,
           dbg_cache: 1,
           dbg_net: 1;
    int8   rssi;
    int8   evm;
    uint8  channel;
    uint8  wifi_mode;
    uint8  bssid[6];
    uint16 wifi_status_code;
    uint16 wifi_reason_code;
    struct {
        uint32 ipaddr, netmask, svrip, router, dns1, dns2, last_router;
    } dhcpc_result;
};

/* 系统参数信息 */
struct sys_config {
    /******* 参数区头部: 这部分区域不能修改  *****/
    uint16 magic_num, crc;
    uint16 size, rev1, rev2, rev3;
    /*******************************************/

    uint8  wifi_mode, bss_bw, tx_mcs, channel;
    uint8  bssid[6], mac[6];
    uint8  ssid[SSID_MAX_LEN + 1];
    uint8  psk[32];
    uint8  passwd[PASSWD_MAX_LEN + 1];
    uint8  wifi_hwmode;
    uint16 bss_max_idle, beacon_int;
    uint16 ack_tmo, dtim_period;
    uint32 key_mgmt;

    /*标识位*/
    uint32 dhcpc_en: 1,
           dhcpd_en: 1,
           use4addr: 1,
           cfg_init: 1,
           ap_hide: 1,
           xxxxxx: 27;

    uint32 ipaddr, netmask, gw_ip;
    uint32 dhcpd_startip, dhcpd_endip, dhcpd_lease_time, dhcpd_dns1, dhcpd_dns2, dhcpd_router;

    /* 只能在后面添加参数，保证参数区的兼容性 *****
     * 添加参数注意对齐，避免浪费Memory空间                  ******/
    uint32 mipi_csi0_hs_zero_cnt : 16, mipi_csi1_hs_zero_cnt : 16;
    uint32 mipi_csi0_sensor_id   :  8, mipi_csi1_sensor_id   : 8, reserved : 16; 
};

extern struct sys_config sys_cfgs;
extern struct system_status sys_status;

void syscfg_default(void);
int32 wificfg_flush(uint8 ifidx);
void syscfg_check(void);
void syscfg_dump(void);

#endif

