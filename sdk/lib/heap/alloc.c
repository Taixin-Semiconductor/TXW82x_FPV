#include "sys_config.h"
#include "typesdef.h"
#include "osal/string.h"
#include "lib/heap/sysheap.h"
#include "lib/common/rbuffer.h"

#ifdef MEM_RECLIST_CNT
static uint16 g_mfreelist_pos, g_malloclist_pos;
struct mem_reclist {
    void  *addr;
    void  *lr;
    uint64 tick;
};
struct mem_reclist g_mfreelist[MEM_RECLIST_CNT];
struct mem_reclist g_malloclist[MEM_RECLIST_CNT];
#endif

void mem_free_rec(void *addr, void *lr)
{
#ifdef MEM_RECLIST_CNT
    uint32 flag = disable_irq();
    g_mfreelist[g_mfreelist_pos].addr = addr;
    g_mfreelist[g_mfreelist_pos].lr   = lr;
    g_mfreelist[g_mfreelist_pos].tick = os_jiffies();
    g_mfreelist_pos++;
    if (g_mfreelist_pos >= MEM_RECLIST_CNT) {
        g_mfreelist_pos = 0;
    }
    enable_irq(flag);
#endif
}

void mem_alloc_rec(void *addr, void *lr)
{
#ifdef MEM_RECLIST_CNT
    uint32 flag = disable_irq();
    g_malloclist[g_malloclist_pos].addr = addr;
    g_malloclist[g_malloclist_pos].lr   = lr;
    g_malloclist[g_malloclist_pos].tick = os_jiffies();
    g_malloclist_pos++;
    if (g_malloclist_pos >= MEM_RECLIST_CNT) {
        g_malloclist_pos = 0;
    }
    enable_irq(flag);
#endif
}

void mem_reclist_dump(void)
{
#ifdef MEM_RECLIST_CNT
    uint32 i = 0;
    uint32 flag = disable_irq();
    os_printf("------------------------------------------\r\n");
    os_printf("MEM ALLOC Reclist:\r\n");
    for (i = 0; i < MEM_RECLIST_CNT; i++) {
        if (g_malloclist[i].addr) {
            os_printf("     MEM:%p, LR:%p, tick:%lld\r\n", g_malloclist[i].addr, g_malloclist[i].lr, g_malloclist[i].tick);
        }
    }
    os_printf("MEM FREE Reclist:\r\n");
    for (i = 0; i < MEM_RECLIST_CNT; i++) {
        if (g_mfreelist[i].addr) {
            os_printf("     MEM:%p, LR:%p, tick:%lld\r\n", g_mfreelist[i].addr, g_mfreelist[i].lr, g_mfreelist[i].tick);
        }
    }
    os_printf("------------------------------------------\r\n");
    enable_irq(flag);
#endif
}

#ifdef MPOOL_ALLOC
void *__malloc(struct sys_heap *heap, int size, void *lr)
{
    void *ptr = sysheap_alloc(heap, size, lr, 0);
    if (ptr) {
        mem_alloc_rec(ptr, lr);
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, heap->pool.align));
    } else {
        os_printf(KERN_WARNING"%s: malloc fail, size=%d [LR:%p]\tremain size:%d\r\n", heap->name, size, lr,sysheap_freesize(heap));
    }
    return ptr;
}

void *__zalloc(struct sys_heap *heap, int size, void *lr)
{
    void *ptr = __malloc(heap, size, lr);
    if (ptr) {
        os_memset(ptr, 0, size);
    }
    return ptr;
}

void *__realloc(struct sys_heap *heap, void *ptr, int size, void *lr)
{
    void *nptr = __malloc(heap, size, lr);
    if (nptr) {
        if (ptr) {
            os_memcpy(nptr, ptr, size);
            __free(heap, ptr, lr);
        }
    }else{
        os_printf(KERN_WARNING"Realloc failed, be careful of memory leaks! (size=%d, LR:%p)\r\n", size, lr);
    }    
    return nptr;
}
void __free(struct sys_heap *heap, void *ptr, void *lr)
{
    if (ptr) {
        mem_free_rec(ptr, lr);
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, heap->pool.align));
        if (sysheap_free(heap, ptr)) {
            os_printf(KERN_WARNING"%s: free error, ptr=%p, [LR:%p]\r\n", heap->name, ptr, lr);
        }
    }
}

void *__malloc_t(struct sys_heap *heap, int size, const char *func, int line)
{
    void *ptr = sysheap_alloc(heap, size, func, line);
    if (ptr) {
        mem_alloc_rec(ptr, (void *)func);
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, heap->pool.align));
    } else {
        os_printf(KERN_WARNING"%s: malloc fail, size=%d [%s:%d]\tremain size:%d\r\n", heap->name, size, func, line, sysheap_freesize(heap));
    }
    return ptr;
}

void *__zalloc_t(struct sys_heap *heap, int size, const char *func, int line)
{
    void *ptr = __malloc_t(heap, size, func, line);
    if (ptr) {
        os_memset(ptr, 0, size);
    }
    return ptr;
}

void *__realloc_t(struct sys_heap *heap, void *ptr, int size, const char *func, int line)
{
    void *nptr = __malloc_t(heap, size, func, line);
    if (nptr) {
        if (ptr) {
            os_memcpy(nptr, ptr, size);
            __free_t(heap, ptr, func, line);
        }
    }else{
        os_printf(KERN_WARNING"Realloc failed, be careful of memory leaks! (size=%d, %s:%d)\r\n", size, func, line);
    }

    return nptr;
}

void __free_t(struct sys_heap *heap, void *ptr, const char *func, int line)
{
    if (ptr) {
        mem_free_rec(ptr, (void *)func);
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, heap->pool.align));
        if (sysheap_free(heap, ptr)) {
            os_printf(KERN_WARNING"%s: free error, ptr=%p, [%s:%d]\r\n", heap->name, ptr, func, line);
        }
    }
}
#endif


