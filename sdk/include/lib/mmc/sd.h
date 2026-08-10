/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2011-07-25     weety         first version
 */

#ifndef __SD_H__
#define __SD_H__


#include "lib/mmc/mmcsd_host.h"

#ifdef __cplusplus
extern "C" {
#endif

int32 mmcsd_send_if_cond(struct mmcsd_host *host, uint32 ocr);
int32 mmcsd_send_app_op_cond(struct mmcsd_host *host, uint32 ocr, uint32 *rocr);

int32 mmcsd_get_card_addr(struct mmcsd_host *host, uint32 *rca);
int32 mmcsd_get_scr(struct mmcsd_card *card, uint32 *scr);

int32 sd_card_detect(struct mmcsd_host *host, uint32 ocr);

#ifdef __cplusplus
}
#endif

#endif
