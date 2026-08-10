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
#include "gen420_encode_core.h"

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

/***********************************************jpgdecode的特有函数************************************************************* */
static void jpg_encode_channel_free_data(void *ptr, void *priv)
{
    if (ptr)
    {
        STREAM_FREE(ptr);
    }
    return;
}

static int32_t jpg_lock(void *encode, void *hd)
{
    int32_t              ret;
    struct gen420_hdl_s *hdl = hd;
    uint8_t              last_lock_value;
    ret = jpg_mutex_lock(hdl->which_jpg, JPG_LOCK_GEN420_ENCODE, &last_lock_value);
    if (last_lock_value != JPG_LOCK_GEN420_ENCODE)
    {
        hdl->belong = 0;
    }
    else
    {
        hdl->belong = 1;
    }
    return ret;
}
static int32_t jpg_unlock(void *en, void *hd)
{
    int32_t              ret = RET_ERR;
    struct gen420_hdl_s *hdl = hd;
    ret                      = jpg_mutex_unlock(hdl->which_jpg, JPG_LOCK_GEN420_ENCODE);
    return ret;
}

static int32_t get_jpg_encode_size(void *en, void *hd)
{
    return 0;
}

static int32_t jpg_encode_ready(void *en, void *hd)
{
    return 0;
}

static int32_t jpg_encode_kick(void *en, void *hd)
{
    return RET_OK;
}

static int32_t jpg_pre_encode(void *en, void *hd)
{
    int32_t                   ret = RET_OK;
    struct gen420_hdl_s      *hdl = hd;
    struct vcodec_encode_req *req = hdl->req;
    if (req)
    {
        txYuvInfo_t *yuv_info = (txYuvInfo_t *) req->fb->codec_info;
        hdl->w                = yuv_info->width;
        hdl->h                = yuv_info->height;
    }
    return ret;
}

// 接收完成,需要将fb组成一张图
static int32_t jpg_encode_done(void *en, void *hd)
{
    struct gen420_core_s     *encode = (struct gen420_core_s *) en;
    struct gen420_hdl_s      *hdl    = hd;
    struct vcodec_encode_req *req    = hdl->req;
    uint32_t                  jpg_len;
    int32_t                   ret             = RET_OK;
    uint8_t                  *jpg_psram_space = NULL;
    struct framebuff         *fb              = encode->recv_fb;
    struct framebuff         *fb_out          = req->fb_out;
    struct jpg_node_s        *jpg_priv        = (struct jpg_node_s *) fb->priv;
    if (jpg_priv)
    {
        jpg_len         = jpg_priv->jpg_len;
        jpg_psram_space = (uint8_t *) STREAM_MALLOC(jpg_len);
    }

    if (!jpg_psram_space)
    {
        ret = RET_ERR;
    }
    else
    {
        sys_dcache_invalid_range((uint32_t *) jpg_psram_space, jpg_len);
        uint32_t          offset     = 0;
        uint32_t          remain_len = jpg_len;
        uint32_t          cp_len     = 0;
        struct framebuff *tmp_fb     = fb;
        // 开始拷贝数据
        while (tmp_fb && remain_len)
        {
            if (remain_len > tmp_fb->len)
            {
                cp_len = tmp_fb->len;
            }
            else
            {
                cp_len = remain_len;
            }
            hw_memcpy_no_cache(jpg_psram_space + offset, tmp_fb->data, cp_len);
            offset += cp_len;
            remain_len -= cp_len;
            tmp_fb = tmp_fb->next;
        }

        // 赋值到fb_out
        fb_out->data = jpg_psram_space;
        fb_out->len  = jpg_len;

        txVideoInfo_t *info = (txVideoInfo_t *) fb_out->codec_info;
        info->width         = hdl->w;
        info->height        = hdl->h;
    }

    msi_delete_fb(NULL, fb);

    return ret;
}

static int32_t jpg_encode_free_hdl(void *en, void *hd)
{
    struct gen420_hdl_s *hdl = hd;
    os_event_del(&hdl->evt);
    STREAM_LIBC_FREE(hdl);
    return 0;
}

static const struct encode_fn jpg_encode_fn = {
        .free            = jpg_encode_channel_free_data,
        .lock            = jpg_lock,
        .unlock          = jpg_unlock,
        .get_encode_size = get_jpg_encode_size,
        .encode_ready    = jpg_encode_ready,
        .encode_kick     = jpg_encode_kick,
        .pre_encode      = jpg_pre_encode,
        .encode_done     = jpg_encode_done,
        .encode_free_hdl = jpg_encode_free_hdl,
};

