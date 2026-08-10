#ifndef _AUFADE_H
#define _AUFADE_H

#ifdef __cplusplus
extern "C" {
#endif


/**
  * @brief AUFADE calculate mode types
  */
enum aufade_mode {
	AUFADE_IN  = BIT(0),
	
	AUFADE_OUT = BIT(1),
};


/**
  * @brief AUFADE irq_flag type
  */
enum aufade_irq_flag {

    /*!
     * @brief:
     * 
     */
    AUFADE_IRQ_FLAG_NONE = BIT(0),
};

/**
  * @brief AUFADE ioctl_cmd type
  */
enum aufade_ioctl_cmd {

    AUFADE_IOCTL_CMD_GET_CURRENT_VOL = BIT(0),

    AUFADE_IOCTL_CMD_CHECK_IS_DONE   = BIT(1),
};



/* User interrupt handle */
typedef void (*aufade_irq_hdl)(uint32 irq, uint32 irq_data);

/* AUFADE api for user */

struct aufade_device {
    struct dev_obj dev;
};

struct aufade_hal_ops {
    struct devobj_ops ops;
    int32(*open)(struct aufade_device *aufade);
    int32(*close)(struct aufade_device *aufade);
    int32(*start)(struct aufade_device *aufade, uint32 from, uint32 to);
    int32(*ioctl)(struct aufade_device *aufade, enum aufade_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct aufade_device *aufade, enum aufade_irq_flag irq_flag, aufade_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct aufade_device *aufade, enum aufade_irq_flag irq_flag);
};



/* AUFADE API functions */



/**
 * @brief aufade_open
 *
 *
 * @note  .
 *
 * @param aufade       The address of aufade device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 aufade_open(struct aufade_device *aufade);

/**
 * @brief aufade_close
 *
 *
 * @note  .
 *
 * @param aufade       The address of aufade device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 aufade_close(struct aufade_device *aufade);

/**
 * @brief aufade_start
 *
 *
 * @note  .
 *
 * @param aufade       The address of aufade device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 aufade_start(struct aufade_device *aufade, uint32 from, uint32 to);

/**
 * @brief aufade_ioctl
 *
 *
 * @note  .
 *
 * @param aufade       The address of aufade device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 aufade_ioctl(struct aufade_device *aufade, enum aufade_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);

/**
 * @brief aufade_request_irq
 *
 *
 * @note  .
 *
 * @param aufade       The address of aufade device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 aufade_request_irq(struct aufade_device *aufade, enum aufade_irq_flag irq_flag, aufade_irq_hdl irq_hdl, uint32 irq_data);

/**
 * @brief aufade_release_irq
 *
 *
 * @note  .
 *
 * @param aufade       The address of aufade device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 aufade_release_irq(struct aufade_device *aufade, enum aufade_irq_flag irq_flag);


int32 aufade_get_cur_vol(struct aufade_device *aufade, uint32 *vol);

int32 aufade_check_is_done(struct aufade_device *aufade, uint32 *result);


#ifdef __cplusplus
}
#endif
#endif
