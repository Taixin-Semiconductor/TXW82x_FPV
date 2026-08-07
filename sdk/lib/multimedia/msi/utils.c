#include "basic_include.h"
#include "lib/multimedia/msi.h"


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

