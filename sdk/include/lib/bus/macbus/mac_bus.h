
#ifndef _MAC_BUS_H_
#define _MAC_BUS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define DATA_AREA_SIZE (1700)

enum mac_bus_type {
    MAC_BUS_TYPE_NONE,
    MAC_BUS_TYPE_SDIO,
    MAC_BUS_TYPE_USB,
    MAC_BUS_TYPE_EMAC,
    MAC_BUS_TYPE_UART,
    MAC_BUS_TYPE_NDEV,
    MAC_BUS_TYPE_QC,
    MAC_BUS_TYPE_COMBI,
    MAC_BUS_TYPE_SPI,
    MAC_BUS_TYPE_MMC,
    MAC_BUS_TYPE_GDUART,
};

enum MAC_BUS_IOCTL{
    MACBUS_IOCTL_IS_BUSY = 1,
    MACBUS_IOCTL_REINIT_BUS,
    MACBUS_IOCTL_BOOTDL_FLAG,
    MACBUS_IOCTL_BUFFER_IDLE,
};

struct macbus_param{
    uint16 drv_aggsize;
    uint16 sdio_clk;
};


struct mac_bus {
    int type;
    void *priv;
    uint32 rxerr, txerr;
    int (*write)(struct mac_bus *bus, unsigned char *data, int len);
    int (*write_scatter)(struct mac_bus *bus, scatter_data *data, int count);
    int (*recv)(struct mac_bus *bus, unsigned char *data, int len);
    int (*ioctl)(struct mac_bus *bus, uint32 cmd, uint32 param1, uint32 param2);
};

typedef int (*mac_bus_recv)(struct mac_bus *bus, unsigned char *data, int len);
extern struct mac_bus *mac_bus_attach(int bus_type, mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_sdio_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_usb_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_uart_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_gmac_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_qa_gmac_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_qc_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_ndev_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_combi_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_sdspi_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_spi_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_mmc_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);
extern struct mac_bus *mac_bus_gduart_attach(mac_bus_recv recv, void *priv, struct macbus_param *param);

extern void mac_bus_sdio_detach(struct mac_bus *bus);
extern void mac_bus_usb_detach(struct mac_bus *bus);
extern void mac_bus_gmac_detach(struct mac_bus *bus);
extern void mac_bus_spi_detach(struct mac_bus *bus);

#define MACBUS_IS_BUSY(bus)         (bus)->ioctl(bus, MACBUS_IOCTL_IS_BUSY, 0, 0)
#define MACBUS_BOOTDL(bus, bootdl)  (bus)->ioctl(bus, MACBUS_IOCTL_BOOTDL_FLAG, bootdl, 0)
#define MACBUS_BUFFER_IDLE(bus)     (bus)->ioctl(bus, MACBUS_IOCTL_BUFFER_IDLE, 0, 0)
#define MACBUS_REINIT(bus)          (bus)->ioctl(bus, MACBUS_IOCTL_REINIT_BUS, 0, 0)

#ifdef __cplusplus
}
#endif

#endif

