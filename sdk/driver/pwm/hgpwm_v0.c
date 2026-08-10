/**
 * @file hgpwm_v0.c
 * @author bxd
 * @brief pwm
 * @version
 * TXW80X; TXW81X
 * @date 2023-08-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "hal/timer_device.h"
#include "hal/pwm.h"
#include "dev/pwm/hgpwm_v0.h"

/**********************************************************************************/
/*                           LOW LAYER FUNCTION                                   */
/**********************************************************************************/

static int32 hgpwm_v0_switch_hal_pwm_channel(enum pwm_channel param) {

    switch (param) {
        case (PWM_CHANNEL_0):
            return 0;
            break;
        case (PWM_CHANNEL_1):
            return 1;
            break;
        case (PWM_CHANNEL_2):
            return 2;
            break;
        case (PWM_CHANNEL_3):
            return 3;
            break;
        case (PWM_CHANNEL_4):
            return 4;
            break;
        case (PWM_CHANNEL_5):
            return 5;
            break;
//        case (PWM_CHANNEL_6):
//            return 6;
//            break;
//        case (PWM_CHANNEL_7):
//            return 7;
//            break;
//        case (PWM_CHANNEL_8):
//            return 8;
//            break;
//        case (PWM_CHANNEL_9):
//            return 9;
//            break;
//        case (PWM_CHANNEL_10):
//            return 10;
//            break;
        default:
            return -1;
            break;
    }
}

static int32 hgpwm_v0_config(struct hgpwm_v0 *dev, enum pwm_channel channel, struct hgpwm_v0_config *config) {

    struct hgpwm_v0_config *p_config       = config;
    uint32                  channel_to_reg = 0;
    int32                   ret_val        = RET_OK;

    /* Make sure the config struct pointer */
    if (!p_config) {
        return -EINVAL;
    }

    /* Make sure the channel */
    channel_to_reg = hgpwm_v0_switch_hal_pwm_channel(channel);
    if ((-1) == channel_to_reg) {
        return -EINVAL;
    }

    /* Make sure the timer is attached */
    if ((dev->channel[channel_to_reg])) {
        ret_val = timer_device_ioctl((struct timer_device *)dev->channel[channel_to_reg], HGPWM_V0_FUNC_CMD_MASK, (uint32)p_config, 0);
    } else {
        ret_val = RET_ERR;
    }
    
    return ret_val;
}

/**********************************************************************************/
/*                             ATTCH FUNCTION                                     */
/**********************************************************************************/

static int32 hgpwm_v0_init(struct pwm_device *pwm, enum pwm_channel channel, uint32 period_sysclkpd_cnt, uint32 h_duty_sysclkpd_cnt) {

    struct hgpwm_v0        *dev         = (struct hgpwm_v0 *)pwm;
    struct hgpwm_v0_config  config      = {0};
    int32                   channel_num = 0;
    int32                   ret_val     = RET_OK;

    /* Make sure the channel */
    channel_num = hgpwm_v0_switch_hal_pwm_channel(channel);
    if ((-1) == channel_num) {
        return -EINVAL;
    }

    /* Make busy status */
    if (dev->opened[channel_num]) {
        return -EBUSY;
    }

    /* config pwm */
    config.period   = period_sysclkpd_cnt;
    config.duty     = h_duty_sysclkpd_cnt;
    config.func_cmd = HGPWM_V0_FUNC_CMD_INIT;

    ret_val = hgpwm_v0_config(dev, channel, &config);

    dev->opened[channel_num] = 1;

   return ret_val;
}

static int32 hgpwm_v0_deinit(struct pwm_device *pwm, enum pwm_channel channel) {

    struct hgpwm_v0        *dev         = (struct hgpwm_v0 *)pwm;
    struct hgpwm_v0_config  config      = {0};
    int32                   channel_num = 0;
    int32                   ret_val     = RET_OK;
    
    /* Make sure the channel */
    channel_num = hgpwm_v0_switch_hal_pwm_channel(channel);
    if ((-1) == channel_num) {
        return -EINVAL;
    }

    /* Make busy status */
    if (!(dev->opened[channel_num])) {
        return RET_OK;
    }

    config.func_cmd = HGPWM_V0_FUNC_CMD_DEINIT;

    ret_val = hgpwm_v0_config(dev, channel, &config);

    dev->opened[channel_num] = 0;

    return ret_val;
}

