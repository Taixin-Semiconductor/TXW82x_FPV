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

#define SEMA_MAGIC (0x3a8d7c1d)

int32 os_sema_init(os_semaphore_t *sem, int32 val)
{
    UINT32 hdl = 0;
    ASSERT(sem);

    if(sem->magic == SEMA_MAGIC){
        os_printf(KERN_WARNING"sem repeat initialization ????\r\n");
    }

    if (LOS_OK == LOS_SemCreate(val, &hdl)) {
        sem->hdl = (void *)(hdl + 1);
        sem->magic = SEMA_MAGIC;
    } else {
        os_printf(KERN_ERR"os_sema_init fail\r\n");
    }
    return (sem->hdl ? RET_OK : RET_ERR);
}

int32 os_sema_down(os_semaphore_t *sem, int32 tmo_ms)
{
    ASSERT(sem && sem->hdl);
    INT32  ret = 0;
    UINT32 hdl = (UINT32)sem->hdl;
    ret = LOS_SemPend((hdl - 1), LOS_MS2Tick(tmo_ms));
    return (ret == LOS_OK) ? 1 : 0;
}

int32 os_sema_up(os_semaphore_t *sem)
{
    ASSERT(sem && sem->hdl);
    UINT32 hdl = (UINT32)sem->hdl;
    return LOS_SemPost(hdl - 1);
}

int32 os_sema_del(os_semaphore_t *sem)
{
    ASSERT(sem && sem->hdl);
    UINT32 hdl = (UINT32)sem->hdl;
    UINT32 ret = LOS_SemDelete(hdl - 1);
    sem->hdl = NULL;
    sem->magic = 0;
    return ret;
}

int32 os_sema_count(os_semaphore_t *sem)
{
    INT32 val = 0;
    if (sem && sem->hdl) {
        UINT32 hdl = (UINT32)sem->hdl;
        LOS_SemGetValue(hdl - 1, &val);
    }
    return val > 0 ? val : 0;
}

void os_sema_eat(os_semaphore_t *sem)
{
    int32 ret = 0;
    while (os_sema_count(sem) > 0) {
        ret = os_sema_down(sem, 0);
        if (!ret) {
            break;
        }
    }
}

#endif

