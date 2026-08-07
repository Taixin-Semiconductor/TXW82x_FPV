#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/condv.h"

#ifdef CSKY_OS
#include "csi_kernel.h"
#include <k_api.h>

#define CONDV_MAGIC (0x3a8d7c1d)

int32 os_condv_init(os_condv_t *cond)
{
    if(cond->magic == CONDV_MAGIC){
        os_printf(KERN_WARNING"condv repeat initialization ????\r\n");
    }

    cond->sema = csi_kernel_sem_new(65535, 0);
    if (cond->sema) {
        cond->magic = CONDV_MAGIC;
        atomic_set(&cond->waitings, 0);
    }
    return (cond->sema ? RET_OK : RET_ERR);
}

int32 os_condv_broadcast(os_condv_t *cond)
{
    uint32 i;
    uint32 wait = atomic_read(&cond->waitings);

    ASSERT(cond && cond->magic == CONDV_MAGIC);
    while (wait > 0) {
        if (wait == atomic_cmpxchg(&cond->waitings, wait, 0)) {
            for (i = 0; i < wait; i++) {
                csi_kernel_sem_post(cond->sema);
            }
        }
        wait = atomic_read(&cond->waitings);
    }

    return RET_OK;
}

int32 os_condv_signal(os_condv_t *cond)
{
    uint32 wait = atomic_read(&cond->waitings);
    ASSERT(cond && cond->magic == CONDV_MAGIC);
    while (wait > 0) {
        if (wait == atomic_cmpxchg(&cond->waitings, wait, wait - 1)) {
            csi_kernel_sem_post(cond->sema);
            break;
        }
        wait = atomic_read(&cond->waitings);
    }
	return RET_OK;
}

int32 os_condv_del(os_condv_t *cond)
{
    ASSERT(cond && cond->magic == CONDV_MAGIC);
    csi_kernel_sem_del(cond->sema);
    cond->sema = NULL;
    cond->magic = 0;
    return RET_OK;
}

int32 os_condv_wait(os_condv_t *cond, os_mutex_t *mutex, uint32 tmo_ms)
{
    int32 ret;
    ASSERT(cond && cond->magic == CONDV_MAGIC);
    atomic_inc(&cond->waitings);
    os_mutex_unlock(mutex);
    ret = csi_kernel_sem_wait(cond->sema, tmo_ms);
    //os_printf(KERN_NOTICE"condv 0x%x waitings:%d, tmo_ms:%d, ret:%d\r\n", cond, atomic_read(&cond->waitings), tmo_ms, ret);
    os_mutex_lock(mutex, osWaitForever);
    atomic_dec(&cond->waitings);
    return ret ? -ETIMEDOUT : RET_OK;
}

#endif

