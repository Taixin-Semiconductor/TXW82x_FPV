#include "sys_config.h"
#include "typesdef.h"
#include "osal/string.h"
#include "lib/heap/sysheap.h"
#include "lib/common/rbuffer.h"

#if defined(MPOOL_ALLOC) && defined(AV_HEAP)
__bobj struct sys_av_heap av_heap;

__init void av_heap_init(void *start_addr,uint32_t size,uint32_t flags)
{
    //uint32 flags = SYSHEAP_FLAGS_MEM_ALIGN_32;
    //flags |= SYSHEAP_FLAGS_MEM_LEAK_TRACE | SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK;
    if(size)
    {
        av_heap.name = "av_sram";
        av_heap.ops  = &mmpool1_ops;
        sysheap_init(&av_heap, (void *)start_addr, size, flags);
    }
}

//打印当前heap的状态,参数为buf以及buf的空间(world为单位)
void av_heap_status(uint32_t *buf,uint32_t size)
{
    if(size)
    {
        sysheap_status(&av_heap, buf, size, 0);
    }
}

void *av_malloc(int size)
{
    return __malloc((struct sys_heap *)&av_heap, size, RETURN_ADDR());
}

void av_free(void *ptr)
{
    __free((struct sys_heap *)&av_heap, ptr, RETURN_ADDR());
}

void *av_zalloc(int size)
{
    return __zalloc((struct sys_heap *)&av_heap, size, RETURN_ADDR());
}

void *av_calloc(int nmemb, int size)
{
    return __zalloc((struct sys_heap *)&av_heap, nmemb * size, RETURN_ADDR());
}

void *av_realloc(void *ptr, int size)
{
    return __realloc((struct sys_heap *)&av_heap, ptr, size, RETURN_ADDR());
}

void *av_malloc_t(int size, const char *func, int line)
{
    return __malloc_t((struct sys_heap *)&av_heap, size, func, line);
}

void av_free_t(void *ptr, const char *func, int line)
{
    return __free_t((struct sys_heap *)&av_heap, ptr, func, line);
}

void *av_zalloc_t(int size, const char *func, int line)
{
    return __zalloc_t((struct sys_heap *)&av_heap, size, func, line);
}

void *av_calloc_t(int nmemb, int size, const char *func, int line)
{
    return __zalloc_t((struct sys_heap *)&av_heap, nmemb * size, func, line);
}

void *av_realloc_t(void *ptr, int size, const char *func, int line)
{
    return __realloc_t((struct sys_heap *)&av_heap, ptr, size, func, line);
}

#endif

