#ifndef _AUDAC_H
#define _AUDAC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief AUDAC sample_rate type
  */
enum audac_sample_rate {

    AUDAC_SAMPLE_RATE_8K,

    AUDAC_SAMPLE_RATE_11_025K,

    AUDAC_SAMPLE_RATE_12K,
    
    AUDAC_SAMPLE_RATE_16K,

    AUDAC_SAMPLE_RATE_22_05K,

    AUDAC_SAMPLE_RATE_24K,

    AUDAC_SAMPLE_RATE_32K,

    AUDAC_SAMPLE_RATE_44_1K,

    AUDAC_SAMPLE_RATE_48K,
};

/**
  * @brief AUDAC irq_flag type
  */
enum audac_irq_flag {

    /*!
     * @brief:
     * 
     */
    AUDAC_IRQ_FLAG_HALF = BIT(0),

    /*!
     * @brief:
     * 
     */
    AUDAC_IRQ_FLAG_FULL = BIT(1),
};

/**
  * @brief AUDAC ioctl_cmd type
  */
enum audac_ioctl_cmd {

    /*!
     * @brief:
     * 
     */
    AUDAC_IOCTL_CMD_SET_SAMPLE_RATE,

    /*!
     * @brief:
     * 
     */
    AUDAC_IOCTL_CMD_SET_SOUND_CHANNEL,
    
    /*!
     * @brief: Adjust the power digital gain
     * 
     */
    AUDAC_IOCTL_CMD_SET_DIGITAL_GAIN,
    
    /*!
     * @brief: Adjust the power analog gain
     * 
     */
    AUDAC_IOCTL_CMD_SET_ANALOG_GAIN, 
    
    /*!
     * @brief: Soft mute
     * 
     */
    AUDAC_IOCTL_CMD_SOFT_MUTE,

	
    /*!
     * @brief: Change sample rate
     * @note :
     *         1.Ensure DAC is muted by special method
     * 		   2.You should call "audac_write" API after using this cmd
     * 
     */
    AUDAC_IOCTL_CMD_CHANGE_SAMPLE_RATE, 
	
	AUDAC_IOCTL_CMD_GET_DIGITAL_GAIN,
};



/* User interrupt handle */
typedef void (*audac_irq_hdl)(uint32 irq, uint32 irq_data);

/* AUDAC api for user */

struct audac_device {
    struct dev_obj dev;
};

struct audac_hal_ops {
    struct devobj_ops ops;
    int32(*open)(struct audac_device *audac, enum audac_sample_rate sample_rate);
    int32(*close)(struct audac_device *audac);
    int32(*write)(struct audac_device *audac, void* buf, uint32 bytes);
    int32(*ioctl)(struct audac_device *audac, enum audac_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct audac_device *audac, enum audac_irq_flag irq_flag, audac_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct audac_device *audac, enum audac_irq_flag irq_flag);
};



/* AUDAC API functions */



/**
 * @brief audac_open
 *
 *
 * @note  .
 *
 * @param audac       The address of audac device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 audac_open(struct audac_device *audac, enum audac_sample_rate sample_rate);

/**
 * @brief audac_close
 *
 *
 * @note  .
 *
 * @param audac       The address of audac device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 audac_close(struct audac_device *audac);

/**
 * @brief audac_write
 *
 *
 * @note  .
 *
 * @param audac       The address of audac device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 audac_write(struct audac_device *audac, void* buf, uint32 bytes);

/**
 * @brief audac_ioctl
 *
 *
 * @note  .
 *
 * @param audac       The address of audac device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 audac_ioctl(struct audac_device *audac, enum audac_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);

/**
 * @brief audac_request_irq
 *
 *
 * @note  .
 *
 * @param audac       The address of audac device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 audac_request_irq(struct audac_device *audac, enum audac_irq_flag irq_flag, audac_irq_hdl irq_hdl, uint32 irq_data);

/**
 * @brief audac_release_irq
 *
 *
 * @note  .
 *
 * @param audac       The address of audac device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 audac_release_irq(struct audac_device *audac, enum audac_irq_flag irq_flag);

#ifdef __cplusplus
}
#endif
#endif