static int32 hgpwm_v0_start(struct pwm_device *pwm, enum pwm_channel channel) {

    struct hgpwm_v0        *dev         = (struct hgpwm_v0 *)pwm;
    struct hgpwm_v0_config  config      = {0};
    int32                   channel_num = 0;
    int32                   ret_val     = RET_OK;
    
    /* Make sure the channel */
    channel_num = hgpwm_v0_switch_hal_pwm_channel(channel);
    if ((-1) == channel_num) {
        return -EINVAL;
    }

    /* Make busy status */
    if (!(dev->opened[channel_num])) {
        return RET_ERR;
    }

    config.func_cmd = HGPWM_V0_FUNC_CMD_START;
    
    ret_val = hgpwm_v0_config(dev, channel, &config);

    return ret_val;
}

static int32 hgpwm_v0_stop(struct pwm_device *pwm, enum pwm_channel channel) {

    struct hgpwm_v0        *dev         = (struct hgpwm_v0 *)pwm;
    struct hgpwm_v0_config  config      = {0};
    int32                   channel_num = 0;
    int32                   ret_val     = RET_OK;
    
    /* Make sure the channel */
    channel_num = hgpwm_v0_switch_hal_pwm_channel(channel);
    if ((-1) == channel_num) {
        return -EINVAL;
    }

    /* Make busy status */
    if (!(dev->opened[channel_num])) {
        return RET_OK;
    }

    config.func_cmd = HGPWM_V0_FUNC_CMD_STOP;
    
    ret_val = hgpwm_v0_config(dev, channel, &config);

    return ret_val;
}

#ifdef CONFIG_SLEEP
static int32 hgpwm_v0_suspend(struct pwm_device *pwm, enum pwm_channel channel)
{
    struct hgpwm_v0        *dev         = (struct hgpwm_v0 *)pwm;
    struct hgpwm_v0_config  config      = {0};
    int32                   channel_num = 0;
    int32                   ret_val     = RET_OK;
    
    /* Make sure the channel */
    channel_num = hgpwm_v0_switch_hal_pwm_channel(channel);
    if ((-1) == channel_num) {
        return -EINVAL;
    }

    /* Make busy status */
    if (!(dev->opened[channel_num])) {
        return RET_OK;
    }

    config.func_cmd = HGPWM_V0_FUNC_CMD_SUSPEND;
    
    ret_val = hgpwm_v0_config(dev, channel, &config);

    return ret_val;
}

static int32 hgpwm_v0_resume(struct pwm_device *pwm, enum pwm_channel channel)
{
    struct hgpwm_v0        *dev         = (struct hgpwm_v0 *)pwm;
    struct hgpwm_v0_config  config      = {0};
    int32                   channel_num = 0;
    int32                   ret_val     = RET_OK;
    
    /* Make sure the channel */
    channel_num = hgpwm_v0_switch_hal_pwm_channel(channel);
    if ((-1) == channel_num) {
        return -EINVAL;
    }

    /* Make busy status */
    if (!(dev->opened[channel_num])) {
        return RET_OK;
    }

    config.func_cmd = HGPWM_V0_FUNC_CMD_RESUME;
    
    ret_val = hgpwm_v0_config(dev, channel, &config);

    return ret_val;
}
#endif

