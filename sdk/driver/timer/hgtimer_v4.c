/**
 * @file hgtimer_v4.c
 * @author bxd
 * @brief normal timer
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
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "hal/timer_device.h"
#include "hal/pwm.h"
#include "dev/pwm/hgpwm_v0.h"
#include "hal/capture.h"
#include "dev/capture/hgcapture_v0.h"
#include "dev/timer/hgtimer_v4.h"
#include "hgtimer_v4_hw.h"

void hgtimer_v4_clk_gating(void *hw, int en)
{
    if (en) {
        if (TIMER0_BASE == (uint32)hw) {
            sysctrl_tmr0_clk_open();
        } else if (TIMER1_BASE == (uint32)hw) {
            sysctrl_tmr1_clk_open();
        } else if (TIMER2_BASE == (uint32)hw) {
            sysctrl_tmr2_clk_open();
        } else if (TIMER3_BASE == (uint32)hw) {
            sysctrl_tmr3_clk_open();
        }
    } else {
        if (TIMER0_BASE == (uint32)hw) {
            sysctrl_tmr0_clk_close();
        } else if (TIMER1_BASE == (uint32)hw) {
            sysctrl_tmr1_clk_close();
        } else if (TIMER2_BASE == (uint32)hw) {
            sysctrl_tmr2_clk_close();
        } else if (TIMER3_BASE == (uint32)hw) {
            sysctrl_tmr3_clk_close();
        }
    }

}


/**********************************************************************************/
/*                           PWM FUNCTION START                                   */
/**********************************************************************************/
static int32 hgtimer_v4_pwm_switch_func_cmd(enum hgpwm_v0_func_cmd param)
{

    switch (param) {
        case (HGPWM_V0_FUNC_CMD_INIT):
            return HGTIMER_V4_PWM_FUNC_CMD_INIT;
            break;
        case (HGPWM_V0_FUNC_CMD_DEINIT):
            return HGTIMER_V4_PWM_FUNC_CMD_DEINIT;
            break;
        case (HGPWM_V0_FUNC_CMD_START):
            return HGTIMER_V4_PWM_FUNC_CMD_START;
            break;
        case (HGPWM_V0_FUNC_CMD_STOP):
            return HGTIMER_V4_PWM_FUNC_CMD_STOP;
            break;
        case (HGPWM_V0_FUNC_CMD_IOCTL_SET_PERIOD_DUTY):
            return HGTIMER_V4_PWM_FUNC_CMD_IOCTL_SET_PERIOD_DUTY;
            break;
        case (HGPWM_V0_FUNC_CMD_REQUEST_IRQ_COMPARE):
            return HGTIMER_V4_PWM_FUNC_CMD_REQUEST_IRQ_COMPARE;
            break;
        case (HGPWM_V0_FUNC_CMD_REQUEST_IRQ_PERIOD):
            return HGTIMER_V4_PWM_FUNC_CMD_REQUEST_IRQ_PERIOD;
            break;
        case (HGPWM_V0_FUNC_CMD_RELEASE_IRQ):
            return HGTIMER_V4_PWM_FUNC_CMD_RELEASE_IRQ;
            break;
        case (HGPWM_V0_FUNC_CMD_SUSPEND):
            return HGTIMER_V4_PWM_FUNC_CMD_SUSPEND;
            break;
        case (HGPWM_V0_FUNC_CMD_RESUME):
            return HGTIMER_V4_PWM_FUNC_CMD_RESUME;
            break;
        case (HGPWM_V0_FUNC_CMD_IOCTL_SET_PRESCALER):
            return HGTIMER_V4_PWM_FUNC_CMD_IOCTL_SET_PRESCALER;
            break;
        case (HGPWM_V0_FUNC_CMD_IOCTL_SET_PERIOD_DUTY_IMMEDIATELY):
            return HGTIMER_V4_PWM_FUNC_CMD_IOCTL_SET_PERIOD_DUTY_IMMEDIATELY;
            break;
        default:
            return -1;
            break;
    }
}

static inline int32 hgtimer_v4_pwm_func_init(struct hgtimer_v4 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if (dev->opened) {
        if (!dev->dsleep) {
            return -EBUSY;
        }
    }

    hgtimer_v4_clk_gating(hw, 1);

    if (pin_func(dev->dev.dev.dev_id, 1) != RET_OK) {
        return RET_ERR;
    }

    if ((p_config->duty < 0) || ((p_config->duty) > (p_config->period))) {
        return RET_ERR;
    }

    hw->TMR_CON = LL_TIMER_CON_INC_SRC_SEL(0) | \
                  LL_TIMER_CON_MODE_SEL(1)    | \
                  LL_TIMER_CON_PSC(0)         ;
    hw->TMR_IE  = 0;
    hw->TMR_EN  = 0;

    /* period */
    /* 5.5ns */
    hw->TMR_CAP1 = p_config->period;
    hw->TMR_CAP3 = p_config->period;

    /* compare */
    hw->TMR_CAP2 = p_config->duty;
    hw->TMR_CAP4 = p_config->duty;

    /* clear the current cnt  */
    hw->TMR_CNT = 0;

    dev->opened = 1;
    dev->pwm_en = 1;

    return RET_OK;
}

static inline int32 hgtimer_v4_pwm_func_deinit(struct hgtimer_v4 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->opened) || (!dev->pwm_en)) {
        return RET_OK;
    }

    hgtimer_v4_clk_gating(hw, 0);

    irq_disable(dev->irq_num);
    pin_func(dev->dev.dev.dev_id, 0);

    hw->TMR_EN   &= ~ LL_TIMER_EN_TMREN;
    hw->TMR_CON   = 0x00000000;
    hw->TMR_CLR   = 0xffffffff;
    hw->TMR_CNT   = 0;
    dev->opened   = 0;
    dev->pwm_en   = 0;

    return RET_OK;
}

