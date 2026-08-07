#ifndef _SMAC_DEV_H_
#define _SMAC_DEV_H_

#ifdef __cplusplus
extern "C" {
#endif

struct lmac_ops *hgic_smac_detect(uint8 bus_type, struct macbus_param *param, const char *fw_data, uint8 reset_io);

#ifdef __cplusplus
}
#endif

#endif

