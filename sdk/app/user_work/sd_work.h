#ifndef __USER_WORK_H
#define __USER_WORK_H
#include "osal/sleep.h"
#include "osal/work.h"
#include "osal/irq.h"
int32 os_run_sdwork(struct os_work *work);
int32 os_run_sdwork_delay(struct os_work *work, uint32 delay_ms);
void sd_workqueue_init(uint16 pri,void *stack,uint16 stack_size);
#endif