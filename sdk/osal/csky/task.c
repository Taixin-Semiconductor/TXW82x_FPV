#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/string.h"
#include "osal/task.h"

#ifdef CSKY_OS
#include <csi_kernel.h>
#include <k_api.h>

extern uint32_t csi_kernel_task_runtime(struct os_task_info *tsk_rumtime, int count);
extern void csi_kernel_task_dump(k_task_handle_t task_handle, void *stack);
extern void csi_kernel_task_print(void);
extern int csi_kernel_task_count(void);

static void os_task_entry(void *args)
{
    os_task_t *task = (os_task_t *)args;
    task->func((void *)task->args);
}

int32 os_task_init(const uint8 *name, os_task_t *task, os_task_func_t func, uint32 data)
{
    //ASSERT(task);
    os_memset(task, 0, sizeof(os_task_t));
    task->args = data;
    task->func = func;
    task->name = (const char *)name;
    return RET_OK;
}

int32 os_task_priority(os_task_t *task)
{
    //ASSERT(task);
    return task->priority;
}
int32 os_task_priority2(void *hdl)
{
    return csi_kernel_task_get_prio(hdl);
}

int32 os_task_stacksize(os_task_t *task)
{
    //ASSERT(task);
    return task->stack_size;
}
int32 os_task_stacksize2(void *hdl)
{
    return csi_kernel_task_get_stack_size(hdl);
}

int32 _os_task_set_priority(os_task_t *task, uint32 prio)
{
    int32 pri = KPRIO_NORMAL;
    uint8 priority  = prio & 0xff;

    //ASSERT(task);
    //ASSERT(!task->hdl);
    if (priority < OS_TASK_PRIORITY_LOW) {
        pri = KPRIO_LOW0;
    } else if (priority < OS_TASK_PRIORITY_BELOW_NORMAL) {
        pri = KPRIO_LOW0 + (priority - OS_TASK_PRIORITY_LOW);
        if (pri > KPRIO_LOW7) { pri = KPRIO_LOW7; }
    } else if (priority >= OS_TASK_PRIORITY_BELOW_NORMAL && priority < OS_TASK_PRIORITY_NORMAL) {
        pri = KPRIO_NORMAL_BELOW0 + (priority - OS_TASK_PRIORITY_BELOW_NORMAL);
        if (pri > KPRIO_NORMAL_BELOW7) { pri = KPRIO_NORMAL_BELOW7; }
    } else if (priority >= OS_TASK_PRIORITY_NORMAL && priority < OS_TASK_PRIORITY_ABOVE_NORMAL) {
        pri = KPRIO_NORMAL + (priority - OS_TASK_PRIORITY_NORMAL);
        if (pri > KPRIO_NORMAL7) { pri = KPRIO_NORMAL7; }
    } else if (priority >= OS_TASK_PRIORITY_ABOVE_NORMAL && priority < OS_TASK_PRIORITY_HIGH) {
        pri = KPRIO_NORMAL_ABOVE0 + (priority - OS_TASK_PRIORITY_ABOVE_NORMAL);
        if (pri > KPRIO_NORMAL_ABOVE7) { pri = KPRIO_NORMAL_ABOVE7; }
    } else if (priority >= OS_TASK_PRIORITY_HIGH && priority < OS_TASK_PRIORITY_REALTIME) {
        pri = KPRIO_HIGH0 + (priority - OS_TASK_PRIORITY_HIGH);
        if (pri > KPRIO_HIGH7) { pri = KPRIO_HIGH7; }
    } else if (priority >= OS_TASK_PRIORITY_REALTIME && priority < OS_TASK_PRIORITY_ISR) {
        pri = KPRIO_REALTIME0 + (priority - OS_TASK_PRIORITY_REALTIME);
        if (pri > KPRIO_REALTIME7) { pri = KPRIO_REALTIME7; }
    } else {
        pri = KPRIO_ISR;
    }

    if (task) {
        task->priority = pri;
        task->lprun    = (prio & OS_TASK_FLAGS_LPRUN) ? 1 : 0;
        if (task->hdl) {
            csi_kernel_task_set_prio(task->hdl, task->priority);
            csi_kernel_task_set_lprun(task->hdl, task->lprun);
        }
    }
    return pri;
}

int32 os_task_set_priority(os_task_t *task, uint32 pri)
{
    uint8 priority = pri & 0xff;
    if (priority >= OS_TASK_PRIORITY_HIGH) {
        priority = OS_TASK_PRIORITY_HIGH - 1;
        pri &= 0xffffff00;
        pri |= priority;
        os_printf("INVALID PRIORITY\r\n");
    }
    return _os_task_set_priority(task, pri);
}

