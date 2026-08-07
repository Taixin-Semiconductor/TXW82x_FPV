
#include "scale_msi.h"
#include "dev/vpp/hgvpp.h"
#include "lib/video/vpp/vpp_dev.h"
#include "dev/scale/hgscale.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "user_work/user_work.h"
#include "scale3_normal_msi.h"

extern int32_t takephoto_name_no_dir2(char *filename, int filename_size,struct timeval *t);
#define EXTERN_RB_COUNT 50

uint32_t yuv_buf_line(uint8_t which);

#define MAX_COUNT 3

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

struct scale3_normal_msi
{
    struct os_work              work;
    struct msi                 *msi;
    struct scale_device        *scale_dev;
    struct fbpool               pool;
    struct os_msgqueue          msgq;
    uint8_t                    *buf;
    uint16_t                    iw, ih;
    uint16_t                    ow, oh;
    struct framebuff           *fb;
    struct framebuff           *extern_fb;
    struct scale3_normal_cmd_s *normal_cmd;
    uint32_t                    last_time;
    uint8_t                     force_stype;
    uint8_t                     ready : 1, extern_fb_ready : 1, start : 1,exit:1;
    RBUFFER_DEF(extern_rb, struct scale3_normal_cmd_s *, EXTERN_RB_COUNT);
};

void   *get_vpp_buf(uint8_t which);
uint8_t get_vpp_w_h(uint16_t *w, uint16_t *h);

