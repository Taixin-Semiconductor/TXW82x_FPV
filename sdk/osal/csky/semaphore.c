#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/string.h"
#include "osal/semaphore.h"

#ifdef CSKY_OS
#include "csi_kernel.h"
#include <k_api.h>

#define SEMA_MAGIC (0x3a8d7c1d)

int32 os_sema_init(os_semaphore_t *sem, int32 val)
{
    if (sem->magic == SEMA_MAGIC) {
        os_printf(KERN_WARNING"sem repeat initialization ????\r\n");
    }

    sem->hdl = csi_kernel_sem_new(65535, val);
    if (sem->hdl) { sem->magic = SEMA_MAGIC; }
    return (sem->hdl ? RET_OK : RET_ERR);
}

int32 os_sema_down(os_semaphore_t *sem, int32 tmo_ms)
{
    int32  ret = 0;
    ASSERT(sem && sem->hdl);
    ret = csi_kernel_sem_wait(sem->hdl, csi_kernel_ms2tick(tmo_ms));
    return (ret == RHINO_SUCCESS) ? 1 : 0;
}

int32 os_sema_up(os_semaphore_t *sem)
{
    ASSERT(sem && sem->hdl);
    return csi_kernel_sem_post(sem->hdl);
}

int32 os_sema_del(os_semaphore_t *sem)
{
    ASSERT(sem && sem->hdl);
    int32 ret = csi_kernel_sem_del(sem->hdl);
    sem->hdl = NULL;
    sem->magic = 0;
    return ret;
}

int32 os_sema_count(os_semaphore_t *sem)
{
    return (sem && sem->hdl) ? csi_kernel_sem_get_count(sem->hdl) : 0;
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

