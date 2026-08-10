
#include "basic_include.h"
#include "dev/jpg/hgjpg.h"
#include "dev/scale/hgscale.h"
#include "devid.h"
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
#include "lib/scale/scale_common.h"
#include "hal/vcodec.h"
#include "multimedia/video.h"
#include "stream_define.h"
#include "decode_mem.h"
#include "decode_common.h"
#include "scale/scale_dev.h"
// data 申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC os_malloc
#define STREAM_LIBC_FREE   os_free
#define STREAM_LIBC_ZALLOC os_zalloc

#define DE_Y_SIZE(w, h)  ((ALIGN(w, 8) * h + 0x0f) & (~0x0f))
#define DE_UV_SIZE(w, h) (((ALIGN(w, 8) * h + 0xf) & (~0x0f)) / 4)

#define DE_U_OFF(w, h) ((ALIGN(w, 8) * h + 0x0f) & (~0x0f))
#define DE_V_OFF(w, h) (((ALIGN(w, 8) * h + 0xf) & (~0x0f)) / 4)

void scale2_from_jpeg_config_for_msi(struct scale_device *scale_dev, uint32_t yinsram, uint32_t uinsram, uint32_t vinsram, uint32_t yuvoutbuf, uint32 in_w, uint32 in_h, uint32 out_w, uint32 out_h,
                                     uint8_t larger);

void scale2_from_jpeg_config_for_msi2(struct scale_config *config);
struct jpg_decode_hdl
{
    // 通用结构体
    DECODE_COMMON_HDL;

    // 私有结构
};

/***********************************************jpgdecode的特有函数************************************************************* */
static void jpg_decode_channel_free_data(void *ptr, void *priv)
{
    if (ptr)
    {
        decode_mem_free(ptr);
    }
}

static int32_t jpg_lock(void *decode, void *hd)
{
    uint8_t last_value = 0;
    int32_t ret        = jpg_mutex_lock(JPGID1, JPG_LOCK_DECODE, &last_value);
    return ret;
}
static int32_t jpg_unlock(void *de, void *hd)
{
    struct jpg_decode_hdl *hdl = hd;
    int32_t                ret = RET_ERR;
    jpg_close((struct jpg_device *) hdl->decode_dev);
    ret = jpg_mutex_unlock(JPGID1, JPG_LOCK_DECODE);
    return ret;
}

static int32_t get_jpg_decode_size(void *de, void *hd)
{
    struct decode_msi_s       *decode = (struct decode_msi_s *) de;
    struct jpg_decode_hdl     *hdl    = (struct jpg_decode_hdl *) decode->current_hdl;
    struct vcodec_decode_req  *req;
    struct vcodec_decode_info *decode_info;

    req          = hdl->req;
    decode_info  = &req->info;
    // 计算解码后的空间
    uint32_t t_w = decode_info->target_width ? decode_info->target_width : hdl->w;
    uint32_t t_h = decode_info->target_height ? decode_info->target_height : hdl->h;

    hdl->dw = t_w;
    hdl->dh = t_h;

    int32_t need_size = DE_Y_SIZE(t_w, t_h) + DE_UV_SIZE(t_w, t_h) + DE_UV_SIZE(t_w, t_h);
    os_printf("need_size:%X\n", need_size);
    return need_size;
}

static int32_t jpg_decode_ready(void *de, void *hd)
{
    struct decode_msi_s      *decode = (struct decode_msi_s *) de;
    struct jpg_decode_hdl    *hdl    = (struct jpg_decode_hdl *) decode->current_hdl;
    struct vcodec_decode_req *req    = hdl->req;
    uint32_t                  dst    = (uint32_t) hdl->decode_addr;
    uint32_t                  y      = (uint32_t) decode->scaler2buf_y;
    uint32_t                  u      = (uint32_t) decode->scaler2buf_u;
    uint32_t                  v      = (uint32_t) decode->scaler2buf_v;
    uint16_t                  iw     = hdl->w;
    uint16_t                  ih     = hdl->h;
    uint16_t                  ow     = hdl->dw;
    uint16_t                  oh     = hdl->dh;

    // 计算y u v的偏移
    uint32_t y_off = dst;
    uint32_t u_off = dst + DE_U_OFF(ow, oh);
    uint32_t v_off = dst + DE_U_OFF(ow, oh) + DE_V_OFF(ow, oh);
    if (req)
    {
        // 设置好yoff uoff voff
        req->info.actual_width  = hdl->dw;
        req->info.actual_height = hdl->dh;
        req->info.y_off         = (uint8_t*)y_off;
        req->info.u_off         = (uint8_t*)u_off;
        req->info.v_off         = (uint8_t*)v_off;
    }

    // jpg_open(decode->jpg_dev);
    // 将输入输出的值打印出来
    // os_printf("JPG w:%d\th:%d\tdw:%d\tdh:%d\n", iw, ih, ow, oh);
    struct scale_config config;
    config.y_off      = y_off;
    config.u_off      = u_off;
    config.v_off      = v_off;
    config.y_off_sram = y;
    config.u_off_sram = u;
    config.v_off_sram = v;
    config.in_w       = iw;
    config.in_h       = ih;
    config.out_w      = ow;
    config.out_h      = oh;
    config.larger     = 10;
    config.scale_dev  = decode->scale_dev;

    scale2_from_jpeg_config_for_msi2(&config);
    scale_request_irq(decode->scale_dev, FRAME_END, general_scale2_done, (uint32) decode);
    scale_request_irq(decode->scale_dev, INBUF_OV, general_scale2_ov, (uint32) decode);
    jpg_request_irq((struct jpg_device *) hdl->decode_dev, general_decode_err, JPG_IRQ_FLAG_ERROR, (void *) decode);
    jpg_request_irq((struct jpg_device *) hdl->decode_dev, general_decode_done, JPG_IRQ_FLAG_JPG_DONE, (void *) decode);
    jpg_decode_target((struct jpg_device *) hdl->decode_dev, 1);

    return 0;
}

