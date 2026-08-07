#include "basic_include.h"
#include "lib/multimedia/msi.h"
#ifdef MORE_SRAM 
#define FBQ_MALLOC os_malloc_psram
#define FBQ_FREE   os_free_psram
#define FBQ_ZALLOC os_zalloc_psram
#else
#define FBQ_MALLOC os_malloc
#define FBQ_FREE   os_free
#define FBQ_ZALLOC os_zalloc
#endif

int32 fbq_init(struct fbqueue *q, uint8 *qbuff, int32 qsize)
{
    if (qbuff == NULL && qsize) {
        qbuff = (uint8 *)FBQ_MALLOC(sizeof(struct framebuff *) * (qsize + 1));
        q->alloc = 1;
    }

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

int32 fbq_destory(struct fbqueue *q)
{
    if (q && q->init) {
        while (RB_COUNT(&q->rbQ)) {
            fb_put(fbq_dequeue(q, 0));
        }
        if (q->alloc) {
            FBQ_FREE(q->rbQ.rbq);
        }
        if (q->readers) {
            FBQ_FREE(q->readers);
        }
        os_sema_del(&q->sema);
        q->init = 0;
    }
    return RET_OK;
}

int32 fbq_enqueue(struct fbqueue *q, struct framebuff *fb)
{
    int32 ret;
    uint32 i;
    uint32 rpos;
    uint32 flags;
    struct framebuff *last = NULL;

    if (!q || !q->init || !fb) {
        return 0;
    }

    fb_get(fb);

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
            fb_put(fb);
            return 0;
        }
    }
}

static int32 fbq_trace_pos(struct fbqueue *q, struct framebuff *fb, uint32 start, uint32 end, int8 discard)
{
    uint32 pos;
    for (pos = start; pos < end; pos++) {
        if (q->rbQ.rbq[pos] == fb) {
            if (discard) {
                q->rbQ.rbq[pos] = NULL;
            }
            return 1;
        }
    }
    return 0;
}

int32 fbq_trace(struct fbqueue *q, struct framebuff *fb, int8 discard)
{
    uint32 ret = 0;
    uint32 flag;

    if (!q || !q->init || !fb) {
        return 0;
    }

    flag = disable_irq();
    if (q->rbQ.rpos <= q->rbQ.wpos) {
        ret = fbq_trace_pos(q, fb, q->rbQ.rpos, q->rbQ.wpos, discard);
    } else {
        ret = fbq_trace_pos(q, fb, 0, q->rbQ.wpos, discard);
        if (!ret) {
            ret = fbq_trace_pos(q, fb, q->rbQ.rpos, q->rbQ.qsize, discard);
        }
    }
    enable_irq(flag);

    if (ret && discard) {
        fb_put(fb);
    }
    return 0;
}

struct framebuff *fbq_dequeue(struct fbqueue *q, uint32 tmo_ms)
{
    uint64 jiff;
    struct framebuff *fb;

    if (!q || !q->init) {
        return NULL;
    }

    do {
        if (!RB_EMPTY(&q->rbQ)) {
            RB_GET(&q->rbQ, fb);
            return fb;
        }

        if(tmo_ms == 0){
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

    ptr = (uint32 *)FBQ_MALLOC(reader_max * sizeof(uint32));
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

