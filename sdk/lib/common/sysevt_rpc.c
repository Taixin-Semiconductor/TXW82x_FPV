#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "osal/string.h"
#include "osal/mutex.h"
#include "osal/work.h"
#include "lib/common/rbuffer.h"
#include "lib/common/sysevt.h"
#include "lib/rpc/cpurpc.h"

int32 sys_event_new(uint32 event_id, uint32 data)
{
    uint32 args[] = {event_id, data};
    switch(event_id){
        case SYS_EVENT(SYS_EVENT_SYSTEM, SYSEVT_TASK_DELETE): //过滤掉，不能发送到CPU0
            return RET_OK;
        default:
            return CPU_RPC_CALL(sys_event_new);            
    }
}

int32 sys_ieee80211_event_rpc(uint8 ifidx, uint16 evt, uint32 param1, uint32 param2)
{
    uint32 args[] = {ifidx, evt, param1, param2};
    return CPU_RPC_CALL(sys_wifi_event_cb);
}

