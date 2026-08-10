#include "aurpc_heap.h"

#if defined(MPOOL_ALLOC)

__bobj struct sys_sramheap aurpc_sram_heap;
__bobj struct sys_psramheap aurpc_psram_heap;

__init int32 aurpc_sram_heap_init(void *heap_start, uint32 heap_size, uint32 flags)
{
    flags |= SYSHEAP_FLAGS_MEM_ALIGN_32;
    flags |= SYSHEAP_FLAGS_MEM_LEAK_TRACE;
    if(heap_size > 0) {
        aurpc_sram_heap.name = "aurpc_sram_heap";
        aurpc_sram_heap.ops  = &mmpool1_ops;
        return sysheap_init(&aurpc_sram_heap, heap_start, heap_size, flags);
    }
    else {
        return RET_ERR;
    }
}

__init int32 aurpc_psram_heap_init(void *heap_start, uint32 heap_size, uint32 flags)
{
    flags |= SYSHEAP_FLAGS_MEM_ALIGN_32;
    flags |= SYSHEAP_FLAGS_MEM_LEAK_TRACE;
    if(heap_size > 0) {
        aurpc_psram_heap.name = "aurpc_psram_heap";
        aurpc_psram_heap.ops  = &mmpool1_ops;
        
        return sysheap_init(&aurpc_psram_heap, heap_start, heap_size, flags);
    }
    else {
        return RET_ERR;
    }
}

#endif