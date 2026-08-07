#ifndef _HAL_TIMER_DEVICE_H_
#define _HAL_TIMER_DEVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief timer type
  */
enum timer_type {
    TIMER_TYPE_ONCE,

    TIMER_TYPE_PERIODIC,
};

enum timer_ioctl_cmd {
    TIMER_GET_TIMESTAMP,

    TIMER_CALC_TIME_DIFF_US,

    /*
     * configure the prescaler of clk
     */
    TIMER_SET_CLK_PSC,

    /*
     * configure type the slave mode
     */
    TIMER_SET_SLAVE_MODE,

    TIMER_SET_PERIOD,

    TIMER_SET_CLK_SRC,
};

enum timer_irq_flag {
    /*!
     * @brief Interrupt: Timer timeout intterupt.
     * @note
     *
     */
    TIMER_IRQ_FLAG_TIMEOUT   = BIT(0),

    /*!
     * @brief Interrupt: Timer receives has been reset.
     * @note
     *        Possible need to reset parameters.
     */
    TIMER_IRQ_FLAG_RESET     = BIT(1),
};

typedef void (*timer_irq_hdl)(uint32 irq_data, uint32 irq_flag);

struct timer_device {
    struct dev_obj dev;
};

struct timer_hal_ops {
    struct devobj_ops ops;
    int32(*open)(struct timer_device *timer, enum timer_type type, uint32 flags);
    int32(*close)(struct timer_device *timer);
    int32(*start)(struct timer_device *timer, uint32 tmo_us, timer_irq_hdl hdl, uint32 data);
    int32(*stop)(struct timer_device *timer);
    int32(*suspend)(struct timer_device *timer);
    int32(*resume)(struct timer_device *timer);
    int32(*ioctl)(struct timer_device *timer, uint32 cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct timer_device *timer, uint32 irq_flag, timer_irq_hdl hdl, uint32 irq_data);
    int32(*release_irq)(struct timer_device *timer, uint32 irq_flag);
};

int32 timer_device_open(struct timer_device *timer, enum timer_type mode, uint32 flags);
int32 timer_device_close(struct timer_device *timer);
int32 timer_device_start(struct timer_device *timer, uint32 period_sysclkpd_cnt, timer_irq_hdl hdl, uint32 irq_data);
int32 timer_device_stop(struct timer_device *timer);
int32 timer_device_suspend(struct timer_device *timer);
int32 timer_device_resume(struct timer_device *timer);
int32 timer_device_ioctl(struct timer_device *timer, uint32 cmd, uint32 param1, uint32 param2);
int32 timer_request_irq(struct timer_device *timer, uint32 irq_flag, timer_irq_hdl hdl, uint32 irq_data);
int32 timer_release_irq(struct timer_device *timer, uint32 irq_flag);


#ifdef __cplusplus
}
#endif

#endif

