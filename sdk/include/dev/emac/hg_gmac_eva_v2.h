#ifndef _HG_GMAC_EVA_V2_H_
#define _HG_GMAC_EVA_V2_H_

#include "osal/task.h"
#include "lib/net/ethphy/eth_phy.h"
#include "hal/netdev.h"

#ifdef __cplusplus
extern "C" {
#endif

#define gmac_v2_dbg(fmt, ...)               os_printf(fmt, ##__VA_ARGS__)

struct hg_gmac_data_v2 {
    struct hg_gmac_v2_rx_descriptor volatile  *p_rx_des;
    struct hg_gmac_v2_tx_descriptor volatile  *p_tx_des;

    uint8                                  *p_rx_malloc;
    uint8                                  *p_tx_malloc;
    
    /*! In order not to splicing buf, the buf of the last RX descriptor
     *  should be able to fill one frame.
     */
    /*!
     *      struct hg_gmac_v2_tx_descriptor  rx_des[rx_des_num];
     *      uint8                         rx_buf[rx_des_num-1][HG_GMAC_V2_RX_PER_BUF_SIZE];
     *      uint8                         last_rx_buf[HG_GMAC_V2_RX_FRAME_MAX_SIZE];
     */
    uint32                                  rx_des_num;
    struct hg_gmac_v2_rx_descriptor        *p_rx_des_buf;

    uint8                                  *p_rx_buf;
    /*!
     *      struct hg_gmac_v2_tx_descriptor  tx_des[tx_des_num];
     *      uint8                         tx_buf[tx_des_num][HG_GMAC_V2_TX_PER_BUF_SIZE];
     */
    uint32                                  tx_des_num;
    struct hg_gmac_v2_tx_descriptor        *p_tx_des_buf;
    uint8                                  *p_tx_buf;
};

struct hg_gmac_eva_v2_statistics {
    uint32                          tx_cnt;
    uint32                          rx_cnt;
    uint32                          ru_pending_cnt;
    uint32                          rx_err_cnt;
};

struct hg_gmac_eva_v2 {
    struct netdev                   dev;
    uint32                          hw;
    uint32                          modbus_devid,
                                    phy_devid,
                                    crc_devid;
    struct crc_dev                 *crcdev;
    uint32                          mdio_pin;
    uint32                          mdc_pin;
    uint32                          irq_num;
    uint32                          opened:1, tx_nonblock:1, rev:30;
	uint32 							rgmii_en;
    struct hg_gmac_data_v2          ctrl_data;
    uint8                           mac_addr[6];
    netdev_input_cb                 input_cb;
    void                           *input_priv;
    uint32                          tx_buf_size;
    uint32                          rx_buf_size;
    os_task_t                  receive_task;
    struct ethernet_phy_device     *phydev;
    struct ethernet_mdio_bus       *mdio_bus;
    
    /* Save link information */
    int32                           speed;
    int32                           duplex;
    int32                           link_status;
    
    os_semaphore_t             receive_sema;
    os_mutex_t                 mdio_mutex;
    os_mutex_t                 send_mutex;
    
    struct hg_gmac_eva_v2_statistics statistics;
};


int32 hg_gmac_v2_attach(uint32 dev_id, struct hg_gmac_eva_v2 *gmac);
int32 hg_gmac_v2_has_received_frame(struct netdev *ndev);
int32 hg_gmac_v2_receive_data(struct netdev *emac_dev);

#ifdef __cplusplus
}
#endif

#endif
