#include "sys_config.h"
#include "typesdef.h"
#include "osal/string.h"
#include "lib/heap/sysheap.h"
#include "lib/common/rbuffer.h"

/***********************************************************************
MEM_RECLIST - 内存 申请/释放 操作流水记录。
使用方法：
  1. 添加宏定义： 
     #define MEM_RECLIST_CNT 256  - 表示记录最后256条内存访问记录

  2. 在需要检测的 申请/释放 API里面添加：mem_alloc_rec 和 mem_free_rec
      -申请内存成功时调用：mem_alloc_rec
      -释放内存时调用：mem_free_rec
     __malloc和__free函数有示例代码。

  3. 执行 mem_reclist_dump API 打印被记录的内存访问信息
     SDK在检测到内存越界或内存信息损坏时，也会自动执行 mem_reclist_dump
***********************************************************************/
//#define MEM_RECLIST_CNT 256
#ifdef MEM_RECLIST_CNT
struct mem_reclist {
    void  *addr;
    void  *lr;
    uint64 tick;
};
static uint16 g_mfreelist_pos, g_malloclist_pos;
static struct mem_reclist g_mfreelist[MEM_RECLIST_CNT];
static struct mem_reclist g_malloclist[MEM_RECLIST_CNT];
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
        //mem_alloc_rec(ptr, lr);
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
        //mem_free_rec(ptr, lr);
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, heap->pool.align));
        if (sysheap_free(heap, ptr)) {
            os_printf(KERN_WARNING"%s: free error, ptr=%p, [LR:%p]\r\n", heap->name, ptr, lr);
        }
    }
}

void *__malloc_t(struct sys_heap *heap, int size, void *lr, int line)
{
    void *ptr = sysheap_alloc(heap, size, lr, line);
    if (ptr) {
        //mem_alloc_rec(ptr, (void *)func);
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, heap->pool.align));
    } else {
        os_printf(KERN_WARNING"%s: malloc fail, size=%d [lr:%p:%d]\tremain size:%d\r\n", heap->name, size, lr, line, sysheap_freesize(heap));
    }
    return ptr;
}

void *__zalloc_t(struct sys_heap *heap, int size, void *lr, int line)
{
    void *ptr = __malloc_t(heap, size, lr, line);
    if (ptr) {
        os_memset(ptr, 0, size);
    }
    return ptr;
}


void __free_t(struct sys_heap *heap, void *ptr, void *lr, int line)
{
    if (ptr) {
        //mem_free_rec(ptr, (void *)func);
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, heap->pool.align));
        if (sysheap_free(heap, ptr)) {
            os_printf(KERN_WARNING"%s: free error, ptr=%p, [lr:%p:%d]\r\n", heap->name, ptr, lr, line);
        }
    }
}

void *__realloc_t(struct sys_heap *heap, void *ptr, int size, void *lr, int line)
{
    void *nptr = __malloc_t(heap, size, lr, line);
    if (nptr) {
        if (ptr) {
            os_memcpy(nptr, ptr, size);
            __free_t(heap, ptr, lr, line);
        }
    }else{
        os_printf(KERN_WARNING"Realloc failed, be careful of memory leaks! (size=%d, lr:%p:%d)\r\n", size, lr, line);
    }

    return nptr;
}

#endif


