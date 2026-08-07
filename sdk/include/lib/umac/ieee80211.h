#ifndef _HGIC_IEEE80211_H_
#define _HGIC_IEEE80211_H_

#include "lib/lmac/hgic.h"
#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_SAE) || defined(CONFIG_OWE)
#define CONFIG_ECC      
#define sae_dbg(fmt, ...) //os_printf("%s:%d:: __SAE:"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define sae_dbg(fmt, ...)
#endif

/* IEEE 802.11r */
#define MOBILITY_DOMAIN_ID_LEN 2
#define FT_R0KH_ID_MAX_LEN 48
#define FT_R1KH_ID_LEN 6
#define WPA_PMK_NAME_LEN 16

#define WPA_KEY_MGMT_PSK        BIT(1)
#define WPA_KEY_MGMT_NONE       BIT(2)
#define WPA_KEY_MGMT_PSK_SHA256 BIT(8)
#define WPA_KEY_MGMT_SAE        BIT(10)
#define WPA_KEY_MGMT_OWE        BIT(22)

#define WPA_CIPHER_NONE         BIT(0)
#define WPA_CIPHER_TKIP         BIT(3)
#define WPA_CIPHER_CCMP         BIT(4)
#define WPA_CIPHER_CCMP_256     BIT(9)

#define WPA_PROTO_WPA           BIT(0)
#define WPA_PROTO_RSN           BIT(1)

enum IEEE80211_BAND {
    IEEE80211_BAND_2GHZ,
    IEEE80211_BAND_5GHZ,
    IEEE80211_BAND_60GHZ,
    IEEE80211_BAND_S1GHZ,

    NUM_IEEE80211_BANDS,
};

enum ieee80211_hwmode {
    IEEE80211_HWMODE_NONE,
    IEEE80211_HWMODE_11B,  // b only
    IEEE80211_HWMODE_11G,  // bg mix
    IEEE80211_HWMODE_11N,  // bgn mix
    IEEE80211_HWMODE_11AH, // ah only
};

enum country_code {
    COUNTRY_US,
    COUNTRY_EU,
    COUNTRY_JP,
    COUNTRY_CN,
    COUNTRY_KR,
    COUNTRY_SG,
    COUNTRY_AU,
    COUNTRY_NZ,

    NUM_COUNTRIES,
};

