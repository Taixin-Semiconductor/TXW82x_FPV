#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/string.h"
#include "osal/event.h"

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
#include "osal/timer.h"

#define TIMER_MAGIC (0x6a7b3c4d)

static void _os_timer_cb(void *args)
{
    uint64 t1, t2, t3;
    os_timer_t *timer = (os_timer_t *)args;
    timer->trigger_cnt++;
    t1 = os_jiffies();
    timer->cb(timer->data);
    t2 = os_jiffies();
    t3 = t2 - t1;
    timer->total_time += t3;
    if (t3 > timer->max_time) {
        timer->max_time = t3;
    }

    if (timer->mode == OS_TIMER_MODE_ONCE) {
        timer->hdl = NULL;
    }
}

static int32 _os_timer_create(os_timer_t *timer, int32 interval)
{
    int32 ret;
    UINT32 hdl = 0;
#if (LOSCFG_BASE_CORE_SWTMR_ALIGN == 1)
    ret = LOS_SwtmrCreate(interval, timer->mode, (SWTMR_PROC_FUNC)_os_timer_cb, &hdl, (UINT32)timer, 1, 0);
#else
    ret = LOS_SwtmrCreate(interval, timer->mode, (SWTMR_PROC_FUNC)_os_timer_cb, &hdl, (UINT32)timer);
#endif
    if (ret == LOS_OK) {
        timer->hdl = (void *)(hdl + 1);
        timer->magic = TIMER_MAGIC;
    }
    return ret;
}

int os_timer_init(os_timer_t *timer, os_timer_func_t func, enum OS_TIMER_MODE mode, void *arg)
{
    int32 ret = RET_OK;

    if(timer->magic == TIMER_MAGIC){
        os_printf(KERN_WARNING"timer repeat initialization ????\r\n");
    }
    
    memset(timer, 0, sizeof(os_timer_t));
    timer->cb   = func;
    timer->data = arg;
    timer->mode = mode;
    if (mode == OS_TIMER_MODE_PERIODIC) {
        ret = _os_timer_create(timer, 1);
    }
    return ret;
}

int os_timer_start(os_timer_t *timer, unsigned long expires)
{
    int ret = RET_ERR;
    UINT32 intSave;
    UINT32 hdl;
    SWTMR_CTRL_S *swtmr = NULL;

    intSave = LOS_IntLock();
    switch (timer->mode) {
        case OS_TIMER_MODE_ONCE:
            if (timer->hdl) os_timer_del(timer);
            ret = _os_timer_create(timer, LOS_MS2Tick(expires));
            break;
        case OS_TIMER_MODE_PERIODIC:
            if (timer->hdl) {
                hdl = (UINT32)timer->hdl;
                swtmr = OS_SWT_FROM_SID(hdl - 1);
                swtmr->uwInterval = LOS_MS2Tick(expires);
                if (swtmr->uwInterval == 0) {
                    swtmr->uwInterval = 1;
                }
                ret = RET_OK;
            }
            break;
        default:
            break;
    }

    if (ret == LOS_OK) {
        hdl = (UINT32)timer->hdl;
        ret = LOS_SwtmrStart(hdl - 1);
    }
    LOS_IntRestore(intSave);
    return ret;
}

int os_timer_stop(os_timer_t *timer)
{
    int ret = 0;
    UINT32 hdl = (UINT32)timer->hdl;
    //ASSERT(timer->hdl);
    if (timer->hdl) {
        ret = LOS_SwtmrStop(hdl - 1);
    }
    return 0;
}

int os_timer_del(os_timer_t *timer)
{
    int ret = 0;
    UINT32 hdl = (UINT32)timer->hdl;
    //ASSERT(timer->hdl);
    if (timer->hdl) {
        ret = LOS_SwtmrDelete(hdl - 1);
        timer->hdl = 0;
        timer->magic = 0;
    }
    return 0;
}

int os_timer_stat(os_timer_t *timer)
{
    uint32 ticks = 0;
    UINT32 hdl = (UINT32)timer->hdl;
    if (timer->hdl) {
        return LOS_SwtmrTimeGet(hdl - 1, &ticks);
    }
    return ticks ? 1 : 0;
}

#endif

