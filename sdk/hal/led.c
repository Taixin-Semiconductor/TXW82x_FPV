#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "hal/led.h"


int32 led_open(struct led_device *led, enum led_work_mode work_mode, enum led_seg_cnt seg_cnt, enum led_com_cnt com_cnt)
{
    if (led && ((const struct led_hal_ops *)led->dev.ops)->open) {
        return ((const struct led_hal_ops *)led->dev.ops)->open(led, work_mode, seg_cnt, com_cnt);
    }
    return RET_ERR;
}

int32 led_on(struct led_device *led, uint32 num, enum led_com_cnt pos)
{
    if (led && ((const struct led_hal_ops *)led->dev.ops)->on) {
        return ((const struct led_hal_ops *)led->dev.ops)->on(led, num, pos);
    }
    return RET_ERR;
}

int32 led_off(struct led_device *led, enum led_com_cnt pos)
{
    if (led && ((const struct led_hal_ops *)led->dev.ops)->off) {
        return ((const struct led_hal_ops *)led->dev.ops)->off(led, pos);
    }
    return RET_ERR;
}

int32 led_close(struct led_device *led)
{
    if (led && ((const struct led_hal_ops *)led->dev.ops)->close) {
        return ((const struct led_hal_ops *)led->dev.ops)->close(led);
    }
    return RET_ERR;
}

int32 led_ioctl(struct led_device *led, enum led_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2)
{
    if (led && ((const struct led_hal_ops *)led->dev.ops)->ioctl) {
        return ((const struct led_hal_ops *)led->dev.ops)->ioctl(led, ioctl_cmd, param1, param2);
    }
    return RET_ERR;
}

int32 led_request_irq(struct led_device *led, enum led_irq_flag irq_flag, led_irq_hdl irq_hdl, uint32 irq_data)
{
    if (led && ((const struct led_hal_ops *)led->dev.ops)->request_irq) {
        return ((const struct led_hal_ops *)led->dev.ops)->request_irq(led, irq_flag, irq_hdl, irq_data);
    }
    return RET_ERR;
}

int32 led_release_irq(struct led_device *led, enum led_irq_flag irq_flag)
{
    if (led && ((const struct led_hal_ops *)led->dev.ops)->release_irq) {
        return ((const struct led_hal_ops *)led->dev.ops)->release_irq(led, irq_flag);
    }
    return RET_ERR;
}

