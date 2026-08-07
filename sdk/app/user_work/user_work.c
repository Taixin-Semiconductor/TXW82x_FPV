#include "osal/sleep.h"
#include "osal/work.h"
#include "osal/irq.h"
__bobj struct os_workqueue user_wkq;
#define USER_WKQ    user_wkq

int32 os_run_userwork(struct os_work *work)
{
    if (!USER_WKQ.init || work == NULL || work->func == NULL) {
        return -EINVAL;
    }

    os_work_schedule(&USER_WKQ, work);
    return RET_OK;
}

int32 os_run_userwork_delay(struct os_work *work, uint32 delay_ms)
{
    if (!USER_WKQ.init || work == NULL || work->func == NULL) {
        return -EINVAL;
    }

    if (delay_ms == 0) {
        os_work_schedule(&USER_WKQ, work);
    } else {
        os_work_schedule_delay(&USER_WKQ, work, delay_ms);
    }
    return RET_OK;
}

void user_workqueue_init(uint16 pri,void *stack,uint16 stack_size)
{
    os_workqueue_init(&USER_WKQ,"userworkqueue",pri,stack,stack_size);
}