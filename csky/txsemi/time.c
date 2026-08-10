#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/sleep.h"
#include "osal/irq.h"

#include "csi_kernel.h"
#include <k_api.h>
#include <sys/time.h>

extern uint32_t g_cpuloading;
extern uint64_t g_sys_tick_count;
static uint64 sys_time_base_ns = 0;
static uint64 sys_time_real_ns = 0;

uint64 os_jiffies(void)
{
    return (g_sys_tick_count);
}

uint32 os_cpuloading(void)
{
    return g_cpuloading;
}

uint32 os_seconds(void)
{
    return os_jiffies_to_msecs(g_sys_tick_count)/1000;
}

uint64 os_mseconds(void)
{
    return krhino_curr_nanosec()/1000000;
}

uint64 os_useconds(void)
{
    return krhino_curr_nanosec()/1000;
}

void os_systime(struct timespec *tm)
{
    uint64 diff_ns = krhino_curr_nanosec() - sys_time_base_ns;
    tm->tv_sec  = (sys_time_real_ns + diff_ns) / NANOSECONDS_PER_SECOND;
    tm->tv_nsec = (sys_time_real_ns + diff_ns) % NANOSECONDS_PER_SECOND;
}

#ifdef __NEWLIB__
int gettimeofday(struct timeval *ptimeval, void *ptimezone)
#else
int gettimeofday(struct timeval *ptimeval, struct timezone *ptimezone)
#endif
{
    uint64 diff_ns = krhino_curr_nanosec() - sys_time_base_ns;
    ptimeval->tv_sec  = (sys_time_real_ns + diff_ns) / NANOSECONDS_PER_SECOND;
    ptimeval->tv_usec = ((sys_time_real_ns + diff_ns) % NANOSECONDS_PER_SECOND) / 1000;
    return 0;
}

int gettimeofday2(struct timeval *ptimeval, uint64 msec)
{
    int64 diff_ns = (msec * 1000 * 1000) - sys_time_base_ns;
    ptimeval->tv_sec  = (sys_time_real_ns + diff_ns) / NANOSECONDS_PER_SECOND;
    ptimeval->tv_usec = ((sys_time_real_ns + diff_ns) % NANOSECONDS_PER_SECOND) / 1000;
    return 0;
}

int settimeofday(const struct timeval *tv, const struct timezone *tz)
{
    uint32 f = disable_irq();
    sys_time_base_ns = krhino_curr_nanosec();
    sys_time_real_ns = (uint64)tv->tv_sec * NANOSECONDS_PER_SECOND + (uint64)tv->tv_usec * 1000UL;
    enable_irq(f);
    return 0;
}

time_t time(time_t *t)
{
    uint64 v;
    uint64 diff_ns = krhino_curr_nanosec() - sys_time_base_ns;
    v = (sys_time_real_ns + diff_ns) / NANOSECONDS_PER_SECOND;
    if (t) *t = (time_t)v;
    return (time_t)v;
}

int clock_gettime(uint32 clk_id, struct timespec *tp)
{
    if(tp){
        uint64 ns   = krhino_curr_nanosec();
        tp->tv_sec  = ns / NANOSECONDS_PER_SECOND;
        tp->tv_nsec = ns % NANOSECONDS_PER_SECOND;
    }
    return 0;
}

int32 timespec_validate(const struct timespec *time)
{
    int32 ret = 0;

    if (time != NULL) {
        /* Verify 0 <= tv_nsec < 1000000000. */
        if ((time->tv_nsec >= 0) && (time->tv_nsec < NANOSECONDS_PER_SECOND)) {
            ret = 1;
        }
    }

    return ret;
}

int32 timespec_cmp(const struct timespec *x, const struct timespec *y)
{
    int32 ret = 0;

    /* Check parameters */
    if ((x == NULL) && (y == NULL)) {
        ret = 0;
    } else if (y == NULL) {
        ret = 1;
    } else if (x == NULL) {
        ret = -1;
    } else if (x->tv_sec > y->tv_sec) {
        ret = 1;
    } else if (x->tv_sec < y->tv_sec) {
        ret = -1;
    } else {
        /* seconds are equal compare nano seconds */
        if (x->tv_nsec > y->tv_nsec) {
            ret = 1;
        } else if (x->tv_nsec < y->tv_nsec) {
            ret = -1;
        } else {
            ret = 0;
        }
    }

    return ret;
}


