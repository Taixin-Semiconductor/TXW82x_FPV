
#include "scale_msi.h"
#include "dev/vpp/hgvpp.h"
#include "lib/video/vpp/vpp_dev.h"
#include "dev/scale/hgscale.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "user_work/user_work.h"
#include "scale3_normal_msi.h"
#include "multimedia/video.h"
#include "hal/isp.h"
#include "decode/decode_mem.h"

#define EXTERN_RB_COUNT 50
#define MAX_TTL         1000

uint32_t yuv_buf_line(uint8_t which);

#define MAX_COUNT 5

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
    // 申请一个内存池块,用于保存scale3的数据,支持超时释放,
    struct mem_info           **mem_info;
    uint32_t                    mem_info_size;
    struct fbpool               pool;
    struct os_msgqueue          msgq;
    uint8_t                    *buf;
    uint16_t                    iw, ih;
    uint16_t                    ow, oh;
    uint16_t                    x, y;
    struct framebuff           *fb;
    struct framebuff           *extern_fb;
    struct scale3_normal_cmd_s *normal_cmd;
    uint32_t                    last_time;
    uint8_t                     force_stype;
    uint8_t                     ready : 1, extern_fb_ready : 1, start : 1, exit : 1, splice_en : 1, splice_kick : 1, is_thumb : 1;
    RBUFFER_DEF(extern_rb, struct scale3_normal_cmd_s *, EXTERN_RB_COUNT);
};

void   *get_vpp_buf(uint8_t which);
uint8_t get_vpp_w_h(uint16_t *w, uint16_t *h);

static int32_t vpp_start_scale3(uint32_t irq_data)
{
    struct scale3_normal_msi *scale3 = (struct scale3_normal_msi *) irq_data;
    // 如果需要拼接,就要等待镜头2完成才能启动
    if (video_msg.video_type_cur == ISP_VIDEO_1 || video_msg.camera_mode != CAM_DUAL_SPLICE_SLAVE_MODE)
    {
        scale_open(scale3->scale_dev);
        return 1;
    }
    else
    {
        return 0;
    }
}

static uint8_t *scale3_fb_buf_malloc(struct scale3_normal_msi *scale3, uint8_t malloc_flag, uint16_t ow, uint16_t oh, uint16_t x, uint16_t y)
{
    uint8_t *p_buf;
    uint32_t buf_size  = ow * oh * 3 / 2;
    uint32_t need_size = buf_size + sizeof(txYuvInfo_t);
    if (malloc_flag)
    {
        p_buf = (uint8_t *) decode_mem_malloc(scale3->mem_info, scale3->mem_info_size, need_size, STREAM_MALLOC);
    }
    else
    {
        p_buf = (uint8_t *) decode_mem_malloc(scale3->mem_info, scale3->mem_info_size, need_size, NULL);
    }

    if (p_buf)
    {
        txYuvInfo_t *yuvinfo = (txYuvInfo_t *) (p_buf + buf_size);
        yuvinfo->width       = ow;
        yuvinfo->height      = oh;
        yuvinfo->x           = x;
        yuvinfo->y           = y;
        yuvinfo->y_off       = p_buf;
        yuvinfo->u_off       = p_buf + ow * oh;
        yuvinfo->v_off       = p_buf + ow * oh + ow * oh / 4;
    }
    return p_buf;
}