static inline int32 hgtimer_v4_pwm_func_start(struct hgtimer_v4 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    hw->TMR_EN |= LL_TIMER_EN_TMREN;

    return RET_OK;
}

static inline int32 hgtimer_v4_pwm_func_stop(struct hgtimer_v4 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    hw->TMR_EN &= ~ LL_TIMER_EN_TMREN;

    return RET_OK;
}

static inline int32 hgtimer_v4_pwm_func_ioctl_set_period_duty(struct hgtimer_v4 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    if ((p_config->duty < 0) || ((p_config->duty) > (p_config->period))) {
        return RET_ERR;
    }

    /* period */
    hw->TMR_CAP3 = p_config->period;

    /* compare */
    hw->TMR_CAP4 = p_config->duty;

    return RET_OK;
}

static inline int32 hgtimer_v4_pwm_func_request_irq_compare(struct hgtimer_v4 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    dev->_pwm_irq_hdl = p_config->irq_hdl;
    dev->irq_data     = p_config->irq_data;
    hw->TMR_IE       |= LL_TIMER_IE_CMP_IE;
    irq_enable(dev->irq_num);

    return RET_OK;
}

static inline int32 hgtimer_v4_pwm_func_request_irq_period(struct hgtimer_v4 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    dev->_pwm_irq_hdl = p_config->irq_hdl;
    dev->irq_data     = p_config->irq_data;
    hw->TMR_IE       |= LL_TIMER_IE_PRD_IE;
    irq_enable(dev->irq_num);

    return RET_OK;
}

static inline int32 hgtimer_v4_pwm_func_release_irq(struct hgtimer_v4 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    irq_disable(dev->irq_num);
    dev->_pwm_irq_hdl = NULL;
    dev->irq_data     = 0;
    hw->TMR_IE        = 0;

    return RET_OK;
}

static int32 hgtimer_v4_pwm_func_ioctl_set_prescaler(struct hgtimer_v4 *dev, struct hgpwm_v0_config *p_config)
{
    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_ERR;
    }

	hw->TMR_CON = (hw->TMR_CON &~ (LL_TIMER_CON_PSC(0x7))) | (LL_TIMER_CON_PSC(p_config->param1));


	return RET_OK;
}

static int32 hgtimer_v4_pwm_func_ioctl_set_period_duty_immediately(struct hgtimer_v4 *dev, struct hgpwm_v0_config *p_config)
{
    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_ERR;
    }

	hw->TMR_EN  &=~ LL_TIMER_EN_TMREN;
	hw->TMR_CNT  = 0;
	hw->TMR_CAP1 = p_config->period;
	hw->TMR_CAP2 = p_config->duty;
	hw->TMR_CAP3 = p_config->period;
	hw->TMR_CAP4 = p_config->duty;
	hw->TMR_CNT  = 0;
	hw->TMR_CLR  = 0xFFFFFFFF;
	hw->TMR_EN   = LL_TIMER_EN_TMREN;
	

	return RET_OK;
}


#ifdef CONFIG_SLEEP
static inline int32 hgtimer_v4_pwm_func_suspend(struct hgtimer_v4 *dev, struct hgpwm_v0_config *p_config)
{
    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_OK;
    }

    if (0 > os_mutex_lock(&dev->bp_suspend_lock, 10000)) {
        return RET_ERR;
    }

    /*
     * close irq
     */
    irq_disable(dev->irq_num);

    /*
     * clear pending
     */
    hw->TMR_FLG = 0xFFFFFFFF;
    hw->TMR_CLR = 0xFFFFFFFF;

    os_memset((void *)&dev->bp_regs, 0, sizeof(dev->bp_regs));

    /*
     * save the reglist
     */
    dev->bp_regs.tmr_con     = hw->TMR_CON    ;
    dev->bp_regs.tmr_en      = hw->TMR_EN     ;
    dev->bp_regs.tmr_cap1    = hw->TMR_CAP1   ;
    dev->bp_regs.tmr_cap2    = hw->TMR_CAP2   ;
    dev->bp_regs.tmr_cap3    = hw->TMR_CAP3   ;
    dev->bp_regs.tmr_cap4    = hw->TMR_CAP4   ;
    dev->bp_regs.tmr_dadr    = hw->TMR_DADR   ;
    dev->bp_regs.tmr_dcnt    = hw->TMR_DCNT   ;
    dev->bp_regs.tmr_dctl    = hw->TMR_DCTL   ;
    dev->bp_regs.tmr_dlen    = hw->TMR_DLEN   ;
    dev->bp_regs.tmr_ie      = hw->TMR_IE     ;
    dev->bp_regs.tmr_ir_bcnt = hw->TMR_IR_BCNT;


    /*
     * save the irq_hdl created by user
     */
    dev->bp_irq_hdl_pwm  = dev->_pwm_irq_hdl;
    dev->bp_irq_data_pwm = dev->irq_data;


    hgtimer_v4_clk_gating(hw, 0);

    dev->dsleep = 1;

    os_mutex_unlock(&dev->bp_suspend_lock);

    return RET_OK;
}

