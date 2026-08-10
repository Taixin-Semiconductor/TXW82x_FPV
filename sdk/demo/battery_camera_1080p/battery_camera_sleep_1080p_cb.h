#ifndef __BATTERY_CAMERA_SLEEP_1080P_CB_H__
#define __BATTERY_CAMERA_SLEEP_1080P_CB_H__
#ifdef CONFIG_SLEEP
#include "lib/lmac/lmac_dsleep.h"
#include "lib/common/dsleepdata.h"
#endif

int app_1080p_lowpower_register(void);

#endif