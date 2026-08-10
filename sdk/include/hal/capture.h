#ifndef _HAL_CAPTURE_H_
#define _HAL_CAPTURE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief CAPTURE channel type
  */
enum capture_channel {

    /*! Compatible version: V0;
     *@ Describe:
     * 
     */
    CAPTURE_CHANNEL_0,

    /*! Compatible version: V0;
     *@ Describe:
     * 
     */
    CAPTURE_CHANNEL_1,

    /*! Compatible version: V0;
     *@ Describe:
     * 
     */
    CAPTURE_CHANNEL_2,

    /*! Compatible version: V0;
     *@ Describe:
     * 
     */
    CAPTURE_CHANNEL_3,
};

/**
  * @brief CAPTURE mode type
  */
enum capture_mode {

    /*! Compatible version: V0;
     *@ Describe:
     * 
     */
    CAPTURE_MODE_RISE,

    /*! Compatible version: V0;
     *@ Describe:
     * 
     */
    CAPTURE_MODE_FALL,

    /*! Compatible version: V0;
     *@ Describe:
     * 
     */
    CAPTURE_MODE_ALL,
};

/**
  * @brief CAPTURE irq_flag type
  */
enum capture_irq_flag {

    /*! Compatible version: V0;
     *@ Describe:
     * 
     */
    CAPTURE_IRQ_FLAG_CAPTURE,

    /*! Compatible version: V0;
     *@ Describe:
     * 
     */
    CAPTURE_IRQ_FLAG_OVERFLOW,
};

/**
  * @brief CAPTURE ioctl_cmd type
  */
enum capture_ioctl_cmd {
    NONE,
};



/* User interrupt handle */
typedef void (*capture_irq_hdl)(uint32 irq, uint32 irq_data);


/* CAPTURE api for user */
struct capture_device {
    struct dev_obj dev;
};

struct capture_hal_ops{
    struct devobj_ops ops;
    int32(*init)(struct capture_device *cap, enum capture_channel channel, enum capture_mode mode);
    int32(*deinit)(struct capture_device *cap, enum capture_channel channel);
    int32(*start)(struct capture_device *cap, enum capture_channel channel);
    int32(*stop)(struct capture_device *cap, enum capture_channel channel);
    int32(*suspend)(struct capture_device *cap, enum capture_channel channel);
    int32(*resume)(struct capture_device *cap, enum capture_channel channel);
    int32(*ioctl)(struct capture_device *cap, enum capture_channel channel, enum capture_ioctl_cmd cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct capture_device *cap, enum capture_channel channel, enum capture_irq_flag irq_flag, capture_irq_hdl irq_hdl, uint32 data);
    int32(*release_irq)(struct capture_device *cap, enum capture_channel channel);
};


/* CAPTURE API functions */



/**
 * @brief capture_init
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param capture       The address of capture device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 capture_init(struct capture_device *capture, enum capture_channel channel, enum capture_mode mode);

/**
 * @brief capture_deinit
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param capture       The address of capture device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 capture_deinit(struct capture_device *capture, enum capture_channel channel);

/**
 * @brief capture_start
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param capture       The address of capture device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 capture_start(struct capture_device *capture, enum capture_channel channel);

/**
 * @brief capture_stop
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param capture       The address of capture device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 capture_stop(struct capture_device *capture, enum capture_channel channel);

/**
 * @brief capture_suspend channel
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param capture       The address of capture device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 capture_suspend(struct capture_device *capture, enum capture_channel channel);

/**
 * @brief capture_resume channel
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param capture       The address of capture device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 capture_resume(struct capture_device *capture, enum capture_channel channel);

/**
 * @brief capture_ioctl
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param capture       The address of capture device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 capture_ioctl(struct capture_device *capture, enum capture_channel channel, enum capture_ioctl_cmd cmd, uint32 param1, uint32 param2);

/**
 * @brief capture_request_irq
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param capture       The address of capture device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 capture_request_irq(struct capture_device *capture, enum capture_channel channel, enum capture_irq_flag irq_flag, capture_irq_hdl irq_hdl, uint32 data);

/**
 * @brief capture_release_irq
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param capture       The address of capture device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 capture_release_irq(struct capture_device *capture, enum capture_channel channel);

#ifdef __cplusplus
}
#endif
#endif
