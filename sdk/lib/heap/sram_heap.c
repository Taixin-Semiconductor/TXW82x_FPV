#include "sys_config.h"
#include "typesdef.h"
#include "osal/string.h"
#include "lib/heap/sysheap.h"
#include "lib/common/rbuffer.h"

#ifdef MPOOL_ALLOC
__bobj struct sys_sramheap  sram_heap;

void *_os_malloc(int size)
{
    return __malloc((struct sys_heap *)&sram_heap, size, RETURN_ADDR());
}

void _os_free(void *ptr)
{
    __free((struct sys_heap *)&sram_heap, ptr, RETURN_ADDR());
}

void *_os_zalloc(int size)
{
    return __zalloc((struct sys_heap *)&sram_heap, size, RETURN_ADDR());
}

void *_os_calloc(size_t nmemb, size_t size)
{
    return __zalloc((struct sys_heap *)&sram_heap, nmemb * size, RETURN_ADDR());
}

void *_os_realloc(void *ptr, int size)
{
    return __realloc((struct sys_heap *)&sram_heap, ptr, size, RETURN_ADDR());
}

#ifndef MALLOC_IN_PSRAM
void *malloc(size_t size)                 __alias(_os_malloc);
void  free(void *ptr)                     __alias(_os_free);
void *zalloc(size_t size)                 __alias(_os_zalloc);
void *calloc(size_t nmemb, size_t size)   __alias(_os_calloc);
void *realloc(void *ptr, size_t size)     __alias(_os_realloc);
#endif

#ifndef PSRAM_HEAP
void *malloc_psram(int size)                        __alias(_os_malloc);
void  free_psram(void *ptr)                         __alias(_os_free);
void *zalloc_psram(int size)                        __alias(_os_zalloc);
void *calloc_psram(int nmemb, int size)             __alias(_os_calloc);
void *realloc_psram(void *ptr, int size)            __alias(_os_realloc);
void *_os_malloc_psram(int size)                    __alias(_os_malloc);
void  _os_free_psram(void *ptr)                     __alias(_os_free);
void *_os_zalloc_psram(int size)                    __alias(_os_zalloc);
void *_os_calloc_psram(int nmemb, int size)         __alias(_os_calloc);
void *_os_realloc_psram(void *ptr, int size)        __alias(_os_realloc);
#endif

#ifndef AV_HEAP
void *_av_malloc(int size)                                             __alias(_os_malloc);
void  _av_free(void *ptr)                                              __alias(_os_free);
void *_av_zalloc(int size)                                             __alias(_os_zalloc);
void *_av_calloc(int nmemb, int size)                                  __alias(_os_calloc);
void *_av_realloc(void *ptr, int size)                                 __alias(_os_realloc);
#endif

#endif