int32 timespec_to_ticks(const struct timespec *time, uint64 *result)
{
    int32 ret = 0;
    uint64 llTotalTicks = 0;
    long lNanoseconds = 0;

    /* Check parameters. */
    if ((time == NULL) || (result == NULL)) {
        ret = -EINVAL;
    } else if ((ret == 0) && (timespec_validate(time) == FALSE)) {
        ret = -EINVAL;
    } else {
        /* Convert timespec.tv_sec to ticks. */
        llTotalTicks = (uint64) OS_SYSTICK_HZ * (time->tv_sec);

        /* Convert timespec.tv_nsec to ticks. This value does not have to be checked
         * for overflow because a valid timespec has 0 <= tv_nsec < 1000000000 and
         * NANOSECONDS_PER_TICK > 1. */
        lNanoseconds = time->tv_nsec / (long) NANOSECONDS_PER_TICK +                    /* Whole nanoseconds. */
                       (long)(time->tv_nsec % (long) NANOSECONDS_PER_TICK != 0);        /* Add 1 to round up if needed. */

        /* Add the nanoseconds to the total ticks. */
        llTotalTicks += (uint64) lNanoseconds;

        /* Write result. */
        *result = (uint64) llTotalTicks;
    }

    return ret;
}

void nanosec_to_timespec(int64 llSource, struct timespec *time)
{
    long lCarrySec = 0;

    /* Convert to timespec. */
    time->tv_sec = (time_t)(llSource / NANOSECONDS_PER_SECOND);
    time->tv_nsec = (long)(llSource % NANOSECONDS_PER_SECOND);

    /* Subtract from tv_sec if tv_nsec < 0. */
    if (time->tv_nsec < 0L) {
        /* Compute the number of seconds to carry. */
        lCarrySec = (time->tv_nsec / (long) NANOSECONDS_PER_SECOND) + 1L;

        time->tv_sec -= (time_t)(lCarrySec);
        time->tv_nsec += lCarrySec * (long) NANOSECONDS_PER_SECOND;
    }
}

int32 timespec_add(const struct timespec *x, const struct timespec *y, struct timespec *result)
{
    int64 llPartialSec = 0;
    int32 ret = 0;

    /* Check parameters. */
    if ((result == NULL) || (x == NULL) || (y == NULL)) {
        return -1;
    }

    /* Perform addition. */
    result->tv_nsec = x->tv_nsec + y->tv_nsec;

    /* check for overflow in case nsec value was invalid */
    if (result->tv_nsec < 0) {
        ret = 1;
    } else {
        llPartialSec = (result->tv_nsec) / NANOSECONDS_PER_SECOND;
        result->tv_nsec = (result->tv_nsec) % NANOSECONDS_PER_SECOND;
        result->tv_sec = x->tv_sec + y->tv_sec + llPartialSec;

        /* check for overflow */
        if (result->tv_sec < 0) {
            ret = 1;
        }
    }

    return ret;
}

int32 timespec_add_nanosec(const struct timespec *x, int64 llNanoseconds, struct timespec *result)
{
    int64 llTotalNSec = 0;
    int32 ret = 0;

    /* Check parameters. */
    if ((result == NULL) || (x == NULL)) {
        return -1;
    }

    /* add nano seconds */
    llTotalNSec = x->tv_nsec + llNanoseconds;

    /* check for nano seconds overflow */
    if (llTotalNSec < 0) {
        ret = 1;
    } else {
        result->tv_nsec = llTotalNSec % NANOSECONDS_PER_SECOND;
        result->tv_sec = x->tv_sec + (llTotalNSec / NANOSECONDS_PER_SECOND);

        /* check for seconds overflow */
        if (result->tv_sec < 0) {
            ret = 1;
        }
    }

    return ret;
}

