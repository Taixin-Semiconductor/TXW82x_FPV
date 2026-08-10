#include "basic_include.h"
#include "lib/multimedia/msi.h"

static void fb_free(struct framebuff *fb)
{
    if (fb) {
        struct msi *msi = fb->msi;
        ASSERT(atomic_read(&fb->users) > 0 && fb->free);
        if (!atomic_dec_and_test(&fb->users)) {
            return;
        }

        if (msi) {
            if (!fb->pool) atomic_inc(&msi->fb_limits);
            msi_do_cmd(msi, MSI_CMD_FREE_FB, (uint32)fb, 0);
        }

        fb->free(fb, fb->free_priv);

        if(msi){
            msi_do_cmd(msi, MSI_CMD_FREE_FB_END, 0, 0);
            msi_put(msi);
        }
    }
}

//framebuff 引用计数加1
void fb_get(struct framebuff *fb)
{
    if (fb) {
        fb_get(fb->next);
        atomic_inc(&fb->users);
    }
}

struct framebuff *fb_clone(struct framebuff *fb, uint16 type, struct msi *msi, void *priv)
{
    struct framebuff *fb_n = msi_alloc_fb(msi, priv, fb->data, fb->len, 0, 0);
    if (fb_n) {
        fb_n->clone = 1;
        fb_n->time  = fb->time;
        fb_n->priv  = fb->priv;
        fb_n->codec_info  = fb->codec_info;
        fb_n->srcID = fb->srcID;
        fb_n->mtype = type >> 8;
        fb_n->stype = type & 0xff;
        fb_n->datatag = fb->datatag;
        fb_get(fb);
        fb_n->next = fb;
    }
    return fb_n;
}

//framebuff 引用计数减1，当计数减至0时会释放空间
void fb_put(struct framebuff *fb)
{
    if (fb) {
        fb_put(fb->next);
        fb_free(fb);
    }
}

//使用新的framebuff引用关联另1个framebuff，fb_old引用计数加1
void fb_ref(struct framebuff *fb_new, struct framebuff *fb_old)
{
    ASSERT(fb_new->next == NULL);
    fb_get(fb_old);
    fb_new->next = fb_old;
}

//获取framebuff链表中指定type的数据的第1个节点
struct framebuff *fb_find(struct framebuff *fb, uint8 mtype, uint8 stype)
{
    if (mtype == 0 || fb == NULL || fb->next == NULL) {
        return fb;
    }

    while (fb) {
        if (fb->mtype == mtype && (fb->stype == stype || stype == 0)) {
            return fb;
        }
        fb = fb->next;
    }
    return NULL;
}

//获取framebuff链表中指定type的数据的总长度
uint32 fb_len(struct framebuff *fb,  uint8 mtype, uint8 stype)
{
    uint32 len = 0;

    if (mtype == 0 || fb == NULL) {
        return fb ? fb->len : 0;
    }

    if (fb->mtype != mtype) {
        return 0;
    }

    while (fb) {
        if (fb->mtype == mtype && (fb->stype == stype || stype == 0)) {
            len += fb->len;
        }
        fb = fb->next;
    }
    return len;
}

