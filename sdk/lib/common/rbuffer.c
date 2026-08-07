#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "errno.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "lib/common/rbuffer.h"

int32 rbuffer_init(struct rbuffer *rb, uint32 size, void *buff)
{
    ASSERT(buff && size > 1);
    os_memset(rb, 0, sizeof(struct rbuffer));
    rb->qsize = size;
    rb->rbq   = buff;
    rb->bpos  = -1;
    return RET_OK;
}

void rbuffer_destroy(struct rbuffer *rb)
{
    rbuffer_reset(rb);
    rb->qsize = 0;
}

void rbuffer_reset(struct rbuffer *rb)
{
    uint32 flag = disable_irq();
    rb->wpos = 0;
    rb->rpos = 0;
    enable_irq(flag);
}

int32 rbuffer_set(struct rbuffer *rb, void *data, uint32 length)
{
    uint32 len;
    uint32 rpos, wpos;
    uint32 flag;

    if (rb->rbq == NULL) {
        return -ENOBUFS;
    }

    flag = disable_irq();
    rpos = rb->rpos;
    wpos = rb->wpos;
    enable_irq(flag);

    len = ((wpos < rpos) ? (rpos - wpos - 1) : ((rb)->qsize - wpos + rpos - 1));
    if (len < length) {
        return -ENOBUFS;
    }

    if (wpos < rpos) {
        os_memcpy(rb->rbq + wpos, data, length);
    } else {
        len = rb->qsize - wpos;
        if (len < length) { //rewind
            os_memcpy(rb->rbq + wpos, data, len);
            os_memcpy(rb->rbq, data + len, length - len);
        } else {
            os_memcpy(rb->rbq + wpos, data, length);
        }
    }

    wpos += length;
    if (wpos >= rb->qsize) {
        wpos -= rb->qsize;
    }

    flag = disable_irq();
    rb->wpos = wpos;
    enable_irq(flag);
    return length;
}

int32 rbuffer_set_force(struct rbuffer *rb, void *data, uint32 length)
{
    uint32 flag;
    uint32 len;

    if (rb->rbq == NULL || (rb->qsize - 1) < length) {
        return -ENOBUFS;
    }

    flag = disable_irq();
    len  = RB_IDLE(rb);
    if (len < length) {
        rb->rpos = RB_NPOS(rb, rpos, length - len);
    }
    enable_irq(flag);
    return rbuffer_set(rb, data, length);
}

int32 rbuffer_get(struct rbuffer *rb, void *buff, uint32 size)
{
    int32 len = 0; // right length
    int32 count = 0;
    uint32 rpos, wpos;
    uint32 flag;

    flag = disable_irq();
    rpos = rb->rpos;
    wpos = rb->wpos;
    enable_irq(flag);

    if (rb->rbq == NULL || rpos == wpos) {
        return 0;
    }

    count = ((rpos <= wpos) ? (wpos - rpos) : ((rb)->qsize - rpos + wpos));
    if (count > size) {
        count = size;
    }

    if (rpos <= wpos) {
        os_memcpy(buff, rb->rbq + rpos, count);
    } else {
        len = rb->qsize - rpos;
        if (len >= count) {
            os_memcpy(buff, rb->rbq + rpos, count);
        } else {
            os_memcpy(buff, rb->rbq + rpos, len);
            os_memcpy(buff + len, rb->rbq, count - len);
        }
    }

    rpos += count;
    if (rpos >= rb->qsize) {
        rpos -= rb->qsize;
    }

    flag = disable_irq();
    rb->rpos = rpos;
    enable_irq(flag);
    return count;
}

int32 rbuffer_set_block(struct rbuffer *rb, void *data, uint32 length, uint8 block)
{
    uint32 bpos = rb->wpos;
    int32 ret  = rbuffer_set(rb, data, length);
    if (ret == length && block) {
        rb->bpos = bpos;
    }
    return ret;
}

int32 rbuffer_get_block(struct rbuffer *rb, void *buff, uint32 size, uint8 *block)
{
    if (rb->rpos == rb->bpos) {
        *block = 1;
        rb->bpos = -1;
    } else {
        *block = 0;
    }
    return rbuffer_get(rb, buff, size);
}

int32 rbuffer_alloc(struct rbuffer *rb, uint32 size)
{
    void *buff = os_malloc(size);
    ASSERT(buff);
    return rbuffer_init(rb, size, buff);
}

void rbuffer_free(struct rbuffer *rb)
{
    rb->qsize = 0;
    os_free(rb->rbq);
}

