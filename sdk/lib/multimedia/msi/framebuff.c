#include "basic_include.h"
#include "lib/multimedia/msi.h"

static void fb_free(struct framebuff *fb)
{
    int32 ret = 0;
    struct msi *msi;

    if (fb) {
        ASSERT(atomic_read(&fb->users) > 0);
        if (!atomic_dec_and_test(&fb->users)) {
            return;
        }

        msi = fb->msi;
        ret = msi_do_cmd(fb->msi, MSI_CMD_FREE_FB, (uint32)fb, 0);
        msi_put(msi);
        if (ret == RET_OK) { //不等于OK表示模块自己管理fb，例如预分配的fb
            ASSERT(!fb->pool); //预分配的framebuff不能执行到这里
            FB_FREE(fb);
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

struct framebuff *fb_alloc(uint8 *data, int32 size, uint16 type, struct msi *msi)
{
    int32 len = data ? 0 : size;
    struct framebuff *fb = FB_ALLOC(sizeof(struct framebuff) + len);
    if (fb) {
        os_memset(fb, 0, sizeof(struct framebuff));
        atomic_set(&fb->users, 1);
        fb->len  = size;
        fb->msi  = msi;
        fb->mtype = (type >> 8) & 0xff;
        fb->stype = type & 0xff;
        msi_get(fb->msi);
        if (data) {
            fb->data = data;
        } else if (size) {
            fb->data = (uint8 *)(fb + 1);
        }
    }
    return fb;
}

struct framebuff *fb_clone(struct framebuff *fb, uint16 type, struct msi *msi)
{
    struct framebuff *fb_n = FB_ALLOC(sizeof(struct framebuff));
    if (fb_n) {
        os_memset(fb_n, 0, sizeof(struct framebuff));
        atomic_set(&fb_n->users, 1);
        fb_n->clone = 1;
        fb_n->msi  = msi;
        fb_n->mtype = (type >> 8) & 0xff;
        fb_n->stype = type & 0xff;
        msi_get(fb_n->msi);
        fb_n->data = fb->data;
        fb_n->len  = fb->len;
        fb_n->time = fb->time;
        fb_n->priv = fb->priv;
        fb_n->srcID = fb->srcID;
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

//根据fb->mtype 识别fb携带的数据为 视频，音频，图片，字幕
MEDIA_DATA_CATEGORY fb_category(struct framebuff *fb)
{
    if (fb->mtype < 32) {        //video
        return MEDIA_DATA_VIDEO;
    } else if (fb->mtype < 64) { //audio
        return MEDIA_DATA_AUDIO;
    } else if (fb->mtype < 96) { //picture
        return MEDIA_DATA_PICTURE;
    } else if (fb->mtype < 128) { //text
        return MEDIA_DATA_TEXT;
    } else {
        return MEDIA_DATA_MAX;
    }
}

