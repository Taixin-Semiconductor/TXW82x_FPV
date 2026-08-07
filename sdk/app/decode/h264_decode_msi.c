#include "basic_include.h"
#include "dev.h"
#include "dev/jpg/hgjpg.h"
#include "dev/scale/hgscale.h"
#include "devid.h"
#include "lib/lcd/lcd.h"
#include "osal/work.h"
#include "stream_frame.h"
#include "sys_config.h"
#include "typesdef.h"
#include "utlist.h"

#include "basic_include.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/multimedia/msi.h"
#include "lib/video/dvp/jpeg/jpg_common.h"
#include "osal/event.h"
#include "user_work/user_work.h"
#include "lib/video/h264/h264_drv.h"
#include "dev/h264/hg264.h"
#include "lib/scale/scale_common.h"
#include "scale/scale_dev.h"

#ifndef DECODE_MAX_W
#define DECODE_MAX_W (320)
#endif

#ifndef DECODE_MAX_H
#define DECODE_MAX_H (180)
#endif
#define DECODE_MAX_SIZE (DECODE_MAX_W * DECODE_MAX_H * 3 / 2)

#define H264_ROM_MIN_SIZE (100 * 1024 + 4096)

extern void scale2_from_jpeg_config_for_msi(struct scale_device *scale_dev, uint32_t yinsram, uint32_t uinsram, uint32_t vinsram, uint32_t yuvoutbuf, uint32 in_w, uint32 in_h, uint32 out_w,
                                            uint32 out_h, uint8_t larger);
// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define MAX_DECODE_NUM (8)

#ifndef MAX_DECODE_YUV_TX
#define MAX_DECODE_YUV_TX (1)
#endif

struct h264_msi_s
{
    struct os_work       work;
    struct msi          *msi;
    struct fbpool        tx_pool;
    uint8_t             *scaler2buf_y;
    uint8_t             *scaler2buf_u;
    uint8_t             *scaler2buf_v;
    struct h264_device  *h264_dev;
    struct scale_device *scale_dev;
    uint32_t             last_decode_time; // 记录上一次解码的时间,预防有异常的时候可以计算超时
    uint32_t             dec_y_offset;
    uint32_t             dec_uv_offset;
    uint16_t             now_decode_pw;
    uint16_t             scale_p1_w;
    uint16_t             p1_w;
    uint16_t             p1_h;
    struct framebuff    *parent_fb;
    struct framebuff    *current_fb;
    uint32_t             max_data_size;

    struct str_info    h264_str;
    struct h264_header h264_head;
    struct h264_cfg_t  dec_cfg;
    struct h264_ctl_t  dec_ctl;

    uint32_t rom_max_size;
    uint8_t *rom;
    uint8_t *ref_mem;
    uint32_t ref_max_size;

    uint16_t w, h;
    // 硬件模块是否准备好
    // 是否自动释放空间(可能会导致碎片化严重,但是可以充分利用空间)
    uint8_t  hardware_ready : 1, auto_free_space : 1, hardware_err : 1, is_register_isr : 1, sps_flag : 1, only_I_H264 : 1, unlock : 1, rev : 1;
};

static void stream_jpg_decode_scale2_done(uint32 irq_flag, uint32 irq_data, uint32 param1)
{
}

static void stream_jpg_decode_scale2_ov_isr(uint32 irq_flag, uint32 irq_data, uint32 param1)
{
    // struct scale_device *scale_dev = (struct scale_device *)irq_data;
}

static int32_t h264_dec_done(uint32 irq_flags, uint32 irq_data, uint32 param)
{
    struct h264_msi_s *decode = (struct h264_msi_s *) irq_data;
    decode->hardware_ready    = 1;
    return 0;
}

