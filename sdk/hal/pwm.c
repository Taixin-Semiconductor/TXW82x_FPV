#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "hal/pwm.h"


int32 pwm_init(struct pwm_device *pwm, enum pwm_channel channel, uint32 period_sysclkpd_cnt, uint32 h_duty_sysclkpd_cnt)
{
    if (pwm && ((const struct pwm_hal_ops *)pwm->dev.ops)->init) {
        return ((const struct pwm_hal_ops *)pwm->dev.ops)->init(pwm, channel, period_sysclkpd_cnt, h_duty_sysclkpd_cnt);
    }
    return RET_ERR;
}

int32 pwm_deinit(struct pwm_device *pwm, enum pwm_channel channel)
{
    if (pwm && ((const struct pwm_hal_ops *)pwm->dev.ops)->deinit) {
        return ((const struct pwm_hal_ops *)pwm->dev.ops)->deinit(pwm, channel);
    }
    return RET_ERR;
}

int32 pwm_start(struct pwm_device *pwm, enum pwm_channel channel)
{
    if (pwm && ((const struct pwm_hal_ops *)pwm->dev.ops)->start) {
        return ((const struct pwm_hal_ops *)pwm->dev.ops)->start(pwm, channel);
    }
    return RET_ERR;
}

int32 pwm_stop(struct pwm_device *pwm, enum pwm_channel channel)
{
    if (pwm && ((const struct pwm_hal_ops *)pwm->dev.ops)->stop) {
        return ((const struct pwm_hal_ops *)pwm->dev.ops)->stop(pwm, channel);
    }
    return RET_ERR;
}

int32 pwm_suspend(struct pwm_device *pwm, enum pwm_channel channel)
{
    if (pwm && ((const struct pwm_hal_ops *)pwm->dev.ops)->suspend) {
        return ((const struct pwm_hal_ops *)pwm->dev.ops)->suspend(pwm, channel);
    }
    return RET_ERR;
}

int32 pwm_resume(struct pwm_device *pwm, enum pwm_channel channel)
{
    if (pwm && ((const struct pwm_hal_ops *)pwm->dev.ops)->resume) {
        return ((const struct pwm_hal_ops *)pwm->dev.ops)->resume(pwm, channel);
    }
    return RET_ERR;
}

int32 pwm_ioctl(struct pwm_device *pwm, enum pwm_channel channel, enum pwm_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2)
{
    if (pwm && ((const struct pwm_hal_ops *)pwm->dev.ops)->ioctl) {
        return ((const struct pwm_hal_ops *)pwm->dev.ops)->ioctl(pwm, channel, ioctl_cmd, param1, param2);
    }
    return RET_ERR;
}

int32 pwm_request_irq(struct pwm_device *pwm, enum pwm_channel channel, enum pwm_irq_flag irq_flag, pwm_irq_hdl irq_hdl, uint32 data)
{
    if (pwm && ((const struct pwm_hal_ops *)pwm->dev.ops)->request_irq) {
        return ((const struct pwm_hal_ops *)pwm->dev.ops)->request_irq(pwm, channel, irq_flag, irq_hdl, data);
    }
    return RET_ERR;
}

int32 pwm_release_irq(struct pwm_device *pwm, enum pwm_channel channel)
{
    if (pwm && ((const struct pwm_hal_ops *)pwm->dev.ops)->release_irq) {
        return ((const struct pwm_hal_ops *)pwm->dev.ops)->release_irq(pwm, channel);
    }
    return RET_ERR;
}

