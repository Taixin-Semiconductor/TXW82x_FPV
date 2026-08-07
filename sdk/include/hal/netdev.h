#ifndef _HAL_NETDEVICE_H_
#define _HAL_NETDEVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

enum netdev_events {
    NETDEV_EVT_LINK_STATUS_CHANGE,
};

enum netdev_link_status {
    NETDEV_LINK_DOWN,
    NETDEV_LINK_UP,
};

enum netdev_phy_event {
    NETDEV_PHY_EVENT_LINK_CHANGED,
};

enum netdev_ioctl_cmd {
    NETDEV_IOCTL_GET_ADDR,
    NETDEV_IOCTL_SET_ADDR,
    NETDEV_IOCTL_GET_LINK_STATUS,
    NETDEV_IOCTL_GET_LINK_SPEED,
    NETDEV_IOCTL_FORCE_LINK,
    NETDEV_IOCTL_SET_FILTER_MAC,
    NETDEV_IOCTL_SET_WIFI_MODE,
    NETDEV_IOCTL_HOOK_INPUTDATA,
    NETDEV_IOCTL_ENABLE_WIFIBRIDGE,
    NETDEV_IOCTL_ENABLE_ICMPMNTR,
    NETDEV_IOCTL_CLEAR_ROUTETBL,
    NETDEV_IOCTL_SET_MCAST_FIREWALL,    
    NETDEV_IOCTL_SET_TX_NONBLOCK,
};

struct netdev;

typedef void (*netdev_input_cb)(struct netdev *ndev, uint8 *data, uint32 size, void *priv);
typedef void (*netdev_event_cb)(struct netdev *ndev, uint32 event, uint32 param, void *priv);

struct netdev {
    struct dev_obj dev;
    void  *stack_data;
    uint64 rx_bytes, tx_bytes;
};

struct netdev_hal_ops {
    struct devobj_ops ops;
    int32(*open)(struct netdev *ndev, netdev_input_cb input_cb, netdev_event_cb evt_cb, void *priv);
    int32(*close)(struct netdev *ndev);
    int32(*ioctl)(struct netdev *ndev, uint32 cmd, uint32 param1, uint32 param2);
    int32(*send_data)(struct netdev *ndev, uint8 *p_data, uint32 size);
    int32(*send_scatter_data)(struct netdev *ndev, scatter_data *data, uint32 count);
    int32(*mdio_write)(struct netdev *ndev, uint16 phy_addr, uint16 reg_addr, uint16 data);
    int32(*mdio_read)(struct netdev *ndev, uint16 phy_addr, uint16 reg_addr);
    void (*phy_event)(void *phydev, uint32 event, uint32 param1, uint32 param2);
};

int32 netdev_open(struct netdev *ndev, netdev_input_cb input_cb, netdev_event_cb evt_cb, void *priv);
int32 netdev_close(struct netdev *ndev);
int32 netdev_send_data(struct netdev *ndev, uint8 *p_data, uint32 size);
int32 netdev_send_scatter_data(struct netdev *ndev, scatter_data *p_data, uint32 count);
int32 netdev_ioctl(struct netdev *ndev, uint32 cmd, uint32 param1, uint32 param2);
int32 netdev_mdio_write(struct netdev *ndev, uint16 phy_addr, uint16 reg_addr, uint16 data);
int32 netdev_mdio_read(struct netdev *ndev, uint16 phy_addr, uint16 reg_addr);

#define netdev_link_speed(ndev) netdev_ioctl(ndev, NETDEV_IOCTL_GET_LINK_SPEED, 0, 0)
#define netdev_link_status(ndev) netdev_ioctl(ndev, NETDEV_IOCTL_GET_LINK_STATUS, 0, 0)
#define netdev_set_macaddr(ndev, addr) netdev_ioctl(ndev, NETDEV_IOCTL_SET_ADDR, (uint32)addr, 0)
#define netdev_get_macaddr(ndev, addr) netdev_ioctl(ndev, NETDEV_IOCTL_GET_ADDR, (uint32)addr, 0)
#define netdev_force_link(ndev, link_speed, duplex_flag) netdev_ioctl(ndev, NETDEV_IOCTL_FORCE_LINK, link_speed, duplex_flag)
#define netdev_set_filter_mac(ndev, mac_table, size) netdev_ioctl(ndev, NETDEV_IOCTL_SET_FILTER_MAC, (uint32)mac_table, (uint32)size)
#define netdev_set_wifi_mode(ndev, mode) netdev_ioctl(ndev, NETDEV_IOCTL_SET_WIFI_MODE, mode, 0)
#define netdev_hook_inputdata(ndev, data, len) netdev_ioctl(ndev, NETDEV_IOCTL_HOOK_INPUTDATA, data, len)
#define netdev_set_wifi_bridge(ndev, en) netdev_ioctl(ndev, NETDEV_IOCTL_ENABLE_WIFIBRIDGE, en, 0)
#define netdev_clear_routetbl(ndev) netdev_ioctl(ndev, NETDEV_IOCTL_CLEAR_ROUTETBL, 0, 0)
#define netdev_set_mcastfw(ndev, tot_1s, each_1s) netdev_ioctl(ndev, NETDEV_IOCTL_SET_MCAST_FIREWALL, tot_1s, each_1s)
#define netdev_set_tx_nonblock(ndev, nonblock) netdev_ioctl(ndev, NETDEV_IOCTL_SET_TX_NONBLOCK, nonblock, 0)


#ifdef __cplusplus
}
#endif
#endif

