
#include "basic_include.h"
#include "dev/jpg/hgjpg.h"
#include "dev/scale/hgscale.h"
#include "devid.h"
#include "hal/h264.h"
#include "lib/lcd/lcd.h"
#include "osal/work.h"
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

#include "hal/vcodec.h"
#include "multimedia/video.h"
#include "video/h264/h264_drv.h"
#include "stream_define.h"
#include "decode_mem.h"
#include "decode_common.h"

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define H264_ROM_MIN_SIZE (100 * 1024 + 4096)
void scale2_from_h264_config_for_msi(struct scale_device *scale_dev, uint32_t yinsram, uint32_t uinsram, uint32_t vinsram, uint32_t yuvoutbuf, uint32 in_w, uint32 in_h, uint32 out_w, uint32 out_h,
                                     uint8_t larger);

struct h264_decode_hdl
{
    // 通用结构体
    DECODE_COMMON_HDL;
    // 私有结构
    uint8_t           *ref_mem;
    uint32_t           ref_max_size;
    struct str_info    h264_str;
    struct h264_header h264_head;
    struct h264_cfg_t  dec_cfg;
    struct h264_ctl_t  dec_ctl;
};

/***********************************************h264的特有函数************************************************************* */
static void h264_decode_channel_free_data(void *ptr, void *priv)
{
    if (ptr)
    {
        decode_mem_free(ptr);
    }
}

static int32_t h264_lock(void *decode, void *hd)
{
    return RET_OK;
}
static int32_t h264_unlock(void *decode, void *hd)
{
    return RET_OK;
}

static int32_t get_h264_decode_size(void *de, void *hd)
{
    struct decode_msi_s       *decode = (struct decode_msi_s *) de;
    struct h264_decode_hdl    *hdl    = decode->current_hdl;
    struct vcodec_decode_req  *req;
    struct vcodec_decode_info *decode_info;
    req          = hdl->req;
    // 先去搜索是否有足够空间的内存,如果没有就申请
    decode_info  = &req->info;
    // 计算解码后的空间
    uint32_t t_w = decode_info->target_width ? decode_info->target_width : hdl->w;
    uint32_t t_h = decode_info->target_height ? decode_info->target_height : hdl->h;

    hdl->dw           = t_w;
    hdl->dh           = t_h;
    int32_t need_size = t_w * t_h * 3 / 2;
    return need_size;
}

static int32_t h264_decode_ready(void *de, void *hd)
{
    struct decode_msi_s      *decode = (struct decode_msi_s *) de;
    struct h264_decode_hdl   *hdl    = decode->current_hdl;
    struct vcodec_decode_req *req    = hdl->req;
    uint32_t                  dst    = (uint32_t) hdl->decode_addr;
    uint32_t                  y      = (uint32_t) decode->scaler2buf_y;
    uint32_t                  u      = (uint32_t) decode->scaler2buf_u;
    uint32_t                  v      = (uint32_t) decode->scaler2buf_v;
    uint16_t                  iw     = hdl->w;
    uint16_t                  ih     = hdl->h;
    uint16_t                  ow     = hdl->dw;
    uint16_t                  oh     = hdl->dh;

    if (req)
    {
        // 设置好yoff uoff voff
        req->info.actual_width  = hdl->dw;
        req->info.actual_height = hdl->dh;
        req->info.y_off         = hdl->decode_addr;
        req->info.u_off         = hdl->decode_addr + hdl->dw * hdl->dh;
        req->info.v_off         = hdl->decode_addr + hdl->dw * hdl->dh + hdl->dw * hdl->dh / 4;
    }
    //os_printf("H264 w:%d\th:%d\tdw:%d\tdh:%d\n", iw, ih, ow, oh);
    scale2_from_h264_config_for_msi(decode->scale_dev, y, u, v, dst, iw, ih, ow, oh, 10);
    scale_request_irq(decode->scale_dev, FRAME_END, general_scale2_done, (uint32) decode);
    scale_request_irq(decode->scale_dev, INBUF_OV, general_scale2_ov, (uint32) decode);
    h264_request_irq((struct h264_device *) hdl->decode_dev, H264_FRAME_DONE, (h264_irq_hdl) general_decode_done, (uint32) decode);

    return 0;
}

