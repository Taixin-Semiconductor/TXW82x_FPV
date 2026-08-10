#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/multimedia/video.h"
#include "hal/vcodec.h"
#include "stream_define.h"
#include "user_work/user_work.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

#define MAX_RECV_DECODE_NUM (8)

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

enum
{
    DECODE_IDLE = BIT(0), // 通道被清空
};

typedef struct
{
    struct os_work           work; // workqueue 工作项
    struct msi              *msi;  // MSI 基类
    struct vcodec_device    *dev;  // HG_MJPEG_DEC_DEVID 设备
    void                    *chan; // 长生命周期解码通道
    struct framebuff        *decode_fb;
    uint16_t                 d_w, d_h;
    uint8_t                  mtype, stype;
    struct os_event          evt;
    struct vcodec_decode_req req; // 解码请求
} jpg_decode_s;

static void jpg_decode_done(struct vcodec_decode_req *req, int32 status)
{
    jpg_decode_s *jpg_decode = (jpg_decode_s *) req->priv;
    if (status == RET_OK)
    {
        msi_output_fb(jpg_decode->msi, jpg_decode->req.fb_out, 0);
    }
    else
    {
        msi_delete_fb(NULL, jpg_decode->req.fb_out);
    }
    // 将fb output出去,然后设置idle
    os_event_set(&jpg_decode->evt, DECODE_IDLE, NULL);
    os_run_work(&jpg_decode->work);
}

static int32_t jpg_decode_work(struct os_work *work)
{
    struct framebuff *fb;
    int32_t           ret = RET_OK;
    int32_t           event_wait;
    jpg_decode_s     *jpg_decode = (jpg_decode_s *) work;

    event_wait = os_event_wait(&jpg_decode->evt, DECODE_IDLE, NULL, OS_EVENT_WMODE_OR, 0);
    // 如果空闲,则尝试去获取解码的图片数据
    if (event_wait == RET_OK)
    {
        if (jpg_decode->decode_fb)
        {
            fb = jpg_decode->decode_fb;
        }
        else
        {
            fb = msi_get_fb(jpg_decode->msi, 0);
        }

        if (fb)
        {
            // 申请output后的fb
            struct framebuff *plyfb = msi_alloc_fb(jpg_decode->msi, NULL, NULL, 0, sizeof(txYuvInfo_t), 0);
            if (plyfb)
            {
                struct vcodec_decode_req *req     = &jpg_decode->req;
                txYuvInfo_t              *yuvinfo = (txYuvInfo_t *) plyfb->codec_info;
                req->priv                         = (void *) jpg_decode;
                req->chan                         = jpg_decode->chan;
                req->fb                           = fb;
                req->fb_out                       = plyfb;
                plyfb->mtype                      = F_YUV;
                plyfb->stype                      = jpg_decode->stype;
                plyfb->time                       = fb->time;
                req->done                         = jpg_decode_done;
                req->info.target_width            = jpg_decode->d_w;
                req->info.target_height           = jpg_decode->d_h;
                yuvinfo->width                    = jpg_decode->d_w;
                yuvinfo->height                   = jpg_decode->d_h;
                yuvinfo->x                        = ~0;
                yuvinfo->y                        = ~0;
                jpg_decode->decode_fb             = NULL;
                os_event_wait(&jpg_decode->evt, DECODE_IDLE, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
                if (vcodec_decode(jpg_decode->dev, &jpg_decode->req))
                {
                    jpg_decode->req.fb     = NULL;
                    jpg_decode->req.fb_out = NULL;
                    msi_delete_fb(NULL, plyfb);
                    os_event_set(&jpg_decode->evt, DECODE_IDLE, NULL);
                }
                msi_delete_fb(NULL, fb);
            }
            else
            {
                jpg_decode->decode_fb = fb;
            }
        }
    }

    os_run_work_delay(work, 1);
    return ret;
};

static int32 jpg_decode_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int           ret        = RET_OK;
    jpg_decode_s *jpg_decode = (jpg_decode_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            vcodec_close(jpg_decode->dev, jpg_decode->chan);
            os_event_del(&jpg_decode->evt);
            STREAM_FREE(jpg_decode);
        }
        break;
        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&jpg_decode->work, 1);
            if (jpg_decode->decode_fb)
            {
                msi_delete_fb(NULL, jpg_decode->decode_fb);
                jpg_decode->decode_fb = NULL;
            }
        }
        break;
        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            if (fb)
            {
                vcodec_ioctl(jpg_decode->dev, jpg_decode->chan, VDEC_IOCTL_RELEASE_FBDATA, (uint32) fb->data);
            }
            break;
        }
        case MSI_CMD_TRANS_FB_END:
        {
            os_run_work(&jpg_decode->work);
        }
        break;

        default:
            break;
    }
    return ret;
}

struct msi *jpg_decode_msi(const char *name)
{
    return NULL;
}

// 测试代码,后续会被删除
struct msi *new_jpg_decode_msi(const char *name, uint16_t dw, uint16_t dh)
{
    int32_t       err        = RET_ERR;
    struct msi   *msi        = NULL;
    uint8_t       isnew      = 0;
    jpg_decode_s *jpg_decode = NULL;
    msi                      = msi_new(name, MAX_RECV_DECODE_NUM, &isnew);
    if (msi && isnew)
    {
        jpg_decode = (jpg_decode_s *) STREAM_ZALLOC(sizeof(jpg_decode_s));
        if (!jpg_decode)
        {
            goto jpg_decode_end;
        }

        jpg_decode->dev = (struct vcodec_device *) dev_get(HG_JPEG_DEC_DEVID);
        if (!jpg_decode->dev)
        {
            goto jpg_decode_end;
        }
        jpg_decode->chan = vcodec_open(jpg_decode->dev);
        if (!jpg_decode->chan)
        {
            goto jpg_decode_end;
        }
        jpg_decode->d_w = dw;
        jpg_decode->d_h = dh;
        jpg_decode->msi = msi;
        os_event_init(&jpg_decode->evt);
        os_event_set(&jpg_decode->evt, DECODE_IDLE, NULL);
        msi->priv              = (void *) jpg_decode;
        msi->action            = jpg_decode_action;
        msi->fb_limits.counter = MAX_RECV_DECODE_NUM;
        msi->type              = F_JPG << 8 | 0xff;
        msi->enable            = 1;
        OS_WORK_INIT(&jpg_decode->work, jpg_decode_work, 0);
        err = RET_OK;
    }

jpg_decode_end:
    if (err)
    {
        os_printf(KERN_ERR "%s new err\n", __FUNCTION__);
        if (jpg_decode)
        {
            STREAM_FREE(jpg_decode);
        }
        if (msi)
        {
            msi_destroy(msi);
            msi = NULL;
        }
    }
    return msi;
}