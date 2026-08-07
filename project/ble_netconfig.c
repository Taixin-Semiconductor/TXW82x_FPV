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

/* BLE 配网 */
#if SYS_APP_BLENC
#define BLE_NETCONFIG_OK "network_connect_succ"

void sys_event_ble_netconfig(uint32 event_id, uint32 data, uint32 priv)
{
    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_CONNECTTED):
#if BLE_PROV_MODE == 2
            os_printf(KERN_NOTICE"WiFi Connected, BLE Notify: %s\r\n", BLE_NETCONFIG_OK);
            uble_gatt_notify(10, (uint8 *)BLE_NETCONFIG_OK, os_strlen(BLE_NETCONFIG_OK));
            os_sleep(1);
            os_printf(KERN_NOTICE"BLE Close!\r\n");
            ble_set_mode(0, 38);     //close BLE
            ble_set_coexist_en(0, 0);
#endif
            break;

        case SYS_EVENT(SYS_EVENT_BLE, SYSEVT_BLE_NETWORK_CONFIGURED):
            wpa_passphrase(sys_cfgs.ssid, (char *)sys_cfgs.passwd, sys_cfgs.psk);
            sys_cfgs.wifi_mode = WIFI_MODE_STA;
            syscfg_save();

            ieee80211_iface_stop(WIFI_MODE_AP);
            ieee80211_iface_stop(WIFI_MODE_STA);
            wificfg_flush(WIFI_MODE_STA);
            netdev_set_wifi_mode((struct netdev *)dev_get(HG_WIFI0_DEVID), WIFI_MODE_STA);
            ieee80211_iface_start(WIFI_MODE_STA);
#if BLE_PROV_MODE == 1
            ble_set_mode(0, 38);
#endif
            break;
    }
}

__init void sys_ble_netconfig_init()
{
    ble_demo_init();
#if BLE_PROV_MODE == 1
    ble_set_mode(1, 38);
#elif BLE_PROV_MODE == 2
    ble_set_coexist_en(1, 0);
    ble_set_mode(3, 38);
#endif
}
#endif

