#ifndef _HUGEIC_GPIO_V5_H_
#define _HUGEIC_GPIO_V5_H_
#include "hal/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HGGPIO_V5_MAX_PINS            (16)

struct hggpio_v5 {
    struct gpio_device dev;
    uint32 hw;
    int32 pin_id[HGGPIO_V5_MAX_PINS];
    gpio_irq_hdl irq_hdl[HGGPIO_V5_MAX_PINS];
    uint32 pin_num[2];
    uint8 irq_num;
};


int32 hggpio_v5_attach(uint32 dev_id, struct hggpio_v5 *gpio);

#ifdef __cplusplus
}
#endif


#endif

