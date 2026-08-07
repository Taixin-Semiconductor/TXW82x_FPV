/**
  ******************************************************************************
  * @file    User/xxx.c
  * @author  HUGE-IC Application Team
  * @version V1.0.0
  * @date    01-08-2018
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2018 HUGE-IC</center></h2>
  *
  *
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/i2c.h"

/** @addtogroup Template_Project
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

int32 i2c_open(struct i2c_device *i2c, enum i2c_mode mode, enum i2c_addr_mode addr_mode, uint32 addr)
{
    if (i2c) {
        return ((const struct i2c_hal_ops *)i2c->dev.ops)->open(i2c, mode, addr_mode, addr);
    }
    return RET_ERR;
}

int32 i2c_close(struct i2c_device *i2c)
{
    if (i2c) {
        return ((const struct i2c_hal_ops *)i2c->dev.ops)->close(i2c);
    }
    return RET_ERR;
}

int32 i2c_set_baudrate(struct i2c_device *i2c, uint32 baudrate)
{
    if (i2c) {
        return ((const struct i2c_hal_ops *)i2c->dev.ops)->baudrate(i2c, baudrate);
    }

    return RET_ERR;
}

int32 i2c_ioctl(struct i2c_device *i2c, uint32 cmd, uint32 param)
{
    if (i2c && ((const struct i2c_hal_ops *)i2c->dev.ops)->ioctl) {
        return ((const struct i2c_hal_ops *)i2c->dev.ops)->ioctl(i2c, cmd, param);
    }
    return RET_ERR;
}

int32 i2c_read(struct i2c_device *i2c, int8 *addr, uint32 addr_len, int8 *buf, uint32 buf_len)
{
    if (i2c && ((const struct i2c_hal_ops *)i2c->dev.ops)->read) {
        return ((const struct i2c_hal_ops *)i2c->dev.ops)->read(i2c, addr, addr_len, buf, buf_len);
    }
    return RET_ERR;
}

int32 i2c_write(struct i2c_device *i2c, int8 *addr, uint32 addr_len, int8 *buf, uint32 buf_len)
{
    if (i2c && ((const struct i2c_hal_ops *)i2c->dev.ops)->write) {
        return ((const struct i2c_hal_ops *)i2c->dev.ops)->write(i2c, addr, addr_len, buf, buf_len);
    }
    return RET_ERR;
}

int32 i2c_write_scatter(struct i2c_device *i2c, int8 *addr, uint32 addr_len, scatter_data *data, uint32 count)
{
    if (i2c && ((const struct i2c_hal_ops *)i2c->dev.ops)->write_scatter) {
        return ((const struct i2c_hal_ops *)i2c->dev.ops)->write_scatter(i2c, addr, addr_len, data, count);
    }
    return RET_ERR;
}

int32 i2c_master_write_table(struct i2c_device *i2c, uint8 cmd_len, uint8 slave_id, scatter_data *data)
{
    if (i2c && ((const struct i2c_hal_ops *)i2c->dev.ops)->write_table) {
        return ((const struct i2c_hal_ops *)i2c->dev.ops)->write_table(i2c, cmd_len, slave_id, data);
    }
    return RET_ERR; 
}

int32 i2c_request_irq(struct i2c_device *i2c, i2c_irq_hdl handle, uint32 irq_data, uint32 irq_flag)
{
    if (i2c && ((const struct i2c_hal_ops *)i2c->dev.ops)->request_irq) {
        return ((const struct i2c_hal_ops *)i2c->dev.ops)->request_irq(i2c, handle, irq_data, irq_flag);
    }
    return RET_ERR;
}

int32 i2c_release_irq(struct i2c_device *i2c, uint32 irq_flag)
{
    if (i2c && ((const struct i2c_hal_ops *)i2c->dev.ops)->release_irq) {
        return ((const struct i2c_hal_ops *)i2c->dev.ops)->release_irq(i2c, irq_flag);
    }
    return RET_ERR;
}


/**
  * @}
  */

/*************************** (C) COPYRIGHT 2018 HUGE-IC ***** END OF FILE *****/
