#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "vdec_wkq.h"
#include "vdec_msi.h"

#define H264_DECODER_MAX (8)

struct h264_decoder_msi {
    struct video_dec_msi dec;
    //可以扩展私有信息
};

static const char *h264dec_names[H264_DECODER_MAX] = {
    "h264dec#1", "h264dec#2", "h264dec#3", "h264dec#4",
};

static struct h264_decoder_msi *h264_decoders[H264_DECODER_MAX];

static struct video_dec_mgr g_h264dec = {
    .chans      = (struct video_dec_msi **)h264_decoders,
    .chan_names = h264dec_names,
    .msi_size   = sizeof(struct h264_decoder_msi),
    .chan_cnt   = H264_DECODER_MAX,
    .plyQ_size  = 2,
};

int h264_dec_msi_init(void)
{
    uint8 inited = 0;
    struct vcodec_device *dev = (struct vcodec_device *)dev_get(HG_H264_DEC_DEVID);

    ASSERT(dev);
    g_h264dec.msi = msi_new(H264DEC_MSI, 0, &inited);
    if (g_h264dec.msi == NULL) {
        vdec_err("h264_decoder: no memory!\r\n");
        return -ENOMEM;
    }

    if (inited) {
        g_h264dec.dev           = dev;
        g_h264dec.msi->priv     = &g_h264dec;
        g_h264dec.msi->action   = (msi_action)vdec_mgr_action;
        g_h264dec.msi->type     = MEDIA_DATA_VIDEO << 8 | VIDEO_CODEC_H264;
        g_h264dec.msi->chan_mgr = 1;
        g_h264dec.msi->enable   = 1;
    }
    return RET_OK;
}

