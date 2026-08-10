#ifndef _AURPC_HEAP_H_
#define _AURPC_HEAP_H_

#include "sys_config.h"
#include "typesdef.h"
#include "osal/string.h"
#include "lib/heap/sysheap.h"

#if defined(MPOOL_ALLOC)

extern struct sys_sramheap aurpc_sram_heap;
extern struct sys_psramheap aurpc_psram_heap;

int32 aurpc_sram_heap_init(void *heap_start, uint32 heap_size, uint32 flags);
int32 aurpc_psram_heap_init(void *heap_start, uint32 heap_size, uint32 flags);

#endif

#endif