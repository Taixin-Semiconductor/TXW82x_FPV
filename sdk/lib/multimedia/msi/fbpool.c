#include "basic_include.h"
#include "lib/multimedia/msi.h"


#ifdef MORE_SRAM 
#define FBPOOL_MALLOC os_malloc_psram
#define FBPOOL_FREE   os_free_psram
#define FBPOOL_ZALLOC os_zalloc_psram
#else
#define FBPOOL_MALLOC os_malloc
#define FBPOOL_FREE   os_free
#define FBPOOL_ZALLOC os_zalloc
#endif

int32 fbpool_init(struct fbpool *pool, uint8 size)
{
    uint8 i;
    if (pool && !pool->inited) {
        pool->pool = FBPOOL_ZALLOC(sizeof(struct framebuff) * size);
        ASSERT(pool->pool);
        pool->inited = 1;
        pool->size   = size;
        for (i = 0; i < size; i++) {
            pool->pool[i].index = i;
            pool->pool[i].used = 0;
            pool->pool[i].pool = 1;
        }
    }
    return RET_OK;
}

struct framebuff *fbpool_get(struct fbpool *pool, uint16 type, struct msi *msi)
{
    uint8 i;
    uint32 flag;
    struct framebuff *fb = NULL;

    if (pool && pool->inited) {
        flag = disable_irq();
        for (i = 0; i < pool->size; i++) {
            if (!pool->pool[i].used) {
                fb = &pool->pool[i];
                fb->mtype = (type >> 8) & 0xff;
                fb->stype = type & 0xff;
                fb->msi  = msi;
                fb->used = 1;
                fb->next = NULL;
                msi_get(fb->msi);
                atomic_inc(&fb->users); //GET 加1
                break;
            }
        }
        enable_irq(flag);
    }
    return fb;
}

int32 fbpool_put(struct fbpool *pool, struct framebuff *fb)
{
    uint32 flag;
    if (pool && pool->inited && fb && fb->pool &&
        (fb->index < pool->size) && (&pool->pool[fb->index] == fb)) {
        flag = disable_irq();
        pool->pool[fb->index].used = 0;
        pool->pool[fb->index].msi  = NULL;
        pool->pool[fb->index].next = NULL;
        atomic_set(&fb->users,0);
        enable_irq(flag);
        return RET_OK;
    } else {
        os_printf(KERN_ERR"fbpool_put error, pool=%p, fb=%p\r\n", pool, fb);
        return RET_ERR;
    }
}

int32 fbpool_destroy(struct fbpool *pool)
{
    int32 i = 0;
    if (pool && pool->inited) {
        for (i = 0; i < pool->size; i++) {
            msi_discard_fb(pool->pool[i].msi, &pool->pool[i]);
        }
        FBPOOL_FREE(pool->pool);
        pool->inited = 0;
        pool->size   = 0;
    }
    return RET_OK;
}

