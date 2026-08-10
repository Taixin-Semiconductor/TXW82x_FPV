#include "sys_config.h"
#include "basic_include.h"
#include "lib/net/skmonitor/skmonitor.h"
#include "lib/net/dhcpd/dhcpd.h"
#include "lib/net/uhttpd/uhttpd.h"
#include "lib/umac/ieee80211.h"
#include "lib/net/utils.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/tcpip.h"
#include "lwip/dns.h"
#include "lwip/dhcp.h"
#include "netif/ethernetif.h"
#include "lwip/apps/netbiosns.h"
#include "lwip/dns.h"
#include "syscfg.h"
#include "lib/bluetooth/uble/ble_demo.h"
#include "sysevt_usb/sysevt_usb.h"

extern void sys_event_hdl_wifi_pair(uint32 event_id, uint32 data, uint32 priv);
extern int32 sys_wifi_event_hdl_wifi_pair(uint8 ifidx, uint16 evt, uint32 param1, uint32 param2);
extern int32 sys_wifi_event_hdl_pairled(uint8 ifidx, uint16 evt, uint32 param1, uint32 param2);
extern void sys_mount_device(uint16 dev_id, uint16 dev_type, uint8 umount);

//更新 sys_status 信息
static void sys_event_hdl_dhcp(uint32 event_id, uint32 data, uint32 priv)
{
    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_CONNECTTED):
            if (sys_cfgs.dhcpc_en) {
                sys_status.dhcpc_done = 0;
                lwip_netif_set_dhcp2("w0", 1);
                os_printf(KERN_NOTICE"wifi connected, start dhcp client ...\r\n");
            }
            if(sys_cfgs.wifi_mode == WIFI_MODE_STA) {
                ieee80211_conf_get_ssid(sys_cfgs.wifi_mode, sys_cfgs.ssid);
                ieee80211_conf_get_psk(sys_cfgs.wifi_mode, sys_cfgs.psk);
                sys_cfgs.key_mgmt = ieee80211_conf_get_keymgmt(sys_cfgs.wifi_mode);
                syscfg_save();
            }
            break;

        case SYS_EVENT(SYS_EVENT_NETWORK, SYSEVT_LWIP_DHCPC_DONE): {
            struct netif *nif = netif_find("w0");
            sys_status.dhcpc_done = 1;
            sys_status.dhcpc_result.ipaddr  = ip_addr_get_ip4_u32(&nif->ip_addr);
            sys_status.dhcpc_result.netmask = ip_addr_get_ip4_u32(&nif->netmask);
            sys_status.dhcpc_result.svrip   = 0;
            sys_status.dhcpc_result.router  = ip_addr_get_ip4_u32(&nif->gw);
            sys_status.dhcpc_result.dns1    = ip_addr_get_ip4_u32(dns_getserver(0));
            sys_status.dhcpc_result.dns2    = ip_addr_get_ip4_u32(dns_getserver(1));
            os_printf(KERN_NOTICE"dhcp done, ip:"IPSTR", mask:"IPSTR", gw:"IPSTR"\r\n",
                      IP2STR_N(ip_addr_get_ip4_u32(&nif->ip_addr)), 
                      IP2STR_N(ip_addr_get_ip4_u32(&nif->netmask)),
                      IP2STR_N(ip_addr_get_ip4_u32(&nif->gw)));
        }
        break;
    }
}

void sys_event_hdl_lte(uint32 event_id, uint32 data, uint32 priv)
{
#if defined(RT_USBH_WIRELESS_RNDIS) && !defined(STATIC_RNDIS_NETDEV)
    // 只在启用了RNDIS功能且是动态申请网口时才需要事件进行切换
    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_LTE, SYSEVT_LTE_CONNECTED):
            os_printf("lte connected\r\n");
            if (sys_status.wifi_connected == 0) {
                lwip_netif_set_default2("l0");
                lwip_netif_set_dhcp2("l0", 1);
                os_printf("start dhcp client on l0 ...\r\n");
            }
            break;
        case SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_CONNECTTED):
            lwip_netif_updown2("l0", 0);
            lwip_netif_set_default2("w0");
            os_printf("wifi connected, down lte netif ...\r\n");
            break;
        case SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_DISCONNECT):
            lwip_netif_updown2("l0", 1);
            lwip_netif_set_default2("l0");
            os_printf("wifi disconnected, up lte netif ...\r\n");
            break; 
    }
#endif
}

