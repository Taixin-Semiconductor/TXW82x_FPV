#ifndef _UBLE_HCI_HOST_H_
#define _UBLE_HCI_HOST_H_

#include "basic_include.h"

#ifdef __cplusplus
extern "C" {
#endif

struct hci_data {
    uint8   type;
    uint8   *data;
    uint32  len;
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

int32 bt_hci_tx_acl_data(uint8 *data, uint32 len);
int32 bt_hci_tx_adv_data(uint8 *data, uint32 len);

int32 bt_hci_set_mode(uint8 mode, uint8 chan);
int32 bt_hci_set_advdata(uint8 *data, uint32 data_len);
int32 bt_hci_set_scan_rsp(uint8 *data, uint32 data_len);
int32 bt_hci_set_devaddr(uint8 *addr);
int32 bt_hci_set_adv_interval(uint32 interval);
int32 bt_hci_set_adv_en(uint8 enable);
int32 bt_hci_set_adv_type_filter(uint8 type);
int32 bt_hci_set_coexist_en(uint8 coexist_en, uint8 duty_en);
int32 bt_hci_set_ll_length(uint16 length);

#ifdef __cplusplus
}
#endif

#endif


