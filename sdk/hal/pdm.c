#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "hal/pdm.h"

int32 pdm_open(struct pdm_device *pdm, enum pdm_sample_freq freq, enum pdm_channel channel)
{
    if (pdm && ((const struct pdm_hal_ops *)pdm->dev.ops)->open) {
        return ((const struct pdm_hal_ops *)pdm->dev.ops)->open(pdm, freq, channel);
    }
    return RET_ERR;
}

int32 pdm_read(struct pdm_device *pdm, void *buf, uint32 len)
{
    if (pdm && ((const struct pdm_hal_ops *)pdm->dev.ops)->read) {
        return ((const struct pdm_hal_ops *)pdm->dev.ops)->read(pdm, buf, len);
    }
    return RET_ERR;
}

int32 pdm_close(struct pdm_device *pdm)
{
    if (pdm && ((const struct pdm_hal_ops *)pdm->dev.ops)->close) {
        return ((const struct pdm_hal_ops *)pdm->dev.ops)->close(pdm);
    }
    return RET_ERR;
}

int32 pdm_request_irq(struct pdm_device *pdm, enum pdm_irq_flag flag, pdm_irq_hdl irq_hdl, uint32 data)
{
    if (pdm && ((const struct pdm_hal_ops *)pdm->dev.ops)->request_irq) {
        return ((const struct pdm_hal_ops *)pdm->dev.ops)->request_irq(pdm, flag, irq_hdl, data);
    }
    return RET_ERR;
}

int32 pdm_release_irq(struct pdm_device *pdm, enum pdm_irq_flag irq_flag)
{
    if (pdm && ((const struct pdm_hal_ops *)pdm->dev.ops)->release_irq) {
        return ((const struct pdm_hal_ops *)pdm->dev.ops)->release_irq(pdm, irq_flag);
    }
    return RET_ERR;
}

int32 pdm_ioctl(struct pdm_device *pdm, enum pdm_ioctl_cmd cmd, uint32 param)
{
    if (pdm && ((const struct pdm_hal_ops *)pdm->dev.ops)->ioctl) {
        return ((const struct pdm_hal_ops *)pdm->dev.ops)->ioctl(pdm, cmd, param);
    }
    return RET_ERR;
}

