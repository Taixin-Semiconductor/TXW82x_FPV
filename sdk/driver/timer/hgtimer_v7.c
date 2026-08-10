/**
 * @file hgtimer_v7.c
 * @author bxd
 * @brief simple timer
 * @version
 * TXW81X; TXW8000
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
#include "dev/timer/hgtimer_v7.h"
#include "hgtimer_v7_hw.h"

void hgtimer_v7_clk_gating(void *hw, int en)
{
    static int stmr_cgctl = 0;
    if (en) {
        switch ((uint32)hw)
        {
        case SIMPLE_TIMER0_BASE:
            stmr_cgctl |= 1<<0;
            break;
        case SIMPLE_TIMER1_BASE:
            stmr_cgctl |= 1<<1;
            break;
        case SIMPLE_TIMER2_BASE:
            stmr_cgctl |= 1<<2;
            break;
        case SIMPLE_TIMER3_BASE:
            stmr_cgctl |= 1<<3;
            break;
        case SIMPLE_TIMER4_BASE:
            stmr_cgctl |= 1<<4;
            break;
        case SIMPLE_TIMER5_BASE:
            stmr_cgctl |= 1<<5;
            break;   
        default:
            break;
        }
        sysctrl_simtmr_clk_open();
    } else {
        switch ((uint32)hw)
        {
        case SIMPLE_TIMER0_BASE:
            stmr_cgctl &= ~(1<<0);
            break;
        case SIMPLE_TIMER1_BASE:
            stmr_cgctl &= ~(1<<1);
            break;
        case SIMPLE_TIMER2_BASE:
            stmr_cgctl &= ~(1<<2);
            break;
        case SIMPLE_TIMER3_BASE:
            stmr_cgctl &= ~(1<<3);
            break;
        case SIMPLE_TIMER4_BASE:
            stmr_cgctl &= ~(1<<4);
            break;
        case SIMPLE_TIMER5_BASE:
            stmr_cgctl &= ~(1<<5);
            break;   
        default:
            break;
        }
        if (stmr_cgctl == 0) {
            sysctrl_simtmr_clk_close();
        }
    }
}

#ifdef CONFIG_SLEEP
static int32 hgtimer_v7_suspend(struct dev_obj *obj)
{
    struct hgtimer_v7 *dev = (struct hgtimer_v7 *)obj;
    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if (!(dev->opened)) {
        return RET_ERR;
    }

    if ((dev->dsleep)) {
        return RET_OK;
    }
    
    irq_disable(dev->irq_num);

    os_memset((void *)&dev->bp_regs, 0, sizeof(dev->bp_regs));

    dev->bp_regs.tmr_ctl = hw->TMR_CTL;
    dev->bp_regs.tmr_cnt = hw->TMR_CNT;
    dev->bp_regs.tmr_pr  = hw->TMR_PR ;
    dev->bp_regs.tmr_pwm = hw->TMR_PWM;

    hw->TMR_CTL &= ~ LL_SIMPLE_TIMER_MODE_SEL(0x3);

    hw->TMR_CTL |= LL_SIMPLE_TIMER_PERIOD_ING;

    sysctrl_simtmr_clk_close();
    dev->dsleep = 1;
    return RET_OK;
}

static int32 hgtimer_v7_resume(struct dev_obj *obj)
{
    struct hgtimer_v7 *dev = (struct hgtimer_v7 *)obj;
    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if (!dev->opened) {
        return RET_ERR;
    }

    if (!dev->dsleep) {
        return RET_OK;
    }

    sysctrl_simtmr_clk_open();

    hw->TMR_CNT    = dev->bp_regs.tmr_cnt;
    hw->TMR_PR     = dev->bp_regs.tmr_pr ;
    hw->TMR_PWM    = dev->bp_regs.tmr_pwm;
    hw->TMR_CTL    = dev->bp_regs.tmr_ctl;

    os_memset((void *)&dev->bp_regs, 0, sizeof(dev->bp_regs));

    irq_enable(dev->irq_num);
    dev->dsleep = 0;
    return RET_OK;
}
#endif

/**********************************************************************************/
/*                           PWM FUNCTION START                                   */
/**********************************************************************************/
static int32 hgtimer_v7_pwm_switch_func_cmd(enum hgpwm_v0_func_cmd param)
{

    switch (param) {
        case (HGPWM_V0_FUNC_CMD_INIT):
            return HGTIMER_V7_PWM_FUNC_CMD_INIT;
            break;
        case (HGPWM_V0_FUNC_CMD_DEINIT):
            return HGTIMER_V7_PWM_FUNC_CMD_DEINIT;
            break;
        case (HGPWM_V0_FUNC_CMD_START):
            return HGTIMER_V7_PWM_FUNC_CMD_START;
            break;
        case (HGPWM_V0_FUNC_CMD_STOP):
            return HGTIMER_V7_PWM_FUNC_CMD_STOP;
            break;
        case (HGPWM_V0_FUNC_CMD_IOCTL_SET_PERIOD_DUTY):
            return HGTIMER_V7_PWM_FUNC_CMD_IOCTL_SET_PERIOD_DUTY;
            break;
        case (HGPWM_V0_FUNC_CMD_REQUEST_IRQ_COMPARE):
            return HGTIMER_V7_PWM_FUNC_CMD_REQUEST_IRQ_COMPARE;
            break;
        case (HGPWM_V0_FUNC_CMD_REQUEST_IRQ_PERIOD):
            return HGTIMER_V7_PWM_FUNC_CMD_REQUEST_IRQ_PERIOD;
            break;
        case (HGPWM_V0_FUNC_CMD_RELEASE_IRQ):
            return HGTIMER_V7_PWM_FUNC_CMD_RELEASE_IRQ;
            break;
        case (HGPWM_V0_FUNC_CMD_SUSPEND):
            return HGTIMER_V7_PWM_FUNC_CMD_SUSPEND;
            break;
        case (HGPWM_V0_FUNC_CMD_RESUME):
            return HGTIMER_V7_PWM_FUNC_CMD_RESUME;
            break;
        case (HGPWM_V0_FUNC_CMD_IOCTL_SET_PRESCALER):
            return HGTIMER_V7_PWM_FUNC_CMD_IOCTL_SET_PRESCALER;
            break;
        case (HGPWM_V0_FUNC_CMD_IOCTL_SET_PERIOD_DUTY_IMMEDIATELY):
            return HGTIMER_V7_PWM_FUNC_CMD_IOCTL_SET_PERIOD_DUTY_IMMEDIATELY;
            break;
        default:
            return -1;
            break;
    }
}

