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

#ifdef TXWSDK_POSIX

int sem_destroy(sem_t *sem)
{
    os_semaphore_t *s = (os_semaphore_t *)(*sem);
    os_sema_del(s);
    os_free(s);
    return 0;
}

int sem_getvalue(sem_t *sem, int *sval)
{
    *sval = os_sema_count((os_semaphore_t *)(*sem));
    return 0;
}

int sem_init(sem_t *sem, int pshared, unsigned value)
{
    os_semaphore_t *s;

    if (sem == NULL) {
        return -EINVAL;
    }

    s = (os_semaphore_t *)os_zalloc(sizeof(os_semaphore_t));
    if (s == NULL) {
        pthread_err("alloc fail\r\n");
        return -ENOMEM;
    }

    os_sema_init(s, 0);
    *sem = (sem_t)s;
    return RET_OK;
}

int sem_post(sem_t *sem)
{
    return os_sema_up((os_semaphore_t *)(*sem));
}

int sem_timedwait(sem_t *sem, const struct timespec *abstime)
{
    return os_sema_down((os_semaphore_t *)(*sem), pthread_timespec_delta(abstime));
}

int sem_trywait(sem_t *sem)
{
    return os_sema_down((os_semaphore_t *)(*sem), 0);
}

int sem_wait(sem_t *sem)
{
    return os_sema_down((os_semaphore_t *)(*sem), osWaitForever);
}

#endif