enum ieee80211_event {
    IEEE80211_EVENT_PAIR_START = 1,     // 1 - start pairing, param1:0, param2:0
    IEEE80211_EVENT_PAIR_SUCCESS,       // 2 - pairing success, param1: pairing peer's MAC, param2:6
    IEEE80211_EVENT_PAIR_FAIL,          // 3 - pairing fail. param1:1, param2:2
    IEEE80211_EVENT_PAIR_DONE,          // 4 - paring complete. param1: pairing peer's MAC, param2:6
    IEEE80211_EVENT_SCAN_START,         // 5 - start scanning. param1:0, param2:0
    IEEE80211_EVENT_SCAN_DONE,          // 6 - stop scanning. param1:0, param2:0
    IEEE80211_EVENT_CONNECT_START,      // 7 - start connect. param1:AP's MAC, param2:0
    IEEE80211_EVENT_CONNECTED,          // 8 - connect success. param1: AP/STA MAC, param2:0
    IEEE80211_EVENT_CONNECT_FAIL,       // 9 - connect fail. param1: status code, param2:0
    IEEE80211_EVENT_DISCONNECTED,       // 10 - disconnect. param1: AP/STA MAC, param2: reason code.
    IEEE80211_EVENT_RX_FRAME,           // 11 - recieve wifi frames. param1: the pointer of frame, type: uint8 *, param2: rx status, type: struct ieee80211_rx_status * 
    IEEE80211_EVENT_RSSI,               // 12 - RSSI changed. param1: new rssi, param2: sta's MAC.
    IEEE80211_EVENT_STATE_CHANGE,       // 13 - <ignore>.
    IEEE80211_EVENT_NEW_BSS,            // 14 - find a new BSS. param1: BSS's bssid, param2: BSS's ssid.
    IEEE80211_EVENT_UPDATE_BSS,         // 15 - <ignore>.
    IEEE80211_EVENT_PS_START,           // 16 - <ignore>.
    IEEE80211_EVENT_PS_END,             // 17 - <ignore>.
    IEEE80211_EVENT_STA_PS_START,       // 18 - STA enter ps mode. param1: STA's MAC, param2:0
    IEEE80211_EVENT_STA_PS_END,         // 19 - STA exit ps mode. param1: STA's MAC, param2:0
    IEEE80211_EVENT_INTERFACE_ENABLE,   // 20 - wifi stack interface enabled. param1:0, param2:0
    IEEE80211_EVENT_INTERFACE_DISABLE,  // 21 - wifi stack interface disabled. param1:0, param2:0
    IEEE80211_EVENT_ADD_CUSTOMER_IE,    // 22 - notify to add customer IE data. param1: the pointer of wifi frame, param2:customer IE data, output parameter, type: uint8 **, return value: length of customer IE data.
    IEEE80211_EVENT_MAC_CHANGE,         // 23 - mac changed. param1: new MAC, param2:0
    IEEE80211_EVENT_EVM,                // 24 - EVM changed. param1: new evm, param2:sta's MAC.
    IEEE80211_EVENT_UNKNOWN_STA,        // 25 - <ignore>
    IEEE80211_EVENT_PRE_AUTH,           // 26 - STA request to AUTH, you can check blacklist. param1: sta's MAC, param2:0
    IEEE80211_EVENT_PRE_ASSOC,          // 27 - STA request to ASSOC, you can check blacklist. param1: sta's MAC, param2:0
    IEEE80211_EVENT_UNPAIR_SUCCESS,     // 28 - unpaired. param1: peer sta's MAC, param2:0
    IEEE80211_EVENT_TX_BITRATE,         // 29 - <ignore>.
    IEEE80211_EVENT_EXCEPTION_INFO,     // 30 - <ignore>.
    IEEE80211_EVENT_ACS_DONE,           // 31 - <ignore>.
    IEEE80211_EVENT_TX_FRAME,           // 32 - events before WiFi stack sends data. param1: wifi frame data, param2: 0, return value: 0:contuine tx, else stop tx.
    IEEE80211_EVENT_RX_CUSTMGMT,        // 33 - received custom management frame, from the peer sta executed the ieee80211_tx_custmgmt API. param1: struct ieee80211_custmgmt_data *, param2:0.
    IEEE80211_EVENT_FT_START,           // 34
    IEEE80211_EVENT_WRONG_KEY,          // 35
    IEEE80211_EVENT_CHANNEL_CHANGE,     // 36
    IEEE80211_EVENT_P2P_NGO_DONE,       // 37
    IEEE80211_EVENT_WSC_DONE,           // 38
    IEEE80211_EVENT_TX_STATUS,          // 39
    IEEE80211_EVENT_P2P_FOUND,          // 40
    IEEE80211_EVENT_AP_CSA_DONE,        // 41
    IEEE80211_EVENT_SCAN_IDLE,          // 42
    IEEE80211_EVENT_GEN_BEACON,         // 43
    IEEE80211_EVENT_BEACON_IDLE,        // 44
    IEEE80211_EVENT_RELAY_UPDATE,       // 45
    IEEE80211_EVENT_RELAY_ERROR,        // 46
    IEEE80211_EVENT_PAIR_NGO,           // 47
    IEEE80211_EVENT_ROMAING_FAIL,       // 48
    IEEE80211_EVENT_REQ_PAIR_AID,       // 49
};

/* txsemi custom reason code */
enum IEEE80211_TXREASON{
    WLAN_REASON_AP_NOT_FOUND = 60000,
    WLAN_REASON_ASSOC_TIMEOUT,
    WLAN_REASON_PAIR_START,
    WLAN_REASON_UNPAIR,
    WLAN_REASON_RELAY_DISCONNECT,
};

enum wpa_states {
    WPA_DISCONNECTED,       //0
    WPA_INTERFACE_DISABLED, //1
    WPA_INACTIVE,           //2
    WPA_SCANNING,           //3
    WPA_AUTHENTICATING,     //4
    WPA_ASSOCIATING,        //5
    WPA_ASSOCIATED,         //6
    WPA_4WAY_HANDSHAKE,     //7
    WPA_GROUP_HANDSHAKE,    //8
    WPA_COMPLETED,          //9
};

