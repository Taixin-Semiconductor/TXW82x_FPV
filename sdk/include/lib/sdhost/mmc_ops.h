#ifndef __MMC_OPS_H__
#define __MMC_OPS_H__
#include "sys_config.h"
#include "typesdef.h"
#include "devid.h"
#include "list.h"
#include "dev.h"

int mmc_get_ext_csd(struct sdh_device *host, uint8_t *ext_csd);
int mmc_parse_ext_csd(struct sdh_device *host, uint8_t *ext_csd);
int mmc_switch(struct sdh_device *host, uint8_t set, uint8_t index, uint8_t value);
int mmc_compare_ext_csds(struct sdh_device *host, uint8_t *ext_csd, uint32_t bus_width);
int mmc_select_bus_width(struct sdh_device *host, uint8_t *ext_csd);
int mmc_set_card_addr(struct sdh_device *host, uint32_t rca);
int __send_status(struct sdh_device *host, uint32_t *status, unsigned retries);
int card_busy_detect(struct sdh_device *host, unsigned int timeout_ms, uint32_t *resp_errs);
uint32_t send_op_cond(struct sdh_device *host, uint32_t ocr, uint32_t *rocr);
uint32 send_card_status(struct sdh_device * host);

int mmc_cmdq_switch(struct sdh_device *host, bool enable);

#endif