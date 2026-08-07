#ifndef HGIC_WIFI_MGR_H_
#define HGIC_WIFI_MGR_H_

#include "sys_config.h"
#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "list.h"
#include "osal/string.h"
#include "osal/mutex.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/task.h"
#include "osal/timer.h"
#include "hal/netdev.h"
#include "lib/common/rbuffer.h"
#include "lib/bus/macbus/mac_bus.h"
#include "lib/skb/skb.h"
#include "lib/skb/skb_list.h"
#include "lib/lmac/hgic.h"
#include "lib/net/utils.h"
#include "lib/umac/ieee80211.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ETH_P_WIFIMGR (0x4855)
#define ETH_P_HGRAW   (0x4884)

#define WIFIMGR_WORK_CNT    (4)
#define WIFIMGR_LOCAL_IDCNT (16)

struct wifi_mgr;

enum wifimgr_frm_type {
    WIFIMGR_FRM_TYPE_ETHER = 0,
    WIFIMGR_FRM_TYPE_HGIC,
    WIFIMGR_FRM_TYPE_RAW,
};

enum wifimgr_stype {
    WIFI_MGR_STYPE_NONE    = 0,
    WIFI_MGR_STYPE_PSALIVE = 1,
    WIFI_MGR_STYPE_MGRCMD  = 2,
};

enum wifimgr_cmd{
    WIFIMGR_CMD_RESET_STA,
    WIFIMGR_CMD_SET_STA_FREQINFO,
};

struct wifimgr_hdr {
    uint16 magic, len, crc;
    uint8  type, r1, r2, r3;
};

typedef int32(*wifimgr_work_cb)(struct wifi_mgr *mgr, uint32 param1, uint32 param2);
struct wifimgr_work {
    uint32 param1, param2;
    wifimgr_work_cb run;
};

struct wifi_dev {
    struct netdev   ndev;
    netdev_input_cb input_cb;
    uint8  addr[6];
    uint8  ifidx;
    uint8  open:1;
    struct wifi_mgr *mgr;
};

struct wifi_mgr {
    struct netdev   ndev;
    netdev_input_cb input_cb;
    void           *input_priv;
    uint8  addr[6];
    uint8  frm_type;
    uint8  open:1, netif:1, init_report:1, icmp_mntr:1;
    uint32 if_recv, if_write, if_err;
    uint16 bus_txcookie, bus_rxcookie;
    uint16 drv_aggsize;
    ieee80211_evt_cb old_cb;
    struct list_head smod;
    struct mac_bus *bus;
    struct skb_list cmdlist;
    struct skb_list up2host;
    struct skb_list cachedata;
    struct os_work  work;
    os_mutex_t lock;
    os_timer_t timer;

    uint32 agg_len;
    uint8 *agg_buff;
    int8   rssi[2];
    uint16 host_alive;
    struct sk_buff *cmdskb;
    struct lmac_ops *lops;
    struct bt_ops   *btops;
    uint32 local_ids[WIFIMGR_LOCAL_IDCNT];
};

struct wifimgr_submod {
    struct list_head list;
    struct wifi_mgr *mgr;
    uint8 type;
    const char *name;
    int32(*event_proc)(struct wifimgr_submod *smod, uint8 ifidx, uint16 evt, uint32 param1, uint32 param2);
    int32(*cmd_proc)(struct wifimgr_submod *smod, struct sk_buff **skb);
    int32(*rx_wifimgr)(struct wifimgr_submod *smod, uint8 ifidx, uint8 *data, uint32 len);
};

struct sk_buff *wifi_mgr_alloc_resp(struct sk_buff *skb, int8 *data, int32 len);
void wifi_mgr_print2host(void *priv, char *msg, int len);
int32 wifi_mgr_notify_host(struct wifi_mgr *mgr, uint8 event_id, uint8 *args, int32 len);
int32 wifi_mgr_buswrite_cmdresp(struct wifi_mgr *mgr, struct sk_buff *skb, int status);
int32 wifi_mgr_buswrite_data(struct wifi_mgr *mgr, uint8 *data, uint32 len, uint32 flags);
int32 wifi_mgr_send_wifimgr_data(uint8 *dest, uint8 stype, uint8 *data, uint32 len);
void wifi_mgr_switch(int8 to_host);
void wifi_mgr_add_submod(struct wifimgr_submod *smod);
int32 wifi_mgr_init(enum mac_bus_type bus_type, int8 frm_type, void *ops, void *btops, struct macbus_param *param);
void wifi_mgr_proc_hgic_cmd(struct wifi_mgr *mgr, struct sk_buff *skb);
void wifi_mgr_status(void);

int32 wifi_mgr_reset_sta(struct wifi_mgr *mgr, uint8 *addr);
int32 wifi_mgr_set_sta_freqinfo(struct wifi_mgr *mgr, uint8 *freqinfo, uint32 size);
int32 wifi_mgr_enable_psalive(uint8 ifidx, uint8 wnb_support);
void wifi_mgrcmd_enable(void);
int32 wifi_mgr_enable_psconnect(void);
void wifi_mgr_enable_dhcpc(void);

int32 host_cmd_resp(int16 ret, uint8 *resp, uint32 resp_len, void *hdr);
int32 host_data_send(uint8 *data, uint32 data_len);
int32 host_event_new(uint32 evt_id, uint8 *data, uint32 data_len);



#ifdef __cplusplus
}
#endif
#endif