static int32_t scale3_fb_buf_free(struct scale3_normal_msi *scale3, uint8_t *p_buf)
{
    (void) scale3;
    decode_mem_free((uint8_t *) p_buf);
    return 0;
}

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
            os_msgq_del(&scale3->msgq);

            struct scale3_normal_cmd_s *normal_cmd;
            if (scale3->normal_cmd)
            {
                STREAM_FREE(scale3->normal_cmd);
                scale3->normal_cmd = NULL;
            }
            while (RB_GET(&scale3->extern_rb, normal_cmd))
            {
                STREAM_FREE(normal_cmd);
            }
            fbpool_destroy(&scale3->pool);
            decode_mem_free_all(scale3->mem_info, scale3->mem_info_size, STREAM_FREE);
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
            scale3->exit  = 1;

            if (scale3->extern_fb)
            {
                msi_delete_fb(scale3->msi, scale3->extern_fb);
                scale3->extern_fb = NULL;
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
            scale3_fb_buf_free(scale3, (uint8_t *) fb->data);
            fb->data = NULL;
            if (fb->keyfrm)
            {
                scale3->extern_fb_ready = 1;
            }
            if (!scale3->exit)
            {
                os_run_work(&scale3->work);
            }
        }
        break;

        // 私有命令,特定结构体{w,h,time,stype}
        case MSI_CMD_SCALE3_NORMAL:
        {
            uint32_t cmd_self = (uint32_t) param1;
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
    struct scale3_normal_msi *scale3 = (struct scale3_normal_msi *) irq_data;
    struct framebuff         *fb     = NULL;
    txYuvInfo_t              *yuvinfo;
    uint8_t                  *p_buf;
    uint32_t                  ow = 0, oh = 0;
    uint16_t                  r_oh;
    uint16_t                  iw;
    uint16_t                  ih;
    uint8_t                   new_frame_flag = 0;
    // 报错,直接退出
    if (param1)
    {
        return 0;
    }

    // 是否可以获取新的帧
    if (scale3->splice_kick % 2 == 0)
    {
        new_frame_flag = 1;
    }
    else
    {
        fb = scale3->fb;
    }

    // 如果是需要额外抽一帧生成特定size的yuv
    if (new_frame_flag)
    {
        fb = NULL;
        if (scale3->extern_fb)
        {
            p_buf             = (uint8_t *) scale3->extern_fb->data;
            fb                = scale3->extern_fb;
            scale3->extern_fb = NULL;
        }
        else
        {
            if (scale3->start)
            {
                ow                = scale3->ow;
                oh                = scale3->oh;
                // 先检查内存池是否有符合内存块,没有就去workqueue去申请
                uint32_t buf_size = ow * oh * 3 / 2;
                p_buf             = scale3_fb_buf_malloc(scale3, 0, ow, oh, scale3->x, scale3->y);
                if (p_buf)
                {
                    fb = fbpool_get(&scale3->pool, 0, scale3->msi);
                    // 没有fb,则释放内存
                    if (!fb)
                    {
                        scale3_fb_buf_free(scale3, p_buf);
                    }
                    else
                    {

                        fb->codec_info = (void *) (p_buf + buf_size);
                        fb->data       = (uint8_t *) p_buf;
                        fb->len        = buf_size;
                    }
                }
            }
        }
    }
    // 空间不够或者说没有需要产生新的帧,则关闭scale3
    if (!fb)
    {
        scale_close(scale3->scale_dev);
        goto scale3_stream_done_end;
    }

    yuvinfo = (txYuvInfo_t *) fb->codec_info;
    ow      = yuvinfo->width;
    oh      = yuvinfo->height;
    r_oh    = scale3->splice_en ? oh / 2 : oh;
    iw      = scale3->iw;
    ih      = scale3->splice_en ? scale3->ih / 2 : scale3->ih;
    p_buf   = (uint8_t *) fb->data;
    scale_set_in_out_size(scale3->scale_dev, iw, ih, ow, r_oh);
    scale_set_step(scale3->scale_dev, iw, ih, ow, r_oh);
    scale_set_out_yaddr(scale3->scale_dev, (uint32) p_buf + ow * r_oh * scale3->splice_kick);
    scale_set_out_uaddr(scale3->scale_dev, (uint32) p_buf + ow * oh + ow * r_oh / 4 * scale3->splice_kick);
    scale_set_out_vaddr(scale3->scale_dev, (uint32) p_buf + ow * oh + ow * oh / 4 + ow * r_oh / 4 * scale3->splice_kick);

scale3_stream_done_end:
    // 发送now_data,发送失败也要返回
    if (scale3->splice_kick % 2 == 0)
    {
        if (os_msgq_put(&scale3->msgq, (uint32_t) scale3->fb, 0))
        {
            // 正常不能中断del,但是这个模块是内部,只要del没有一些等待信号量操作,问题不大
            msi_delete_fb(NULL, scale3->fb);
            scale3->fb = NULL;
        }
    }
    if (!fb)
    {
        scale3->ready = 1;
    }
    scale3->fb = fb;
    if (scale3->splice_en)
    {
        scale3->splice_kick++;
    }
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
    struct scale3_normal_msi *scale3     = (struct scale3_normal_msi *) work;
    struct scale_device      *scale_dev  = scale3->scale_dev;
    uint16_t                  delay_time = 1000;
    struct framebuff         *fb         = NULL;
    struct framebuff         *e_fb       = NULL;
    txYuvInfo_t              *yuvinfo    = NULL;
    uint16_t                  ow = 0, oh = 0;
    uint16_t                  iw;
    uint16_t                  ih;
    uint16_t                  r_oh;
    uint16_t                  x, y;
    int32_t                   err = -1;
    uint8_t                  *p_buf;
    uint32_t                  buf_size;
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
            x = scale3->normal_cmd->x;
            y = scale3->normal_cmd->y;
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
            buf_size = ow * oh * 3 / 2;
            p_buf    = (uint8_t *) scale3_fb_buf_malloc(scale3, 1, ow, oh, x, y);
            if (p_buf)
            {
                e_fb = msi_alloc_fb(scale3->msi, NULL, p_buf, buf_size, 0, 0);

                // 如果w和h其中一个为0,则使用iw和ih
                e_fb->keyfrm            = 1;
                e_fb->mtype             = F_YUV;
                e_fb->stype             = scale3->normal_cmd->force_type;
                e_fb->datatag           = ~0; // 设置特殊标志,用于识别是否是额外生成的fb
                e_fb->codec_info        = (void *) (p_buf + buf_size);
                scale3->extern_fb_ready = 0;
                scale3->extern_fb       = e_fb;
                STREAM_FREE(scale3->normal_cmd);
                scale3->normal_cmd = NULL;
            }
            else
            {
                scale3_fb_buf_free(scale3, p_buf);
            }
        }
    }

    fb = (struct framebuff *) os_msgq_get2(&scale3->msgq, 0, &err);
    if (fb)
    {
        if (!fb->keyfrm)
        {
            _os_printf(KERN_INFO "S");
            if (fb->codec_info)
            {
                fb->mtype = F_YUV;
                fb->stype = scale3->force_stype;
            }
        }

        if (fb->codec_info)
        {
            msi_output_fb(scale3->msi, fb, 0);
        }
        else
        {
            msi_delete_fb(scale3->msi, fb);
        }
    }
    fb = NULL;
    // scale3可能空间不够关闭了中断,也可能是第一次启动
    if (scale3->ready)
    {
        if (scale3->extern_fb)
        {
            fb                = scale3->extern_fb;
            scale3->extern_fb = NULL;
        }
        else
        {
            if (scale3->start)
            {
                ow       = scale3->ow;
                oh       = scale3->oh;
                x        = scale3->x;
                y        = scale3->y;
                buf_size = ow * oh * 3 / 2;
                p_buf    = (uint8_t *) scale3_fb_buf_malloc(scale3, 1, ow, oh, x, y);
                if (p_buf)
                {

                    fb = fbpool_get(&scale3->pool, 0, scale3->msi);
                    // 没有fb,则释放内存
                    if (!fb)
                    {
                        scale3_fb_buf_free(scale3, (uint8_t *) p_buf);
                    }
                    else
                    {

                        fb->codec_info = (void *) (p_buf + buf_size);
                        fb->data       = (uint8_t *) p_buf;
                        fb->len        = buf_size;
                    }
                }
            }
        }
        scale3->fb = fb;
        fb         = NULL;
        if (!scale3->fb)
        {
            _os_printf(KERN_INFO "D");
            goto scale3_normal_msi_work_end;
        }
        yuvinfo = (txYuvInfo_t *) scale3->fb->codec_info;
        ow      = yuvinfo->width;
        oh      = yuvinfo->height;
        iw      = scale3->iw;
        // 拼接虽然是两个镜头拼接,实际硬件还是一个一个镜头数据输入,所以output偏移修改,硬件寄存器依然按照原来配置
        if (scale3->splice_en)
        {
            ih   = scale3->ih / 2;
            r_oh = oh / 2;
        }
        else
        {
            ih   = scale3->ih;
            r_oh = oh;
        }
        scale_set_in_out_size(scale_dev, iw, ih, ow, r_oh);
        scale_set_step(scale_dev, iw, ih, ow, r_oh);
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
        scale3->ready       = 0;
        scale3->splice_kick = 0;
        // 双镜头拼接,需要等待特定镜头完成才能正常开始
        if (scale3->splice_en)
        {
            scale3->splice_kick++;
            vppdone_func_register(SCALE3_KICK, vpp_start_scale3, (uint32) scale3);
        }
        else
        {
            scale_open(scale_dev);
        }
    }

