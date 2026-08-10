#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/sleep.h"

#include "csi_kernel.h"
#include <k_api.h>
#include <sys/time.h>

void os_sleep(int32 sec)
{
    uint32 ticks = csi_kernel_ms2tick(sec * 1000);
    if(ticks == 0) ticks = 1;
    csi_kernel_delay(ticks);
}

void os_sleep_ms(int32 msec)
{
    if(msec < OS_MS_PERIOD_TICK) 
        msec = OS_MS_PERIOD_TICK;
    csi_kernel_delay_ms(msec);
}

