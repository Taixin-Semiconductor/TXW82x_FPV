#ifndef __HG_LV_MEM_H_
#define __HG_LV_MEM_H_

typedef void* (*hg_lv_malloc) (int size);
typedef void* (*hg_lv_zalloc) (int size);
typedef void* (*hg_lv_realloc) (void *ptr, int size);
typedef void  (*hg_lv_free) (void *ptr);
typedef void* (*hg_lv_memset)(void *addr, int c, unsigned int len);
typedef void* (*hg_lv_memcpy)(void *dest, const void *src, unsigned int len);

struct hg_lv_mem_hooks {
    hg_lv_malloc malloc;
    hg_lv_zalloc zalloc;
    hg_lv_realloc realloc;
    hg_lv_memset memset;
    hg_lv_memcpy memcpy;
    hg_lv_free free;
};

void *hg_lv_mem_malloc(int size);
void *hg_lv_mem_zalloc(int size);
void *hg_lv_mem_realloc(void *ptr, int size);
void hg_lv_mem_free(void *ptr);
void *hg_lv_mem_memset(void *addr, int c, unsigned int len);
void *hg_lv_mem_memcpy(void *dest, const void *src, unsigned int len);
int hg_lv_mem_register(struct hg_lv_mem_hooks* hook);
int hg_lv_mem_unregister();

#endif