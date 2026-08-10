#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "vdec_wkq.h"
#include "vdec_msi.h"

#define MJPEG_DECODER_MAX (4)

struct mjpeg_decoder_msi {
    struct video_dec_msi dec;
    // 可以扩展私有信息
};

static const char *mjpegdec_names[MJPEG_DECODER_MAX] = {
    "mjpegdec#1", "mjpegdec#2", "mjpegdec#3", "mjpegdec#4",
};

static struct mjpeg_decoder_msi *mjpeg_decoders[MJPEG_DECODER_MAX];

static struct video_dec_mgr g_mjpegdec = {
    .chans      = (struct video_dec_msi **)mjpeg_decoders,
    .chan_names = mjpegdec_names,
    .msi_size   = sizeof(struct mjpeg_decoder_msi),
    .chan_cnt   = MJPEG_DECODER_MAX,
    .plyQ_size  = 2, 
};

int mjpeg_dec_msi_init(void)
{
    uint8 inited = 0;
    struct vcodec_device *dev = (struct vcodec_device *)dev_get(HG_JPEG_DEC_DEVID);

    ASSERT(dev);
    g_mjpegdec.msi = msi_new(MJPEGDEC_MSI, 0, &inited);
    if (g_mjpegdec.msi == NULL) {
        vdec_err("mjpeg_decoder: no memory!\r\n");
        return -ENOMEM;
    }

    if (inited) {
        g_mjpegdec.dev           = dev;
        g_mjpegdec.msi->priv     = &g_mjpegdec;
        g_mjpegdec.msi->action   = (msi_action)vdec_mgr_action;
        g_mjpegdec.msi->type     = MEDIA_DATA_VIDEO << 8 | VIDEO_CODEC_MJPEG;
        g_mjpegdec.msi->chan_mgr = 1;
        g_mjpegdec.msi->enable   = 1;
    }
    return RET_OK;
}
