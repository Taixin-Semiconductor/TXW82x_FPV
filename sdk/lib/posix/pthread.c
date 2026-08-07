#include "sys_config.h"
#include "typesdef.h"
#include "errno.h"
#include "osal/semaphore.h"
#include "osal/task.h"
#include "osal/string.h"
#include "osal/timer.h"
#include "osal/irq.h"
#include "osal/mutex.h"
#include "lib/common/rbuffer.h"
#include "lib/posix/pthread.h"
#include "osal/sleep.h"

#ifdef TXWSDK_POSIX

#define PTHREAD_MAX (32)

struct pthread_task {
    char name[11];
    uint8 inited;
    void *task_hdl;
    os_semaphore_t join_sema;
    pthread_attr_t attr;
    void *arg;
    void *(*start)(void *);
    void  *tls;
} ;

static pthread_t pthreads[PTHREAD_MAX];

static int32 pthread_set(pthread_t thread)
{
    int32 i = 0;
    uint32 flag = disable_irq();
    for (i = 0; thread && i < PTHREAD_MAX; i++) {
        if (0 == pthreads[i]) {
            pthreads[i] = thread;
            break;
        }
    }
    enable_irq(flag);
    return (thread && i < PTHREAD_MAX);
}

static struct pthread_task *pthread_get(void *thread)
{
    int32 i = 0;
    struct pthread_task *thd = NULL;
    uint32 flag = disable_irq();
    for (i = 0; thread && i < PTHREAD_MAX; i++) {
        if (thread == pthreads[i]) {
            thd = (struct pthread_task *)thread;
            break;
        }
    }
    enable_irq(flag);
    return thd;
}

static void pthread_del(pthread_t thread)
{
    int32 i = 0;
    uint32 flag = disable_irq();
    for (i = 0; i < PTHREAD_MAX; i++) {
        if (thread == pthreads[i]) {
            pthreads[i] = 0;
            break;
        }
    }
    enable_irq(flag);
}

static void pthread_free(struct pthread_task *thd)
{
    pthread_del((pthread_t)thd);
    os_sema_del(&thd->join_sema);
    pthread_key_destroy(thd->tls);
    os_free(thd->tls);
    os_free(thd);
}

static void pthread_task_entry(void *args)
{
    struct pthread_task *thd = (struct pthread_task *)args;
    thd->start(thd->arg);
    while(!thd->inited) os_sleep_ms(5);
    if (thd->attr.detachstate == PTHREAD_CREATE_JOINABLE) {
        pthread_dbg("pthread %s exit: JOINABLE!\r\n", thd->name);
        os_sema_up(&thd->join_sema);
    } else {
        pthread_dbg("pthread %s  exit: DETACHED!\r\n", thd->name);
        pthread_free(thd);
    }
}

static int pthread_priority(pthread_attr_t *attr)
{
    if (attr) {
        if (attr->schedparam.sched_priority == 0) {
            return OS_TASK_PRIORITY_NORMAL;
        } else if (attr->schedparam.sched_priority <= 20) {
            return OS_TASK_PRIORITY_LOW + attr->schedparam.sched_priority / 2;
        } else if (attr->schedparam.sched_priority <= 40) {
            return OS_TASK_PRIORITY_BELOW_NORMAL + (attr->schedparam.sched_priority - 20) / 2;
        } else if (attr->schedparam.sched_priority <= 60) {
            return OS_TASK_PRIORITY_NORMAL + (attr->schedparam.sched_priority - 40) / 2;
        } else {
            return OS_TASK_PRIORITY_ABOVE_NORMAL + (attr->schedparam.sched_priority - 60) / 4;
        }
    }
    return OS_TASK_PRIORITY_NORMAL;
}

