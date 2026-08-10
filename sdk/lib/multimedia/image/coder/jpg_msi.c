#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/multimedia/video.h"
#include "hal/vcodec.h"
#include "hal/vdd.h"
#include "stream_define.h"
#include "video/coder/vdec_wkq.h"
#include "lib/lcd/lcd.h"

struct jpg_msi {
    struct msi *msi;            // MSI 基类
    struct vcodec_device *dev;  // HG_MJPEG_DEC_DEVID 设备
    void *chan;                 // 长生命周期解码通道
    struct os_work work;        // workqueue 工作项
    struct os_mutex lock;       // 互斥锁
    struct vdd_rect   vdd;
    uint8_t decoding;           // 1: 正在解码中
    int32_t decode_status;      // 解码状态
    uint16_t disp_x;            // 当前帧显示 X 偏移
    uint16_t disp_y;            // 当前帧显示 Y 偏移
    struct vcodec_decode_req req; // 解码请求
};

static struct jpg_msi *g_jpg_msi;

static int32_t jpg_msi_work(struct os_work *work);

static void jpg_msi_calc_fit_size(uint16_t img_w, uint16_t img_h,
                                  uint16_t *out_w, uint16_t *out_h,
                                  uint16_t *out_x, uint16_t *out_y)
{
#if 0
     uint16_t scr_w = lcdstruct.video_w ? lcdstruct.video_w : 800;
     uint16_t scr_h = lcdstruct.video_h ? lcdstruct.video_h : 480;
     uint32_t disp_w = img_w;
     uint32_t disp_h = img_h;

     if (disp_w > scr_w || disp_h > scr_h) {
         uint32_t scale_w = (scr_w * 256) / disp_w;
         uint32_t scale_h = (scr_h * 256) / disp_h;
         uint32_t scale = (scale_w < scale_h) ? scale_w : scale_h;
         disp_w = (disp_w * scale) / 256;
         disp_h = (disp_h * scale) / 256;
     }

     disp_w = (disp_w + 7) & ~7;
     disp_h = (disp_h + 7) & ~7;

     if (disp_w < 8) disp_w = 8;
     if (disp_h < 8) disp_h = 8;

     *out_w = (uint16_t)disp_w;
     *out_h = (uint16_t)disp_h;
    *out_x = (scr_w > *out_w) ? ((scr_w - *out_w) / 2) : 0;
	*out_y = (scr_h > *out_h) ? ((scr_h - *out_h) / 2) : 0;
#endif
}

static void jpg_msi_decode_done(struct vcodec_decode_req *req, int32 status)
{
    struct jpg_msi *jpg = (struct jpg_msi *)req->priv;
    jpg->decode_status = status;
    vdec_work_run(&jpg->work);
}

static int32_t jpg_msi_work(struct os_work *work)
{
    struct jpg_msi *jpg = container_of(work, struct jpg_msi, work);
    os_mutex_lock(&jpg->lock, osWaitForever);

    if (jpg->decoding && jpg->decode_status != 1) {
        if (jpg->decode_status == RET_OK && jpg->req.fb_out && jpg->req.fb_out->len) {
            txYuvInfo_t *YUVinfo = (txYuvInfo_t *)jpg->req.fb_out->codec_info;
            os_memset(YUVinfo, 0, sizeof(txYuvInfo_t));
            YUVinfo->width = jpg->req.info.actual_width;
            YUVinfo->height = jpg->req.info.actual_height;
            YUVinfo->x = jpg->vdd.x;
            YUVinfo->y = jpg->vdd.y;
            YUVinfo->y_off = jpg->req.info.y_off;
            YUVinfo->u_off = jpg->req.info.u_off;
            YUVinfo->v_off = jpg->req.info.v_off;
            jpg->req.fb_out->time = jpg->req.fb->time;
            jpg->req.fb_out->keyfrm = 1;
            jpg->req.fb_out->last = jpg->req.fb->last;
            jpg->req.fb_out->mtype = F_YUV;
            jpg->req.fb_out->stype = FSTYPE_YUV_P0;
            msi_output_fb(jpg->msi, jpg->req.fb_out, 0);
        } else {
            fb_put(jpg->req.fb_out);
        }

        fb_put(jpg->req.fb);
        jpg->req.fb = NULL;
        jpg->req.fb_out = NULL;
        jpg->decoding = 0;
    }

    if (!jpg->decoding) {
        struct framebuff *fb = msi_get_fb(jpg->msi, 0);
        if (fb) {
            struct framebuff *plyfb = msi_alloc_fb(jpg->msi, NULL, NULL, 0, sizeof(txYuvInfo_t), 0);
            if (!plyfb) {
                fbq_enqueue(&jpg->msi->fbQ, fb, 0);
                vdec_work_delay_run(&jpg->work, 5);
            } else {
                os_memset(&jpg->vdd, 0, sizeof(jpg->vdd));
                os_memset(&jpg->req.info, 0, sizeof(jpg->req.info));
                msi_cmd2(jpg->msi, MSI_CMD_GET_VDD_RECT, (uint32)(&jpg->vdd), 0);

                jpg->req.priv = jpg;
                jpg->req.chan = jpg->chan;
                jpg->req.fb = fb;
                jpg->req.fb_out = plyfb;
                jpg->req.done = jpg_msi_decode_done;

                txVideoDecInfo_t *info = (txVideoDecInfo_t *)fb->priv;
                if(info){
                    jpg->req.info.target_height = info->height;
                    jpg->req.info.target_width  = info->width;
                }else{
                    jpg->req.info.target_height = jpg->vdd.height;
                    jpg->req.info.target_width  = jpg->vdd.width;
                }

                jpg->disp_x = 0;
                jpg->disp_y = 0;
                jpg->decoding = 1;
                jpg->decode_status = 1;
                if (vcodec_decode(jpg->dev, &jpg->req)) {
                    jpg->decoding = 0;
                    jpg->decode_status = 0;
                    jpg->req.fb = NULL;
                    jpg->req.fb_out = NULL;
                    fb_put(fb);
                    fb_put(plyfb);
                }
            }
        }
    }

    os_mutex_unlock(&jpg->lock);
    return 0;
}

