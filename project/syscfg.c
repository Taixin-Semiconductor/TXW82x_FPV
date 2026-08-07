#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "osal/task.h"
#include "osal/sleep.h"
#include "osal/timer.h"
#include "osal/work.h"
#include "hal/gpio.h"
#include "hal/uart.h"
#include "lib/common/common.h"
#include "lib/umac/ieee80211.h"
#include "lib/syscfg/syscfg.h"
#include "lib/atcmd/libatcmd.h"
#include "lib/net/dhcpd/dhcpd.h"
#include "lwip/ip_addr.h"
#include "lwip/netdb.h"
#include "netif/ethernetif.h"
#include "syscfg.h"

#define SYSCFG_WIFI_MODE(mode) ((mode) == WIFI_MODE_STA ? "sta" : ((mode) == WIFI_MODE_AP ? "ap" : "apsta"))

struct sys_config sys_cfgs = {
    .wifi_mode              = WIFI_MODE_DEFAULT,
    .wifi_hwmode            = WIFI_HWMODE_DEFAULT,
    .channel                = WIFI_CHANNEL_DEFAULT,
    .beacon_int             = BEACON_INTVAL_DEFAULT,
    .dtim_period            = DTIM_PERIOD_DEFAULT,
    .bss_max_idle           = BSS_MAX_IDLE_DEFAULT,
    .key_mgmt               = KEY_MGMT_DEFAULT,
    .bss_bw                 = WIFI_BSSBW_DEFAULT,
    .ipaddr                 = NET_IP_ADDR_DEFAULT,
    .netmask                = NET_MASK_DEFAULT,
    .gw_ip                  = NET_GW_IP_DEFAULT,
    .dhcpd_startip          = DHCPD_START_IP_DEFAULT,
    .dhcpd_endip            = DHCPD_END_IP_DEFAULT,
    .dhcpd_lease_time       = DHCPD_LEASETIME_DEFAULT,
    .dhcpd_dns1             = DHCPD_DNS1_DEFAULT,
    .dhcpd_dns2             = DHCPD_DNS2_DEFAULT,
    .dhcpd_router           = DHCPD_ROUTER_DEFAULT,
    .dhcpd_en               = DHCPD_EN,
    .dhcpc_en               = DHCPC_EN,
    .mipi_csi0_hs_zero_cnt  = 0,
    .mipi_csi1_hs_zero_cnt  = 0,
};

static void wificfg_custom_ap_wmm_param(uint8 ifidx)
{
#ifdef SYS_CUSTOM_APWMM
    const uint8 txq[][8] = { //使用自定义的wmm 参数
        {0x4F, 0x00, 0x02, 0x00, 0x09, 0x00, 0x01, 0x00,},
        {0x4F, 0x00, 0x02, 0x00, 0x09, 0x00, 0x01, 0x01,},
        {0x80, 0x00, 0x02, 0x00, 0x04, 0x00, 0x01, 0x02,},
        {0x41, 0x00, 0x02, 0x00, 0x03, 0x00, 0x01, 0x03,},
    };

    if (sys_cfgs.wifi_mode == WIFI_MODE_AP && ifidx == WIFI_MODE_AP) {
        ieee80211_conf_set_wmm_param(WIFI_MODE_AP, 0xf0, (struct ieee80211_wmm_param *)&txq[0]);
        ieee80211_conf_set_wmm_param(WIFI_MODE_AP, 0xf1, (struct ieee80211_wmm_param *)&txq[1]);
        ieee80211_conf_set_wmm_param(WIFI_MODE_AP, 0xf2, (struct ieee80211_wmm_param *)&txq[2]);
        ieee80211_conf_set_wmm_param(WIFI_MODE_AP, 0xf3, (struct ieee80211_wmm_param *)&txq[3]);
    }
#endif
}

int32 wificfg_flush(uint8 ifidx)
{
    wificfg_custom_ap_wmm_param(ifidx);

    if (sys_cfgs.wifi_hwmode) {
        ieee80211_conf_set_hwmode(ifidx, sys_cfgs.wifi_hwmode);
    }

    if (ifidx == WIFI_MODE_AP) {
        if (sys_cfgs.channel == 0 && sys_status.channel) {
            ieee80211_conf_set_channel(ifidx, sys_status.channel);
        } else {
            ieee80211_conf_set_channel(ifidx, sys_cfgs.channel);
        }
    }

    ieee80211_conf_set_mac(ifidx, sys_cfgs.mac);
    ieee80211_conf_set_hwmode(ifidx, sys_cfgs.wifi_hwmode);
    ieee80211_conf_set_channel(ifidx, sys_status.channel);
    ieee80211_conf_set_beacon_int(ifidx, sys_cfgs.beacon_int);
    ieee80211_conf_set_dtim_int(ifidx, sys_cfgs.dtim_period);
    ieee80211_conf_set_bss_max_idle(ifidx, sys_cfgs.bss_max_idle);
    ieee80211_conf_set_aphide(ifidx, sys_cfgs.ap_hide);
    ieee80211_conf_set_bssbw(ifidx, sys_cfgs.bss_bw);

    if (ifidx == WIFI_MODE_AP && sys_cfgs.wifi_mode == WIFI_MODE_APSTA && sys_cfgs.cfg_init) {
        //ieee80211_conf_set_ssid(ifidx, sys_cfgs.r_ssid);
        //ieee80211_conf_set_keymgmt(ifidx, sys_cfgs.r_key_mgmt);
        //ieee80211_conf_set_psk(ifidx, sys_cfgs.r_psk);
        //ieee80211_conf_set_passwd(ifidx, sys_cfgs.r_passwd);
    } else {
        ieee80211_conf_set_ssid(ifidx, sys_cfgs.ssid);
        ieee80211_conf_set_keymgmt(ifidx, sys_cfgs.key_mgmt);
        ieee80211_conf_set_psk(ifidx, sys_cfgs.psk);
        ieee80211_conf_set_passwd(ifidx, (char*)sys_cfgs.passwd);
    }

    if (sys_cfgs.wifi_mode == WIFI_MODE_APSTA) {
        //ieee80211_conf_set_relay_mode(WIFI_MODE_AP, sys_cfgs.relay_en, sys_cfgs.relay_level, sys_cfgs.relay_mcast);
        //ieee80211_conf_set_relay_mode(WIFI_MODE_STA, sys_cfgs.relay_en, sys_cfgs.relay_level, sys_cfgs.relay_mcast);
    } else {
        //ieee80211_conf_set_relay_mode(ifidx, 0, 0, 0);
    }

    netdev_set_wifi_mode((struct netdev *)dev_get(HG_WIFI0_DEVID), sys_cfgs.wifi_mode);
    return 0;
}

