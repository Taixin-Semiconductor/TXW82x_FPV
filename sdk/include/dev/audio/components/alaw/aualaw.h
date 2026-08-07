#ifndef _AUALAW_H
#define _AUALAW_H

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief AUALAW irq_flag type
  */
enum aualaw_irq_flag {

    /*!
     * @brief:
     * 
     */
    AUALAW_IRQ_FLAG_HALF = BIT(0),

    /*!
     * @brief:
     * 
     */
    AUALAW_IRQ_FLAG_FULL = BIT(1),
};

/**
  * @brief AUALAW ioctl_cmd type
  */
enum aualaw_ioctl_cmd {

    /*!
     * @brief:
     * 
     */
    AUALAW_IOCTL_CMD_NONE,
};



/* User interrupt handle */
typedef void (*aualaw_irq_hdl)(uint32 irq, uint32 irq_data);

/* AUALAW api for user */

struct aualaw_device {
    struct dev_obj dev;
};

struct aualaw_hal_ops {
    struct devobj_ops ops;
    int32(*open)(struct aualaw_device *aualaw);
    int32(*close)(struct aualaw_device *aualaw);
    int32(*encode)(struct aualaw_device *aualaw, void* dat_buf, uint32 bytes, void* result_buf);
    int32(*decode)(struct aualaw_device *aualaw, void* dat_buf, uint32 bytes, void* result_buf);
    int32(*ioctl)(struct aualaw_device *aualaw, enum aualaw_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct aualaw_device *aualaw, enum aualaw_irq_flag irq_flag, aualaw_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct aualaw_device *aualaw, enum aualaw_irq_flag irq_flag);
};



/* AUALAW API functions */



/**
 * @brief aualaw_open
 *
 *
 * @note  .
 *
 * @param aualaw       The address of aualaw device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 aualaw_open(struct aualaw_device *aualaw);

/**
 * @brief aualaw_close
 *
 *
 * @note  .
 *
 * @param aualaw       The address of aualaw device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 aualaw_close(struct aualaw_device *aualaw);

/**
 * @brief aualaw_encode
 *
 *
 * @note  .
 *
 * @param aualaw       The address of aualaw device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 aualaw_encode(struct aualaw_device *aualaw, void* dat_buf, uint32 bytes, void* result_buf);

/**
 * @brief aualaw_decode
 *
 *
 * @note  .
 *
 * @param aualaw       The address of aualaw device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 aualaw_decode(struct aualaw_device *aualaw, void* dat_buf, uint32 bytes, void* result_buf);

/**
 * @brief aualaw_ioctl
 *
 *
 * @note  .
 *
 * @param aualaw       The address of aualaw device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 aualaw_ioctl(struct aualaw_device *aualaw, enum aualaw_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);

/**
 * @brief aualaw_request_irq
 *
 *
 * @note  .
 *
 * @param aualaw       The address of aualaw device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 aualaw_request_irq(struct aualaw_device *aualaw, enum aualaw_irq_flag irq_flag, aualaw_irq_hdl irq_hdl, uint32 irq_data);

/**
 * @brief aualaw_release_irq
 *
 *
 * @note  .
 *
 * @param aualaw       The address of aualaw device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 aualaw_release_irq(struct aualaw_device *aualaw, enum aualaw_irq_flag irq_flag);

#ifdef __cplusplus
}
#endif
#endif
