#include "basic_include.h"
#include "hal/auproc.h"

int32 auproc_open(struct auproc_device *auproc, uint32 sample_rate, uint32 channels)
{
    if (auproc) {
		HALDEV_SUSPENDED(auproc);
        return ((const struct auproc_hal_ops *)auproc->dev.ops)->open(auproc, sample_rate, channels);
    }
    return RET_ERR;
}

int32 auproc_close(struct auproc_device *auproc)
{
    if (auproc) {
        HALDEV_SUSPENDED(auproc);
        return ((const struct auproc_hal_ops *)auproc->dev.ops)->close(auproc);
    }
    return RET_ERR;
}

int32 auproc_process(struct auproc_device *auproc, struct auproc_req *req)
{
    if (auproc) {
        HALDEV_SUSPENDED(auproc);
        return ((const struct auproc_hal_ops *)auproc->dev.ops)->process(auproc, req);
    }
    return RET_ERR;
}

int32 auproc_ioctl(struct auproc_device *auproc, enum auproc_ioctl_cmd cmd, uint32 param1, uint32 param2)
{
    if (auproc) {
        HALDEV_SUSPENDED(auproc);
        return ((const struct auproc_hal_ops *)auproc->dev.ops)->ioctl(auproc, cmd, param1, param2);
    }
    return RET_ERR;
}