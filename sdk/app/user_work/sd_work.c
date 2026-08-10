#include "osal/sleep.h"
#include "osal/work.h"
#include "osal/irq.h"
#include "basic_include.h"

__bobj struct os_workqueue sd_wkq;
#define SD_WKQ sd_wkq
int32 os_run_sdwork(struct os_work *work)
{
    if (!SD_WKQ.init || work == NULL || work->func == NULL)
    {
        os_printf(KERN_ERR "sdworkqueue run error,%s:%d\tinit:%d\twork:%X\tfunc:%X\n", __FUNCTION__, __LINE__, SD_WKQ.init, work, work->func);
        return -EINVAL;
    }

    os_work_schedule(&SD_WKQ, work);
    return RET_OK;
}

int32 os_run_sdwork_delay(struct os_work *work, uint32 delay_ms)
{
    if (!SD_WKQ.init || work == NULL || work->func == NULL)
    {
        os_printf(KERN_ERR "sdworkqueue run error,%s:%d\tinit:%d\twork:%X\tfunc:%X\n", __FUNCTION__, __LINE__, SD_WKQ.init, work, work->func);
        return -EINVAL;
    }

    if (delay_ms == 0)
    {
        os_work_schedule(&SD_WKQ, work);
    }
    else
    {
        os_work_schedule_delay(&SD_WKQ, work, delay_ms);
    }
    return RET_OK;
}

void sd_workqueue_init(uint16 pri, void *stack, uint16 stack_size)
{
    os_workqueue_init(&SD_WKQ, "sdworkqueue", pri, stack, stack_size);
}
