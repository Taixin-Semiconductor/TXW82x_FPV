#ifndef _HGTIMER_V7_H
#define _HGTIMER_V7_H
#include "hal/timer_device.h"

#ifdef __cplusplus
extern "C" {
#endif


/*-----timer lower layer: counter mode-----*/
#define HGTIMER_V7_TYPE_ONCE       BIT(0)
#define HGTIMER_V7_TYPE_PERIODIC   BIT(1)
#define HGTIMER_V7_TYPE_COUNTER    BIT(2)


/*----timer lower layer: PWM function cmd----*/
#define HGTIMER_V7_PWM_FUNC_CMD_INIT                   				HGPWM_V0_FUNC_CMD_INIT
#define HGTIMER_V7_PWM_FUNC_CMD_DEINIT                 				HGPWM_V0_FUNC_CMD_DEINIT
#define HGTIMER_V7_PWM_FUNC_CMD_START                  				HGPWM_V0_FUNC_CMD_START
#define HGTIMER_V7_PWM_FUNC_CMD_STOP                   				HGPWM_V0_FUNC_CMD_STOP
/* IOCTL CMD */                                        
#define HGTIMER_V7_PWM_FUNC_CMD_IOCTL_SET_PERIOD_DUTY  				HGPWM_V0_FUNC_CMD_IOCTL_SET_PERIOD_DUTY
#define HGTIMER_V7_PWM_FUNC_CMD_IOCTL_GET_PERIOD_DUTY  				HGPWM_V0_FUNC_CMD_IOCTL_GET_PERIOD_DUTY
#define HGTIMER_V7_PWM_FUNC_CMD_IOCTL_SET_PRESCALER        			HGPWM_V0_FUNC_CMD_IOCTL_SET_PRESCALER
#define HGTIMER_V7_PWM_FUNC_CMD_IOCTL_SET_PERIOD_DUTY_IMMEDIATELY   HGPWM_V0_FUNC_CMD_IOCTL_SET_PERIOD_DUTY_IMMEDIATELY

/* IRQ_FLAG */                                         
#define HGTIMER_V7_PWM_FUNC_CMD_REQUEST_IRQ_COMPARE    				HGPWM_V0_FUNC_CMD_REQUEST_IRQ_COMPARE
#define HGTIMER_V7_PWM_FUNC_CMD_REQUEST_IRQ_PERIOD     				HGPWM_V0_FUNC_CMD_REQUEST_IRQ_PERIOD
	                                                       
#define HGTIMER_V7_PWM_FUNC_CMD_RELEASE_IRQ            				HGPWM_V0_FUNC_CMD_RELEASE_IRQ

#define HGTIMER_V7_PWM_FUNC_CMD_SUSPEND                				HGPWM_V0_FUNC_CMD_SUSPEND
#define HGTIMER_V7_PWM_FUNC_CMD_RESUME                 				HGPWM_V0_FUNC_CMD_RESUME






/*----timer lower layer: cap function cmd----*/
#define HGTIMER_V7_CAPTURE_FUNC_CMD_INIT                   HGCAPTURE_V0_FUNC_CMD_INIT
#define HGTIMER_V7_CAPTURE_FUNC_CMD_DEINIT                 HGCAPTURE_V0_FUNC_CMD_DEINIT
#define HGTIMER_V7_CAPTURE_FUNC_CMD_START                  HGCAPTURE_V0_FUNC_CMD_START
#define HGTIMER_V7_CAPTURE_FUNC_CMD_STOP                   HGCAPTURE_V0_FUNC_CMD_STOP
/* IOCTL CMD */                                        

/* IRQ_FLAG */                                         
#define HGTIMER_V7_CAPTURE_FUNC_CMD_REQUEST_IRQ_CAPTURE    HGCAPTURE_V0_FUNC_CMD_REQUEST_IRQ_CAPTURE
#define HGTIMER_V7_CAPTURE_FUNC_CMD_REQUEST_IRQ_OVERFLOW   HGCAPTURE_V0_FUNC_CMD_REQUEST_IRQ_OVERFLOW
                                                       
#define HGTIMER_V7_CAPTURE_FUNC_CMD_RELEASE_IRQ            HGCAPTURE_V0_FUNC_CMD_RELEASE_IRQ

#define HGTIMER_V7_CAPTURE_FUNC_CMD_SUSPEND                HGCAPTURE_V0_FUNC_CMD_SUSPEND
#define HGTIMER_V7_CAPTURE_FUNC_CMD_RESUME                 HGCAPTURE_V0_FUNC_CMD_RESUME 




struct hgtimer_v7 {
    struct timer_device   dev;
    uint32                hw;
    timer_irq_hdl          _counter_irq_hdl;
    pwm_irq_hdl           _pwm_irq_hdl;
    capture_irq_hdl       _capture_irq_hdl;
    uint32                irq_data;
    uint32                irq_num;
    uint16                type;
    uint16                counter_once_en  :1,
                          counter_period_en:1,
                          counter_en       :1,
                          pwm_en           :1,
                          cap_en           :1,
                          ir_en            :1,
                          opened           :1,
                          dsleep           :1;
#ifdef CONFIG_SLEEP
    struct {
        uint32 tmr_ctl;
        uint32 tmr_cnt;
        uint32 tmr_pr;
        uint32 tmr_pwm;
    }bp_regs;
    uint32               bp_irq_flags;
    timer_irq_hdl         bp_irq_hdl_timer;
    pwm_irq_hdl          bp_irq_hdl_pwm;
    capture_irq_hdl      bp_irq_hdl_capture;
    uint32               bp_irq_data_timer;
    uint32               bp_irq_data_pwm;
    uint32               bp_irq_data_capture;
    os_mutex_t      bp_suspend_lock;
    os_mutex_t      bp_resume_lock;
#endif

};



int32 hgtimer_v7_attach(uint32 dev_id, struct hgtimer_v7 *timer);


#ifdef __cplusplus
}
#endif
#endif

