#include "sys_config.h"
#include "typesdef.h"
#include "errno.h"
#include "osal/semaphore.h"
#include "osal/task.h"
#include "osal/string.h"
#include "osal/timer.h"
#include "osal/mutex.h"
#include "lib/common/rbuffer.h"
#include "lib/posix/pthread.h"

#ifdef TXWSDK_POSIX

int sched_yield(void)
{
    os_task_yield();
    return 0;
}

int sched_get_priority_min(int policy)
{
    return OS_TASK_PRIORITY_LOW;
}

int sched_get_priority_max(int policy)
{
    return OS_TASK_PRIORITY_HIGH;
}

int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param)
{
    return -EOPNOTSUPP;
}

#endif
