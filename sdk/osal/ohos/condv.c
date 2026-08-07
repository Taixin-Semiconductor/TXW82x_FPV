#include "typesdef.h"
#include "errno.h"
#include "list.h"
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
#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/condv.h"


#define CONDV_MAGIC (0x3a8d7c1d)

int32 os_condv_init(os_condv_t *cond)
{
    UINT32 hdl = 0;
    ASSERT(cond);

    if(cond->magic == CONDV_MAGIC){
        os_printf(KERN_WARNING"condv repeat initialization ????\r\n");
    }

    if (LOS_OK == LOS_SemCreate(0, &hdl)) {
        cond->sema = (void *)(hdl + 1);
        cond->magic = CONDV_MAGIC;
        atomic_set(&cond->waitings, 0);
    }
    return (cond->sema ? RET_OK : RET_ERR);
}

int32 os_condv_broadcast(os_condv_t *cond)
{
    uint32 i;
    UINT32 hdl  = (UINT32)cond->sema;
    uint32 wait = atomic_read(&cond->waitings);

    ASSERT(cond && cond->magic == CONDV_MAGIC);
    while (wait > 0) {
        if (wait == atomic_cmpxchg(&cond->waitings, wait, 0)) {
            for (i = 0; i < wait; i++) {
                LOS_SemPost(hdl - 1);
            }
        }
        wait = atomic_read(&cond->waitings);
    }

    return RET_OK;
}

int32 os_condv_signal(os_condv_t *cond)
{
    ASSERT(cond && cond->magic == CONDV_MAGIC);
    UINT32 hdl  = (UINT32)cond->sema;
    uint32 wait = atomic_read(&cond->waitings);
    while (wait > 0) {
        if (wait == atomic_cmpxchg(&cond->waitings, wait, wait - 1)) {
            LOS_SemPost(hdl - 1);
            break;
        }
        wait = atomic_read(&cond->waitings);
    }
    return RET_OK;
}

int32 os_condv_del(os_condv_t *cond)
{
    ASSERT(cond && cond->magic == CONDV_MAGIC);
    UINT32 hdl  = (UINT32)cond->sema;
    LOS_SemDelete(hdl - 1);
    cond->sema = NULL;
    cond->magic = 0;
    return RET_OK;
}

int32 os_condv_wait(os_condv_t *cond, os_mutex_t *mutex, uint32 tmo_ms)
{
    ASSERT(cond && cond->magic == CONDV_MAGIC);
    int32 ret;
    UINT32 hdl  = (UINT32)cond->sema;

    atomic_inc(&cond->waitings);
    os_mutex_unlock(mutex);
    ret = LOS_SemPend(hdl - 1, LOS_MS2Tick(tmo_ms));
    os_mutex_lock(mutex, osWaitForever);
    atomic_dec(&cond->waitings);
    return ret ? -ETIMEDOUT : RET_OK;
}

#endif

