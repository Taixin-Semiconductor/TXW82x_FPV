#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "osal/mutex.h"
#include "osal/work.h"
#include "osal/semaphore.h"
#include "osal/timer.h"
#include "hal/netdev.h"
#include "hal/dma.h"
#include "hal/netdev.h"
#include "lib/common/common.h"
#include "lib/heap/sysheap.h"
#include "lib/umac/ieee80211.h"
#include "lib/skb/skb.h"
#include "lib/skb/skb_list.h"
#include "lib/net/utils.h"
#include "syscfg.h"

#define WIFI_DEV_ICMP_MONITOR 0

struct wifi_dev {
    struct netdev   wifi;
    netdev_input_cb input_cb;
    void           *input_priv;
    uint8           addr[6];
    uint8           ifidx;
    uint8           icmp_mntr: 1, rev: 7;
    uint32          no_mem;
};

static void wifi_dev_icmp_monitor(struct wifi_dev *wifi, uint8 *data, uint32 len, const char *prefix)
{
#if WIFI_DEV_ICMP_MONITOR
    if (wifi->icmp_mntr) {
        icmp_pkt_monitor(prefix, data, len);
    }
#endif
}

static void wifi_dev_icmp_monitor_scatter(struct wifi_dev *wifi, scatter_data *data, uint32 count, const char *prefix)
{
#if WIFI_DEV_ICMP_MONITOR
    if (wifi->icmp_mntr) {
        icmp_pkt_monitor_scatter(prefix, data, count);
    }
#endif
}

static int32 wifi_dev_open(struct netdev *ndev, netdev_input_cb cb, netdev_event_cb evt_cb, void *priv)
{
    struct wifi_dev *wifi = (struct wifi_dev *)ndev;
    uint8 ifidx = (wifi->ifidx == WIFI_MODE_APSTA ? WIFI_MODE_STA : wifi->ifidx);

    if (cb) {
        uint32 flags = disable_irq();
        wifi->input_cb   = cb;
        wifi->input_priv = priv;
        enable_irq(flags);
    }

    return ieee80211_iface_start(ifidx);
}

static int32 wifi_dev_close(struct netdev *ndev)
{
    struct wifi_dev *wifi = (struct wifi_dev *)ndev;
    uint8 ifidx = (wifi->ifidx == WIFI_MODE_APSTA ? WIFI_MODE_STA : wifi->ifidx);
    return ieee80211_iface_stop(ifidx);
}

static int32 wifi_dev_send(struct netdev *ndev, uint8 *data, uint32 size)
{
    struct wifi_dev *wifi = (struct wifi_dev *)ndev;
    uint8 ifidx = (wifi->ifidx == WIFI_MODE_APSTA ? 0 : wifi->ifidx);

    wifi->wifi.tx_bytes += size;
    wifi_dev_icmp_monitor(wifi, data, size, "WIFI_DEV TX");
    return ieee80211_tx(ifidx, (uint8 *)data, size);
}

static int32 wifi_dev_scatter_send(struct netdev *ndev, scatter_data *data, uint32 count)
{
    struct wifi_dev *wifi = (struct wifi_dev *)ndev;
    uint8 ifidx = (wifi->ifidx == WIFI_MODE_APSTA ? 0 : wifi->ifidx);

    wifi->wifi.tx_bytes += scatter_size(data, count);
    wifi_dev_icmp_monitor_scatter(wifi, data, count, "WIFI_DEV TX");
    return ieee80211_scatter_tx(ifidx, data, count);
}

static int32 wifi_dev_hook_data(struct netdev *ndev, uint8 *data, uint32 len)
{
    struct wifi_dev *wifi = (struct wifi_dev *)ndev;
    uint8 ifidx = wifi->ifidx == WIFI_MODE_APSTA ? WIFI_MODE_STA : wifi->ifidx;
    if (IS_MCAST_ADDR(data) && (ieee80211_conf_get_connstate(ifidx) >= WPA_GROUP_HANDSHAKE)) {
        return IEEE80211_PKTHDL_CONTINUE;
    }
    return ieee80211_hook_ext_data(ifidx, 1, data, len);
}

