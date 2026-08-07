
// @file    pin_function.c
// @author  wangying
// @brief   This file contains all the mars pin functions.

// Revision History
// V1.0.0  06/01/2019  First Release, copy from 4001a project
// V1.0.1  07/05/2019  add lmac pin-func
// V1.0.2  07/06/2019  add sdio pull-up regs config
// V1.0.3  07/09/2019  add agc/rx-bw/lo-freq-idx gpio control
// V1.0.4  07/18/2019  change gpio-agc default index to 5
// V1.0.5  07/19/2019  uart1 only init tx
// V1.0.6  07/23/2019  add uart-rx pull-up resistor config
// V1.0.7  07/24/2019  switch-en1 disable; delete fdd/tdd macro-def switch
// V1.0.8  07/26/2019  not use pb17 for mac debug for it is used to reset ext-rf
// V1.0.9  07/29/2019  add function dbg_pin_func()
// V1.1.0  02/11/2020  add spi-pin-function
// V1.1.1  02/27/2020  fix uart1 pin-function
// V1.2.0  03/02/2020  add uart0-pin code and rf-pin code
// V1.2.1  03/26/2020  fix vmode pin
// V1.2.2  04/14/2020  use pa2 as lmac debug1:rx_req

#include "sys_config.h"
#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "hal/gpio.h"
#include "lib/lcd/lcd.h"


#ifdef PIN_FROM_PARAM
#include "pin_param.h"

struct sys_param2
{
    uint16_t size; // size of sys_factory_param2
    uint16_t pin_max;
    uint8_t pin[1];      //pin脚定义,255无效,最大搜索范围是pin_max
};

uint8_t get_sys_param_pin(uint32_t pin_enum)
{
    struct sys_param2 *param = (struct sys_param2 *)iocfg_psram;
    if (pin_enum < param->pin_max) {
        return param->pin[pin_enum];
    }
    return 0xff;
}
#endif

__weak void user_pin_func(int dev_id, int request) {};

/** 
  * @brief  Configure the GPIO pin driver strength.
  * @param  pin       : which pin to set.\n
  *                     This parameter can be Px_y where x can be (A..C) and y can be (0..15)\n
  *                     But the value of the y only be (0..5) when x is C.
  * @strength         : Driver strength to configure the GPIO pin, reference @ref gpio_private_pin_driver_strength.
  * @return
  *         - RET_OK  : Configure the GPIO pin driver strength successfully.
  *         - RET_ERR : Configure the GPIO pin driver strength unsuccessfully.
  */
int32 gpio_driver_strength(uint32 pin, enum pin_driver_strength strength)
{
    struct gpio_device *gpio = gpio_get(pin);
    if (gpio && ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl) {
        return ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl(gpio, pin, GPIO_CMD_DRIVER_STRENGTH, strength, 0);
    }
    return RET_ERR;
}


/** 
  * @brief  Configure the GPIO module AFIO.
  * @param  pin       : which pin to set.\n
  *                     This parameter can be Px_y where x can be (A..C) and y can be (0..15)\n
  *                     But the value of the y only be (0..5) when x is C.
  * @afio             : AFIO value, reference @ref gpio_private_afio_set.
  * @return
  *         - RET_OK  : GPIO module configure AFIO successfully.
  *         - RET_ERR : GPIO module configure AFIO unsuccessfully.
  */
int32 gpio_set_altnt_func(uint32 pin, enum gpio_afio_set afio)
{
    struct gpio_device *gpio = gpio_get(pin);
    if (gpio && ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl) {
        return ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl(gpio, pin, GPIO_CMD_AFIO_SET, afio, 0);
    }
    return RET_ERR;
}


/** 
  * @brief  Configure the GPIO module IOMAP_OUTPUT.
  * @param  pin       : which pin to set.\n
  *                     This parameter can be Px_y where x can be (A..C) and y can be (0..15)\n
  *                     But the value of the y only be (0..5) when x is C.
  * @func_sel         : IOMAP_OUTPUT function value, reference @ref gpio_private_iomap_out_func.
  * @return
  *         - RET_OK  : GPIO module configure IOMAP_OUTPUT successfully.
  *         - RET_ERR : GPIO module configure IOMAP_OUTPUT unsuccessfully.
  */
int32 gpio_iomap_output(uint32 pin, enum gpio_iomap_out_func func_sel)
{

    struct gpio_device *gpio = gpio_get(pin);
    if (gpio && ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl) {
        return ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl(gpio, pin, GPIO_CMD_IOMAP_OUT_FUNC, func_sel, 0);
    }

    return RET_ERR;
}


/** 
  * @brief  Configure the GPIO module IOMAP_INPUT.
  * @param  pin       : which pin to set.\n
  *                     This parameter can be Px_y where x can be (A..C) and y can be (0..15)\n
  *                     But the value of the y only be (0..5) when x is C.
  * @func_sel         : IOMAP_INPUT function value, reference @ref gpio_private_iomap_in_func.
  * @return
  *         - RET_OK  : GPIO module configure IOMAP_INPUT successfully.
  *         - RET_ERR : GPIO module configure IOMAP_INPUT unsuccessfully.
  */
int32 gpio_iomap_input(uint32 pin, enum gpio_iomap_in_func func_sel)
{

    struct gpio_device *gpio = gpio_get(pin);
    if (gpio && ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl) {
        return ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl(gpio, pin, GPIO_CMD_IOMAP_IN_FUNC, func_sel, 0);
    }

    return RET_ERR;
}


/** 
  * @brief  Configure the GPIO module IOMAP_INOUT.
  * @param  pin       : which pin to set.\n
  *                     This parameter can be Px_y where x can be (A..C) and y can be (0..15)\n
  *                     But the value of the y only be (0..5) when x is C.
  * @in_func_sel      : IOMAP_INPUT function value, reference @ref gpio_private_iomap_in_func.
  * @out_func_sel     : IOMAP_OUTPUT function value, reference @ref gpio_private_iomap_out_func.
  * @return
  *         - RET_OK  : GPIO module configure IOMAP_INOUT successfully.
  *         - RET_ERR : GPIO module configure IOMAP_INOUT unsuccessfully.
  */
int32 gpio_iomap_inout(uint32 pin, enum gpio_iomap_in_func in_func_sel, enum gpio_iomap_out_func out_func_sel)
{

    struct gpio_device *gpio = gpio_get(pin);
    if (gpio && ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl) {
        return ((const struct gpio_hal_ops *)gpio->dev.ops)->ioctl(gpio, pin, GPIO_CMD_IOMAP_INOUT_FUNC, in_func_sel, out_func_sel);
    }

    return RET_ERR;
}

static int uart_pin_func(int dev_id, int request)
{
    int ret = RET_OK;

    switch (dev_id) {
        case HG_UART0_DEVID:
            if (request) {
                gpio_iomap_output(MACRO_PIN(PIN_UART0_TX), GPIO_IOMAP_OUT_UART0_OUT);
                gpio_iomap_input(MACRO_PIN(PIN_UART0_RX), GPIO_IOMAP_IN_UART0_IN);
                gpio_set_mode(MACRO_PIN(PIN_UART0_RX), GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
            } else {
                gpio_set_dir(MACRO_PIN(PIN_UART0_TX), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_UART0_RX), GPIO_DIR_INPUT);
            }
            break;
        case HG_UART1_DEVID: //if use uart1 for print, just init tx
            if (request) {
                gpio_iomap_input(MACRO_PIN(PIN_UART1_RX), GPIO_IOMAP_IN_UART1_IN_LCD_D9_IN_M1_28);
                gpio_iomap_output(MACRO_PIN(PIN_UART1_TX), GPIO_IOMAP_OUT_UART1_OUT);
                gpio_set_mode(MACRO_PIN(PIN_UART1_RX), GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
            } else {
                gpio_set_dir(MACRO_PIN(PIN_UART1_RX), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_UART1_TX), GPIO_DIR_INPUT);
            }
            break;
        case (HG_UART4_DEVID):
            if (request) {
                gpio_iomap_output(MACRO_PIN(PIN_UART4_TX), GPIO_IOMAP_OUT_UART4_TX);
                gpio_iomap_input(MACRO_PIN(PIN_UART4_RX), GPIO_IOMAP_IN_UART4_IN);
            } else {
                gpio_set_dir(MACRO_PIN(PIN_UART4_TX), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_UART4_RX), GPIO_DIR_INPUT);
            }
            break;
        case (HG_UART5_DEVID):
            if (request) {
                gpio_iomap_output(MACRO_PIN(PIN_UART5_TX), GPIO_IOMAP_OUT_UART5_TX);
                gpio_iomap_input(MACRO_PIN(PIN_UART5_RX), GPIO_IOMAP_IN_IIS1_MCLK_IN_Uart5_IN_LCD_D22_IN_M2_9);
            } else {
                gpio_set_dir(MACRO_PIN(PIN_UART5_TX), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_UART5_RX), GPIO_DIR_INPUT);
            }
            break;
        case (HG_UART6_DEVID):
            if (request) {
                gpio_iomap_output(MACRO_PIN(PIN_UART6_TX), GPIO_IOMAP_OUT_UART6_TX);
                gpio_iomap_input(MACRO_PIN(PIN_UART6_RX), GPIO_IOMAP_IN_IIS0_DAT_IN_UART6_RX_M3_9);
            } else {
                gpio_set_dir(MACRO_PIN(PIN_UART6_TX), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_UART6_RX), GPIO_DIR_INPUT);
            }
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}

