#include "basic_include.h"
#include "lib/multimedia/msi.h"

int32 fbq_init(struct fbqueue *q, uint8 *qbuff, int32 qsize, struct msi *msi)
{
    if (qbuff == NULL && qsize) {
        qbuff = (uint8 *)fb_mem_alloc(sizeof(struct framebuff *) * (qsize + 1));
        q->alloc = 1;
    }

    q->msi = msi;
    ASSERT(qbuff);
    if (qbuff && qsize) {
        RB_INIT_R(&q->rbQ, (qsize + 1), (struct framebuff **)qbuff);
        os_sema_init(&q->sema, 0);
        q->reader_max = 1;
        q->readers = NULL;
        q->init = 1;
        return RET_OK;
    }
    return RET_ERR;
}

void fbq_clear(struct fbqueue *q)
{
    while (RB_COUNT(&q->rbQ)) {
        fb_put(fbq_dequeue(q, 0));
    }
}

int32 fbq_count(struct fbqueue *q)
{
    return RB_COUNT(&q->rbQ);
}

int32 fbq_destroy(struct fbqueue *q)
{
    if (q && q->init) {
        fbq_clear(q);
        if (q->alloc) {
            fb_mem_free(q->rbQ.rbq);
        }
        if (q->readers) {
            fb_mem_free(q->readers);
        }
        os_sema_del(&q->sema);
        q->init = 0;
    }
    return RET_OK;
}

int32 fbq_enqueue(struct fbqueue *q, struct framebuff *fb, uint8 ref)
{
    int32 ret;
    uint32 i;
    uint32 rpos;
    uint32 flags;
    struct framebuff *last = NULL;

    if (!q || !q->init || !fb) {
        return 0;
    }

    if (ref) {
        fb_get(fb);
    }

    if (q->reader_max > 1) {
        flags = disable_irq();
        if (RB_FULL(&q->rbQ)) { // FULL ! move rpos.
            last = q->rbQ.rbq[q->rbQ.rpos];
            rpos = RB_NPOS(&q->rbQ, rpos, 1);
            for (i = 0; i < q->reader_max; i++) {
                if (q->readers[i] == q->rbQ.rpos) {
                    q->readers[i] = rpos;
                }
            }
            q->rbQ.rpos = rpos;
        }
        q->rbQ.rbq[q->rbQ.wpos] = fb;
        q->rbQ.wpos = RB_NPOS(&q->rbQ, wpos, 1);
        enable_irq(flags);

        fb_put(last);
        return 1;
    } else {
        ret = RB_INT_SET(&q->rbQ, fb);
        if (ret) {
            os_sema_up(&q->sema);
            return 1;
        } else {
            q->full++;
            if (q->full >= 0x64) {
                os_printf(KERN_WARNING"[%s] input fb queue full: %d\r\n", q->msi->name, q->full);
                q->full = 0;
            }
            if (ref) {
                fb_put(fb);
            }
            return 0;
        }
    }
}

static void fbq_trace_pos(struct fbqueue *q, struct framebuff *fb, struct msi *owner, int8 discard, int8 dump, uint32 start, uint32 end)
{
    int32 i = 0;
    uint32 flag;
    struct framebuff *p;

    for (i = start; i < end; i++) {
        flag = disable_irq();
        p = q->rbQ.rbq[i];
        if (p){
            fb_get(p);
            if (discard && ((p == fb) || (owner && p->msi == owner))) {
                q->rbQ.rbq[i] = NULL;
            }
        }
        enable_irq(flag);

        if (p == NULL) continue;

        if (dump) {
            os_printf(KERN_NOTICE"    fb:%p, users:%d, type:%d/%d, time:%d, data:%p, len:%d, srcID:%d, Tag:%d%s%s%s%s%s, msi:%s\r\n",
                      p, p->users.counter, p->mtype, p->stype, p->time, p->data, p->len, p->srcID, p->datatag,
                      p->pool ? ", Pool" : ", Alloc",
                      p->used ? ", Used" : "",
                      p->clone ? ", Clone" : "",
                      p->keyfrm ? ", KeyFrame" : "",
                      p->last ? ", Last" : "",
                      p->msi ? p->msi->name : "");
        }

        if ((p == fb) || (owner && p->msi == owner)) {
            if (discard) {
                fb_put(p);
            } else {
                os_printf(KERN_NOTICE"%s: trace fb:%p, owner:%s!\r\n", q->msi->name, p, p->msi->name);
            }
        }
        fb_put(p);
    }
}

