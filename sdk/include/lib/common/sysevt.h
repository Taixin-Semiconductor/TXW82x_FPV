#ifndef _SDK_SYSEVT_H_
#define _SDK_SYSEVT_H_
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYSEVT_CONTINUE = 0,
    SYSEVT_CONSUMED = 1,
} sysevt_hdl_res;

typedef sysevt_hdl_res(*sysevt_hdl)(uint32 event_id, uint32 data, uint32 priv);

int32 sys_event_init(uint16 evt_max_cnt);
int32 sys_event_new(uint32 event_id, uint32 data);
int32 sys_event_take(uint32 event_id, sysevt_hdl hdl, uint32 priv);
void sys_event_untake(uint32 event_id, sysevt_hdl hdl);

#define SYS_EVENT(main, sub) ((main)<<16|(sub&0xffff))

enum SYSEVT_MAINID { /* uint16 */
    SYS_EVENT_NETWORK = 1,
    SYS_EVENT_WIFI,
    SYS_EVENT_LMAC,
    SYS_EVENT_SYSTEM,
    SYS_EVENT_BLE,
    SYS_EVENT_LTE,
    SYS_EVENT_MEDIA,

    ////////////////////////////////////
    SYSEVT_MAINID_ID,
};

//////////////////////////////////////////
enum SYSEVT_SYSTEM_SUBEVT { /* uint16 */
    SYSEVT_SYSTEM_RESUME = 1,
    SYSEVT_SYSTEM_SD_MOUNT,
    SYSEVT_SYSTEM_SD_UNMOUNT,
    SYSEVT_TASK_DELETE,
};
#define SYSEVT_NEW_SYSTEM_EVT(subevt, data)    sys_event_new(SYS_EVENT(SYS_EVENT_SYSTEM, subevt), data)

//////////////////////////////////////////
enum SYSEVT_LMAC_SUBEVT { /* uint16 */
    SYSEVT_LMAC_ACS_DONE = 1,
    SYSEVT_LMAC_TX_STATUS = 2,
    SYSEVT_LMAC_APP_HBDATA_DETECT = 3,  //检测到应用心跳包
};
#define SYSEVT_NEW_LMAC_EVT(subevt, data)    sys_event_new(SYS_EVENT(SYS_EVENT_LMAC, subevt), data)

//////////////////////////////////////////
enum SYSEVT_WIFI_SUBEVT { /* uint16 */
    SYSEVT_WIFI_CONNECT_START = 1, //STA start connect, event data: 0
    SYSEVT_WIFI_CONNECTTED,        //STA connect success, event data: my AID.
    SYSEVT_WIFI_CONNECT_FAIL,      //STA connect fail, event data: status code.
    SYSEVT_WIFI_DISCONNECT,        //unused.
    SYSEVT_WIFI_SCAN_START,        //Start Scanning, event data: 0
    SYSEVT_WIFI_SCAN_DONE,         //Scan done, event data: 0
    SYSEVT_WIFI_STA_DISCONNECT,    //STA disconnect, event data: STA's AID.
    SYSEVT_WIFI_STA_CONNECTTED,    //STA connected, event data: STA's AID.
    SYSEVT_WIFI_STA_PS_START,      //STA enter ps mode, event data: STA's AID.
    SYSEVT_WIFI_STA_PS_END,        //STA exit ps mode, event data: STA's AID.
    SYSEVT_WIFI_PAIR_DONE,         //pair done, event data: 1:success, 0:fail.
    SYSEVT_WIFI_TX_SUCCESS,        //wifi tx success, event data: data tag setted by ieee80211_conf_set_datatag.
    SYSEVT_WIFI_TX_FAIL,           //wifi tx fail, event data: data tag setted by ieee80211_conf_set_datatag.
    SYSEVT_WIFI_UNPAIR,            //unpaired, event data:0
    SYSEVT_WIFI_WRONG_KEY,         //wifi password is wrong. event data:0
    SYSEVT_WIFI_P2P_DONE,          //p2p wsc done. update wifi syscfg
};
#define SYSEVT_NEW_WIFI_EVT(subevt, data)    sys_event_new(SYS_EVENT(SYS_EVENT_WIFI, subevt), data)