static int32 jpg_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2)
{
    struct jpg_msi *jpg = (struct jpg_msi *)msi->priv;
    struct framebuff *fb;
    (void)param2;

    switch (cmd_id) {
        case MSI_CMD_TRANS_FB:
            break;
        case MSI_CMD_TRANS_FB_END:
            if (!jpg->decoding) {
                vdec_work_run(&jpg->work);
            }
            break;
        case MSI_CMD_FREE_FB:
            fb = (struct framebuff *)param1;
            if (fb) {
                vcodec_ioctl(jpg->dev, jpg->chan,
                             VDEC_IOCTL_RELEASE_FBDATA, (uint32)fb->data);
            }
            break;
        case MSI_CMD_START:
            vdec_work_run(&jpg->work);
            break;
        case MSI_CMD_STOP:
            os_work_cancle(&jpg->work, 1);
            vcodec_ioctl(jpg->dev, jpg->chan,
                         VDEC_IOCTL_CANCLE, (uint32)&jpg->req);
            while (jpg->decode_status == 1) {
                os_sleep_ms(5);
            }
            msi_clear(jpg->msi);
            if (jpg->req.fb) {
                fb_put(jpg->req.fb);
                jpg->req.fb = NULL;
            }
            if (jpg->req.fb_out) {
                fb_put(jpg->req.fb_out);
                jpg->req.fb_out = NULL;
            }
            jpg->decoding = 0;
            jpg->decode_status = 0;
            break;
        case MSI_CMD_POST_DESTROY:
            if (jpg->chan) {
                vcodec_close(jpg->dev, jpg->chan);
                jpg->chan = NULL;
            }
            os_mutex_del(&jpg->lock);
            decoder_mem_free(jpg);
            g_jpg_msi = NULL;
            break;
        default:
            break;
    }
    return RET_OK;
}

int jpg_dec_msi_init(void)
{
    struct vcodec_device *dev = (struct vcodec_device *)dev_get(HG_JPEG_DEC_DEVID);
    if (!dev) {
        return -ENODEV;
    }

    struct msi *msi = msi_new("jpgdec", 4, NULL);
    if (!msi) {
        return -ENOMEM;
    }

    struct jpg_msi *jpg = (struct jpg_msi *)decoder_mem_zalloc(sizeof(*jpg));
    if (!jpg) {
        msi_destroy(msi);
        return -ENOMEM;
    }

    jpg->msi = msi;
    jpg->dev = dev;
    jpg->chan = vcodec_open(dev);
    if (!jpg->chan) {
        decoder_mem_free(jpg);
        msi_destroy(msi);
        return -ENOMEM;
    }

    msi->priv = jpg;
    msi->action = jpg_msi_action;
    msi->type = MEDIA_DATA_PICTURE << 8 | PICTURE_FORMAT_JPEG;
    msi->enable = 1;
    msi->fb_limits.counter = 2;
    msi->fb_alloc = (malloc_cb_t)decoder_mem_alloc;
    msi->fb_free = (mfree_cb_t)decoder_mem_free;

    OS_WORK_INIT(&jpg->work, jpg_msi_work, 0);
    os_mutex_init(&jpg->lock);

    g_jpg_msi = jpg;
    return RET_OK;
}
