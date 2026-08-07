#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "hal/capture.h"


int32 capture_init(struct capture_device *capture, enum capture_channel channel, enum capture_mode mode)
{
    if (capture) {
        return ((const struct capture_hal_ops *)capture->dev.ops)->init(capture, channel, mode);
    }
    return RET_ERR;
}

int32 capture_deinit(struct capture_device *capture, enum capture_channel channel)
{
    if (capture) {
        return ((const struct capture_hal_ops *)capture->dev.ops)->deinit(capture, channel);
    }
    return RET_ERR;
}

int32 capture_start(struct capture_device *capture, enum capture_channel channel)
{
    if (capture) {
        return ((const struct capture_hal_ops *)capture->dev.ops)->start(capture, channel);
    }
    return RET_ERR;
}

int32 capture_stop(struct capture_device *capture, enum capture_channel channel)
{
    if (capture) {
        return ((const struct capture_hal_ops *)capture->dev.ops)->stop(capture, channel);
    }
    return RET_ERR;
}

int32 capture_suspend(struct capture_device *capture, enum capture_channel channel)
{
    if (capture && ((const struct capture_hal_ops *)capture->dev.ops)->suspend) {
        return ((const struct capture_hal_ops *)capture->dev.ops)->suspend(capture, channel);
    }
    return RET_ERR;
}

int32 capture_resume(struct capture_device *capture, enum capture_channel channel)
{
    if (capture && ((const struct capture_hal_ops *)capture->dev.ops)->resume) {
        return ((const struct capture_hal_ops *)capture->dev.ops)->resume(capture, channel);
    }
    return RET_ERR;
}

int32 capture_ioctl(struct capture_device *capture, enum capture_channel channel, enum capture_ioctl_cmd cmd, uint32 param1, uint32 param2)
{
    if (capture && ((const struct capture_hal_ops *)capture->dev.ops)->ioctl) {
        return ((const struct capture_hal_ops *)capture->dev.ops)->ioctl(capture, channel, cmd, param1, param2);
    }
    return RET_ERR;
}

int32 capture_request_irq(struct capture_device *capture, enum capture_channel channel, enum capture_irq_flag irq_flag, capture_irq_hdl irq_hdl, uint32 data)
{
    if (capture && ((const struct capture_hal_ops *)capture->dev.ops)->request_irq) {
        return ((const struct capture_hal_ops *)capture->dev.ops)->request_irq(capture, channel, irq_flag, irq_hdl, data);
    }
    return RET_ERR;
}

int32 capture_release_irq(struct capture_device *capture, enum capture_channel channel)
{
    if (capture && ((const struct capture_hal_ops *)capture->dev.ops)->release_irq) {
        return ((const struct capture_hal_ops *)capture->dev.ops)->release_irq(capture, channel);
    }
    return RET_ERR;
}