//////////////////////////////////////////
enum SYSEVT_NETWORK_SUBEVT { /* uint16 */
    SYSEVT_GMAC_LINK_UP = 1,
    SYSEVT_GMAC_LINK_DOWN,
    SYSEVT_LWIP_DHCPC_START,
    SYSEVT_LWIP_DHCPC_DONE,
    SYSEVT_WIFI_DHCPC_START,
    SYSEVT_WIFI_DHCPC_DONE,
    SYSEVT_DHCPD_NEW_IP,
    SYSEVT_DHCPD_IPPOOL_FULL,
    SYSEVT_NTP_UPDATE,
};
#define SYSEVT_NEW_NETWORK_EVT(subevt, data) sys_event_new(SYS_EVENT(SYS_EVENT_NETWORK, subevt), data)

//////////////////////////////////////////
enum SYSEVT_BLE_SUBEVT { /* uint16 */
    SYSEVT_BLE_CONNECTED = 1,
    SYSEVT_BLE_DISCONNECT,
    SYSEVT_BLE_NETWORK_CONFIGURED,
    SYSEVT_BLE_EXCHANGE_MTU,
};
#define SYSEVT_NEW_BLE_EVT(subevt, data) sys_event_new(SYS_EVENT(SYS_EVENT_BLE, subevt), data)

//////////////////////////////////////////
enum SYSEVT_LTE_SUBEVT { /* uint16 */
    SYSEVT_LTE_CONNECTED = 1,
    SYSEVT_LTE_DISCONNECT,
    SYSEVT_LTE_NETWORK_CONFIGURED,
    SYSEVT_LTE_OVERLAP_WIFI,
};
#define SYSEVT_NEW_LTE_EVT(subevt, data) sys_event_new(SYS_EVENT(SYS_EVENT_LTE, subevt), data)


//////////////////////////////////////////
enum SYSEVT_MEDIA_SUBEVT { /* uint16 */
    SYSEVT_MEDIA_PLAY_START = 1, //启动播放
    SYSEVT_MEDIA_PLAY_STOP,      //停止播放
    SYSEVT_MEDIA_PLAYING,        //进入播放状态
    SYSEVT_MEDIA_PLAY_PAUSE,     //暂停播放
    SYSEVT_MEDIA_PLAY_SPEED,     //播放速度改变
    SYSEVT_MEDIA_PLAY_SEEK_ERR,  //播放跳转失败
    SYSEVT_MEDIA_PLAY_CLOSE,     //播放器关闭

    SYSEVT_MEDIA_OPEN_SUCCESS,   //数据流打开成功
    SYSEVT_MEDIA_OPEN_FAIL,      //数据流打开失败
    SYSEVT_MEDIA_READ_EOF,       //数据流读取完成
    SYSEVT_MEDIA_READ_ERROR,     //数据流读取出错
    SYSEVT_MEDIA_BUFFERING_DATA, //数据流缓冲等待
    
    SYSEVT_MEDIA_UNKNOWN_CONTAINER, //未支持的container类型
    SYSEVT_MEDIA_UNKNOWN_DECODER,   //未支持的decoder类型
    
    SYSEVT_MEDIA_DECODER_MATCH,   //decoder成功匹配
    SYSEVT_MEDIA_DECODER_ERROR,   //decoder解码异常

    SYSEVT_MEDIA_CONTAINER_MATCH,       //container成功匹配
    SYSEVT_MEDIA_CONTAINER_OPEN_ERROR,  //container打开失败
    SYSEVT_MEDIA_CONTAINER_DEMUX_ERROR, //container解码异常
};
#define SYSEVT_NEW_MEDIA_EVT(subevt, data) sys_event_new(SYS_EVENT(SYS_EVENT_MEDIA, subevt), data)


#ifdef __cplusplus
}
#endif
#endif