static inline int32 hgtimer_v7_pwm_func_init(struct hgtimer_v7 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if (dev->opened) {
        if (!dev->dsleep) {
            return -EBUSY;
        }
    }

    /*
     * open TIMER clk
     */
    hgtimer_v7_clk_gating(hw, 1);

    if (pin_func(dev->dev.dev.dev_id, 1) != RET_OK) {
        return RET_ERR;
    }

    if ((p_config->duty < 0) || ((p_config->duty) > (p_config->period))) {
        return RET_ERR;
    }


    hw->TMR_CTL = LL_SIMPLE_TIMER_PSC(0)           |  /*1分频*/
                  LL_SIMPLE_TIMER_INC_SRC_SEL(0x5) |  /*选择系统时钟计数源*/
                  LL_SIMPLE_TIMER_PERIOD_ING       |  /*清周期标志位*/
                  LL_SIMPLE_TIMER_CAP_ING          ;  /*清捕获标志位*/
    hw->TMR_PR  = 0;
    hw->TMR_PWM = 0;
    hw->TMR_CNT = 0;


    /* period */
    /* 5.5ns */
    hw->TMR_PR  = p_config->period;

    /* compare */
    hw->TMR_PWM = p_config->duty;

    /* clear the current cnt  */
    hw->TMR_CNT = 0;

    dev->opened = 1;
    dev->pwm_en = 1;

    return RET_OK;
}

static inline int32 hgtimer_v7_pwm_func_deinit(struct hgtimer_v7 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->opened) || (!dev->pwm_en)) {
        return RET_OK;
    }

    hgtimer_v7_clk_gating(hw, 0);

    irq_disable(dev->irq_num);
    pin_func(dev->dev.dev.dev_id, 0);

    hw->TMR_CTL   = 0;
    hw->TMR_PR    = 0;
    hw->TMR_PWM   = 0;
    hw->TMR_CNT   = 0;
    dev->opened   = 0;
    dev->pwm_en   = 0;

    return RET_OK;
}

