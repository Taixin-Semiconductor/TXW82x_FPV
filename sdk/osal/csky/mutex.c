#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/string.h"
#include "osal/mutex.h"

#ifdef CSKY_OS
#include "csi_kernel.h"

#define MUTEX_MAGIC (0xa8b4c2d5)

int32 os_mutex_init(os_mutex_t *mutex)
{
    if(mutex->magic == MUTEX_MAGIC){
        os_printf(KERN_WARNING"mutex repeat initialization ????\r\n");
    }

    mutex->hdl = csi_kernel_mutex_new();
    if(mutex->hdl) mutex->magic = MUTEX_MAGIC;
    return (mutex->hdl ? RET_OK : RET_ERR);
}

int32 os_mutex_lock(os_mutex_t *mutex, int32 tmo)
{
    ASSERT(mutex && mutex->hdl);
    return csi_kernel_mutex_lock(mutex->hdl, csi_kernel_ms2tick(tmo), (uint32_t)RETURN_ADDR());
}

int32 os_mutex_unlock(os_mutex_t *mutex)
{
    ASSERT(mutex && mutex->hdl);
    return csi_kernel_mutex_unlock(mutex->hdl);
}

int32 os_mutex_del(os_mutex_t *mutex)
{
    int32 ret = 0;
    ASSERT(mutex && mutex->hdl);
    ret = csi_kernel_mutex_del(mutex->hdl);
    mutex->hdl = NULL;
    mutex->magic = 0;
    return ret;
}

void *os_mutex_owner(os_mutex_t *mutex)
{
    ASSERT(mutex && mutex->hdl);
    return csi_kernel_mutex_get_owner(mutex->hdl);
}

#endif

