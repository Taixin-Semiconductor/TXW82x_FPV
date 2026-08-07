#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "stream_frame.h"

#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/video/dvp/jpeg/jpg_common.h"
#include "gen420_hardware_msi.h"
#include "video_msi.h"

struct msi *jpg_decode_msg_msi(const char *name, uint16_t out_w, uint16_t out_h, uint16_t step_w, uint16_t step_h, uint32_t filter);
struct msi *thumb_over_dpi_msi_init(const char *msi_name, uint8_t jpg_type, uint8_t stype, uint32_t magic);
struct msi *jpg_decode_msi(const char *name);

// 过滤类型,因为缩略图的stype一定大于FSYPTE_INVALID
static uint8_t filter(void *f, uint8_t recv_type)
{
    struct framebuff *fb  = (struct framebuff *) f;
    uint8_t           res = 1;

    if (fb->mtype == F_JPG && fb->stype == recv_type)
    {
        res = 0;
    }

    return res;
}

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

struct video_thumb_s
{
    struct msi *msi;
    uint8_t     thumb_name[64];
    uint16_t    filter_type;
};

int32_t video_thumb_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;

    struct video_thumb_s *video_thumb = (struct video_thumb_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            STREAM_LIBC_FREE(video_thumb);
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

            if (video_thumb->filter_type)
            {
                if (video_thumb->filter_type != fb->stype)
                {
                    ret = RET_ERR;
                }
            }
            if (ret == RET_OK)
            {
                // fb->stype < FSYPTE_INVALID,防止生成的缩略图误认为是图片,video_thumb->filter_type最好
                if (fb->mtype == F_JPG && fb->stype < FSYPTE_INVALID)
                {
                    // 转发然后生成缩略图,然后关闭使能
                    struct framebuff *c_fb = fb_clone(fb, F_THUMB_JPG << 8 | fb->stype, msi);
                    if (c_fb)
                    {
                        c_fb->priv = video_thumb->thumb_name;
                        msi_output_fb(msi, c_fb);
                        msi->enable = 0;
                    }
                }
            }
            // 这个主要为了生成缩略图,所以这里不需要接收数据
            ret = RET_ERR;
        }
        break;
        case MSI_CMD_FREE_FB:
        {
        }
        break;
        // 这里是demo,复用MSI_CMD_JPG_THUMB
        case MSI_CMD_JPG_THUMB:
        {
            uint32_t cmd_self = (uint32_t) param1;
            switch (cmd_self)
            {
                // 修改保存文件的名称
                case MSI_JPG_THUMB_TAKEPHOTO:
                {
                    char *filename = (char *) param2;
                    video_thumb    = (struct video_thumb_s *) msi->priv;
                    os_memcpy(video_thumb->thumb_name, filename, strlen(filename) + 1);
                    // os_printf(KERN_INFO"save thumb name:%s\n", video_thumb->thumb_name);
                    msi->enable = 1;
                }
                break;
            }
        }

        break;
        default:
            break;
    }
    return ret;
}

struct msi *video_thumb_msi_init(const char *msi_name, uint16_t filter)
{
    uint8_t     is_new;
    struct msi *msi = msi_new(msi_name, 0, &is_new);

    struct video_thumb_s *video_thumb = NULL;

    if (is_new)
    {
        video_thumb              = (struct video_thumb_s *) STREAM_LIBC_ZALLOC(sizeof(struct video_thumb_s));
        msi->priv                = video_thumb;
        video_thumb->msi         = msi;
        video_thumb->filter_type = filter;
        msi->action              = video_thumb_msi_action;
    }
    return msi;
}

/**********************************************************************************************************************************************************
 * thumb_msi_name(如果是原图就去解码,如果是缩略图就保存)->R_THUMB_DECODE_MSG->R_THUMB_DECODE_MSG->R_GEN420_THUMB_JPG
 *     ^                                                                                                |
 *     |                                                                                                |
 *     |                                                                                                |
 *     |                                                                                                V
 *     --------------------------------------------------------------------------------------------------
 *****************************************************************************************************************************/
// 这里是demo,msi的名字与sdk的名字一致,所以这里仅仅做参考
void video_thumb_init(const char *thumb_msi_name)
{
    uint32_t magic = 0;

    // 启动解码模块,名称已经固定,参数已经无效
    struct msi *decode_msi = jpg_decode_msi(S_JPG_DECODE);
    if (decode_msi)
    {
        // 给到gen420去重新生成缩略图jpg
        msi_add_output(decode_msi, NULL, R_GEN420_THUMB_JPG);
    }

    // 生成缩略图的size
    struct msi *decode_msg_msi = jpg_decode_msg_msi(R_THUMB_DECODE_MSG, 320, 180, 320, 180, 0);
    // 生成一个随机magic
    do
    {
        magic ^= os_jiffies();
        magic ^= (uint32_t) decode_msg_msi;
    } while (!magic);

    struct msi *thumb_msi = thumb_over_dpi_msi_init(thumb_msi_name, 0,FSTYPE_NORMAL_THUMB_JPG, magic);
    if (thumb_msi)
    {
        // 处理原图,然后给到解码后生成缩略图
        msi_add_output(thumb_msi, NULL, R_THUMB_DECODE_MSG);

        msi_add_output(thumb_msi, NULL, R_FILE_MSI);
    }

    if (decode_msg_msi && decode_msi)
    {
        msi_do_cmd(decode_msg_msi, MSI_CMD_DECODE_JPEG_MSG, MSI_JPEG_DECODE_MAGIC, magic);
        msi_add_output(decode_msg_msi, NULL, decode_msi->name);
    }

    // 启动一个专门用yuv->gen420->mjpg的模块,这个模块会接收mjpg图片,并且通过filter函数决定是否转发
    struct msi *gen420_jpg_msi = gen420_jpg_msi_init(R_GEN420_THUMB_JPG, JPGID0,FSTYPE_NORMAL_THUMB_JPG, JPG_LOCK_GEN420_THUBM_ENCODE, GEN420_QUEUE_THUMB_JPEG, NULL, filter);
    if (gen420_jpg_msi && thumb_msi)
    {
        msi_do_cmd(gen420_jpg_msi, MSI_CMD_JPG_RECODE, MSI_JPG_RECODE_MAGIC, magic);
        msi_add_output(gen420_jpg_msi, NULL, thumb_msi->name);
    }
}