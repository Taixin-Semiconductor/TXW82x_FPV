#include "basic_include.h"
#include "hal/vcodec.h"

void *vcodec_open(struct vcodec_device *dec)
{
    if (dec) {
        if(dev_suspended((struct dev_obj *)dec, 1)) return NULL;
        return ((const struct vcodec_hal_ops *)dec->dev.ops)->open(dec);
    }
    return NULL;
}

int32 vcodec_close(struct vcodec_device *dec, void *chan)
{
    if (dec && chan) {
        HALDEV_SUSPENDED(dec);
        return ((const struct vcodec_hal_ops *)dec->dev.ops)->close(dec, chan);
    }
    return RET_ERR;
}

int32 vcodec_decode(struct vcodec_device *dec, struct vcodec_decode_req *req)
{
    if (dec) {
        HALDEV_SUSPENDED(dec);
        return ((const struct vcodec_hal_ops *)dec->dev.ops)->decode(dec, req);
    }
    return RET_ERR;
}

int32 vcodec_encode(struct vcodec_device *dec, struct vcodec_encode_req *req)
{
    if (dec) {
        HALDEV_SUSPENDED(dec);
        return ((const struct vcodec_hal_ops *)dec->dev.ops)->encode(dec, req);
    }
    return RET_ERR;
}

int32 vcodec_ioctl(struct vcodec_device *dec, void *chan, enum vcodec_ioctl_cmd cmd, uint32 param)
{
    if (dec && chan) {
        HALDEV_SUSPENDED(dec);
        return ((const struct vcodec_hal_ops *)dec->dev.ops)->ioctl(dec, chan, cmd, param);
    }
    return RET_ERR;
}