static int32_t h264_decode_kick(void *de, void *hd)
{
    struct decode_msi_s      *decode  = (struct decode_msi_s *) de;
    struct h264_decode_hdl   *hdl     = (struct h264_decode_hdl *) decode->current_hdl;
    struct vcodec_decode_req *req     = hdl->req;
    uint32_t                  ref_mem = (uint32_t) (hdl->ref_mem + 0xfff) & (~0xfff);
    struct framebuff         *rfb     = req->fb;
    uint32_t                  dst     = (uint32_t) rfb->data;
    uint32_t                  dst_len = rfb->len;
    uint8_t                  *rom_ptr = (uint8_t *) (((uint32_t) decode->rom + 0xfff) & (~0xfff));

    struct str_info    *h264_str  = &hdl->h264_str;
    struct h264_header *h264_head = &hdl->h264_head;
    struct h264_cfg_t  *dec_cfg   = &hdl->dec_cfg;
    struct h264_ctl_t  *dec_ctl   = &hdl->dec_ctl;
    txVideoInfo_t      *info      = (txVideoInfo_t *) req->fb->codec_info;
    h264_avcc_info_t   *h264_info = (h264_avcc_info_t *) info->extradata;

    decode->hardware_ready    = 0;
    decode->last_decode_time  = os_jiffies();
    decode->rom_last_use_time = os_jiffies();
    h264_clr_intr((struct h264_device *) hdl->decode_dev);
    // 这里开启对应的h264解码
    // 如果是I帧,就去处理sps和pps
    if (rfb->keyfrm)
    {
        sps_setting(h264_str, h264_head, hdl->w, hdl->h);
        cfg_setting((void *) hdl->decode_dev, h264_head, dec_cfg, dec_ctl);
        h264_dec_refbuf_set((struct h264_device *) hdl->decode_dev, (uint32_t) ref_mem, dec_cfg, dec_ctl);
        pps_setting(h264_str, h264_head, h264_info->pps_data, h264_info->pps_size);
        hdl->sps_flag = 1;
    }
    h264_rom_memcpy(rom_ptr, (uint8_t *) dst, dst_len);
    h264_decode_I_P_setting(h264_str, h264_head, rom_ptr);

    if (rfb->keyfrm)
    {
        dec_ctl->frm_type = 2;
    }
    else
    {
        dec_ctl->frm_type = 0;
    }

    h264_dec_src_room_set((struct h264_device *) hdl->decode_dev, (uint32_t) ref_mem, dec_cfg, dec_ctl);
    h264_dec_a_frame((struct h264_device *) hdl->decode_dev, dst_len + 4, dec_ctl, h264_head, h264_str, (uint32_t) rom_ptr);
    h264_decode_start((struct h264_device *) hdl->decode_dev);
    return RET_OK;
}

