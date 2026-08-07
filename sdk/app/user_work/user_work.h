#ifndef __USER_WORK_H
#define __USER_WORK_H
#include "osal/sleep.h"
#include "osal/work.h"
#include "osal/irq.h"
//如果添加这个头文件,就意味着使用user_work去代替默认sdk的workqueue
#define os_run_work(work) os_run_userwork(work)
#define os_run_work_delay(work,time) os_run_userwork_delay(work,time)
int32 os_run_userwork(struct os_work *work);
int32 os_run_userwork_delay(struct os_work *work, uint32 delay_ms);
void user_workqueue_init(uint16 pri,void *stack,uint16 stack_size);
#endif