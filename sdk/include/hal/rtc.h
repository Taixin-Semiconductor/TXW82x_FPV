#ifndef _RTC_H
#define _RTC_H

#ifdef __cplusplus
extern "C" {
#endif


/**
  * @brief RTC week type
  */
enum rtc_week_type {
    MONDAY     = 1,
    TUESDAY    = 2,
    WEDNESDAY  = 3,
    THURSDAY   = 4,
    FRIDAY     = 5,
    SATURDAY   = 6,
    SUNDAY     = 0,
};

/**
  * @brief RTC time type
  */
struct rtc_time_type {
    uint16 year;
    uint8  month;
    uint8  date;
    uint8  week;
    uint8  hour;
    uint8  minute;
    uint8  second;
};


/**
  * @brief RTC ioctl_cmd type
  */
enum rtc_ioctl_cmd {
    RES,
};

/**
  * @brief RTC irq_flag type
  */
enum rtc_irq_flag {

    /*! Compatible version: V0;
     *@ Describe:
     *
     */
    RTC_IRQ_FLAG_SECOND = BIT(0),
};



/* User interrupt handle */
typedef void (*rtc_irq_hdl)(uint32 irq, uint32 irq_data);

/* RTC api for user */
struct rtc_device {
    struct dev_obj dev;
};

struct rtc_hal_ops{
    struct devobj_ops ops;
    int32(*open)(struct rtc_device *rtc, struct rtc_time_type *rtc_time);
    int32(*close)(struct rtc_device *rtc);
    int32(*set_time)(struct rtc_device *rtc, struct rtc_time_type *rtc_time);
    int32(*get_time)(struct rtc_device *rtc, struct rtc_time_type *rtc_time);
    int32(*ioctl)(struct rtc_device *rtc, uint32 cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct rtc_device *rtc, uint32 irq_flag, rtc_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct rtc_device *rtc, uint32 irq_flag);
};

/* RTC API functions */



/**
 * @brief rtc_open
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param rtc       The address of rtc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 rtc_open(struct rtc_device *rtc, struct rtc_time_type *rtc_time);

/**
 * @brief rtc_close
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param rtc       The address of rtc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 rtc_close(struct rtc_device *rtc);

/**
 * @brief rtc_set_time
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param rtc       The address of rtc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 rtc_set_time(struct rtc_device *rtc, struct rtc_time_type *rtc_time);

/**
 * @brief rtc_get_time
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param rtc       The address of rtc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 rtc_get_time(struct rtc_device *rtc, struct rtc_time_type *rtc_time);

/**
 * @brief rtc_ioctl
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param rtc       The address of rtc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 rtc_ioctl(struct rtc_device *rtc, uint32 cmd, uint32 param1, uint32 param2);

/**
 * @brief rtc_request_irq
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param rtc       The address of rtc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 rtc_request_irq(struct rtc_device *rtc, uint32 irq_flag, rtc_irq_hdl irq_hdl, uint32 irq_data);

/**
 * @brief rtc_release_irq
 *
 * @Compatible version: V0;
 *
 * @note  .
 *
 * @param rtc       The address of rtc device in the RTOS.
 * @param .
 * @param .
 *
 * @return
 *     - RET_OK              Success
 *     - RET_ERR             Fail
 */
int32 rtc_release_irq(struct rtc_device *rtc, uint32 irq_flag);

#ifdef __cplusplus
}
#endif
#endif

