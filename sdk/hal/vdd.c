#include "basic_include.h"
#include "hal/vdd.h"

void *vdd_open(struct vdd_device *dev)
{
    if (dev) {
        if(dev_suspended((struct dev_obj *)dev, 1)) return NULL;
        return ((const struct vdd_hal_ops *)dev->dev.ops)->open(dev);
    }
    return NULL;
}

int32_t vdd_close(struct vdd_device *dev, void *vdd)
{
    if (dev && vdd) {
        HALDEV_SUSPENDED(dev);
        return ((const struct vdd_hal_ops *)dev->dev.ops)->close(dev, vdd);
    }
    return RET_ERR;
}

int32_t vdd_display(struct vdd_device *dev, void *vdd, struct framebuff *fb)
{
    if (dev && vdd) {
        HALDEV_SUSPENDED(dev);
        return ((const struct vdd_hal_ops *)dev->dev.ops)->display(dev, vdd, fb);
    }
    return RET_ERR;
}

int32_t vdd_ioctl(struct vdd_device *dev, void *vdd, enum vdd_ioctl_cmd cmd, uint32_t param)
{
    if (dev && vdd) {
        HALDEV_SUSPENDED(dev);
        return ((const struct vdd_hal_ops *)dev->dev.ops)->ioctl(dev, vdd, cmd, param);
    }
    return RET_ERR;
}
