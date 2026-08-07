#ifndef _HAL_I2S_H_
#define _HAL_I2S_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief i2s bit width per sample type
  */
enum i2s_sample_bits {
    /*! Compatible version: V0
     *@Describe:
     * i2s bit width per sample: 8bits
     */
    I2S_SAMPLE_BITS_8BITS,
    /*! Compatible version: V0
     *@Describe:
     * i2s bit width per sample: 16bits
     */
    I2S_SAMPLE_BITS_16BITS,
    /*! Compatible version: V0
     *@Describe:
     * i2s bit width per sample: 24bits
     */
    I2S_SAMPLE_BITS_24BITS,
};

/**
  * @brief i2s sample frequency type
  */
enum i2s_sample_freq {

    I2S_SAMPLE_FREQ_8K,


    I2S_SAMPLE_FREQ_16K,

    /*! Compatible version: V0
     *@Describe:
     * i2s sample frequency: 44.1K(WS frequency)
     */
    I2S_SAMPLE_FREQ_44_1K,
    /*! Compatible version: V0
     *@Describe:
     * i2s sample frequency: 48K(WS frequency)
     */
    I2S_SAMPLE_FREQ_48K,
};


/**
  * @brief i2s channel type
  */
enum i2s_channel {
    /*! Compatible version: V0
     *@Describe:
     * i2s mono channel
     */
    I2S_CHANNEL_MONO,
    /*! Compatible version: V0
     *@Describe:
     * i2s stereo channel
     */
    I2S_CHANNEL_STEREO,
};

/**
  * @brief i2s data format type
  */
enum i2s_data_fmt {
    /*! Compatible version: V0
     *@Describe:
     * I2S communication format I2S
     */
    I2S_DATA_FMT_I2S,

    /*! Compatible version: V0
     *@Describe:
     * I2S communication format MSB(right justified)
     */
    I2S_DATA_FMT_MSB,

    /*! Compatible version: V0
     *@Describe:
     * I2S communication format LSB(left justified)
     */
    I2S_DATA_FMT_LSB,

    /*! Compatible version: V0
     *@Describe:
     * I2S communication format PCM
     */
    I2S_DATA_FMT_PCM,
};

/**
  * @brief i2s mode type
  */
enum i2s_mode {
    /*! Compatible version: V0
     *@Describe:
     * I2S master mode setting
     */
    I2S_MODE_MASTER,
    /*! Compatible version: V0
     *@Describe:
     * I2S slave mode setting
     */
    I2S_MODE_SLAVE,
};

/**
  * @brief spi ioctl cmd
  *
  */
enum i2s_ioctl_cmd {
    /*! Compatible version: V0
     *@Describe:
     * Set the polarity of WSCLK
     *@The specific Settings:
     * (1)left channel WSCLK level is low , right channel WSCLK level is high
     * (2)left channel WSCLK level is high, right channel WSCLK level is low
     */
    I2S_IOCTL_CMD_SET_WSCLK_POL,

    /*! Compatible version: V0
     *@Describe:
     * i2s bit width per sample
     *@The specific Settings:
     * (1)I2S_SAMPLE_BITS_8BITS
     * (2)I2S_SAMPLE_BITS_16BITS
     * (3)I2S_SAMPLE_BITS_24BITS
     */
    I2S_IOCTL_CMD_SET_SAMPLE_BITS,

    /*! Compatible version: V0
     *@Describe:
     * i2s bit width per sample
     *@The specific Settings:
     * (1)I2S_CHANNEL_MONO
     * (2)I2S_CHANNEL_STEREO
     */
    I2S_IOCTL_CMD_SET_CHANNEL,

    /*! Compatible version: V0
     *@Describe:
     * i2s communication format
     *@The specific Settings:
     * (1)I2S_DATA_FMT_I2S
     * (2)I2S_DATA_FMT_MSB
     * (3)I2S_DATA_FMT_LSB
     */
    I2S_IOCTL_CMD_SET_DATA_FMT,

    /*! Compatible version: V0
     *@Describe:
     * i2s open WSCLK & BCLK & DATA debounce
     * (Only for i2s slave)
     */
    I2S_IOCTL_CMD_SET_DEBOUNCE,

    /*! Compatible version: V0
     *@Describe:
     * i2s open duplex using iis0 & iis1
     */
    I2S_IOCTL_CMD_SET_DUPLEX,
};

