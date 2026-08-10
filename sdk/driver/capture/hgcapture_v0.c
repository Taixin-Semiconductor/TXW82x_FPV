/**
 * @file hgcapture_v0.c
 * @author bxd
 * @brief capture
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
#include "hal/capture.h"
#include "dev/capture/hgcapture_v0.h"

/**********************************************************************************/
/*                           LOW LAYER FUNCTION                                   */
/**********************************************************************************/

static int32 hgcapture_v0_switch_hal_capture_channel(enum capture_channel param) {

    switch (param) {
        case (CAPTURE_CHANNEL_0):
            return 0;
            break;
        case (CAPTURE_CHANNEL_1):
            return 1;
            break;
        case (CAPTURE_CHANNEL_2):
            return 2;
            break;
        case (CAPTURE_CHANNEL_3):
            return 3;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hgcapture_v0_switch_hal_capture_mode(struct hgcapture_v0_config *config, enum capture_mode param) {

    switch (param) {
        case (CAPTURE_MODE_RISE):
            config->cap_sel = 0;
            config->cap_pol = 0;
            return 0;
            break;
        case (CAPTURE_MODE_FALL):
            config->cap_sel = 0;
            config->cap_pol = 1;
            return 1;
            break;
        case (CAPTURE_MODE_ALL):
            config->cap_sel = 1;
            config->cap_pol = 0;
            return 2;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hgcapture_v0_config(struct hgcapture_v0 *dev, enum capture_channel channel, struct hgcapture_v0_config *config) {

    struct hgcapture_v0_config *p_config       = config;
    uint32                      channel_to_reg = 0;
    int32                       ret_val        = RET_OK;

    /* Make sure the config struct pointer */
    if (!p_config) {
        return -EINVAL;
    }

    /* Make sure the channel */
    channel_to_reg = hgcapture_v0_switch_hal_capture_channel(channel);
    if ((-1) == channel_to_reg) {
        return -EINVAL;
    }

    /* Make sure the timer is attached */
    if ((dev->channel[channel_to_reg])) {
        ret_val = timer_device_ioctl((struct timer_device *)dev->channel[channel_to_reg], HGCAPTURE_V0_FUNC_CMD_MASK, (uint32)p_config, 0);
    } else {
        ret_val = RET_ERR;
    }
    
    return ret_val;
}


/**********************************************************************************/
/*                             ATTCH FUNCTION                                     */
/**********************************************************************************/

static int32 hgcapture_v0_init(struct capture_device *capture, enum capture_channel channel, enum capture_mode mode) {

    struct hgcapture_v0        *dev = (struct hgcapture_v0 *)capture;
    struct hgcapture_v0_config  config      = {0};
    int32                       channel_num = 0;
    int32                       result      = 0;
    int32                       ret_val     = RET_OK;

    /* switch hal enum */
    channel_num = hgcapture_v0_switch_hal_capture_channel(channel);
    result      = hgcapture_v0_switch_hal_capture_mode(&config, mode);
    if (((-1) == channel_num) ||
        ((-1) == result     )) {
        return -EINVAL;
    }

    /* Make sure busy status */
    if (dev->opened[channel_num]) {
        return -EBUSY;
    }

    /* config capture */
    config.func_cmd = HGCAPTURE_V0_FUNC_CMD_INIT;

    ret_val = hgcapture_v0_config(dev, channel, &config);

    dev->opened[channel_num] = 1;

    return ret_val;
}

static int32 hgcapture_v0_deinit(struct capture_device *capture, enum capture_channel channel) {

    struct hgcapture_v0 *dev = (struct hgcapture_v0 *)capture;
    struct hgcapture_v0_config  config      = {0};
    int32                       channel_num = 0;
    int32                       ret_val     = RET_OK;
    
    /* Make sure the channel */
    channel_num = hgcapture_v0_switch_hal_capture_channel(channel);
    if ((-1) == channel_num) {
        return -EINVAL;
    }

    /* Make busy status */
    if (!(dev->opened[channel_num])) {
        return RET_OK;
    }

    config.func_cmd = HGCAPTURE_V0_FUNC_CMD_DEINIT;

    ret_val = hgcapture_v0_config(dev, channel, &config);

    dev->opened[channel_num] = 0;

    return ret_val;
}

static int32 hgcapture_v0_start(struct capture_device *capture, enum capture_channel channel) {

    struct hgcapture_v0 *dev = (struct hgcapture_v0 *)capture;
    struct hgcapture_v0_config  config      = {0};
    int32                       channel_num = 0;
    int32                       ret_val     = RET_OK;
    
    /* Make sure the channel */
    channel_num = hgcapture_v0_switch_hal_capture_channel(channel);
    if ((-1) == channel_num) {
        return -EINVAL;
    }

    /* Make busy status */
    if (!(dev->opened[channel_num])) {
        return RET_ERR;
    }

    config.func_cmd = HGCAPTURE_V0_FUNC_CMD_START;
    
    ret_val = hgcapture_v0_config(dev, channel, &config);

    return ret_val;
}

static int32 hgcapture_v0_stop(struct capture_device *capture, enum capture_channel channel) {

    struct hgcapture_v0 *dev = (struct hgcapture_v0 *)capture;
    struct hgcapture_v0_config  config      = {0};
    int32                       channel_num = 0;
    int32                       ret_val     = RET_OK;
    
    /* Make sure the channel */
    channel_num = hgcapture_v0_switch_hal_capture_channel(channel);
    if ((-1) == channel_num) {
        return -EINVAL;
    }

    /* Make busy status */
    if (!(dev->opened[channel_num])) {
        return RET_OK;
    }

    config.func_cmd = HGCAPTURE_V0_FUNC_CMD_STOP;
    
    ret_val = hgcapture_v0_config(dev, channel, &config);

    return ret_val;
}

#ifdef CONFIG_SLEEP
static int32 hgcapture_v0_suspend(struct capture_device *capture, enum capture_channel channel) {

    struct hgcapture_v0 *dev = (struct hgcapture_v0 *)capture;
    struct hgcapture_v0_config  config      = {0};
    int32                       channel_num = 0;
    int32                       ret_val     = RET_OK;
    
    /* Make sure the channel */
    channel_num = hgcapture_v0_switch_hal_capture_channel(channel);
    if ((-1) == channel_num) {
        return -EINVAL;
    }

    /* Make busy status */
    if (!(dev->opened[channel_num])) {
        return RET_ERR;
    }

    config.func_cmd = HGCAPTURE_V0_FUNC_CMD_SUSPEND;
    
    ret_val = hgcapture_v0_config(dev, channel, &config);

    return ret_val;
}

static int32 hgcapture_v0_resume(struct capture_device *capture, enum capture_channel channel) {

    struct hgcapture_v0 *dev = (struct hgcapture_v0 *)capture;
    struct hgcapture_v0_config  config      = {0};
    int32                       channel_num = 0;
    int32                       ret_val     = RET_OK;
    
    /* Make sure the channel */
    channel_num = hgcapture_v0_switch_hal_capture_channel(channel);
    if ((-1) == channel_num) {
        return -EINVAL;
    }

    /* Make busy status */
    if (!(dev->opened[channel_num])) {
        return RET_OK;
    }

    config.func_cmd = HGCAPTURE_V0_FUNC_CMD_RESUME;
    
    ret_val = hgcapture_v0_config(dev, channel, &config);

    return ret_val;
}
#endif

static int32 hgcapture_v0_ioctl(struct capture_device *capture, enum capture_channel channel, enum capture_ioctl_cmd cmd, uint32 param1, uint32 param2) {

    return RET_OK;
}

static int32 hgcapture_v0_request_irq(struct capture_device *capture, enum capture_channel channel, enum capture_irq_flag irq_flag, capture_irq_hdl irq_hdl, uint32 data) {

    struct hgcapture_v0        *dev     = (struct hgcapture_v0 *)capture;
    struct hgcapture_v0_config  config  = {0};

    switch (irq_flag) {
        case (CAPTURE_IRQ_FLAG_CAPTURE):
            config.func_cmd       = HGCAPTURE_V0_FUNC_CMD_REQUEST_IRQ_CAPTURE;
            config.irq_hdl        = irq_hdl;
            config.irq_data       = data;
            hgcapture_v0_config(dev, channel, &config);
            break;
        case (CAPTURE_IRQ_FLAG_OVERFLOW):
            config.func_cmd       = HGCAPTURE_V0_FUNC_CMD_REQUEST_IRQ_OVERFLOW;
            config.irq_hdl        = irq_hdl;
            config.irq_data       = data;
            hgcapture_v0_config(dev, channel, &config);
            break;
    }

    return RET_OK;
}

static int32 hgcapture_v0_release_irq(struct capture_device *capture, enum capture_channel channel) {

    struct hgcapture_v0 *dev = (struct hgcapture_v0 *)capture;
    struct hgcapture_v0_config  config = {0};

    config.func_cmd       = HGCAPTURE_V0_FUNC_CMD_RELEASE_IRQ;
    config.irq_data       = 0;
    config.irq_hdl        = NULL;

    hgcapture_v0_config(dev, channel, &config);
    
    return RET_OK;
}

static const struct capture_hal_ops cap_ops = {
    .init        = hgcapture_v0_init,
    .deinit      = hgcapture_v0_deinit,
    .start       = hgcapture_v0_start,
    .stop        = hgcapture_v0_stop,
    .ioctl       = hgcapture_v0_ioctl,
    .request_irq = hgcapture_v0_request_irq,
    .release_irq = hgcapture_v0_release_irq,
#ifdef CONFIG_SLEEP
    .ops.suspend = NULL,//hgcapture_v0_suspend,
    .ops.resume  = NULL,//hgcapture_v0_resume,
#endif
};

int32 hgcapture_v0_attach(uint32 dev_id, struct hgcapture_v0 *capture) {

    uint32 i = 0;

    for (i=0; i<HGCAPTURE_MAX_CAPTURE_CHANNEL; i++) {
        capture->opened[i] = 0;
    }
    capture->dev.dev.ops = (const struct devobj_ops *)&cap_ops;
    dev_register(dev_id, (struct dev_obj *)capture);    
    return RET_OK;
}