static inline int32 hgtimer_v4_pwm_func_resume(struct hgtimer_v4 *dev, struct hgpwm_v0_config *p_config)
{
    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_OK;
    }

    if (0 > os_mutex_lock(&dev->bp_resume_lock, 10000)) {
        return RET_ERR;
    }


    hgtimer_v4_clk_gating(hw, 1);


    /*
     * recovery the reglist from sram
     */
    hw->TMR_CAP1    = dev->bp_regs.tmr_cap1   ;
    hw->TMR_CAP2    = dev->bp_regs.tmr_cap2   ;
    hw->TMR_CAP3    = dev->bp_regs.tmr_cap3   ;
    hw->TMR_CAP4    = dev->bp_regs.tmr_cap4   ;
    hw->TMR_DADR    = dev->bp_regs.tmr_dadr   ;
    hw->TMR_DCNT    = dev->bp_regs.tmr_dcnt   ;
    hw->TMR_DCTL    = dev->bp_regs.tmr_dctl   ;
    hw->TMR_DLEN    = dev->bp_regs.tmr_dlen   ;
    hw->TMR_IE      = dev->bp_regs.tmr_ie     ;
    hw->TMR_IR_BCNT = dev->bp_regs.tmr_ir_bcnt;
    hw->TMR_CON     = dev->bp_regs.tmr_con    ;
    hw->TMR_EN      = dev->bp_regs.tmr_en     ;


    /*
     * recovery the irq handle and data
     */
    dev->_pwm_irq_hdl = dev->bp_irq_hdl_pwm;
    dev->irq_data     = dev->bp_irq_data_pwm;

    os_memset((void *)&dev->bp_regs, 0, sizeof(dev->bp_regs));

    /*
     * open irq
     */
    irq_enable(dev->irq_num);

    dev->dsleep = 0;

    os_mutex_unlock(&dev->bp_resume_lock);

    return RET_OK;
}
#endif

static int32 hgtimer_v4_pwm_config(struct hgtimer_v4 *dev, uint32 config, uint32 param)
{

    struct hgpwm_v0_config *p_config;
    int32  hgtimer_v4_pwm_func_cmd = 0;
    int32  ret_val = RET_OK;

    /* Make sure the config struct pointer */
    if (!config) {
        return -EINVAL;
    }

    p_config = (struct hgpwm_v0_config *)config;
    hgtimer_v4_pwm_func_cmd = hgtimer_v4_pwm_switch_func_cmd(p_config->func_cmd);
    if ((-1) == hgtimer_v4_pwm_func_cmd) {
        return -EINVAL;
    }

    switch (hgtimer_v4_pwm_func_cmd) {
        case (HGTIMER_V4_PWM_FUNC_CMD_INIT):
            ret_val = hgtimer_v4_pwm_func_init(dev, p_config);
            break;
        case (HGTIMER_V4_PWM_FUNC_CMD_DEINIT):
            ret_val = hgtimer_v4_pwm_func_deinit(dev, p_config);
            break;
        case (HGTIMER_V4_PWM_FUNC_CMD_START):
            ret_val = hgtimer_v4_pwm_func_start(dev, p_config);
            break;
        case (HGTIMER_V4_PWM_FUNC_CMD_STOP):
            ret_val = hgtimer_v4_pwm_func_stop(dev, p_config);
            break;
        case (HGTIMER_V4_PWM_FUNC_CMD_IOCTL_SET_PERIOD_DUTY):
            ret_val = hgtimer_v4_pwm_func_ioctl_set_period_duty(dev, p_config);
            break;
        case (HGTIMER_V4_PWM_FUNC_CMD_REQUEST_IRQ_COMPARE):
            ret_val = hgtimer_v4_pwm_func_request_irq_compare(dev, p_config);
            break;
        case (HGTIMER_V4_PWM_FUNC_CMD_REQUEST_IRQ_PERIOD):
            ret_val = hgtimer_v4_pwm_func_request_irq_period(dev, p_config);
            break;
        case (HGTIMER_V4_PWM_FUNC_CMD_RELEASE_IRQ):
            ret_val = hgtimer_v4_pwm_func_release_irq(dev, p_config);
            break;
        case (HGTIMER_V4_PWM_FUNC_CMD_IOCTL_SET_PRESCALER):
            ret_val = hgtimer_v4_pwm_func_ioctl_set_prescaler(dev, p_config);
            break;
        case (HGTIMER_V4_PWM_FUNC_CMD_IOCTL_SET_PERIOD_DUTY_IMMEDIATELY):
            ret_val = hgtimer_v4_pwm_func_ioctl_set_period_duty_immediately(dev, p_config);
            break;
#ifdef CONFIG_SLEEP
        case (HGTIMER_V4_PWM_FUNC_CMD_SUSPEND):
            ret_val = hgtimer_v4_pwm_func_suspend(dev, p_config);
            break;
        case (HGTIMER_V4_PWM_FUNC_CMD_RESUME):
            ret_val = hgtimer_v4_pwm_func_resume(dev, p_config);
            break;
#endif
        default:
            ret_val = -ENOTSUPP;
            break;
    }

    return ret_val;
}

/**********************************************************************************/
/*                            PWM FUNCTION END                                    */
/**********************************************************************************/





/**********************************************************************************/
/*                            CAP FUNCTION START                                  */
/**********************************************************************************/
static int32 hgtimer_v4_capture_switch_func_cmd(enum hgcapture_v0_func_cmd param)
{

    switch (param) {
        case (HGCAPTURE_V0_FUNC_CMD_INIT):
            return HGTIMER_V4_CAPTURE_FUNC_CMD_INIT;
            break;
        case (HGCAPTURE_V0_FUNC_CMD_DEINIT):
            return HGTIMER_V4_CAPTURE_FUNC_CMD_DEINIT;
            break;
        case (HGCAPTURE_V0_FUNC_CMD_START):
            return HGTIMER_V4_CAPTURE_FUNC_CMD_START;
            break;
        case (HGCAPTURE_V0_FUNC_CMD_STOP):
            return HGTIMER_V4_CAPTURE_FUNC_CMD_STOP;
            break;
        case (HGCAPTURE_V0_FUNC_CMD_REQUEST_IRQ_CAPTURE):
            return HGTIMER_V4_CAPTURE_FUNC_CMD_REQUEST_IRQ_CAPTURE;
            break;
        case (HGCAPTURE_V0_FUNC_CMD_REQUEST_IRQ_OVERFLOW):
            return HGTIMER_V4_CAPTURE_FUNC_CMD_REQUEST_IRQ_OVERFLOW;
            break;
        case (HGCAPTURE_V0_FUNC_CMD_RELEASE_IRQ):
            return HGTIMER_V4_CAPTURE_FUNC_CMD_RELEASE_IRQ;
            break;
        case (HGCAPTURE_V0_FUNC_CMD_SUSPEND):
            return HGTIMER_V4_CAPTURE_FUNC_CMD_SUSPEND;
            break;
        case (HGCAPTURE_V0_FUNC_CMD_RESUME):
            return HGTIMER_V4_CAPTURE_FUNC_CMD_RESUME;
            break;
        default:
            return -1;
            break;
    }
}

