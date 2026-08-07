#ifndef _HAL_ADC_H_
#define _HAL_ADC_H_

#ifdef __cplusplus
extern "C" {
#endif


/**
  * @brief ADC ioctl_cmd type
  */
enum adc_ioctl_cmd {
    ADC_IOCTL_SET_REF_TMP,
    ADC_IOCTL_GET_VREF,
};

/**
  * @brief ADC irq_flag type
  */
enum adc_irq_flag {

    /*! Compatible version: V0;
     *@ Describe:
     * 
     */
    ADC_IRQ_FLAG_SAMPLE_DONE = BIT(0),
};



/* User interrupt handle */
typedef void (*adc_irq_hdl)(uint32 irq, uint32 channel, uint32 irq_data);


/* ADC api for user */
struct adc_device {
    struct dev_obj dev;
};


struct adc_hal_ops{
    struct devobj_ops ops;
    int32(*open)(struct adc_device *adc);
    int32(*close)(struct adc_device *adc);
    int32(*add_channel)(struct adc_device *adc, uint32 channel);
    int32(*delete_channel)(struct adc_device *adc, uint32 channel);
    int32(*get_value)(struct adc_device *adc, uint32 channel, uint32 *raw_data);
    int32(*ioctl)(struct adc_device *adc, enum adc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct adc_device *adc, enum adc_irq_flag irq_flag, adc_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct adc_device *adc, enum adc_irq_flag irq_flag);
};

/* ADC API functions */



/**
 * @brief adc_open
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param adc       The address of adc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 adc_open(struct adc_device *adc);

/**
 * @brief adc_close
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param adc       The address of adc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 adc_close(struct adc_device *adc);

/**
 * @brief adc_add_channel
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param adc       The address of adc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 adc_add_channel(struct adc_device *adc, uint32 channel);

/**
 * @brief adc_delete_channel
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param adc       The address of adc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 adc_delete_channel(struct adc_device *adc, uint32 channel);

/**
 * @brief adc_get_value
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param adc       The address of adc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 adc_get_value(struct adc_device *adc, uint32 channel, uint32 *raw_data);


/**
 * @brief adc_ioctl
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param adc       The address of adc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 adc_ioctl(struct adc_device *adc, enum adc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);

/**
 * @brief adc_request_irq
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param adc       The address of adc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 adc_request_irq(struct adc_device *adc, enum adc_irq_flag irq_flag, adc_irq_hdl irq_hdl, uint32 irq_data);

/**
 * @brief adc_release_irq
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param adc       The address of adc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 adc_release_irq(struct adc_device *adc, enum adc_irq_flag irq_flag);


int32 adc_get_vref(struct adc_device *adc, float *vref);


#ifdef __cplusplus
}
#endif
#endif