int32 timespec_sub(const struct timespec *x, const struct timespec *y, struct timespec *result)
{
    int32 cmp_ret = 0;
    int32 ret = 0;

    /* Check parameters. */
    if ((result == NULL) || (x == NULL) || (y == NULL)) {
        return -1;
    }

    cmp_ret = timespec_cmp(x, y);

    /* if x < y then result would be negative, return 1 */
    if (cmp_ret == -1) {
        ret = 1;
    } else if (cmp_ret == 0) {
        /* if times are the same return zero */
        result->tv_sec = 0;
        result->tv_nsec = 0;
    } else {
        /* If x > y Perform subtraction. */
        result->tv_sec = x->tv_sec - y->tv_sec;
        result->tv_nsec = x->tv_nsec - y->tv_nsec;

        /* check if nano seconds value needs to borrow */
        if (result->tv_nsec < 0) {
            /* Based on comparison, tv_sec > 0 */
            result->tv_sec--;
            result->tv_nsec += (long) NANOSECONDS_PER_SECOND;
        }

        /* if nano second is negative after borrow, it is an overflow error */
        if (result->tv_nsec < 0) {
            ret = -1;
        }
    }

    return ret;
}

int32 timespec_detal_ticks(const struct timespec *abstime, const struct timespec *curtime, uint64 *result)
{
    int32 ret = 0;
    struct timespec diff = { 0 };

    if(abstime == NULL || curtime == NULL || result == NULL){
        return -EINVAL;
    }

    ret = timespec_sub(abstime, curtime, &diff);

    if (ret == 1) {
        /* abstime was in the past. */
        ret = -ETIMEDOUT;
    } else if (ret == -1) {
        /* error */
        ret = -EINVAL;
    }

    /* Convert the time difference to ticks. */
    if (ret == 0) {
        ret = timespec_to_ticks(&diff, result);
    }

    return ret;
}

uint64 os_jiffies_to_msecs(uint64 jiff)
{
    return ((jiff)*OS_MS_PERIOD_TICK);
}

uint64 os_msecs_to_jiffies(uint64 msec)
{
    return ((msec)/OS_MS_PERIOD_TICK);
}

/* 判断闰年 */
static int is_leap(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/* 每月天数（非闰年） */
static const int month_days[12] = {
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
};

struct tm *gmtime_r(const time_t *timep, struct tm *result) {
    if (timep == NULL || result == NULL)
        return NULL;

    time_t total_sec = *timep;
    const long SECS_PER_DAY = 86400;
    const long SECS_PER_HOUR = 3600;
    const long SECS_PER_MIN = 60;

    /* 计算天数（自1970-01-01起）和当天剩余秒数 */
    long days = total_sec / SECS_PER_DAY;
    long rem = total_sec % SECS_PER_DAY;
    if (rem < 0) {          /* 使余数非负 */
        rem += SECS_PER_DAY;
        days--;
    }

    /* 计算星期几：1970-01-01 是星期四 (tm_wday = 4) */
    long wday = (4 + days) % 7;
    if (wday < 0) wday += 7;

    /* 计算时、分、秒 */
    int hour = rem / SECS_PER_HOUR;
    int minute = (rem % SECS_PER_HOUR) / SECS_PER_MIN;
    int second = rem % SECS_PER_MIN;

    /* 计算年份，将 days 转换为一年的第几天 */
    int year = 1970;
    /* 处理负天数（1970年之前） */
    while (days < 0) {
        --year;
        days += is_leap(year) ? 366 : 365;
    }
    /* 处理正天数 */
    while (days >= (is_leap(year) ? 366 : 365)) {
        days -= is_leap(year) ? 366 : 365;
        ++year;
    }
    /* 此时 days 为从当年1月1日起的天数偏移（0-based） */
    int yday = days;

    /* 计算月份和日期 */
    int month;
    for (month = 0; month < 12; ++month) {
        int md = month_days[month];
        if (month == 1 && is_leap(year))  /* 二月闰年多一天 */
            md = 29;
        if (days < md)
            break;
        days -= md;
    }
    int mday = days + 1;   /* 日期转换为1-based */

    /* 填写结构体 */
    result->tm_sec   = second;
    result->tm_min   = minute;
    result->tm_hour  = hour;
    result->tm_mday  = mday;
    result->tm_mon   = month;          /* 0 = 一月 */
    result->tm_year  = year - 1900;
    result->tm_wday  = wday;
    result->tm_yday  = yday;
    result->tm_isdst = 0;              /* UTC 没有夏令时 */

    return result;
}