scale3_normal_msi_work_end:
    // 定时唤醒,释放内存
    if (delay_time)
    {
        os_run_work_delay(work, delay_time);
    }
    decode_mem_check(scale3->mem_info, scale3->mem_info_size, MAX_TTL, STREAM_FREE);
    return 0;
}

// 由于硬件scale3只有一个,当前msi名称先固定
struct msi *scale3_normal_msi(const char *name, struct scale3_cfg *cfg)
{
    uint8_t isnew = 0;
    (void) name;
    if (!cfg)
    {
        return NULL;
    }
    struct msi *msi = msi_new(S_PREVIEW_SCALE3, 0, &isnew);
    if (isnew)
    {
        struct scale_device      *scale_dev = (struct scale_device *) dev_get(HG_SCALE3_DEVID);
        struct scale3_normal_msi *scale3    = (struct scale3_normal_msi *) STREAM_ZALLOC(sizeof(struct scale3_normal_msi) + sizeof(struct mem_info *) * MAX_COUNT);
        scale3->scale_dev                   = scale_dev;
        scale3->ready                       = 1;
        scale3->ow                          = cfg->ow;
        scale3->oh                          = cfg->oh;
        scale3->msi                         = msi;
        scale3->force_stype                 = cfg->force_stype;
        scale3->splice_en                   = cfg->splice_en;
        scale3->extern_fb_ready             = 1;
        scale3->start                       = cfg->start;
        scale3->mem_info                    = (struct mem_info **) (scale3 + 1); // 放到结构体的后面
        scale3->mem_info_size               = MAX_COUNT;
        msi->priv                           = (void *) scale3;
        msi->action                         = const_scale3_msi_action;
        msi->enable                         = 1;

        os_msgq_init(&scale3->msgq, MAX_COUNT);
        fbpool_init(&scale3->pool, MAX_COUNT, NULL, NULL);
        RB_INIT(&scale3->extern_rb, EXTERN_RB_COUNT);
        get_vpp_w_h(&scale3->iw, &scale3->ih);
        OS_WORK_INIT(&scale3->work, scale3_normal_msi_work, 0);
        os_run_work(&scale3->work);
    }
    return msi;
}