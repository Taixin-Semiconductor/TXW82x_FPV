#include "osal/string.h"
#include "hg_lv_mem.h"

static struct hg_lv_mem_hooks *g_func = NULL;

void *hg_lv_mem_malloc(int size)
{
    if (g_func) {
        if (g_func->malloc) {
            return g_func->malloc(size);
        }
    }
    return os_malloc_psram(size);
}

void *hg_lv_mem_zalloc(int size)
{
    if (g_func) {
        if (g_func->zalloc) {
            return g_func->zalloc(size);
        }
    }
    return os_zalloc_psram(size);
}

void *hg_lv_mem_realloc(void *ptr, int size)
{
    if (g_func) {
        if (g_func->realloc) {
            return g_func->realloc(ptr, size);
        }
    }
    return os_realloc_psram(ptr, size); 
}

void hg_lv_mem_free(void *ptr)
{
    if (g_func) {
        if (g_func->free) {
            g_func->free(ptr);
            return;
        }
    }
    os_free_psram(ptr);
    return;
}

void *hg_lv_mem_memset(void *addr, int c, unsigned int len)
{
    if (g_func) {
        if (g_func->memset) {
            return g_func->memset(addr, c, len);
        }
    }
    return os_memset(addr, c, len);
}

void *hg_lv_mem_memcpy(void *dest, const void *src, unsigned int len)
{
    if (g_func) {
        if (g_func->memcpy) {
            return g_func->memcpy(dest, src, len);
        }
    }
    return os_memcpy(dest, src, len);
}

int hg_lv_mem_register(struct hg_lv_mem_hooks* hook)
{
    if (g_func != NULL) {
        os_printf("%s %d had already register hg lv mem!\n",__FUNCTION__,__LINE__);
        return 1;
    }

    struct hg_lv_mem_hooks *func = (struct hg_lv_mem_hooks *)os_zalloc(sizeof(struct hg_lv_mem_hooks));

    if (func) {
        if (hook) {
            if (hook->malloc)  {
                func->malloc  = hook->malloc;
            }
        
            if (hook->zalloc)  {
                func->zalloc  = hook->zalloc;
            }
        
            if (hook->realloc) {
                func->realloc = hook->realloc;
            }
        
            if (hook->free)    {
                func->free    = hook->free;
            }

            if (hook->memcpy)  {
                func->memcpy  = hook->memcpy;
            }

            if (hook->memset)  {
                func->memset  = hook->memset;
            }
        }
    } else {
        os_printf("%s %d malloc func failed!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    g_func = func;
    return 0;
}

int hg_lv_mem_unregister()
{
    if (g_func) {
        os_free(g_func);
        g_func = NULL;
    }
    return 0;
}