#include "sys_config.h"
#include "basic_include.h"
#include "hal/adc.h"
#include "lib/rpc/cpurpc.h"
#include "lib/atcmd/libatcmd.h"
#include "lib/common/atcmd.h"
#include "lib/umac/ieee80211.h"
#include "lib/lmac/lmac.h"
#include "lib/bluetooth/uble/ble_demo.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/tcpip.h"
#include "netif/ethernetif.h"

#include "lib/net/dhcpd/dhcpd.h"
#include "lib/net/utils.h"
#include "lib/net/skmonitor/skmonitor.h"
#include "lib/net/urlfile/urlfile.h"
#include "lwip/nd6.h"
#include "syscfg.h"
#include "demo/user_app.h"
#if URLFILE_ENABLE
static const struct ufprotocol ufprotos[] ={
    {"http", &uf_curl_http},
};
#endif

__init static void sys_network_add_netif(uint8 devid, char *name, uint8 dhcpc, uint8 setip)
{
    ip_addr_t ipaddr, netmask, gw;
    struct netdev *ndev = (struct netdev *)dev_get(devid);
    if (ndev) {
        ip_addr_set_ip4_u32_val(ipaddr, sys_cfgs.ipaddr);
        ip_addr_set_ip4_u32_val(netmask,sys_cfgs.netmask);
        ip_addr_set_ip4_u32_val(gw, sys_cfgs.gw_ip); 
        
        os_printf(KERN_INFO"netif %s setttings: "IPSTR"/"IPSTR"/"IPSTR"!\r\n", name, 
            IP2STR_N(ip_addr_get_ip4_u32(&ipaddr)), 
            IP2STR_N(ip_addr_get_ip4_u32(&netmask)), 
            IP2STR_N(ip_addr_get_ip4_u32(&gw)));
        lwip_netif_add(ndev, name, (setip ? &ipaddr : NULL), (setip ? &netmask : NULL), (setip ? &gw : NULL));
        if (dhcpc) {
            sys_status.dhcpc_done = 0;
            lwip_netif_set_dhcp(ndev, 1);
            os_printf(KERN_INFO"netif %s start dhcp client!\r\n", name);
        }
#if LWIP_IPV6
        sys_network_ipv6_init(ndev, name);
#endif
    } else {
        os_printf(KERN_ERR"No Netdev: %d\r\n", devid);
    }
}

__init void sys_network_init(void)
{
    tcpip_init(NULL, NULL);
    sock_monitor_init();

    sys_network_add_netif(HG_WIFI0_DEVID, "w0", sys_cfgs.dhcpc_en, 1);
    lwip_netif_set_default2("w0");
#if GMAC_EN
    uint8_t mac[6];
    sysctrl_efuse_mac_addr_calc(mac);
    mac[5] |= 3;
    netdev_set_macaddr((struct netdev *)dev_get(HG_GMAC_DEVID), mac);
    sys_network_add_netif(HG_GMAC_DEVID, "e0", sys_cfgs.dhcpc_en, 1);
    lwip_netif_set_default2("e0");
#endif

#if URLFILE_ENABLE
    urlfile_init(ufprotos, ARRAY_SIZE(ufprotos));    
#endif

    sys_status.dhcpc_result.ipaddr  = sys_cfgs.ipaddr;
    sys_status.dhcpc_result.netmask = sys_cfgs.netmask;
    sys_status.dhcpc_result.router  = sys_cfgs.gw_ip;
}

//启动DHCP服务器
void sys_dhcpd_start()
{
    struct dhcpd_param param;

    if (sys_cfgs.dhcpd_en && sys_cfgs.wifi_mode == WIFI_MODE_AP) {
        os_memset(&param, 0, sizeof(param));
        param.start_ip   = sys_cfgs.dhcpd_startip;
        param.end_ip     = sys_cfgs.dhcpd_endip;
        param.netmask    = sys_cfgs.netmask;
        param.lease_time = sys_cfgs.dhcpd_lease_time;
        param.dns1       = sys_cfgs.dhcpd_dns1;
        param.dns2       = sys_cfgs.dhcpd_dns2;
        param.router     = sys_cfgs.dhcpd_router;
        if (dhcpd_start("w0", &param)) {
            os_printf("dhcpd start error\r\n");
        }
    }
}

