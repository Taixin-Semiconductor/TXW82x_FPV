#ifndef __TOUCH_PAD_H_
#define __TOUCH_PAD_H_
#include "basic_include.h"
#ifdef PIN_FROM_PARAM
#include "pin_param.h"
#endif

/* ------------ Selection of touch chips ------------ */

#define CST226SE_TOUCH_PAD  1

/* ------------------------------------------------- */

#define MAX_POINT_NUM   5

#undef PIN_TP_I2C_SCL
#undef PIN_TP_I2C_SDA
#undef PIN_TP_INT
#undef PIN_TP_RST

struct touch_multipoint_pos {
    uint8_t point_num;
    uint16_t pos_x[MAX_POINT_NUM];
    uint16_t pos_y[MAX_POINT_NUM];
};
typedef struct touch_multipoint_pos touch_multipoint_pos_t;

touch_multipoint_pos_t *touch_pad_get_multipoint_xy();
uint32_t touch_pad_free_multipoint_xy(touch_multipoint_pos_t *data);
void touch_pad_hareware_init();

#endif