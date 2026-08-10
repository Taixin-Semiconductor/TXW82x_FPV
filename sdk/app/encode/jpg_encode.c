#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/multimedia/video.h"
#include "hal/vcodec.h"
#include "stream_define.h"
#include "user_work/user_work.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

#define MAX_RECV_ENCODE_NUM (8)

#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

enum
{
    ENCODE_IDLE = BIT(0),
};

typedef struct
{
    struct os_work           work;
    struct msi              *msi;
    struct vcodec_device    *dev;
    void                    *chan;
    struct framebuff        *encode_fb;
    struct os_event          evt;
    struct vcodec_encode_req req;
} jpg_encode_s;

static void jpg_encode_done(struct vcodec_encode_req *req, int32 status)
{
    jpg_encode_s *jpg_encode = (jpg_encode_s *) req->priv;
    if (status == RET_OK)
    {
        msi_output_fb(jpg_encode->msi, jpg_encode->req.fb_out, 0);
    }
    else
    {
        msi_delete_fb(NULL, jpg_encode->req.fb_out);
    }
    os_event_set(&jpg_encode->evt, ENCODE_IDLE, NULL);
    os_run_work(&jpg_encode->work);
}

static int32_t yuv_to_jpg_encode_work(struct os_work *work)
{
    struct framebuff *fb;
    int32_t           ret = RET_OK;
    int32_t           event_wait;
    jpg_encode_s     *jpg_encode = (jpg_encode_s *) work;

    event_wait = os_event_wait(&jpg_encode->evt, ENCODE_IDLE, NULL, OS_EVENT_WMODE_OR, 0);
    if (event_wait == RET_OK)
    {
        if (jpg_encode->encode_fb)
        {
            fb = jpg_encode->encode_fb;
        }
        else
        {
            fb = msi_get_fb(jpg_encode->msi, 0);
        }

        if (fb)
        {
            struct framebuff *plyfb = msi_alloc_fb(jpg_encode->msi, NULL, NULL, 0, sizeof(txVideoInfo_t), 0);
            if (plyfb)
            {
                struct vcodec_encode_req *req = &jpg_encode->req;
                req->priv                     = (void *) jpg_encode;
                req->chan                     = jpg_encode->chan;
                req->fb                       = fb;
                req->fb_out                   = plyfb;
                req->done                     = jpg_encode_done;
                plyfb->mtype                  = F_JPG;
                plyfb->stype                  = fb->stype;
                plyfb->time                   = fb->time;
                jpg_encode->encode_fb         = NULL;
                os_event_wait(&jpg_encode->evt, ENCODE_IDLE, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
                if (vcodec_encode(jpg_encode->dev, &jpg_encode->req))
                {
                    jpg_encode->req.fb     = NULL;
                    jpg_encode->req.fb_out = NULL;
                    msi_delete_fb(NULL, plyfb);
                    os_event_set(&jpg_encode->evt, ENCODE_IDLE, NULL);
                }
                msi_delete_fb(NULL, fb);
            }
            else
            {
                jpg_encode->encode_fb = fb;
            }
        }
    }
    else
    {
        os_run_work_delay(work, 1);
    }
    return ret;
}

static int32 jpg_encode_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    int           ret        = RET_OK;
    jpg_encode_s *jpg_encode = (jpg_encode_s *) msi->priv;
    switch (cmd_id)
    {
        case MSI_CMD_POST_DESTROY:
        {
            vcodec_close(jpg_encode->dev, jpg_encode->chan);
            os_event_del(&jpg_encode->evt);
            STREAM_FREE(jpg_encode);
        }
        break;
        case MSI_CMD_PRE_DESTROY:
        {
            os_work_cancle2(&jpg_encode->work, 1);
            if (jpg_encode->encode_fb)
            {
                msi_delete_fb(NULL, jpg_encode->encode_fb);
                jpg_encode->encode_fb = NULL;
            }
        }
        break;
        case MSI_CMD_FREE_FB:
        {
            struct framebuff *fb = (struct framebuff *) param1;
            if (fb)
            {
                vcodec_ioctl(jpg_encode->dev, jpg_encode->chan, VDEC_IOCTL_RELEASE_FBDATA, (uint32) fb->data);
            }
            break;
        }
        case MSI_CMD_TRANS_FB_END:
        {
            os_run_work(&jpg_encode->work);
        }
        break;
        default:
            break;
    }
    return ret;
}

struct msi *new_jpg_encode_msi(const char *name)
{
    int32_t       err        = RET_ERR;
    struct msi   *msi        = NULL;
    uint8_t       isnew      = 0;
    jpg_encode_s *jpg_encode = NULL;
    msi                      = msi_new(name, MAX_RECV_ENCODE_NUM, &isnew);
    if (msi && isnew)
    {
        jpg_encode = (jpg_encode_s *) STREAM_ZALLOC(sizeof(jpg_encode_s));
        if (!jpg_encode)
        {
            goto new_jpg_encode_msi_end;
        }

        jpg_encode->dev = (struct vcodec_device *) dev_get(HG_JPEG_ENC_DEVID);
        if (!jpg_encode->dev)
        {
            goto new_jpg_encode_msi_end;
        }
        jpg_encode->chan = vcodec_open(jpg_encode->dev);
        if (!jpg_encode->chan)
        {
            goto new_jpg_encode_msi_end;
        }
        jpg_encode->msi = msi;
        os_event_init(&jpg_encode->evt);
        os_event_set(&jpg_encode->evt, ENCODE_IDLE, NULL);
        msi->priv              = (void *) jpg_encode;
        msi->action            = jpg_encode_action;
        msi->fb_limits.counter = MAX_RECV_ENCODE_NUM;
        msi->type              = F_YUV << 8 | 0xff;
        msi->enable            = 1;
        OS_WORK_INIT(&jpg_encode->work, yuv_to_jpg_encode_work, 0);
        err = RET_OK;
    }

new_jpg_encode_msi_end:
    if (err)
    {
        os_printf(KERN_ERR "%s new err\n", __FUNCTION__);
        if (jpg_encode)
        {
            STREAM_FREE(jpg_encode);
        }
        if (msi)
        {
            msi_destroy(msi);
            msi = NULL;
        }
    }
    return msi;
}
