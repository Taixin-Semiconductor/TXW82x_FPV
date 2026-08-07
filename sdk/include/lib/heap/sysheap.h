#ifndef _SYS_HEAP_H_
#define _SYS_HEAP_H_

#include "lib/heap/mmpool.h"

#ifdef __cplusplus
extern "C" {
#endif

enum SYSHEAP_FLAGS {
    SYSHEAP_FLAGS_MEM_LEAK_TRACE     = (1u << 0),
    SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK = (1u << 1),
    SYSHEAP_FLAGS_MEM_ALIGN_16       = (1u << 2),
    SYSHEAP_FLAGS_MEM_ALIGN_32       = (1u << 3),
    SYSHEAP_FLAGS_MEM_TAIL_ALIGN_4   = (1u << 4),
    SYSHEAP_FLAGS_MEM_TAIL_ALIGN_16  = (1u << 5),
    SYSHEAP_FLAGS_MEM_TAIL_ALIGN_32  = (1u << 6),
};

struct sys_heap {
    const char *name;
    const struct mmpool_ops *ops;
    struct mmpool_base pool;
};

struct sys_sramheap {
    const char *name;
    const struct mmpool_ops *ops;
    struct mmpool1 pool; 
    //可以修改为mmpool3, 同时修改malloc_init函数：sram_heap.ops  = &mmpool3_ops;
};

struct sys_psramheap {
    const char *name;
    const struct mmpool_ops *ops;
    struct mmpool1 pool; 
    //可以修改为mmpool3, 同时修改malloc_psram_init函数：psram_heap.ops  = &mmpool3_ops;
};

struct sys_av_heap {
    const char *name;
    const struct mmpool_ops *ops;
    struct mmpool1 pool;
    //可以修改为mmpool3, 同时修改初始化函数的ops赋值
};

void *__malloc(struct sys_heap *heap, int size, void *lr);
void __free(struct sys_heap *heap, void *ptr, void *lr);
void *__zalloc(struct sys_heap *heap, int size, void *lr);
void *__realloc(struct sys_heap *heap, void *ptr, int size, void *lr);
void *__malloc_t(struct sys_heap *heap, int size, const char *func, int line);
void __free_t(struct sys_heap *heap, void *ptr, const char *func, int line);
void *__zalloc_t(struct sys_heap *heap, int size, const char *func, int line);
void *__realloc_t(struct sys_heap *heap, void *ptr, int size, const char *func, int line);

int32 _sysheap_init(struct sys_heap *heap, void *heap_start, uint32 heap_size, uint32 flags);
void *_sysheap_alloc(struct sys_heap *heap, int size, const char *func, int line);
int32 _sysheap_free(struct sys_heap *heap, void *ptr);
uint32 _sysheap_freesize(struct sys_heap *heap);
uint32 _sysheap_totalsize(struct sys_heap *heap);
void _sysheap_collect_init(struct sys_heap *heap);
int32 _sysheap_add(struct sys_heap *heap, uint32 start_addr, uint32 end_addr);
int32 _sysheap_of_check(struct sys_heap *heap, void *ptr, uint32 size);
void _sysheap_status(struct sys_heap *heap, uint32 *status_buf, int32 buf_size, uint32 mini_size);
int32 _sysheap_valid_addr(struct sys_heap *heap, void *ptr, uint8 first);
uint32 _sysheap_time(struct sys_heap *heap);
int32 _sysheap_used_list(struct sys_heap *heap, uint32 *list_buf, int32 buf_size);
void _sysheap_dump(struct sys_heap *heap);

#define sysheap_init(heap, heap_start, heap_size, flags) \
    _sysheap_init((struct sys_heap *)(heap), heap_start, heap_size, flags)

#define sysheap_time(heap)\
    _sysheap_time((struct sys_heap *)(heap))

#define sysheap_alloc(heap, size, func, line)\
    _sysheap_alloc((struct sys_heap *)(heap), size, (const char *)func, line)

#define sysheap_free(heap, ptr)\
    _sysheap_free((struct sys_heap *)(heap), ptr)

#define sysheap_freesize(heap)\
    _sysheap_freesize((struct sys_heap *)(heap))

#define sysheap_totalsize(heap)\
    _sysheap_totalsize((struct sys_heap *)(heap))

#define sysheap_collect_init(heap)\
    _sysheap_collect_init((struct sys_heap *)(heap))

#define sysheap_add(heap, start_addr, end_addr)\
    _sysheap_add((struct sys_heap *)(heap), start_addr, end_addr)

#define sysheap_of_check(heap, ptr, size)\
    _sysheap_of_check((struct sys_heap *)(heap), ptr, size)

#define sysheap_status(heap, status_buf, buf_size, mini_size)\
    _sysheap_status((struct sys_heap *)(heap), status_buf, buf_size, mini_size)

#define sysheap_valid_addr(heap, ptr, first)\
    _sysheap_valid_addr((struct sys_heap *)(heap), ptr, first)

#define sysheap_time(heap)\
    _sysheap_time((struct sys_heap *)(heap))

#define sysheap_used_list(heap, list_buf, buf_size)\
    _sysheap_used_list((struct sys_heap *)(heap), list_buf, buf_size)

#define sysheap_dump(heap)\
    _sysheap_dump((struct sys_heap *)(heap))

extern struct sys_sramheap  sram_heap;
extern struct sys_psramheap psram_heap;

#endif

#ifdef __cplusplus
}
#endif