int32 os_task_set_stack(os_task_t *task, void *stack, int32 stack_size)
{
    //ASSERT(task);
    //ASSERT(!task->hdl);
    task->stack = stack;
    task->stack_size = stack_size;
    return RET_OK;
}

int32 os_task_run(os_task_t *task)
{
    //ASSERT(task);
    int32 ret = csi_kernel_task_new(os_task_entry, task->name,
                                    (void *)task, task->priority, 0, task->stack,
                                    task->stack_size, &task->hdl);
    if(ret == RET_OK){
        csi_kernel_task_set_lprun(task->hdl, task->lprun);
    }
    //ASSERT(!ret);
    return ret;
}

int32 os_task_stop(os_task_t *task)
{
    //ASSERT(0);
    csi_kernel_task_terminate(task->hdl);
    return RET_OK;
}

int32 os_task_del(os_task_t *task)
{
    int32 ret = 0;
    //ASSERT(task);
    ASSERT(task->hdl);
    void *hdl = task->hdl;
    task->hdl = NULL;
    ret = csi_kernel_task_del(hdl);

    return ret;
}

int32 os_task_runtime(struct os_task_info *tsk_times, int32 count)
{
    return csi_kernel_task_runtime(tsk_times, count);
}

void os_task_print(void)
{
    csi_kernel_task_print();
}

int32 os_task_count(void)
{
    return csi_kernel_task_count();
}

void *os_task_current(void)
{
    return csi_kernel_task_get_cur();
}

int32 os_task_suspend(os_task_t *task)
{
    return csi_kernel_task_suspend(task->hdl);
}

int32 os_task_resume(os_task_t *task)
{
    return csi_kernel_task_resume(task->hdl);
}

void os_task_dump(void *hdl, void *stack)
{
    csi_kernel_task_dump(hdl, stack);
}

int32 os_task_yield(void)
{
    return csi_kernel_task_yield();
}

int32 os_sched_disable(void)
{
    krhino_sched_disable();
    return 0;
}

int32 os_sched_enbale(void)
{
    krhino_sched_enable();
    return 0;
}

os_task_t *os_task_hdl2tsk(void *hdl)
{
    ktask_t *task = (ktask_t *)hdl;
    return (task && task->arg) ? ((os_task_t *)task->arg) : NULL;
}

void *os_task_data(void *hdl)
{
    ktask_t *task = (ktask_t *)hdl;
    return task->arg;
}

void *os_task_create(const char *name, os_task_func_t func, void *args, uint32 prio, uint32 time, void *stack, uint32 stack_size)
{
    k_task_handle_t hdl = NULL;
    uint32 priority = os_task_set_priority(NULL, prio);
    int32 ret = csi_kernel_task_new(func, name, args, priority, time, stack, stack_size, &hdl);
    if(ret == RET_OK){
        if(prio & OS_TASK_FLAGS_LPRUN){
            csi_kernel_task_set_lprun(hdl, 1);
        }
    }
    ASSERT(!ret);
    return hdl;
}

int32 os_task_destroy(void *hdl)
{
    return csi_kernel_task_del(hdl);
}

int32 os_task_suspend2(void *hdl)
{
    return csi_kernel_task_suspend((k_task_handle_t)hdl);
}

int32 os_task_resume2(void  *hdl)
{
    return csi_kernel_task_resume((k_task_handle_t)hdl);
}

int32 os_blklist_init(os_blklist_t *blkobj)
{
	void *csi_kernel_blklist_new();
    blkobj->hdl = csi_kernel_blklist_new();
    return  blkobj->hdl ? RET_OK : RET_ERR;
}

void os_blklist_del(os_blklist_t *blkobj)
{
	void csi_kernel_blklist_del(void *hdl);
    csi_kernel_blklist_del(blkobj->hdl);
}

void os_blklist_suspend(os_blklist_t *blkobj, void *task_hdl)
{
	void csi_kernel_blklist_suspend(void *hdl, k_task_handle_t task_hdl);
    csi_kernel_blklist_suspend(blkobj->hdl, task_hdl);
}

void os_blklist_resume(os_blklist_t *blkobj)
{
	void csi_kernel_blklist_wakeup(void *hdl);
    csi_kernel_blklist_wakeup(blkobj->hdl);
}

void os_lpower_mode(uint8 enable)
{
    csi_kernel_lpower_mode(enable);
}

#endif
