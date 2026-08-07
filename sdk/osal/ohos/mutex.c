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
#include "osal/mutex.h"

#define MUTEX_MAGIC (0xa8b4c2d5)

int32 os_mutex_init(os_mutex_t *mutex)
{
    UINT32 hdl = 0;
    
    if(mutex->magic == MUTEX_MAGIC){
        os_printf(KERN_WARNING"mutex repeat initialization ????\r\n");
    }

    if (LOS_OK == LOS_MuxCreate(&hdl)) {
        mutex->hdl = (void *)(hdl+1);
        mutex->magic = MUTEX_MAGIC;
    }
    return (mutex->hdl ? RET_OK : RET_ERR);
}

int32 os_mutex_lock(os_mutex_t *mutex, int32 tmo)
{
    ASSERT(mutex && mutex->hdl);
    UINT32 hdl = (UINT32)mutex->hdl;
    return LOS_MuxPend(hdl - 1, LOS_MS2Tick(tmo), (uint32_t)RETURN_ADDR());
}

int32 os_mutex_unlock(os_mutex_t *mutex)
{
    ASSERT(mutex && mutex->hdl);
    UINT32 hdl = (UINT32)mutex->hdl;
    return LOS_MuxPost(hdl - 1);
}

int32 os_mutex_del(os_mutex_t *mutex)
{
    ASSERT(mutex && mutex->hdl);
    int32 ret = 0;
    UINT32 hdl = (UINT32)mutex->hdl;
    ret = LOS_MuxDelete(hdl - 1);
    mutex->hdl = NULL;
    mutex->magic = 0;
    return ret;
}

#endif

