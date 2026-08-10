#include "sys_config.h"
#include "basic_include.h"
#include "lib/rpc/cpurpc.h"
#include "lib/atcmd/libatcmd.h"
#include "lib/common/atcmd.h"
#include "lib/umac/ieee80211.h"
#include "lwip/etharp.h"
#include "netif/ethernetif.h"
#include "syscfg.h"

/* WiFi配对 */
#if SYS_WIFI_PAIR
struct wifi_pair_status {
    uint8 pair_success;
    uint8 pair_mac[6];
    uint8 aid;
} wifi_pair;

#define WIFI_PAIR_MAGIC (0x1234)

extern void netcfg_flush(void);

void sys_wifi_pair_init()
{
    ieee80211_pair_enable(WIFI_MODE_AP, WIFI_PAIR_MAGIC);
    ieee80211_pair_enable(WIFI_MODE_STA, WIFI_PAIR_MAGIC);
    ieee80211_pair_enable(WIFI_MODE_RMESH, WIFI_PAIR_MAGIC);
}

void sys_rmesh_prepare_pair(uint8 ifidx)
{
#if WIFI_RMESH_SUPPORT
    if (ifidx == WIFI_MODE_RMESH) {
        sys_cfgs.rmesh_devid = 0;
        os_random_bytes(sys_cfgs.psk, 32);
        os_random_bytes(sys_cfgs.rmesh_meshid, 6);
        os_sprintf((char *)sys_cfgs.ssid, MACSTR, MAC2STR(sys_cfgs.rmesh_meshid));
        ieee80211_conf_set_ssid(ifidx, sys_cfgs.ssid);
        ieee80211_conf_set_keymgmt(ifidx, sys_cfgs.key_mgmt);
        ieee80211_conf_set_psk(ifidx, sys_cfgs.psk);
        ieee80211_conf_set_mutl_pair(ifidx, 1);
    }
#endif
}

/* 参数：
   magic == 0 --- 停止配对
   magic == 1 --- 启动配对
   magic >  1 --- 启动配对，并修改magic (初始值为WIFI_PAIR_MAGIC)
*/
void sys_wifi_pair_start(uint8 ifidx, uint16 magic)
{
    if(magic){
        wifi_pair.aid = 1;
        etharp_cleanup_netif(netif_find("w0")); //是否需要清空ARP？
        sys_rmesh_prepare_pair(ifidx);
    }

    ieee80211_pairing(ifidx, magic);
}

void sys_event_hdl_wifi_pair(uint32 event_id, uint32 data, uint32 priv)
{
    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_PAIR_DONE):
            //配对成功，保存参数
            if (wifi_pair.pair_success) {
                ieee80211_conf_get_ssid(sys_cfgs.wifi_mode, sys_cfgs.ssid);
                ieee80211_conf_get_psk(sys_cfgs.wifi_mode, sys_cfgs.psk);
                sys_cfgs.key_mgmt = ieee80211_conf_get_keymgmt(sys_cfgs.wifi_mode);

                os_printf(KERN_NOTICE"wifi pair done! ssid:%s, ngo:%d\r\n", sys_cfgs.ssid, data);
                if ((int32)data == 1 && sys_cfgs.wifi_mode == WIFI_MODE_STA) {
                    os_printf(KERN_NOTICE"  ->NGO role AP!\r\n");
                    sys_cfgs.wifi_mode = WIFI_MODE_AP;
                    ieee80211_iface_stop(WIFI_MODE_STA); //stop STA
                    wificfg_flush(WIFI_MODE_AP);
                    ieee80211_iface_start(WIFI_MODE_AP); //switch to AP
                    sys_cfgs.dhcpd_en = 1;
                    sys_cfgs.dhcpc_en = 0;
                    netcfg_flush();
                }

                if ((int32)data == -1 && sys_cfgs.wifi_mode == WIFI_MODE_AP) {
                    os_printf(KERN_NOTICE"  ->NGO role STA!\r\n");
                    sys_cfgs.wifi_mode = WIFI_MODE_STA;
                    ieee80211_iface_stop(WIFI_MODE_AP); //stop AP
                    wificfg_flush(WIFI_MODE_STA);
                    ieee80211_iface_start(WIFI_MODE_STA); //switch to STA.
                    sys_cfgs.dhcpd_en = 0;
                    sys_cfgs.dhcpc_en = 1;
                    netcfg_flush();
                }

#if WIFI_RMESH_SUPPORT
                if (sys_cfgs.wifi_mode == WIFI_MODE_RMESH) {
                    if((int32)data == -1){
                        sys_cfgs.rmesh_devid = wifi_pair.aid;
                        str2mac((char *)sys_cfgs.ssid, sys_cfgs.rmesh_meshid);
                        os_printf(KERN_NOTICE"  ->meshid:"MACSTR", aid:%d\r\n", MAC2STR(sys_cfgs.rmesh_meshid), sys_cfgs.rmesh_devid);
                    }

                    sys_cfgs.ipaddr = 0xC0A87B01 + sys_cfgs.rmesh_devid;
                    sys_cfgs.ipaddr = os_htonl(sys_cfgs.ipaddr);    
                    ip_addr_t ipaddr, netmask, gw;
                    ipaddr.addr  = sys_cfgs.ipaddr;
                    netmask.addr = sys_cfgs.netmask;
                    gw.addr      = sys_cfgs.gw_ip;
                    lwip_netif_set_ip2("w0", &ipaddr, &netmask, &gw);

                    ieee80211_iface_stop(WIFI_MODE_RMESH);
                    wificfg_flush(WIFI_MODE_RMESH);
                    ieee80211_iface_start(WIFI_MODE_RMESH);
                }
#endif

                if (sys_cfgs.wifi_mode == WIFI_MODE_STA || data) {
                    syscfg_save();
                }
            }
            break;
    }
}

