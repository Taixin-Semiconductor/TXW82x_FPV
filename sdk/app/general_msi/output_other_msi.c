#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"


#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#ifdef MORE_SRAM
#define STREAM_LIBC_MALLOC av_psram_malloc
#define STREAM_LIBC_FREE   av_psram_free
#define STREAM_LIBC_ZALLOC av_psram_zalloc
#else
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc
#endif

typedef struct
{
    struct msi  *msi;
    struct msi  *target;
} recv_msi_s;

static int32 output_other_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int          ret = RET_OK;
    recv_msi_s  *rm = (recv_msi_s *) msi->priv;

    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            msi_put(rm->target);
            STREAM_FREE(rm);
        }
        break;
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            if (rm->target && fb)
            {
                fb_get(fb);
                msi_output_fb(rm->target, fb, 0);
            }
        }
        break;
        default:
            break;
    }
    return ret;
}

struct msi *new_output_other_msi(const char *name, struct msi *target)
{
    int32_t     err   = RET_ERR;
    struct msi *msi   = NULL;
    uint8_t     isnew = 0;
    recv_msi_s *rm    = NULL;

    msi = msi_new(name, 0, &isnew);
    if (msi && isnew)
    {
        rm = (recv_msi_s *) STREAM_ZALLOC(sizeof(recv_msi_s));
        if (!rm)
        {
            goto output_other_msi_end;
        }

        rm->msi    = msi;
        rm->target = msi_get(target);
        msi->priv   = (void *) rm;
        msi->action = output_other_msi_action;
        msi->enable = 1;
        err = RET_OK;
    }

output_other_msi_end:
    if (err)
    {
        if (rm)   STREAM_FREE(rm);
        if (msi)  msi_destroy(msi), msi = NULL;
    }
    return msi;
}