int pthread_create(pthread_t *thread,      const pthread_attr_t *attr, void *(*start)(void *), void *arg)
{
    struct pthread_task *thd = os_zalloc(sizeof(struct pthread_task));
    if (thd == NULL) {
        pthread_err("alloc fail\r\n");
        return -ENOMEM;
    }

    if (!pthread_set((pthread_t)thd)) {
        pthread_err("no free pthread, max %d\r\n", PTHREAD_MAX);
        os_free(thd);
        return -ESRCH;
    }

    if (attr) {
        thd->attr = *attr;
        if (attr->schedparam.sched_priority > 99) {
            thd->attr.schedparam.sched_priority = 99;
        }
    } else {
        pthread_attr_init(&thd->attr);
    }

    os_sema_init(&thd->join_sema, 0);
    thd->arg = arg;
    thd->start = start;
    os_sprintf(thd->name, "p_%x", (uint32)start);
    thd->task_hdl = os_task_create(thd->name, pthread_task_entry, thd, pthread_priority(&thd->attr), 0, NULL, thd->attr.stacksize);
    *thread = (pthread_t)thd;
    thd->inited = 1;
    return RET_OK;
}

pthread_t pthread_self(void)
{
    pthread_t thd = (pthread_t)os_task_data(os_task_current());
    return (pthread_t)pthread_get(thd);
}

int pthread_join(pthread_t thread, void **value_ptr)
{
    struct pthread_task *thd = pthread_get(thread);
    if (thd) {
        if (thread == pthread_self()) {
            pthread_err("pthread_join self!\r\n");
            return -EDEADLK;
        }
        if (thd->attr.detachstate == PTHREAD_CREATE_DETACHED) {
            pthread_err("pthread_join DETACHED thread!\r\n");
            return -EINVAL; /* join on a detached pthread */
        }

        os_sema_down(&thd->join_sema, osWaitForever);
        pthread_free(thd);
        pthread_dbg("pthread_join DONE!\r\n");
    } else {
        pthread_dbg("pthread is not exist!\r\n");
    }
    return 0;
}

void pthread_exit(void *value_ptr)
{
    struct pthread_task *thd = pthread_get(pthread_self());
    if (thd) {
        os_task_destroy(thd->task_hdl);
        pthread_free(thd);
    } else {
        pthread_err("pthread is not exist!\r\n");
    }
}

int pthread_detach(pthread_t thread)
{
    struct pthread_task *thd = pthread_get(thread);
    if (thd) {
        thd->attr.detachstate = PTHREAD_CREATE_DETACHED;
    } else {
        pthread_dbg("pthread is not exist!\r\n");
    }
    return 0;
}

unsigned int pthread_timespec_delta(const struct timespec *abstime)
{
    uint64 ticks;
    uint32 tmo = osWaitForever;
    struct timespec cur;

    if (abstime) {
        os_systime(&cur);
        if (timespec_detal_ticks(abstime, &cur, &ticks) == RET_OK) {
            tmo = os_jiffies_to_msecs(ticks);
        }
    }

    return tmo;
}

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
    uint8  init = 0;
    uint32 flag = disable_irq();

    if (once_control == NULL || init_routine == NULL) {
        return -EINVAL;
    }

#ifdef __NEWLIB__
    if (!once_control->is_initialized) {
        once_control->is_initialized = 1;
        init = 1;
    }
#else
    if (!(*once_control)) {
        *once_control = 1;
        init = 1;
    }
#endif

    enable_irq(flag);
    if (init) {
        init_routine();
    }
    return 0;
}

void *pthread_tls(pthread_t thread, int create, int size)
{
    void *ptr;
    uint32 flag;
    struct pthread_task *thd = pthread_get(thread);
    if (thd) {
        if (!create) {
            flag = disable_irq();
            ptr = thd->tls;
            thd->tls = NULL;
            enable_irq(flag);
            os_free(ptr);
        } else {
            flag = disable_irq();
            ptr = thd->tls;
            if (size && ptr == NULL) {
                ptr = os_malloc(size);
                if (ptr) {
                    thd->tls = ptr;
                }
            }
            enable_irq(flag);
            return ptr;
        }
    }
    return NULL;
}

#endif
