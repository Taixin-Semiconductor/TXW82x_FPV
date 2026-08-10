#ifndef _HAL_SPI_H_
#define _HAL_SPI_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief spi working mode type
  */
enum spi_work_mode {
    /*! Compatible version: V0; V1; V2
     *@Describe:
     * The host mode of SPI
     */
    SPI_MASTER_MODE,

    /*! Compatible version: V0; V1; V2
     *@Describe:
     * The slave mode of SPI
     */
    SPI_SLAVE_MODE,

    /*! Compatible version: V1
     *@Describe:
     * The slave FSM mode of SPI0, only for spi0
     */
    SPI_SLAVE_FSM_MODE,
};

/**
  * @brief spi wire mode
  */
enum spi_wire_mode {
    /*! Compatible version: V0; V1; V2
     *@Describe:
     * The wire_single_mode of SPI, contain: CLK; CS; IO0
     * A CLK transmits 1bit of data
     */
    SPI_WIRE_SINGLE_MODE,

    /*! Compatible version: V0; V1; V2
     *@Describe:
     * The wire_normal_mode of SPI, contain: CLK; CS; IO0; IO1
     * A CLK transmits 1bit of data
     */
    SPI_WIRE_NORMAL_MODE,

    /*! Compatible version: V0; V1
     *@Describe:
     * The wire_normal_mode of SPI, contain: CLK; CS; IO0; IO1
     * A CLK transmits 2bit of data
     */
    SPI_WIRE_DUAL_MODE,

    /*! Compatible version: V0; V1
     *@Describe:
     * The wire_normal_mode of SPI, contain: CLK; CS; IO0; IO1; IO2; IO3
     * A CLK transmits 4bit of data
     */
    SPI_WIRE_QUAD_MODE,
};

/**
  * @brief spi clock mode
  */
enum spi_clk_mode {
    /*! Compatible version: V0; V1; V2
     *@Describe:
     * Inactive state of serial clock is low, serial clock toggles in middle of first data bit
     */
    SPI_CPOL_0_CPHA_0 = 0,

    /*! Compatible version: V0; V1; V2
     *@Describe:
     * Inactive state of serial clock is low, serial clock toggles at start of first data bit
     */
    SPI_CPOL_0_CPHA_1 = 1,

    /*! Compatible version: V0; V1; V2
     *@Describe:
     * Inactive state of serial clock is high, serial clock toggles in middle of first data bit
     */
    SPI_CPOL_1_CPHA_0 = 2,

    /*! Compatible version: V0; V1; V2
     *@Describe:
     * Inactive state of serial clock is high, serial clock toggles at start of first data bit
     */
    SPI_CPOL_1_CPHA_1 = 3,

    SPI_CLK_MODE_0    = SPI_CPOL_0_CPHA_0,  /*!< Equal to \ref SPI_CPOL_0_CPHA_0 */
    SPI_CLK_MODE_1    = SPI_CPOL_0_CPHA_1,  /*!< Equal to \ref SPI_CPOL_0_CPHA_1 */
    SPI_CLK_MODE_2    = SPI_CPOL_1_CPHA_0,  /*!< Equal to \ref SPI_CPOL_1_CPHA_0 */
    SPI_CLK_MODE_3    = SPI_CPOL_1_CPHA_1   /*!< Equal to \ref SPI_CPOL_1_CPHA_1 */
};

/**
  * @brief spi ioctl cmd
  *
  */
enum spi_ioctl_cmd {

    /*! Compatible version: V0; V1
     *@Describe:
     * SPI line mode setting
     */
    SPI_WIRE_MODE_SET,

    /*! Compatible version: V0; V1
     *@Describe:
     * Get SPI line mode current setting
     */
    SPI_WIRE_MODE_GET,

    /*! Compatible version: V1
     *@Describe:
     * SPI host sampling delay
     */
    SPI_SAMPLE_DELAY,

    /*! Compatible version: V0; V1
     *@Describe:
     * Set SPI frame size
     */
    SPI_SET_FRAME_SIZE,

    /*! Compatible version: V1
     *@Describe:
     * Sets the length threshold for using DMA, unit: byte
     */
    SPI_SET_LEN_THRESHOLD,

    /*! Set SPI CS value.
     */
    SPI_SET_CS,

    /*! Set SPI uses none cs pin.
     */
    SPI_CFG_SET_NONE_CS,

    /*! Read DMA RX cnt.
     */
    SPI_READ_DMA_RX_CNT,

    /*! Kick SPI dma rx.
     */
    SPI_KICK_DMA_RX,

    /*! Kick SPI dma tx.
     */
    SPI_KICK_DMA_TX,

    /*! set SPI write/read timeout.
     */
    SPI_SET_TIMEOUT,

    /*! set SPI LSB/MSB first.
     */
    SPI_SET_LSB_FIRST,

    /*!
     * Only for XIP
     */
    SPI_XIP_SET_ADR,

    /*!
     * Only for XIP
     */
    SPI_XIP_ERASE_4KB,

    /*!
     * Only for XIP
     */
    SPI_XIP_ERASE_64KB,

    /*!
     * Only for XIP
     */
    SPI_XIP_ERASE_SECURITY_REG,


    /*!
     * Only for XIP
     */
    SPI_XIP_PROGRAM_SECURITY_REG,


    /*!
     * Only for XIP
     */
    SPI_XIP_READ_SECURITY_REG,


    SPI_SET_SLAVE_SW_RESERVED,

    /*
    *  spi sensor
    */
    SPI_SENSOR_VSYNC_TIMEOUT,

    SPI_SENSOR_VSYNC_HEAD_CNT,

    SPI_SENSOR_VSYNC_EN,

