#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "stream_frame.h"

#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

struct mp4_thumb_s
{
    struct msi *msi;
    uint8_t     thumb_name[64];
    uint8_t     filter_type;
    uint8_t     srcID;
};

int32_t mp4_thumb_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t             ret       = RET_OK;
    struct mp4_thumb_s *mp4_thumb = (struct mp4_thumb_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            STREAM_LIBC_FREE(mp4_thumb);
        }
        break;

        // 需要关闭,则将关联的msi关闭
        case MSI_CMD_PRE_DESTROY:
        {
        }
        break;

        // 接收到数据,唤醒workqueue,去组图,理论有图都要重新组,然后通过output的时候过滤类型,这里也可以先过滤部分类型
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;

            if (fb->mtype == F_JPG && mp4_thumb->srcID == fb->srcID && (!mp4_thumb->filter_type || mp4_thumb->filter_type == fb->stype))
            {
                struct framebuff *c_fb = fb_clone(fb, F_THUMB_JPG << 8 | fb->stype, msi);
                if (c_fb)
                {
                    c_fb->priv = mp4_thumb->thumb_name;
                    msi_output_fb(msi, c_fb);
                    msi->enable = 0;
                }
                break;
            }
            ret = RET_ERR;
        }
        break;
        case MSI_CMD_FREE_FB:
        {
        }
        break;
        default:
            break;
    }
    return ret;
}

struct msi *mp4_thumb_msi_init(const char *filename, uint8_t srcID, uint8_t filter)
{
    uint8_t     is_new;
    struct msi *msi = msi_new(R_MP4_THUMB, 0, &is_new);

    struct mp4_thumb_s *mp4_thumb = NULL;

    if (is_new)
    {
        mp4_thumb = (struct mp4_thumb_s *) STREAM_LIBC_ZALLOC(sizeof(struct mp4_thumb_s));
        os_memcpy(mp4_thumb->thumb_name, filename, strlen(filename) + 1);
        msi->priv              = mp4_thumb;
        mp4_thumb->msi         = msi;
        mp4_thumb->filter_type = filter;
        mp4_thumb->srcID       = srcID;
        msi->action            = mp4_thumb_msi_action;
        // 给到解码然后生成缩略图
        msi_add_output(msi, NULL, R_THUMB);
        msi->enable = 1;
        msi_add_output(NULL, AUTO_JPG, R_MP4_THUMB); // mp4缩略图?
    }
    else
    {
        mp4_thumb = (struct mp4_thumb_s *) msi->priv;
        os_memcpy(mp4_thumb->thumb_name, filename, strlen(filename) + 1);
        msi->enable = 1;
    }

    return msi;
}