static int gmac_pin_func(int dev_id, int request)
{
    int ret = RET_OK;

    switch (dev_id) {
        case HG_GMAC_DEVID:
            if (request) {
                gpio_set_altnt_func(MACRO_PIN(PIN_GMAC_RMII_REF_CLKIN), 0);
                gpio_set_altnt_func(MACRO_PIN(PIN_GMAC_RMII_RXD0), 0);
                gpio_set_altnt_func(MACRO_PIN(PIN_GMAC_RMII_RXD1), 0);
                gpio_set_altnt_func(MACRO_PIN(PIN_GMAC_RMII_TXD0), 0);
                gpio_set_altnt_func(MACRO_PIN(PIN_GMAC_RMII_TXD1), 0);
                gpio_set_altnt_func(MACRO_PIN(PIN_GMAC_RMII_CRS_DV), 0);
                gpio_set_altnt_func(MACRO_PIN(PIN_GMAC_RMII_TX_EN), 0);
                gpio_set_dir(MACRO_PIN(PIN_GMAC_RMII_MDIO), GPIO_DIR_OUTPUT);
                gpio_set_dir(MACRO_PIN(PIN_GMAC_RMII_MDC), GPIO_DIR_OUTPUT);
            } else {
                gpio_set_dir(MACRO_PIN(PIN_GMAC_RMII_REF_CLKIN), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_GMAC_RMII_RXD0), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_GMAC_RMII_RXD1), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_GMAC_RMII_TXD0), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_GMAC_RMII_TXD1), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_GMAC_RMII_CRS_DV), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_GMAC_RMII_TX_EN), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_GMAC_RMII_MDIO), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_GMAC_RMII_MDC), GPIO_DIR_INPUT);
            }
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}


