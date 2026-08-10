#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"

static atomic8_t           g_vdecwqk_init;
static struct os_workqueue g_videodec_wkq; //共用的decode task


//支持多次调用
int32 vdec_wkq_init(uint32 priority, void *stack, uint16 stack_size)
{
    uint8 init = atomic_inc2_return(&g_vdecwqk_init);
    if (init == 0) {
        if(priority == 0) priority = OS_TASK_PRIORITY_ABOVE_NORMAL;
        return os_workqueue_init(&g_videodec_wkq, "vdec_wkq", priority, stack, stack_size);
    }
    return RET_OK;
}

//支持多次调用
int32 vdec_wkq_deinit(void)
{
    uint8 deinit = atomic_dec2_return(&g_vdecwqk_init);
    if (deinit == 1) {
        os_workqueue_deinit(&g_videodec_wkq);
    }
    return RET_OK;
}

int32 vdec_work_run(struct os_work *work)
{
    if (atomic_read(&g_vdecwqk_init)) {
        os_work_schedule(&g_videodec_wkq, work);
		return RET_OK;
    } else {
        return -EINVAL;
    }
}

int32 vdec_work_delay_run(struct os_work *work, uint32 delay_ms)
{
    if (atomic_read(&g_vdecwqk_init)) {
        os_work_schedule_delay(&g_videodec_wkq, work, delay_ms);
		return RET_OK;
    } else {
        return -EINVAL;
    }
}

