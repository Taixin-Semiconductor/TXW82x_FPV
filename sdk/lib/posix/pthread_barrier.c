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

int pthread_barrierattr_destroy(pthread_barrierattr_t *attr)
{
    if (!attr) {
        return -EINVAL;
    }
    return 0;
}

int pthread_barrierattr_init(pthread_barrierattr_t *attr)
{
    if (!attr) {
        return -EINVAL;
    }
    *attr = PTHREAD_PROCESS_PRIVATE;
    return 0;
}

int pthread_barrierattr_getpshared(const pthread_barrierattr_t *attr, int *pshared)
{
    if (!attr) {
        return -EINVAL;
    }
    *pshared = (int) * attr;
    return 0;
}

int pthread_barrierattr_setpshared(pthread_barrierattr_t *attr, int pshared)
{
    if (!attr) {
        return -EINVAL;
    }
    if (pshared == PTHREAD_PROCESS_PRIVATE) {
        attr = PTHREAD_PROCESS_PRIVATE;
    }
    return -EINVAL;
}

int pthread_barrier_destroy(pthread_barrier_t *barrier)
{
    int32 result;

    if (!barrier) {
        return -EINVAL;
    }
    result = pthread_cond_destroy(&(barrier->cond));
    return result;
}

int pthread_barrier_init(pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, int count)
{
    if (!barrier) {
        return -EINVAL;
    }

    if (attr && (*attr != PTHREAD_PROCESS_PRIVATE)) {
        return -EINVAL;
    }

    barrier->count = count;
    pthread_cond_init(&(barrier->cond), NULL);
    pthread_mutex_init(&(barrier->mutex), NULL);
    return 0;
}

int pthread_barrier_wait(pthread_barrier_t *barrier)
{
    int32 result;

    if (!barrier) {
        return -EINVAL;
    }

    result = pthread_mutex_lock(&(barrier->mutex));
    if (result != 0) {
        return -EINVAL;
    }

    if (barrier->count == 0) {
        result = EINVAL;
    } else {
        barrier->count -= 1;
        if (barrier->count == 0) { /* broadcast condition */
            pthread_cond_broadcast(&(barrier->cond));
        } else {
            pthread_cond_wait(&(barrier->cond), &(barrier->mutex));
        }
    }
    pthread_mutex_unlock(&(barrier->mutex));
    return result;
}

#endif

