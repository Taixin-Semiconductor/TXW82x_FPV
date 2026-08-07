/*
 * @Author: jiejie
 * @Github: https://github.com/jiejieTop
 * @Date: 2019-12-26 19:11:40
 * @LastEditTime : 2019-12-28 01:15:29
 * @Description: the code belongs to jiejie, please keep the author information and source code according to the license.
 */
#include "salof_defconfig.h"

#ifdef SALOF_USING_LOG

void *salof_alloc(unsigned int size)
{
    return os_malloc(size);
}

void salof_free(void *mem)
{
    os_free(mem);
}

salof_tcb salof_task_create(const char *name,
                            void (*task_entry)(void *param),
                            void * const param,
                            unsigned int stack_size,
                            unsigned int priority,
                            unsigned int tick)
{
    salof_tcb task = NULL;
    void *stack = NULL;          //124 = sizeof(ktask_t)

    task = salof_alloc(sizeof(salof_tcb_t));
    if (task == NULL) {
        os_printf("[%s]:task malloc failed!!!\n",__FUNCTION__);
        return NULL;
    }
    stack = salof_alloc(stack_size+124);
    if (stack == NULL) {
        os_printf("[%s]:stack malloc failed!!!\n",__FUNCTION__);
        salof_free(task);
        return NULL;
    }
    os_task_init((const uint8 *)name, task, task_entry, (uint32)param);
    os_task_set_priority(task, priority);
    os_task_set_stack(task, stack, stack_size+124); //124 = sizeof(ktask_t)
    os_task_run(task);
    return task;
}

salof_mutex salof_mutex_create(void)
{
    salof_mutex mutex = salof_alloc(sizeof(salof_mutex_t));
    if (mutex == NULL) {
        return NULL;
    }
    if (os_mutex_init(mutex) != RET_OK) {
        salof_free(mutex);
        return NULL;
    }
    return mutex;
}


void salof_mutex_delete(salof_mutex mutex)
{
    os_mutex_del(mutex);
    salof_free(mutex);
}


int salof_mutex_pend(salof_mutex mutex, unsigned int timeout)
{
    return os_mutex_lock(mutex, timeout) == RET_OK ? 0 : -1;
}

int salof_mutex_post(salof_mutex mutex)
{
    return os_mutex_unlock(mutex) == RET_OK ? 0 : -1;
}


salof_sem salof_sem_create(void)
{
    salof_sem sem = salof_alloc(sizeof(salof_sem_t));
    if (sem == NULL) {
        return NULL;
    }
    if (os_sema_init(sem, 0) != RET_OK) {
        salof_free(sem);
        return NULL;
    }
    return sem;
}

void salof_sem_delete(salof_sem sem)
{
    os_sema_del(sem);
    salof_free(sem);
}

int salof_sem_pend(salof_sem sem, unsigned int timeout)
{
    return os_sema_down(sem, timeout) == RET_OK ? 0 : -1;
}

int salof_sem_post(salof_sem sem)
{
    return os_sema_up(sem) == RET_OK ? 0 : -1;
}

unsigned int salof_get_tick(void)
{
    return os_jiffies_to_msecs(os_jiffies());
}

char *salof_get_task_name(void)
{
    struct os_task *task = NULL;
	task = (struct os_task *)os_task_hdl2tsk(os_task_current());
    return task ? (char *)task->name : NULL;
}

int send_buff(char *buf, int len)
{
    hgprintf_out(buf, len, 0);
    return len;
}

#endif
