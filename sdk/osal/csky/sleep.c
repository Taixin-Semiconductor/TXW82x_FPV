#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/sleep.h"

#ifdef CSKY_OS
#include "csi_kernel.h"
#include <k_api.h>
#include <sys/time.h>

void os_sleep(int32 sec)
{
    csi_kernel_delay(csi_kernel_ms2tick(sec * 1000));
}

void os_sleep_ms(int32 msec)
{
    csi_kernel_delay_ms(msec);
}

#endif