static inline int32 hgtimer_v4_capture_func_init(struct hgtimer_v4 *dev, struct hgcapture_v0_config *p_config)
{

    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if (dev->opened) {
        if (!dev->dsleep) {
            return -EBUSY;
        }
    }

    hgtimer_v4_clk_gating(hw, 1);

    if (pin_func(dev->dev.dev.dev_id, 2) != RET_OK) {
        return RET_ERR;
    }

    hw->TMR_CON = LL_TIMER_CON_INC_SRC_SEL(0)             | \
                  LL_TIMER_CON_MODE_SEL(2)                | \
                  LL_TIMER_CON_CAP1POL(p_config->cap_pol) | \
                  LL_TIMER_CON_CAP_SEL(p_config->cap_sel) | \
                  LL_TIMER_CON_CTRRST1                    | \
                  LL_TIMER_CON_PSC(0)                     ;
    hw->TMR_IE  = 0;
    hw->TMR_EN  = 0;

    hw->TMR_CNT = 0;

    dev->opened = 1;
    dev->cap_en = 1;

    return RET_OK;
}

static inline int32 hgtimer_v4_capture_func_deinit(struct hgtimer_v4 *dev, struct hgcapture_v0_config *p_config)
{

    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->opened) || (!dev->pwm_en)) {
        return RET_OK;
    }

    hgtimer_v4_clk_gating(hw, 0);

    irq_disable(dev->irq_num);
    pin_func(dev->dev.dev.dev_id, 0);

    hw->TMR_EN   &= ~ LL_TIMER_EN_TMREN;
    hw->TMR_CON   = 0x00000000;
    hw->TMR_CLR   = 0xffffffff;
    hw->TMR_CNT   = 0;
    dev->opened   = 0;
    dev->cap_en   = 0;

    return RET_OK;
}

static inline int32 hgtimer_v4_capture_func_start(struct hgtimer_v4 *dev, struct hgcapture_v0_config *p_config)
{

    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->cap_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    hw->TMR_EN |= LL_TIMER_EN_TMREN;

    return RET_OK;
}

static inline int32 hgtimer_v4_capture_func_stop(struct hgtimer_v4 *dev, struct hgcapture_v0_config *p_config)
{

    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->cap_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    hw->TMR_EN &= ~ LL_TIMER_EN_TMREN;

    return RET_OK;
}

static inline int32 hgtimer_v4_capture_func_request_irq_capture(struct hgtimer_v4 *dev, struct hgcapture_v0_config *p_config)
{

    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->cap_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    dev->_capture_irq_hdl = p_config->irq_hdl;
    dev->irq_data         = p_config->irq_data;
    hw->TMR_IE           |= LL_TIMER_IE_CAP1_IE;
    irq_enable(dev->irq_num);

    return RET_OK;
}

static inline int32 hgtimer_v4_capture_func_request_irq_overflow(struct hgtimer_v4 *dev, struct hgcapture_v0_config *p_config)
{

    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->cap_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    dev->_capture_irq_hdl = p_config->irq_hdl;
    dev->irq_data         = p_config->irq_data;
    hw->TMR_IE           |= LL_TIMER_IE_OVF_IE;
    irq_enable(dev->irq_num);

    return RET_OK;
}

static inline int32 hgtimer_v4_capture_func_release_irq(struct hgtimer_v4 *dev, struct hgcapture_v0_config *p_config)
{

    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->cap_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    irq_disable(dev->irq_num);
    dev->_capture_irq_hdl = NULL;
    dev->irq_data         = 0;
    hw->TMR_IE            = 0;

    return RET_OK;
}

#ifdef CONFIG_SLEEP
static inline int32 hgtimer_v4_capture_func_suspend(struct hgtimer_v4 *dev, struct hgcapture_v0_config *p_config)
{
    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->cap_en) || (dev->dsleep)) {
        return RET_OK;
    }

    if (0 > os_mutex_lock(&dev->bp_suspend_lock, 10000)) {
        return RET_ERR;
    }

    /*
     * close irq
     */
    irq_disable(dev->irq_num);

    /*
     * clear pending
     */
    hw->TMR_FLG = 0xFFFFFFFF;
    hw->TMR_CLR = 0xFFFFFFFF;

    os_memset((void *)&dev->bp_regs, 0, sizeof(dev->bp_regs));

    /*
     * save the reglist
     */
    dev->bp_regs.tmr_con     = hw->TMR_CON    ;
    dev->bp_regs.tmr_en      = hw->TMR_EN     ;
    dev->bp_regs.tmr_cap1    = hw->TMR_CAP1   ;
    dev->bp_regs.tmr_cap2    = hw->TMR_CAP2   ;
    dev->bp_regs.tmr_cap3    = hw->TMR_CAP3   ;
    dev->bp_regs.tmr_cap4    = hw->TMR_CAP4   ;
    dev->bp_regs.tmr_dadr    = hw->TMR_DADR   ;
    dev->bp_regs.tmr_dcnt    = hw->TMR_DCNT   ;
    dev->bp_regs.tmr_dctl    = hw->TMR_DCTL   ;
    dev->bp_regs.tmr_dlen    = hw->TMR_DLEN   ;
    dev->bp_regs.tmr_ie      = hw->TMR_IE     ;
    dev->bp_regs.tmr_ir_bcnt = hw->TMR_IR_BCNT;


    /*
     * save the irq_hdl created by user
     */
    dev->bp_irq_hdl_capture  = dev->_capture_irq_hdl;
    dev->bp_irq_data_capture = dev->irq_data;


    hgtimer_v4_clk_gating(hw, 0);

    dev->dsleep = 1;

    os_mutex_unlock(&dev->bp_suspend_lock);

    return RET_OK;
}

