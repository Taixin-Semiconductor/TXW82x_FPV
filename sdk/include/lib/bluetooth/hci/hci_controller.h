#ifndef _HCI_CONTROLLER_H_
#define _HCI_CONTROLLER_H_

#include "basic_include.h"
#include "lib/skb/skb.h"
#include "lib/lmac/lmac_def.h"

#ifdef __cplusplus
extern "C" {
#endif

enum BLE_LL_TYPE {
    BLE_LL_TYPE_ADV_TRX = 0x0,
    BLE_LL_TYPE_ADV_CUSTOM,
    BLE_LL_TYPE_CONNECT,
};

enum BLE_LL_EVENT {
    BLE_LL_EVENT_CONNECTED = 0x1,
    BLE_LL_EVENT_DISCONNECTED,
    BLE_LL_EVENT_EXCHANGE_LEHGTH,
};

enum BLE_LL_IOCTL {
    BLE_IOCTL_SET_DEV_ADDR = 1,
    BLE_IOCTL_SET_ADV_DATA = 2,
    BLE_IOCTL_SET_SCAN_RESP = 3,
    BLE_IOCTL_SET_ADV_INTERVAL = 4,
    BLE_IOCTL_SET_ADV_EN = 5,
    BLE_IOCTL_SET_ADV_TYPE_FILTER = 6,
    BLE_IOCTL_GET_PERR_ADDRTYPE = 7,
    BLE_IOCTL_GET_PERR_ADDR = 8,
    BLE_IOCTL_GET_CONN_INTERVAL = 9,
    BLE_IOCTL_GET_CONN_LATENCY = 10,
    BLE_IOCTL_GET_SUPERVISION_TIMEOUT = 11,
    BLE_IOCTL_SET_COEXIST_EN = 12,
    BLE_IOCTL_GET_COEXIST_CRASH = 13,
    BLE_IOCTL_SET_LL_LENGTH = 14,
    BLE_IOCTL_SET_DISCONNECT = 15,
    BLE_IOCTL_GET_RSSI = 16,
	BLE_IOCTL_GET_BLE_EN = 17,
};

struct bt_rx_info {
    uint8   channel;    //current channel
    uint8   con_handle; //hci handle
    int8    rssi;
    uint8   frm_type;
    uint32  rev;
};

struct bt_tx_info {
    uint16   con_handle; //hci handle
    uint8    data_type;  //hci type
    uint8    r1;
};

struct bt_ops {
    void *priv;
    void *lops;
    uint8 headroom, tailroom;
    int32(*open)(struct bt_ops *ops, uint8 type, uint8 chan);
    int32(*close)(struct bt_ops *ops);
    int32(*tx)(struct bt_ops *ops, struct sk_buff *skb);
    int32(*tx_status)(void *priv, struct sk_buff *skb);
    int32(*rx)(void *priv, struct sk_buff *skb);
    int32(*ioctl)(struct bt_ops *ops, uint32 cmd, uint32 param1, uint32 param2);
    int32(*event)(void *priv, uint32 event_id, uint32 param1, uint32 param2);
};

#define ble_ll_open(ops, type, chan)                      ops->open(ops, type, chan)
#define ble_ll_close(ops)                                 ops->close(ops)
#define ble_ll_set_advdata(ops, data, len)                ops->ioctl(ops, BLE_IOCTL_SET_ADV_DATA, (uint32)(data), (len))
#define ble_ll_set_scan_rsp(ops, data, len)               ops->ioctl(ops, BLE_IOCTL_SET_SCAN_RESP, (uint32)(data), (len))
#define ble_ll_set_devaddr(ops, addr)                     ops->ioctl(ops, BLE_IOCTL_SET_DEV_ADDR, (uint32)(addr), 0)
#define ble_ll_set_adv_interval(ops, interval)            ops->ioctl(ops, BLE_IOCTL_SET_ADV_INTERVAL, (uint32)(interval), 0)
#define ble_ll_set_adv_en(ops, en)                        ops->ioctl(ops, BLE_IOCTL_SET_ADV_EN, (uint32)(en), 0)
#define ble_ll_set_adv_type_filter(ops, type)             ops->ioctl(ops, BLE_IOCTL_SET_ADV_TYPE_FILTER, (uint32)(type), 0)
#define ble_ll_get_peer_addr(ops, hdl)                    ops->ioctl(ops, BLE_IOCTL_GET_PERR_ADDR, (uint32)(hdl), 0)
#define ble_ll_get_peer_addrtype(ops, hdl)                ops->ioctl(ops, BLE_IOCTL_GET_PERR_ADDRTYPE, (uint32)(hdl), 0)
#define ble_ll_get_conn_interval(ops, hdl)                ops->ioctl(ops, BLE_IOCTL_GET_CONN_INTERVAL, (uint32)(hdl), 0)
#define ble_ll_get_conn_latency(ops, hdl)                 ops->ioctl(ops, BLE_IOCTL_GET_CONN_LATENCY, (uint32)(hdl), 0)
#define ble_ll_get_supervision_timeout(ops, hdl)          ops->ioctl(ops, BLE_IOCTL_GET_SUPERVISION_TIMEOUT, (uint32)(hdl), 0)
#define ble_ll_set_coexist_en(ops, coexist_en, duty_en)   ops->ioctl(ops, BLE_IOCTL_SET_COEXIST_EN, (uint32)(coexist_en), (uint32)(duty_en))
#define ble_ll_get_coexist_crash(ops)                     ops->ioctl(ops, BLE_IOCTL_GET_COEXIST_CRASH, 0, 0)
#define ble_ll_set_ll_length(ops, length)                 ops->ioctl(ops, BLE_IOCTL_SET_LL_LENGTH, length, 0)
#define ble_ll_set_disconnect(ops)                        ops->ioctl(ops, BLE_IOCTL_SET_DISCONNECT, 0, 0)
#define ble_ll_get_rssi(ops)                              ops->ioctl(ops, BLE_IOCTL_GET_RSSI, 0, 0)
#define ble_ll_get_ble_en(ops)                            ops->ioctl(ops, BLE_IOCTL_GET_BLE_EN, 0, 0)

struct bt_ops *ble_ll_init(struct lmac_ops *ops);
int32 bt_hci_init(struct bt_ops *btops);

#ifdef __cplusplus
}
#endif

#endif

