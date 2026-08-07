#include "sys_config.h"
#include "typesdef.h"
#include "errno.h"
#include "osal/semaphore.h"
#include "osal/task.h"
#include "osal/string.h"
#include "osal/timer.h"
#include "osal/mutex.h"
#include "osal/condv.h"
#include "lib/common/rbuffer.h"
#include "lib/posix/pthread.h"

#ifdef TXWSDK_POSIX

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    os_condv_t *condv;

#ifdef __MINILIBC__
    if (NULL == cond) {
        return -EINVAL;
    }

    os_memset(cond, 0, sizeof(pthread_cond_t));

    condv = os_zalloc(sizeof(os_condv_t));
    if (NULL == condv) {
        pthread_err("alloc fail\r\n");
        return -ENOMEM;
    } 

    if (os_condv_init(condv)) {
        os_free(condv);
        pthread_err("os_condv_init fail\r\n");
        return RET_ERR;
    }

    cond->lock = condv;
#else
    if (*cond == 0) {
        condv = os_zalloc(sizeof(os_condv_t));
        if (condv) {
            if (os_condv_init(condv)) {
                os_free(condv);
                pthread_err("os_condv_init fail\r\n");
                return RET_ERR;
            }

            *cond = (pthread_cond_t)condv;
            return RET_OK;
        } else {
            pthread_err("alloc fail\r\n");
        }
        return -ENOMEM;
    }
#endif
    return RET_OK;
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
#ifdef __MINILIBC__
    if (NULL == cond) {
        return -EINVAL;
    }

    if (NULL == cond->lock) {
        pthread_cond_init(cond, NULL);
    }

    return os_condv_broadcast((os_condv_t *)(cond->lock));
#else
    pthread_cond_init(cond, NULL);
    return os_condv_broadcast((os_condv_t *)(*cond));
#endif
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
#ifdef __MINILIBC__
    if (NULL == cond) {
        return -EINVAL;
    }

    if (cond->lock) {
        os_condv_del((os_condv_t *)(cond->lock));
        os_free(cond->lock);
    }
#else
    os_condv_del((os_condv_t *)(*cond));
#endif

    return 0;
}

int pthread_cond_signal(pthread_cond_t *cond)
{
#ifdef __MINILIBC__
    if (NULL == cond) {
        return -EINVAL;
    }

    if (NULL == cond->lock) {
        pthread_cond_init(cond, NULL);
    }

    return os_condv_signal((os_condv_t *)(cond->lock));
#else
    pthread_cond_init(cond, NULL);
    return os_condv_signal((os_condv_t *)(*cond));
#endif
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
#ifdef __MINILIBC__
    if (NULL == cond) {
        return -EINVAL;
    }

    if (NULL == cond->lock) {
        pthread_cond_init(cond, NULL);
    }

    return os_condv_wait((os_condv_t *)(cond->lock), (os_mutex_t *)(mutex->mutex), pthread_timespec_delta(abstime));
#else
    pthread_cond_init(cond, NULL);
    return os_condv_wait((os_condv_t *)(*cond), (os_mutex_t *)(*mutex), pthread_timespec_delta(abstime));
#endif
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
#ifdef __MINILIBC__
    if (NULL == cond) {
        return -EINVAL;
    }

    if (NULL == cond->lock) {
        pthread_cond_init(cond, NULL);
    }

    return os_condv_wait((os_condv_t *)(cond->lock), (os_mutex_t *)(mutex->mutex), osWaitForever);
#else
    pthread_cond_init(cond, NULL);
    return os_condv_wait((os_condv_t *)(*cond), (os_mutex_t *)(*mutex), osWaitForever);
#endif
}

int pthread_condattr_destroy(pthread_condattr_t *attr)
{
    if (!attr) {
        return -EINVAL;
    }

    return 0;
}

int pthread_condattr_init(pthread_condattr_t *attr)
{
    if (!attr) {
        return -EINVAL;
    }
#ifdef LIB_PTHREAD_DEF
#if defined(_POSIX_THREAD_PROCESS_SHARED) || defined(__MINILIBC__)
    attr->pshared = PTHREAD_PROCESS_PRIVATE;
#endif
#else
    *attr = PTHREAD_PROCESS_PRIVATE;
#endif
    return 0;
}

int pthread_condattr_getclock(const pthread_condattr_t *attr, clockid_t         *clock_id)
{
    return 0;
}

int pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t         clock_id)
{
    return 0;
}

int pthread_condattr_getpshared(const pthread_condattr_t *attr, int *pshared)
{
    if (!attr || !pshared) {
        return -EINVAL;
    }

    *pshared = PTHREAD_PROCESS_PRIVATE;
    return 0;
}

int pthread_condattr_setpshared(pthread_condattr_t *attr, int pshared)
{
    if ((pshared != PTHREAD_PROCESS_PRIVATE) && (pshared != PTHREAD_PROCESS_SHARED)) {
        return -EINVAL;
    }
    if (pshared != PTHREAD_PROCESS_PRIVATE) {
        return -ENOSYS;
    }
    return 0;
}

#endif

