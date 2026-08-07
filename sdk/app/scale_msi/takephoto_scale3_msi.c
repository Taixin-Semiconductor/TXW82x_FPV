
#include "scale_msi.h"
#include "dev/vpp/hgvpp.h"
#include "lib/video/vpp/vpp_dev.h"
#include "dev/scale/hgscale.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "user_work/user_work.h"
#include "video_app/file_common_api.h"

uint32_t yuv_buf_line(uint8_t which);

/*********************************************************************
 * 这个模块是分别输出两种图片的yuv,一种是原图,一种是缩略图
 ********************************************************************/
// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

uint8_t *test_buf = NULL;

#define THUMB_W 320
#define THUMB_H 180

struct takephoto_scale3_msi_s
{
    struct os_work       work;
    struct msi          *msi;
    struct scale_device *scale_dev;
    uint8_t             *buf;
    uint32_t             kick_times;
    uint16_t             iw, ih;
    struct framebuff    *fb;
    uint32_t             thumb_magic;
    uint32_t             normal_magic;
    char              filename[64];
    uint8_t              ready;
};

void   *get_vpp_buf(uint8_t which);
uint8_t get_vpp_w_h(uint16_t *w, uint16_t *h);

static int32_t const_scale3_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t                        ret    = RET_OK;
    struct takephoto_scale3_msi_s *scale3 = (struct takephoto_scale3_msi_s *) msi->priv;
    switch (cmd_id)
    {

        // 这里msi已经被删除,那么就要考虑tx_pool的资源释放了
        // 能进来这里,就是代表所有fb都已经用完了
        case MSI_CMD_POST_DESTROY:
        {
            STREAM_LIBC_FREE(scale3);
        }
        break;

        // 停止硬件,移除没有必要的资源(但是fb的资源不能现在删除,这个时候fb可能外部还在调用)
        case MSI_CMD_PRE_DESTROY:
        {
        }
        break;
        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            if (fb->data)
            {
                STREAM_FREE(fb->data);
            }
            if (fb->priv)
            {
                STREAM_LIBC_FREE(fb->priv);
            }
        }
        break;

        case MSI_CMD_TAKEPHOTO_SCALE3:
        {
            uint32_t cmd_self = (uint32_t) param1;
            uint32_t arg      = param2;
            switch (cmd_self)
            {
                case MSI_TAKEPHOTO_SCALE3_KICK:
                {
                    scale3->kick_times += (arg * 2);
                    os_run_work(&scale3->work);
                    break;
                }
                case MSI_TAKEPHOTO_SCALE3_STOP:
                {
                    os_work_cancle2(&scale3->work, 1);
                    scale3->kick_times = 0;
                    os_run_work(&scale3->work);
                    break;
                }

                default:
                {
                    break;
                }
            }
        }
        break;

        default:
            break;
    }
    return ret;
}

static int32_t takephoto_scale3_stream_done(uint32 irq_flag, uint32 irq_data, uint32 param1)
{
    struct takephoto_scale3_msi_s *scale3 = (struct takephoto_scale3_msi_s *) irq_data;
    _os_printf("CO");
    scale_close(scale3->scale_dev);
    os_run_work(&scale3->work);
    scale3->ready = 1;
    return 0;
}