//heartbeat det
static void sys_event_hdl_dsleep(uint32 event_id, uint32 data, uint32 priv)
{
#ifdef CONFIG_SLEEP
    struct netif *nif = netif_find("w0"); 
    uint32 t0, t1, t2;

    switch (event_id) {
            case SYS_EVENT(SYS_EVENT_LMAC, SYSEVT_LMAC_APP_HBDATA_DETECT):
                os_printf("%s: event_id= %d\r\n", __func__, event_id);
                os_printf("SYSEVT_LMAC_APP_HBDATA_DETECT: data= 0x%x\r\n", data);
            break;
            case SYS_EVENT(SYS_EVENT_NETWORK, SYSEVT_NTP_UPDATE):
                os_printf("%s: event_id= %d\r\n", __func__, event_id);
                os_printf("SYSEVT_NTP_UPDATE: data= 0x%x\r\n", data);
                dsleep_set_ntp_update();
            break;
            case SYS_EVENT(SYS_EVENT_NETWORK, SYSEVT_LWIP_DHCPC_DONE):
                os_printf("%s: event_id= %d\r\n", __func__, event_id);
                os_printf("SYSEVT_LWIP_DHCPC_DONE: data= 0x%x\r\n", data);
                dhcp_get_leasetime(nif, &t0, &t1, &t2);
                dsleep_set_router(ip_addr_get_ip4_u32(&nif->gw), t1);
                dsleep_set_ip_addr(ip_addr_get_ip4_u32(&nif->ip_addr));
                os_printf("ip= 0x%x router= 0x%x t0= %d t1= %d netif= 0x%x\r\n", 
                ip_addr_get_ip4_u32(&nif->ip_addr), ip_addr_get_ip4_u32(&nif->gw), t0, t1, nif);
            break;
            default:
            break;
    }
#endif
}

static void sys_event_hdl_hotplug(uint32 event_id, uint32 data, uint32 priv)
{
    uint16 dev_id   = (data>>16) & 0xffff;
    uint16 dev_type = data & 0xffff;
    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_SYSTEM, SYSEVT_SYSTEM_PLUGIN):
            sys_mount_device(dev_id, dev_type, 0);
            break;
        case SYS_EVENT(SYS_EVENT_SYSTEM, SYSEVT_SYSTEM_PLUGOUT):
            sys_mount_device(dev_id, dev_type, 1);
            break;
        default:
            break;
    }
}

sysevt_hdl_res sys_event_hdl(uint32 event_id, uint32 data, uint32 priv)
{
#if SYS_WIFI_PAIR
    sys_event_hdl_wifi_pair(event_id, data, priv);
#endif

    /*
     * 继续添加其他模块的处理函数 ...
     * 一些小功能的事件处理没必要使用 sys_event_take，在此添加API调用即可。
     * 复杂功能的事件处理，可以使用 sys_event_take API 注册事件处理函数。
     * 使用 sys_event_take 注册，每次会消耗16byte heap memory
     */

    sys_event_hdl_dhcp(event_id, data, priv);
    sys_event_hdl_lte(event_id, data, priv);

    system_event_usbh_video_hdl(event_id, data, priv);
    sys_event_hdl_dsleep(event_id, data, priv);         //put last

    sys_event_hdl_hotplug(event_id, data, priv);

    return SYSEVT_CONTINUE;
}

//更新 sys_status 信息
static int32 sys_wifi_event_hdl_default(uint8 ifidx, uint16 evt, uint32 param1, uint32 param2)
{
    switch (evt) {
        case IEEE80211_EVENT_CONNECTED:
            sys_status.wifi_connected = 1;
            sys_status.wifi_status_code = 0;
            sys_status.channel = ieee80211_conf_get_channel(WIFI_MODE_STA);
            os_memcpy(sys_status.bssid, param1, 6);
            break;
        case IEEE80211_EVENT_RSSI:
            sys_status.rssi = (int8)param1;
            break;
        case IEEE80211_EVENT_EVM:
            sys_status.evm = (int8)param1;
            break;
        case IEEE80211_EVENT_CONNECT_FAIL:
            sys_status.wifi_connected = 0;
            sys_status.wifi_status_code = param2;
            break;
        case IEEE80211_EVENT_DISCONNECTED:
            sys_status.wifi_connected = 0;
            sys_status.wifi_reason_code = param2;
            break;
        case IEEE80211_EVENT_INTERFACE_ENABLE:
            sys_status.wifi_mode = param1;
            break;
        case IEEE80211_EVENT_CHANNEL_CHANGE:
            sys_status.channel = param2;
            sys_cfgs.channel = param2;
            break;
        case IEEE80211_EVENT_PAIR_SUCCESS:
            if(WIFI_MODE_STA == ifidx)
            {
                ieee80211_pairing(sys_cfgs.wifi_mode, 0);
            }  
            break;
        default:
            break;
    }

    return 0;
}

//WiFi协议栈的事件回调函数
// 该callback是在WiFi协议栈的Task中执行，因此该callback中不能执行耗时的代码
// 耗时的代码逻辑，可以使用sys_event_hdl来执行
int32 sys_wifi_event_cb(uint8 ifidx, uint16 evt, uint32 param1, uint32 param2)
{
    int32 ret = 0;

#if SYS_WIFI_PAIR
    ret |= sys_wifi_event_hdl_wifi_pair(ifidx, evt, param1, param2);
#endif

#if SYS_WIFI_PAIRLED
    ret |= sys_wifi_event_hdl_pairled(ifidx, evt, param1, param2);
#endif

    /*
     * 继续添加其他模块的处理函数 ...
     */
#if SYS_APP_BLENC
    ret |= sys_event_ble_netconfig(ifidx, evt, param1, param2);
#endif

    ret |= sys_wifi_event_hdl_default(ifidx, evt, param1, param2);
    return ret;
}