static inline int32 hgtimer_v4_capture_func_resume(struct hgtimer_v4 *dev, struct hgcapture_v0_config *p_config)
{
    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->cap_en) || (dev->dsleep)) {
        return RET_OK;
    }

    if (0 > os_mutex_lock(&dev->bp_resume_lock, 10000)) {
        return RET_ERR;
    }


    hgtimer_v4_clk_gating(hw, 1);


    /*
     * recovery the reglist from sram
     */
    hw->TMR_CAP1    = dev->bp_regs.tmr_cap1   ;
    hw->TMR_CAP2    = dev->bp_regs.tmr_cap2   ;
    hw->TMR_CAP3    = dev->bp_regs.tmr_cap3   ;
    hw->TMR_CAP4    = dev->bp_regs.tmr_cap4   ;
    hw->TMR_DADR    = dev->bp_regs.tmr_dadr   ;
    hw->TMR_DCNT    = dev->bp_regs.tmr_dcnt   ;
    hw->TMR_DCTL    = dev->bp_regs.tmr_dctl   ;
    hw->TMR_DLEN    = dev->bp_regs.tmr_dlen   ;
    hw->TMR_IE      = dev->bp_regs.tmr_ie     ;
    hw->TMR_IR_BCNT = dev->bp_regs.tmr_ir_bcnt;
    hw->TMR_CON     = dev->bp_regs.tmr_con    ;
    hw->TMR_EN      = dev->bp_regs.tmr_en     ;


    /*
     * recovery the irq handle and data
     */
    dev->_capture_irq_hdl = dev->bp_irq_hdl_capture;
    dev->irq_data         = dev->bp_irq_data_capture;

    os_memset((void *)&dev->bp_regs, 0, sizeof(dev->bp_regs));

    /*
     * open irq
     */
    irq_enable(dev->irq_num);

    dev->dsleep = 0;

    os_mutex_unlock(&dev->bp_resume_lock);

    return RET_OK;
}
#endif

static int32 hgtimer_v4_capture_config(struct hgtimer_v4 *dev, uint32 config, uint32 param)
{

    struct hgcapture_v0_config *p_config;
    int32  hgtimer_v4_cap_func_cmd = 0;
    int32  ret_val = RET_OK;

    /* Make sure the config struct pointer */
    if (!config) {
        return -EINVAL;
    }

    p_config = (struct hgcapture_v0_config *)config;
    hgtimer_v4_cap_func_cmd = hgtimer_v4_capture_switch_func_cmd(p_config->func_cmd);
    if ((-1) == hgtimer_v4_cap_func_cmd) {
        return -EINVAL;
    }

    switch (hgtimer_v4_cap_func_cmd) {
        case (HGTIMER_V4_CAPTURE_FUNC_CMD_INIT):
            ret_val = hgtimer_v4_capture_func_init(dev, p_config);
            break;
        case (HGTIMER_V4_CAPTURE_FUNC_CMD_DEINIT):
            ret_val = hgtimer_v4_capture_func_deinit(dev, p_config);
            break;
        case (HGTIMER_V4_CAPTURE_FUNC_CMD_START):
            ret_val = hgtimer_v4_capture_func_start(dev, p_config);
            break;
        case (HGTIMER_V4_CAPTURE_FUNC_CMD_STOP):
            ret_val = hgtimer_v4_capture_func_stop(dev, p_config);
            break;
        case (HGTIMER_V4_CAPTURE_FUNC_CMD_REQUEST_IRQ_CAPTURE):
            ret_val = hgtimer_v4_capture_func_request_irq_capture(dev, p_config);
            break;
        case (HGTIMER_V4_CAPTURE_FUNC_CMD_REQUEST_IRQ_OVERFLOW):
            ret_val = hgtimer_v4_capture_func_request_irq_overflow(dev, p_config);
            break;
        case (HGTIMER_V4_CAPTURE_FUNC_CMD_RELEASE_IRQ):
            ret_val = hgtimer_v4_capture_func_release_irq(dev, p_config);
            break;
#ifdef CONFIG_SLEEP
        case (HGTIMER_V4_CAPTURE_FUNC_CMD_SUSPEND):
            ret_val = hgtimer_v4_capture_func_suspend(dev, p_config);
            break;
        case (HGTIMER_V4_CAPTURE_FUNC_CMD_RESUME):
            ret_val = hgtimer_v4_capture_func_resume(dev, p_config);
            break;
#endif
        default:
            ret_val = -ENOTSUPP;
            break;
    }

    return ret_val;
}

/**********************************************************************************/
/*                            CAP FUNCTION END                                    */
/**********************************************************************************/