static inline int32 hgtimer_v7_pwm_func_start(struct hgtimer_v7 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_ERR;
    }

    hw->TMR_CNT = 0;
    hw->TMR_CTL = (hw->TMR_CTL &~ (LL_SIMPLE_TIMER_MODE_SEL(0x3))) | LL_SIMPLE_TIMER_MODE_SEL(0x2);

    return RET_OK;
}

static inline int32 hgtimer_v7_pwm_func_stop(struct hgtimer_v7 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    hw->TMR_CTL &= ~ LL_SIMPLE_TIMER_MODE_SEL(0x3);


    return RET_OK;
}

static inline int32 hgtimer_v7_pwm_func_ioctl_set_period_duty(struct hgtimer_v7 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    if ((p_config->duty < 0) || ((p_config->duty) > (p_config->period))) {
        return RET_ERR;
    }

    /* period */
    hw->TMR_PR  = p_config->period;

    /* compare */
    hw->TMR_PWM = p_config->duty;

    return RET_OK;
}

static inline int32 hgtimer_v7_pwm_func_cmd_ioctl_set_prescaler(struct hgtimer_v7 *dev, struct hgpwm_v0_config *p_config)
{
    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_ERR;
    }

	hw->TMR_CTL = (hw->TMR_CTL &~ (LL_SIMPLE_TIMER_PSC(0x7))) | (LL_SIMPLE_TIMER_PSC(0x7));

	return RET_OK;
}

static inline int32 hgtimer_v7_pwm_func_cmd_ioctl_set_period_duty_immediately(struct hgtimer_v7 *dev, struct hgpwm_v0_config *p_config)
{
    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_ERR;
    }

	hw->TMR_CTL &= ~ LL_SIMPLE_TIMER_MODE_SEL(0x3);

	hw->TMR_CNT = 0;
	hw->TMR_PR  = p_config->period;
	hw->TMR_PWM = p_config->duty;

	hw->TMR_CTL = (hw->TMR_CTL &~ (LL_SIMPLE_TIMER_MODE_SEL(0x3))) | LL_SIMPLE_TIMER_MODE_SEL(0x2);

	return RET_OK;
}

static inline int32 hgtimer_v7_pwm_func_request_irq_compare(struct hgtimer_v7 *dev, struct hgpwm_v0_config *p_config)
{

    return RET_OK;
}

static inline int32 hgtimer_v7_pwm_func_request_irq_period(struct hgtimer_v7 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    dev->_pwm_irq_hdl = p_config->irq_hdl;
    dev->irq_data     = p_config->irq_data;
    hw->TMR_CTL      |= LL_SIMPLE_TIMER_PERIOD_IE;
    irq_enable(dev->irq_num);

    return RET_OK;
}

static inline int32 hgtimer_v7_pwm_func_release_irq(struct hgtimer_v7 *dev, struct hgpwm_v0_config *p_config)
{

    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    irq_disable(dev->irq_num);
    dev->_pwm_irq_hdl = NULL;
    dev->irq_data     = 0;
    hw->TMR_CTL      &= ~ LL_SIMPLE_TIMER_PERIOD_IE;

    return RET_OK;
}

#ifdef CONFIG_SLEEP
static inline int32 hgtimer_v7_pwm_func_suspend(struct hgtimer_v7 *dev, struct hgpwm_v0_config *p_config)
{
    //struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_OK;
    }

    hgtimer_v7_suspend((void *)dev);

    return RET_OK;
}

static inline int32 hgtimer_v7_pwm_func_resume(struct hgtimer_v7 *dev, struct hgpwm_v0_config *p_config)
{
    //struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->pwm_en) || (dev->dsleep)) {
        return RET_OK;
    }

    hgtimer_v7_resume((void *)dev);

    return RET_OK;
}
#endif