void netcfg_flush(void)
{
    ip_addr_t ipaddr, netmask, gw;
    ipaddr.addr  = sys_cfgs.ipaddr;
    netmask.addr = sys_cfgs.netmask;
    gw.addr      = sys_cfgs.gw_ip;
    lwip_netif_set_ip2("w0", &ipaddr, &netmask, &gw);

    if (sys_cfgs.dhcpd_en) {
        dhcpd_start("w0", NULL);
    } else {
        dhcpd_stop();
    }

    lwip_netif_set_dhcp2("w0", sys_cfgs.dhcpc_en);
}

void syscfg_flush(int32 reset)
{
    if (reset) {
        ieee80211_iface_stop(WIFI_MODE_STA);
        ieee80211_iface_stop(WIFI_MODE_AP);
    }

    if (sys_cfgs.wifi_mode == WIFI_MODE_APSTA) {
        wificfg_flush(WIFI_MODE_AP);
        wificfg_flush(WIFI_MODE_STA);
        if (reset) {
            ieee80211_iface_start(WIFI_MODE_STA);
            ieee80211_iface_start(WIFI_MODE_AP);
        }
    } else {
        wificfg_flush(sys_cfgs.wifi_mode);
        if (reset) {
            ieee80211_iface_start(sys_cfgs.wifi_mode);
        }
    }

    if (!sys_cfgs.dhcpc_en) {
        sys_status.dhcpc_result.ipaddr  = sys_cfgs.ipaddr;
        sys_status.dhcpc_result.netmask = sys_cfgs.netmask;
        sys_status.dhcpc_result.router  = sys_cfgs.gw_ip;
    }
}

int32 syscfg_save(void)
{
    return syscfg_write("syscfg", &sys_cfgs, sizeof(sys_cfgs));
}

void syscfg_default()
{
    sysctrl_efuse_mac_addr_calc(sys_cfgs.mac);
    if (IS_ZERO_ADDR(sys_cfgs.mac)) {
        os_random_bytes(sys_cfgs.mac, 6);
        sys_cfgs.mac[0] &= 0xfe;
        os_printf(KERN_WARNING"use random mac "MACSTR"\r\n", MAC2STR(sys_cfgs.mac));
    }
    os_snprintf((char *)sys_cfgs.ssid, SSID_MAX_LEN, WIFI_SSID_PREFIX"%02X%02X%02X", sys_cfgs.mac[3], sys_cfgs.mac[4], sys_cfgs.mac[5]);
    os_strcpy(sys_cfgs.passwd, WIFI_PASSWD_DEFAULT);
    wpa_passphrase(sys_cfgs.ssid, (char*)sys_cfgs.passwd, sys_cfgs.psk);
}

void syscfg_dump()
{
    VERSION_SHOW();
    _os_printf("System Settings:\r\n");
    _os_printf("  mac:"MACSTR", dhcpc_en:%d, dhcpd_en:%d\r\n", MAC2STR(sys_cfgs.mac), sys_cfgs.dhcpc_en, sys_cfgs.dhcpd_en);
    _os_printf("  wifi:\r\n");
    _os_printf("     mode:%s, bw%d, channel:%d(%d)\r\n", SYSCFG_WIFI_MODE(sys_cfgs.wifi_mode), sys_cfgs.bss_bw, sys_cfgs.channel, sys_status.channel);
    _os_printf("     ssid:%s, passwd:%s, keymgmt:0x%x\r\n", sys_cfgs.ssid, sys_cfgs.passwd, sys_cfgs.key_mgmt);
    _os_printf("     bss_maxidle:%d, beacon_int:%d, dtim_period:%d\r\n", sys_cfgs.bss_max_idle, sys_cfgs.beacon_int, sys_cfgs.dtim_period);
    _os_printf("  network:\r\n");
    _os_printf("     ipaddr:"IPSTR", netmask:"IPSTR", gw:"IPSTR"\n", IP2STR_N(sys_cfgs.ipaddr), IP2STR_N(sys_cfgs.netmask), IP2STR_N(sys_cfgs.gw_ip));
    _os_printf("     dhcpserver: "IPSTR"-"IPSTR", leaseTime:%d\n", IP2STR_N(sys_cfgs.dhcpd_startip), IP2STR_N(sys_cfgs.dhcpd_endip), sys_cfgs.dhcpd_lease_time);    
}

