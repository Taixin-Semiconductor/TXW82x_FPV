#ifndef __TOUCH_PAD_H_
#define __TOUCH_PAD_H_
#include "basic_include.h"
#ifdef PIN_FROM_PARAM
#include "pin_param.h"
#endif

/* ------------ Selection of touch chips ------------ */

struct touch_chip_hardware_ops {
    uint32_t (*free_multipoint_xy)(void *user_data, void *data);
    void*    (*get_multipoint_xy)(void *user_data);
    int32_t  (*touch_chip_init)(void **user_data);
    int32_t  (*touch_chip_deinit)(void *user_data);
};

/* 新增触控芯片只能从末尾添加 */
typedef enum
{
    cst226se_touch_chip = 0,
    chsc6x_touch_chip,
    touch_chip_count,
} touch_chip_set;

extern const struct touch_chip_hardware_ops cst226se_ops;
extern const struct touch_chip_hardware_ops chsc6x_ops;

/* ------------------------------------------------- */

#define MAX_POINT_NUM   5

#undef PIN_TP_I2C_SCL
#undef PIN_TP_I2C_SDA
#undef PIN_TP_INT
#undef PIN_TP_RST

struct touch_pad_dev {
    struct dev_obj dev;
    const struct touch_chip_hardware_ops *hw_ops;
    void *user_data;
};

struct touch_multipoint_pos {
    uint8_t point_num;
    uint16_t pos_x[MAX_POINT_NUM];
    uint16_t pos_y[MAX_POINT_NUM];
};
typedef struct touch_multipoint_pos touch_multipoint_pos_t;

touch_multipoint_pos_t *touch_pad_get_multipoint_xy(void *dev);
uint32_t touch_pad_free_multipoint_xy(void *dev, touch_multipoint_pos_t *data);
int32_t touch_pad_hardware_init(uint16_t dev_id, touch_chip_set touch_chip_select);
int32_t touch_pad_hardware_deinit(void *dev);

#endif