#include "sys_config.h"
#include "typesdef.h"
#include "errno.h"
#include "osal/semaphore.h"
#include "osal/task.h"
#include "osal/string.h"
#include "osal/timer.h"
#include "osal/mutex.h"
#include "lib/common/rbuffer.h"
#include "lib/posix/pthread.h"
#include "osal/sleep.h"

#ifdef TXWSDK_POSIX

#define TIMESPEC_IS_ZERO(tm)        (tm.tv_sec == 0 && tm.tv_nsec == 0)
#define TIMESPEC_IS_NOT_ZERO(tm)    (!(TIMESPEC_IS_ZERO( tm)))

struct posix_timer {
    os_timer_t timer;
    struct sigevent event;
    uint64 period_time;
    uint64 expired_time;
};

void posix_timer_cb(void *arg)
{
    pthread_t pthd;
    struct posix_timer *ptimer = (struct posix_timer *)(arg);

    /* Update the timer period, which may need to be set to an it_interval
     * argument. This call should not block. */
    if (ptimer->period_time > 0) {
        os_timer_start(&ptimer->timer, ptimer->period_time);
    }

    /* Create the timer notification thread if requested. */
    if (ptimer->event.sigev_notify == SIGEV_THREAD) {
        /* if the user has provided thread attributes, create a thread
         * with the provided attributes. Otherwise dispatch callback directly */
        if (ptimer->event.sigev_notify_attributes == NULL) {
            (*ptimer->event.sigev_notify_function)(ptimer->event.sigev_value);
        } else {
            pthread_create(&pthd,
                           ptimer->event.sigev_notify_attributes,
                           (void *(*)(void *))ptimer->event.sigev_notify_function,
                           ptimer->event.sigev_value.sival_ptr);
        }
    }
}

int timer_create(clockid_t clockid, struct sigevent *evp, timer_t *timerid)
{
    int ret = 0;
    struct posix_timer *ptimer = NULL;

    /* POSIX specifies that when evp is NULL, the behavior shall be as is
     * sigev_notify is SIGEV_SIGNAL. SIGEV_SIGNAL is currently not supported. */
    if ((evp == NULL) || (evp->sigev_notify == SIGEV_SIGNAL)) {
        return -ENOTSUP;
    }

    ptimer = os_zalloc(sizeof(struct posix_timer));
    if (ptimer == NULL) {
        pthread_err("alloc fail\r\n");
        return -ENOMEM;
    }

    ptimer->event = *evp;
    if (ptimer->event.sigev_notify_attributes && ptimer->event.sigev_notify_attributes->detachstate != PTHREAD_CREATE_DETACHED) {
        pthread_err("timer sigev_notify_attributes detachstate is NOT DETACHED\r\n");
        ptimer->event.sigev_notify_attributes->detachstate = PTHREAD_CREATE_DETACHED;
    }
    ret = os_timer_init(&ptimer->timer, posix_timer_cb, OS_TIMER_MODE_ONCE, ptimer);
    if (!ret) {
        *timerid = (timer_t)ptimer;
    } else {
        pthread_err("os_timer_init fail\r\n");
        os_free(ptimer);
    }
    return ret;
}

int timer_delete(timer_t timerid)
{
    struct posix_timer *ptimer = (struct posix_timer *)(timerid);
    os_timer_stop(&ptimer->timer);
    while (os_timer_stat(&ptimer->timer)) {
        os_sleep_ms(1);
    }
    os_timer_del(&ptimer->timer);
    os_free(ptimer);
    return 0;
}

int timer_getoverrun(timer_t timerid)
{
    return 0;
}