static int32 h264_decode_work(struct os_work *work)
{
    int                ret;
    uint8_t            unlock = 0;
    struct h264_msi_s *decode = (struct h264_msi_s *) work;
    struct framebuff  *fb;
    // static int count = 0;
    // 检测解码模块是否完成或者超时代表解码失败
    // 能进去,模块需要reset或者已经解码完毕,可以重新去解码,否则只能慢慢等超时或者硬件解码完成
    if (decode->hardware_err || decode->hardware_ready || os_jiffies() - decode->last_decode_time > 1000)
    {
        if (decode->current_fb && (os_jiffies() - decode->last_decode_time > 1000))
        {
            // 解码可能失败了
            os_printf(KERN_ERR "decode failed1:%d\n", decode->hardware_ready);
            os_printf(KERN_ERR "decode->hardware_err:%d\n", decode->hardware_err);

            decode->sps_flag       = 0;
            decode->hardware_ready = 1;
            // 无论是完成还是失败,都要释放这张图片了
            msi_delete_fb(NULL, decode->parent_fb);
            decode->parent_fb = NULL;

            // 不再解码了
            msi_delete_fb(NULL, decode->current_fb);
            decode->current_fb = NULL;

            // 这里最好将硬件模块停止
            // 解锁
            unlock = 1;
        }
        // 解码异常
        else if (decode->hardware_err)
        {
            h264_dec_intr_status(decode->h264_dev, &decode->dec_ctl);
            //-- clear the frm end flag
            h264_clr_intr(decode->h264_dev);
            h264_dec_flag_chk(decode->dec_ctl.enc_end_flags);
            _os_printf(KERN_ERR "decode failed2\n");
            decode->hardware_err   = 0;
            decode->hardware_ready = 1;
            if (decode->current_fb)
            {
                // 无论是完成还是失败,都要释放这张图片了
                msi_delete_fb(NULL, decode->parent_fb);
                decode->parent_fb = NULL;

                // 不再解码了
                msi_delete_fb(NULL, decode->current_fb);
                decode->current_fb = NULL;
            }

            // 解锁
            unlock = 1;
        }
        // 解码完成,检查有解码完的数据需要发送
        else if (decode->current_fb)
        {
            h264_dec_intr_status(decode->h264_dev, &decode->dec_ctl);
            //-- clear the frm end flag
            h264_clr_intr(decode->h264_dev);
            h264_dec_flag_chk(decode->dec_ctl.enc_end_flags);
            // os_printf("%s:%d\tbuf:%X\n",__FUNCTION__,__LINE__,get_stream_real_data(decode->current_fb));
            fb          = decode->current_fb;
            fb->time    = decode->parent_fb->time;
            fb->mtype   = F_YUV;
            fb->stype   = decode->parent_fb->stype;
            fb->datatag = decode->parent_fb->datatag;
            fb->srcID   = decode->parent_fb->srcID;
            _os_printf("&");

            msi_output_fb(decode->msi, fb);
            // 无论是完成还是失败,都要释放这张图片了
            msi_delete_fb(NULL, decode->parent_fb);
            decode->parent_fb       = NULL;
            decode->current_fb      = NULL;
            decode->is_register_isr = 0;
            // 解锁
            unlock                  = 1;
        }
        // 硬件可用,但是current_fb不存在
        // 有两种可能
        // 一、没有需要解码的图片
        // 二、有需要解码的图片,但是fb没有申请成功或者内存空间不足以去解码图片数据
        else
        {
        }

        if (unlock)
        {
            scale_mutex_unlock(2, SCALE_LOCK_H264_DECODE1);
            unlock         = 0;
            decode->unlock = 0;
        }

        // 这里没有解码的图片,尝试去看看有没有需要解码图片
        if (!decode->parent_fb)
        {
            // 接收图片,尝试看看是否要解码
            decode->parent_fb = msi_get_fb(decode->msi, 0);

            // 需要解码,然后申请空间
            if (decode->parent_fb)
            {
                goto start_decode;
            }
            // 不需要解码
            else
            {
                // 为了腾出空间,释放空间
                if (decode->auto_free_space)
                {
                    if (decode->scaler2buf_y)
                    {
                        STREAM_LIBC_FREE(decode->scaler2buf_y);
                        decode->scaler2buf_y = NULL;
                    }

                    if (decode->scaler2buf_u)
                    {
                        STREAM_LIBC_FREE(decode->scaler2buf_u);
                        decode->scaler2buf_u = NULL;
                    }

                    if (decode->scaler2buf_v)
                    {
                        STREAM_LIBC_FREE(decode->scaler2buf_v);
                        decode->scaler2buf_v = NULL;
                    }
                    decode->now_decode_pw = 0;
                }
                goto not_decode;
            }
        }
        else
        {
            goto start_decode;
        }

    // 开始尝试解码
    start_decode:
        // 检查是否需要解码(检查绑定msi的模块是否需要接收)
        {
            uint32_t send_count = msi_output_fb(decode->msi, NULL);
            // os_printf("send_count:%X\n",send_count);
            if (!send_count)
            {
                msi_delete_fb(NULL, decode->parent_fb);
                decode->parent_fb = NULL;
                goto not_decode;
            }
        }

        struct framebuff *rfb       = decode->parent_fb->next;
        struct fb_h264_s *h264_priv = (struct fb_h264_s *) rfb->priv;
        // w变化,就需要等待I帧
        if (decode->w != h264_priv->w || decode->h != h264_priv->h)
        {
            decode->sps_flag = 0;
            if (decode->ref_mem && (uint32_t) decode->ref_mem != 0x40000000)
            {
                uint32_t r_w          = ((h264_priv->w + 15) >> 4) << 4;
                uint32_t r_h          = ((h264_priv->h + 15) >> 4) << 4;
                uint32_t ref_max_size = (r_w * (r_h + 48)) + (r_w * (r_h + 48)) / 2 + 3 * 4096;
                if (decode->ref_max_size < ref_max_size)
                {
                    STREAM_FREE(decode->ref_mem);
                    decode->ref_mem      = NULL;
                    decode->ref_max_size = 0;
                    decode->w            = 0;
                    decode->h            = 0;
                }
                // 如果空间足够,修改frm_width与frm_height
                else
                {
                    decode->w                  = h264_priv->w;
                    decode->h                  = h264_priv->h;
                    decode->dec_cfg.frm_width  = r_w;
                    decode->dec_cfg.frm_height = r_h;
                }
            }
        }

        // 如果不是I帧,并且sps_flag没有解析过,就删除编码,等待到I帧
        if (h264_priv->type != 1 && !decode->sps_flag)
        {
            msi_delete_fb(NULL, decode->parent_fb);
            decode->parent_fb = NULL;
            os_printf("type:%d\n", h264_priv->type);
            os_printf("sps_flag:%d\n", decode->sps_flag);
            goto not_decode;
        }

        // 如果空间不够,那么就重新申请空间
        if (decode->rom_max_size < rfb->len)
        {
            STREAM_FREE(decode->rom);
            decode->rom          = NULL;
            decode->rom_max_size = 0;
        }
        if (!decode->rom)
        {
            uint32_t rom_max_size = rfb->len > H264_ROM_MIN_SIZE ? rfb->len : H264_ROM_MIN_SIZE;
            decode->rom           = STREAM_MALLOC(rom_max_size + 4096);
            if (decode->rom)
            {
                decode->rom_max_size = rom_max_size;
            }
        }
        if (h264_priv->type == 1)
        {
            if (decode->only_I_H264)
            {
                uint32_t w   = h264_priv->w;
                uint32_t h   = h264_priv->h;
                uint32_t r_w = ((w + 15) >> 4) << 4;
                uint32_t r_h = ((h + 15) >> 4) << 4;

                decode->dec_cfg.frm_width  = r_w;
                decode->dec_cfg.frm_height = r_h;

                decode->w = w;
                decode->h = h;

                decode->ref_mem = (uint8_t *) 0x40000000;
            }
            else
            {
                if (!decode->ref_mem)
                {
                    uint32_t w   = h264_priv->w;
                    uint32_t h   = h264_priv->h;
                    uint32_t r_w = ((w + 15) >> 4) << 4;
                    uint32_t r_h = ((h + 15) >> 4) << 4;

                    decode->ref_mem = STREAM_MALLOC((r_w * (r_h + 48)) + (r_w * (r_h + 48)) / 2 + 3 * 4096);
                    if (decode->ref_mem)
                    {
                        decode->ref_max_size = (r_w * (r_h + 48)) + (r_w * (r_h + 48)) / 2 + 3 * 4096;
                        sys_dcache_clean_range((uint32_t *) decode->ref_mem, decode->ref_max_size);

                        decode->dec_cfg.frm_width  = r_w;
                        decode->dec_cfg.frm_height = r_h;

                        decode->w = w;
                        decode->h = h;
                    }
                }
            }
        }
        // 如果是P帧,如果是I帧only就不需要解码P帧
        else if (h264_priv->type == 2 && decode->only_I_H264)
        {
            msi_delete_fb(NULL, decode->parent_fb);
            decode->parent_fb = NULL;
            goto not_decode;
        }

        if (!decode->only_I_H264 && !decode->ref_mem)
        {
            os_printf("malloc fail ref_mem:%X\n", decode->ref_mem);
            goto not_decode;
        }
        // 申请空间失败
        if (!decode->rom)
        {
            os_printf("malloc fail rom:%X\tref_mem:%X\n", decode->rom, decode->ref_mem);
            goto not_decode;
        }

        if (decode->w != h264_priv->w || decode->h != h264_priv->h)
        {
            os_printf(KERN_ERR "h264_decode_msi: w[%d] or h[%d] not match dw[%d],dh[%d]\n", h264_priv->w, h264_priv->h, decode->w, decode->h);
            // 移除,不符合要求,不解码
            msi_delete_fb(NULL, decode->parent_fb);
            decode->parent_fb = NULL;
            goto not_decode;
        }
        // 获取锁
        ret = scale_mutex_lock(2, SCALE_LOCK_H264_DECODE1, NULL);
        if (ret)
        {
            goto not_decode;
        }
        decode->unlock = 1;
        // 申请到fb,申请解码空间
        fb             = fbpool_get(&decode->tx_pool, 0, decode->msi);
        if (fb)
        {
            // 因为这个是解码的数据,所以默认parent_fb的data是一个参数内容,而不是真实的data,真实jpg的data应该是附在parent_fb后面其他节点
            struct jpg_decode_arg_s *msg = (struct jpg_decode_arg_s *) decode->parent_fb->data;
            // 没有找到,代表发送过来的不是jpg或者说对应参数没有配置,不解码
            if (msg)
            {
                // 为fb申请解码空间,申请不到下次申请
                fb->len = msg->yuv_arg.out_w * msg->yuv_arg.out_h * 3 / 2;
                if (fb->len <= decode->max_data_size)
                {
                    struct jpg_decode_arg_s *cfg;
                    cfg = (struct jpg_decode_arg_s *) fb->priv;
                    memset(cfg, 0, sizeof(struct jpg_decode_arg_s));
                    // 提前配置好data的长度
                    memcpy(fb->priv, msg, sizeof(struct jpg_decode_arg_s));
                    msi_do_cmd(decode->msi, MSI_CMD_DECODE, MSI_DECODE_SET_W_H, (uint32_t) fb);

                    // fb->type = SET_DATA_TYPE(YUV, GET_DATA_TYPE2(decode->parent_fb->type));

                    // msi_do_cmd(decode->msi, MSI_CMD_DECODE, MSI_DECODE_SET_STEP, (uint32_t) cfg);
                    msi_do_cmd(decode->msi, MSI_CMD_DECODE, MSI_DECODE_READY, (uint32_t) fb);
                    if (!decode->hardware_err)
                    {
                        msi_do_cmd(decode->msi, MSI_CMD_DECODE, MSI_DECOE_START, (uint32_t) decode->parent_fb);
                    }

                    decode->current_fb       = fb;
                    decode->last_decode_time = os_jiffies();
                }
                else
                {
                    os_printf(KERN_ERR "config err,max size w:%d\th:%d\tmax_mem:%d\n", DECODE_MAX_W, DECODE_MAX_H, decode->max_data_size);
                    // 不符合,需要删除,不去解码
                    msi_delete_fb(NULL, decode->parent_fb);
                    decode->parent_fb = NULL;

                    msi_delete_fb(NULL, fb);
                    fb = NULL;

                    // 解锁
                    unlock = 1;
                }
            }
            else
            {
                os_printf("%s:%d\tdecode msg isn't normal\tmsg:%X\tname:%s\n", __FUNCTION__, __LINE__, msg, decode->parent_fb->msi->name);

                // 不符合,需要删除,不去编码
                msi_delete_fb(NULL, decode->parent_fb);
                decode->parent_fb = NULL;

                msi_delete_fb(NULL, fb);
                fb     = NULL;
                // 解锁
                unlock = 1;
            }
        }
        else
        {
            // 解锁
            unlock = 1;
        }
    }
not_decode:
    if (unlock)
    {
        scale_mutex_unlock(2, SCALE_LOCK_H264_DECODE1);
        decode->unlock = 0;
    }
    os_run_work_delay(work, 1);
    return 0;
}

