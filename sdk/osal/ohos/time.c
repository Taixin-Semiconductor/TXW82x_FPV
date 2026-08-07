#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/event.h"
#include "osal/irq.h"

#ifdef OHOS
#include "los_event.h"
#include "los_membox.h"
#include "los_memory.h"
#include "los_interrupt.h"
#include "los_mux.h"
#include "los_queue.h"
#include "los_sem.h"
#include "los_swtmr.h"
#include "los_task.h"
#include "los_timer.h"
#include "los_debug.h"
#if (LOSCFG_MUTEX_CREATE_TRACE == 1)
#include "los_arch.h"
#endif
#include "osal/sleep.h"
#include <sys/time.h>

static uint64 sys_time_base_ns = 0;
static uint64 sys_time_real_ns = 0;

uint64 os_jiffies(void)
{
    return LOS_TickCountGet();
}

uint32 os_seconds(void)
{
    return LOS_CurrNanosec()/1000000000;
}

uint64 os_mseconds(void)
{
    return LOS_CurrNanosec()/1000000;
}

uint64 os_useconds(void)
{
    return LOS_CurrNanosec()/1000;
}

void os_systime(struct timespec *tm)
{
    uint64 diff_ns = LOS_CurrNanosec() - sys_time_base_ns;
    tm->tv_sec  = (sys_time_real_ns + diff_ns) / NANOSECONDS_PER_SECOND;
    tm->tv_nsec = (sys_time_real_ns + diff_ns) % NANOSECONDS_PER_SECOND;
}

#ifdef __NEWLIB__
int gettimeofday(struct timeval *ptimeval, void *ptimezone)
#else
int gettimeofday(struct timeval *ptimeval, struct timezone *ptimezone)
#endif
{
    uint64 diff_ns = LOS_CurrNanosec() - sys_time_base_ns;
    ptimeval->tv_sec  = (sys_time_real_ns + diff_ns) / NANOSECONDS_PER_SECOND;
    ptimeval->tv_usec = ((sys_time_real_ns + diff_ns) % NANOSECONDS_PER_SECOND) / 1000;
    return 0;
}

int settimeofday(const struct timeval *tv, const struct timezone *tz)
{
    uint32 f = disable_irq();
    sys_time_base_ns = LOS_CurrNanosec();
    sys_time_real_ns = (uint64)tv->tv_sec * NANOSECONDS_PER_SECOND + (uint64)tv->tv_usec * 1000UL;
    enable_irq(f);
    return 0;
}

time_t time(time_t *t)
{
    time_t v;
    uint64 diff_ns = LOS_CurrNanosec() - sys_time_base_ns;
    v = (sys_time_real_ns + diff_ns) / NANOSECONDS_PER_SECOND;
    if (t) *t = v;
    return v;
}

int clock_gettime(uint32 clk_id, struct timespec *tp)
{
    if(tp){
        uint64 ns   = LOS_CurrNanosec();
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

    if (abstime == NULL || curtime == NULL || result == NULL) {
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
    return LOS_Tick2MS(jiff);
}

uint64 os_msecs_to_jiffies(uint64 msec)
{
    return LOS_MS2Tick(msec);
}

#endif

