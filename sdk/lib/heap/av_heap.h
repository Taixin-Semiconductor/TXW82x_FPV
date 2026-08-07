#ifndef _AV_HEAP
#define _AV_HEAP
#include "sys_config.h"
#include "typesdef.h"
#include "osal/string.h"
#if defined(MPOOL_ALLOC) && defined(AV_HEAP)
extern struct sys_av_heap av_heap;
void av_heap_init(uint8_t *start_addr,uint32_t size,uint32_t flags);
void av_heap_status(uint32_t *buf,uint32_t size);
void *av_malloc(int size);
void av_free(void *ptr);
void *av_zalloc(int size);
void *av_calloc(int nmemb, int size);
void *av_realloc(void *ptr, int size);
void *av_malloc_t(int size, const char *func, int line);
void av_free_t(void *ptr, const char *func, int line);
void *av_zalloc_t(int size, const char *func, int line);
void *av_calloc_t(int nmemb, int size, const char *func, int line);
void *av_realloc_t(void *ptr, int size, const char *func, int line);
#else

#define av_malloc _os_malloc
#define av_free _os_free
#define av_zalloc _os_zalloc
#define av_calloc _os_calloc
#define av_realloc _os_realloc
#endif


#endif