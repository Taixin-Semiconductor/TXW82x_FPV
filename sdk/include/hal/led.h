#ifndef _HAL_LED_H_
#define _HAL_LED_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief LED work_mode type
  */
enum led_work_mode {

    /*! Compatible version: V0;
     *@ Describe:
     * Work mode for common anode digital tube
     */
    LED_WORK_MODE_COMMON_ANODE,

    /*! Compatible version: V0;
     *@ Describe:
     * Work mode for common cathode digital tube
     */
    LED_WORK_MODE_COMMON_CATHODE,
};

/**
  * @brief LED seg_cnt type
  */
enum led_seg_cnt {

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of segument: 1 piece
     */
    LED_SEG_CNT_1,

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of segument: 2 piece
     */
    LED_SEG_CNT_2,

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of segument: 3 piece
     */
    LED_SEG_CNT_3,

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of segument: 4 piece
     */
    LED_SEG_CNT_4,

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of segument: 5 piece
     */
    LED_SEG_CNT_5,

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of segument: 6 piece
     */
    LED_SEG_CNT_6,

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of segument: 7 piece
     */
    LED_SEG_CNT_7,

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of segument: 8 piece
     */
    LED_SEG_CNT_8,

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of segument: 9 piece
     */
    LED_SEG_CNT_9,

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of segument: 10 piece
     */
    LED_SEG_CNT_10,

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of segument: 11 piece
     */
    LED_SEG_CNT_11,

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of segument: 12 piece
     */
    LED_SEG_CNT_12,
};

/**
  * @brief LED com_cnt type
  */
enum led_com_cnt {

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of com: 1 piece
     *@ Note:
     * It is also position for "led_open" function's parameter
     */
    LED_COM_CNT_1,

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of com: 2 piece
     *@ Note:
     * It is also position for "led_open" function's parameter
     */
    LED_COM_CNT_2,

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of com: 3 piece
     *@ Note:
     * It is also position for "led_open" function's parameter
     */
    LED_COM_CNT_3,

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of com: 4 piece
     *@ Note:
     * It is also position for "led_open" function's parameter
     */
    LED_COM_CNT_4,

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of com: 5 piece
     *@ Note:
     * It is also position for "led_open" function's parameter
     */
    LED_COM_CNT_5,

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of com: 6 piece
     *@ Note:
     * It is also position for "led_open" function's parameter
     */
    LED_COM_CNT_6,

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of com: 7 piece
     *@ Note:
     * It is also position for "led_open" function's parameter
     */
    LED_COM_CNT_7,

    /*! Compatible version: V0;
     *@ Describe:
     * Amount of com: 8 piece
     *@ Note:
     * It is also position for "led_open" function's parameter
     */
    LED_COM_CNT_8,
};

/**
  * @brief LED irq_flag type
  */
enum led_irq_flag {

    /*! Compatible version: V0;
     *@ Describe:
     * emmm...
     */
    LED_IRQ_FLAG_INT = BIT(0),
};

/**
  * @brief LED ioctl_cmd type
  */
enum led_ioctl_cmd {

    /*! Compatible version: V0;
     *@ Describe:
     * Set scan frequency and set the length of time
     */
    LED_IOCTL_CMD_SET_SCAN_FREQ,
};



/* User interrupt handle */
typedef void (*led_irq_hdl)(uint32 irq, uint32 irq_data);

/* LED api for user */
struct led_device {
    struct dev_obj dev;
};

struct led_hal_ops{
    struct devobj_ops ops;
    int32(*open)(struct led_device *led, enum led_work_mode work_mode, enum led_seg_cnt seg_cnt, enum led_com_cnt com_cnt);
    int32(*on)(struct led_device *led, uint32 num, enum led_com_cnt pos);
    int32(*off)(struct led_device *led, enum led_com_cnt pos);
    int32(*close)(struct led_device *led);
    int32(*ioctl)(struct led_device *led, enum led_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct led_device *led, enum led_irq_flag irq_flag, led_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct led_device *led, enum led_irq_flag irq_flag);
};


/* LED API functions */



/**
 * @brief led_open
 *
 * @Compatible version: V0;
 *
 * @note  This function must be called before any led driver operations.
 *
 * @param led       The address of led device in the RTOS.
 * @param work_mode The tube.work mode setting   - see "enum led_work_mode".
 * @param seg_cnt   The tube's segmument amounts - see "enum led_seg_cnt".
 * @param com_cnt   The tube's com amounts       - see "enum led_com_cnt".
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 led_open(struct led_device *led, enum led_work_mode work_mode, enum led_seg_cnt seg_cnt, enum led_com_cnt com_cnt);

/**
 * @brief led_on
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param led       The address of led device in the RTOS.
 * @param num       What does the user want to display the numbers, must be 0~9.
 * @param pos       Where does the user want to display the numbers - see "enum led_com_cnt".
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 led_on(struct led_device *led, uint32 num, enum led_com_cnt pos);

/**
 * @brief led_off
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param led       The address of led device in the RTOS.
 * @param num       What does the user want to display the numbers, must be 0~9.
 * @param pos       Where does the user want to display the numbers - see "enum led_com_cnt".
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 led_off(struct led_device *led, enum led_com_cnt pos);


/**
 * @brief led_close
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param led       The address of led device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 led_close(struct led_device *led);

/**
 * @brief led_ioctl
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param led       The address of led device in the RTOS.
 * @param ioctl_cmd The ioctl command - - see "enum led_ioctl_cmd".
 * @param param1    The parameter to function.
 * @param param2    The parameter to function.
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 led_ioctl(struct led_device *led, enum led_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);

/**
 * @brief led_request_irq
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param led       The address of led device in the RTOS.
 * @param irq_flag  The type of interrupt - see"enum led_irq_flag".
 * @param irqhdl    The interrupt function.
 * @param irq_data  The parameter of the interrupt function
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 led_request_irq(struct led_device *led, enum led_irq_flag irq_flag, led_irq_hdl irq_hdl, uint32 irq_data);

/**
 * @brief led_release_irq
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param led       The address of led device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 led_release_irq(struct led_device *led, enum led_irq_flag irq_flag);

#ifdef __cplusplus
}
#endif
#endif
