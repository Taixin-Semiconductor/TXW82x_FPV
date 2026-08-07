#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "stream_define.h"
#include "user_work/user_work.h"

struct msi *h264_msi_init_with_mode(uint32_t drv1_from, uint16_t drv1_w, uint16_t drv1_h, uint16_t drv2_from, uint16_t drv2_w, uint16_t drv2_h);

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

struct auto_h264_msi_s
{
    struct os_work work;
    struct msi    *msi;
    struct msi    *register_h264_msi;
    uint8_t        src_from0;
    uint8_t        src_from1;
    uint8_t        stop;
    uint16_t       w0, w1, h0, h1;
};

static int32 auto_h264_work(struct os_work *work)
{
    uint8_t                 delay_time    = 40;
    struct auto_h264_msi_s *auto_h264     = (struct auto_h264_msi_s *) work;
    struct msi             *auto_h264_msi = auto_h264->msi;
    // 这里尝试发送数据?检查一下是否有接收的msi,如果有接收的msi,则启动mjpeg,如果没有,则将硬件停下来?
    uint32_t                send_count    = msi_output_fb(auto_h264->msi, NULL);
    if (send_count)
    {
        if (auto_h264->stop == 1)
        {
            // 启动h264
            if (auto_h264->register_h264_msi)
            {
                msi_del_output(auto_h264->register_h264_msi, NULL, auto_h264_msi->name);
                msi_destroy(auto_h264->register_h264_msi);
                auto_h264->register_h264_msi = NULL;
            }

            // 尝试重新创建,如果创建失败,下一次再继续创建
            auto_h264->register_h264_msi = h264_msi_init_with_mode(auto_h264->src_from0, auto_h264->w0, auto_h264->h0, auto_h264->src_from1, auto_h264->w1, auto_h264->h1);
            if (auto_h264->register_h264_msi)
            {
                msi_add_output(auto_h264->register_h264_msi, NULL, auto_h264_msi->name);
                auto_h264->stop = 0;
            }
        }
    }
    else if (send_count == 0)
    {
        if (auto_h264->stop == 0)
        {
            // 停止h264的启动
            auto_h264->stop = 1;
            if (auto_h264->register_h264_msi)
            {
                msi_del_output(auto_h264->register_h264_msi, NULL, auto_h264_msi->name);
                msi_destroy(auto_h264->register_h264_msi);
                auto_h264->register_h264_msi = NULL;
            }
        }
    }
    os_run_work_delay(work, delay_time);
    return 0;
}

static int32_t auto_h264_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t                 ret       = RET_OK;
    struct auto_h264_msi_s *auto_h264 = (struct auto_h264_msi_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            os_work_cancle2(&auto_h264->work, 1);
            STREAM_LIBC_FREE(auto_h264);
        }
        break;
        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&auto_h264->work, 1);
            if (auto_h264->register_h264_msi)
            {
                msi_destroy(auto_h264->register_h264_msi);
                auto_h264->register_h264_msi = NULL;
            }
        }
        break;
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            // 如果是mjpeg,帮忙转发到其他数据流
            // 如果需要支持264,也要帮忙转发
            if (fb->mtype == F_H264)
            {
                // 类型匹配才可以转发
                if ((fb->stype == FSTYPE_H264_VPP_DATA0 + auto_h264->src_from0) || (fb->stype == FSTYPE_H264_VPP_DATA0 + auto_h264->src_from1))
                {
                    _os_printf("+");
                    struct framebuff *fb = (struct framebuff *) param1;
                    fb_get(fb);
                    msi_output_fb(msi, fb);
                }
                ret = RET_OK + 1;
                break;
            }
        }
        break;

        default:
            break;
    }
    return ret;
}

//如果不使能,src_from0设置最大值,其他设置为0
//src_from为VPP_DATA0或者VPP_DATA1,是支持自动设置w和h
struct msi *auto_h264_msi_init(const char *auto_h264_name, uint8_t src_from0, uint16_t w0, uint16_t h0, uint8_t src_from1, uint16_t w1, uint16_t h1)
{
    uint8_t                 isnew;
    struct msi             *msi       = msi_new(auto_h264_name, 0, &isnew);
    struct auto_h264_msi_s *auto_h264 = (struct auto_h264_msi_s *) msi->priv;
    if (isnew)
    {
        auto_h264            = (struct auto_h264_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct auto_h264_msi_s));
        msi->priv            = (void *) auto_h264;
        msi->action          = auto_h264_msi_action;
        auto_h264->msi       = msi;
        auto_h264->stop      = 1;
        auto_h264->src_from0 = src_from0;
        auto_h264->src_from1 = src_from1;
        auto_h264->w0        = w0;
        auto_h264->w1        = w1;
        auto_h264->h0        = h0;
        auto_h264->h1        = h1;
        msi->enable          = 1;
        // 启动workqueue
        OS_WORK_INIT(&auto_h264->work, auto_h264_work, 0);
        os_run_work(&auto_h264->work);
    }

    return msi;
}