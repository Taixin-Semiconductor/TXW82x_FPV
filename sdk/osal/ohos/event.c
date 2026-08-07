#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/event.h"
#include "osal/string.h"

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

#define EVENT_MAGIC (0xa67b3cd4)

int32 os_event_init(os_event_t *evt)
{
    UINT32 ret;
    PEVENT_CB_S pstEventCB;

    if(evt->magic == EVENT_MAGIC){
        os_printf(KERN_WARNING"event repeat initialization ????\r\n");
    }

    pstEventCB = (PEVENT_CB_S)os_malloc(sizeof(EVENT_CB_S));
    if (pstEventCB == NULL) {
        return RET_ERR;
    }

    ret = LOS_EventInit(pstEventCB);
    if (ret == LOS_OK) {
        evt->hdl = pstEventCB;
        evt->magic = EVENT_MAGIC;
        return RET_OK;
    } else {
        os_free(pstEventCB);
        return RET_ERR;
    }
}

int32 os_event_del(os_event_t *evt)
{
    ASSERT(evt && evt->magic == EVENT_MAGIC);
    int32 ret = LOS_EventDestroy((PEVENT_CB_S)evt->hdl);
    os_free(evt->hdl);
    evt->hdl = NULL;
    evt->magic = 0;
    return ret;
}

int32 os_event_set(os_event_t *evt, uint32 flags, uint32 *rflags)
{
    PEVENT_CB_S pstEventCB = (PEVENT_CB_S)evt->hdl;
    UINT32 ret;

    ASSERT(evt && evt->magic == EVENT_MAGIC);
    if (pstEventCB == NULL) {
        return -EINVAL;
    }

    ret = LOS_EventWrite(pstEventCB, (UINT32)flags);
    if (ret == LOS_OK) {
        *rflags = pstEventCB->uwEventID;
        return ret;
    } else {
        return RET_ERR;
    }
}

int32 os_event_clear(os_event_t *evt, uint32 flags, uint32 *rflags)
{
    PEVENT_CB_S pstEventCB = (PEVENT_CB_S)evt->hdl;
    UINT32 intSave;
    UINT32 ret;

    if (pstEventCB == NULL) {
        return RET_ERR;
    }

    intSave = LOS_IntLock();
    *rflags = pstEventCB->uwEventID;
    ret = LOS_EventClear(pstEventCB, ~flags);
    LOS_IntRestore(intSave);
    return ret;
}

int32 os_event_get(os_event_t *evt, uint32 *rflags)
{
    PEVENT_CB_S pstEventCB = (PEVENT_CB_S)evt->hdl;
    UINT32 intSave;

    if (pstEventCB == NULL) {
        return RET_ERR;
    }

    intSave = LOS_IntLock();
    *rflags = pstEventCB->uwEventID;
    LOS_IntRestore(intSave);

    return 0;
}

int32 os_event_wait(os_event_t *evt, uint32 flags, uint32 *rflags, uint32 mode, int32 timeout)
{
    PEVENT_CB_S pstEventCB = (PEVENT_CB_S)evt->hdl;
    UINT32 imode = 0;
    UINT32 ret;

    if (pstEventCB == NULL) {
        return RET_ERR;
    }

    if (OS_INT_ACTIVE && (timeout != 0)) {
        return RET_ERR;
    }

    if (mode & OS_EVENT_WMODE_OR) {
        imode |= LOS_WAITMODE_OR;
    }

    if (mode & OS_EVENT_WMODE_AND) {
        imode |= LOS_WAITMODE_AND;
    }

    if (mode & OS_EVENT_WMODE_CLEAR) {
        imode &= ~LOS_WAITMODE_CLR;
    } else {
        imode |= LOS_WAITMODE_CLR;
    }

    ret = LOS_EventRead(pstEventCB, (UINT32)flags, imode, (UINT32)timeout);
    switch (ret) {
        case LOS_ERRNO_EVENT_PTR_NULL:
        case LOS_ERRNO_EVENT_EVENTMASK_INVALID:
        case LOS_ERRNO_EVENT_FLAGS_INVALID:
        case LOS_ERRNO_EVENT_SETBIT_INVALID:
            return RET_ERR;

        case LOS_ERRNO_EVENT_READ_IN_INTERRUPT:
        case LOS_ERRNO_EVENT_READ_IN_LOCK:
            return RET_ERR;

        case LOS_ERRNO_EVENT_READ_TIMEOUT:
            return -ETIMEDOUT;

        default:
            *rflags = (uint32_t)ret;
            return RET_OK;
    }
}

#endif