static int32_t scale3_stream_ov(uint32 irq_flag, uint32 irq_data, uint32 param1)
{
    os_printf("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;
}

// 如果空间申请不到,就延时去输出
static int32 takephoto_scale3_msi_work(struct os_work *work)
{
    struct takephoto_scale3_msi_s *scale3     = (struct takephoto_scale3_msi_s *) work;
    struct scale_device           *scale_dev  = scale3->scale_dev;
    uint8_t                        delay_time = 0;
    uint8_t                       *buf        = NULL;
    struct framebuff              *fb         = NULL;
    struct takephoto_yuv_arg_s    *arg        = NULL;
    uint32_t                       yuvsize;
    uint16_t                       ow, oh;
    uint32_t                       magic;
    if (scale3->ready)
    {
        if (scale3->fb)
        {
            // 还需要配置一些默认私有结构体参数
            msi_output_fb(scale3->msi, scale3->fb);
            scale3->fb  = NULL;
        }
        if (scale3->kick_times)
        {
            // os_printf("scale3->kick_times:%d\n", scale3->kick_times);
            //  原图?
            if (scale3->kick_times % 2 == 0)
            {
                ow    = scale3->iw;
                oh    = scale3->ih;
                magic = scale3->normal_magic;
                takephoto_name_no_dir(scale3->filename,sizeof(scale3->filename));
            }
            // 缩略图
            else
            {
                ow    = THUMB_W;
                oh    = THUMB_H;
                magic = scale3->thumb_magic;
            }
            yuvsize = ow * oh * 3 / 2;
            buf     = STREAM_MALLOC(yuvsize);
            if (buf)
            {
                arg = STREAM_LIBC_ZALLOC(sizeof(struct takephoto_yuv_arg_s));
                if (!arg)
                {
                    delay_time = 5;
                    goto takephoto_scale3_msi_work_end;
                }
                fb = fb_alloc(buf, yuvsize, F_YUV << 8 | FSTYPE_YUV_TAKEPHOTO, scale3->msi);

                if (!fb)
                {
                    delay_time = 5;
                    goto takephoto_scale3_msi_work_end;
                }
                fb->priv   = (void *) arg;
                scale3->fb = fb;

                arg->yuv_arg.y_size = ow * oh;
                arg->yuv_arg.y_off  = 0;
                arg->yuv_arg.uv_off = 0;
                arg->yuv_arg.out_w  = ow;
                arg->yuv_arg.out_h  = oh;
                arg->yuv_arg.magic  = magic;
                arg->yuv_arg.type   = YUV_ARG_TAKEPHOTO;
                os_memcpy(arg->name,scale3->filename,os_strlen(scale3->filename)+1);
                os_printf("scale3->filename:%s\tthumb_magic:%X\n",scale3->filename,magic);
                arg                 = NULL;
                buf                 = NULL;
            }
            else
            {
                delay_time = 5;
                goto takephoto_scale3_msi_work_end;
            }
            // 暂时用同样的iw ih ow oh
            scale_set_in_out_size(scale_dev, scale3->iw, scale3->ih, ow, oh);
            scale_set_step(scale_dev, scale3->iw, scale3->ih, ow, oh);
            scale_set_start_addr(scale_dev, 0, 0);
            // 暂时固定,如果遇到需要动态修改的,可以通过参数之类来切换
            scale_set_dma_to_memory(scale_dev, 1);
            scale_set_data_from_vpp(scale_dev, 1);
            scale_set_line_buf_num(scale_dev, yuv_buf_line(0));
            scale_set_in_yaddr(scale_dev, (uint32) get_vpp_buf(0));
            scale_set_in_uaddr(scale_dev, (uint32) get_vpp_buf(0) + scale3->iw * yuv_buf_line(0));
            scale_set_in_vaddr(scale_dev, (uint32) get_vpp_buf(0) + scale3->iw * yuv_buf_line(0) + scale3->iw * yuv_buf_line(0) / 4);

            scale_set_out_yaddr(scale_dev, (uint32) fb->data);
            scale_set_out_uaddr(scale_dev, (uint32) fb->data + ow * oh);
            scale_set_out_vaddr(scale_dev, (uint32) fb->data + ow * oh + ow * oh / 4);
            scale_request_irq(scale_dev, FRAME_END, takephoto_scale3_stream_done, (uint32) scale3);
            scale_request_irq(scale_dev, INBUF_OV, scale3_stream_ov, (uint32) scale3);
            scale3->ready = 0;
            scale_open(scale_dev);

            scale3->kick_times--;
        }
    }

takephoto_scale3_msi_work_end:
    if (delay_time)
    {
        os_run_work_delay(work, delay_time);
    }

    if (buf)
    {
        STREAM_FREE(buf);
        buf = NULL;
    }

    if (arg)
    {
        STREAM_LIBC_FREE(arg);
        arg = NULL;
    }
    return 0;
}

struct msi *scale3_msi_pic_thumb(const char *name, uint32_t normal_magic, uint32_t thumb_magic)
{
    uint8_t     isnew;
    struct msi *msi = msi_new(name, 0, &isnew);
    if (isnew)
    {
        struct scale_device *scale_dev = (struct scale_device *) dev_get(HG_SCALE3_DEVID);
        os_printf("%s:%d new success\n", __FUNCTION__, __LINE__);
        struct takephoto_scale3_msi_s *scale3 = (struct takephoto_scale3_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct takephoto_scale3_msi_s));
        scale3->scale_dev                     = scale_dev;
        scale3->ready                         = 1;
        scale3->msi                           = msi;
        msi->priv                             = (void *) scale3;
        msi->action                           = const_scale3_msi_action;
        scale3->kick_times                    = 0;
        scale3->normal_magic                  = normal_magic;
        scale3->thumb_magic                   = thumb_magic;
        msi->enable                           = 1;
        get_vpp_w_h(&scale3->iw, &scale3->ih);
        OS_WORK_INIT(&scale3->work, takephoto_scale3_msi_work, 0);
        // os_run_work(&scale3->work);
    }
    os_printf("%s:%d\tmsi:%X\n", __FUNCTION__, __LINE__, msi);
    return msi;
}
