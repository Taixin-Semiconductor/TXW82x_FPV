#include "sys_config.h"
#include "typesdef.h"
#include "osal/string.h"
#include "lib/heap/sysheap.h"
#include "lib/common/rbuffer.h"

#if defined(MPOOL_ALLOC) && defined(PSRAM_HEAP)
__bobj struct sys_psramheap psram_heap;

void *_os_malloc_psram(int size)
{
    return __malloc((struct sys_heap *)&psram_heap, size, RETURN_ADDR());
}

void _os_free_psram(void *ptr)
{
    __free((struct sys_heap *)&psram_heap, ptr, RETURN_ADDR());
}

void *_os_zalloc_psram(int size)
{
    return __zalloc((struct sys_heap *)&psram_heap, size, RETURN_ADDR());
}

void *_os_calloc_psram(int nmemb, int size)
{
    return __zalloc((struct sys_heap *)&psram_heap, nmemb * size, RETURN_ADDR());
}

void *_os_realloc_psram(void *ptr, int size)
{
    return __realloc((struct sys_heap *)&psram_heap, ptr, size, RETURN_ADDR());
}

void *_os_malloc_psram_t(int size, const char *func, int line)
{
    return __malloc_t((struct sys_heap *)&psram_heap, size, func, line);
}

void _os_free_psram_t(void *ptr, const char *func, int line)
{
    return __free_t((struct sys_heap *)&psram_heap, ptr, func, line);
}

void *_os_zalloc_psram_t(int size, const char *func, int line)
{
    return __zalloc_t((struct sys_heap *)&psram_heap, size, func, line);
}

void *_os_calloc_psram_t(int nmemb, int size, const char *func, int line)
{
    return __zalloc_t((struct sys_heap *)&psram_heap, nmemb * size, func, line);
}

void *_os_realloc_psram_t(void *ptr, int size, const char *func, int line)
{
    return __realloc_t((struct sys_heap *)&psram_heap, ptr, size, func, line);
}

void *malloc_psram(int size)                        __alias(_os_malloc_psram);
void  free_psram(void *ptr)                         __alias(_os_free_psram);
void *zalloc_psram(int size)                        __alias(_os_zalloc_psram);
void *calloc_psram(int nmemb, int size)             __alias(_os_calloc_psram);
void *realloc_psram(void *ptr, int size)            __alias(_os_realloc_psram);

#ifdef MALLOC_IN_PSRAM
void *malloc(size_t size)                           __alias(_os_malloc_psram);
void  free(void *ptr)                               __alias(_os_free_psram);
void *zalloc(size_t size)                           __alias(_os_zalloc_psram);
void *calloc(size_t nmemb, size_t size)             __alias(_os_calloc_psram);
void *realloc(void *ptr, size_t size)               __alias(_os_realloc_psram);
#endif

#endif

