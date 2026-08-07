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
#include "netif/ethernetif.h"
#include "lwip/apps/netbiosns.h"
#include "lwip/dns.h"
#include "syscfg.h"
#include "lib/bluetooth/uble/ble_demo.h"

extern int32 sys_wifi_event_hdl_wifi_pair(uint8 ifidx, uint16 evt, uint32 param1, uint32 param2);
extern int32 sys_wifi_event_hdl_pairled(uint8 ifidx, uint16 evt, uint32 param1, uint32 param2);
extern int32 sys_wifi_event_hdl_pairled(uint8 ifidx, uint16 evt, uint32 param1, uint32 param2);

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
            break;

        case SYS_EVENT(SYS_EVENT_NETWORK, SYSEVT_LWIP_DHCPC_DONE): {
            struct netif *nif = netif_find("w0");
            sys_status.dhcpc_done = 1;
            sys_status.dhcpc_result.ipaddr  = nif->ip_addr.addr;
            sys_status.dhcpc_result.netmask = nif->netmask.addr;
            sys_status.dhcpc_result.svrip   = 0;
            sys_status.dhcpc_result.router  = nif->gw.addr;
            sys_status.dhcpc_result.dns1    = (dns_getserver(0))->addr;
            sys_status.dhcpc_result.dns2    = (dns_getserver(1))->addr;
            os_printf(KERN_NOTICE"dhcp done, ip:"IPSTR", mask:"IPSTR", gw:"IPSTR"\r\n",
                      IP2STR_N(nif->ip_addr.addr), IP2STR_N(nif->netmask.addr), IP2STR_N(nif->gw.addr));
        }
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

    ret |= sys_wifi_event_hdl_default(ifidx, evt, param1, param2);
    return ret;
}

