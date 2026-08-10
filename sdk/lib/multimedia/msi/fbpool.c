#include "basic_include.h"
#include "lib/multimedia/msi.h"

static int32 fbpool_put2(struct framebuff *fb, struct fbpool *pool)
{
    return fbpool_put(pool, fb);
}

int32 fbpool_init(struct fbpool *pool, uint8 size, mfree_cb_t free_cb, void *free_priv)
{
    uint8 i;
    if (pool && !pool->inited) {
        pool->pool = fb_mem_alloc(sizeof(struct framebuff) * size);
        ASSERT(pool->pool);
        pool->inited = 1;
        pool->size   = size;
        os_memset(pool->pool, 0, sizeof(struct framebuff) * size);
        for (i = 0; i < size; i++) {
            pool->pool[i].index = i;
            pool->pool[i].pool  = 1;
            pool->pool[i].mtype = 0xff;
            pool->pool[i].stype = 0xff;
            pool->pool[i].free = free_cb ? free_cb : (mfree_cb_t)fbpool_put2;
            pool->pool[i].free_priv = free_priv ? free_priv : pool;
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
                pool->used++;
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
        pool->pool[fb->index].mtype = 0xff;
        pool->pool[fb->index].stype = 0xff;
        atomic_set(&fb->users, 0);
        pool->used--;
        enable_irq(flag);
        return RET_OK;
    } else {
        os_printf(KERN_ERR"fbpool_put error, pool=%p, fb=%p\r\n", pool, fb);
        return RET_ERR;
    }
}

int32 fbpool_destroy(struct fbpool *pool)
{
    uint8 i = 0;
    uint32 flag;
    struct msi *msi = NULL;

    if (pool && pool->inited) {
        os_printf("fbpool size:%d, used %d.\r\n", pool->size, pool->used);
        flag = disable_irq();
        for (i = 0; i < pool->size; i++) {
            msi = pool->pool[i].msi;
            enable_irq(flag);
            msi_discard_fb(msi, &pool->pool[i], NULL);
            flag = disable_irq();
        }
        enable_irq(flag);

        i = 0;
        while(pool->used){ 
            if(i++ == 0) os_printf(KERN_WARNING"fbpool: wait for fb to be released! %d\r\n", pool->used);
            os_sleep_ms(5);
        }

        fb_mem_free(pool->pool);
        pool->inited = 0;
        pool->size   = 0;
    }
    return RET_OK;
}

