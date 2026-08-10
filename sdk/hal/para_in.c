#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/para_in.h"

int32 para_in_open(struct para_in_device *p_dev)
{
    if (p_dev && ((const struct para_in_hal_ops *)p_dev->dev.ops)->open) {
        HALDEV_SUSPENDED(p_dev);
        return ((const struct para_in_hal_ops *)p_dev->dev.ops)->open(p_dev);
    }
    return RET_ERR;
}

int32 para_in_close(struct para_in_device *p_dev)
{
    if (p_dev && ((const struct para_in_hal_ops *)p_dev->dev.ops)->close) {
        HALDEV_SUSPENDED(p_dev);
        return ((const struct para_in_hal_ops *)p_dev->dev.ops)->close(p_dev);
    }
    return RET_ERR;
}

int32 para_in_ioctl(struct para_in_device *p_dev, uint32 cmd, uint32 param1, uint32 param2)
{
    if (p_dev && ((const struct para_in_hal_ops *)p_dev->dev.ops)->ioctl) {
        HALDEV_SUSPENDED(p_dev);
        return ((const struct para_in_hal_ops *)p_dev->dev.ops)->ioctl(p_dev, cmd, param1, param2);
    }
    return RET_ERR;
}

int32 para_in_request_irq(struct para_in_device *p_dev, uint32 irq_flags, para_in_irq_hdl handle, uint32 data)
{
    if (p_dev && ((const struct para_in_hal_ops *)p_dev->dev.ops)->request_irq) {
        HALDEV_SUSPENDED(p_dev);
        return ((const struct para_in_hal_ops *)p_dev->dev.ops)->request_irq(p_dev, irq_flags, handle, data);
    }
    return RET_ERR;
}

int32 para_in_release_irq(struct para_in_device *p_dev, uint32 irq_flag)
{
    if (p_dev && ((const struct para_in_hal_ops *)p_dev->dev.ops)->release_irq) {
        HALDEV_SUSPENDED(p_dev);
        return ((const struct para_in_hal_ops *)p_dev->dev.ops)->release_irq(p_dev, irq_flag);
    }
    return RET_ERR;
}