static int32 hgtimer_v7_pwm_config(struct hgtimer_v7 *dev, uint32 config, uint32 param)
{

    struct hgpwm_v0_config *p_config;
    int32  hgtimer_v7_pwm_func_cmd = 0;
    int32  ret_val = RET_OK;

    /* Make sure the config struct pointer */
    if (!config) {
        return -EINVAL;
    }

    p_config = (struct hgpwm_v0_config *)config;
    hgtimer_v7_pwm_func_cmd = hgtimer_v7_pwm_switch_func_cmd(p_config->func_cmd);
    if ((-1) == hgtimer_v7_pwm_func_cmd) {
        return -EINVAL;
    }

    switch (hgtimer_v7_pwm_func_cmd) {
        case (HGTIMER_V7_PWM_FUNC_CMD_INIT):
            ret_val = hgtimer_v7_pwm_func_init(dev, p_config);
            break;
        case (HGTIMER_V7_PWM_FUNC_CMD_DEINIT):
            ret_val = hgtimer_v7_pwm_func_deinit(dev, p_config);
            break;
        case (HGTIMER_V7_PWM_FUNC_CMD_START):
            ret_val = hgtimer_v7_pwm_func_start(dev, p_config);
            break;
        case (HGTIMER_V7_PWM_FUNC_CMD_STOP):
            ret_val = hgtimer_v7_pwm_func_stop(dev, p_config);
            break;
        case (HGTIMER_V7_PWM_FUNC_CMD_IOCTL_SET_PERIOD_DUTY):
            ret_val = hgtimer_v7_pwm_func_ioctl_set_period_duty(dev, p_config);
            break;
        case (HGTIMER_V7_PWM_FUNC_CMD_REQUEST_IRQ_COMPARE):
            ret_val = hgtimer_v7_pwm_func_request_irq_compare(dev, p_config);
            break;
        case (HGTIMER_V7_PWM_FUNC_CMD_REQUEST_IRQ_PERIOD):
            ret_val = hgtimer_v7_pwm_func_request_irq_period(dev, p_config);
            break;
        case (HGTIMER_V7_PWM_FUNC_CMD_RELEASE_IRQ):
            ret_val = hgtimer_v7_pwm_func_release_irq(dev, p_config);
            break;
        case (HGTIMER_V7_PWM_FUNC_CMD_IOCTL_SET_PRESCALER):
            ret_val = hgtimer_v7_pwm_func_cmd_ioctl_set_prescaler(dev, p_config);
            break;
        case (HGTIMER_V7_PWM_FUNC_CMD_IOCTL_SET_PERIOD_DUTY_IMMEDIATELY):
            ret_val = hgtimer_v7_pwm_func_cmd_ioctl_set_period_duty_immediately(dev, p_config);
            break;
#ifdef CONFIG_SLEEP
        case (HGTIMER_V7_PWM_FUNC_CMD_SUSPEND):
            ret_val = hgtimer_v7_pwm_func_suspend(dev, p_config);
            break;
        case (HGTIMER_V7_PWM_FUNC_CMD_RESUME):
            ret_val = hgtimer_v7_pwm_func_resume(dev, p_config);
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
static int32 hgtimer_v7_capture_switch_func_cmd(enum hgcapture_v0_func_cmd param)
{

    switch (param) {
        case (HGCAPTURE_V0_FUNC_CMD_INIT):
            return HGTIMER_V7_CAPTURE_FUNC_CMD_INIT;
            break;
        case (HGCAPTURE_V0_FUNC_CMD_DEINIT):
            return HGTIMER_V7_CAPTURE_FUNC_CMD_DEINIT;
            break;
        case (HGCAPTURE_V0_FUNC_CMD_START):
            return HGTIMER_V7_CAPTURE_FUNC_CMD_START;
            break;
        case (HGCAPTURE_V0_FUNC_CMD_STOP):
            return HGTIMER_V7_CAPTURE_FUNC_CMD_STOP;
            break;
        case (HGCAPTURE_V0_FUNC_CMD_REQUEST_IRQ_CAPTURE):
            return HGTIMER_V7_CAPTURE_FUNC_CMD_REQUEST_IRQ_CAPTURE;
            break;
        case (HGCAPTURE_V0_FUNC_CMD_REQUEST_IRQ_OVERFLOW):
            return HGTIMER_V7_CAPTURE_FUNC_CMD_REQUEST_IRQ_OVERFLOW;
            break;
        case (HGCAPTURE_V0_FUNC_CMD_RELEASE_IRQ):
            return HGTIMER_V7_CAPTURE_FUNC_CMD_RELEASE_IRQ;
            break;
        case (HGCAPTURE_V0_FUNC_CMD_SUSPEND):
            return HGTIMER_V7_CAPTURE_FUNC_CMD_SUSPEND;
            break;
        case (HGCAPTURE_V0_FUNC_CMD_RESUME):
            return HGTIMER_V7_CAPTURE_FUNC_CMD_RESUME;
            break;
        default:
            return -1;
            break;
    }
}

static inline int32 hgtimer_v7_capture_func_init(struct hgtimer_v7 *dev, struct hgcapture_v0_config *p_config)
{

    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if (dev->opened) {
        if (!dev->dsleep) {
            return -EBUSY;
        }
    }

    hgtimer_v7_clk_gating(hw, 1);

    if (pin_func(dev->dev.dev.dev_id, 2) != RET_OK) {
        return RET_ERR;
    }

    hw->TMR_CTL = LL_SIMPLE_TIMER_PSC(0)                                              |   //1分频
                  LL_SIMPLE_TIMER_INC_SRC_SEL(0x5)                                    |   //选择系统时钟计数源
                  LL_SIMPLE_TIMER_CAP_SEL(0)                                          |   //默认选择IO捕获
                  LL_SIMPLE_TIMER_EDG_SCL((p_config->cap_sel<<1) | p_config->cap_pol) |   //选择捕获边沿
                  LL_SIMPLE_TIMER_PERIOD_ING                                          |   //清周期标志位
                  LL_SIMPLE_TIMER_CAP_ING                                             ;    //清捕获标志位
    hw->TMR_PR  = 0;
    hw->TMR_PWM = 0;
    hw->TMR_CNT = 0;
    

    dev->opened = 1;
    dev->cap_en = 1;

    return RET_OK;
}

static inline int32 hgtimer_v7_capture_func_deinit(struct hgtimer_v7 *dev, struct hgcapture_v0_config *p_config)
{

    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->opened) || (!dev->pwm_en)) {
        return RET_OK;
    }

    hgtimer_v7_clk_gating(hw, 0);

    irq_disable(dev->irq_num);
    pin_func(dev->dev.dev.dev_id, 0);

    hw->TMR_CTL   = 0;
    hw->TMR_PR    = 0;
    hw->TMR_PWM   = 0;
    hw->TMR_CNT   = 0;
    dev->opened   = 0;
    dev->cap_en   = 0;

    return RET_OK;
}

