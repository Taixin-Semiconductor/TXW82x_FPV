/**
  ******************************************************************************
  * @file    sdk/include/hal/i2c.h
  * @author  HUGE-IC Application Team
  * @version V1.0.0
  * @date    01-08-2018
  * @brief   Main program body header file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2018 HUGE-IC</center></h2>
  *
  *
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _HAL_I2C_H_
#define _HAL_I2C_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/** @addtogroup Template_Project
  * @{
  */

/**
  * @brief Enumeration constant for IIC ioctrl cmd
  */
enum i2c_ioctl_cmd {
    /*! IIC SDA output delay
     */
    IIC_SDA_OUTPUT_DELAY,

    /*! IIC SCL strong output
     *@note: I/O cannot be configured when the strong output is on
     */
    IIC_STRONG_OUTPUT,

    IIC_FILTERING,

    IIC_SET_DEVICE_ADDR,
    
    IIC_SET_WRITE_TABLE_MODE,

	IIC_GET_TX_STATUS,
};


/**
  * @brief Enumeration constant for IIC mode selection
  */
enum i2c_mode {
    /*! IIC selects the master mode.
     */
    IIC_MODE_MASTER,

    /*! IIC selects the slave mode.
     */
    IIC_MODE_SLAVE,
};

/**
  * @brief Enumeration constant for IIC address bit selection
  */
enum i2c_addr_mode {
    /*! The IIC communication address selects the 7-bit mode.
     */

    IIC_ADDR_7BIT,
    /*! The IIC communication address selects the 10-bit mode.
     */
    IIC_ADDR_10BIT,
};

enum i2c_irq_flag {
    /*!
     * @brief Interrupt: IIC send data completed.
     * @Note
     *        This interrupt is only used for IIC transmit.
     */
    I2C_IRQ_FLAG_TX_DONE     = BIT(0),

    /*!
     * @brief Interrupt: IIC receive data completed.
     * @Note:
     *        This interrupt is only used for IIC receive.
     */
    I2C_IRQ_FLAG_RX_DONE     = BIT(1),

    /*!
     * @brief Interrupt: IIC slave nack to IIC master
     */
    I2C_IRQ_FLAG_RX_NACK     = BIT(2),

    /*!
     * @brief Interrupt: IIC rececive error
     * @Note:
     *        IIC buf overflow means rx error
     */
    I2C_IRQ_FLAG_RX_ERROR    = BIT(3),

    /*!
     * @brief Interrupt: IIC detect stop
     * @Note:
     *       IIC has detected the stop signal on the bus
     */
    I2C_IRQ_FLAG_DETECT_STOP = BIT(4),

	/*
	* @brief Interrupt: IIC slave addressed
	* @Note :
	* 		The interrupt triggered when the II slave is addressed
	*/

	I2C_IRQ_FLAG_SLAVE_ADDRESSED = BIT(5)


};


typedef void (*i2c_irq_hdl)(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2);

struct i2c_device {
    struct dev_obj dev;
};

struct i2c_hal_ops{
    struct devobj_ops ops;
    int32(*open)(struct i2c_device *i2c, enum i2c_mode mode, enum i2c_addr_mode addr_mode, uint32 addr);
    int32(*close)(struct i2c_device *i2c);
    int32(*ioctl)(struct i2c_device *i2c, int32 cmd, uint32 param);
    int32(*baudrate)(struct i2c_device *i2c, uint32 baudrate);
    int32(*read)(struct i2c_device *i2c, int8 *addr, uint32 addr_len, int8 *buf, uint32 len);
    int32(*write)(struct i2c_device *i2c, int8 *addr, uint32 addr_len, int8 *buf, uint32 len);
    int32(*write_scatter)(struct i2c_device *i2c, int8 *addr, uint32 addr_len, scatter_data *data, uint32 count);
    int32(*write_table)(struct i2c_device *i2c, uint8 cmd_len, uint8 slave_id, scatter_data *data);
    int32(*request_irq)(struct i2c_device *i2c, i2c_irq_hdl irqhdl, uint32 irq_data, uint32 irq_flag);
    int32(*release_irq)(struct i2c_device *i2c, uint32 irq_flag);
};

/**
  * @}
  */


/** @defgroup HGI2C_DW_Exported_Functions
  * @{
  */

int32 i2c_open(struct i2c_device *i2c, enum i2c_mode mode, enum i2c_addr_mode addr_mode, uint32 addr);
int32 i2c_close(struct i2c_device *i2c);
int32 i2c_set_baudrate(struct i2c_device *i2c, uint32 baudrate);
int32 i2c_ioctl(struct i2c_device *i2c, uint32 cmd, uint32 param);
int32 i2c_read(struct i2c_device *i2c, int8 *addr, uint32 addr_len, int8 *buf, uint32 buf_len);
int32 i2c_write(struct i2c_device *i2c, int8 *addr, uint32 addr_len, int8 *buf, uint32 buf_len);
int32 i2c_write_scatter(struct i2c_device *i2c, int8 *addr, uint32 addr_len, scatter_data *data, uint32 count);
int32 i2c_master_write_table(struct i2c_device *i2c, uint8 cmd_len, uint8 slave_id, scatter_data *data);
int32 i2c_request_irq(struct i2c_device *i2c, i2c_irq_hdl handle, uint32 irq_data, uint32 irq_flag);
int32 i2c_release_irq(struct i2c_device *i2c, uint32 irq_flag);

/**
  * @}
  */


#ifdef __cplusplus
}
#endif
#endif

