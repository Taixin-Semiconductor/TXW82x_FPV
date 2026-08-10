/**
  ******************************************************************************
  * @file       sdk/hal/uart.h
  * @author     HUGE-IC Application Team
  * @version    V1.0.0
  * @date       2022-01-11
  * @brief      This file contains all the UART HAL functions.
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
#include "hal/uart.h"

/** @defgroup Docxygenid_UART_Driver UART Driver
  * @brief    Mainly the driver part of the UART module
  * @{
  */


/** @defgroup Docxygenid_UART_functions UART functions
  * @ingroup  Docxygenid_UART_Driver
  * @brief    UART Initialization And Configuration functions
  * @{
  */


/**
  * @brief  Open uart with baudrate and the default configuration.
  * @param  uart      : UART handle. Usually use @ref dev_get() function to get the handle.
  * @param  baudrate  : Configure the baudrate for UART.
  * @return
  *         - RET_OK  : UART module open successfully.
  *         - RET_ERR : UART module open unsuccessfully.
  * @note
  *         The default configuration is full duplex mode, no parity mode,\n
  *         data bit is 8bit, stop bit is 1bit.If you need to change the configuration,\n
  *         you can use the @ref uart_ioctl function to make changes after initialization is complete.
  */
int32 uart_open(struct uart_device *uart, uint32 baudrate)
{
    if (uart) {
        HALDEV_SUSPENDED(uart);
        return ((const struct uart_hal_ops *)uart->dev.ops)->open(uart, baudrate);
    }
    return RET_ERR;
}

/**
  * @brief  Close the UART module.
  * @param  uart : UART handle. Usually use @ref dev_get() function to get the handle.
  * @return
  *         - RET_OK  : UART module close successfully.
  *         - RET_ERR : UART module close unsuccessfully.
  * @note
  *         The UART will not be able to send and receive data normally,\n
  *         and all configurations (including interrupt-related configurations) will be invalid.
  */
int32 uart_close(struct uart_device *uart)
{
    if (uart) {
        HALDEV_SUSPENDED(uart);
        return ((const struct uart_hal_ops *)uart->dev.ops)->close(uart);
    }
    return RET_ERR;
}

/**
  * @brief  UART sends a frame of data.
  * @param  uart  : UART handle. Usually use @ref dev_get() function to get the handle.
  * @param  vaule : The data to be sent by the UART, the unit is 1 frame.
  * @return
  *         - RET_OK  : UART module send successfully.
  *         - RET_ERR : UART module send unsuccessfully.
  * @note
  *         If the UART configuration data bit is 8bit, one frame of data is 8bit;\n
  *         if the data bit is 9bit, one frame of data is 9bit.
  */
int32 uart_putc(struct uart_device *uart, int8 value)
{
    if (uart) {
        HALDEV_SUSPENDED(uart);
        return ((const struct uart_hal_ops *)uart->dev.ops)->putc(uart, value);
    }
    return RET_ERR;
}

/**
  * @brief  UART receives a frame of data.
  * @param  uart : UART handle. Usually use @ref dev_get() function to get the handle.
  * @return
  *         - value : The data value.
  * @note
  *         If the UART configuration data bit is 8bit, one frame of data is 8bit;\n
  *         if the data bit is 9bit, one frame of data is 9bit.
  */
uint8 uart_getc(struct uart_device *uart)
{
    if (uart) {
        HALDEV_SUSPENDED(uart);
        return ((const struct uart_hal_ops *)uart->dev.ops)->getc(uart);
    }
    return RET_ERR;
}

/**
  * @brief  The UART sends data according to the address and data number of the DATA BUFFER.
  * @param  uart : UART handle. Usually use @ref dev_get() function to get the handle.
  * @param  buf  : Start address of DATA BUFFER.
  * @param  len  : The number of data to be sent, the default unit is 8bit.
  * @return
  *         - RET_OK  : UART module send successfully.
  *         - RET_ERR : UART module send unsuccessfully.
  * @note
  *         Send uses CPU by default. To use DMA transmission,\n
  *         it needs to be configured using the @ref uart_ioctl() function.
  */
