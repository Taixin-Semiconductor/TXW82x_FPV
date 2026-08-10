#ifndef _AUASRC_H
#define _AUASRC_H

#ifdef __cplusplus
extern "C" {
#endif


/**
  * @brief AUASRC ioctl_cmd type
  */
enum auasrc_ioctl_cmd {

    /*!
     * @brief:
     * 
     */
    AUASRC_IOCTL_CMD_NONE,
};




/* AUASRC api for user */

struct auasrc_device {
    struct dev_obj dev;
};

struct auasrc_hal_ops {
    struct devobj_ops ops;
    int32(*open)(struct auasrc_device *auasrc);
    int32(*close)(struct auasrc_device *auasrc);
    int32(*run)(struct auasrc_device *auasrc, uint32 hz_from, uint32 hz_to);
    int32(*ioctl)(struct auasrc_device *auasrc, enum auasrc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
};



/* AUASRC API functions */



/**
 * @brief auasrc_open
 *
 *
 * @note  .
 *
 * @param auasrc       The address of auasrc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 auasrc_open(struct auasrc_device *auasrc);

/**
 * @brief auasrc_close
 *
 *
 * @note  .
 *
 * @param auasrc       The address of auasrc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 auasrc_close(struct auasrc_device *auasrc);

/**
 * @brief auasrc_encode
 *
 *
 * @note  .
 *
 * @param auasrc       The address of auasrc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 auasrc_run(struct auasrc_device *auasrc, uint32 hz_from, uint32 hz_to);


/**
 * @brief auasrc_ioctl
 *
 *
 * @note  .
 *
 * @param auasrc       The address of auasrc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 auasrc_ioctl(struct auasrc_device *auasrc, enum auasrc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);

#ifdef __cplusplus
}
#endif
#endif
