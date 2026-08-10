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

#include "osal/string.h"
#include "osal/task.h"
#include "los_cpup.h"


void os_kernel_init(void)
{
	LOS_KernelInit();
}

void os_kernel_start(void)
{
	LOS_Start();
}

void os_console_uart(void *uart)
{
}

void OS_INTRPT_ENTER(int irqn)
{
}

void OS_INTRPT_EXIT(int irqn)
{
}