static int32_t const_scale3_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t                   ret    = RET_OK;
    struct scale3_normal_msi *scale3 = (struct scale3_normal_msi *) msi->priv;
    switch (cmd_id)
    {
        // 这里msi已经被删除,那么就要考虑tx_pool的资源释放了
        // 能进来这里,就是代表所有fb都已经用完了
        case MSI_CMD_POST_DESTROY:
        {
            struct framebuff *fb;
            // 释放资源fb资源文件
            while (1)
            {
                fb = fbpool_get(&scale3->pool, 0, NULL);
                if (!fb)
                {
                    break;
                }
                // 预分配空间释放
                if (fb->priv)
                {
                    STREAM_LIBC_FREE(fb->priv);
                }

                if (fb->data)
                {
                    STREAM_FREE(fb->data);
                }
            }
            os_msgq_del(&scale3->msgq);
            
            struct scale3_normal_cmd_s *normal_cmd;
            if(scale3->normal_cmd)
            {
                STREAM_FREE(scale3->normal_cmd);
                scale3->normal_cmd = NULL;
            }
            while(RB_GET(&scale3->extern_rb, normal_cmd))
            {
                STREAM_FREE(normal_cmd);
            }
            STREAM_FREE(scale3);
        }
        break;

        // 停止硬件,移除没有必要的资源(但是fb的资源不能现在删除,这个时候fb可能外部还在调用)
        case MSI_CMD_PRE_DESTROY:
        {
            scale_close(scale3->scale_dev);
            // 关闭work
            os_work_cancle2(&scale3->work, 1);

            scale3->start = 0;
            scale3->exit = 1;

            if (scale3->extern_fb)
            {
                msi_delete_fb(scale3->msi, scale3->fb);
                scale3->fb = NULL;
            }

            if (scale3->fb)
            {
                msi_delete_fb(scale3->msi, scale3->fb);
                scale3->fb = NULL;
            }
            struct framebuff *fb = NULL;
            while (1)
            {
                fb = (struct framebuff *) os_msgq_get2(&scale3->msgq, 0, NULL);
                if (fb)
                {
                    msi_delete_fb(NULL, fb);
                    fb = NULL;
                }
                else
                {
                    break;
                }
            }
        }
        break;
        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            // sys_dcache_invalid_range(fb->data, scale3->ow * scale3->oh * 3 / 2);
            if (fb->datatag != 0xff)
            {
                fbpool_put(&scale3->pool, fb);
                ret = RET_OK + 1;
            }
            else
            {
                STREAM_FREE(fb->data);
                STREAM_LIBC_FREE(fb->priv);
                scale3->extern_fb_ready = 1;
            }
            if(!scale3->exit)
            {
                os_run_work(&scale3->work);
            }
        }
        break;

        // 私有命令,特定结构体{w,h,time,stype}
        case MSI_CMD_SCALE3_NORMAL:
        {
            uint32_t cmd_self = (uint32_t) param1;
            // uint32_t arg      = param2;
            switch (cmd_self)
            {
                case MSI_SCALE3_START:
                {
                    scale3->start = param2 & 0x01;
                    os_run_work(&scale3->work);
                    break;
                }
                break;
                case MSI_SCLAE3_NORMAL_ADD_DPI:
                {
                    // 申请一个arg结构体给到ringbuf,等待workqueue去生成对应的fb
                    struct scale3_normal_cmd_s *cmd = STREAM_MALLOC(sizeof(struct scale3_normal_cmd_s));
                    if (cmd)
                    {
                        memcpy(cmd, (void *) param2, sizeof(struct scale3_normal_cmd_s));
                        RB_SET(&scale3->extern_rb, cmd);
                        os_run_work(&scale3->work);
                    }

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

static int32_t scale3_stream_done(uint32 irq_flag, uint32 irq_data, uint32 param1)
{
    struct scale3_normal_msi   *scale3 = (struct scale3_normal_msi *) irq_data;
    struct framebuff           *fb;
    struct takephoto_yuv_arg_s *arg;
    uint8_t                    *p_buf;
    uint32_t                    ow = 0, oh = 0;
    // 如果是需要额外抽一帧生成特定size的yuv
    if (scale3->extern_fb)
    {
        arg               = scale3->extern_fb->priv;
        ow                = arg->yuv_arg.out_w;
        oh                = arg->yuv_arg.out_h;
        p_buf             = (uint8_t *) scale3->extern_fb->data;
        fb                = scale3->extern_fb;
        scale3->extern_fb = NULL;
    }
    else
    {
        if (scale3->start)
        {
            ow = scale3->ow;
            oh = scale3->oh;
            fb = fbpool_get(&scale3->pool, 0, scale3->msi);
        }
        else
        {
            fb = NULL;
        }

        // 空间不够,则使用原来的空间
        if (!fb)
        {
            scale_close(scale3->scale_dev);
            goto scale3_stream_done_end;
        }

        // 配置新的空间地址
        p_buf = (uint8_t *) fb->data;
    }

    scale_set_in_out_size(scale3->scale_dev, scale3->iw, scale3->ih, ow, oh);
    scale_set_step(scale3->scale_dev, scale3->iw, scale3->ih, ow, oh);
    scale_set_out_yaddr(scale3->scale_dev, (uint32) p_buf);
    scale_set_out_uaddr(scale3->scale_dev, (uint32) p_buf + ow * oh);
    scale_set_out_vaddr(scale3->scale_dev, (uint32) p_buf + ow * oh + ow * oh / 4);
scale3_stream_done_end:
    // 发送now_data,发送失败也要返回
    if (os_msgq_put(&scale3->msgq, (uint32_t) scale3->fb, 0))
    {
        // 正常不能中断del,但是这个模块是内部,只要del没有一些等待信号量操作,问题不大
        msi_delete_fb(NULL, scale3->fb);
        scale3->fb = NULL;
        // return 0;
    }
    if (!fb)
    {
        scale3->ready = 1;
    }
    scale3->fb = fb;
    os_run_work(&scale3->work);
    return 0;
}

static int32_t scale3_stream_ov(uint32 irq_flag, uint32 irq_data, uint32 param1)
{
    os_printf("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;
}

// 如果空间申请不到,就延时去输出
static int32 scale3_normal_msi_work(struct os_work *work)
{
    struct scale3_normal_msi   *scale3     = (struct scale3_normal_msi *) work;
    struct scale_device        *scale_dev  = scale3->scale_dev;
    uint8_t                     delay_time = 0;
    struct framebuff           *fb         = NULL;
    struct framebuff           *e_fb       = NULL;
    struct takephoto_yuv_arg_s *arg        = NULL;
    uint16_t                    ow, oh;
    uint32_t                    magic = 0;
    int32_t                     err   = -1;
    // 先去检查是否有需要生成额外的yuv数据没
    // 检查如果没有extern_fb,并且有额外命令,则生成一个fb
    if (scale3->extern_fb_ready && !scale3->extern_fb)
    {
        if (!scale3->normal_cmd)
        {
            RB_GET(&scale3->extern_rb, scale3->normal_cmd);
        }

        if (scale3->normal_cmd)
        {
            if (scale3->normal_cmd->w && scale3->normal_cmd->h)
            {
                ow = scale3->normal_cmd->w;
                oh = scale3->normal_cmd->h;
            }
            else
            {
                ow = scale3->iw;
                oh = scale3->ih;
            }

            uint8_t *data = STREAM_MALLOC(ow * oh * 3 / 2);
            if (data)
            {

                arg  = (struct takephoto_yuv_arg_s *) STREAM_LIBC_ZALLOC(sizeof(struct takephoto_yuv_arg_s));
                e_fb = fb_alloc(data, ow * oh * 3 / 2, F_YUV << 8 | FSTYPE_NONE, scale3->msi);
                // 如果w和h其中一个为0,则使用iw和ih

                e_fb->datatag       = ~0; // 设置特殊标志,用于识别是否是额外生成的fb
                e_fb->priv          = arg;
                arg->yuv_arg.y_size = ow * oh;
                arg->yuv_arg.y_off  = 0;
                arg->yuv_arg.uv_off = 0;
                arg->yuv_arg.out_w  = ow;
                arg->yuv_arg.out_h  = oh;
                arg->yuv_arg.magic  = scale3->normal_cmd->magic;
                arg->yuv_arg.type   = scale3->normal_cmd->force_type;

                takephoto_name_no_dir2(arg->name, sizeof(arg->name), &scale3->normal_cmd->t);
                scale3->extern_fb_ready = 0;
                scale3->extern_fb       = e_fb;

                sys_dcache_invalid_range((uint32_t *) data, ow * oh * 3 / 2);
                STREAM_FREE(scale3->normal_cmd);

                scale3->normal_cmd = NULL;
            }
        }
    }

    fb = (struct framebuff *) os_msgq_get2(&scale3->msgq, 0, &err);
    if (fb)
    {
        if (fb->datatag != (uint8_t) ~0)
        {
            _os_printf(KERN_INFO "S");
            fb->mtype           = F_YUV;
            fb->stype           = scale3->force_stype;
            arg                 = fb->priv;
            ow                  = scale3->ow;
            oh                  = scale3->oh;
            arg->yuv_arg.y_size = ow * oh;
            arg->yuv_arg.y_off  = 0;
            arg->yuv_arg.uv_off = 0;
            arg->yuv_arg.out_w  = ow;
            arg->yuv_arg.out_h  = oh;
            arg->yuv_arg.magic  = magic;
            arg->yuv_arg.type   = YUV_ARG_NONE;
            msi_output_fb(scale3->msi, fb);
        }
        else
        {
            msi_output_fb(scale3->msi, fb);
        }

        fb = NULL;
    }

    // scale3可能空间不够关闭了中断,也可能是第一次启动
    if (scale3->ready)
    {

        if (scale3->extern_fb)
        {
            arg               = scale3->extern_fb->priv;
            ow                = arg->yuv_arg.out_w;
            oh                = arg->yuv_arg.out_h;
            scale3->fb        = scale3->extern_fb;
            scale3->extern_fb = NULL;
            fb                = NULL;
        }
        else
        {
            if (scale3->start)
            {
                fb = fbpool_get(&scale3->pool, F_YUV << 8 | FSTYPE_YUV_P0, scale3->msi);
                if (fb)
                {
                    ow         = scale3->ow;
                    oh         = scale3->oh;
                    scale3->fb = fb;
                    fb         = NULL;
                }
            }
        }

        if (!scale3->fb)
        {
            _os_printf(KERN_INFO "D");
            delay_time = 0;
            goto scale3_normal_msi_work_end;
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

        scale_set_out_yaddr(scale_dev, (uint32) scale3->fb->data);
        scale_set_out_uaddr(scale_dev, (uint32) scale3->fb->data + ow * oh);
        scale_set_out_vaddr(scale_dev, (uint32) scale3->fb->data + ow * oh + ow * oh / 4);
        scale_request_irq(scale_dev, FRAME_END, scale3_stream_done, (uint32) scale3);
        scale_request_irq(scale_dev, INBUF_OV, scale3_stream_ov, (uint32) scale3);
        scale3->ready = 0;
        scale_open(scale_dev);
    }

scale3_normal_msi_work_end:
    if (delay_time)
    {
        os_run_work_delay(work, delay_time);
    }

    if (fb)
    {
        msi_delete_fb(NULL, fb);
        fb = NULL;
    }
    return 0;
}

struct msi *scale3_normal_msi(const char *name, uint16_t ow, uint16_t oh)
{
    uint8_t     isnew;
    struct msi *msi = msi_new(name, 0, &isnew);
    if (isnew)
    {
        struct scale_device      *scale_dev = (struct scale_device *) dev_get(HG_SCALE3_DEVID);
        struct scale3_normal_msi *scale3    = (struct scale3_normal_msi *) STREAM_LIBC_ZALLOC(sizeof(struct scale3_normal_msi));
        scale3->scale_dev                   = scale_dev;
        scale3->ready                       = 1;
        scale3->ow                          = ow;
        scale3->oh                          = oh;
        scale3->msi                         = msi;
        scale3->force_stype                 = FSTYPE_NONE;
        scale3->extern_fb_ready             = 1;
        scale3->start                       = 1;
        msi->priv                           = (void *) scale3;
        msi->action                         = const_scale3_msi_action;
        msi->enable                         = 1;
        os_msgq_init(&scale3->msgq, MAX_COUNT);
        fbpool_init(&scale3->pool, MAX_COUNT);

        uint16_t                    init_count = 0;
        uint8_t                    *m_buff;
        struct takephoto_yuv_arg_s *arg;
        // 初始化framebuffer节点数量空间?最后由workqueue去从ringbuf去获取一个节点
        while (init_count < MAX_COUNT)
        {
            m_buff = STREAM_MALLOC(ow * oh * 3 / 2);
            arg    = (struct takephoto_yuv_arg_s *) STREAM_LIBC_ZALLOC(sizeof(struct takephoto_yuv_arg_s));
            ASSERT(m_buff);
            ASSERT(arg);
            sys_dcache_invalid_range((uint32_t *) m_buff, ow * oh * 3 / 2);
            FBPOOL_SET_INFO(&scale3->pool, init_count, m_buff, ow * oh * 3 / 2, arg);
            init_count++;
        }
        get_vpp_w_h(&scale3->iw, &scale3->ih);
        OS_WORK_INIT(&scale3->work, scale3_normal_msi_work, 0);
        os_run_work(&scale3->work);
    }
    return msi;
}

struct msi *scale3_normal_msi2(const char *name, uint8_t force_stype, uint16_t ow, uint16_t oh)
{
    uint8_t     isnew;
    struct msi *msi = msi_new(name, 0, &isnew);
    if (isnew)
    {
        struct scale_device      *scale_dev = (struct scale_device *) dev_get(HG_SCALE3_DEVID);
        struct scale3_normal_msi *scale3    = (struct scale3_normal_msi *) STREAM_ZALLOC(sizeof(struct scale3_normal_msi));
        scale3->scale_dev                   = scale_dev;
        scale3->ready                       = 1;
        scale3->ow                          = ow;
        scale3->oh                          = oh;
        scale3->msi                         = msi;
        scale3->force_stype                 = force_stype;
        scale3->extern_fb_ready             = 1;
        scale3->start                       = 0;
        msi->priv                           = (void *) scale3;
        msi->action                         = const_scale3_msi_action;
        msi->enable                         = 1;
        os_msgq_init(&scale3->msgq, MAX_COUNT);
        fbpool_init(&scale3->pool, MAX_COUNT);
        RB_INIT(&scale3->extern_rb, EXTERN_RB_COUNT);

        uint16_t                    init_count = 0;
        uint8_t                    *m_buff;
        struct takephoto_yuv_arg_s *arg;
        // 初始化framebuffer节点数量空间?最后由workqueue去从ringbuf去获取一个节点
        while (init_count < MAX_COUNT)
        {
            m_buff = STREAM_MALLOC(ow * oh * 3 / 2);
            arg    = (struct takephoto_yuv_arg_s *) STREAM_LIBC_ZALLOC(sizeof(struct takephoto_yuv_arg_s));
            ASSERT(m_buff);
            ASSERT(arg);
            sys_dcache_invalid_range((uint32_t *) m_buff, ow * oh * 3 / 2);
            FBPOOL_SET_INFO(&scale3->pool, init_count, m_buff, ow * oh * 3 / 2, arg);
            init_count++;
        }
        get_vpp_w_h(&scale3->iw, &scale3->ih);
        OS_WORK_INIT(&scale3->work, scale3_normal_msi_work, 0);
        os_run_work(&scale3->work);
    }
    return msi;
}