static int32 wifi_dev_ioctl(struct netdev *ndev, uint32 cmd, uint32 param1, uint32 param2)
{
    int32 ret = RET_OK;
    struct wifi_dev *wifi = (struct wifi_dev *)ndev;
    uint8 ifidx = (wifi->ifidx == WIFI_MODE_APSTA ? WIFI_MODE_STA : wifi->ifidx);

    switch (cmd) {
        case NETDEV_IOCTL_GET_ADDR:
            ieee80211_conf_get_mac(ifidx, wifi->addr);
            os_memcpy((uint8 *)param1, wifi->addr, 6);
            break;
        case NETDEV_IOCTL_SET_ADDR:
            ieee80211_conf_set_mac(ifidx, (uint8 *)param1);
            os_memcpy(wifi->addr, (uint8 *)param1, 6);
            break;
        case NETDEV_IOCTL_GET_LINK_STATUS:
            ret = (ieee80211_conf_get_connstate(ifidx) >= WPA_GROUP_HANDSHAKE);
            break;
        case NETDEV_IOCTL_GET_LINK_SPEED:
            break;
#if WIFI_SINGLE_DEV
        case NETDEV_IOCTL_SET_WIFI_MODE:
            wifi->ifidx = (uint8)param1;
            break;
#endif
        case NETDEV_IOCTL_HOOK_INPUTDATA:
            ret = !wifi_dev_hook_data(ndev, (uint8 *)param1, param2);
            break;
        case NETDEV_IOCTL_ENABLE_ICMPMNTR:
            wifi->icmp_mntr = param1 ? 1 : 0;
            break;
        default:
            ret = -ENOTSUPP;
            break;
    }
    return ret;
}

int32 sys_wifi_recv(void *priv, uint8 *data, uint32 len, uint32 flags)
{
    netdev_input_cb  input_cb;
    void            *input_priv;
    struct wifi_dev *wifi  = (struct wifi_dev *)priv;

    uint32 f = disable_irq();
    input_cb   = wifi->input_cb;
    input_priv = wifi->input_priv;
    enable_irq(f);

    if (input_cb) {
        wifi_dev_icmp_monitor(wifi, data, len, "WIFI_DEV RX");
        input_cb(&wifi->wifi, data, len, input_priv);
        wifi->wifi.rx_bytes += len;
    }

    return RET_OK;
}

static const struct netdev_hal_ops wifi_dev_ops = {
    .open      = wifi_dev_open,
    .close     = wifi_dev_close,
    .ioctl     = wifi_dev_ioctl,
    .send_data = wifi_dev_send,
    .send_scatter_data = wifi_dev_scatter_send,
};

__init void *sys_wifi_register(uint32 ifidx)
{
    struct wifi_dev *wifi;
    uint32 dev_id = HG_WIFI0_DEVID + ifidx - 1;

#if WIFI_SINGLE_DEV
    dev_id = HG_WIFI0_DEVID;
    wifi   = (struct wifi_dev *)dev_get(dev_id);
    if (wifi) {
        return wifi;
    }
#endif

    wifi = (struct wifi_dev *)os_zalloc(sizeof(struct wifi_dev));
    ASSERT(wifi);
    wifi->wifi.dev.ops = (const struct devobj_ops *)&wifi_dev_ops;
    wifi->ifidx        = (uint8)ifidx;
    dev_register(dev_id, (struct dev_obj *)wifi);
    return wifi;
}

void wifi_dev_status(uint32 dev_id)
{
    struct wifi_dev *wifi = (struct wifi_dev *)dev_get(dev_id);

    if (wifi == NULL) {
        return;
    }

    os_printf("WiFi Dev Status: no_mem=%u\r\n", wifi->no_mem);
    os_printf("    wifi: tx_bytes:%llu, rx_bytes:%llu\r\n", wifi->wifi.tx_bytes, wifi->wifi.rx_bytes);
}