/**********************************************************************************/
/*                         COUNTER FUNCTION START                                 */
/**********************************************************************************/
static int32 hgtimer_v4_counter_switch_hal_type(struct hgtimer_v4 *dev, enum timer_type param)
{

    switch (param) {
        case (TIMER_TYPE_ONCE):
            dev->type = HGTIMER_V4_TYPE_ONCE;
            return 0;
            break;
        case (TIMER_TYPE_PERIODIC):
            dev->type = HGTIMER_V4_TYPE_PERIODIC;
            return 1;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hgtimer_v4_counter_set_psc(struct hgtimer_v4 *timer, uint32 psc)
{
    struct hgtimer_v4 *dev = (struct hgtimer_v4 *)timer;
    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if (!dev->counter_en) {
        return RET_ERR;
    }

    hw->TMR_EN  = 0;
    hw->TMR_CON = (hw->TMR_CON & ~ LL_TIMER_CON_PSC(0x7)) | LL_TIMER_CON_PSC(psc);
    hw->TMR_CNT = 0;

    return RET_OK;
}

static int32 hgtimer_v4_counter_set_slavemode(struct hgtimer_v4 *timer, uint32 slavemode)
{
    struct hgtimer_v4 *dev = (struct hgtimer_v4 *)timer;
    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if (!dev->opened || !dev->counter_en) {
        return RET_ERR;
    }

    hw->TMR_EN  = 0;
    hw->TMR_CON = (hw->TMR_CON & ~ LL_TIMER_CON_SLAVE_MODE(0x3)) | LL_TIMER_CON_SLAVE_MODE(slavemode);
    hw->TMR_CNT = 0;

    return RET_OK;
}

static int32 hgtimer_v4_counter_set_period(struct hgtimer_v4 *timer, uint32 period)
{
    struct hgtimer_v4 *dev = (struct hgtimer_v4 *)timer;
    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

	hw->TMR_EN  &=~ LL_TIMER_EN_TMREN;
	hw->TMR_CNT  = 0;
	hw->TMR_CAP1 = period;
	hw->TMR_CAP3 = period;
	hw->TMR_CNT  = 0;
	hw->TMR_CLR  = 0xFFFFFFFF;
	hw->TMR_EN   = LL_TIMER_EN_TMREN;

    return RET_OK;

}

static int32 hgtimer_v4_counter_set_source(struct hgtimer_v4 *timer, uint32 src)
{
    struct hgtimer_v4 *dev = (struct hgtimer_v4 *)timer;
    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if (src > 0x7) {
        return RET_ERR;
    }

    hw->TMR_CON  = ((hw->TMR_CON) &~ (LL_TIMER_CON_INC_SRC_SEL(0x7))) | (LL_TIMER_CON_INC_SRC_SEL(src));

    return RET_OK;

}

static int32 hgtimer_v4_counter_func_open(struct timer_device *timer, enum timer_type type, uint32 flags)
{

    struct hgtimer_v4 *dev = (struct hgtimer_v4 *)timer;
    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if (dev->opened) {
        if (!dev->dsleep) {
            return -EBUSY;
        }
    }

    hgtimer_v4_clk_gating(hw, 1);


    if ((-1) == hgtimer_v4_counter_switch_hal_type(dev, type)) {
        return RET_ERR;
    }


    /* clear reg */
    hw->TMR_CON     = 0;
    hw->TMR_EN      = 0;
    hw->TMR_CAP1    = 0;
    hw->TMR_CAP2    = 0;
    hw->TMR_CAP3    = 0;
    hw->TMR_CAP4    = 0;
    hw->TMR_CNT     = 0;
    hw->TMR_DADR    = 0;
    hw->TMR_DCNT    = 0;
    hw->TMR_DCTL    = 0;
    hw->TMR_DLEN    = 0;
    hw->TMR_FLG     = 0xFFFFFFFF;
    hw->TMR_CLR     = 0xFFFFFFFF;
    hw->TMR_IE      = 0;
    hw->TMR_IR_BCNT = 0;


    irq_enable(dev->irq_num);

    switch (dev->type) {
        /* TIMER_TYPE_ONCE */
        case (HGTIMER_V4_TYPE_ONCE):

        /* TIMER_TYPE_PERIODIC */
        case (HGTIMER_V4_TYPE_PERIODIC):

        /* TIMER_TYPE_COUNTER */
        case (HGTIMER_V4_TYPE_COUNTER):
            hw->TMR_CON = LL_TIMER_CON_INC_SRC_SEL(0) | \
                          LL_TIMER_CON_MODE_SEL(0)    | \
                          LL_TIMER_CON_PSC(0)         ;
            hw->TMR_IE  = 0;
            hw->TMR_EN  = 0;
            hw->TMR_CNT = 0;
            break;
        default:
            return RET_ERR;
            break;
    }

    dev->opened     = 1;
    dev->counter_en = 1;
    dev->dsleep     = 0;

    return RET_OK;
}

static int32 hgtimer_v4_counter_func_close(struct timer_device *timer)
{

    struct hgtimer_v4 *dev = (struct hgtimer_v4 *)timer;
    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->opened) || (!dev->counter_en)) {
        return RET_OK;
    }

    hgtimer_v4_clk_gating(hw, 0);

    irq_disable(dev->irq_num);

    hw->TMR_EN       &= ~ LL_TIMER_EN_TMREN;
    hw->TMR_CON       = 0x00000000;
    hw->TMR_CLR       = 0xffffffff;
    hw->TMR_CNT       = 0;
    dev->opened       = 0;
    dev->counter_en   = 0;
    dev->dsleep       = 0;

    return RET_OK;
}

