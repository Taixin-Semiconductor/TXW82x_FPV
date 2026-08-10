#ifndef _WIFI_ZROAM_H_
#define _WIFI_ZROAM_H_

#include "lib/skb/skb.h"
#include "lib/bus/macbus/mac_bus.h"
#include "lib/skb/skb_list.h"
#include "lib/lmac/lmac_def.h"
#include "lib/lmac/hgic.h"

struct wifi_zroam_config {
    uint16 MDID;
    uint8  zroam_psk[32];
};

struct lmac_ops *wifi_zroam_init(uint8 master, struct lmac_ops *lops, uint32 bus_type, struct wifi_zroam_config *cfg);
void wifi_zroam_start(uint8 start);
int32 wifi_zroam_recv(uint8 *data, uint32 len);

#endif