int32 sys_wifi_event_hdl_wifi_pair(uint8 ifidx, uint16 evt, uint32 param1, uint32 param2)
{
    int32 ret = 0;

    switch (evt) {
        case IEEE80211_EVENT_PAIR_START:
            wifi_pair.pair_success = 0;
            os_memset(wifi_pair.pair_mac, 0, 6);
            os_printf(KERN_NOTICE"pair start! %d\r\n", sys_status.pair_role);
            break;
        case IEEE80211_EVENT_PAIR_SUCCESS:
            wifi_pair.pair_success = 1;
            os_memcpy(wifi_pair.pair_mac, param1, 6); //对方的MAC地址
            os_printf(KERN_NOTICE"pair success with "MACSTR", aid:%d, role:%d\r\n", MAC2STR(wifi_pair.pair_mac), param2, sys_status.pair_role);
            if(sys_status.pair_role != 1){ //STA端自动停止配对
                wifi_pair.aid = param2;
                ieee80211_pairing(ifidx, 0); //自动停止配对
            }
            break;
        case IEEE80211_EVENT_PAIR_DONE:
            sys_status.pair_role = 0;
            os_printf(KERN_NOTICE"pair DONE, ngo=%d\r\n", param2);
            sys_event_new(SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_PAIR_DONE), param2);
            break;
        case IEEE80211_EVENT_REQ_PAIR_AID: //只有AP才会产生此事件
            sys_status.pair_role = 1;
            ret = wifi_pair.aid++;
            os_printf(KERN_NOTICE"alloc AID %d for "MACSTR"\r\n", ret, MAC2STR((uint8 *)param1));
            break;
        case IEEE80211_EVENT_PAIR_NGO:
            //配对角色协商事件，在此事件可以控制角色协商结果：详细信息请阅读"TXSDK_WiFi开发指南，4.1.30"
            if(sys_cfgs.wifi_mode == WIFI_MODE_RMESH && wifi_pair.aid >= WIFI_RMESH_DEVMAX){
                ret = 2; //拒绝配对，AID已满。
                os_printf(KERN_WARNING"RMesh: no free AID. max:%d\r\n", WIFI_RMESH_DEVMAX);
            }else{
                ret = sys_status.pair_role;
            }
            // ret = 1;    //协商结果：本机AP
            // ret = -1;   //协商结果：本机做STA
            // ret = 0;    //应用程序忽略协商，由协议栈内部规则协商
            // ret = 2;    //禁止协商配对
            break;
    }

    return ret;
}

#endif