int32 uart_puts(struct uart_device *uart, uint8 *buf, uint32 len)
{
    if (uart) {
        HALDEV_SUSPENDED(uart);
        return ((const struct uart_hal_ops *)uart->dev.ops)->puts(uart, buf, len);
    }
    return RET_ERR;
}

/**
  * @brief  The UART receive data according to the address and data number of the DATA BUFFER.
  * @param  uart : UART handle. Usually use @ref dev_get() function to get the handle.
  * @param  buf  : Start address of DATA BUFFER.
  * @param  len  : The number of data to be sent, the default unit is 8bit.
  * @return
  *         - RET_OK  : UART module send successfully.
  *         - RET_ERR : UART module send unsuccessfully.
  * @note
  *         Send uses CPU by default. To use DMA transmission,
  *         it needs to be configured using the @ref uart_ioctl() function.
  */
int32 uart_gets(struct uart_device *uart, uint8 *buf, uint32 len)
{
    if (uart) {
        HALDEV_SUSPENDED(uart);
        return ((const struct uart_hal_ops *)uart->dev.ops)->gets(uart, buf, len);
    }
    return RET_ERR;
}

/**
  * @brief  Configure the UART module by ioctl_cmd.
  * @param  uart      : UART handle. Usually use @ref dev_get() function to get the handle.
  * @param  ioctl_cmd : Commands to configure the UART module, reference @ref uart_ioctl_cmd.
  * @param  param1    : Parameter 1, according to the command.
  * @param  param2    : Parameter 2, according to the command.
  * @return
  *         - RET_OK  : UART module configure successfully.
  *         - RET_ERR : UART module configure unsuccessfully.
  * @note
  *         None.
  */
int32 uart_ioctl(struct uart_device *uart, enum uart_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2)
{
    if (uart && ((const struct uart_hal_ops *)uart->dev.ops)->ioctl) {
        HALDEV_SUSPENDED(uart);
        return ((const struct uart_hal_ops *)uart->dev.ops)->ioctl(uart, ioctl_cmd, param1, param2);
    }
    return RET_ERR;
}

/**
  * @brief  Request interrupt by irq_flag.
  * @param  uart     : UART handle. Usually use @ref dev_get() function to get the handle.
  * @param  irq_hdl  : Interrupt handle, executed after the interrupt is generated.
  * @param  irq_flag : Interrupt type, reference @ref uart_irq_flag.
  * @param  irq_data : Parameters for interrupt handler.
  * @return
  *         - RET_OK  : UART module request interrupt successfully.
  *         - RET_ERR : UART module request interrupt unsuccessfully.
  * @note
  *         Support multiple interrupts to request together.
  */
int32 uart_request_irq(struct uart_device *uart, uart_irq_hdl irq_hdl, uint32 irq_flag,  uint32 irq_data)
{
    if (uart && ((const struct uart_hal_ops *)uart->dev.ops)->request_irq) {
        HALDEV_SUSPENDED(uart);
        return ((const struct uart_hal_ops *)uart->dev.ops)->request_irq(uart, irq_hdl, irq_flag, irq_data);
    }
    return RET_ERR;
}

/**
  * @brief  Release interrupt by irq_flag.
  * @param  uart     : UART handle. Usually use @ref dev_get() function to get the handle.
  * @param  irq_flag : Interrupt type, reference @ref uart_irq_flag.
  * @return
  *         - RET_OK  : UART module release interrupt successfully.
  *         - RET_ERR : UART module release interrupt unsuccessfully.
  * @note
  *         Support multiple interrupts to release together.
  */
int32 uart_release_irq(struct uart_device *uart, uint32 irq_flag)
{
    if (uart && ((const struct uart_hal_ops *)uart->dev.ops)->release_irq) {
        HALDEV_SUSPENDED(uart);
        return ((const struct uart_hal_ops *)uart->dev.ops)->release_irq(uart, irq_flag);
    }
    return RET_ERR;
}

/** @} Docxygenid_UART_functions*/

/** @} Docxygenid_UART_Driver*/



/*************************** (C) COPYRIGHT 2016-2022 HUGE-IC ***** END OF FILE *****/
