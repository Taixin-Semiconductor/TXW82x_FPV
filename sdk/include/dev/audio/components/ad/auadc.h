#ifndef _AUADC_H
#define _AUADC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief AUADC sample_rate type
  */
enum auadc_sample_rate {

    AUADC_SAMPLE_RATE_8K,

    AUADC_SAMPLE_RATE_11_025K,

    AUADC_SAMPLE_RATE_16K,

    AUADC_SAMPLE_RATE_12K,

    AUADC_SAMPLE_RATE_22_05K,

    AUADC_SAMPLE_RATE_24K,

    AUADC_SAMPLE_RATE_32K,

    AUADC_SAMPLE_RATE_44_1K,

    AUADC_SAMPLE_RATE_48K,
};

/**
  * @brief AUADC irq_flag type
  */
enum auadc_irq_flag {

    /*!
     * @brief:
     * 
     */
    AUADC_IRQ_FLAG_HALF = BIT(0),

    /*!
     * @brief:
     * 
     */
    AUADC_IRQ_FLAG_FULL = BIT(1),
};

/**
  * @brief AUADC ioctl_cmd type
  */
enum auadc_ioctl_cmd {

    /*!
     * @brief:
     * 
     */
    AUADC_IOCTL_CMD_SET_SAMPLE_RATE,
    /*!
     * @brief:
     * 
     */
    AUADC_IOCTL_CMD_SET_SOUND_CHANNEL,
    
    /*!
     * @brief: Adjust the power digital gain
     * 
     */
    AUADC_IOCTL_CMD_SET_DIGITAL_GAIN,
    
    /*!
     * @brief: Adjust the power analog gain
     * 
     */
    AUADC_IOCTL_CMD_SET_ANALOG_GAIN, 
};



/* User interrupt handle */
typedef void (*auadc_irq_hdl)(uint32 irq, uint32 irq_data);


/* AUADCC api for user */

struct auadc_device {
    struct dev_obj dev;
};


/* AUADC api for user */
struct auadc_hal_ops {
    struct devobj_ops ops;
    int32(*open)(struct auadc_device *auadc, enum auadc_sample_rate sample_rate);
    int32(*close)(struct auadc_device *auadc);
    int32(*read)(struct auadc_device *auadc, void* buf, uint32 bytes);
    int32(*ioctl)(struct auadc_device *auadc, enum auadc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct auadc_device *auadc, enum auadc_irq_flag irq_flag, auadc_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct auadc_device *auadc, enum auadc_irq_flag irq_flag);
};



/* AUADC API functions */



/**
 * @brief auadc_open
 *
 *
 * @note  .
 *
 * @param auadc       The address of auadc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 auadc_open(struct auadc_device *auadc, enum auadc_sample_rate sample_rate);

/**
 * @brief auadc_close
 *
 *
 * @note  .
 *
 * @param auadc       The address of auadc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 auadc_close(struct auadc_device *auadc);

/**
 * @brief auadc_read
 *
 *
 * @note  .
 *
 * @param auadc       The address of auadc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 auadc_read(struct auadc_device *auadc, void* buf, uint32 bytes);

/**
 * @brief auadc_ioctl
 *
 *
 * @note  .
 *
 * @param auadc       The address of auadc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 auadc_ioctl(struct auadc_device *auadc, enum auadc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);

/**
 * @brief auadc_request_irq
 *
 *
 * @note  .
 *
 * @param auadc       The address of auadc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 auadc_request_irq(struct auadc_device *auadc, enum auadc_irq_flag irq_flag, auadc_irq_hdl irq_hdl, uint32 irq_data);

/**
 * @brief auadc_release_irq
 *
 *
 * @note  .
 *
 * @param auadc       The address of auadc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 auadc_release_irq(struct auadc_device *auadc, enum auadc_irq_flag irq_flag);

#ifdef __cplusplus
}
#endif
#endif
