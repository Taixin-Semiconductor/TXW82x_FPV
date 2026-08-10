#include "basic_include.h"
#include "hal/auchange.h"

void *auchange_open(struct auchange_device *auchange, uint32 sample_rate, uint32 channels, uint32 max_nsamples)
{
    if (auchange) {
		if(dev_suspended((struct dev_obj *)auchange, 1)) return NULL;
        return ((const struct auchange_hal_ops *)auchange->dev.ops)->open(auchange, sample_rate, channels, max_nsamples);
    }
    return NULL;
}

int32 auchange_close(struct auchange_device *auchange, void *chan)
{
    if (auchange && chan) {
        HALDEV_SUSPENDED(auchange);
        return ((const struct auchange_hal_ops *)auchange->dev.ops)->close(auchange, chan);
    }
    return RET_ERR;
}

int32 auchange_write(struct auchange_device *auchange, struct auchange_req *req)
{
    if (auchange && req->chan) {
        HALDEV_SUSPENDED(auchange);
        return ((const struct auchange_hal_ops *)auchange->dev.ops)->write(auchange, req);
    }
    return RET_ERR;
}

int32 auchange_read(struct auchange_device *auchange, struct auchange_req *req)
{
    if (auchange && req->chan) {
        HALDEV_SUSPENDED(auchange);
        return ((const struct auchange_hal_ops *)auchange->dev.ops)->read(auchange, req);
    }
    return RET_ERR;
}

int32 auchange_ioctl(struct auchange_device *auchange, void *chan, enum auchange_ioctl_cmd cmd, uint32 param)
{
    if (auchange && chan) {
        HALDEV_SUSPENDED(auchange);
        return ((const struct auchange_hal_ops *)auchange->dev.ops)->ioctl(auchange, chan, cmd, param);
    }
    return RET_ERR;
}