#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "audio_coder.h"

static atomic8_t           g_audio_wkq_init;
static struct os_workqueue g_audiodec_wkq; //共用的decode task

#define AUDIO_DECBUFF_SIZE  (5*1024)       //共用的decode buffer size: 5K
int16 *g_audio_decbuff;                   //共用的decode buffer

//支持多次调用
int32 audio_coder_msi_init(uint32 priority, void *stack, uint16 stack_size)
{
    uint8 init = atomic_inc2_return(&g_audio_wkq_init);
    if (init == 0) {
        if(priority == 0) priority = OS_TASK_PRIORITY_ABOVE_NORMAL;
        g_audio_decbuff = aucoder_msi_malloc(AUDIO_DECBUFF_SIZE);
        audio_coder_module_init();
        return os_workqueue_init(&g_audiodec_wkq, "Audio_Coder", priority, stack, stack_size);
    }
    return RET_OK;
}

//支持多次调用
int32 audio_coder_msi_deinit(void)
{
    uint8 deinit = atomic_dec2_return(&g_audio_wkq_init);
    if (deinit == 1) {
        os_workqueue_deinit(&g_audiodec_wkq);
        aucoder_msi_free(g_audio_decbuff);
        g_audio_decbuff = NULL;
    }
    return RET_OK;
}

int32 audio_coder_msi_run(struct os_work *work)
{
    if (atomic_read(&g_audio_wkq_init)) {
        os_work_schedule(&g_audiodec_wkq, work);
		return RET_OK;
    } else {
        return -EINVAL;
    }
}

int32 audio_coder_msi_delay_run(struct os_work *work, uint32 delay_ms)
{
    if (atomic_read(&g_audio_wkq_init)) {
        os_work_schedule_delay(&g_audiodec_wkq, work, delay_ms);
		return RET_OK;
    } else {
        return -EINVAL;
    }
}

