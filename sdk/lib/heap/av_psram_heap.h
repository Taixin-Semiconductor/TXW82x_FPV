#ifndef _AV_PSRAM_HEAP
#define _AV_PSRAM_HEAP
#include "sys_config.h"
#include "typesdef.h"
#include "osal/string.h"
#if defined(MPOOL_ALLOC) && defined(AV_PSRAM_HEAP) && defined(PSRAM_HEAP)
extern struct sys_psramheap av_psram_heap;
void av_psram_heap_init(uint8_t *start_addr,uint32_t size,uint32_t flags);
void av_psram_heap_status(uint32_t *buf,uint32_t size);
void *av_psram_malloc(int size);
void av_psram_free(void *ptr);
void *av_psram_zalloc(int size);
void *av_psram_calloc(int nmemb, int size);
void *av_psram_realloc(void *ptr, int size);
void *av_psram_malloc_t(int size, const char *func, int line);
void av_psram_free_t(void *ptr, const char *func, int line);
void *av_psram_zalloc_t(int size, const char *func, int line);
void *av_psram_calloc_t(int nmemb, int size, const char *func, int line);
void *av_psram_realloc_t(void *ptr, int size, const char *func, int line);
#else

#define av_psram_malloc _os_malloc_psram
#define av_psram_free _os_free_psram
#define av_psram_zalloc _os_zalloc_psram
#define av_psram_calloc _os_calloc_psram
#define av_psram_realloc _os_realloc_psram
#endif


#endif