void fbq_trace(struct fbqueue *q, struct framebuff *fb, struct msi *owner, int8 discard, int8 dump)
{
    uint32 rpos = q->rbQ.rpos;
    uint32 wpos = q->rbQ.wpos;

    if (dump && q->init && rpos != wpos) {
        os_printf(KERN_NOTICE"fbQ [%s], Count:%d/%d\r\n", q->msi ? q->msi->name : "", fbq_count(q), q->rbQ.qsize);
    }

    if (rpos <= wpos) {
        fbq_trace_pos(q, fb, owner, discard, dump, rpos, wpos);
    } else {
        fbq_trace_pos(q, fb, owner, discard, dump, rpos, q->rbQ.qsize);
        fbq_trace_pos(q, fb, owner, discard, dump, 0, wpos);
    }
}

void fbq_dump(struct fbqueue *q)
{
    return fbq_trace(q, NULL, NULL, 0, 1);
}

struct framebuff *fbq_dequeue(struct fbqueue *q, uint32 tmo_ms)
{
    uint64 jiff;
    uint32 flags;
    struct framebuff *fb;

    if (!q || !q->init) {
        return NULL;
    }

    do {
        flags = disable_irq();
        if (!RB_EMPTY(&q->rbQ)) {
            RB_GET_CLEAN(&q->rbQ, fb);
            enable_irq(flags);
            return fb;
        }
        enable_irq(flags);

        if (tmo_ms == 0) {
            return NULL;
        }

        jiff = os_jiffies();
        os_sema_down(&q->sema, tmo_ms);
        jiff = DIFF_JIFFIES(jiff, os_jiffies());
        jiff = os_jiffies_to_msecs(jiff);
        if (jiff >= tmo_ms) {
            break;
        }

        tmo_ms -= jiff;
    } while (tmo_ms);

    return NULL;
}

struct framebuff *fbq_dequeue_r(struct fbqueue *q, uint8 reader)
{
    uint32 i = 0;
    uint32 pos;
    uint32 flags;
    struct framebuff *fb  = NULL;
    struct framebuff *last = NULL;

    if (!q || !q->init) {
        return NULL;
    }

    ASSERT(reader > 0 && reader <= q->reader_max);
    ASSERT(q->readers[reader - 1] != 0xffffffff);

    reader -= 1;
    flags = disable_irq();
    pos   = q->readers[reader];

    if (pos == q->rbQ.wpos) { //no fb for this reader
        enable_irq(flags);
        return fb;
    }

    fb = q->rbQ.rbq[pos];
    q->readers[reader] = NEXT_RPOS(pos, q->rbQ.qsize, 1);

    //move rpos: find min pos.
    pos = 0xffffffff;
    for (i = 0; i < q->reader_max; i++) {
        if (q->readers[i] < pos) {
            pos = q->readers[i];
        }
    }
    if (pos != 0xffffffff && q->rbQ.rpos != pos) {
        last = q->rbQ.rbq[q->rbQ.rpos];
        q->rbQ.rpos = pos;
    }

    fb_get(fb);
    enable_irq(flags);

    fb_put(last);
    return fb;
}

int32 fbq_conf_readers(struct fbqueue *q, uint16 reader_max)
{
    uint32 i;
    uint32 *ptr;

    if (!q || !q->init || reader_max <= 1 || q->readers) {
        return -EINVAL;
    }

    ptr = (uint32 *)fb_mem_alloc(reader_max * sizeof(uint32));
    if (ptr) {
        for (i = 0; i < q->reader_max; i++) {
            q->readers[i] = 0xffffffff;
        }
        q->readers = ptr;
        q->reader_max = reader_max;
        return RET_OK;
    }
    return -ENOMEM;
}

int32 fbq_open_reader(struct fbqueue *q)
{
    uint32 i;
    uint32 flags;

    if (!q || !q->init || !q->readers) {
        return 0;
    }

    flags = disable_irq();
    for (i = 0; i < q->reader_max; i++) {
        if (q->readers[i] == 0xffffffff) {
            q->readers[i] = q->rbQ.rpos;
            break;
        }
    }
    enable_irq(flags);
    return i < q->reader_max ? (i + 1) : 0;
}

int32 fbq_close_reader(struct fbqueue *q, uint8 reader)
{
    uint32 flags;

    if (!q || !q->init || !q->readers) {
        return -EINVAL;
    }

    ASSERT(reader > 0 && reader <= q->reader_max);
    flags = disable_irq();
    q->readers[reader - 1] = 0xffffffff;
    enable_irq(flags);
    return RET_OK;
}

