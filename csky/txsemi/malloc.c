#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "hal/uart.h"
#include "lib/heap/sysheap.h"

#include "csi_core.h"

#if MPOOL_ALLOC
void *krhino_mm_alloc(size_t size, void *caller)
{
#if defined(PSRAM_HEAP) && defined(PSRAM_TASK_STACK)
    return sysheap_alloc(&psram_heap, size, caller, 0);
#else
    return sysheap_alloc(&sram_heap, size, caller, 0);
#endif
}

void krhino_mm_free(void *ptr)
{
#if defined(PSRAM_HEAP) && defined(PSRAM_TASK_STACK)
    os_free_psram(ptr);
#else
    os_free(ptr);
#endif
}
#endif

