#ifndef __OS_TIME_H_
#define __OS_TIME_H_
#ifdef __cplusplus
extern "C" {
#endif

#define MICROSECONDS_PER_SECOND    ( 1000000LL )                                   /**< Microseconds per second. */
#define NANOSECONDS_PER_SECOND     ( 1000000000LL )                                /**< Nanoseconds per second. */
#define NANOSECONDS_PER_TICK       ( NANOSECONDS_PER_SECOND / OS_SYSTICK_HZ ) /**< Nanoseconds per FreeRTOS tick. */

uint64 os_jiffies(void);      //系统开机后经过的 tick数
uint32 os_seconds(void);      //系统开机后经过的 秒数
uint64 os_mseconds(void); //系统开机后经过的 毫秒数
uint64 os_useconds(void); //系统开机后经过的 微秒数
uint32 os_cpuloading(void);

int clock_gettime(uint32 clk_id, struct timespec *tp);

void os_systime(struct timespec *tm);
int32 timespec_detal_ticks(const struct timespec *abstime, const struct timespec *curtime, uint64 *result);
int32 timespec_to_ticks(const struct timespec *time, uint64 *result);
void nanosec_to_timespec(int64 llSource, struct timespec *time);
int32 timespec_add(const struct timespec *x, const struct timespec *y, struct timespec *result);
int32 timespec_add_nanosec(const struct timespec *x, int64 llNanoseconds, struct timespec *result);
int32 timespec_sub(const struct timespec *x, const struct timespec *y, struct timespec *result);
int32 timespec_cmp(const struct timespec *x, const struct timespec *y);
int32 timespec_validate(const struct timespec *time);

uint64 os_jiffies_to_msecs(uint64 jiff); //tick数 转换为 毫秒数
uint64 os_msecs_to_jiffies(uint64 msec); //毫秒数 转换为 tick数

#ifdef __cplusplus
}
#endif

#endif

