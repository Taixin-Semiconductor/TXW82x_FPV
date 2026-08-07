#ifndef _AUVAD_H
#define _AUVAD_H

#ifdef __cplusplus
extern "C" {
#endif


/**
  * @brief AUVAD calculate mode types
  */
enum auvad_calc_mode {
	AUVAD_CALC_MODE_ENERGY = BIT(0),
	
	AUVAD_CALC_MODE_ZCR    = BIT(1),
};


/**
  * @brief AUVAD irq_flag type
  */
enum auvad_irq_flag {

    /*!
     * @brief:
     * 
     */
    NONE = BIT(0),
};

/**
  * @brief AUVAD ioctl_cmd type
  */
enum auvad_ioctl_cmd {

    /*!
     * @brief:
     * 
     */
    AUVAD_IOCTL_CMD_SET_CALC_MODE,

	/*!
     * @brief:
     * 
     */
    AUVAD_IOCTL_CMD_SET_THRESHOLD_TH0,
    
	/*!
     * @brief:
     * 
     */
    AUVAD_IOCTL_CMD_SET_THRESHOLD_TH1,
    
	/*!
     * @brief:
     * 
     */
    AUVAD_IOCTL_CMD_SET_THRESHOLD_ZCR,
};



/* User interrupt handle */
typedef void (*auvad_irq_hdl)(uint32 irq, uint32 irq_data);

/* AUVAD api for user */

struct auvad_device {
    struct dev_obj dev;
};

struct auvad_hal_ops {
    struct devobj_ops ops;
    int32(*open)(struct auvad_device *auvad, enum auvad_calc_mode mode);
    int32(*close)(struct auvad_device *auvad);
    int32(*calc)(struct auvad_device *auvad, void* dat_buf, uint32 bytes, uint32 calc_type, void* result_buf);
    int32(*ioctl)(struct auvad_device *auvad, enum auvad_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct auvad_device *auvad, enum auvad_irq_flag irq_flag, auvad_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct auvad_device *auvad, enum auvad_irq_flag irq_flag);
};



/* AUVAD API functions */



/**
 * @brief auvad_open
 *
 *
 * @note  .
 *
 * @param auvad       The address of auvad device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 auvad_open(struct auvad_device *auvad, enum auvad_calc_mode mode);

/**
 * @brief auvad_close
 *
 *
 * @note  .
 *
 * @param auvad       The address of auvad device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 auvad_close(struct auvad_device *auvad);

/**
 * @brief auvad_encode
 *
 *
 * @note  .
 *
 * @param auvad       The address of auvad device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 auvad_encode(struct auvad_device *auvad, void* dat_buf, uint32 bytes, void* result_buf);

/**
 * @brief auvad_decode
 *
 *
 * @note  .
 *
 * @param auvad       The address of auvad device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 auvad_calc(struct auvad_device *auvad, void* dat_buf, uint32 bytes, uint32 calc_type, void* result_buf);

/**
 * @brief auvad_ioctl
 *
 *
 * @note  .
 *
 * @param auvad       The address of auvad device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 auvad_ioctl(struct auvad_device *auvad, enum auvad_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);

/**
 * @brief auvad_request_irq
 *
 *
 * @note  .
 *
 * @param auvad       The address of auvad device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 auvad_request_irq(struct auvad_device *auvad, enum auvad_irq_flag irq_flag, auvad_irq_hdl irq_hdl, uint32 irq_data);

/**
 * @brief auvad_release_irq
 *
 *
 * @note  .
 *
 * @param auvad       The address of auvad device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 auvad_release_irq(struct auvad_device *auvad, enum auvad_irq_flag irq_flag);

#ifdef __cplusplus
}
#endif
#endif
