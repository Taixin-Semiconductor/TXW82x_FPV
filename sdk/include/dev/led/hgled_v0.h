#ifndef _HGLED_V0_H
#define _HGLED_V0_H
#include "hal/led.h"

#ifdef __cplusplus
extern "C" {
#endif

struct hgled_v0 {
    struct led_device   dev;
    uint32              hw;
    led_irq_hdl         irq_hdl;
    uint32              irq_data;
    uint32              irq_num;
    uint16              seg_cnt;
    uint16              com_cnt;
    uint32              tube_type;
    uint32              opened;
};


int32 hgled_v0_attach(uint32 dev_id, struct hgled_v0 *led);


#ifdef __cplusplus
}
#endif
#endif