static int32 hgtimer_v4_counter_func_start(struct timer_device *timer, uint32 period_sysclkpd_cnt, timer_irq_hdl cb, uint32 cb_data)
{

    struct hgtimer_v4 *dev = (struct hgtimer_v4 *)timer;
    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->counter_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    switch (dev->type) {

        /* TIMER_TYPE_ONCE */
        case (HGTIMER_V4_TYPE_ONCE):

            dev->counter_once_en = 1;

            /* config period */
            hw->TMR_CAP1 = period_sysclkpd_cnt;
            hw->TMR_CAP3 = period_sysclkpd_cnt;

            hw->TMR_CNT  = 0;

            if (cb) {
                dev->_counter_irq_hdl = cb;
                dev->irq_data         = cb_data;
                hw->TMR_IE           |= LL_TIMER_IE_PRD_IE;
            }

            hw->TMR_EN |= LL_TIMER_EN_TMREN;
            break;

        /* TIMER_TYPE_PERIODIC */
        case (HGTIMER_V4_TYPE_PERIODIC):

        /* TIMER_TYPE_COUNTER */
        case (HGTIMER_V4_TYPE_COUNTER):

            dev->counter_period_en = 1;

            /* config period */
            hw->TMR_CAP1 = period_sysclkpd_cnt;
            hw->TMR_CAP3 = period_sysclkpd_cnt;

            hw->TMR_CNT  = 0;

            if (cb) {
                dev->_counter_irq_hdl = cb;
                dev->irq_data         = cb_data;
                hw->TMR_IE      |= LL_TIMER_IE_PRD_IE;
            }

            hw->TMR_EN |= LL_TIMER_EN_TMREN;
            break;
        default:
            return RET_ERR;
            break;
    }

    return RET_OK;
}

static int32 hgtimer_v4_counter_func_stop(struct timer_device *timer)
{

    struct hgtimer_v4 *dev = (struct hgtimer_v4 *)timer;
    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->counter_en) || (dev->dsleep)) {
        return RET_ERR;
    }

    hw->TMR_EN &= ~ LL_TIMER_EN_TMREN;

    return RET_OK;
}

#ifdef CONFIG_SLEEP
static int32 hgtimer_v4_counter_func_suspend(struct dev_obj *obj)
{
    struct hgtimer_v4 *dev = (struct hgtimer_v4 *)obj;
    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->counter_en) || (dev->dsleep)) {
        return RET_OK;
    }

    if (0 > os_mutex_lock(&dev->bp_suspend_lock, 10000)) {
        return RET_ERR;
    }

    /*
     * close irq
     */
    irq_disable(dev->irq_num);

    /*
     * clear pending
     */
    hw->TMR_FLG = 0xFFFFFFFF;
    hw->TMR_CLR = 0xFFFFFFFF;

    os_memset((void *)&dev->bp_regs, 0, sizeof(dev->bp_regs));

    /*
     * save the reglist
     */
    dev->bp_regs.tmr_con     = hw->TMR_CON    ;
    dev->bp_regs.tmr_en      = hw->TMR_EN     ;
    dev->bp_regs.tmr_cap1    = hw->TMR_CAP1   ;
    dev->bp_regs.tmr_cap2    = hw->TMR_CAP2   ;
    dev->bp_regs.tmr_cap3    = hw->TMR_CAP3   ;
    dev->bp_regs.tmr_cap4    = hw->TMR_CAP4   ;
    dev->bp_regs.tmr_dadr    = hw->TMR_DADR   ;
    dev->bp_regs.tmr_dcnt    = hw->TMR_DCNT   ;
    dev->bp_regs.tmr_dctl    = hw->TMR_DCTL   ;
    dev->bp_regs.tmr_dlen    = hw->TMR_DLEN   ;
    dev->bp_regs.tmr_ie      = hw->TMR_IE     ;
    dev->bp_regs.tmr_ir_bcnt = hw->TMR_IR_BCNT;


    /*
     * save the irq_hdl created by user
     */
    dev->bp_irq_hdl_timer  = dev->_counter_irq_hdl;
    dev->bp_irq_data_timer = dev->irq_data;


    hgtimer_v4_clk_gating(hw, 0);

    dev->dsleep = 1;

    os_mutex_unlock(&dev->bp_suspend_lock);

    return RET_OK;
}

static int32 hgtimer_v4_counter_func_resume(struct dev_obj *obj)
{
    struct hgtimer_v4 *dev = (struct hgtimer_v4 *)obj;
    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;

    if ((!dev->counter_en) || (dev->dsleep)) {
        return RET_OK;
    }

    if (0 > os_mutex_lock(&dev->bp_resume_lock, 10000)) {
        return RET_ERR;
    }


    hgtimer_v4_clk_gating(hw, 1);


    /*
     * recovery the reglist from sram
     */
    hw->TMR_CAP1    = dev->bp_regs.tmr_cap1   ;
    hw->TMR_CAP2    = dev->bp_regs.tmr_cap2   ;
    hw->TMR_CAP3    = dev->bp_regs.tmr_cap3   ;
    hw->TMR_CAP4    = dev->bp_regs.tmr_cap4   ;
    hw->TMR_DADR    = dev->bp_regs.tmr_dadr   ;
    hw->TMR_DCNT    = dev->bp_regs.tmr_dcnt   ;
    hw->TMR_DCTL    = dev->bp_regs.tmr_dctl   ;
    hw->TMR_DLEN    = dev->bp_regs.tmr_dlen   ;
    hw->TMR_IE      = dev->bp_regs.tmr_ie     ;
    hw->TMR_IR_BCNT = dev->bp_regs.tmr_ir_bcnt;
    hw->TMR_CON     = dev->bp_regs.tmr_con    ;
    hw->TMR_EN      = dev->bp_regs.tmr_en     ;


    /*
     * recovery the irq handle and data
     */
    dev->_counter_irq_hdl = dev->bp_irq_hdl_timer;
    dev->irq_data         = dev->bp_irq_data_timer;

    os_memset((void *)&dev->bp_regs, 0, sizeof(dev->bp_regs));

    /*
     * open irq
     */
    irq_enable(dev->irq_num);

    dev->dsleep = 0;

    os_mutex_unlock(&dev->bp_resume_lock);

    return RET_OK;
}
#endif


