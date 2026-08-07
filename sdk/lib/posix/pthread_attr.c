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

int pthread_attr_init(pthread_attr_t *attr)
{
    if (attr) {
        os_memset(attr, 0, sizeof(pthread_attr_t));
        attr->stacksize = 1024;
    }
    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr)
{
    if (attr) {
        os_memset(attr, 0, sizeof(pthread_attr_t));
    }
    return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int state)
{
    if (state != PTHREAD_CREATE_JOINABLE && state != PTHREAD_CREATE_DETACHED) {
        return -EINVAL;
    }
    attr->detachstate = state;
    return 0;
}

int pthread_attr_getdetachstate(pthread_attr_t const *attr, int *state)
{
    *state = (int)attr->detachstate;
    return 0;
}

int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy)
{
    //attr->schedpolicy = policy;
    return 0;
}

int pthread_attr_getschedpolicy(pthread_attr_t const *attr, int *policy)
{
    //*policy = (int)attr->schedpolicy;
    return 0;
}

int pthread_attr_setschedparam(pthread_attr_t           *attr,  struct sched_param const *param)
{
    attr->schedparam.sched_priority = param->sched_priority;
    return 0;
}

int pthread_attr_getschedparam(pthread_attr_t const *attr, struct sched_param   *param)
{
    param->sched_priority = attr->schedparam.sched_priority;
    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stack_size)
{
    if (attr) {
        attr->stacksize = stack_size;
        if (stack_size >= 4096) {
            pthread_dbg("set stack size %d, maybe too large!\r\n");
        }
    }
    return 0;
}

int pthread_attr_getstacksize(pthread_attr_t const *attr, size_t *stack_size)
{
    *stack_size = attr->stacksize;
    return 0;
}

int pthread_attr_setstackaddr(pthread_attr_t *attr, void *stack_addr)
{
    return -EOPNOTSUPP;
}

int pthread_attr_getstackaddr(pthread_attr_t const *attr, void **stack_addr)
{
    return -EOPNOTSUPP;
}

int pthread_attr_setstack(pthread_attr_t *attr, void *stack_base, size_t          stack_size)
{
    //attr->stackaddr = stack_base;
    attr->stacksize = ALIGN(stack_size, 4);
    return 0;
}

int pthread_attr_getstack(pthread_attr_t const *attr, void                **stack_base, size_t               *stack_size)
{
    *stack_base = 0;//attr->stackaddr;
    *stack_size = attr->stacksize;
    return 0;
}

int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guard_size)
{
    return EOPNOTSUPP;
}

int pthread_attr_getguardsize(pthread_attr_t const *attr, size_t *guard_size)
{
    return EOPNOTSUPP;
}

int pthread_attr_setscope(pthread_attr_t *attr, int scope)
{
    return EOPNOTSUPP;
}

int pthread_attr_getscope(pthread_attr_t const *attr, int *scope)
{
    return EOPNOTSUPP;
}

#endif
