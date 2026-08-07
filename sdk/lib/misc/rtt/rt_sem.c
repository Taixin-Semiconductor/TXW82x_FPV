#include "rtthread.h"

rt_err_t rt_sem_init(rt_sem_t sem, const char *name, rt_uint32_t value, rt_uint8_t flag)
{
    return os_sema_init(sem, value);
}

rt_err_t rt_sem_detach(rt_sem_t sem)
{
    return os_sema_del(sem);
}

rt_sem_t rt_sem_create(const char *name, rt_uint32_t value, rt_uint8_t flag)
{
    struct rt_semaphore *sem = os_malloc(sizeof(struct rt_semaphore));
    if (sem) {
        os_sema_init(sem, value);
    }
    return sem;
}

rt_err_t rt_sem_delete(rt_sem_t sem)
{
    os_sema_del(sem);
    os_free(sem);
    return RT_EOK;
}

rt_err_t rt_sem_take(rt_sem_t sem, rt_int32_t time)
{
    return (os_sema_down(sem, time) == 1) ? RT_EOK : RT_ERROR;
}

rt_err_t rt_sem_trytake(rt_sem_t sem)
{
    return (os_sema_down(sem, 0) == 1) ? RT_EOK : RT_ERROR;
}

rt_err_t rt_sem_release(rt_sem_t sem)
{
    return os_sema_up(sem);
}

