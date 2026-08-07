
#ifndef _OS_TASK_WORK_H_
#define _OS_TASK_WORK_H_

#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/string.h"
#include "osal/time.h"
#include "osal/timer.h"
#include "osal/mutex.h"
#include "osal/task.h"
#include "osal/semaphore.h"

#ifdef __cplusplus
extern "C" {
#endif

struct os_work;
typedef int32 (*os_work_func_t)(struct os_work *work);
typedef void (*os_run_func_t)(uint32 param1, uint32 parma2, uint32 param3);

struct os_work {
    struct list_head list;
    struct os_workqueue *wkq;
    uint16 running: 1, alloc : 1, delay: 1, pri: 5, init: 1, rev: 7;
    uint16 schedule;
    uint64 expired;
    uint32 runtime;
    os_work_func_t func;
};

struct os_workqueue {
    char  *name;
    uint16 priority: 10, init: 1, rev: 5;
    uint16 stack_size;
    os_timer_t  timer;
    os_task_t   task;
    struct list_head works;
    struct list_head delay_works;
    os_semaphore_t sema;
    void  *run_func;
    uint64 run_jiff;
};

int32 mainwkq_monitor_init(void);
int32 os_workqueue_init(struct os_workqueue *wkq, char *name, uint16 priority, void *stack, uint16 stack_size);
void os_work_schedule(struct os_workqueue *wkq, struct os_work *work);
void os_work_schedule_delay(struct os_workqueue *wkq, struct os_work *work, uint32 delay_ms);
void os_work_cancle(struct os_work *work, uint8 sync);
void os_work_cancle2(struct os_work *work, uint8 sync); //执行此API后，必须设置init=1才能再次schedule work
int32 os_run_func(os_run_func_t func, uint32 param1, uint32 param2, uint32 param3);
int32 os_run_func_delay(os_run_func_t func, uint32 param1, uint32 param2, uint32 delay_ms);
int32 os_run_work(struct os_work *work);
int32 os_run_work_delay(struct os_work *work, uint32 delay_ms);
int32 OS_WORK_INIT(struct os_work *work, os_work_func_t func, int32 priority);
int32 OS_WORK_REINIT(struct os_work *work);

#ifdef __cplusplus
}
#endif

#endif
