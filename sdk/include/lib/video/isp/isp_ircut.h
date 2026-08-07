#ifndef _ISP_IRCUT_H_
#define _ISP_IRCUT_H_

#include "hal/isp.h"
#include "osal/work.h"

typedef struct ircut_info{
    uint8 ircut_opt_status;
    uint8 whiteled_status;      // whiteled status flag( on : open whiteled)
    uint8 irled_status;         // irled status flag( on : open irled)
    uint8 ircut_status;         // ircut status flag( on : open ircut)
    uint8 irdet_status;         // ircut status flag( on : open ircut)
    uint8 irled_detect_mode;    // irled detece select
    uint8 isp_mode;
    uint8 frame_cnt;
    uint8 frame_to_switch;
    uint8 ircut_en;         // ircut_hardware config gpio
    uint8 action_status;
    uint8 action_type;
    uint8 ircut_gpio_en, irled_gpio_en, irdet_gpio_en, whiteled_gpio_en;
    uint8 switch_cnt, switch_max;
    float to_day_bv, to_night_bv, curr_bv;
    struct os_work ircut_action_work;
    struct isp_device *dev;
}IRCUT_INFO;

enum {
    IRLED_OFF = 0,
    IRLED_ON,
};

enum {
    IRDET_OFF = 0,
    IRDET_ON,
};

enum {
    WHITELED_OFF = 0,
    WHITELED_ON,
};

enum {
    IRCUT_OPT_STATE_IDLE,
    IRCUT_OPT_STATE_SW_ON,
    IRCUT_OPT_STATE_SW_OFF,
    IRCUT_OPT_STATE_ON,
    IRCUT_OPT_STATE_OFF,
};

enum {
    IRCUT_OFF,
    IRCUT_ON,
};

enum {
    IRCUT_ACTION_STOP,
    IRCUT_ACTION_START,
};

enum {
    IRCUT_DET_MODE_SW,
    IRCUT_DET_MODE_HW,
    IRCUT_DET_MODE_MANUAL,
};

enum {
    ISP_MODE_DAY_COLOR   = 0,
    ISP_MODE_NIGHT_COLOR = 1,
    ISP_MODE_NIGHT_MONO  = 2,
};

void ircut_init();
#endif
