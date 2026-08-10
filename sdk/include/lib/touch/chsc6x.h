#ifndef __CHSC6X_H_
#define __CHSC6X_H_

#include "basic_include.h"
#include "hal/i2c.h"
#include "app_iic/app_iic.h"
#include "lib/touch/hyn_core.h"
#include "lib/touch/touch_pad.h"

int32_t chsc6x_init(void **user_data);
int32_t chsc6x_deinit(void *user_data);
void *chsc6x_get_multipoint_xy(void *user_data);
uint32_t chsc6x_free_multipoint_xy(void *user_data, void *data);
int chsc6x_suspend(void);
int chsc6x_resume(void);

#endif
