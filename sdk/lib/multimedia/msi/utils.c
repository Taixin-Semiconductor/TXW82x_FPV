#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/AVContainer.h"


static int32 msi_rbuffer_fb_exist(struct rbuffer *rb, struct framebuff *fb, uint32 start, uint32 end)
{
    uint32 i = 0;
    struct framebuff **q = (struct framebuff **)rb->rbq;

    for (i = start; i < end; i++) {
        if (q[i] == fb) {
            return 1;
        } else {
            struct framebuff *next = q[i]->next;
            while (next) {
                if (next == fb) {
                    return 1;
                } else {
                    next = next;
                }
            }
        }
    }
    return 0;
}

int32 msi_rbuffer_trace_fb(struct msi *mif, struct rbuffer *rb, struct framebuff *fb)
{
    uint32 rpos = rb->rpos;
    uint32 wpos = rb->wpos;

    if (!RB_EMPTY(rb)) {
        if (rpos < wpos) {
            if (msi_rbuffer_fb_exist(rb, fb, rpos, wpos)) {
                os_printf(KERN_NOTICE"MSI %s: FB %p is here!\r\n", mif->name, fb);
                return 1;
            }
        } else {
            if (msi_rbuffer_fb_exist(rb, fb, rpos, rb->qsize)) {
                os_printf(KERN_NOTICE"MSI %s: FB %p is here!\r\n", mif->name, fb);
                return 1;
            }
            if (msi_rbuffer_fb_exist(rb, fb, 0, wpos)) {
                os_printf(KERN_NOTICE"MSI %s: FB %p is here!\r\n", mif->name, fb);
                return 1;
            }
        }
    }
    return 0;
}

const struct AVDemuxer *AVDemuxer_Get(uint32 type)
{
    extern uint32 __avdemuxer_start;
    extern uint32 __avdemuxer_end;
    uint32 *start = (uint32 *)&__avdemuxer_start;
    uint32 *end   = (uint32 *)&__avdemuxer_end;
    const struct AVDemuxer *demuxer = (const struct AVDemuxer *)start;

    while ((uint32 *)demuxer < end) {
        if (demuxer->type == type) {
            return demuxer;
        }
        demuxer++;
    }
    return NULL;
}

#if 0
const struct AVMuxer *AVMuxer_Get(uint32 type)
{
    extern uint32 __avmuxer_start;
    extern uint32 __avmuxer_end;
    uint32 *start = (uint32 *)&__avmuxer_start;
    uint32 *end   = (uint32 *)&__avmuxer_end;
    const struct AVMuxer *muxer = (struct AVMuxer *)start;

    while ((uint32 *)muxer < end) {
        if (muxer->type == type) {
            return muxer;
        }
        muxer++;
    }
    return NULL;
}
#endif

void *decoder_mem_zalloc(size_t size)
{
    void *p = decoder_mem_alloc(size);
    if (p) {
        os_memset(p, 0, size);
    }
    return p;
}

void *decoder_mem_realloc(void *ptr, size_t size)
{
    void *nptr = decoder_mem_alloc(size);
    if (nptr) {
        if (ptr) {
            os_memcpy(nptr, ptr, size);
            decoder_mem_free(ptr);
        }
    } else {
        os_printf(KERN_WARNING"decoder_mem_realloc failed, be careful of memory leaks! (size=%d, LR:%p)\r\n", size, RETURN_ADDR());
    }
    return nptr;
}

void *decoder_mem_calloc(size_t nitems, size_t size)
{
    void *ptr = decoder_mem_alloc(nitems * size);
    if(ptr){
        os_memset(ptr, 0, nitems * size);
    }
    return ptr;
}

void *encoder_mem_zalloc(size_t size)
{
    void *p = decoder_mem_alloc(size);
    if (p) {
        os_memset(p, 0, size);
    }
    return p;
}

void *encoder_mem_realloc(void *ptr, size_t size)
{
    void *nptr = decoder_mem_alloc(size);
    if (nptr) {
        if (ptr) {
            os_memcpy(nptr, ptr, size);
            decoder_mem_free(ptr);
        }
    } else {
        os_printf(KERN_WARNING"encoder_mem_realloc failed, be careful of memory leaks! (size=%d, LR:%p)\r\n", size, RETURN_ADDR());
    }
    return nptr;
}

void *encoder_mem_calloc(size_t nitems, size_t size)
{
    return decoder_mem_alloc(nitems * size);
}

