#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/event.h"

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
#include "osal/sleep.h"

void os_sleep(int32 sec)
{
    uint32 ticks = LOS_MS2Tick(sec * 1000);
    if(ticks == 0) ticks = 1;
    LOS_TaskDelay(ticks);
}

void os_sleep_ms(int32 msec)
{
    uint32 ticks = LOS_MS2Tick(msec);
    if(ticks == 0) ticks = 1;
    LOS_TaskDelay(ticks);
}


