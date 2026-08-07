#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/event.h"

#ifdef OHOS
#include "los_event.h"
#include "los_membox.h"
#include "los_memory.h"
#include "los_interrupt.h"
#include "los_mux.h"
#include "los_queue.h"
#include "los_sem.h"
#include "los_swtmr.h"
#include "los_task.h"
#include "los_timer.h"
#include "los_debug.h"
#if (LOSCFG_MUTEX_CREATE_TRACE == 1)
#include "los_arch.h"
#endif
#include "osal/string.h"
#include "osal/task.h"
#include "los_cpup.h"

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
    UINT32 tsk_hdl = (UINT32)hdl;
    if(tsk_hdl > 0){
        return LOS_TaskPriGet(tsk_hdl - 1);
    }else{
        return 0;
    }
}

int32 os_task_stacksize(os_task_t *task)
{
    //ASSERT(task);
    return task->stack_size;
}
int32 os_task_stacksize2(void *hdl)
{
    UINT32 tsk_hdl = (UINT32)hdl;
    if(tsk_hdl > 0){
        LosTaskCB *taskCB = OS_TCB_FROM_TID(tsk_hdl - 1);
        return taskCB->stackSize;
    }else{
        return 0;
    }    
}

int32 _os_task_set_priority(os_task_t *task, uint32 prio)
{
    int32 pri      = 12;
    uint8 priority = prio&0xff;

    //ASSERT(task);
    //ASSERT(!task->hdl);
    if (priority < OS_TASK_PRIORITY_LOW) {
        pri = 31;
    } else if (priority < OS_TASK_PRIORITY_BELOW_NORMAL) {
        pri = 24 + (priority - OS_TASK_PRIORITY_LOW);
        if (pri > 30) { pri = 30; }
    } else if (priority >= OS_TASK_PRIORITY_BELOW_NORMAL && priority < OS_TASK_PRIORITY_NORMAL) {
        pri = 18 + (priority - OS_TASK_PRIORITY_BELOW_NORMAL);
        if (pri > 24) { pri = 24; }
    } else if (priority >= OS_TASK_PRIORITY_NORMAL && priority < OS_TASK_PRIORITY_ABOVE_NORMAL) {
        pri = 12 + (priority - OS_TASK_PRIORITY_NORMAL);
        if (pri > 18) { pri = 18; }
    } else if (priority >= OS_TASK_PRIORITY_ABOVE_NORMAL && priority < OS_TASK_PRIORITY_HIGH) {
        pri = 6 + (priority - OS_TASK_PRIORITY_ABOVE_NORMAL);
        if (pri > 12) { pri = 12; }
    } else if (priority >= OS_TASK_PRIORITY_HIGH && priority < OS_TASK_PRIORITY_REALTIME) {
        pri = 2 + (priority - OS_TASK_PRIORITY_HIGH);
        if (pri > 6) { pri = 6; }
    } else if (priority >= OS_TASK_PRIORITY_REALTIME && priority < OS_TASK_PRIORITY_ISR) {
        pri = 1;
    } else {
        pri = 0;
    }

    if (task) {
        task->priority = pri;
        task->lprun    = (prio & OS_TASK_FLAGS_LPRUN) ? 1 : 0;
        if (task->hdl) {
            LOS_TaskPriSet((UINT32)task->hdl - 1, task->priority);
            LOS_Task_LpowerRun((UINT32)task->hdl - 1, task->lprun);
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
    UINT32 hdl = 0;
    TSK_INIT_PARAM_S stTskInitParam;
    os_memset(&stTskInitParam, 0, sizeof(stTskInitParam));
    stTskInitParam.pfnTaskEntry = (TSK_ENTRY_FUNC)os_task_entry;
    stTskInitParam.uwArg = (UINT32)task;
    stTskInitParam.uwStackSize = task->stack_size;
    stTskInitParam.pcName = (char *)task->name;
    stTskInitParam.uwResved = LOS_TASK_ATTR_JOINABLE;
    stTskInitParam.usTaskPrio = task->priority;
    if (LOS_OK == LOS_TaskCreate(&hdl, &stTskInitParam)) {
        LOS_Task_LpowerRun(hdl, task->lprun);
        task->hdl = (void *)(hdl + 1);
        return RET_OK;
    }
    return RET_ERR;
}

int32 os_task_stop(os_task_t *task)
{
    //ASSERT(0);
    return RET_OK;
}

int32 os_task_del(os_task_t *task)
{
    UINT32 hdl = (UINT32)task->hdl;
    task->hdl = NULL;
    if(hdl > 0){
        return LOS_TaskDelete(hdl - 1);
    }else{
        return -EINVAL;
    }
}

int32 os_task_runtime(struct os_task_info *tsk_times, int32 count)
{
#if (LOSCFG_BASE_CORE_CPUP == 1)
    int32 i = 0;
    int32 j = 0;
    LosTaskCB *taskCB;
    CPUP_INFO_S info[LOSCFG_BASE_CORE_TSK_LIMIT + 1];
    os_memset(info, 0, sizeof(info));
    LOS_AllTaskCpuUsage(info, CPUP_IN_10S);
    for (i = 0; i < count && i < LOSCFG_BASE_CORE_TSK_LIMIT + 1; i++) {
        if (info[i].usStatus && i != g_idleTaskID) {
            taskCB = OS_TCB_FROM_TID(i);
            tsk_times[j].id    = i;
            tsk_times[j].name  = (const char *)taskCB->taskName;
            tsk_times[j].prio  = taskCB->priority;
            tsk_times[j].arg   = i+1;
            tsk_times[j].stack = taskCB->stackSize / 4;
            tsk_times[j].time  = info[i].uwUsage / LOS_CPUP_PRECISION_MULT;
            tsk_times[j].status = (const char *)OsConvertTskStatus(g_taskCBArray[i].taskStatus);
            j++;
        }
    }
    return j;
#else
    return 0;
#endif
}

void os_task_print(void)
{
#if (LOSCFG_BASE_CORE_CPUP == 1)
    int32 i = 0;
    LosTaskCB *taskCB;
    CPUP_INFO_S info[LOSCFG_BASE_CORE_TSK_LIMIT + 1];
    os_memset(info, 0, sizeof(info));
    LOS_AllTaskCpuUsage(info, CPUP_IN_10S);
    for (i = 0; i < LOSCFG_BASE_CORE_TSK_LIMIT + 1; i++) {
        if (info[i].usStatus) {
            taskCB = OS_TCB_FROM_TID(i);
            printf("\0011Task %s %s ticks:%d stack:%d prio:%d, arg:0x%x\r\n", taskCB->taskName, 
                    (const char *)OsConvertTskStatus(info[i].usStatus),
                    info[i].uwUsage / LOS_CPUP_PRECISION_MULT,
                    taskCB->stackSize / 4, taskCB->priority, taskCB->arg);
        }
    }
#endif
}

int32 os_task_count(void)
{
#if (LOSCFG_BASE_CORE_CPUP == 1)
    int32 i = 0;
    int32 j = 0;
    CPUP_INFO_S info[LOSCFG_BASE_CORE_TSK_LIMIT + 1];
    os_memset(info, 0, sizeof(info));
    LOS_AllTaskCpuUsage(info, CPUP_IN_10S);
    for (i = 0; i < LOSCFG_BASE_CORE_TSK_LIMIT + 1; i++) {
        if (info[i].usStatus) {
            j++;
        }
    }
    return j;
#else
    return 0;
#endif
}

void *os_task_current(void)
{
    return (void *)(g_losTask.runTask->taskID + 1);
}

int32 os_task_suspend(os_task_t *task)
{
    UINT32 hdl = (UINT32)task->hdl;
    if(hdl > 0){
        return LOS_TaskSuspend(hdl - 1);
    }else{
        return -EINVAL;
    }
}

int32 os_task_resume(os_task_t *task)
{
    UINT32 hdl = (UINT32)task->hdl;

    if(hdl > 0){
        return LOS_TaskResume(hdl - 1);
    }else{
        return -EINVAL;
    }
}

UINT8 os_task_state_convert(UINT16 taskStatus)
{
    if (taskStatus & OS_TASK_STATUS_RUNNING) {
        return 1;
    } else if (taskStatus & OS_TASK_STATUS_READY) {
        return 1; //READY
    } else if (taskStatus & OS_TASK_STATUS_EXIT) {
        return 7; //delete
    } else if (taskStatus & OS_TASK_STATUS_SUSPEND) {
        return 3; //suspend
    } else if (taskStatus & OS_TASK_STATUS_DELAY) {
        return 5; //sleep
    } else if (taskStatus & OS_TASK_STATUS_PEND) {
        if (taskStatus & OS_TASK_STATUS_PEND_TIME) {
            return 6; //
        }
        return 2; //pend
    }

    return 0;
}

void os_task_dump(void *hdl, void *stack)
{
    UINT32 i = 0;
    UINT32 *p;
    UINT32 *addr = 0;
    LosTaskCB *task;
    UINT32 taskID = (UINT32)hdl;
    UINT32 *sp = (uint32_t *)__get_SP();

    if(hdl == NULL){
        return;
    }

    taskID = taskID - 1;
    task = OS_TCB_FROM_TID(taskID);
    p = (stack ? stack : task->stackPointer);
__lable:
    printf("\0011Task:%s\r\n", task->taskName);
    printf("\0011    task_state: %d\r\n", os_task_state_convert(task->taskStatus));
    printf("\0011    stack_size: %d\r\n", task->stackSize);
    printf("\0011    task_stack: 0x%08x,0x%08x,0x%08x\r\n", (uint32_t)task->topOfStack, (uint32_t)p, (uint32_t)(task->topOfStack + task->stackSize));
    if(task == g_losTask.runTask){
        printf("\0011    task_lr   : %p\r\n", __builtin_return_address(0));
        printf("\0011    task_pc   : %p\r\n", &&__lable);
        p = sp;
    }

    printf("\0011    stack dump:\r\n    ");
    for(addr=p; (UINT32)addr<task->topOfStack+task->stackSize; addr++){
        printf("\00110x%08x,", *addr);
        if(++i == 4){ 
            printf("\0011\r\n    ");
            i = 0;
        }
    }
    printf("\0011\r\n");
}

int32 os_task_yield(void)
{
    return LOS_TaskYield();
}

int32 os_sched_disable(void)
{
    LOS_TaskLock();
    return 0;
}

int32 os_sched_enbale(void)
{
    LOS_TaskUnlock();
    return 0;
}

void *os_task_data(void *hdl)
{
    LosTaskCB *task;
    UINT32 taskID = (UINT32)hdl;
    if(taskID > 0){
        task = OS_TCB_FROM_TID(taskID-1);
        return (void *)task->arg;
    }else{
        return NULL;
    }
}

void *os_task_create(const char *name, os_task_func_t func, void *args, uint32 prio, uint32 time, void *stack, uint32 stack_size)
{
    UINT32 hdl = 0;
    TSK_INIT_PARAM_S stTskInitParam;
    os_memset(&stTskInitParam, 0, sizeof(stTskInitParam));
    stTskInitParam.pfnTaskEntry = (TSK_ENTRY_FUNC)func;
    stTskInitParam.uwArg = (UINT32)args;
    stTskInitParam.uwStackSize = stack_size;
    stTskInitParam.pcName = (char *)name;
    stTskInitParam.uwResved = LOS_TASK_ATTR_JOINABLE;
    stTskInitParam.usTaskPrio = os_task_set_priority(NULL, prio);
    if (LOS_OK == LOS_TaskCreate(&hdl, &stTskInitParam)) {
        if(prio & OS_TASK_FLAGS_LPRUN){
            LOS_Task_LpowerRun(hdl, 1);
        }
        hdl++;
    }
    return (void *)hdl;
}

int32 os_task_destroy(void *hdl)
{
    UINT32 tsk_hdl = (UINT32)hdl;
    if(tsk_hdl > 0){
        return LOS_TaskDelete(tsk_hdl - 1);
    }else{
        return -EINVAL;
    }
}

void os_lpower_mode(uint8 enable)
{
    UINT32       intSave;
    UINT32       loopNum;
    LosTaskCB    *taskCB = (LosTaskCB *)NULL;
    intSave = LOS_IntLock();
    for (loopNum = 0; loopNum < g_taskMaxNum; loopNum++) {
        taskCB = (((LosTaskCB *)g_taskCBArray) + loopNum);
        if (taskCB->taskStatus & OS_TASK_STATUS_UNUSED) {
            continue;
        }
        if(loopNum != g_idleTaskID && !taskCB->lpRun){
            if(enable){
                LOS_TaskSuspend(loopNum);
            }else{
                LOS_TaskResume(loopNum);
            }
        }
    }
    LOS_IntRestore(intSave);
}


int32 os_task_suspend2(void *task_hdl)
{
	UINT32 hdl = (UINT32)task_hdl;
    if(hdl > 0){
        return LOS_TaskSuspend(hdl - 1);
    }else{
        return -EINVAL;
    }
}

int32 os_task_resume2(void *task_hdl)
{
	UINT32 hdl = (UINT32)task_hdl;
    if(hdl > 0){
        return LOS_TaskResume(hdl - 1);
    }else{
        return -EINVAL;
    }
}

int32 os_blklist_init(os_blklist_t *blkobj)
{
    blkobj->hdl = LOS_Blklist_New();
    return  blkobj->hdl ? RET_OK : RET_ERR;
}

void os_blklist_del(os_blklist_t *blkobj)
{
    if(blkobj->hdl){
        LOS_Blklist_Delete(blkobj->hdl);
    }
}

void os_blklist_suspend(os_blklist_t *blkobj, void *hdl)
{
    UINT32 task_hdl = (UINT32)hdl;
    if(blkobj->hdl && task_hdl > 0){
        LOS_Blklist_Suspend(blkobj->hdl, task_hdl-1);
    }
}

void os_blklist_resume(os_blklist_t *blkobj)
{
    if(blkobj->hdl){
        LOS_Blklist_Resume(blkobj->hdl);
    }
}

#endif

