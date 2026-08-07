#include "rtthread.h"

void rt_timer_init(rt_timer_t      timer, const char *name, void (*timeout)(void *parameter), void *parameter, rt_tick_t time, rt_uint8_t flag)
{
    uint8 mode = flag & RT_TIMER_FLAG_PERIODIC ? OS_TIMER_MODE_PERIODIC : OS_TIMER_MODE_ONCE;
    os_timer_init(timer, timeout, mode, parameter);
    timer->interval = time;
}

rt_err_t rt_timer_detach(rt_timer_t timer)
{
    return os_timer_del(timer);
}

rt_timer_t rt_timer_create(const char *name, void (*timeout)(void *parameter), void *parameter, rt_tick_t time, rt_uint8_t flag)
{
    uint8 mode = flag & RT_TIMER_FLAG_PERIODIC ? OS_TIMER_MODE_PERIODIC : OS_TIMER_MODE_ONCE;
    struct rt_timer *timer = os_malloc(sizeof(struct rt_timer));
    if (timer) {
        os_timer_init(timer, timeout, mode, parameter);
        timer->interval = time;
    }
    return timer;
}

rt_err_t rt_timer_delete(rt_timer_t timer)
{
    return os_timer_del(timer);
}

rt_err_t rt_timer_start(rt_timer_t timer)
{
    return os_timer_start(timer, timer->interval);
}

rt_err_t rt_timer_stop(rt_timer_t timer)
{
    return os_timer_stop(timer);
}


