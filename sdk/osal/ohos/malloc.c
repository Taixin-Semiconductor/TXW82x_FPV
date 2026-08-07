#include "basic_include.h"
#include "los_config.h"
#include "los_compiler.h"

#ifdef OHOS

UINT8 *m_aucSysMem0 = NULL;

VOID *LOS_MemAlloc(VOID *pool, UINT32 size)
{
    void *caller = RETURN_ADDR();
#if defined(PSRAM_HEAP) && defined(PSRAM_TASK_STACK)
    return sysheap_alloc(&psram_heap, size, caller, 0);
#else
    return sysheap_alloc(&sram_heap, size, caller, 0);
#endif
}

UINT32 LOS_MemFree(VOID *pool, VOID *ptr)
{
#if defined(PSRAM_HEAP) && defined(PSRAM_TASK_STACK)
    os_free_psram(ptr);
#else
    os_free(ptr);
#endif
	return RET_OK;
}

VOID *LOS_MemAllocAlign(VOID *pool, UINT32 size, UINT32 boundary)
{
    void *ptr = LOS_MemAlloc(NULL, size + boundary);
    if(ptr){
        ptr = (void *)ALIGN((uint32)ptr, boundary);
    }
    return ptr;
}

UINT32 LOS_MemIntegrityCheck(const VOID *pool)
{
    return 0;
}

#endif

