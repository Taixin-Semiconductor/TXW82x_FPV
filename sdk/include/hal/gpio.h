/**
 * @file gpio.h
 * @author HUGE-IC Application Team
 * @brief GPIO HAL driver
 * @version 1.0.0
 * @date 2021-9-8
 *
 * @copyright Copyright (c) 2021 HUGE-IC
 */
#ifndef _HAL_GPIO_H_
#define _HAL_GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif
/**
  * @brief Enumeration constant for GPIO pin direction select
  * @note : this enum must mapping to OS @PinDirection
  */
enum gpio_pin_direction {
    /*! Set GPIO to input mode.
     */
    GPIO_DIR_INPUT,

    /*! Set GPIO to output mode.
     */
    GPIO_DIR_OUTPUT,
};

/**
  * @breif : enum of GPIO pin mode
  * @note : this enum must mapping to OS @PinMode
  */
enum gpio_pin_mode {
    /*! gpio mode : push-pull & pull none
     */
    GPIO_PULL_NONE,

    /*! gpio mode : push-pull & pull up
     */
    GPIO_PULL_UP,

    /*! gpio mode : push-pull & pull down
     */
    GPIO_PULL_DOWN,

    /*! gpio mode : open-drain & pull none
     */
    GPIO_OPENDRAIN_PULL_NONE,

    /*! gpio mode : open-drain & pull up
     */
    GPIO_OPENDRAIN_PULL_UP,

    /*! Set the GPIO drop open-drain mode to NO.
     */
    GPIO_OPENDRAIN_DROP_NONE,

    /*! Set the GPIO drop open-drain mode to down mode.
     */
    GPIO_OPENDRAIN_DROP_DOWN,

};

/**
  * @breif : Enumeration constant for GPIO pin interrupt event.
  */
enum gpio_irq_event {
    /*! None interrupt event.
     */
    GPIO_IRQ_EVENT_NONE,

    /*! Rising interrupt event.
     */
    GPIO_IRQ_EVENT_RISE,

    /*! Falling interrupt event.
     */
    GPIO_IRQ_EVENT_FALL,

    /*! Rising and falling interrupt event.
     */
    GPIO_IRQ_EVENT_ALL,
};


/**
  * @breif : enum of io control cmd
  */
enum gpio_ioctl_cmd {
    /*! switch GPIO input delay on or off
     */
    GPIO_INPUT_DELAY_ON_OFF,

    /*! Atomic operation in GPIO direction.
     */
    GPIO_DIR_ATOMIC,

    /*! Atomic operation in GPIO vaule.
     */
    GPIO_VALUE_ATOMIC,

    /*! LOCK GPIO config exclude I/O DAT register until MCU reset or peripheral reset.
     */
    GPIO_LOCK,

    /*! GPIO debunce
     */
    GPIO_DEBUNCE,

    /*! GPIO toggle, use for output data toggle
     */
    GPIO_OUTPUT_TOGGLE,

    /*! Set GPIO pin to general analog function
     */
    GPIO_GENERAL_ANALOG,

    /*! Set GPIO pin to analog function with disable AIOEN
     */
    GPIO_AIODIS_ANALOG,

    GPIO_CMD_SET_IEEN,
    /*! Get IOMAP O_FUNC according to pin
     */
    GPIO_GET_OUTMAP,

    /*! Get IOMAP I_FUNC according to pin
     */
    GPIO_GET_INMAP,

    /* PARA1:  enum smap_node
    *  PARA2:  enum smap_output/smap_input
    */
    GPIO_SET_SMAP,
    /*! Get pin according to IOMAP O_FUNC
     */
    GPIO_GET_OUTMAP_PIN,
};


typedef void (*gpio_irq_hdl)(int32 id, enum gpio_irq_event event, uint32 param1, uint32 param2);

struct gpio_device {
    struct dev_obj dev;
};

struct gpio_hal_ops {
    struct devobj_ops ops;
    int32(*mode)(struct gpio_device *gpio, uint32 pin, enum gpio_pin_mode mode, enum gpio_pull_level level);
    int32(*dir)(struct gpio_device *gpio, uint32 pin, enum gpio_pin_direction direction);
    int32(*set)(struct gpio_device *gpio, uint32 pin, uint32 val);
    int32(*get)(struct gpio_device *gpio, uint32 pin);
    int32(*request_pin_irq)(struct gpio_device *gpio, uint32 pin, gpio_irq_hdl handler, uint32 data, enum gpio_irq_event evt);
    int32(*release_pin_irq)(struct gpio_device *gpio, uint32 pin, enum gpio_irq_event evt);
    int32(*ioctl)(struct gpio_device *gpio, uint32 pin, uint32 cmd, uint32 param1, uint32 param2);
};

int32 gpio_set_mode(uint32 pin, enum gpio_pin_mode mode, enum gpio_pull_level level);
int32 gpio_set_dir(uint32 pin, enum gpio_pin_direction direction);
int32 gpio_set_val(uint32 pin, uint32 value);
int32 gpio_get_val(uint32 pin);
int32 gpio_request_pin_irq(uint32 pin, gpio_irq_hdl handler, uint32 data, enum gpio_irq_event evt);
int32 gpio_release_pin_irq(uint32 pin, enum gpio_irq_event evt);
int32 gpio_ioctl(uint32 pin, uint32 cmd, uint32 param1, uint32 param2);

extern struct gpio_device *gpio_get(uint32 pin);

#ifdef __cplusplus
}
#endif
#endif

