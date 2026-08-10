#include "basic_include.h"
#include "hal/aures.h"

void *aures_open(struct aures_device *aures, uint32 src_sample_rate, uint32 dest_sample_rate, uint32 channels)
{
    if (aures) {
		if(dev_suspended((struct dev_obj *)aures, 1)) return NULL;
        return ((const struct aures_hal_ops *)aures->dev.ops)->open(aures, src_sample_rate, dest_sample_rate, channels);
    }
    return NULL;
}

int32 aures_close(struct aures_device *aures, void *chan)
{
    if (aures && chan) {
        HALDEV_SUSPENDED(aures);
        return ((const struct aures_hal_ops *)aures->dev.ops)->close(aures, chan);
    }
    return RET_ERR;
}

int32 aures_resample(struct aures_device *aures, struct aures_req *req)
{
    if (aures && req->chan) {
        HALDEV_SUSPENDED(aures);
        return ((const struct aures_hal_ops *)aures->dev.ops)->resample(aures, req);
    }
    return RET_ERR;
}

int32 aures_ioctl(struct aures_device *aures, void *chan, enum aures_ioctl_cmd cmd, uint32 param)
{
    if (aures && chan) {
        HALDEV_SUSPENDED(aures);
        return ((const struct aures_hal_ops *)aures->dev.ops)->ioctl(aures, chan, cmd, param);
    }
    return RET_ERR;
}