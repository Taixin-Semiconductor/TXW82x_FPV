#ifndef _AUDRC_H
#define _AUDRC_H

#ifdef __cplusplus
extern "C" {
#endif


/**
  * @brief AUDRC ioctl_cmd type
  */
enum audrc_ioctl_cmd {

    /*!
     * @brief:
     * 
     */
    AUDRC_IOCTL_CMD_NONE,
};




/* AUDRC api for user */

struct audrc_device {
    struct dev_obj dev;
};

struct audrc_hal_ops {
    struct devobj_ops ops;
    int32(*open)(struct audrc_device *audrc);
    int32(*close)(struct audrc_device *audrc);
    int32(*run)(struct audrc_device *audrc, void *p_content, uint32 bytes);
    int32(*ioctl)(struct audrc_device *audrc, enum audrc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
};



/* AUDRC API functions */



/**
 * @brief audrc_open
 *
 *
 * @note  .
 *
 * @param audrc       The address of audrc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 audrc_open(struct audrc_device *audrc);

/**
 * @brief audrc_close
 *
 *
 * @note  .
 *
 * @param audrc       The address of audrc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 audrc_close(struct audrc_device *audrc);

/**
 * @brief audrc_encode
 *
 *
 * @note  .
 *
 * @param audrc       The address of audrc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 audrc_run(struct audrc_device *audrc, void *p_content, uint32 bytes);


/**
 * @brief audrc_ioctl
 *
 *
 * @note  .
 *
 * @param audrc       The address of audrc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 audrc_ioctl(struct audrc_device *audrc, enum audrc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);

#ifdef __cplusplus
}
#endif
#endif