/**
  * @brief spi request interrupt flag
  *
  */
enum i2s_irq_flag {
    /*! Compatible version: V0
     *@Describe:
     * I2S interrupt signal that transmits half of the data
     */
    I2S_IRQ_FLAG_HALF  = BIT(0),
    /*! Compatible version: V0
     *@Describe:
     * I2S interrupt signal that transmits all data
     */
    I2S_IRQ_FLAG_FULL  = BIT(1),
    
    I2S_IRQ_FLAG_RESET = BIT(2),
};


/* User interrupt handle */
typedef void (*i2s_irq_hdl)(uint32 irq, uint32 irq_data);

/* I2S api for user */
struct i2s_device {
    struct dev_obj dev;
};

struct i2s_hal_ops{
    struct devobj_ops ops;
    int32(*open)(struct i2s_device *i2s, enum i2s_mode mode, enum i2s_sample_freq frequency, enum i2s_sample_bits bits);
    int32(*close)(struct i2s_device *i2s);
    int32(*ioctl)(struct i2s_device *i2s, uint32 cmd, uint32 param);
    int32(*read)(struct i2s_device *i2s, void *buf, uint32 len);
    int32(*write)(struct i2s_device *i2s, const void *buf, uint32 len);
    int32(*write_scatter)(struct i2s_device *i2s, const scatter_data *data, uint32 count);
    int32(*request_irq)(struct i2s_device *i2s, uint32 irq_flag, i2s_irq_hdl irqhdl, uint32 irq_data);
    int32(*release_irq)(struct i2s_device *i2s, uint32 irq_flag);
};


/* I2S API functions */

/**
 * @brief i2s_open.
 *
 * @Compatible version: V0
 *
 * @note  This function must be called before any I2S driver operations.
 *
 * @param i2s       The address of I2S device in the RTOS.
 * @param mode      I2S work mode setting        - see "enum i2s_mode".
 * @param frequency I2S sample frequency setting - see "enum i2s_sample_freq".
 * @param bits      I2S sample bits setting      - see "enum i2s_sample_bits".
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 i2s_open(struct i2s_device *i2s, enum i2s_mode mode, enum i2s_sample_freq frequency, enum i2s_sample_bits bits);

/**
 * @brief i2s_close.
 *
 * @Compatible version: V0
 *
 * @param i2s       The address of I2S device in the RTOS.
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 i2s_close(struct i2s_device *i2s);

/**
 * @brief i2s_write.
 *
 * @Compatible version: V0
 *
 * @param i2s       The address of I2S device in the RTOS.
 * @param buf       Write the start address of buf.
 * @param len       Write length, uint: byte.
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 i2s_write(struct i2s_device *i2s, const void *buf, uint32 len);

int32 i2s_write_scatter(struct i2s_device *i2s, const scatter_data *data, uint32 count);

/**
 * @brief i2s_read.
 *
 * @Compatible version: V0
 *
 * @param i2s       The address of I2S device in the RTOS.
 * @param buf       Read the start address of buf.
 * @param len       Read length, uint: byte.
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 i2s_read(struct i2s_device *i2s, void *buf, uint32 len);

/**
 * @brief i2s_ioctl.
 *
 * @Compatible version: V0
 *
 * @param i2s       The address of I2S device in the RTOS.
 * @param cmd       Control command - see"enum i2s_ioctl_cmd".
 * @param param     Parameters required to execute the current Control command.
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 i2s_ioctl(struct i2s_device *i2s, uint32 cmd, uint32 param);

/**
 * @brief i2s_request_irq.
 *
 * @Compatible version: V0
 *
 * @param i2s       The address of I2S device in the RTOS.
 * @param irq_flag  The type of interrupt - see"enum i2s_irq_flag".
 * @param irqhdl    The interrupt function.
 * @param irq_data  The parameters of the interrupt function
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 i2s_request_irq(struct i2s_device *i2s, uint32 irq_flag, i2s_irq_hdl irqhdl, uint32 irq_data);

/**
 * @brief i2s_release_irq.
 *
 * @Compatible version: V0
 *
 * @param i2s       The address of I2S device in the RTOS.
 * @param irq_flag  The type of interrupt - see"enum i2s_irq_flag".
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 i2s_release_irq(struct i2s_device *i2s, uint32 irq_flag);

#ifdef __cplusplus
}
#endif

#endif