int timer_settime(timer_t timerid, int flags, const struct itimerspec *value, struct itimerspec *ovalue)
{
    int ret = 0;
    struct posix_timer *ptimer = (struct posix_timer *)(timerid);
    uint64 expired_jiff = 0;
    uint64 period_jiff  = 0;

    /* Validate the value argument, but only if the timer isn't being disarmed. */
    if (TIMESPEC_IS_NOT_ZERO(value->it_value)) {
        if (!timespec_validate(&value->it_interval) || !timespec_validate(&value->it_value)) {
            return -EINVAL;
        }
    }

    /* Set ovalue, if given. */
    if (ovalue != NULL) {
        timer_gettime(timerid, ovalue);
    }

    /* Stop the timer if it's currently active. */
    if (os_timer_stat(&ptimer->timer)) {
        os_timer_stop(&ptimer->timer);
    }

    /* Only restart the timer if it_value is not zero. */
    if (TIMESPEC_IS_NOT_ZERO(value->it_value)) {
        /* Convert it_interval to ticks, but only if it_interval is not 0. If
         * it_interval is 0, then the timer is not periodic. */
        if (TIMESPEC_IS_NOT_ZERO(value->it_interval)) {
            timespec_to_ticks(&value->it_interval, &period_jiff);
        }

        /* Set the new timer period. A non-periodic timer will have its period set
         * to portMAX_DELAY. */
        ptimer->period_time = period_jiff;

        /* Convert it_value to ticks, but only if it_value is not 0. If it_value
         * is 0, then the timer will remain disarmed. */
        if (TIMESPEC_IS_NOT_ZERO(value->it_value)) {
            /* Absolute timeout. */
            if ((flags & TIMER_ABSTIME) == TIMER_ABSTIME) {
                struct timespec cur_tm = { 0 };

                /* Get current time */
                if (clock_gettime(CLOCK_REALTIME, &cur_tm) != 0) {
                    ret = -EINVAL;
                } else {
                    ret = timespec_detal_ticks(&value->it_value, &cur_tm, &expired_jiff);
                }

                /* Make sure expired_jiff is zero in case we got negative time difference */
                if (ret != 0) {
                    expired_jiff = 0;
                    if (ret == -ETIMEDOUT) {
                        /* Set Status to 0 as absolute time is past is treated as expiry but not an error */
                        ret = 0;
                    }
                }
            } else {/* Relative timeout. */
                timespec_to_ticks(&value->it_value, &expired_jiff);
            }
        }

        /* If expired_jiff is still 0, that means that it_value specified
         * an absolute timeout in the past. Per POSIX spec, a notification should be
         * triggered immediately. */
        if (expired_jiff == 0) {
            posix_timer_cb(ptimer);
        } else {
            ptimer->expired_time = os_jiffies() + expired_jiff;
            os_timer_start(&ptimer->timer, os_jiffies_to_msecs(expired_jiff));
        }
    }

    return ret;
}

int timer_gettime(timer_t timerid, struct itimerspec *value)
{
    struct posix_timer *ptimer = (struct posix_timer *)(timerid);
    uint64 expired_time = ptimer->expired_time - os_jiffies();
    uint64 period_time  = ptimer->period_time;

    /* Set it_value only if the timer is armed. Otherwise, set it to 0. */
    if (os_timer_stat(&ptimer->timer)) {
        value->it_value.tv_sec = (time_t)(expired_time / OS_SYSTICK_HZ);
        value->it_value.tv_nsec = (long)((expired_time % OS_SYSTICK_HZ) * NANOSECONDS_PER_TICK);
    } else {
        value->it_value.tv_sec = 0;
        value->it_value.tv_nsec = 0;
    }

    /* Set it_interval only if the timer is periodic. Otherwise, set it to 0. */
    if (period_time != osWaitForever) {
        value->it_interval.tv_sec = (time_t)(period_time / OS_SYSTICK_HZ);
        value->it_interval.tv_nsec = (long)((period_time % OS_SYSTICK_HZ) * NANOSECONDS_PER_TICK);
    } else {
        value->it_interval.tv_sec = 0;
        value->it_interval.tv_nsec = 0;
    }

    return 0;
}

#if 0 //see: include/osal/csky/time.h
void timeradd(struct timeval *a, struct timeval *b, struct timeval *res)
{
    res->tv_sec = a->tv_sec + b->tv_sec;
    res->tv_usec = a->tv_usec + b->tv_usec;
    if (res->tv_usec >= 1000000) {
        ++res->tv_sec;
        res->tv_usec -= 1000000;
    }
}

void timersub(struct timeval *a, struct timeval *b, struct timeval *res)
{
    res->tv_sec = a->tv_sec - b->tv_sec;
    res->tv_usec = a->tv_usec - b->tv_usec;
    if (res->tv_usec < 0) {
        --res->tv_sec;
        res->tv_usec += 1000000;
    }
}
#endif

#endif

