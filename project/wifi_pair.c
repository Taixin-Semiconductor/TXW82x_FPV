#include "sys_config.h"
#include "basic_include.h"
#include "lib/rpc/cpurpc.h"
#include "lib/atcmd/libatcmd.h"
#include "lib/common/atcmd.h"
#include "lib/umac/ieee80211.h"

/* WiFi配对 */
#if SYS_WIFI_PAIR
struct wifi_pair_status {
    uint8 pair_sucess;
    uint8 pair_mac[6];
    uint8 aid;
    uint8 ngo_ap;
} wifi_pair;

#define WIFI_PAIR_MAGIC (0x1234)

void sys_wifi_pair_init()
{
    ieee80211_pair_enable(WIFI_MODE_AP, WIFI_PAIR_MAGIC);
    ieee80211_pair_enable(WIFI_MODE_STA, WIFI_PAIR_MAGIC);
}


/* 参数：
   magic == 0 --- 停止配对
   magic == 1 --- 启动配对
   magic >  1 --- 启动配对，并修改magic (初始值为WIFI_PAIR_MAGIC)
*/
void sys_wifi_pair_start(uint8 ifidx, uint16 magic)
{
    etharp_cleanup_netif(netif_find("w0")); //是否需要清空ARP？
    ieee80211_pairing(ifidx, magic);
}

void sys_event_hdl_wifi_pair(uint32 event_id, uint32 data, uint32 priv)
{
    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_PAIR_DONE):
            //配对成功，保存参数
            if (wifi_pair.pair_sucess) {
                ieee80211_conf_get_ssid(sys_cfgs.wifi_mode, sys_cfgs.ssid);
                ieee80211_conf_get_psk(sys_cfgs.wifi_mode, sys_cfgs.psk);
                sys_cfgs.key_mgmt = ieee80211_conf_get_keymgmt(sys_cfgs.wifi_mode);

                if ((int32)data == 1 && sys_cfgs.wifi_mode == WIFI_MODE_STA) {
                    os_printf(KERN_NOTICE"wifi pair done, NGO role AP!\r\n");
                    sys_cfgs.wifi_mode = WIFI_MODE_AP;
                    ieee80211_iface_stop(WIFI_MODE_STA); //stop STA
                    wificfg_flush(WIFI_MODE_AP);
                    ieee80211_iface_start(WIFI_MODE_AP); //switch to AP
                }

                if ((int32)data == -1 && sys_cfgs.wifi_mode == WIFI_MODE_AP) {
                    os_printf(KERN_NOTICE"wifi pair done, NGO role STA!\r\n");
                    sys_cfgs.wifi_mode = WIFI_MODE_STA;
                    ieee80211_iface_stop(WIFI_MODE_AP); //stop AP
                    wificfg_flush(WIFI_MODE_STA);
                    ieee80211_iface_start(WIFI_MODE_STA); //switch to STA.
                }

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
            wifi_pair.ngo_ap = 0;
            wifi_pair.pair_sucess = 0;
            os_memset(wifi_pair.pair_mac, 0, 6);
            os_printf(KERN_NOTICE"Pair Start\r\n");
            break;
        case IEEE80211_EVENT_PAIR_SUCCESS:
            wifi_pair.pair_sucess = 1;
            os_memcpy(wifi_pair.pair_mac, param1, 6); //对方的MAC地址
            os_printf(KERN_NOTICE"pair success with "MACSTR", AID:%d\r\n", MAC2STR(wifi_pair.pair_mac), param2);
            if(!wifi_pair.ngo_ap){ //STA端自动停止配对
                ieee80211_pairing(ifidx, 0); //自动停止配对
            }
            break;
        case IEEE80211_EVENT_PAIR_DONE:
            os_printf(KERN_NOTICE"pair DONE, ngo=%d\r\n", param2);
            sys_event_new(SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_PAIR_DONE), param2);
            break;
        case IEEE80211_EVENT_REQ_PAIR_AID: //只有AP才会产生此事件
            wifi_pair.ngo_ap = 1;
            ret = (wifi_pair.aid++) + 1;
            break;
        case IEEE80211_EVENT_PAIR_NGO:
            //配对角色协商事件，在此事件可以控制角色协商结果：详细信息请阅读"TXSDK_WiFi开发指南，4.1.30"
            // ret = 1;    //协商结果：本机AP
            // ret = -1;   //协商结果：本机做STA
            break;
    }
    
    return ret;
}

#endif

