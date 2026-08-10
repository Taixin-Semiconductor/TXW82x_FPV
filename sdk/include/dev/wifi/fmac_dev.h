#ifndef _FMAC_DEV_H_
#define _FMAC_DEV_H_

#ifdef __cplusplus
extern "C" {
#endif

int32 hgic_fmac_attach(uint32 dev_id, int32 bus_type, const char *fw_data, uint8 reset_io);

int32 fmac_dev_open(struct netdev *ndev);
int32 fmac_dev_close(struct netdev *ndev);
int32 fmac_dev_set_work_mode(struct netdev *ndev, char *mode);
int32 fmac_dev_get_work_mode(struct netdev *ndev, char *buff);//buff size >= 12
int32 fmac_dev_set_ssid(struct netdev *ndev, char *ssid);
int32 fmac_dev_get_ssid(struct netdev *ndev, char *buff);//buff size >= 32
int32 fmac_dev_set_key_mgmt(struct netdev *ndev, char *key_mgmt);
int32 fmac_dev_get_key_mgmt(struct netdev *ndev, char *buff);//buff size >= 12
int32 fmac_dev_set_wpa_psk(struct netdev *ndev, char *psk);
int32 fmac_dev_get_wpa_psk(struct netdev *ndev, char *buff);//buff size >= 64
int32 fmac_dev_set_bss_bw(struct netdev *ndev, uint8 bss_bw);
int32 fmac_dev_set_chan_list(struct netdev *ndev, uint16 *chan_list, uint32 count);
int32 fmac_dev_set_paired_stas(struct netdev *ndev, uint8 *mac_list, uint32 count);
int32 fmac_dev_set_customer_dvrdata(struct netdev *ndev, uint16 cmd_id, uint8 *data, uint32 len); //可以自定义CMD ID
int32 fmac_dev_set_cust_driver_data(struct netdev *ndev, uint8 *data, uint32 len); //不能自定义CMD ID

#ifdef __cplusplus
}
#endif

#endif