static int32 hgpwm_v0_ioctl(struct pwm_device *pwm, enum pwm_channel channel, enum pwm_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2) {

    struct hgpwm_v0        *dev     = (struct hgpwm_v0 *)pwm;
    struct hgpwm_v0_config  config  = {0};
    int32                   ret_val = RET_OK;

    switch (ioctl_cmd) {
        case (PWM_IOCTL_CMD_SET_PERIOD_DUTY):
            config.func_cmd = HGPWM_V0_FUNC_CMD_IOCTL_SET_PERIOD_DUTY;
            config.period   = param1;
            config.duty     = param2;
            ret_val = hgpwm_v0_config(dev, channel, &config);
            break;
        case (PWM_IOCTL_CMD_SET_SINGLE_INCREAM):
            config.func_cmd = HGPWM_V0_FUNC_CMD_IOCTL_SET_SINGLE_INCREAM;
            config.period   = param1;
            config.duty     = param2;
            ret_val = hgpwm_v0_config(dev, channel, &config);
            break;
        case (PWM_IOCTL_CMD_SET_INCREAM_DECREASE):
            config.func_cmd = HGPWM_V0_FUNC_CMD_IOCTL_SET_INCREAM_DECREASE;
            config.period   = param1;
            config.duty     = param2;
            ret_val = hgpwm_v0_config(dev, channel, &config);
            break;
		case (PWM_IOCTL_CMD_SET_PRESCALER):
			/*!
			 * @brief: set prescaler
			 * @param1: prescaler
			 * @param2: NULL
			 */
            config.func_cmd = HGPWM_V0_FUNC_CMD_IOCTL_SET_PRESCALER;
            config.param1   = param1;
            ret_val = hgpwm_v0_config(dev, channel, &config);			
			break;
		case (PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY):
			/*!
			 * @brief: set period and duty immediately
			 * @param1: period
			 * @param2: duty
			 */
            config.func_cmd = HGPWM_V0_FUNC_CMD_IOCTL_SET_PERIOD_DUTY_IMMEDIATELY;
            config.period   = param1;
			config.duty     = param2;
            ret_val = hgpwm_v0_config(dev, channel, &config);			
			break;

        default:
            ret_val = -ENOTSUPP;
            break;
    }

    return ret_val;
}

static int32 hgpwm_v0_request_irq(struct pwm_device *pwm, enum pwm_channel channel, enum pwm_irq_flag irq_flag, pwm_irq_hdl irq_hdl, uint32 data) {

    struct hgpwm_v0        *dev     = (struct hgpwm_v0 *)pwm;
    struct hgpwm_v0_config  config  = {0};

    switch (irq_flag) {
        case (PWM_IRQ_FLAG_COMPARE):
            config.func_cmd       = HGPWM_V0_FUNC_CMD_REQUEST_IRQ_COMPARE;
            config.irq_hdl        = irq_hdl;
            config.irq_data       = data;
            hgpwm_v0_config(dev, channel, &config);
            break;
        case (PWM_IRQ_FLAG_PERIOD):
            config.func_cmd       = HGPWM_V0_FUNC_CMD_REQUEST_IRQ_PERIOD;
            config.irq_hdl        = irq_hdl;
            config.irq_data       = data;
            hgpwm_v0_config(dev, channel, &config);
            break;
    }

    return RET_OK;
}

static int32 hgpwm_v0_release_irq(struct pwm_device *pwm, enum pwm_channel channel) {

    struct hgpwm_v0        *dev    = (struct hgpwm_v0 *)pwm;
    struct hgpwm_v0_config  config = {0};

    config.func_cmd       = HGPWM_V0_FUNC_CMD_RELEASE_IRQ;
    config.irq_data       = 0;
    config.irq_hdl        = NULL;

    hgpwm_v0_config(dev, channel, &config);
    
    return RET_OK;
}

static const struct pwm_hal_ops pwm_v0_ops = {
    .init        = hgpwm_v0_init,
    .deinit      = hgpwm_v0_deinit,
    .start       = hgpwm_v0_start,
    .stop        = hgpwm_v0_stop,
    .ioctl       = hgpwm_v0_ioctl,
    .request_irq = hgpwm_v0_request_irq,
    .release_irq = hgpwm_v0_release_irq,
#ifdef CONFIG_SLEEP
    .ops.suspend = NULL,//hgpwm_v0_suspend,
    .ops.resume  = NULL,//hgpwm_v0_resume,
#endif
};

int32 hgpwm_v0_attach(uint32 dev_id, struct hgpwm_v0 *pwm) {

    uint32 i = 0;
    for (i=0; i<HGPWM_MAX_PWM_CHANNEL; i++) {
        pwm->opened[i] = 0;
    }
    pwm->dev.dev.ops = (const struct devobj_ops *)&pwm_v0_ops;
    dev_register(dev_id, (struct dev_obj *)pwm);    
    return RET_OK;
}
