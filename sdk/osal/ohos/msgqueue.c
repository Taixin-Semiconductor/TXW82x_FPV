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
#include "osal/msgqueue.h"

#define MSGQ_MAGIC (0x4a8b1c9d)

int32 os_msgq_init(os_msgqueue_t *msgq, int32 size)
{
    UINT32 hdl = 0;

    if(msgq->magic == MSGQ_MAGIC){
        os_printf(KERN_WARNING"msgq repeat initialization ????\r\n");
    }
    
    if (LOS_OK == LOS_QueueCreate("msgq", size, &hdl, 0, sizeof(uint32))) {
        msgq->hdl = (void *)(hdl + 1);
        msgq->magic = MSGQ_MAGIC;
    }
    return (msgq->hdl ? RET_OK : RET_ERR);
}

uint32 os_msgq_get(os_msgqueue_t *msgq, int32 tmo_ms)
{
    UINT32 val = 0;
    UINT32 hdl = (UINT32)msgq->hdl;
    ASSERT(msgq->hdl);
    LOS_QueueRead(hdl - 1, &val, sizeof(uint32), LOS_MS2Tick(tmo_ms));
    return val;
}

uint32 os_msgq_get2(struct os_msgqueue *msgq, int32 tmo_ms, int32 *err)
{
    UINT32 ret;
    UINT32 val = 0;
    UINT32 hdl = (UINT32)msgq->hdl;
    ASSERT(msgq->hdl);
    ret = LOS_QueueRead(hdl - 1, &val, sizeof(uint32), LOS_MS2Tick(tmo_ms));
    if(err) *err = ret;
    return val;
}

int32 os_msgq_put(os_msgqueue_t *msgq, uint32 data, int32 tmo_ms)
{
    UINT32 hdl = (UINT32)msgq->hdl;
    ASSERT(msgq->hdl);
    return LOS_QueueWrite(hdl - 1, (void *)data, sizeof(uint32), LOS_MS2Tick(tmo_ms));
}

int32 os_msgq_put_head(os_msgqueue_t *msgq, uint32 data, int32 tmo_ms)
{
    UINT32 hdl = (UINT32)msgq->hdl;
    ASSERT(msgq->hdl);
    return LOS_QueueWriteHead(hdl - 1, (void *)data, sizeof(uint32), LOS_MS2Tick(tmo_ms));
}

int32 os_msgq_del(os_msgqueue_t *msgq)
{
    ASSERT(msgq->hdl);
    UINT32 hdl = (UINT32)msgq->hdl;
    ASSERT(msgq->hdl);
    LOS_QueueDelete(hdl - 1);
    msgq->hdl = NULL;
    msgq->magic = 0;
    return RET_OK;
}

int32 os_msgq_cnt(os_msgqueue_t *msgq)
{
    QUEUE_INFO_S queueInfo;
    UINT32 hdl = (UINT32)msgq->hdl;
    ASSERT(msgq->hdl);
    memset(&queueInfo, 0, sizeof(queueInfo));
    LOS_QueueInfoGet(hdl - 1, &queueInfo);
    return queueInfo.readableCnt;
}

#endif

