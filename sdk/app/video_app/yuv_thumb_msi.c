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

struct yuv_thumb_s
{
    struct msi *msi;
    char     thumb_name[64];
    uint32_t    magic;
    uint8_t     filter_type;
    uint8_t     srcID;
};

static int32_t yuv_thumb_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t             ret       = RET_OK;
    struct yuv_thumb_s *yuv_thumb = (struct yuv_thumb_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            STREAM_LIBC_FREE(yuv_thumb);
        }
        break;

        // 需要关闭,则将关联的msi关闭
        case MSI_CMD_PRE_DESTROY:
        {
        }
        break;

        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            // 如果是yuv,就去生成缩略图
            if (fb->mtype == F_YUV)
            {
                struct takephoto_yuv_arg_s *arg = (struct takephoto_yuv_arg_s *) fb->priv;
                arg->yuv_arg.magic              = yuv_thumb->magic;
                os_memcpy(arg->name, yuv_thumb->thumb_name, strlen(yuv_thumb->thumb_name) + 1);
                arg->yuv_arg.type = YUV_ARG_TAKEPHOTO;
                fb_get(fb);
                int success = msi_output_fb(msi, fb);
                os_printf("success = %d\n", success);
                // 设置文件名
                msi->enable = 0;
            }
            ret = RET_ERR;
        }
        break;
        case MSI_CMD_YUV_THUMB:
        {
            uint32_t cmd_self = (uint32_t) param1;
            uint32_t arg      = param2;
            switch (cmd_self)
            {
                case 0:
                {
                    // 先等待evt
                    // arg是文件名
                    os_memcpy(yuv_thumb->thumb_name, (char *) arg, strlen((char*)arg) + 1);
                    msi->enable = 1;
                    break;
                }

                default:
                    break;
            }
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

struct msi *yuv_thumb_msi_init(const char *output_name,uint32_t magic)
{
    uint8_t     is_new;
    struct msi *msi = msi_new(R_YUV_THUMB, 0, &is_new);

    struct yuv_thumb_s *yuv_thumb = NULL;

    if (is_new)
    {
        yuv_thumb        = (struct yuv_thumb_s *) STREAM_LIBC_ZALLOC(sizeof(struct yuv_thumb_s));
        msi->priv        = yuv_thumb;
        yuv_thumb->msi   = msi;
        yuv_thumb->magic = magic;
        msi->action      = yuv_thumb_msi_action;
        // 给到解码然后生成缩略图
        msi_add_output(msi, NULL, output_name);
        msi->enable = 0;
    }
    return msi;
}