typedef enum {
    IEEE80211_PKTHDL_CONTINUE = 0,
    IEEE80211_PKTHDL_CONSUMED = 1,
} ieee80211_pkthdl_res;

struct ieee80211_custmgmt_data{
    uint8 *from;
    uint8 *data;
    uint8  len;
};

struct ieee80211_csa_param{
    uint8 mode, chan, count;
};

struct ieee80211_hookdata{
    uint8 *data;
    uint16 len;
    uint16 ext: 1, recv: 15;
    uint32 hdl;
};
struct ieee80211_pkthook;
typedef ieee80211_pkthdl_res(*ieee80211_pkthdl)(struct ieee80211_pkthook *hook, uint8 ifidx, struct ieee80211_hookdata *data);
struct ieee80211_pkthook_const{
    const char *name;
    void  *priv;
    uint16 protocol;
    uint16 mcast: 1, ucast:1, rev: 14;
    ieee80211_pkthdl tx;
    ieee80211_pkthdl rx;
};
struct ieee80211_pkthook {
    struct ieee80211_pkthook *next;
    uint32 time_max, consume_data;
    const struct ieee80211_pkthook_const *c_data;
};

typedef int32(*ieee80211_evt_cb)(uint8 ifidx, uint16 evt, uint32 param1, uint32 param2);
struct ieee80211_initparam {
    uint8            vif_maxcnt;
    uint8            bss_maxcnt;
    uint8            headroom, tailroom;
    uint16           sta_maxcnt;
    uint16           bss_lifetime;
    uint16           stack_size;
    uint16           no_rxtask:1, ssid_fuzzy_match:4, rev: 11;
    ieee80211_evt_cb evt_cb;
};

struct ieee80211_scandata {
    uint32 chan_bitmap; //scan channel bitmap
    uint8 ssid[SSID_MAX_LEN+1];
    uint8 scan_time, scan_cnt; //scan time & count at every channel
    uint8 passive_scan: 1, flush: 1, rev: 6;
};

struct ieee80211_stainfo {
    uint16 aid;
    uint8  addr[6];
    int8   rssi, evm;
    uint8  wake_reason;
    uint8  ps: 1, connected: 1, wmm: 1;
    uint32 tx_mcs, rx_mcs;
    uint64 tx_bytes, rx_bytes;
};

struct ieee80211_wmm_param {
    uint16 txop;
    uint16 cw_min;
    uint16 cw_max;
    uint8  aifsn;
    uint8  acm;
};

struct ieee80211_tx_info {
    uint8  band;
    uint8  ifidx;
    uint16 no_ack: 1, rev:15;
    uint32 tag;
};

struct ieee80211_rx_status {
    uint32 freq;
    uint8  band, ifidx;
    int8   rssi, evm;
    uint8  mcs;
    uint8  rx_flags;
    uint16 rxlen;
};

enum ieee80211_rx_flag{
    IEEE80211_RX_FLAG_MORE_DATA = BIT(0),
    IEEE80211_RX_FLAG_AMSDU = BIT(1),
};

struct ieee80211_roaming_param {
    uint8  start: 1, ft_en: 1;
    uint8  state;
    uint8  old_chan;
    int8   rssi_avg, rssi_check;
    uint16 roaming_tmo;
    int16  rssi_sum;
    uint8  bssid_new[6];
    uint8  bssid_old[6];
    uint8  ssid[SSID_MAX_LEN + 1];
    struct ieee80211_bss_info *newbss;
};

struct ieee80211_bss_wpadata {
    uint32 proto;
    uint32 group_cipher;
    uint32 pairwise_cipher;
    uint32 key_mgmt;
};

/*MLME frame expand size*/
struct ieee80211_mlme_size{
    uint16 probe_req, probe_resp;
    uint16 beacon, vendor_spec_action;
    uint16 assoc_req, assoc_resp;
    uint8  auth, deauth, disassoc, addba_resp;
    uint16 p2p_neg;
};

struct ieee80211_linkcost_param{
    //各项指标的上限或下限
    int8   rssi_min;
    int8   evm_min;
    uint8  stas_max;
    uint8  loading_max;
    uint16 txdelay_max;
    uint16 datarate_max;

    //权重系统：百分比
    uint8 rssi_weight;
    uint8 evm_weight;
    uint8 txdelay_weight;
    uint8 loading_weight;
    uint8 stas_weight;
    uint8 per_weight;
    uint8 datarate_weight;

