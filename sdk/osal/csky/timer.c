#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/timer.h"
#include "osal/string.h"

#ifdef CSKY_OS
#include "csi_kernel.h"

#define TIMER_MAGIC (0x6a7b3c4d)

static void _os_timer_cb(void *args)
{
    uint32 t1, t2, t3;
    os_timer_t *timer = (os_timer_t *)args;
    timer->trigger_cnt++;
    t1 = 0;
    timer->cb(timer->data);
    t2 = 0;
    t3 = t2 - t1;
    timer->total_time += t3;
    if (t3 > timer->max_time) {
        timer->max_time = t3;
    }
}
int os_timer_init(os_timer_t *timer, os_timer_func_t func,
                  enum OS_TIMER_MODE mode, void *arg)
{
    ASSERT(timer && func);
    if(timer->magic == TIMER_MAGIC){
        os_printf(KERN_WARNING"timer repeat initialization ????\r\n");
    }

    timer->cb   = func;
    timer->data = arg;
    timer->total_time = 0;
    timer->max_time = 0;
    timer->trigger_cnt = 0;
    timer->hdl = csi_kernel_timer_new(_os_timer_cb, mode, timer);
    if(timer->hdl) timer->magic = TIMER_MAGIC;
    return timer->hdl ? 0 : -1;
}

int os_timer_start(os_timer_t *timer, unsigned long expires)
{
    //ASSERT(timer->hdl);
    if (timer->hdl) {
        return csi_kernel_timer_start(timer->hdl, csi_kernel_ms2tick(expires));
    }
    return 0;
}

int os_timer_stop(os_timer_t *timer)
{
    int ret = 0;
    //ASSERT(timer->hdl);
    if (timer->hdl) {
        ret = csi_kernel_timer_stop(timer->hdl);
    }
    return 0;
}

int os_timer_del(os_timer_t *timer)
{
    int ret = 0;
    //ASSERT(timer->hdl);
    if (timer->hdl) {
        ret = csi_kernel_timer_del(timer->hdl);
        timer->hdl = 0;
        timer->magic = 0;
    }
    return 0;
}

int os_timer_stat(os_timer_t *timer)
{
    if (timer->hdl) {
        return csi_kernel_timer_get_stat(timer->hdl);
    }
    return 0;
}

#endif

