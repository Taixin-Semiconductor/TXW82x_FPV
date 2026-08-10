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
    return RET_OK;
}

void rbuffer_destroy(struct rbuffer *rb)
{
    rbuffer_reset(rb);
    rb->qsize = 0;
}

void rbuffer_reset(struct rbuffer *rb)
{
    rb->wpos = 0;
    rb->rpos = 0;
}

int32 rbuffer_set(struct rbuffer *rb, void *data, uint32 length)
{
    uint32 len;
    uint32 rpos, wpos;
    //uint32 flag;

    if (rb->rbq == NULL) {
        return -ENOBUFS;
    }

    rpos = rb->rpos;
    wpos = rb->wpos;

    len = ((wpos < rpos) ? (rpos - wpos - 1) : ((rb)->qsize - wpos + rpos - 1));
    if (len < length) {
        length = len;
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

    rb->wpos = wpos;
    return length;
}

int32 rbuffer_set_force(struct rbuffer *rb, void *data, uint32 length)
{
    //uint32 flag;
    uint32 len;

    if (rb->rbq == NULL || (rb->qsize - 1) < length) {
        return -ENOBUFS;
    }

    len  = RB_IDLE(rb);
    if (len < length) {
        rb->rpos = RB_NPOS(rb, rpos, length - len);
    }
    return rbuffer_set(rb, data, length);
}

int32 rbuffer_get(struct rbuffer *rb, void *buff, uint32 size)
{
    int32 len = 0; // right length
    int32 count = 0;
    uint32 rpos, wpos;

    rpos = rb->rpos;
    wpos = rb->wpos;

    if (rb->rbq == NULL || rpos == wpos) {
        return 0;
    }

    count = ((rpos <= wpos) ? (wpos - rpos) : ((rb)->qsize - rpos + wpos));
    if (count > size) {
        count = size;
    }

    if (rpos <= wpos) {
        if (buff) {
            os_memcpy(buff, rb->rbq + rpos, count);
        }
    } else {
        len = rb->qsize - rpos;
        if (buff) {
            if (len >= count) {
                os_memcpy(buff, rb->rbq + rpos, count);
            } else {
                os_memcpy(buff, rb->rbq + rpos, len);
                os_memcpy(buff + len, rb->rbq, count - len);
            }
        }
    }

    rpos += count;
    if (rpos >= rb->qsize) {
        rpos -= rb->qsize;
    }

    rb->rpos = rpos;
    return count;
}

int32 rbuffer_move(struct rbuffer *rb, uint32 cur, uint32 offset)
{
    cur += offset;
    if (cur >= rb->qsize) {
        cur -= rb->qsize;
    }
    return cur;
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