static inline int32 hgtimer_v7_capture_func_start(struct hgtimer_v7 *dev, struct hgcapture_v0_config *p_config)
{

    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->cap_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    hw->TMR_CTL = (hw->TMR_CTL &~ (LL_SIMPLE_TIMER_MODE_SEL(0x3))) | LL_SIMPLE_TIMER_MODE_SEL(0x3);

    return RET_OK;
}

static inline int32 hgtimer_v7_capture_func_stop(struct hgtimer_v7 *dev, struct hgcapture_v0_config *p_config)
{

    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->cap_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    hw->TMR_CTL &= ~ LL_SIMPLE_TIMER_MODE_SEL(0x3);

    return RET_OK;
}

static inline int32 hgtimer_v7_capture_func_request_irq_capture(struct hgtimer_v7 *dev, struct hgcapture_v0_config *p_config)
{

    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->cap_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    dev->_capture_irq_hdl = p_config->irq_hdl;
    dev->irq_data         = p_config->irq_data;
    hw->TMR_CTL          |= LL_SIMPLE_TIMER_CAP_IE;
    irq_enable(dev->irq_num);

    return RET_OK;
}

static inline int32 hgtimer_v7_capture_func_request_irq_overflow(struct hgtimer_v7 *dev, struct hgcapture_v0_config *p_config)
{
    return RET_OK;
}

static inline int32 hgtimer_v7_capture_func_release_irq(struct hgtimer_v7 *dev, struct hgcapture_v0_config *p_config)
{

    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->cap_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    irq_disable(dev->irq_num);
    dev->_capture_irq_hdl = NULL;
    dev->irq_data         = 0;
    hw->TMR_CTL          &= ~ LL_SIMPLE_TIMER_CAP_IE;

    return RET_OK;
}

#ifdef CONFIG_SLEEP
static inline int32 hgtimer_v7_capture_func_suspend(struct hgtimer_v7 *dev, struct hgcapture_v0_config *p_config)
{
    //struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->cap_en) || (dev->dsleep)) {
        return RET_OK;
    }

    hgtimer_v7_suspend((void *)dev);

    return RET_OK;
}

static inline int32 hgtimer_v7_capture_func_resume(struct hgtimer_v7 *dev, struct hgcapture_v0_config *p_config)
{
    //struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->cap_en) || (dev->dsleep)) {
        return RET_OK;
    }

    hgtimer_v7_resume((void *)dev);

    return RET_OK;
}
#endif

