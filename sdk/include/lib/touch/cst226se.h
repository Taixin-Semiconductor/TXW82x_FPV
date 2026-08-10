#ifndef __CST226SE_H_
#define __CST226SE_H_

#include "basic_include.h"
#include "hal/i2c.h"
#include "app_iic/app_iic.h"
#include "lib/touch/hyn_core.h"
#include "lib/touch/touch_pad.h"

int32_t cst226se_init(void **user_data);
int32_t cst226se_deinit(void *user_data);
void *cst226se_get_multipoint_xy(void *user_data);
uint32_t cst226se_free_multipoint_xy(void *user_data, void *data);

#endif