    SPI_SENSOR_HSYNC_TIMEOUT,

    SPI_SENSOR_HSYNC_HEAD_CNT,

    SPI_SENSOR_BUF_LEN,

    SPI_SENSOR_SET_ADR0,

    SPI_SENSOR_SET_ADR1,

    SPI_SENSOR_PINGPANG_EN,

    SPI_HIGH_SPEED_CFG,

    SPI_KICK_DMA_EN,

    SPI_RX_TIMEOUT_CFG,

    SPI_SENSOR_LINE_LEN_CFG,

    //新增加命令都往后添加
    SPI_XIP_CUSTOM_READ_CMD,
    SPI_XIP_CUSTOM_WRITE_CMD,
    SPI_XIP_CUSTOM_ERASE_CMD,

    SPI_GET_DMA_PENDING,

    SPI_CLEAR_DMA_PENDING,
    SPI_XIP_CUSTOM_WP_RANGE_CMD,
    SPI_XIP_CUSTOM_WP_EN_CMD,
    SPI_XIP_CUSTOM_DIS_ENCRY_RANGE0,
    SPI_XIP_CUSTOM_DIS_ENCRY_RANGE1,
    SPI_XIP_CUSTOM_SET_REMAP,   //配置reamp地址,SPI_XIP_CUSTOM_SET_ENCRYPT要配置1才能生效
    SPI_XIP_CUSTOM_SET_ENCRYPT, //配置是否要强行加密
    SPI_XIP_CUSTOM_CFG_RUN_OFFSET,
    SPI_XIP_CUSTOM_CFG_ENCRYPT,
    SPI_XIP_GET_SPI_ADDR,       //传入flash绝对地址,返回代码相对地址(比如传入0,返回0x18000000,如果返回0,就是错误的意思,remap一定要比绝对地址小)

    SPI_XIP_REG_OPT,

    SPI_XIP_DMA_LOCK,
    SPI_XIP_DMA_UNLOCK,

    SPI_XIP_WIP_SUSPEND, 
    SPI_XIP_WIP_RESUME,
    SPI_XIP_WIP_SYNC,
    
    SPI_XIP_DTR_INIT,
    SPI_XIP_DTR_EN,  

    SPI_XIP_REGS_OPT,

    SPI_XIP_LOCK,
    SPI_XIP_UNLOCK,

};



enum xip_opcode_cmd {
    XIP_READ,
    XIP_WRITE,
    XIP_ERASE,
    XIP_SECURITY_ERASE,
    XIP_SECURITY_WRITE,
    XIP_SECURITY_READ,
};


/**
  * @brief spi interrupt
  *
  */

enum spi_irq_flag {
    /*! Compatible version: V0; V1; V2
     *@Describe:
     * Transmits the currently configured length of tx data done
     */
    SPI_IRQ_FLAG_TX_DONE                  = BIT(0),

    /*! Compatible version: V0; V1; V2
     *@Describe:
     * Transmits the currently configured length of rx data done
     */
    SPI_IRQ_FLAG_RX_DONE                  = BIT(1),

    /*! Compatible version: V0; V1
     *@Describe:
     * The received buffer is full, max: 40bit
     */
    SPI_IRQ_FLAG_FIFO_OVERFLOW            = BIT(2),

    /*! Compatible version: V0; V1
     *@Describe:
     * Dectect the CS rising
     */
    SPI_IRQ_FLAG_CS_RISING                = BIT(3),

    SPI_IRQ_FLAG_SLAVE_FSM_READ_STATUS    = BIT(4),

    SPI_IRQ_FLAG_SLAVE_FSM_WRONG_CMD      = BIT(5),

    SPI_IRQ_FLAG_RX_TIMEOUT               = BIT(6),

    SPI_IRQ_FLAG_RESET                    = BIT(7),
};


typedef void (*spi_irq_hdl)(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2);


struct spi_device {
    struct dev_obj dev;
};

struct spi_hal_ops {
    struct devobj_ops ops;
    int32(*open)(struct spi_device *p_spi, uint32 clk_freq, uint32 work_mode, uint32 wire_mode, uint32 clk_mode);
    int32(*close)(struct spi_device *p_spi);
    int32(*ioctl)(struct spi_device *p_spi, uint32 cmd, uint32 param1, uint32 param2);
    int32(*read)(struct spi_device *p_spi, void *buf, uint32 size);
    int32(*write)(struct spi_device *p_spi, const void *buf, uint32 size);
    int32(*write_scatter)(struct spi_device *p_spi, const scatter_data *data, uint32 count);
    int32(*request_irq)(struct spi_device *p_spi, uint32 irq_flag, spi_irq_hdl irqhdl, uint32 irq_data);
    int32(*release_irq)(struct spi_device *spi, uint32 irq_flag);
};

int32 spi_open(struct spi_device *p_spi, uint32 clk_freq, uint32 work_mode, uint32 wire_mode, uint32 clk_mode);
int32 spi_close(struct spi_device *p_spi);
int32 spi_ioctl(struct spi_device *p_spi, uint32 cmd, uint32 param1, uint32 param2);
int32 spi_read(struct spi_device *p_spi, void *buf, uint32 size);
int32 spi_write(struct spi_device *p_spi, const void *buf, uint32 size);
int32 spi_write_scatter(struct spi_device *p_spi, const scatter_data *data, uint32 count);
int32 spi_set_cs(struct spi_device *p_spi, uint32 cs, uint32 value);
int32 spi_request_irq(struct spi_device *p_spi, uint32 irq_flag, spi_irq_hdl irqhdl, uint32 irq_data);
int32 spi_release_irq(struct spi_device *spi, uint32 irq_flag);



#ifdef __cplusplus
}
#endif

#endif

