#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "hal/dma2d.h"
#include "string.h"

int32 dma2d_blkcpy(struct dma2d_device *dma2d, struct dma2d_blkcpy_param *p_blkcpy)
{
    if (dma2d && ((const struct dma2d_hal_ops *)dma2d->dev.ops)->blkcpy) {
        return ((const struct dma2d_hal_ops *)dma2d->dev.ops)->blkcpy(dma2d, p_blkcpy);
    }
    return RET_ERR;
}

int32 dma2d_memcpy(struct dma2d_device *dma2d, struct dma2d_memcpy_param *p_memcpy)
{
    if (dma2d && ((const struct dma2d_hal_ops *)dma2d->dev.ops)->memcpy) {
        return ((const struct dma2d_hal_ops *)dma2d->dev.ops)->memcpy(dma2d, p_memcpy);
    }
    return RET_ERR;
}

int32 dma2d_memset(struct dma2d_device *dma2d, struct dma2d_memset_param *p_memset)
{
    if (dma2d && ((const struct dma2d_hal_ops *)dma2d->dev.ops)->memset) {
        return ((const struct dma2d_hal_ops *)dma2d->dev.ops)->memset(dma2d, p_memset);
    }
    return RET_ERR;
}

int32 dma2d_convert(struct dma2d_device *dma2d, struct dma2d_convert_param *p_convert)
{
    if (dma2d && ((const struct dma2d_hal_ops *)dma2d->dev.ops)->convert) {
        return ((const struct dma2d_hal_ops *)dma2d->dev.ops)->convert(dma2d, p_convert);
    }
    return RET_ERR;   
}

int32 dma2d_mixture(struct dma2d_device *dma2d, struct dma2d_mixture_param *p_mixture)
{
    if (dma2d && ((const struct dma2d_hal_ops *)dma2d->dev.ops)->mixture) {
        return ((const struct dma2d_hal_ops *)dma2d->dev.ops)->mixture(dma2d, p_mixture);
    }
    return RET_ERR;   
}

int32 dma2d_check_status(struct dma2d_device *dma2d)
{
    if (dma2d && ((const struct dma2d_hal_ops *)dma2d->dev.ops)->ioctl) {
        return ((const struct dma2d_hal_ops *)dma2d->dev.ops)->ioctl(dma2d, DMA2D_IOCTL_CHECK_RESULT, 0, 0);
    }
    return RET_ERR;
}

int32 dma2d_abort_trans(struct dma2d_device *dma2d)
{
    if (dma2d && ((const struct dma2d_hal_ops *)dma2d->dev.ops)->ioctl) {
        return ((const struct dma2d_hal_ops *)dma2d->dev.ops)->ioctl(dma2d, DMA2D_IOCTL_ABORT, 0, 0);
    }
    return RET_ERR;
}

int32 dma2d_suspend_trans(struct dma2d_device *dma2d)
{
    if (dma2d && ((const struct dma2d_hal_ops *)dma2d->dev.ops)->ioctl) {
        return ((const struct dma2d_hal_ops *)dma2d->dev.ops)->ioctl(dma2d, DMA2D_IOCTL_SUSPEND, 0, 0);
    }
    return RET_ERR; 
}

int32 dma2d_suspend_status(struct dma2d_device *dma2d)
{
    if (dma2d && ((const struct dma2d_hal_ops *)dma2d->dev.ops)->ioctl) {
        return ((const struct dma2d_hal_ops *)dma2d->dev.ops)->ioctl(dma2d, DMA2D_IOCTL_SUSPEND_CHECK, 0, 0);
    }
    return RET_ERR; 
}

int32 dma2d_resume_trans(struct dma2d_device *dma2d)
{
    if (dma2d && ((const struct dma2d_hal_ops *)dma2d->dev.ops)->ioctl) {
        return ((const struct dma2d_hal_ops *)dma2d->dev.ops)->ioctl(dma2d, DMA2D_IOCTL_RESUME, 0, 0);
    }
    return RET_ERR; 
}

int32 dma2d_resume_status(struct dma2d_device *dma2d)
{
    if (dma2d && ((const struct dma2d_hal_ops *)dma2d->dev.ops)->ioctl) {
        return ((const struct dma2d_hal_ops *)dma2d->dev.ops)->ioctl(dma2d, DMA2D_IOCTL_RESUME_CHECK, 0, 0);
    }
    return RET_ERR; 
}

int32 dma2d_foreground_clut_init(struct dma2d_device *dma2d, struct dma2d_clut_param *p_clut)
{
    if (dma2d && ((const struct dma2d_hal_ops *)dma2d->dev.ops)->ioctl) {
        return ((const struct dma2d_hal_ops *)dma2d->dev.ops)->ioctl(dma2d, DMA2D_IOCTL_CONFIG_FG, (uint32)p_clut, 0);
    }
    return RET_ERR; 
}

int32 dma2d_background_clut_init(struct dma2d_device *dma2d, struct dma2d_clut_param *p_clut)
{
    if (dma2d && ((const struct dma2d_hal_ops *)dma2d->dev.ops)->ioctl) {
        return ((const struct dma2d_hal_ops *)dma2d->dev.ops)->ioctl(dma2d, DMA2D_IOCTL_CONFIG_BG, (uint32)p_clut, 0);
    }
    return RET_ERR; 
}