static int32 hgtimer_v4_counter_func_ioctl(struct timer_device *timer, uint32 cmd, uint32 param1, uint32 param2)
{

    int32  ret_val = RET_OK;
    struct hgtimer_v4 *dev = (struct hgtimer_v4 *)timer;

    switch (cmd) {
        case (HGPWM_V0_FUNC_CMD_MASK):
            ret_val = hgtimer_v4_pwm_config(dev, param1, 0);
            break;
        case (HGCAPTURE_V0_FUNC_CMD_MASK):
            ret_val = hgtimer_v4_capture_config(dev, param1, 0);
            break;
        case (TIMER_SET_CLK_PSC):
            ret_val = hgtimer_v4_counter_set_psc(dev, param1);
            break;
        case (TIMER_SET_SLAVE_MODE):
            ret_val = hgtimer_v4_counter_set_slavemode(dev, param1);
            break;
        case (TIMER_SET_PERIOD):
            ret_val = hgtimer_v4_counter_set_period(dev, param1);
            break;
        case (TIMER_SET_CLK_SRC):
            ret_val = hgtimer_v4_counter_set_source(dev, param1);
            break;
        default:
            ret_val = RET_ERR;
            break;
    }

    return ret_val;
}

/**********************************************************************************/
/*                          COUNTER FUNCTION END                                  */
/**********************************************************************************/

static void hgtimer_v4_irq_handler(void *data)
{

    struct hgtimer_v4 *dev = (struct hgtimer_v4 *)data;
    struct hgtimer_v4_hw *hw = (struct hgtimer_v4_hw *)dev->hw;


    if ((hw->TMR_IE & LL_TIMER_IE_CMP_IE) && (hw->TMR_FLG & LL_TIMER_IE_CMP_FLG)) {
        /* clear interrupt flag */
        hw->TMR_CLR = LL_TIMER_IE_CMP_CLR;

        /* pwm mode irq */
        if (dev->opened && dev->pwm_en && dev->_pwm_irq_hdl) {
            dev->_pwm_irq_hdl(PWM_IRQ_FLAG_COMPARE, dev->irq_data);
        }

        /* counter mode irq */
        if (dev->opened && dev->counter_en && dev->_counter_irq_hdl) {
            dev->_counter_irq_hdl(dev->irq_data, TIMER_IRQ_FLAG_TIMEOUT);
        }
    }


    if ((hw->TMR_IE & LL_TIMER_IE_PRD_IE) && (hw->TMR_FLG & LL_TIMER_IE_PRD_FLG)) {
        /* clear interrupt flag */
        hw->TMR_CLR = LL_TIMER_IE_PRD_CLR;

        /* pwm mode irq */
        if (dev->opened && dev->pwm_en && dev->_pwm_irq_hdl) {
            dev->_pwm_irq_hdl(PWM_IRQ_FLAG_PERIOD, dev->irq_data);
        }

        /* counter mode irq */
        if (dev->opened && dev->counter_en) {
            if (dev->counter_once_en) {
                dev->counter_once_en = 0;

                /* close timer & interrupt */
                hw->TMR_EN &= ~ LL_TIMER_EN_TMREN;
                hw->TMR_IE &= ~ LL_TIMER_IE_PRD_IE;
            }
            if (dev->_counter_irq_hdl) {
                dev->_counter_irq_hdl(dev->irq_data, TIMER_IRQ_FLAG_TIMEOUT);
            }
        }
    }



    if ((hw->TMR_IE & LL_TIMER_IE_OVF_IE) && (hw->TMR_FLG & LL_TIMER_IE_OVF_FLG)) {
        /* clear interrupt flag */
        hw->TMR_CLR = LL_TIMER_IE_OVF_CLR;

        /* capture mode irq */
        if (dev->opened && dev->cap_en && dev->_capture_irq_hdl) {
            dev->_capture_irq_hdl(CAPTURE_IRQ_FLAG_OVERFLOW, dev->irq_data);
        }
    }


    if ((hw->TMR_IE & LL_TIMER_IE_CAP1_IE) && (hw->TMR_FLG & LL_TIMER_IE_CAP1_FLG)) {
        /* clear interrupt flag */
        hw->TMR_CLR = LL_TIMER_IE_CAP1_CLR;

        /* capture mode irq */
        if (dev->opened && dev->cap_en && dev->_capture_irq_hdl) {
            dev->_capture_irq_hdl(CAPTURE_IRQ_FLAG_CAPTURE, hw->TMR_CAP1);
        }
    }
}

static const struct timer_hal_ops timer_v4_ops = {
    .open         = hgtimer_v4_counter_func_open,
    .close        = hgtimer_v4_counter_func_close,
    .start        = hgtimer_v4_counter_func_start,
    .stop         = hgtimer_v4_counter_func_stop,
    .ioctl        = hgtimer_v4_counter_func_ioctl,
#ifdef CONFIG_SLEEP
    .ops.suspend  = hgtimer_v4_counter_func_suspend,
    .ops.resume   = hgtimer_v4_counter_func_resume,
#endif
};

int32 hgtimer_v4_attach(uint32 dev_id, struct hgtimer_v4 *timer)
{

    timer->opened           = 0;
    timer->dsleep           = 0;
    timer->_counter_irq_hdl = NULL;
    timer->_pwm_irq_hdl     = NULL;
    timer->_capture_irq_hdl = NULL;
    timer->irq_data         = 0;
    timer->dev.dev.ops      = (const struct devobj_ops *)&timer_v4_ops;
#ifdef CONFIG_SLEEP
    os_mutex_init(&timer->bp_suspend_lock);
    os_mutex_init(&timer->bp_resume_lock);
#endif
    request_irq(timer->irq_num, hgtimer_v4_irq_handler, timer);
    dev_register(dev_id, (struct dev_obj *)timer);
    return RET_OK;
}