static int decode_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int                ret    = RET_OK;
    struct h264_msi_s *decode = (struct h264_msi_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            struct framebuff *fb;
            if (decode->unlock)
            {
                scale_mutex_unlock(2, SCALE_LOCK_H264_DECODE1);
                decode->unlock = 0;
            }

            if (decode->rom)
            {
                STREAM_FREE(decode->rom);
                decode->rom = NULL;
            }

            if (decode->ref_mem && (uint32_t) decode->ref_mem != 0x40000000)
            {
                STREAM_FREE(decode->ref_mem);
                decode->ref_mem      = NULL;
                decode->ref_max_size = 0;
            }
            // 释放资源fb资源文件,priv是独立申请的
            while (1)
            {
                fb = fbpool_get(&decode->tx_pool, 0, NULL);
                if (!fb)
                {
                    break;
                }
                // 预分配空间释放
                if (fb->priv)
                {
                    STREAM_LIBC_FREE(fb->priv);
                }

                // 预分配空间释放
                if (fb->data)
                {
                    STREAM_FREE(fb->data);
                    fb->data = NULL;
                }
            }
            fbpool_destroy(&decode->tx_pool);
            if (decode->scaler2buf_y)
            {
                STREAM_LIBC_FREE(decode->scaler2buf_y);
                decode->scaler2buf_y = NULL;
            }

            if (decode->scaler2buf_u)
            {
                STREAM_LIBC_FREE(decode->scaler2buf_u);
                decode->scaler2buf_u = NULL;
            }

            if (decode->scaler2buf_v)
            {
                STREAM_LIBC_FREE(decode->scaler2buf_v);
                decode->scaler2buf_v = NULL;
            }
            STREAM_LIBC_FREE(decode);
        }
        break;

        // 停止硬件
        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&decode->work, 1);
            // 关闭硬件
            scale_close(decode->scale_dev);

            if (decode->current_fb)
            {
                msi_delete_fb(NULL, decode->current_fb);
                decode->current_fb = NULL;
            }
            if (decode->parent_fb)
            {
                msi_delete_fb(NULL, decode->parent_fb);
                decode->parent_fb = NULL;
            }
        }
        break;

        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            fbpool_put(&decode->tx_pool, fb);
            // 不需要内核去释放fb
            ret = RET_OK + 1;
        }
        break;

        case MSI_CMD_DECODE:
        {
            uint32_t cmd_self = (uint32_t) param1;
            uint32_t arg      = param2;
            switch (cmd_self)
            {
                case MSI_DECODE_SET_W_H:
                {
                    struct framebuff        *fb     = (struct framebuff *) arg;
                    struct jpg_decode_arg_s *cfg    = (struct jpg_decode_arg_s *) fb->priv;
                    struct h264_msi_s       *decode = (struct h264_msi_s *) msi->priv;
                    decode->p1_w                    = cfg->yuv_arg.out_w;
                    decode->p1_h                    = cfg->yuv_arg.out_h;
                    decode->scale_p1_w              = ((decode->p1_w + 3) / 4) * 4;
                    if (cfg->rotate)
                    {
                        decode->dec_y_offset  = decode->p1_w * (decode->p1_h - 1);
                        decode->dec_uv_offset = ((decode->scale_p1_w / 2 + 3) / 4) * 4 * (decode->p1_h / 2 - 1);
                    }
                    else
                    {
                        decode->dec_y_offset  = 0;
                        decode->dec_uv_offset = 0;
                    }

                    cfg->yuv_arg.y_off  = decode->dec_y_offset;
                    cfg->yuv_arg.uv_off = decode->dec_uv_offset;
                    cfg->yuv_arg.y_size = decode->scale_p1_w * decode->p1_h;
                }
                break;

                case MSI_DECODE_SET_IN_OUT_SIZE:
                {
                }
                break;

                case MSI_DECODE_SET_STEP:
                {
                }
                break;

                case MSI_DECODE_READY:
                {

                    struct h264_msi_s       *decode = (struct h264_msi_s *) msi->priv;
                    struct framebuff        *fb     = (struct framebuff *) arg;
                    uint32                   dst    = (uint32_t) fb->data;
                    struct jpg_decode_arg_s *cfg    = (struct jpg_decode_arg_s *) fb->priv;
                    uint8_t                  err    = 0;

#if 0
                    h264_request_irq(decode->h264_dev, H264_FRAME_DONE, h264_dec_done, (uint32) decode);
#else
                    // 如果空间不够,就重新申请把
                    if (decode->p1_w > decode->now_decode_pw)
                    {
                        if (decode->scaler2buf_y)
                        {
                            STREAM_LIBC_FREE(decode->scaler2buf_y);
                            decode->scaler2buf_y = NULL;
                        }

                        if (decode->scaler2buf_u)
                        {
                            STREAM_LIBC_FREE(decode->scaler2buf_u);
                            decode->scaler2buf_u = NULL;
                        }

                        if (decode->scaler2buf_v)
                        {
                            STREAM_LIBC_FREE(decode->scaler2buf_v);
                            decode->scaler2buf_v = NULL;
                        }
                        uint8_t scale_coeff = 0;
                        uint32_t iw = cfg->decode_w;
//                        uint32_t ih = cfg->decode_h;
                        uint32_t ow = decode->p1_w;
//                        uint32_t oh = decode->p1_h;
                        if(iw >= ow)
                        {
                            scale_coeff = 1;
                        }
                        else
                        {
                            scale_coeff = 2;
                        }

                        // 默认一定申请到,没有做申请失败的处理
                        decode->scaler2buf_y = STREAM_LIBC_MALLOC(0x20+ow+scale_coeff*20*SRAMBUF_WLEN*4+256);
                        decode->scaler2buf_u = STREAM_LIBC_MALLOC(((0x12+ow/2+scale_coeff*11*SRAMBUF_WLEN*2+128 + 3)/4)*4);
                        decode->scaler2buf_v = STREAM_LIBC_MALLOC(((0x12+ow/2+scale_coeff*11*SRAMBUF_WLEN*2+128 + 3)/4)*4);
                        if (!decode->scaler2buf_y || !decode->scaler2buf_u || !decode->scaler2buf_v)
                        {
                            os_printf(KERN_ERR "%s:%d err\tpw:%X\tnow_pw:%X\n", __FUNCTION__, __LINE__, decode->p1_w, decode->now_decode_pw);
                            os_printf(KERN_ERR "fail y:%X\tu:%X\tv:%X\n", decode->scaler2buf_y, decode->scaler2buf_u, decode->scaler2buf_v);
                            decode->now_decode_pw = 0;
                            err                   = 1;
                            if (decode->scaler2buf_y)
                            {
                                STREAM_LIBC_FREE(decode->scaler2buf_y);
                                decode->scaler2buf_y = NULL;
                            }

                            if (decode->scaler2buf_u)
                            {
                                STREAM_LIBC_FREE(decode->scaler2buf_u);
                                decode->scaler2buf_u = NULL;
                            }

                            if (decode->scaler2buf_v)
                            {
                                STREAM_LIBC_FREE(decode->scaler2buf_v);
                                decode->scaler2buf_v = NULL;
                            }
                        }
                        else
                        {
                            decode->now_decode_pw = decode->p1_w;
                        }
                    }
                    if (!err)
                    {

                        scale2_from_h264_config_for_msi(decode->scale_dev, (uint32_t) decode->scaler2buf_y, (uint32_t) decode->scaler2buf_u, (uint32_t) decode->scaler2buf_v, dst, cfg->decode_w,
                                                        cfg->decode_h, cfg->yuv_arg.out_w, cfg->yuv_arg.out_h, 10);

                        if (!decode->is_register_isr)
                        {
                            scale_request_irq(decode->scale_dev, FRAME_END, (scale_irq_hdl) &stream_jpg_decode_scale2_done, (uint32) decode);
                            scale_request_irq(decode->scale_dev, INBUF_OV, (scale_irq_hdl) &stream_jpg_decode_scale2_ov_isr, (uint32) decode);
                            h264_request_irq(decode->h264_dev, H264_FRAME_DONE, h264_dec_done, (uint32) decode);
                            // decode->is_register_isr = 1;
                        }
                    }
                    else
                    {
                        decode->hardware_err = 1;
                    }
#endif
                }
                break;

                case MSI_DECOE_START:
                {
                    struct framebuff  *fb        = (struct framebuff *) arg;
                    struct framebuff  *rfb       = fb->next;
                    struct h264_msi_s *decode    = (struct h264_msi_s *) msi->priv;
                    struct fb_h264_s  *h264_priv = (struct fb_h264_s *) rfb->priv;
                    uint32_t           dst       = (uint32_t) rfb->data;
                    uint32_t           dst_len   = rfb->len;
                    uint8_t           *rom_ptr   = (uint8_t *) (((uint32_t) decode->rom + 0xfff) & (~0xfff));

                    decode->hardware_ready   = 0;
                    decode->last_decode_time = os_jiffies();
                    // 这里开启对应的h264解码
                    // 如果是I帧,就去处理sps和pps
                    if (h264_priv->type == 1 && !decode->sps_flag)
                    {
                        sps_setting(&decode->h264_str, &decode->h264_head, decode->w, decode->h);
                        cfg_setting(decode->h264_dev, &decode->h264_head, &decode->dec_cfg, &decode->dec_ctl);
                        h264_dec_refbuf_set(decode->h264_dev, ((uint32_t) decode->ref_mem + 0xfff) & (~0xfff), (struct h264_cfg_t *) &decode->dec_cfg, (struct h264_ctl_t *) &decode->dec_ctl);
                        pps_setting(&decode->h264_str, &decode->h264_head, h264_priv->pps, h264_priv->pps_len);
                        decode->sps_flag = 1;
                    }
                    h264_rom_memcpy(rom_ptr, (uint8_t *) dst, dst_len);
                    h264_decode_I_P_setting(&decode->h264_str, &decode->h264_head, rom_ptr);

                    if (h264_priv->type == 1)
                    {
                        decode->dec_ctl.frm_type = 2;
                    }
                    else
                    {
                        decode->dec_ctl.frm_type = 0;
                    }

                    h264_dec_src_room_set(decode->h264_dev, ((uint32_t) decode->ref_mem + 0xfff) & (~0xfff), (struct h264_cfg_t *) &decode->dec_cfg, (struct h264_ctl_t *) &decode->dec_ctl);
                    h264_dec_a_frame(decode->h264_dev, dst_len + 4, &decode->dec_ctl, &decode->h264_head, &decode->h264_str, (uint32_t) rom_ptr);
                    h264_decode_start(decode->h264_dev);
                }
                break;
            }
        }
        break;

        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            if (fb->mtype != F_JPG_DECODE_MSG)
            {
                ret = RET_OK + 1;
            }
        }

        break;

        default:
            break;
    }

    return ret;
}
// 解码是要先获取jpg,然后申请解码后size的空间,然后启动解码
// 不再支持传入msi的name(因为硬件解码模块只有一个,所以不支持这个msi的名称命名)
struct msi *h264_decode_msi(const char *name, uint8_t only_I_H264)
{
    (void) name; // 不再支持修改名字
    uint8_t            is_new;
    struct msi        *msi    = msi_new(name, MAX_DECODE_NUM, &is_new);
    struct h264_msi_s *decode = (struct h264_msi_s *) msi->priv;
    if (is_new)
    {
        decode      = (struct h264_msi_s *) STREAM_LIBC_ZALLOC(sizeof(struct h264_msi_s));
        decode->msi = msi;
        msi->priv   = (void *) decode;
        msi->action = decode_msi_action;

        uint32_t init_count = 0;
        void    *priv;
        fbpool_init(&decode->tx_pool, 2);
        while (init_count < 2)
        {
            uint8_t *data = (uint8_t *) STREAM_MALLOC(DECODE_MAX_SIZE);
            ASSERT(data);
            sys_dcache_invalid_range((uint32_t *) data, DECODE_MAX_SIZE);
            priv = (void *) STREAM_LIBC_ZALLOC(sizeof(struct jpg_decode_arg_s));
            FBPOOL_SET_INFO(&decode->tx_pool, init_count, data, 0, priv);
            init_count++;
        }

        decode->h264_dev        = (struct h264_device *) dev_get(HG_H264_DEVID);
        decode->scale_dev       = (struct scale_device *) dev_get(HG_SCALE2_DEVID);
        decode->scaler2buf_y    = NULL;
        decode->scaler2buf_u    = NULL;
        decode->scaler2buf_v    = NULL;
        decode->now_decode_pw   = 0;
        decode->hardware_ready  = 1;
        decode->auto_free_space = 1;
        decode->only_I_H264     = only_I_H264;
        // decode->w               = w;
        // decode->h               = h;
        decode->max_data_size   = DECODE_MAX_SIZE;
        // decode->rom             = os_malloc_psram(100 * 1024 + 4096);
        // decode->ref_mem         = os_malloc_psram((w * (h + 48)) + (w * (h + 48)) / 2 + 3 * 4096);
        // sys_dcache_clean_range((uint32_t *) decode->ref_mem, (w * (h + 48)) + (w * (h + 48)) / 2 + 3 * 4096);

        decode->dec_cfg.enc_mode = 0;
        // decode->dec_cfg.frm_width  = w;
        // decode->dec_cfg.frm_height = h;
        msi->enable              = 1;
        OS_WORK_INIT(&decode->work, h264_decode_work, 0);
        os_run_work_delay(&decode->work, 1);
    }

    return msi;
}