static int32_t h264_pre_decode(void *de, void *hd)
{
    int32_t                   ret    = RET_OK;
    struct decode_msi_s      *decode = (struct decode_msi_s *) de;
    struct h264_decode_hdl   *hdl    = decode->current_hdl;
    txVideoInfo_t            *info;
    struct vcodec_decode_req *req;
    uint8_t                  *ref_mem = hdl->ref_mem;
    struct h264_cfg_t        *dec_cfg = &hdl->dec_cfg;
    req                               = hdl->req;
    info                              = (txVideoInfo_t *) req->fb->codec_info;
    struct framebuff *rfb             = req->fb;
    // 每次解码都重新init,所以一定要保证当前没有在编码
    h264_drv_init();
    if (info->width != hdl->w || info->height != hdl->h)
    {
        hdl->sps_flag = 0;
        if (ref_mem && (uint32_t) ref_mem != 0x40000000)
        {
            uint32_t r_w          = ((hdl->w + 15) >> 4) << 4;
            uint32_t r_h          = ((hdl->h + 15) >> 4) << 4;
            uint32_t ref_max_size = (r_w * (r_h + 48)) + (r_w * (r_h + 48)) / 2 + 3 * 4096;
            if (hdl->ref_max_size < ref_max_size)
            {
                STREAM_FREE(ref_mem);
                hdl->ref_mem      = NULL;
                hdl->ref_max_size = 0;
                hdl->w            = 0;
                hdl->h            = 0;
            }
            // 如果空间足够,修改frm_width与frm_height
            else
            {
                hdl->w = info->width;
                hdl->h = info->height;

                dec_cfg->frm_width  = r_w;
                dec_cfg->frm_height = r_h;
            }
        }
    }

    // 如果不是I帧,并且sps_flag没有解析过,就删除编码,等待到I帧
    if (!rfb->keyfrm && !hdl->sps_flag)
    {
        decode_req_done(req, -1);
        decode_mem_free(hdl->decode_addr);
        decode_realse_hdl(decode);
        os_printf("type:%d\n", rfb->keyfrm);
        os_printf("sps_flag:%d\n", hdl->sps_flag);
        ret = RET_ERR;
        goto h264_pre_decode_end;
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
            decode->rom_last_use_time = os_jiffies();
            decode->rom_max_size      = rom_max_size;
        }
    }
    if (rfb->keyfrm)
    {
        if (!ref_mem)
        {
            uint32_t w   = info->width;
            uint32_t h   = info->height;
            uint32_t r_w = ((w + 15) >> 4) << 4;
            uint32_t r_h = ((h + 15) >> 4) << 4;

            ref_mem = hdl->ref_mem = STREAM_MALLOC((r_w * (r_h + 48)) + (r_w * (r_h + 48)) / 2 + 3 * 4096);
            if (ref_mem)
            {
                hdl->ref_max_size = (r_w * (r_h + 48)) + (r_w * (r_h + 48)) / 2 + 3 * 4096;
                sys_dcache_invalid_range((uint32_t *) ref_mem, hdl->ref_max_size);

                dec_cfg->frm_width  = r_w;
                dec_cfg->frm_height = r_h;

                hdl->w = w;
                hdl->h = h;
            }
        }
    }
    // 如果是P帧,如果是I帧only就不需要解码P帧
    else if (!rfb->keyfrm && hdl->I_only)
    {
        decode_req_done(req, -1);
        decode_mem_free(hdl->decode_addr);
        decode_realse_hdl(decode);
        ret = RET_ERR;
        goto h264_pre_decode_end;
    }

    if (!hdl->I_only && !ref_mem)
    {
        os_printf("malloc fail ref_mem:%X\n", ref_mem);
        ret = RET_ERR;
        goto h264_pre_decode_end;
    }
    // 申请空间失败
    if (!decode->rom)
    {
        os_printf("malloc fail rom:%X\tref_mem:%X\n", decode->rom, ref_mem);
        ret = RET_ERR;
        goto h264_pre_decode_end;
    }

    if (info->width != hdl->w || info->height != hdl->h)
    {
        decode_req_done(req, -1);
        os_printf(KERN_ERR "h264_decode_msi: w[%d] or h[%d] not match dw[%d],dh[%d]\n", hdl->w, hdl->h, info->width, info->height);
        // 移除,不符合要求,不解码
        decode_mem_free(hdl->decode_addr);
        decode_realse_hdl(decode);
        ret = RET_ERR;
        goto h264_pre_decode_end;
    }
h264_pre_decode_end:
    return ret;
}

static int32_t h264_decode_done(void *de, void *hd)
{
    int32_t                 ret     = RET_OK;
    struct decode_msi_s    *decode  = (struct decode_msi_s *) de;
    struct h264_decode_hdl *hdl     = decode->current_hdl;
    struct h264_ctl_t      *dec_ctl = &hdl->dec_ctl;
    h264_dec_intr_status((struct h264_device *) hdl->decode_dev, dec_ctl);
    //-- clear the frm end flag
    h264_clr_intr((struct h264_device *) hdl->decode_dev);
    h264_dec_flag_chk(dec_ctl->enc_end_flags);
    return ret;
}

static int32_t h264_decode_free_hdl(void *de, void *hd)
{
    struct h264_decode_hdl *hdl = hd;
    os_event_del(&hdl->evt);
    if (hdl->ref_mem)
    {
        STREAM_FREE(hdl->ref_mem);
    }
    STREAM_LIBC_FREE(hdl);
    return 0;
}

const struct decode_fn h264_decode_fn = {
        .free            = h264_decode_channel_free_data,
        .lock            = h264_lock,
        .unlock          = h264_unlock,
        .get_decode_size = get_h264_decode_size,
        .decode_ready    = h264_decode_ready,
        .decode_kick     = h264_decode_kick,
        .pre_decode      = h264_pre_decode,
        .decode_done     = h264_decode_done,
        .decode_free_hdl = h264_decode_free_hdl,
};