static int sdio_pin_func(int dev_id, int request)
{
    int ret = RET_OK;

    switch (dev_id) {
        case HG_SDIOSLAVE_DEVID:
            if (request) {
                gpio_set_altnt_func(MACRO_PIN(PIN_SDCLK), 0);
                gpio_set_altnt_func(MACRO_PIN(PIN_SDCMD), 0);
                gpio_set_mode(MACRO_PIN(PIN_SDCMD), GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_mode(MACRO_PIN(PIN_SDCLK), GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_altnt_func(MACRO_PIN(PIN_SDDAT0), 0);
                gpio_set_mode(MACRO_PIN(PIN_SDDAT0), GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_altnt_func(MACRO_PIN(PIN_SDDAT1), 0);
                gpio_set_mode(MACRO_PIN(PIN_SDDAT1), GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_altnt_func(MACRO_PIN(PIN_SDDAT2), 0);
                gpio_set_mode(MACRO_PIN(PIN_SDDAT2), GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_altnt_func(MACRO_PIN(PIN_SDDAT3), 0);
                gpio_set_mode(MACRO_PIN(PIN_SDDAT3), GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);

                gpio_ioctl(MACRO_PIN(PIN_SDCMD), GPIO_INPUT_DELAY_ON_OFF, 1, 0);
                gpio_ioctl(MACRO_PIN(PIN_SDCLK), GPIO_INPUT_DELAY_ON_OFF, 1, 0);
                gpio_ioctl(MACRO_PIN(PIN_SDDAT0), GPIO_INPUT_DELAY_ON_OFF, 1, 0);
                gpio_ioctl(MACRO_PIN(PIN_SDDAT1), GPIO_INPUT_DELAY_ON_OFF, 1, 0);
                gpio_ioctl(MACRO_PIN(PIN_SDDAT2), GPIO_INPUT_DELAY_ON_OFF, 1, 0);
                gpio_ioctl(MACRO_PIN(PIN_SDDAT3), GPIO_INPUT_DELAY_ON_OFF, 1, 0);

                gpio_driver_strength(MACRO_PIN(PIN_SDCLK),  GPIO_DS_G1);
                gpio_driver_strength(MACRO_PIN(PIN_SDCMD),  GPIO_DS_G1);
                gpio_driver_strength(MACRO_PIN(PIN_SDDAT0), GPIO_DS_G1);
                gpio_driver_strength(MACRO_PIN(PIN_SDDAT1), GPIO_DS_G1);
                gpio_driver_strength(MACRO_PIN(PIN_SDDAT2), GPIO_DS_G1);
                gpio_driver_strength(MACRO_PIN(PIN_SDDAT3), GPIO_DS_G1);
            } else {
                gpio_set_dir(MACRO_PIN(PIN_SDCLK), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_SDCMD), GPIO_DIR_INPUT);
                gpio_set_mode(MACRO_PIN(PIN_SDCMD), GPIO_PULL_UP, GPIO_PULL_LEVEL_NONE);
                gpio_set_dir(MACRO_PIN(PIN_SDDAT0), GPIO_DIR_INPUT);
                gpio_set_mode(MACRO_PIN(PIN_SDDAT0), GPIO_PULL_UP, GPIO_PULL_LEVEL_NONE);
                gpio_set_dir(MACRO_PIN(PIN_SDDAT1), GPIO_DIR_INPUT);
                gpio_set_mode(MACRO_PIN(PIN_SDDAT1), GPIO_PULL_UP, GPIO_PULL_LEVEL_NONE);
                gpio_set_dir(MACRO_PIN(PIN_SDDAT2), GPIO_DIR_INPUT);
                gpio_set_mode(MACRO_PIN(PIN_SDDAT2), GPIO_PULL_UP, GPIO_PULL_LEVEL_NONE);
                gpio_set_dir(MACRO_PIN(PIN_SDDAT3), GPIO_DIR_INPUT);
                gpio_set_mode(MACRO_PIN(PIN_SDDAT3), GPIO_PULL_UP, GPIO_PULL_LEVEL_NONE);
                gpio_ioctl(MACRO_PIN(PIN_SDCMD), GPIO_INPUT_DELAY_ON_OFF, 0, 0);
                gpio_ioctl(MACRO_PIN(PIN_SDCLK), GPIO_INPUT_DELAY_ON_OFF, 0, 0);
                gpio_ioctl(MACRO_PIN(PIN_SDDAT0), GPIO_INPUT_DELAY_ON_OFF, 0, 0);
                gpio_ioctl(MACRO_PIN(PIN_SDDAT1), GPIO_INPUT_DELAY_ON_OFF, 0, 0);
                gpio_ioctl(MACRO_PIN(PIN_SDDAT2), GPIO_INPUT_DELAY_ON_OFF, 0, 0);
                gpio_ioctl(MACRO_PIN(PIN_SDDAT3), GPIO_INPUT_DELAY_ON_OFF, 0, 0);
            }
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}

static int qspi_pin_func(int dev_id, int request)
{
    int ret = RET_OK;
    static int req_cs1 = 0;

    switch (dev_id) {
        case HG_QSPI_DEVID:
            if (request) {
//                gpio_set_altnt_func(MACRO_PIN(PIN_QSPI_CS),  4);
//                gpio_set_altnt_func(MACRO_PIN(PIN_QSPI_CLK), 4);
//                gpio_set_altnt_func(MACRO_PIN(PIN_QSPI_IO0), 4);
//                gpio_set_altnt_func(MACRO_PIN(PIN_QSPI_IO1), 4);
//                gpio_set_altnt_func(MACRO_PIN(PIN_QSPI_IO2), 4);
//                gpio_set_altnt_func(MACRO_PIN(PIN_QSPI_IO3), 4);
                if (request == 2) {
                    //gpio_set_altnt_func(MACRO_PIN(PIN_QSPI_CS1), 4);
                    req_cs1 = 1;
                }
            } else {
                //gpio_set_dir(MACRO_PIN(PIN_QSPI_CS),  GPIO_DIR_INPUT);
                //gpio_set_dir(MACRO_PIN(PIN_QSPI_CLK), GPIO_DIR_INPUT);
                //gpio_set_dir(MACRO_PIN(PIN_QSPI_IO0), GPIO_DIR_INPUT);
                //gpio_set_dir(MACRO_PIN(PIN_QSPI_IO1), GPIO_DIR_INPUT);
                //gpio_set_dir(MACRO_PIN(PIN_QSPI_IO2), GPIO_DIR_INPUT);
                //gpio_set_dir(MACRO_PIN(PIN_QSPI_IO3), GPIO_DIR_INPUT);
                if (req_cs1) {
                    req_cs1 = 0;
                    //gpio_set_dir(MACRO_PIN(PIN_QSPI_CS1),  GPIO_DIR_INPUT);
                }
            }
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}

static int xspi_pin_func(int dev_id, int request)
{
#define OSPI_MAP1_D4(n)         ((n & 0xF)<<0)
#define OSPI_MAP1_D5(n)         ((n & 0xF)<<4)
#define OSPI_MAP1_D6(n)         ((n & 0xF)<<8)
#define OSPI_MAP1_D7(n)         ((n & 0xF)<<12)
    
#define OSPI_MAP0_CLK(n)        ((n & 0xF)<<0)
#define OSPI_MAP0_CS(n)         ((n & 0xF)<<4)
#define OSPI_MAP0_DQS(n)        ((n & 0xF)<<8)
#define OSPI_MAP0_DM(n)         ((n & 0xF)<<12)
#define OSPI_MAP0_D0(n)         ((n & 0xF)<<16)
#define OSPI_MAP0_D1(n)         ((n & 0xF)<<20)
#define OSPI_MAP0_D2(n)         ((n & 0xF)<<24)
#define OSPI_MAP0_D3(n)         ((n & 0xF)<<28)

    int ret = RET_OK;
    int switch_off = request & 0x80;
    int psram_type = request & 0x7F;

    if (HG_XSPI_DEVID != dev_id) {
        return EINVAL;
    }

    switch (psram_type) {
        case 0:
            if (!switch_off) {
                gpio_driver_strength(PA_7,  GPIO_DS_G0);
                gpio_driver_strength(PA_8,  GPIO_DS_G0);
                gpio_driver_strength(PA_11, GPIO_DS_G0);
                gpio_driver_strength(PA_12, GPIO_DS_G0);
                gpio_driver_strength(PA_13, GPIO_DS_G0); 
                gpio_driver_strength(PA_14, GPIO_DS_G0);
                
                gpio_set_mode(PA_7,  GPIO_PULL_UP,  GPIO_PULL_LEVEL_100K);
                gpio_set_mode(PA_8,  GPIO_PULL_UP,  GPIO_PULL_LEVEL_100K);
                gpio_set_mode(PA_11, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_mode(PA_12, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_mode(PA_13, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_mode(PA_14, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);  
                
                gpio_set_altnt_func(PA_13, GPIO_AF_2);
                gpio_set_altnt_func(PA_14, GPIO_AF_2);
                gpio_set_altnt_func(PA_8,  GPIO_AF_2);
                gpio_set_altnt_func(PA_7,  GPIO_AF_2);
                gpio_set_altnt_func(PA_11, GPIO_AF_2);
                gpio_set_altnt_func(PA_12, GPIO_AF_2); 
            } else {
                gpio_set_dir(PA_7 , GPIO_DIR_INPUT);
                gpio_set_dir(PA_8 , GPIO_DIR_INPUT);
                gpio_set_dir(PA_11, GPIO_DIR_INPUT);
                gpio_set_dir(PA_12, GPIO_DIR_INPUT);
                gpio_set_dir(PA_13, GPIO_DIR_INPUT);
                gpio_set_dir(PA_14, GPIO_DIR_INPUT);
            }
            break;
        case 1:
            if (!switch_off) {
                gpio_set_mode(PE_4, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_mode(PE_5, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_mode(PE_6, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_mode(PE_7, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_mode(PE_8, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_mode(PE_9, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_mode(PE_10,GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                
                gpio_driver_strength(PE_4, GPIO_DS_G0);
                gpio_driver_strength(PE_5, GPIO_DS_G0);
                gpio_driver_strength(PE_6, GPIO_DS_G0);
                gpio_driver_strength(PE_7, GPIO_DS_G0);
                gpio_driver_strength(PE_8, GPIO_DS_G0);  
                gpio_driver_strength(PE_9, GPIO_DS_G0);
                gpio_driver_strength(PE_10,GPIO_DS_G0);
                
                gpio_set_altnt_func(PE_4, GPIO_AF_1);
                gpio_set_altnt_func(PE_5, GPIO_AF_1);
                gpio_set_altnt_func(PE_6, GPIO_AF_1);
                gpio_set_altnt_func(PE_7, GPIO_AF_1);
                gpio_set_altnt_func(PE_8, GPIO_AF_1);
                gpio_set_altnt_func(PE_9, GPIO_AF_1);
                gpio_set_altnt_func(PE_10,GPIO_AF_1);
            } else {
            }
            break;
        case 2:
            if (!switch_off) {
                gpio_set_mode(PE_4, GPIO_PULL_DOWN, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_5, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_6, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_7, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_8, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_9, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_10, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_11, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_12, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_13, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_14, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_15, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                
                gpio_driver_strength(PE_4, GPIO_DS_G0);
                gpio_driver_strength(PE_5, GPIO_DS_G0);
                gpio_driver_strength(PE_6, GPIO_DS_G0);
                gpio_driver_strength(PE_7, GPIO_DS_G0);
                gpio_driver_strength(PE_8, GPIO_DS_G0);  
                gpio_driver_strength(PE_9, GPIO_DS_G0);
                gpio_driver_strength(PE_10, GPIO_DS_G0);
                gpio_driver_strength(PE_11, GPIO_DS_G0);
                gpio_driver_strength(PE_12, GPIO_DS_G0);
                gpio_driver_strength(PE_13, GPIO_DS_G0);
                gpio_driver_strength(PE_14, GPIO_DS_G0);
                gpio_driver_strength(PE_15, GPIO_DS_G0);
#if 0
                gpio_set_altnt_func(PE_4, GPIO_AF_1);
                gpio_set_altnt_func(PE_5, GPIO_AF_2);
                gpio_set_altnt_func(PE_6, GPIO_AF_2);
                gpio_set_altnt_func(PE_7, GPIO_AF_2);
                gpio_set_altnt_func(PE_8, GPIO_AF_2);
                gpio_set_altnt_func(PE_9, GPIO_AF_2);
                gpio_set_altnt_func(PE_10, GPIO_AF_2);
                gpio_set_altnt_func(PE_11, GPIO_AF_2);
                gpio_set_altnt_func(PE_12, GPIO_AF_2);
                gpio_set_altnt_func(PE_13, GPIO_AF_2);
                gpio_set_altnt_func(PE_14, GPIO_AF_2);
                gpio_set_altnt_func(PE_15, GPIO_AF_2);
#else
                /* PE8/PE9 switch */
                 SYSCTRL_REG_OPT( 
                    SYSCTRL->OSPI_MAP_CTL0 = OSPI_MAP0_CLK(8) | OSPI_MAP0_DQS(4) | OSPI_MAP0_DM(15) | OSPI_MAP0_CS(10) |
                                             OSPI_MAP0_D0(14) | OSPI_MAP0_D1(13) | OSPI_MAP0_D2(12) | OSPI_MAP0_D3(11);
                    SYSCTRL->OSPI_MAP_CTL1 = 0xFFFF0000 | OSPI_MAP1_D4(9) | OSPI_MAP1_D5(6) | OSPI_MAP1_D6(7) | OSPI_MAP1_D7(5);
                );
#endif
                /**
                 * PE16_IE  [0]       PE16_OE  [1]      PE16_DRV  [2]    PE16_PD  [3]     PE16_PU  [4]      PE_OUT_DATA  [5]     PE_CLKn[6]
                 */
                //while(1) {SYSCTRL->PE16CON = (0x1<<1) | (0x1<<5); SYSCTRL->PE16CON = (0x1<<1) | (0x1<<1);}
                SYSCTRL_REG_OPT( SYSCTRL->PE16CON = (0x1<<1) | (0x1<<6); );// clkn
                SYSCTRL_REG_OPT( SYSCTRL->SYS_CON15 = (SYSCTRL->SYS_CON15 & ~(0x1<<14)) | (1<<14); );
                SYSCTRL_REG_OPT( SYSCTRL->SYS_CON15 = (SYSCTRL->SYS_CON15 & ~(0x1<<15)) | (1<<15); );
            } else {
            }
            break;
        case 3:
            if (!switch_off) {
                gpio_set_mode(PE_4, GPIO_PULL_DOWN, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_5, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_6, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_7, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_8, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_9, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_10, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_11, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_12, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_13, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_14, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
//                gpio_set_mode(PE_15, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                
                gpio_driver_strength(PE_4, GPIO_DS_G0);
                gpio_driver_strength(PE_5, GPIO_DS_G0);
                gpio_driver_strength(PE_6, GPIO_DS_G0);
                gpio_driver_strength(PE_7, GPIO_DS_G0);
                gpio_driver_strength(PE_8, GPIO_DS_G0);  
                gpio_driver_strength(PE_9, GPIO_DS_G0);
                gpio_driver_strength(PE_10, GPIO_DS_G0);
                gpio_driver_strength(PE_11, GPIO_DS_G0);
                gpio_driver_strength(PE_12, GPIO_DS_G0);
                gpio_driver_strength(PE_13, GPIO_DS_G0);
                gpio_driver_strength(PE_14, GPIO_DS_G0);
                gpio_driver_strength(PE_15, GPIO_DS_G0);
#if 0                
                gpio_set_altnt_func(PE_4, GPIO_AF_1);
                gpio_set_altnt_func(PE_5, GPIO_AF_2);
                gpio_set_altnt_func(PE_6, GPIO_AF_2);
                gpio_set_altnt_func(PE_7, GPIO_AF_2);
                gpio_set_altnt_func(PE_8, GPIO_AF_2);
                gpio_set_altnt_func(PE_9, GPIO_AF_2);
                gpio_set_altnt_func(PE_10, GPIO_AF_2);
                gpio_set_altnt_func(PE_11, GPIO_AF_2);
                gpio_set_altnt_func(PE_12, GPIO_AF_2);
                gpio_set_altnt_func(PE_13, GPIO_AF_2);
                gpio_set_altnt_func(PE_14, GPIO_AF_2);
                //gpio_set_altnt_func(PE_15, GPIO_AF_2);
#else
                SYSCTRL->OSPI_MAP_CTL0 = OSPI_MAP0_CLK(8) | OSPI_MAP0_DQS(4) | OSPI_MAP0_DM(15) | OSPI_MAP0_CS(10) |
                                         OSPI_MAP0_D0(14) | OSPI_MAP0_D1(13) | OSPI_MAP0_D2(12) | OSPI_MAP0_D3(11);
                SYSCTRL->OSPI_MAP_CTL1 = 0xFFFF0000 | OSPI_MAP1_D4(9) | OSPI_MAP1_D5(6) | OSPI_MAP1_D6(7) | OSPI_MAP1_D7(5);
#endif
                /**
                 * PE16_IE  [0]       PE16_OE  [1]      PE16_DRV  [2]    PE16_PD  [3]     PE16_PU  [4]      PE_OUT_DATA  [5]     PE_CLKn[6]
                 */
                SYSCTRL_REG_OPT( SYSCTRL->PE16CON = (0x1<<1) | (0x1<<5); );// rst  = 1
                SYSCTRL_REG_OPT( SYSCTRL->SYS_CON15 = (SYSCTRL->SYS_CON15 & ~(0x1<<14)) | (1<<14); );
                SYSCTRL_REG_OPT( SYSCTRL->SYS_CON15 = (SYSCTRL->SYS_CON15 & ~(0x1<<15)) | (1<<15); );
            } else {
            }
            break;

        default:
            ret = EINVAL;
            break;
    }
    return ret;
}


static int spi_pin_func(int dev_id, int request)
{
    int ret = RET_OK;
	switch (dev_id) {
        case HG_SPI0_DEVID:
            if (request) {
                gpio_iomap_inout(MACRO_PIN(PIN_SPI0_CS), GPIO_IOMAP_IN_SPI0_NSS_IN, GPIO_IOMAP_OUT_SPI0_NSS_OUT);
                gpio_iomap_inout(MACRO_PIN(PIN_SPI0_CLK), GPIO_IOMAP_IN_SPI0_SCK_IN, GPIO_IOMAP_OUT_SPI0_SCK_OUT);
                gpio_iomap_inout(MACRO_PIN(PIN_SPI0_IO0), GPIO_IOMAP_IN_SPI0_IO0_IN, GPIO_IOMAP_OUT_SPI0_IO0_OUT);
                gpio_iomap_inout(MACRO_PIN(PIN_SPI0_IO1), GPIO_IOMAP_IN_SPI0_IO1_IN, GPIO_IOMAP_OUT_SPI0_IO1_OUT);
                gpio_iomap_inout(MACRO_PIN(PIN_SPI0_IO2), GPIO_IOMAP_IN_SPI0_IO2_IN, GPIO_IOMAP_OUT_SPI0_IO2_OUT);
                gpio_iomap_inout(MACRO_PIN(PIN_SPI0_IO3), GPIO_IOMAP_IN_SPI0_IO3_IN, GPIO_IOMAP_OUT_SPI0_IO3_OUT);
            } else {
                gpio_set_dir(MACRO_PIN(PIN_SPI0_CS), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_SPI0_CLK), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_SPI0_IO0), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_SPI0_IO1), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_SPI0_IO2), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_SPI0_IO3), GPIO_DIR_INPUT);
            }
            break;
        case HG_SPI1_DEVID:
            if (request) {
                gpio_iomap_inout(MACRO_PIN(PIN_SPI1_CS), GPIO_IOMAP_IN_SPI1_NSS_IN_LCD_D11_IN_M1_30, GPIO_IOMAP_OUT_SPI1_NSS_OUT);
                gpio_iomap_inout(MACRO_PIN(PIN_SPI1_CLK), GPIO_IOMAP_IN_SPI1_SCK_IN, GPIO_IOMAP_OUT_SPI1_SCK_OUT);
                gpio_iomap_inout(MACRO_PIN(PIN_SPI1_IO0), GPIO_IOMAP_IN_SPI1_IO0_IN, GPIO_IOMAP_OUT_SPI1_IO0_OUT);
                gpio_iomap_inout(MACRO_PIN(PIN_SPI1_IO1), GPIO_IOMAP_IN_SPI1_IO1_IN_LCD_D12_IN_M1_31, GPIO_IOMAP_OUT_SPI1_IO1_OUT);
            } else {
                gpio_set_dir(MACRO_PIN(PIN_SPI1_CS), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_SPI1_CLK), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_SPI1_IO0), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_SPI1_IO1), GPIO_DIR_INPUT);
            }
            break;
//        case (HG_SPI5_DEVID):
//            if (request) {
//                gpio_iomap_inout(MACRO_PIN(PIN_SPI5_CLK), GPIO_IOMAP_IN_SPI5_SCK_IN, GPIO_IOMAP_OUT_SPI5_SCK_OUT);
//                gpio_iomap_inout(MACRO_PIN(PIN_SPI5_IO0), GPIO_IOMAP_IN_SPI5_IO0_IN, GPIO_IOMAP_OUT_SPI5_IO0_OUT);
//                gpio_iomap_inout(MACRO_PIN(PIN_SPI5_IO1), GPIO_IOMAP_IN_SPI5_IO1_IN, GPIO_IOMAP_OUT_SPI5_IO1_OUT);
//            } else {
//                gpio_set_dir(MACRO_PIN(PIN_SPI5_CLK), GPIO_DIR_INPUT);
//                gpio_set_dir(MACRO_PIN(PIN_SPI5_CLK), GPIO_DIR_INPUT);
//                gpio_set_dir(MACRO_PIN(PIN_SPI5_CLK), GPIO_DIR_INPUT);
//            }
//            break;
//        case (HG_SPI6_DEVID):
//            if (request) {
//                gpio_iomap_inout(MACRO_PIN(PIN_SPI6_CLK), GPIO_IOMAP_IN_SPI6_SCK_IN, GPIO_IOMAP_OUT_SPI6_SCK_OUT);
//                gpio_iomap_inout(MACRO_PIN(PIN_SPI6_IO0), GPIO_IOMAP_IN_SPI6_IO0_IN, GPIO_IOMAP_OUT_SPI6_IO0_OUT);
//                gpio_iomap_inout(MACRO_PIN(PIN_SPI6_IO1), GPIO_IOMAP_IN_SPI6_IO1_IN, GPIO_IOMAP_OUT_SPI6_IO1_OUT);
//            } else {
//                gpio_set_dir(MACRO_PIN(PIN_SPI6_CLK), GPIO_DIR_INPUT);
//                gpio_set_dir(MACRO_PIN(PIN_SPI6_CLK), GPIO_DIR_INPUT);
//                gpio_set_dir(MACRO_PIN(PIN_SPI6_CLK), GPIO_DIR_INPUT);
//            }
//            break;
        case HG_SPI2_DEVID:
        case HG_SPI3_DEVID:
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}

static int iis_pin_func(int dev_id, int request)
{
    int ret = RET_OK;

    switch (dev_id) {
//        case HG_IIS0_DEVID:
//            if (request) {
//                gpio_iomap_inout(MACRO_PIN(PIN_IIS0_MCLK), GPIO_IOMAP_IN_IIS0_MCLK_IN, GPIO_IOMAP_OUT_IIS0_MCLK_OUT);
//                gpio_iomap_inout(MACRO_PIN(PIN_IIS0_BCLK), GPIO_IOMAP_IN_IIS0_BCLK_IN, GPIO_IOMAP_OUT_IIS0_BCLK_OUT);
//                gpio_iomap_inout(MACRO_PIN(PIN_IIS0_WCLK), GPIO_IOMAP_IN_IIS0_WSCLK_IN, GPIO_IOMAP_OUT_IIS0_WSCLK_OUT);
//                gpio_iomap_inout(MACRO_PIN(PIN_IIS0_DATA), GPIO_IOMAP_IN_IIS0_DI, GPIO_IOMAP_OUT_IIS0_DO);
//            } else {
//                gpio_set_dir(MACRO_PIN(PIN_IIS0_MCLK), GPIO_DIR_INPUT);
//                gpio_set_dir(MACRO_PIN(PIN_IIS0_BCLK), GPIO_DIR_INPUT);
//                gpio_set_dir(MACRO_PIN(PIN_IIS0_WCLK), GPIO_DIR_INPUT);
//                gpio_set_dir(MACRO_PIN(PIN_IIS0_DATA), GPIO_DIR_INPUT);
//            }
//            break;
//        case HG_IIS1_DEVID:
//            if (request) {
//                gpio_iomap_inout(MACRO_PIN(PIN_IIS1_MCLK), GPIO_IOMAP_IN_IIS1_MCLK_IN, GPIO_IOMAP_OUT_IIS1_MCLK_OUT);
//                gpio_iomap_inout(MACRO_PIN(PIN_IIS1_BCLK), GPIO_IOMAP_IN_IIS1_BCLK_IN, GPIO_IOMAP_OUT_IIS1_BCLK_OUT);
//                gpio_iomap_inout(MACRO_PIN(PIN_IIS1_WCLK), GPIO_IOMAP_IN_IIS1_WSCLK_IN, GPIO_IOMAP_OUT_IIS1_WSCLK_OUT);
//                gpio_iomap_inout(MACRO_PIN(PIN_IIS1_DATA), GPIO_IOMAP_IN_IIS1_DI, GPIO_IOMAP_OUT_IIS1_DO);
//            } else {
//                gpio_set_dir(MACRO_PIN(PIN_IIS1_MCLK), GPIO_DIR_INPUT);
//                gpio_set_dir(MACRO_PIN(PIN_IIS1_BCLK), GPIO_DIR_INPUT);
//                gpio_set_dir(MACRO_PIN(PIN_IIS1_WCLK), GPIO_DIR_INPUT);
//                gpio_set_dir(MACRO_PIN(PIN_IIS1_DATA), GPIO_DIR_INPUT);
//            }
//            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}

static int pdm_pin_func(int dev_id, int request)
{
    int ret = RET_OK;

    switch (dev_id) {
        case HG_PDM0_DEVID:
            if (request) {
                gpio_iomap_output(MACRO_PIN(PIN_PDM_MCLK), GPIO_IOMAP_OUT_PDM_MCLK);
                gpio_iomap_input(MACRO_PIN(PIN_PDM_DATA),  GPIO_IOMAP_IN_PDM_DATA_IN_EPWM_TZ1_M3_2);
            } else {
                gpio_set_dir(MACRO_PIN(PIN_PDM_MCLK), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_PDM_DATA), GPIO_DIR_INPUT);
            }
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}


static int led_pin_func(int dev_id, int request)
{
    int ret = RET_OK;

    switch (dev_id) {
        case HG_LED0_DEVID:
            if (request) {
                gpio_set_altnt_func(MACRO_PIN(PIN_LED_SEG0), GPIO_AF_0);     //seg0
                gpio_set_altnt_func(MACRO_PIN(PIN_LED_SEG1), GPIO_AF_0);     //seg1
                gpio_set_altnt_func(MACRO_PIN(PIN_LED_SEG2), GPIO_AF_0);     //seg2
                gpio_set_altnt_func(MACRO_PIN(PIN_LED_SEG3), GPIO_AF_0);     //seg3
                gpio_set_altnt_func(MACRO_PIN(PIN_LED_SEG4), GPIO_AF_0);     //seg4
                gpio_set_altnt_func(MACRO_PIN(PIN_LED_SEG5), GPIO_AF_0);    //seg5
                gpio_set_altnt_func(MACRO_PIN(PIN_LED_SEG6), GPIO_AF_0);    //seg6
                gpio_set_altnt_func(MACRO_PIN(PIN_LED_SEG7), GPIO_AF_0);    //seg7
//                gpio_set_altnt_func(MACRO_PIN(PIN_LED_SEG8), GPIO_AF_0);    //seg8
//                gpio_set_altnt_func(MACRO_PIN(PIN_LED_SEG9) , GPIO_AF_0);    //seg9
//                gpio_set_altnt_func(MACRO_PIN(PIN_LED_SEG10),GPIO_AF_0);    //seg10
//                gpio_set_altnt_func(MACRO_PIN(PIN_LED_SEG11),GPIO_AF_0);    //seg11

                gpio_set_altnt_func(MACRO_PIN(PIN_LED_COM0),  GPIO_AF_0);    //com0
                gpio_set_altnt_func(MACRO_PIN(PIN_LED_COM1),  GPIO_AF_0);    //com1
                gpio_set_altnt_func(MACRO_PIN(PIN_LED_COM2),  GPIO_AF_0);    //com2
//                gpio_set_altnt_func(MACRO_PIN(PIN_LED_COM3),  GPIO_AF_0);    //com3
//                gpio_set_altnt_func(MACRO_PIN(PIN_LED_COM4),  GPIO_AF_1);    //com4
//                gpio_set_altnt_func(MACRO_PIN(PIN_LED_COM5),  GPIO_AF_1);    //com5
//                gpio_set_altnt_func(MACRO_PIN(PIN_LED_COM6),  GPIO_AF_1);    //com6
//                gpio_set_altnt_func(MACRO_PIN(PIN_LED_COM7),  GPIO_AF_1);    //com7

            } else {
                gpio_set_dir(MACRO_PIN(PIN_LED_SEG0), GPIO_DIR_INPUT);     //seg0
                gpio_set_dir(MACRO_PIN(PIN_LED_SEG1), GPIO_DIR_INPUT);     //seg1
                gpio_set_dir(MACRO_PIN(PIN_LED_SEG2), GPIO_DIR_INPUT);     //seg2
                gpio_set_dir(MACRO_PIN(PIN_LED_SEG3), GPIO_DIR_INPUT);     //seg3
                gpio_set_dir(MACRO_PIN(PIN_LED_SEG4), GPIO_DIR_INPUT);     //seg4
                gpio_set_dir(MACRO_PIN(PIN_LED_SEG5), GPIO_DIR_INPUT);    //seg5
                gpio_set_dir(MACRO_PIN(PIN_LED_SEG6), GPIO_DIR_INPUT);    //seg6
                gpio_set_dir(MACRO_PIN(PIN_LED_SEG7), GPIO_DIR_INPUT);    //seg7
//                gpio_set_dir(MACRO_PIN(PIN_LED_SEG8), GPIO_DIR_INPUT);    //seg8
//                gpio_set_dir(MACRO_PIN(PIN_LED_SEG9), GPIO_DIR_INPUT);    //seg9
//                gpio_set_dir(MACRO_PIN(PIN_LED_SEG10),GPIO_DIR_INPUT);    //seg10
//                gpio_set_dir(MACRO_PIN(PIN_LED_SEG11),GPIO_DIR_INPUT);    //seg11

                gpio_set_dir(MACRO_PIN(PIN_LED_COM0),  GPIO_DIR_INPUT);    //com0
//                gpio_set_dir(MACRO_PIN(PIN_LED_COM1),  GPIO_DIR_INPUT);    //com1
//                gpio_set_dir(MACRO_PIN(PIN_LED_COM2),  GPIO_DIR_INPUT);    //com2
//                gpio_set_dir(MACRO_PIN(PIN_LED_COM3),  GPIO_DIR_INPUT);    //com3
//                gpio_set_dir(MACRO_PIN(PIN_LED_COM4),  GPIO_DIR_INPUT);    //com4
//                gpio_set_dir(MACRO_PIN(PIN_LED_COM5),  GPIO_DIR_INPUT);    //com5
//                gpio_set_dir(MACRO_PIN(PIN_LED_COM6),  GPIO_DIR_INPUT);    //com6
//                gpio_set_dir(MACRO_PIN(PIN_LED_COM7),  GPIO_DIR_INPUT);    //com7
            }
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}

static int iic_pin_func(int dev_id, int request)
{
    int ret = RET_OK;
    switch (dev_id) {
        case HG_I2C1_DEVID:
            if (request) {
                gpio_iomap_inout(MACRO_PIN(PIN_IIC1_SCL), GPIO_IOMAP_IN_SPI1_SCK_IN, GPIO_IOMAP_OUT_SPI1_SCK_OUT);
                gpio_iomap_inout(MACRO_PIN(PIN_IIC1_SDA), GPIO_IOMAP_IN_SPI1_IO0_IN, GPIO_IOMAP_OUT_SPI1_IO0_OUT);
                 gpio_driver_strength(MACRO_PIN(PIN_IIC1_SCL), GPIO_DS_G3);
                gpio_driver_strength(MACRO_PIN(PIN_IIC1_SDA), GPIO_DS_G3);
				gpio_set_mode(MACRO_PIN(PIN_IIC1_SCL), GPIO_OPENDRAIN_PULL_UP, GPIO_PULL_LEVEL_4_7K);
                gpio_set_mode(MACRO_PIN(PIN_IIC1_SDA), GPIO_OPENDRAIN_PULL_UP, GPIO_PULL_LEVEL_4_7K);
            } else {
                gpio_set_dir(MACRO_PIN(PIN_IIC1_SCL), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_IIC1_SDA), GPIO_DIR_INPUT);
            }
            break;
        case HG_I2C2_DEVID:
            if (request) {
                gpio_iomap_inout(MACRO_PIN(PIN_IIC2_SCL), GPIO_IOMAP_IN_SPI2_SCK_IN, GPIO_IOMAP_OUT_SPI2_SCK_OUT);
                gpio_iomap_inout(MACRO_PIN(PIN_IIC2_SDA), GPIO_IOMAP_IN_SPI2_IO0_IN, GPIO_IOMAP_OUT_SPI2_IO0_OUT);
                gpio_driver_strength(MACRO_PIN(PIN_IIC2_SCL), GPIO_DS_G3);
                gpio_driver_strength(MACRO_PIN(PIN_IIC2_SDA), GPIO_DS_G3);			
                gpio_set_mode(MACRO_PIN(PIN_IIC2_SCL), GPIO_OPENDRAIN_PULL_UP, GPIO_PULL_LEVEL_4_7K);
                gpio_set_mode(MACRO_PIN(PIN_IIC2_SDA), GPIO_OPENDRAIN_PULL_UP, GPIO_PULL_LEVEL_4_7K);
            } else {
                gpio_set_dir(MACRO_PIN(PIN_IIC2_SCL), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_IIC2_SDA), GPIO_DIR_INPUT);
            }
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}


static int adc_pin_func(int dev_id, int request) {

    #define ADC_FLAG_REQUEST BIT(26)
    #define ADC_FLAG_SUSPEND BIT(27)
    #define ADC_FLAG_RELEASE BIT(28)

    //struct gpio_device *ptr = NULL;
    uint32 flag = request & (0xFF << 24);
    uint32 adc_pin = (request&0xFFFFFF);

    switch (dev_id) {
        case (HG_ADC0_DEVID):   
            switch (flag) {
                /* release */
                case (ADC_FLAG_RELEASE):
                    gpio_ioctl(adc_pin, GPIO_GENERAL_ANALOG, 0, 0);
                    gpio_set_dir(adc_pin, GPIO_DIR_INPUT);
                    break;
                /* suspend */
                case (ADC_FLAG_SUSPEND):
                    gpio_ioctl(adc_pin, GPIO_GENERAL_ANALOG, 0, 0);
                    break;
                case (ADC_FLAG_REQUEST):
                    gpio_ioctl(adc_pin, GPIO_GENERAL_ANALOG, 1, 0);
                    break;
                default:
                    return RET_ERR;
                    break;
            }
            break;
    }

    return RET_OK;
}

static int timer_pin_func(int dev_id, int request)
{
    int ret = RET_OK;

    switch (dev_id) {
        case HG_TIMER0_DEVID:
            switch (request) {
                /* 0: none */
                case (0):
                    gpio_set_dir(MACRO_PIN(PIN_PWM_CHANNEL_0), GPIO_DIR_INPUT);
                    gpio_set_dir(MACRO_PIN(PIN_CAPTURE_CHANNEL_0), GPIO_DIR_INPUT);
                    break;
                /* 1: pwm */
                case (1):
                    gpio_iomap_output(MACRO_PIN(PIN_PWM_CHANNEL_0), GPIO_IOMAP_OUT_TMR0_PWM_OUT);
                    break;
                /* 2: capture */
                case (2):
                    gpio_iomap_input(MACRO_PIN(PIN_CAPTURE_CHANNEL_0), GPIO_IOMAP_IN_TMR0_CAP_IN);
                    break;
                default:
                    ret = EINVAL;
                    break;
            }
            break;
        case HG_TIMER1_DEVID:
            switch (request) {
                /* 0: none */
                case (0):
                    gpio_set_dir(MACRO_PIN(PIN_PWM_CHANNEL_1), GPIO_DIR_INPUT);
                    gpio_set_dir(MACRO_PIN(PIN_CAPTURE_CHANNEL_1), GPIO_DIR_INPUT);
                    break;
                /* 1: pwm */
                case (1):
                    gpio_iomap_output(MACRO_PIN(PIN_PWM_CHANNEL_1), GPIO_IOMAP_OUT_TMR1_PWM_OUT);
                    break;
                /* 2: capture */
                case (2):
                    gpio_iomap_input(MACRO_PIN(PIN_CAPTURE_CHANNEL_1), GPIO_IOMAP_IN_TMR1_CAP_IN);
                    break;
                default:
                    ret = EINVAL;
                    break;
            }
            break;
        case HG_TIMER2_DEVID:
            switch (request) {
                /* 0: none */
                case (0):
                    gpio_set_dir(MACRO_PIN(PIN_PWM_CHANNEL_2), GPIO_DIR_INPUT);
                    break;
                /* 1: pwm */
                case (1):
                    gpio_iomap_output(MACRO_PIN(PIN_PWM_CHANNEL_2), GPIO_IOMAP_OUT_TMR2_PWM_OUT);
                    break;
                default:
                    ret = EINVAL;
                    break;
            }
            break;			
		case HG_TIMER3_DEVID:
			switch (request) {
				/* 0: none */
				case (0):
					gpio_set_dir(MACRO_PIN(PIN_PWM_CHANNEL_3), GPIO_DIR_INPUT);
					break;
				/* 1: pwm */
				case (1):
					gpio_iomap_output(MACRO_PIN(PIN_PWM_CHANNEL_3), GPIO_IOMAP_OUT_TMR3_PWM_OUT);
					break;
				default:
					ret = EINVAL;
					break;
			}
			break;
        case HG_LED_TIMER0_DEVID:
            switch (request) {
                /* 0: none */
                case (0):
                    gpio_set_dir(MACRO_PIN(PIN_PWM_CHANNEL_2), GPIO_DIR_INPUT);
                    break;
                /* 1: pwm */
                case (1):
                    gpio_iomap_output(MACRO_PIN(PIN_PWM_CHANNEL_2), GPIO_IOMAP_OUT_LED_TMR0_PWM_OUT);
                    break;
                default:
                    ret = EINVAL;
                    break;
            }
            break;
        case HG_SIMTMR0_DEVID:
            switch (request) {
                /* 0: none */
                case (0):
                    gpio_set_dir(MACRO_PIN(PIN_PWM_CHANNEL_3), GPIO_DIR_INPUT);
                    //gpio_set_dir(MACRO_PIN(PIN_CAPTURE_CHANNEL_3), GPIO_DIR_INPUT);
                    break;
                /* 1: pwm */
                case (1):
					SYSCTRL->IOFUNCMASK0 &= ~BIT(27);
                    gpio_iomap_output(MACRO_PIN(PIN_PWM_CHANNEL_3), GPIO_IOMAP_OUT_STMR0_PWM_OUT);
                    break;
                /* 2: capture */
                case (2):
                    //gpio_iomap_input(MACRO_PIN(PIN_CAPTURE_CHANNEL_3), GPIO_IOMAP_IN_STMR0_CAP_IN);
                    break;
                default:
                    ret = EINVAL;
                    break;
            }
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}


static int dvp_pin_func(int dev_id, int request)
{
    int ret = RET_OK;
    switch (dev_id) {   
        case HG_DVP_DEVID:
            gpio_set_altnt_func(MACRO_PIN(PIN_DVP_HSYNC), 1);
            gpio_set_altnt_func(MACRO_PIN(PIN_DVP_VSYNC), 1);
            gpio_set_altnt_func(MACRO_PIN(PIN_DVP_PCLK) , 1);
            gpio_set_altnt_func(MACRO_PIN(PIN_DVP_MCLK) , 1);
            gpio_driver_strength(MACRO_PIN(PIN_DVP_MCLK), GPIO_DS_G1);
            gpio_set_altnt_func(MACRO_PIN(PIN_DVP_DATA0), 1);
            gpio_set_altnt_func(MACRO_PIN(PIN_DVP_DATA1), 1);
            gpio_set_altnt_func(MACRO_PIN(PIN_DVP_DATA2), 1);
            gpio_set_altnt_func(MACRO_PIN(PIN_DVP_DATA3), 1);
            gpio_set_altnt_func(MACRO_PIN(PIN_DVP_DATA4), 1);
            gpio_set_altnt_func(MACRO_PIN(PIN_DVP_DATA5), 1);
            gpio_set_altnt_func(MACRO_PIN(PIN_DVP_DATA6), 1);
            gpio_set_altnt_func(MACRO_PIN(PIN_DVP_DATA7), 1);
            gpio_set_altnt_func(MACRO_PIN(PIN_DVP_DATA8), 1);
            gpio_set_altnt_func(MACRO_PIN(PIN_DVP_DATA9), 1);			
            gpio_set_altnt_func(MACRO_PIN(PIN_DVP_DATA10), 1);
            gpio_set_altnt_func(MACRO_PIN(PIN_DVP_DATA11), 1);
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}

static int mipi_csi_pin_func(int dev_id, int request)
{
    int ret = RET_OK;
    switch (dev_id) {   
        case HG_MIPI_CSI_DEVID:
            gpio_ioctl(MACRO_PIN(PIN_MIPI_CSI0_CLKN), GPIO_AIODIS_ANALOG, 1, 0);       //CLK
			gpio_ioctl(MACRO_PIN(PIN_MIPI_CSI0_CLKP), GPIO_AIODIS_ANALOG, 1, 0);
			if(request == 1){
				gpio_ioctl(MACRO_PIN(PIN_MIPI_CSI0_D0P), GPIO_AIODIS_ANALOG, 1, 0);   //D0
				gpio_ioctl(MACRO_PIN(PIN_MIPI_CSI0_D0N), GPIO_AIODIS_ANALOG, 1, 0);
			}else{
				gpio_ioctl(MACRO_PIN(PIN_MIPI_CSI0_D0P), GPIO_AIODIS_ANALOG, 1, 0);   //D0
				gpio_ioctl(MACRO_PIN(PIN_MIPI_CSI0_D0N), GPIO_AIODIS_ANALOG, 1, 0);
				gpio_ioctl(MACRO_PIN(PIN_MIPI_CSI0_D1N_CSI1_D0N), GPIO_AIODIS_ANALOG, 1, 0);   //D1
				gpio_ioctl(MACRO_PIN(PIN_MIPI_CSI0_D1P_CSI1_D0P), GPIO_AIODIS_ANALOG, 1, 0);				
			}
            break;
        case HG_MIPI1_CSI_DEVID:
            gpio_ioctl(MACRO_PIN(PIN_MIPI_CSI1_CLKN), GPIO_AIODIS_ANALOG, 1, 0);	   //CLK
			gpio_ioctl(MACRO_PIN(PIN_MIPI_CSI1_CLKP), GPIO_AIODIS_ANALOG, 1, 0);
		
			gpio_ioctl(MACRO_PIN(PIN_MIPI_CSI0_D1N_CSI1_D0N), GPIO_AIODIS_ANALOG, 1, 0);       //D0
			gpio_ioctl(MACRO_PIN(PIN_MIPI_CSI0_D1P_CSI1_D0P), GPIO_AIODIS_ANALOG, 1, 0);		
            break;			
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}

static int mipi_dsi_pin_func(int dev_id, int request)
{
    int ret = RET_OK;
    switch (dev_id) {   
        case HG_DSI_DEVID:
            gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_CLKN), GPIO_AIODIS_ANALOG, 1, 0);       //CLK
			gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_CLKP), GPIO_AIODIS_ANALOG, 1, 0);
			if(request == 1){
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D0N), GPIO_AIODIS_ANALOG, 1, 0);   //D0
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D0P), GPIO_AIODIS_ANALOG, 1, 0);
			}else if(request == 2){
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D0N), GPIO_AIODIS_ANALOG, 1, 0);   //D0
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D0P), GPIO_AIODIS_ANALOG, 1, 0);
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D1N), GPIO_AIODIS_ANALOG, 1, 0);   //D1
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D1P), GPIO_AIODIS_ANALOG, 1, 0);				
			}else if(request == 3){
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D0N), GPIO_AIODIS_ANALOG, 1, 0);   //D0
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D0P), GPIO_AIODIS_ANALOG, 1, 0);
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D1N), GPIO_AIODIS_ANALOG, 1, 0);   //D1
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D1P), GPIO_AIODIS_ANALOG, 1, 0);
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D2N), GPIO_AIODIS_ANALOG, 1, 0);   //D2
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D2P), GPIO_AIODIS_ANALOG, 1, 0);				
			}else if(request == 4){
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D0N), GPIO_AIODIS_ANALOG, 1, 0);    //D0
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D0P), GPIO_AIODIS_ANALOG, 1, 0);
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D1N), GPIO_AIODIS_ANALOG, 1, 0);    //D1
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D1P), GPIO_AIODIS_ANALOG, 1, 0);
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D2N), GPIO_AIODIS_ANALOG, 1, 0);    //D2
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D2P), GPIO_AIODIS_ANALOG, 1, 0);				
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D3N), GPIO_AIODIS_ANALOG, 1, 0);    //D3
				gpio_ioctl(MACRO_PIN(PIN_MIPI_DSI_D3P), GPIO_AIODIS_ANALOG, 1, 0);
			}
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}

