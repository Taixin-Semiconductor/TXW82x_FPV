#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "stream_define.h"

#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "user_work/user_work.h"
#include "recode/jpg_recode.h"
#include "file_thumb.h"
#include "sdfunc_work/sdfunc_work.h"
#include "video_app/file_common_api.h"

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc
uint8_t gen_normal_jpg_path2(const char *filename, char *path, uint32_t pathsize);

struct thumb_s
{
    struct msi *msi;        // 主要接收缩略图的msi
    struct msi *output_msi; // 接收jpg图片并且过滤的msi
    struct msi *thumb_msi;  // 产生缩略图的msi,输入jpg,输出也是jpg
    char        rand_msi_name[32];
    char        rand_thumb_msi_name[32];
    char        rand_output_msi_name[32];
    char        filename[64];
    uint16_t    takephoto_num;
    uint8_t     filter_type;
    uint8_t     srcID;
};

struct thumb_output_s
{
    struct os_work    work;
    struct msi       *msi;
    struct framebuff *fb;
    char              filename[64];
};

static int32_t get_photo_path(char *save_filename, uint32_t save_filename_size, uint32_t offset_ms, uint8_t isthumb)
{
    struct timeval r_t;
    char           filename[32];
    gettimeofday2(&r_t, offset_ms);
    takephoto_name_no_dir_time(filename, sizeof(filename), &r_t);
    if (isthumb)
    {
        gen_thumb_path(filename, save_filename, save_filename_size);
    }
    else
    {
        gen_normal_jpg_path2(filename, save_filename, save_filename_size);
    }
    return 0;
}

static int32_t thumb_output_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t                ret          = RET_OK;
    struct thumb_output_s *thumb_output = (struct thumb_output_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_PRE_DESTROY:
        {
            break;
        }
        case MSI_CMD_POST_DESTROY:
        {
            STREAM_LIBC_FREE(thumb_output);
            break;
        }
        // 缩略图写卡
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            get_photo_path(thumb_output->filename, sizeof(thumb_output->filename), fb->time, 1);
            sd_fb_write_work(fb, thumb_output->filename, 1);
            ret = RET_ERR;
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
static struct msi *thumb_output_msi(const char *name)
{
    uint8_t     isnew;
    struct msi *msi = msi_new(name, 0, &isnew);
    if (msi && isnew)
    {
        struct thumb_output_s *thumb_output = (struct thumb_output_s *) STREAM_LIBC_MALLOC(sizeof(struct thumb_output_s));
        ASSERT(thumb_output);
        msi->priv         = (void *) thumb_output;
        thumb_output->msi = msi;
        msi->type         = F_JPG << 8 | 0xff;
        msi->action       = thumb_output_msi_action;
        // 需要将缩略图给到文件写入
        msi->enable       = 1;
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

static int32_t thumb_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t         ret   = RET_OK;
    struct thumb_s *thumb = (struct thumb_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            STREAM_LIBC_FREE(thumb);
        }
        break;

        // 需要关闭,则将关联的msi关闭
        case MSI_CMD_PRE_DESTROY:
        {
            // 将对应的msi关闭
            if (thumb->thumb_msi)
            {
                msi_destroy(thumb->thumb_msi);
                thumb->thumb_msi = NULL;
            }

            if (thumb->output_msi)
            {
                msi_destroy(thumb->output_msi);
                thumb->output_msi = NULL;
            }
        }

        break;

        case MSI_CMD_START:
        {
            if (param1 > 0)
            {
                thumb->takephoto_num = param1;
                msi->enable          = 1;
            }
        }
        break;

        // 接收到数据,唤醒workqueue,去组图,理论有图都要重新组,然后通过output的时候过滤类型,这里也可以先过滤部分类型
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            if (fb->mtype == F_JPG && thumb->srcID == fb->srcID && (!thumb->filter_type || thumb->filter_type == fb->stype))
            {
                if (thumb->takephoto_num)
                {
                    // 生成缩略图
                    msi_recv_fb(thumb->thumb_msi, fb);

                    // 原图写卡
                    get_photo_path(thumb->filename, sizeof(thumb->filename), fb->time, 0);
                    sd_fb_write_work(fb, thumb->filename, 0);
                    thumb->takephoto_num--;
                    if (!thumb->takephoto_num)
                    {
                        msi->enable = 0;
                    }
                }
            }
            ret = RET_ERR;
        }
        break;

        default:
            break;
    }
    return ret;
}

struct msi *new_takephoto_msi(uint8_t srcID, uint8_t filter)
{
    // 一个随机名称?然后其他msi输入
    uint8_t         is_new = 0;
    struct thumb_s *thumb  = (struct thumb_s *) STREAM_LIBC_ZALLOC(sizeof(struct thumb_s));
    os_sprintf(thumb->rand_msi_name, "mp4_thumb_[%05d]_%p", (uint32_t) os_jiffies() % 99999, thumb);
    os_sprintf(thumb->rand_thumb_msi_name, "jpg_to_jpg_[%05d]_%p", (uint32_t) os_jiffies() % 99999, thumb);
    os_sprintf(thumb->rand_output_msi_name, "mp4_thumb_output_[%05d]_%p", (uint32_t) os_jiffies() % 99999, thumb);
    // 由于名称是随机,只要内存够,不应该失败
    // 由于是转发到thumb内部去实现jpg产生缩略图,所以这里不需要队列
    struct msi *msi = msi_new(thumb->rand_msi_name, 0, &is_new);
    if (msi && is_new)
    {
        msi->priv          = thumb;
        thumb->msi         = msi;
        thumb->filter_type = filter;
        thumb->srcID       = srcID;
        msi->action        = thumb_msi_action;
        thumb->thumb_msi   = new_jpg_recode_msi((const char *) thumb->rand_thumb_msi_name, 320, 240);
        thumb->output_msi  = thumb_output_msi((const char *) thumb->rand_output_msi_name);
        msi_add_output(thumb->thumb_msi, NULL, thumb->output_msi, NULL);
        msi->enable = 0;
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