static int32 hgtimer_v7_capture_config(struct hgtimer_v7 *dev, uint32 config, uint32 param)
{

    struct hgcapture_v0_config *p_config;
    int32  hgtimer_v7_cap_func_cmd = 0;
    int32  ret_val = RET_OK;

    /* Make sure the config struct pointer */
    if (!config) {
        return -EINVAL;
    }

    p_config = (struct hgcapture_v0_config *)config;
    hgtimer_v7_cap_func_cmd = hgtimer_v7_capture_switch_func_cmd(p_config->func_cmd);
    if ((-1) == hgtimer_v7_cap_func_cmd) {
        return -EINVAL;
    }

    switch (hgtimer_v7_cap_func_cmd) {
        case (HGTIMER_V7_CAPTURE_FUNC_CMD_INIT):
            ret_val = hgtimer_v7_capture_func_init(dev, p_config);
            break;
        case (HGTIMER_V7_CAPTURE_FUNC_CMD_DEINIT):
            ret_val = hgtimer_v7_capture_func_deinit(dev, p_config);
            break;
        case (HGTIMER_V7_CAPTURE_FUNC_CMD_START):
            ret_val = hgtimer_v7_capture_func_start(dev, p_config);
            break;
        case (HGTIMER_V7_CAPTURE_FUNC_CMD_STOP):
            ret_val = hgtimer_v7_capture_func_stop(dev, p_config);
            break;
        case (HGTIMER_V7_CAPTURE_FUNC_CMD_REQUEST_IRQ_CAPTURE):
            ret_val = hgtimer_v7_capture_func_request_irq_capture(dev, p_config);
            break;
        case (HGTIMER_V7_CAPTURE_FUNC_CMD_REQUEST_IRQ_OVERFLOW):
            ret_val = hgtimer_v7_capture_func_request_irq_overflow(dev, p_config);
            break;
        case (HGTIMER_V7_CAPTURE_FUNC_CMD_RELEASE_IRQ):
            ret_val = hgtimer_v7_capture_func_release_irq(dev, p_config);
            break;
#ifdef CONFIG_SLEEP
        case (HGTIMER_V7_CAPTURE_FUNC_CMD_SUSPEND):
            ret_val = hgtimer_v7_capture_func_suspend(dev, p_config);
            break;
        case (HGTIMER_V7_CAPTURE_FUNC_CMD_RESUME):
            ret_val = hgtimer_v7_capture_func_resume(dev, p_config);
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
static int32 hgtimer_v7_counter_switch_hal_type(struct hgtimer_v7 *dev, enum timer_type param)
{

    switch (param) {
        case (TIMER_TYPE_ONCE):
            dev->type = HGTIMER_V7_TYPE_ONCE;
            return 0;
            break;
        case (TIMER_TYPE_PERIODIC):
            dev->type = HGTIMER_V7_TYPE_PERIODIC;
            return 1;
            break;
        default:
            return -1;
            break;
    }
}

static int32 hgtimer_v7_counter_set_psc(struct hgtimer_v7 *timer, uint32 psc)
{
    struct hgtimer_v7 *dev = (struct hgtimer_v7 *)timer;
    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if (!dev->counter_en) {
        return RET_ERR;
    }

    hw->TMR_CTL &= ~ LL_SIMPLE_TIMER_MODE_SEL(0x3);
    hw->TMR_CTL  = (hw->TMR_CTL &~ LL_SIMPLE_TIMER_PSC(0x7)) | LL_SIMPLE_TIMER_PSC(psc);
    hw->TMR_CNT = 0;

    return RET_OK;
}

static int32 hgtimer_v7_counter_set_slavemode(struct hgtimer_v7 *timer, uint32 slavemode)
{
    return RET_OK;
}

static int32 hgtimer_v7_counter_func_open(struct timer_device *timer, enum timer_type type, uint32 flags)
{

    struct hgtimer_v7 *dev = (struct hgtimer_v7 *)timer;
    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if (dev->opened) {
        if (!dev->dsleep) {
            return -EBUSY;
        }
    }


    if ((-1) == hgtimer_v7_counter_switch_hal_type(dev, type)) {
        return RET_ERR;
    }

    hgtimer_v7_clk_gating(hw, 1);


    /* clear reg */
    hw->TMR_CTL  = 0;
    hw->TMR_CNT = 0;
    hw->TMR_PR  = 0;
    hw->TMR_PWM = 0;



    irq_enable(dev->irq_num);

    switch (dev->type) {
        /* TIMER_TYPE_ONCE */
        case (HGTIMER_V7_TYPE_ONCE):

        /* TIMER_TYPE_PERIODIC */
        case (HGTIMER_V7_TYPE_PERIODIC):

        /* TIMER_TYPE_COUNTER */
        case (HGTIMER_V7_TYPE_COUNTER):
            hw->TMR_CTL = LL_SIMPLE_TIMER_PSC(0)           |  //1分频
                          LL_SIMPLE_TIMER_INC_SRC_SEL(0x5) |  //选择系统时钟计数源
                          LL_SIMPLE_TIMER_PERIOD_ING       |  //清周期标志位
                          LL_SIMPLE_TIMER_CAP_ING          ;  //清捕获标志位
            hw->TMR_PR  = 0;
            hw->TMR_PWM = 0;
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

static int32 hgtimer_v7_counter_func_close(struct timer_device *timer)
{

    struct hgtimer_v7 *dev = (struct hgtimer_v7 *)timer;
    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->opened) || (!dev->counter_en)) {
        return RET_OK;
    }

    hgtimer_v7_clk_gating(hw, 0);

    irq_disable(dev->irq_num);

    hw->TMR_CTL       = 0;
    hw->TMR_PR        = 0;
    hw->TMR_PWM       = 0;
    hw->TMR_CNT       = 0;
    dev->opened       = 0;
    dev->counter_en   = 0;
    dev->dsleep       = 0;

    return RET_OK;
}

static int32 hgtimer_v7_counter_func_start(struct timer_device *timer, uint32 period_sysclkpd_cnt, timer_irq_hdl cb, uint32 cb_data)
{

    struct hgtimer_v7 *dev = (struct hgtimer_v7 *)timer;
    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->counter_en) || (dev->dsleep)) {
        return RET_ERR;
    }


    switch (dev->type) {

        /* TIMER_TYPE_ONCE */
        case (HGTIMER_V7_TYPE_ONCE):

            dev->counter_once_en = 1;

            /* config period */
            hw->TMR_PR  = period_sysclkpd_cnt;
            hw->TMR_PWM = 0;

            hw->TMR_CNT  = 0;

            if (cb) {
                dev->_counter_irq_hdl = cb;
                dev->irq_data         = cb_data;
                hw->TMR_CTL          |= LL_SIMPLE_TIMER_PERIOD_IE;
            }

            hw->TMR_CTL = (hw->TMR_CTL &~ (LL_SIMPLE_TIMER_MODE_SEL(0x3))) | LL_SIMPLE_TIMER_MODE_SEL(0x1);
            break;

        /* TIMER_TYPE_PERIODIC */
        case (HGTIMER_V7_TYPE_PERIODIC):

        /* TIMER_TYPE_COUNTER */
        case (HGTIMER_V7_TYPE_COUNTER):

            dev->counter_period_en = 1;

            /* config period */
            hw->TMR_PR  = period_sysclkpd_cnt;
            hw->TMR_PWM = 0;

            hw->TMR_CNT  = 0;

            if (cb) {
                dev->_counter_irq_hdl = cb;
                dev->irq_data         = cb_data;
                hw->TMR_CTL          |= LL_SIMPLE_TIMER_PERIOD_IE;
            }

            hw->TMR_CTL = (hw->TMR_CTL &~ (LL_SIMPLE_TIMER_MODE_SEL(0x3))) | LL_SIMPLE_TIMER_MODE_SEL(0x1);
            break;
        default:
            return RET_ERR;
            break;
    }

    return RET_OK;
}

