#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "hal/timer_device.h"

int32 timer_device_open(struct timer_device *timer, enum timer_type type, uint32 flags)
{
    if (timer && ((const struct timer_hal_ops *)timer->dev.ops)->open) {
        HALDEV_SUSPENDED(timer);
        return ((const struct timer_hal_ops *)timer->dev.ops)->open(timer, type, flags);
    }
    return RET_ERR;
}

int32 timer_device_close(struct timer_device *timer)
{
    if (timer && ((const struct timer_hal_ops *)timer->dev.ops)->close) {
        HALDEV_SUSPENDED(timer);
        return ((const struct timer_hal_ops *)timer->dev.ops)->close(timer);
    }
    return RET_ERR;
}

int32 timer_device_start(struct timer_device *timer, uint32 period_sysclkpd_cnt, timer_irq_hdl cb, uint32 cb_data)
{
    if (timer && ((const struct timer_hal_ops *)timer->dev.ops)->start) {
        HALDEV_SUSPENDED(timer);
        return ((const struct timer_hal_ops *)timer->dev.ops)->start(timer, period_sysclkpd_cnt, cb, cb_data);
    }
    return RET_ERR;
}

int32 timer_device_stop(struct timer_device *timer)
{
    if (timer && ((const struct timer_hal_ops *)timer->dev.ops)->stop) {
        HALDEV_SUSPENDED(timer);
        return ((const struct timer_hal_ops *)timer->dev.ops)->stop(timer);
    }
    return RET_ERR;
}

int32 timer_device_suspend(struct timer_device *timer)
{
    if (timer && ((const struct timer_hal_ops *)timer->dev.ops)->suspend) {
        HALDEV_SUSPENDED(timer);
        return ((const struct timer_hal_ops *)timer->dev.ops)->suspend(timer);
    }
    return RET_ERR;
}

int32 timer_device_resume(struct timer_device *timer)
{
    if (timer && ((const struct timer_hal_ops *)timer->dev.ops)->resume) {
        HALDEV_SUSPENDED(timer);
        return ((const struct timer_hal_ops *)timer->dev.ops)->resume(timer);
    }
    return RET_ERR;
}

int32 timer_device_ioctl(struct timer_device *timer, uint32 cmd, uint32 param1, uint32 param2)
{
    if (timer && ((const struct timer_hal_ops *)timer->dev.ops)->ioctl) {
        HALDEV_SUSPENDED(timer);
        return ((const struct timer_hal_ops *)timer->dev.ops)->ioctl(timer, cmd, param1, param2);
    }
    return RET_ERR;
}

int32 timer_request_irq(struct timer_device *timer, uint32 irq_flag, timer_irq_hdl cb, uint32 cb_data)
{
    if (timer && ((const struct timer_hal_ops *)timer->dev.ops)->request_irq) {
        HALDEV_SUSPENDED(timer);
        return ((const struct timer_hal_ops *)timer->dev.ops)->request_irq(timer, irq_flag, cb, cb_data);
    }
    return RET_ERR;
}

int32 timer_release_irq(struct timer_device *timer, uint32 irq_flag)
{
    if (timer && ((const struct timer_hal_ops *)timer->dev.ops)->release_irq) {
        HALDEV_SUSPENDED(timer);
        return ((const struct timer_hal_ops *)timer->dev.ops)->release_irq(timer, irq_flag);
    }
    return RET_ERR;
}

