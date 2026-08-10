/**
  ******************************************************************************
  * @file       sdk/hal/gpio.h
  * @author     HUGE-IC Application Team
  * @version    V1.0.0
  * @date       2022-01-11
  * @brief      This file contains all the GPIO HAL functions.
  * @copyright  Copyright (c) 2016-2022 HUGE-IC
  ******************************************************************************
  * @attention
  * None
  *
  *
  *
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "dev.h"
#include "hal/gpio.h"

/** @defgroup Docxygenid_GPIO_Driver GPIO Driver
  * @brief    Mainly the driver part of the GPIO module
  * @{
  */


/** @defgroup Docxygenid_GPIO_function GPIO function
  * @ingroup  Docxygenid_GPIO_Driver
  * @brief    GPIO Initialization And Configuration function
  * @{
  */


/**
  * @brief  Set the gpio pin mode.
  * @param  pin   : which pin to set.\n
  *                 This parameter can be Px_y where x can be (A..C) and y can be (0..15)\n
  *                 But the value of the y only be (0..5) when x is C.
  * @param  mode  : Configure gpio_pin_mode, reference @ref gpio_pin_mode.
  * @param  level : Configure gpio_pull_level, reference @ref gpio_private_pull_level.
  * @return
  *         - RET_OK  : GPIO set mode successfully.
  *         - RET_ERR : GPIO set mode unsuccessfully.
  */
int32 gpio_set_mode(uint32 pin, enum gpio_pin_mode mode, enum gpio_pull_level level)
{
    struct gpio_device *gpio = gpio_get(pin);
    if (gpio && ((const struct gpio_hal_ops *)gpio->dev.ops)->mode) {
        return ((const struct gpio_hal_ops *)gpio->dev.ops)->mode(gpio, pin, mode, level);
    }

    return RET_ERR;
}

/**
  * @brief  Set the gpio pin direction.
  * @param  pin       : which pin to set.\n
  *                     This parameter can be Px_y where x can be (A..C) and y can be (0..15)\n
  *                     But the value of the y only be (0..5) when x is C.
  * @param  direction : Configure gpio pin direction, reference @ref gpio_pin_direction.
  * @return
  *         - RET_OK  : GPIO set direction successfully.
  *         - RET_ERR : GPIO set direction unsuccessfully.
  */
int32 gpio_set_dir(uint32 pin, enum gpio_pin_direction direction)
{
    struct gpio_device *gpio = gpio_get(pin);
    if (gpio && ((const struct gpio_hal_ops *)gpio->dev.ops)->dir) {
        return ((const struct gpio_hal_ops *)gpio->dev.ops)->dir(gpio, pin, direction);
    }
    return RET_ERR;
}


/**
  * @brief  Set the gpio pin value.
  * @param  pin       : which pin to set.\n
  *                     This parameter can be Px_y where x can be (A..C) and y can be (0..15)\n
  *                     But the value of the y only be (0..5) when x is C.
  * @param  direction : Configure gpio pin value, it can be 0 / 1.
  * @return
  *         - RET_OK  : GPIO pin set value successfully.
  *         - RET_ERR : GPIO pin set value unsuccessfully.
  */
int32 gpio_set_val(uint32 pin, uint32 value)
{
    struct gpio_device *gpio = gpio_get(pin);
    if (gpio && ((const struct gpio_hal_ops *)gpio->dev.ops)->set) {
        return ((const struct gpio_hal_ops *)gpio->dev.ops)->set(gpio, pin, value);
    }
    return RET_ERR;
}


/**
  * @brief  Get the gpio pin value.
  * @param  pin       : which pin to set.\n
  *                     This parameter can be Px_y where x can be (A..C) and y can be (0..15)\n
  *                     But the value of the y only be (0..5) when x is C.
  * @return
  *         - value   : GPIO pin value, it can be 0 / 1.
  *         - RET_ERR : Get gpio pin value unsuccessfully.
  */
int32 gpio_get_val(uint32 pin)
{
    struct gpio_device *gpio = gpio_get(pin);
    if (gpio && ((const struct gpio_hal_ops *)gpio->dev.ops)->get) {
        return ((const struct gpio_hal_ops *)gpio->dev.ops)->get(gpio, pin);
    }
    return RET_ERR;
}


/**
  * @brief  Request the gpio pin interrupt.
  * @param  pin       : which pin to set.\n
  *                     This parameter can be Px_y where x can be (A..C) and y can be (0..15)\n
  *                     But the value of the y only be (0..5) when x is C.
  * @handler          : Interrupt handle, executed after the interrupt is generated.
  * @data             : Parameters for interrupt handler.
  * @evt              : Interrupt type, reference @ref gpio_irq_event.
  * @return
  *         - RET_OK  : GPIO request pin interrupt successfully.
  *         - RET_ERR : GPIO request pin interrupt unsuccessfully.
  */
int32 gpio_request_pin_irq(uint32 pin, gpio_irq_hdl handler, uint32 data, enum gpio_irq_event evt)
{
    struct gpio_device *gpio = gpio_get(pin);
    if (gpio && ((const struct gpio_hal_ops *)gpio->dev.ops)->request_pin_irq) {
        return ((const struct gpio_hal_ops *)gpio->dev.ops)->request_pin_irq(gpio, pin, handler, data, evt);
    }
    return RET_ERR;
}


/**
  * @brief  Release the gpio pin interrupt.
  * @param  pin       : which pin to set.\n
  *                     This parameter can be Px_y where x can be (A..C) and y can be (0..15)\n
  *                     But the value of the y only be (0..5) when x is C.
  * @evt              : Interrupt type, reference @ref gpio_irq_event.
  * @return
  *         - RET_OK  : GPIO release pin interrupt successfully.
  *         - RET_ERR : GPIO release pin interrupt unsuccessfully.
  */
int32 gpio_release_pin_irq(uint32 pin, enum gpio_irq_event evt)
{
    struct gpio_device *gpio = gpio_get(pin);
    if (gpio && ((const struct gpio_hal_ops *)gpio->dev.ops)->release_pin_irq) {
        return ((const struct gpio_hal_ops *)gpio->dev.ops)->release_pin_irq(gpio, pin, evt);
    }
    return RET_ERR;
}


/**
  * @brief  Configure the GPIO module by ioctl_cmd.
  * @param  pin       : which pin to set.\n
  *                     This parameter can be Px_y where x can be (A..C) and y can be (0..15)\n
  *                     But the value of the y only be (0..5) when x is C.
  * @cmd              : Commands to configure the GPIO module, reference @ref gpio_ioctl_cmd.
  * @param1           : Parameter 1, according to the command.
  * @param2           : Parameter 2, according to the command.
  * @return
  *         - RET_OK  : GPIO module configure successfully.
  *         - RET_ERR : GPIO module configure unsuccessfully.
  */
int32 gpio_ioctl(uint32 pin, uint32 cmd, uint32 param1, uint32 param2)
{
    struct gpio_device *gpio = gpio_get(pin);
    if (gpio && ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl) {
        return ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl(gpio, pin, cmd, param1, param2);
    }
    return RET_ERR;
}


/** @} Docxygenid_GPIO_functions*/
/** @} Docxygenid_GPIO_Driver*/



/*************************** (C) COPYRIGHT 2016-2022 HUGE-IC ***** END OF FILE *****/