static int32 hgtimer_v7_counter_func_stop(struct timer_device *timer)
{

    struct hgtimer_v7 *dev = (struct hgtimer_v7 *)timer;
    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->counter_en) || (dev->dsleep)) {
        return RET_ERR;
    }

    hw->TMR_CTL &= ~ LL_SIMPLE_TIMER_MODE_SEL(0x3);

    return RET_OK;
}

#ifdef CONFIG_SLEEP
static int32 hgtimer_v7_counter_func_suspend(struct dev_obj *obj)
{
    struct hgtimer_v7 *dev = (struct hgtimer_v7 *)obj;
    //struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->counter_en) || (dev->dsleep)) {
        return RET_OK;
    }

    hgtimer_v7_suspend((void *)dev);
    return RET_OK;
}

static int32 hgtimer_v7_counter_func_resume(struct dev_obj *obj)
{
    struct hgtimer_v7 *dev = (struct hgtimer_v7 *)obj;
    //struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;

    if ((!dev->counter_en) || (!dev->dsleep)) {
        return RET_OK;
    }

    hgtimer_v7_resume((void *)dev);
    return RET_OK;
}
#endif


static int32 hgtimer_v7_counter_func_ioctl(struct timer_device *timer, uint32 cmd, uint32 param1, uint32 param2)
{

    int32  ret_val = RET_OK;
    struct hgtimer_v7 *dev = (struct hgtimer_v7 *)timer;

    switch (cmd) {
        case (HGPWM_V0_FUNC_CMD_MASK):
            ret_val = hgtimer_v7_pwm_config(dev, param1, 0);
            break;
        case (HGCAPTURE_V0_FUNC_CMD_MASK):
            ret_val = hgtimer_v7_capture_config(dev, param1, 0);
            break;
        case (TIMER_SET_CLK_PSC):
            ret_val = hgtimer_v7_counter_set_psc(dev, param1);
            break;
        case (TIMER_SET_SLAVE_MODE):
            ret_val = hgtimer_v7_counter_set_slavemode(dev, param1);
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

static void hgtimer_v7_irq_handler(void *data)
{

    struct hgtimer_v7 *dev = (struct hgtimer_v7 *)data;
    struct hgtimer_v7_hw *hw = (struct hgtimer_v7_hw *)dev->hw;


    if ((hw->TMR_CTL & LL_SIMPLE_TIMER_PERIOD_IE) && (hw->TMR_CTL & LL_SIMPLE_TIMER_PERIOD_ING)) {
        /* clear interrupt flag */
        hw->TMR_CTL |= LL_SIMPLE_TIMER_PERIOD_ING;

        /* pwm mode irq */
        if (dev->opened && dev->pwm_en && dev->_pwm_irq_hdl) {
            dev->_pwm_irq_hdl(PWM_IRQ_FLAG_PERIOD, dev->irq_data);
        }

        /* counter mode irq */
        if (dev->opened && dev->counter_en) {
            if (dev->counter_once_en) {
                dev->counter_once_en = 0;

                /* close timer & interrupt */
                hw->TMR_CTL &= ~ LL_SIMPLE_TIMER_MODE_SEL(0x3);
                hw->TMR_CTL &= ~ LL_SIMPLE_TIMER_PERIOD_IE;
            }
            if (dev->_counter_irq_hdl) {
                dev->_counter_irq_hdl(dev->irq_data, TIMER_IRQ_FLAG_TIMEOUT);
            }
        }
    }


    if ((hw->TMR_CTL & LL_SIMPLE_TIMER_CAP_IE) && (hw->TMR_CTL & LL_SIMPLE_TIMER_CAP_ING)) {
        /* clear interrupt flag */
        hw->TMR_CTL |= LL_SIMPLE_TIMER_CAP_ING;

        /* capture mode irq */
        if (dev->opened && dev->cap_en && dev->_capture_irq_hdl) {
            dev->_capture_irq_hdl(CAPTURE_IRQ_FLAG_CAPTURE, hw->TMR_CNT);
        }
    }
}

static const struct timer_hal_ops timer_v7_ops = {
    .open         = hgtimer_v7_counter_func_open,
    .close        = hgtimer_v7_counter_func_close,
    .start        = hgtimer_v7_counter_func_start,
    .stop         = hgtimer_v7_counter_func_stop,
    .ioctl        = hgtimer_v7_counter_func_ioctl,
#ifdef CONFIG_SLEEP
    .ops.suspend  = hgtimer_v7_suspend,
    .ops.resume   = hgtimer_v7_resume,
#endif
};


int32 hgtimer_v7_attach(uint32 dev_id, struct hgtimer_v7 *timer)
{
    timer->opened           = 0;
    timer->dsleep           = 0;
    timer->_counter_irq_hdl = NULL;
    timer->_pwm_irq_hdl     = NULL;
    timer->_capture_irq_hdl = NULL;
    timer->irq_data         = 0;
    timer->dev.dev.ops      = (const struct devobj_ops *)&timer_v7_ops;
#ifdef CONFIG_SLEEP
    os_mutex_init(&timer->bp_suspend_lock);
    os_mutex_init(&timer->bp_resume_lock);
#endif
    request_irq(timer->irq_num, hgtimer_v7_irq_handler, timer);
    dev_register(dev_id, (struct dev_obj *)timer);

    return RET_OK;
}