static int32_t register_chan(struct msi *m)
{
    struct gen420_core_s *encode = (struct gen420_core_s *) m->priv;
    int32_t               ret    = 1;
    struct gen420_hdl_s  *hdl    = (struct gen420_hdl_s *) STREAM_LIBC_ZALLOC(sizeof(struct gen420_hdl_s));

    if (hdl)
    {
        os_event_init(&hdl->evt);
        os_event_set(&hdl->evt, ENCODE_CHAN_EMPTY, NULL);
        hdl->msi       = m;
        hdl->fn        = &jpg_encode_fn;
        hdl->which_jpg = 1;
        uint32_t flag  = disable_irq();
        ret            = get_gen420_core_free_chan(encode, hdl);
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

static void *jpg_encode_open(struct vcodec_device *dev)
{
    // struct vcodec_device_encode *jpg_dev = (struct vcodec_device_encode *) dev;
    struct msi          *m   = gen420_encode_core("_encode");
    struct gen420_hdl_s *hdl = NULL;
    hdl                      = (struct gen420_hdl_s *) register_chan(m);
    if (!hdl)
    {
        msi_destroy(m);
    }
    return (void *) hdl;
}

static int32 jpg_encode_close(struct vcodec_device *dev, void *chan)
{
    struct gen420_hdl_s  *hdl    = (struct gen420_hdl_s *) chan;
    struct msi           *m      = hdl->msi;
    struct gen420_core_s *encode = (struct gen420_core_s *) m->priv;
    os_printf("%s:%d chan:%d\n", __FUNCTION__, __LINE__, hdl->chan);
    // 设置通道关闭状态
    hdl->closed = 1;

    // 通知workqueue去释放通道
    encode->gc = 1;

    os_run_work(&encode->work);
    os_event_wait(&hdl->evt, ENCODE_CHAN_EMPTY | ENCODE_CHAN_DESTROY, NULL, OS_EVENT_WMODE_AND, -1);
    hdl->fn->encode_free_hdl(encode, hdl);
    // 等待workqueue去停止解码或者msi退出后退出
    msi_destroy(m);
    return 0;
}
static int32 jpg_encode(struct vcodec_device *dev, struct vcodec_encode_req *req)
{

    int32                 ret    = RET_ERR;
    struct gen420_hdl_s  *hdl    = (struct gen420_hdl_s *) req->chan;
    struct msi           *m      = hdl->msi;
    struct gen420_core_s *encode = (struct gen420_core_s *) m->priv;

    ret = os_event_wait(&hdl->evt, ENCODE_CHAN_EMPTY, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
    if (!ret)
    {
        fb_get(req->fb);
        hdl->req = req;
        os_run_work(&encode->work);
    }
    else
    {
        os_printf("ret:%d\taddr:%X\n", ret, RETURN_ADDR());
    }
    return ret;
}

static int32 jpg_ioctl(struct vcodec_device *v, void *chan, enum vcodec_ioctl_cmd cmd, uint32 param)
{
    switch (cmd)
    {
        case VDEC_IOCTL_RELEASE_FBDATA:
        {
            struct gen420_hdl_s  *hdl       = (struct gen420_hdl_s *) chan;
            struct gen420_core_s *encode    = hdl->encode;
            uint8_t              *free_data = (uint8_t *) param;
            hdl->fn->free(free_data, encode);
        }
        break;
        case VDEC_IOCTL_CANCLE:
        {
            struct gen420_hdl_s  *hdl    = (struct gen420_hdl_s *) chan;
            struct msi           *m      = hdl->msi;
            struct gen420_core_s *encode = (struct gen420_core_s *) m->priv;
            // 通知workqueue去释放通道
            encode->gc                   = 1;
            hdl->gc                      = 1;
        }
        break;
        default:
            os_printf("%s:%d fail,cmd:%X\n", __FUNCTION__, __LINE__, cmd);
            break;
    }
    return 0;
}

static const struct vcodec_hal_ops jpg_hal_ops = {
        .open   = jpg_encode_open,
        .close  = jpg_encode_close,
        .encode = jpg_encode,
        .ioctl  = jpg_ioctl,
};

/**
 * @brief               注册jpg解码接口
 * @param dev_id        attatch的id
 * @param max_num       解码最大缓冲区数量(内存不够或者达到最大缓冲区都不会解码),注意与h264是共用的
 * @return              是否attach成功
 */
int32 encode_jpg_attach(uint32 dev_id, uint32_t max_num)
{

    struct vcodec_device_encode *jpg_dev = (struct vcodec_device_encode *) os_zalloc(sizeof(struct vcodec_device_encode));
    if (!jpg_dev)
    {
        return -ENOMEM;
    }
    struct vcodec_device *dev = &jpg_dev->dev;
    dev->dev.ops              = (const struct devobj_ops *) &jpg_hal_ops;
    jpg_dev->max_buf_num      = max_num;
    return dev_register(dev_id, (struct dev_obj *) dev);
}