static int lcdc_pin_func(int dev_id, int request)
{
    int ret = RET_OK;
    switch (dev_id) {
        case HG_LCDC_DEVID:
			if(VS_CS != 255){
				gpio_iomap_output(VS_CS, GPIO_IOMAP_OUT_LCD_VSYNC_OR_CS0);
			}			

			if(HS_DC != 255){
				gpio_iomap_output(HS_DC, GPIO_IOMAP_OUT_LCD_HSYNC_OR_DC);
			}

			if(DE_ERD != 255){
				gpio_iomap_output(DE_ERD, GPIO_IOMAP_OUT_LCD_DE_OR_ERD1);
			}			

			if(DOTCLK_RWR != 255){
				gpio_iomap_output(DOTCLK_RWR, GPIO_IOMAP_OUT_LCD_DOTCLK_OR_RWR0);
			}	

			if(LCD_D0 != 255){
				gpio_iomap_inout(LCD_D0, GPIO_IOMAP_IN_LCD_D0_IN_M1_19 ,GPIO_IOMAP_OUT_LCD_DATA_O_0);
			}

			if(LCD_D1 != 255){
				gpio_iomap_output(LCD_D1, GPIO_IOMAP_OUT_LCD_DATA_O_1);
			}

			if(LCD_D2 != 255){
				gpio_iomap_output(LCD_D2, GPIO_IOMAP_OUT_LCD_DATA_O_2);
			}

			if(LCD_D3 != 255){
				gpio_iomap_output(LCD_D3, GPIO_IOMAP_OUT_LCD_DATA_O_3);
			}

			if(LCD_D4 != 255){
				gpio_iomap_output(LCD_D4, GPIO_IOMAP_OUT_LCD_DATA_O_4);
			}

			if(LCD_D5 != 255){
				gpio_iomap_output(LCD_D5, GPIO_IOMAP_OUT_LCD_DATA_O_5);
			}

			if(LCD_D6 != 255){
				gpio_iomap_output(LCD_D6, GPIO_IOMAP_OUT_LCD_DATA_O_6);
			}

			if(LCD_D7 != 255){
				gpio_iomap_output(LCD_D7, GPIO_IOMAP_OUT_LCD_DATA_O_7);
			}			

			if(LCD_D8 != 255){
				gpio_iomap_output(LCD_D8, GPIO_IOMAP_OUT_LCD_DATA_O_8);
			}

			if(LCD_D9 != 255){
				gpio_iomap_output(LCD_D9, GPIO_IOMAP_OUT_LCD_DATA_O_9);
			}

			if(LCD_D10 != 255){
				gpio_iomap_output(LCD_D10, GPIO_IOMAP_OUT_LCD_DATA_O_10);
			}

			if(LCD_D11 != 255){
				gpio_iomap_output(LCD_D11, GPIO_IOMAP_OUT_LCD_DATA_O_11);
			}

			if(LCD_D12 != 255){
				gpio_iomap_output(LCD_D12, GPIO_IOMAP_OUT_LCD_DATA_O_12);
			}

			if(LCD_D13 != 255){
				gpio_iomap_output(LCD_D13, GPIO_IOMAP_OUT_LCD_DATA_O_13);
			}

			if(LCD_D14 != 255){
				gpio_iomap_output(LCD_D14, GPIO_IOMAP_OUT_LCD_DATA_O_14);
			}

			if(LCD_D15 != 255){
				gpio_iomap_output(LCD_D15, GPIO_IOMAP_OUT_LCD_DATA_O_15);
			}

			if(LCD_D16 != 255){
				gpio_iomap_output(LCD_D16, GPIO_IOMAP_OUT_LCD_DATA_O_16);
			}			

			if(LCD_D17 != 255){
				gpio_iomap_output(LCD_D17, GPIO_IOMAP_OUT_LCD_DATA_O_17);
			}

			if(LCD_D18 != 255){
				gpio_iomap_output(LCD_D18, GPIO_IOMAP_OUT_LCD_DATA_O_18);
			}

			if(LCD_D19 != 255){
				gpio_iomap_output(LCD_D19, GPIO_IOMAP_OUT_LCD_DATA_O_19);
			}

			if(LCD_D20 != 255){
				gpio_iomap_output(LCD_D20, GPIO_IOMAP_OUT_LCD_DATA_O_20);
			}

			if(LCD_D21 != 255){
				gpio_iomap_output(LCD_D21, GPIO_IOMAP_OUT_LCD_DATA_O_21);
			}

			if(LCD_D22 != 255){
				gpio_iomap_output(LCD_D22, GPIO_IOMAP_OUT_LCD_DATA_O_22);
			}

			if(LCD_D23 != 255){
				gpio_iomap_output(LCD_D23, GPIO_IOMAP_OUT_LCD_DATA_O_23);
			}


        break;
        default:
            ret = EINVAL;
        break;
    }
    return ret;

}