    //各项指标的等级划分：差，中等，优，（百分比）
    uint8 rssi_poor, rssi_maiginal, rssi_good;
    uint8 evm_poor, evm_maiginal, evm_good;
    uint8 loading_poor, loading_maiginal, loading_good;
    uint8 stas_poor, stas_maiginal, stas_good;
    uint8 per_poor, per_maiginal, per_good;
    uint8 txdelay_poor, txdelay_maiginal, txdelay_good;
    uint8 datarate_poor, datarate_maiginal, datarate_good;
};

extern int32 wpa_passphrase(uint8 *ssid, char *passwd, uint8 psk[32]);
extern int32 ieee80211_init(struct ieee80211_initparam *param);
extern int32 ieee80211_deliver_init(int32 max, uint32 lifetime);
extern int32 ieee80211_support_txw830x(void *ops);
extern int32 ieee80211_support_txw80x(void *ops);
extern int32 ieee80211_support_txw81x(void *ops);
extern int32 ieee80211_iface_create_ap(uint8 ifidx, uint8 band);
extern int32 ieee80211_iface_create_sta(uint8 ifidx, uint8 band);
extern int32 ieee80211_iface_create_p2pdev(uint8 ifidx, uint8 band, uint8 go, uint8 gc);
extern int32 ieee80211_iface_start(uint8 ifidx);
extern int32 ieee80211_iface_stop(uint8 ifidx);
extern int32 ieee80211_pair_enable(uint8 ifidx, uint16 pair_magic);
extern int32 ieee80211_pairing(uint8 ifidx, uint16 pair_magic);
extern int32 ieee80211_unpair(uint8 ifidx, uint8 *mac);
extern void  ieee80211_status(uint8 *buff, uint32 size);
extern int32 ieee80211_scan(uint8 index, uint8 start, struct ieee80211_scandata *scan_param);
extern int32 ieee80211_scatter_tx(uint8 ifidx, scatter_data *data, uint32 count);
extern int32 ieee80211_tx(uint8 ifidx, uint8 *data, uint32 len);
extern void ieee80211_input(uint8 ifidx, uint8 *data, uint32 len);
extern int32 ieee80211_tx_mgmt(uint8 ifidx, uint8 *mgmt, uint32 len);
extern int32 ieee80211_tx_custmgmt(uint8 ifidx, uint8 *dest, uint8 *data, uint32 len);
extern int32 ieee80211_tx_ether(uint8 ifidx, uint8 *dest, uint16 proto, uint8 *data, uint32 len);
extern int32 ieee80211_disassoc(uint8 ifidx, uint8 *addr);
extern int32 ieee80211_disassoc_all(uint8 ifidx);
extern int32 ieee80211_register_pkthook(struct ieee80211_pkthook *hook);
extern int32 ieee80211_hook_ext_data(uint8 ifidx, uint8 tx, uint8 *data, uint32 len);
extern ieee80211_evt_cb ieee80211_event_cb(ieee80211_evt_cb cb);
extern void ieee80211_cleanup_bsslist(void);
extern int32 ieee80211_get_bsslist(struct hgic_bss_info *bsslist, int32 list_size, int8 ignore_dis);
extern int32 ieee80211_get_stalist(uint8 ifidx, struct hgic_sta_info *stalist, int32 list_size);
extern int32 ieee80211_conf_stabr_table(uint8 ifidx, uint16 max, int32 lifetime);
extern int32 ieee80211_conf_set_bssbw(uint8 ifidx, uint8 bssbw);
extern int32 ieee80211_conf_get_bssbw(uint8 ifidx);
extern int32 ieee80211_conf_set_chanlist(uint8 ifidx, uint16 *chan_list, uint32 count);
extern int32 ieee80211_conf_set_ssid(uint8 ifidx, uint8 *ssid);
extern int32 ieee80211_conf_get_ssid(uint8 ifidx, uint8 *ssid);
extern int32 ieee80211_conf_set_keymgmt(uint8 ifidx, uint32 keymgmt);
extern int32 ieee80211_conf_get_keymgmt(uint8 ifidx);
extern int32 ieee80211_conf_set_psk(uint8 ifidx, uint8 psk[32]);
extern int32 ieee80211_conf_set_passwd(uint8 ifidx, char *passwd);
extern int32 ieee80211_conf_get_psk(uint8 ifidx, uint8 psk[32]);
extern int32 ieee80211_conf_set_beacon_int(uint8 ifidx, uint32 beacon_int);
extern int32 ieee80211_conf_set_dtim_int(uint8 ifidx, uint16 dtim_int);
extern int32 ieee80211_conf_set_bssid(uint8 ifidx, uint8 *bssid);
extern int32 ieee80211_conf_get_bssid(uint8 ifidx, uint8 *bssid);
extern int32 ieee80211_conf_set_channel(uint8 ifidx, uint8 channel);
extern int32 ieee80211_conf_get_channel(uint8 ifidx);
extern int32 ieee80211_conf_get_mac(uint8 ifidx, uint8 *mac);
extern int32 ieee80211_conf_set_mac(uint8 ifidx, uint8 *mac);
extern int32 ieee80211_conf_set_bss_max_idle(uint8 ifidx, uint16 max_idle_period);
extern int32 ieee80211_conf_get_connstate(uint8 ifidx);
extern uint8 ieee80211_conf_get_wkreason(uint8 ifidx);
extern int32 ieee80211_conf_wakeup_sta(uint8 ifidx, uint8 *addr, uint16 aid, uint32 reason);
extern int32 ieee80211_conf_get_txpower(uint8 ifidx);
extern int32 ieee80211_conf_set_heartbeat_int(uint8 ifidx, uint32 heartbeat_int);
extern int32 ieee80211_conf_set_heartbeat_data(uint8 ifidx, void *data, uint8 retry);
extern int32 ieee80211_conf_set_aplost_time(uint8 ifidx, uint32 time);
extern int32 ieee80211_conf_set_acs(uint8 ifidx, uint8 acs, uint8 tmo);
extern int32 ieee80211_conf_set_wmm_enable(uint8 ifidx, uint8 enable);
extern int32 ieee80211_conf_set_use4addr(uint8 ifidx, uint8 enable);
extern int32 ieee80211_conf_get_use4addr(uint8 ifidx);
extern int32 ieee80211_conf_get_stainfo(uint8 ifidx, uint16 aid, uint8 *mac, struct ieee80211_stainfo *sta);
extern int32 ieee80211_conf_get_stalist(uint8 ifidx, struct ieee80211_stainfo *sta, uint32 count);
extern int32 ieee80211_conf_get_sta_snr(uint8 ifidx, uint8 aid, int8 *snr);
extern int32 ieee80211_conf_get_stacnt(uint8 ifidx);
extern int32 ieee80211_conf_set_aphide(uint8 ifidx, uint8 hide);
extern int32 ieee80211_conf_get_bgrssi(uint8 ifidx, uint8 channel, uint8 *bgr);
extern int32 ieee80211_conf_set_mcast_txrate(uint8 ifidx, uint32 txrate);
extern int32 ieee80211_conf_set_hwmode(uint8 ifidx, enum ieee80211_hwmode mode);
extern int32 ieee80211_conf_set_psdata_cnt(uint8 ifidx, uint8 max_cnt);
extern int32 ieee80211_conf_set_wmm_param(uint8 ifidx, uint8 ac, struct ieee80211_wmm_param *param);
extern int32 ieee80211_conf_get_wmm_param(uint8 ifidx, uint8 ac, struct ieee80211_wmm_param *param);
extern int32 ieee80211_conf_set_wpa_group_rekey(uint8 ifidx, uint32 wpa_group_rekey);
extern int32 ieee80211_deliver_status(uint8 *buff, uint32 size);
extern int32 ieee80211_stabr_status(uint8 ifidx, uint8 *buff, uint32 size);
extern int32 ieee80211_conf_set_datatag(uint8 ifidx, uint32 hdl, uint32 tag);
extern int32 ieee80211_conf_get_ant_sel(uint8 ifidx);
extern int32 ieee80211_conf_get_reason_code(uint8 ifidx);
extern int32 ieee80211_conf_get_status_code(uint8 ifidx);
extern int32 ieee80211_conf_get_rtc(uint8 ifidx);
extern int32 ieee80211_conf_get_acs_result(uint8 ifidx, uint8 *buff);
extern int32 ieee80211_conf_set_rtc(uint8 ifidx, uint8 *data);
extern int32 ieee80211_conf_set_radio_onoff(uint8 ifidx, uint8 en);
extern int32 ieee80211_conf_set_isolate(uint8 ifidx, uint8 isolate);
extern int32 ieee80211_conf_set_ft(uint8 ifidx, void *ft_param);
extern int32 ieee80211_chan_center_freq(uint8 ifidx, uint8 channel);
extern int32 ieee80211_get_bssinfo(struct hgic_bss_info *bss, uint8 band, uint8 *ssid, uint8 *bssid);
extern int32 ieee80211_conf_set_dupfilter(uint8 ifidx, uint8 enable);
extern int32 ieee80211_conf_set_conn_timeout(uint8 ifidx, uint32 auth_tmo, uint32 assoc_tmo, uint32 handshake_tmo);
extern int32 ieee80211_bss_add_manualAP(uint8 *ssid, uint8 *bssid, uint8 channel, uint8 band, struct ieee80211_bss_wpadata *wpa_data);
extern int32 ieee80211_bss_get_wpadata(uint8 *bssid, uint8 band, struct ieee80211_bss_wpadata *wpa_data);
extern int32 ieee80211_expand_mlme_size(uint8 ifidx, struct ieee80211_mlme_size *size);
extern int32 ieee80211_conf_set_csa(uint8 ifidx, struct ieee80211_csa_param *csa);
extern void ieee80211_hook_status(void);
extern int32 ieee80211_conf_set_wpa_cipher(uint8 ifidx, uint32 wpa_proto, uint32 pairwise_cipher, uint32 group_cipher);
extern int32 ieee80211_conf_set_p2p_dev_name(uint8 ifidx, char *dev_name);
extern int32 ieee80211_conf_set_p2p_model_name(uint8 ifidx, char *model_name);
extern int32 ieee80211_conf_set_p2p_model_num(uint8 ifidx, char *model_num);
extern int32 ieee80211_conf_set_p2p_vendor_name(uint8 ifidx, char *vendor_name);
extern int32 ieee80211_conf_set_p2p_serial_number(uint8 ifidx, char *serial_number);
extern int32 ieee80211_conf_set_p2p_uuid(uint8 ifidx, char *uuid);
extern int32 ieee80211_conf_set_p2p_methods(uint8 ifidx, char *methods);
extern int32 ieee80211_conf_set_p2p_dev_type(uint8 ifidx, char *dev_type);
extern int32 ieee80211_conf_set_wfd_element(uint8 ifidx, uint8 id, uint8 *data, uint16 len);
extern int32 ieee80211_conf_set_absolute_beacon(uint8 ifidx, uint8 en);
extern int32 ieee80211_conf_set_auto_assoc(uint8 ifidx, uint8 auto_assoc, uint8 auto_scan);
extern int32 ieee80211_conf_set_relay_mode(uint8 ifidx, uint8 enable, uint8 relay_level, uint8 relay_mcast);
extern int32 ieee80211_conf_get_relay_level(uint8 ifidx);
extern int32 ieee80211_conf_set_listen_interval(uint8 ifidx, uint16 listen_interval);
extern int32 ieee80211_conf_set_scan_max(uint8 ifidx, uint16 scan_max);
extern int32 ieee80211_conf_set_signal_threshold(uint8 ifidx, uint8 rssi_thres, uint8 evm_thres);
extern int32 ieee80211_conf_set_security_policy(uint8 ifidx, uint8 security_policy);
extern int32 ieee80211_conf_set_sae_pwe_loop(uint8 ifidx, uint8 loop);
extern int32 ieee80211_conf_set_ap_psmode_en(uint8 ifidx, uint8 enable);
extern int32 ieee80211_conf_set_pair_ngo(uint8 ifidx, uint8 enable);

extern void ieee80211_crypto_ec_support(void);
extern void ieee80211_crypto_ecdh_support(void);
extern void ieee80211_crypto_aes_support(void);
extern void ieee80211_crypto_bignum_support(void);
extern void ieee80211_crypto_dh_support(void);
extern void ieee80211_crypto_sha_support(void);
extern void ieee80211_crypto_hash_support(void);

#ifdef __cplusplus
}
#endif
#endif

