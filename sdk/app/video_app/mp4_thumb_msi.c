#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "stream_define.h"

#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "user_work/user_work.h"
#include "recode/jpg_recode.h"
#include "file_thumb.h"
#include "sdfunc_work/sdfunc_work.h"

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
    struct msi    *msi;        // 主要接收缩略图的msi
    struct msi    *output_msi; // 接收jpg图片并且过滤的msi
    struct msi    *thumb_msi;  // 产生缩略图的msi,输入jpg,输出也是jpg
    char           rand_msi_name[32];
    char           rand_thumb_msi_name[32];
    char           rand_output_msi_name[32];
    uint8_t        thumb_name[64];
    uint8_t        filter_type;
    uint8_t        srcID;
};

struct mp4_thumb_output_s
{
    struct msi    *msi;
    char           thumb_name[64];
};

static int32_t mp4_thumb_output_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t                    ret              = RET_OK;
    struct mp4_thumb_output_s *mp4_thumb_output = (struct mp4_thumb_output_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            sd_fb_write_work(fb, mp4_thumb_output->thumb_name, 1);
            // 只需要写一张缩略图,所以这里直接关闭
            msi->enable = 0;
            ret         = RET_ERR;
            break;
        }
        // 缩略图释放比较特殊,需要释放对应的私有结构体
        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            if (fb && fb->priv)
            {
                STREAM_LIBC_FREE(fb->priv);
            }
            break;
        }

        case MSI_CMD_PRE_DESTROY:
        {
            break;
        }
        case MSI_CMD_POST_DESTROY:
        {
            STREAM_LIBC_FREE(mp4_thumb_output);
            break;
        }
        default:
        {
            break;
        }
    }
    return ret;
}
// 这name是内部,是动态
// 因为是缩略图使用,所以仅仅接收一个缩略图
static struct msi *mp4_thumb_output_msi(const char *name, const char *thumb_name)
{
    uint8_t     isnew;
    struct msi *msi = msi_new(name, 0, &isnew);
    if (msi && isnew)
    {
        struct mp4_thumb_output_s *mp4_thumb_output = (struct mp4_thumb_output_s *) STREAM_LIBC_MALLOC(sizeof(struct mp4_thumb_output_s));
        ASSERT(mp4_thumb_output);
        msi->priv             = (void *) mp4_thumb_output;
        mp4_thumb_output->msi = msi;
        gen_thumb_path(thumb_name, mp4_thumb_output->thumb_name, strlen(thumb_name) + 1);
        msi->type   = F_JPG << 8 | 0xff;
        msi->action = mp4_thumb_output_msi_action;
        // 需要将缩略图给到文件写入
        msi_add_output(msi, NULL, NULL, R_FILE_MSI);
        msi->enable = 1;
    }
    else
    {
        if (msi)
        {
            msi_destroy(msi);
            msi = NULL;
        }
    }
    return msi;
}

static int32_t mp4_thumb_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
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
            // 将对应的msi关闭
            if (mp4_thumb->thumb_msi)
            {
                msi_destroy(mp4_thumb->thumb_msi);
                mp4_thumb->thumb_msi = NULL;
            }

            if (mp4_thumb->output_msi)
            {
                msi_destroy(mp4_thumb->output_msi);
                mp4_thumb->output_msi = NULL;
            }
        }

        break;

        // 接收到数据,唤醒workqueue,去组图,理论有图都要重新组,然后通过output的时候过滤类型,这里也可以先过滤部分类型
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            // 如果fb匹配,需要解码,就给到thumb_msi
            // 然后就停止接收,缩略图只需要一张
            if (fb->mtype == F_JPG && mp4_thumb->srcID == fb->srcID && (!mp4_thumb->filter_type || mp4_thumb->filter_type == fb->stype))
            {
                msi_recv_fb(mp4_thumb->thumb_msi, fb);
                msi->enable = 0;
            }
            ret = RET_ERR;
        }
        break;

        default:
            break;
    }
    return ret;
}

struct msi *new_mp4_thumb_msi(const char *filename, uint8_t srcID, uint8_t filter)
{
    // 一个随机名称?然后其他msi输入
    uint8_t             is_new    = 0;
    struct mp4_thumb_s *mp4_thumb = (struct mp4_thumb_s *) STREAM_LIBC_ZALLOC(sizeof(struct mp4_thumb_s));
    os_sprintf(mp4_thumb->rand_msi_name, "mp4_thumb_[%05d]_%p", (uint32_t) os_jiffies() % 99999, mp4_thumb);
    os_sprintf(mp4_thumb->rand_thumb_msi_name, "jpg_to_jpg_[%05d]_%p", (uint32_t) os_jiffies() % 99999, mp4_thumb);
    os_sprintf(mp4_thumb->rand_output_msi_name, "mp4_thumb_output_[%05d]_%p", (uint32_t) os_jiffies() % 99999, mp4_thumb);
    // 由于名称是随机,只要内存够,不应该失败
    // 由于是转发到thumb内部去实现jpg产生缩略图,所以这里不需要队列
    struct msi *msi = msi_new(mp4_thumb->rand_msi_name, 0, &is_new);
    if (msi && is_new)
    {
        msi->priv              = mp4_thumb;
        mp4_thumb->msi         = msi;
        mp4_thumb->filter_type = filter;
        mp4_thumb->srcID       = srcID;
        msi->action            = mp4_thumb_msi_action;
        mp4_thumb->thumb_msi   = new_jpg_recode_msi((const char *) mp4_thumb->rand_thumb_msi_name, 320, 240);
        mp4_thumb->output_msi  = mp4_thumb_output_msi((const char *) mp4_thumb->rand_output_msi_name, filename);
        msi_add_output(mp4_thumb->thumb_msi, NULL, mp4_thumb->output_msi, NULL);
        msi->enable = 1;
    }
    else
    {
        if (msi)
        {
            msi_destroy(msi);
            msi = NULL;
        }
    }

    return msi;
}