static int sdh_pin_func(int dev_id, int request)
{
    int ret = RET_OK;

    switch (dev_id) {
        case HG_SDIOHOST_DEVID:
            if (request) {
                gpio_set_mode(MACRO_PIN(PIN_SDH_CLK), GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_mode(MACRO_PIN(PIN_SDH_CMD), GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                gpio_set_mode(MACRO_PIN(PIN_SDH_DAT0), GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                if (request == 4) {
                    gpio_set_mode(MACRO_PIN(PIN_SDH_DAT1), GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                    gpio_set_mode(MACRO_PIN(PIN_SDH_DAT2), GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                    gpio_set_mode(MACRO_PIN(PIN_SDH_DAT3), GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);
                }

                int pin_sdh_clk_dri_strngth = MACRO_PIN(PIN_SDH_CLK_DRI_STRENGTH);
                if (pin_sdh_clk_dri_strngth == 255) {
                    pin_sdh_clk_dri_strngth = GPIO_DS_G1;
                }

                gpio_driver_strength(MACRO_PIN(PIN_SDH_CLK), pin_sdh_clk_dri_strngth);
                gpio_driver_strength(MACRO_PIN(PIN_SDH_CMD), GPIO_DS_G1);
                gpio_driver_strength(MACRO_PIN(PIN_SDH_DAT0), GPIO_DS_G1);
                if (request == 4) {
                    gpio_driver_strength(MACRO_PIN(PIN_SDH_DAT1), GPIO_DS_G1);
                    gpio_driver_strength(MACRO_PIN(PIN_SDH_DAT2), GPIO_DS_G1);
                    gpio_driver_strength(MACRO_PIN(PIN_SDH_DAT3), GPIO_DS_G1);
                }
                gpio_iomap_output(MACRO_PIN(PIN_SDH_CLK), GPIO_IOMAP_OUT_SDHOST_SCLK_O);
#if (SD_MODE_TYPE == 0)
                gpio_iomap_inout(MACRO_PIN(PIN_SDH_CMD), GPIO_IOMAP_IN_SDHOST_CMD_IN, GPIO_IOMAP_OUT_SDHOST_CMD_OUT);
#elif (SD_MODE_TYPE == 1)
                gpio_iomap_inout(MACRO_PIN(PIN_SDH_CMD) , GPIO_IOMAP_IN_IIS0_DAT_IN_UART6_RX_M3_9, GPIO_IOMAP_OUT_IIS0_DO);
#elif (SD_MODE_TYPE == 2)
                gpio_iomap_inout(MACRO_PIN(PIN_SDH_CMD) , GPIO_IOMAP_IN_SPI0_IO0_IN, GPIO_IOMAP_OUT_SPI0_IO0_OUT);
#else
                gpio_iomap_inout(MACRO_PIN(PIN_SDH_CMD) , GPIO_IOMAP_IN_SPI2_IO0_IN, GPIO_IOMAP_OUT_SPI2_IO0_OUT);
#endif
                gpio_iomap_inout(MACRO_PIN(PIN_SDH_DAT0), GPIO_IOMAP_IN_SDHOST_DAT0_IN, GPIO_IOMAP_OUT_SDHOST_DAT0_OUT);
                gpio_ioctl(MACRO_PIN(PIN_SDH_DAT0), GPIO_CMD_SET_IEEN, 1, 0);
                if (request == 4) {
                    gpio_iomap_inout(MACRO_PIN(PIN_SDH_DAT1), GPIO_IOMAP_IN_SDHOST_DAT1_IN, GPIO_IOMAP_OUT_SDHOST_DAT1_OUT);
                    gpio_iomap_inout(MACRO_PIN(PIN_SDH_DAT2), GPIO_IOMAP_IN_SDHOST_DAT2_IN, GPIO_IOMAP_OUT_SDHOST_DAT2_OUT);
                    gpio_iomap_inout(MACRO_PIN(PIN_SDH_DAT3), GPIO_IOMAP_IN_SDHOST_DAT3_IN, GPIO_IOMAP_OUT_SDHOST_DAT3_OUT);
                }

            } else {
                gpio_set_dir(MACRO_PIN(PIN_SDH_CLK), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_SDH_CMD), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_SDH_DAT0), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_SDH_DAT1), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_SDH_DAT2), GPIO_DIR_INPUT);
                gpio_set_dir(MACRO_PIN(PIN_SDH_DAT3), GPIO_DIR_INPUT);
            }
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;
}

static int para_in_pin_func(int dev_id, int request)
{
    int ret = RET_OK;
    switch (dev_id) {   
        case HG_PARA_IN_DEVID:
            gpio_set_altnt_func(MACRO_PIN(PIN_BT_HSYNC), 2);
            gpio_set_altnt_func(MACRO_PIN(PIN_BT_VSYNC), 2);
            gpio_set_altnt_func(MACRO_PIN(PIN_BT_PCLK) , 2);
            gpio_set_altnt_func(MACRO_PIN(PIN_BT_BLANK), 2);
            gpio_set_altnt_func(MACRO_PIN(PIN_BT_DATA0), 2);
            gpio_set_altnt_func(MACRO_PIN(PIN_BT_DATA1), 2);
            gpio_set_altnt_func(MACRO_PIN(PIN_BT_DATA2), 2);
            gpio_set_altnt_func(MACRO_PIN(PIN_BT_DATA3), 2);
            gpio_set_altnt_func(MACRO_PIN(PIN_BT_DATA4), 2);
            gpio_set_altnt_func(MACRO_PIN(PIN_BT_DATA5), 2);
            gpio_set_altnt_func(MACRO_PIN(PIN_BT_DATA6), 2);
            gpio_set_altnt_func(MACRO_PIN(PIN_BT_DATA7), 2);			
            break;
        default:
            ret = EINVAL;
            break;
    }
    return ret;    
}

int lmac_fem_pin_func(int request)
{
    int ret = RET_OK;

    if (request) {
        gpio_iomap_output(MACRO_PIN(PIN_LMAC_FEM_RF_TX_EN), GPIO_IOMAP_OUT_RF_TX_EN_FEM);
        gpio_iomap_output(MACRO_PIN(PIN_LMAC_FEM_RF_RX_EN), GPIO_IOMAP_OUT_RF_RX_EN_FEM);
    } else {
        gpio_iomap_output(MACRO_PIN(PIN_LMAC_FEM_RF_TX_EN), GPIO_IOMAP_OUTPUT);
        gpio_iomap_output(MACRO_PIN(PIN_LMAC_FEM_RF_RX_EN), GPIO_IOMAP_OUTPUT);
    }

    return ret;
}


int32 pin_func(uint16 dev_id, int32 request)
{
    int ret = RET_OK;

    sysctrl_unlock();

    switch (dev_id) {
        case HG_UART0_DEVID:
        case HG_UART1_DEVID:
        case HG_UART4_DEVID:
        case HG_UART5_DEVID:
            ret = uart_pin_func(dev_id, request);
            break;
        case HG_GMAC_DEVID:
            ret = gmac_pin_func(dev_id, request);
            break;
        case HG_SDIOSLAVE_DEVID:
            ret = sdio_pin_func(dev_id, request);
            break;
        case HG_SPI0_DEVID:
        case HG_SPI2_DEVID:
        case HG_SPI1_DEVID:
        case HG_SPI3_DEVID:
        case HG_SPI5_DEVID:
        case HG_SPI6_DEVID:
            ret = spi_pin_func(dev_id, request);
            break;
        case HG_I2C0_DEVID:
        case HG_I2C1_DEVID:
        case HG_I2C2_DEVID:
        case HG_I2C3_DEVID:
            ret = iic_pin_func(dev_id, request);
            break;
        case HG_DVP_DEVID:
            ret = dvp_pin_func(dev_id, request);
            break;
        case HG_QSPI_DEVID:
            ret = qspi_pin_func(dev_id, request);
            break;
        case HG_SDIOHOST_DEVID:
            ret = sdh_pin_func(dev_id, request);
            break;
        case HG_IIS0_DEVID:
        case HG_IIS1_DEVID:
            ret = iis_pin_func(dev_id, request);
            break;
        case HG_PDM0_DEVID:
            ret = pdm_pin_func(dev_id, request);
            break;
        case HG_LED0_DEVID:
            ret = led_pin_func(dev_id, request);
            break;
        case HG_TIMER0_DEVID:
        case HG_TIMER1_DEVID:
        case HG_TIMER2_DEVID:
        case HG_TIMER3_DEVID:
        case HG_LED_TIMER0_DEVID:
        case HG_LED_TIMER1_DEVID:
        case HG_LED_TIMER2_DEVID:
        case HG_LED_TIMER3_DEVID:
        case HG_SIMTMR0_DEVID:
        case HG_SIMTMR1_DEVID:
        case HG_SIMTMR2_DEVID:
        case HG_SIMTMR3_DEVID:
        case HG_SIMTMR4_DEVID:
        case HG_SIMTMR5_DEVID:
            ret = timer_pin_func(dev_id, request);
            break;
        case HG_ADC0_DEVID:
        case HG_ADC1_DEVID:
            //ret = adc_pin_func(dev_id, request);
            break;
        case HG_XSPI_DEVID:
            ret = xspi_pin_func(dev_id, request);
            break;
        case HG_LCDC_DEVID:
            ret = lcdc_pin_func(dev_id, request);
            break;
		case HG_MIPI_CSI_DEVID:
			ret = mipi_csi_pin_func(dev_id, request);
			break;
		case HG_MIPI1_CSI_DEVID:
			ret = mipi_csi_pin_func(dev_id, request);
			break;			
		case HG_DSI_DEVID:
			ret = mipi_dsi_pin_func(dev_id, request);
			break;	
        case HG_PARA_IN_DEVID:
            ret = para_in_pin_func(dev_id, request);
            break;
        default:
            break;
    }

    user_pin_func(dev_id, request);
    sysctrl_lock();
    return ret;
}