static int32_t jpg_decode_kick(void *de, void *hd)
{
    struct decode_msi_s      *decode = (struct decode_msi_s *) de;
    struct jpg_decode_hdl    *hdl    = (struct jpg_decode_hdl *) decode->current_hdl;
    struct vcodec_decode_req *req    = hdl->req;
    struct framebuff         *rfb    = req->fb;
    uint32_t                  dst    = (uint32_t) rfb->data;
    decode->hardware_ready           = 0;
    decode->last_decode_time         = os_jiffies();
    // scale_open(decode->scale_dev);
    sys_dcache_clean_range((uint32_t *) dst, rfb->len);
    jpg_decode_photo((struct jpg_device *) hdl->decode_dev, dst, rfb->len);
    return RET_OK;
}

static int32_t jpg_pre_decode(void *de, void *hd)
{
    int32_t                   ret    = RET_OK;
    struct decode_msi_s      *decode = (struct decode_msi_s *) de;
    struct jpg_decode_hdl    *hdl    = (struct jpg_decode_hdl *) decode->current_hdl;
    struct vcodec_decode_req *req    = hdl->req;
    if (req)
    {
        txVideoInfo_t *info;
        info   = (txVideoInfo_t *) req->fb->codec_info;
        hdl->w = info->width;
        hdl->h = info->height;
    }
    return ret;
}

static int32_t jpg_decode_done(void *de, void *hd)
{
    int32_t ret = RET_OK;
    return ret;
}

static int32_t jpg_decode_free_hdl(void *de, void *hd)
{
    struct jpg_decode_hdl *hdl = hd;
    os_event_del(&hdl->evt);
    STREAM_LIBC_FREE(hdl);
    return 0;
}

const struct decode_fn jpg_decode_fn = {
        .free            = jpg_decode_channel_free_data,
        .lock            = jpg_lock,
        .unlock          = jpg_unlock,
        .get_decode_size = get_jpg_decode_size,
        .decode_ready    = jpg_decode_ready,
        .decode_kick     = jpg_decode_kick,
        .pre_decode      = jpg_pre_decode,
        .decode_done     = jpg_decode_done,
        .decode_free_hdl = jpg_decode_free_hdl,
};

static int32_t register_jpg_decode_channel(struct msi *m)
{
    struct decode_msi_s      *decode = (struct decode_msi_s *) m->priv;
    int32_t                   ret    = 1;
    struct common_decode_hdl *hdl    = (struct common_decode_hdl *) STREAM_LIBC_ZALLOC(sizeof(struct common_decode_hdl));

    if (hdl)
    {
        os_event_init(&hdl->evt);
        os_event_set(&hdl->evt, DECODE_CHAN_EMPTY, NULL);
        hdl->decode_dev = (void *) dev_get(HG_JPG1_DEVID);
        // 注册释放的函数
        hdl->fn         = &jpg_decode_fn;
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

static void *jpg_decode_open(struct vcodec_device *dev)
{
    struct vcodec_device_decode *jpg_dev = (struct vcodec_device_decode *) dev;
    struct msi                  *m       = decode_core_msi(DECODE_CORE_NAME, jpg_dev->max_buf_num);
    struct common_decode_hdl    *hdl     = NULL;
    hdl                                  = (struct common_decode_hdl *) register_jpg_decode_channel(m);
    if (!hdl)
    {
        msi_destroy(m);
    }
    return (void *) hdl;
}

static int32 jpg_decode_close(struct vcodec_device *dev, void *chan)
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

static int32 jpg_decode(struct vcodec_device *dev, struct vcodec_decode_req *req)
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

static int32 jpg_ioctl(struct vcodec_device *dev, void *chan, enum vcodec_ioctl_cmd cmd, uint32 param)
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
            os_printf("%s:%d fail,cmd:%X\n", __FUNCTION__, __LINE__, cmd);
            break;
    }
    return 0;
}

static const struct vcodec_hal_ops jpg_hal_ops = {
        .open   = jpg_decode_open,
        .close  = jpg_decode_close,
        .decode = jpg_decode,
        .ioctl  = jpg_ioctl,
};

/**
 * @brief               注册jpg解码接口
 * @param dev_id        attatch的id
 * @param max_num       解码最大缓冲区数量(内存不够或者达到最大缓冲区都不会解码),注意与h264是共用的
 * @return              是否attach成功
 */
int32 decode_jpg_attach(uint32 dev_id, uint32_t max_num)
{

    struct vcodec_device_decode *jpg_dev = (struct vcodec_device_decode *) os_zalloc(sizeof(struct vcodec_device_decode));
    if (!jpg_dev)
    {
        return -ENOMEM;
    }
    struct vcodec_device *dev = &jpg_dev->dev;
    dev->dev.ops              = (const struct devobj_ops *) &jpg_hal_ops;
    jpg_dev->max_buf_num      = max_num;
    return dev_register(dev_id, (struct dev_obj *) dev);
}