static int32_t register_h264_decode_channel(struct msi *m)
{
    struct decode_msi_s    *decode = (struct decode_msi_s *) m->priv;
    int32_t                 ret    = RET_ERR;
    struct h264_decode_hdl *hdl    = (struct h264_decode_hdl *) STREAM_LIBC_ZALLOC(sizeof(struct h264_decode_hdl));

    if (hdl)
    {
        os_event_init(&hdl->evt);
        os_event_set(&hdl->evt, DECODE_CHAN_EMPTY, NULL);
        hdl->decode_dev = (void *) dev_get(HG_H264_DEVID);
        hdl->fn         = &h264_decode_fn;
        hdl->decode     = decode;
        hdl->msi        = m;
        uint32_t flag   = disable_irq();
        ret             = get_free_chan(decode, hdl);
        enable_irq(flag);
        // 失败,返回错误
        if (ret)
        {
            os_event_del(&hdl->evt);
            STREAM_LIBC_FREE(hdl);
            hdl = NULL;
        }
    }
    return (int32_t) hdl;
}

static void *h264_decode_open(struct vcodec_device *dev)
{
    struct vcodec_device_decode *h264_dev = (struct vcodec_device_decode *) dev;
    struct msi                  *m        = decode_core_msi(DECODE_CORE_NAME, h264_dev->max_buf_num);
    struct common_decode_hdl    *hdl      = NULL;
    hdl                                   = (struct common_decode_hdl *) register_h264_decode_channel(m);
    if (!hdl)
    {
        msi_destroy(m);
    }
    return (void *) hdl;
}

static int32 h264_decode_close(struct vcodec_device *dev, void *chan)
{
    struct common_decode_hdl *hdl    = (struct common_decode_hdl *) chan;
    struct msi               *m      = hdl->msi;
    struct decode_msi_s      *decode = (struct decode_msi_s *) m->priv;
    // 通知workqueue去释放通道
    decode->gc                       = 1;
    // 设置通道关闭状态
    hdl->closed                      = 1;
    os_run_work(&decode->work);
    // 等待workqueue去停止解码或者msi退出后退出
    msi_destroy(m);
    return 0;
}

static int32 h264_decode(struct vcodec_device *dev, struct vcodec_decode_req *req)
{

    int32                     ret    = -1;
    struct common_decode_hdl *hdl    = (struct common_decode_hdl *) req->chan;
    struct msi               *m      = hdl->msi;
    struct decode_msi_s      *decode = (struct decode_msi_s *) m->priv;

    ret = os_event_wait(&hdl->evt, DECODE_CHAN_EMPTY, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
    if (!ret)
    {
        fb_get(req->fb);
        hdl->req = req;
        os_run_work(&decode->work);
    }
    else
    {
        os_printf("ret:%d\taddr:%X\n", ret, RETURN_ADDR());
    }
    return ret;
}

static int32 h264_ioctl(struct vcodec_device *dev, void *chan, enum vcodec_ioctl_cmd cmd, uint32 param)
{
    switch (cmd)
    {
        case VDEC_IOCTL_RELEASE_FBDATA:
        {
            struct common_decode_hdl *hdl       = (struct common_decode_hdl *) chan;
            struct decode_msi_s      *decode    = hdl->decode;
            uint8_t                  *free_data = (uint8_t *) param;
            hdl->fn->free(free_data, decode);
        }
        break;
        case VDEC_IOCTL_CANCLE:
        {
            struct common_decode_hdl *hdl    = (struct common_decode_hdl *) chan;
            struct msi               *m      = hdl->msi;
            struct decode_msi_s      *decode = (struct decode_msi_s *) m->priv;
            // 通知workqueue去释放通道
            decode->gc                       = 1;
            hdl->gc                          = 1;
        }
        break;
        default:
            os_printf("%s:%d fail,cmd:%d\n", __FUNCTION__, __LINE__, cmd);
            break;
    }
    return 0;
}

const struct vcodec_hal_ops h264_hal_ops = {
        .open   = h264_decode_open,
        .close  = h264_decode_close,
        .decode = h264_decode,
        .ioctl  = h264_ioctl,
};

/**
 * @brief               注册h264解码接口
 * @param dev_id        attatch的id
 * @param max_num       解码最大缓冲区数量(内存不够或者达到最大缓冲区都不会解码),注意与mjpeg是共用的
 * @return              是否attach成功
 */
int32 decode_h264_attach(uint32 dev_id, uint32_t max_num)
{

    struct vcodec_device_decode *h264_dev = (struct vcodec_device_decode *) os_zalloc(sizeof(struct vcodec_device_decode));
    if (!h264_dev)
    {
        return -ENOMEM;
    }
    struct vcodec_device *dev = &h264_dev->dev;
    dev->dev.ops              = (const struct devobj_ops *) &h264_hal_ops;
    h264_dev->max_buf_num     = max_num;
    return dev_register(dev_id, (struct dev_obj *) dev);
}