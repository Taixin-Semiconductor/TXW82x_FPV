#include "basic_include.h"
#include "hal/acodec.h"

void *acodec_open(struct acodec_device *dec, txAudioInfo_t *info)
{
    if (dec) {
		if(dev_suspended((struct dev_obj *)dec, 1)) return NULL;
        return ((const struct acodec_hal_ops *)dec->dev.ops)->open(dec, info);
    }
    return NULL;
}

int32 acodec_close(struct acodec_device *dec, void *chan)
{
    if (dec && chan) {
        HALDEV_SUSPENDED(dec);
        return ((const struct acodec_hal_ops *)dec->dev.ops)->close(dec, chan);
    }
    return RET_ERR;
}

int32 acodec_decode(struct acodec_device *dec, struct acodec_decode_req *req)
{
    if (dec && req->chan) {
        HALDEV_SUSPENDED(dec);
        return ((const struct acodec_hal_ops *)dec->dev.ops)->decode(dec, req);
    }
    return RET_ERR;
}

int32 acodec_encode(struct acodec_device *dec, struct acodec_encode_req *req)
{
    if (dec && req->chan) {
        HALDEV_SUSPENDED(dec);
        return ((const struct acodec_hal_ops *)dec->dev.ops)->encode(dec, req);
    }
    return RET_ERR;
}

int32 acodec_ioctl(struct acodec_device *dec, void *chan, enum acodec_ioctl_cmd cmd, uint32 param)
{
    if (dec && chan) {
        HALDEV_SUSPENDED(dec);
        return ((const struct acodec_hal_ops *)dec->dev.ops)->ioctl(dec, chan, cmd, param);
    }
    return RET_ERR;
}

