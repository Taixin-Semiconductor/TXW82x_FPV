#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "../audio_coder.h"
#include "adec_msi.h"
#include "aenc_msi.h"

#define ULAW_DECODER_MAX (4)
#define ULAW_ENCODER_MAX (4)

struct ulaw_decoder_msi {
    struct audio_dec_msi dec;
    //可以扩展私有信息
};
struct ulaw_encoder_msi {
    struct audio_enc_msi enc;
    //可以扩展私有信息
};

#if ULAW_DEC_CTRL
static const char *ulawdec_names[ULAW_DECODER_MAX] = {
    "ulawdec#1", "ulawdec#2", "ulawdec#3", "ulawdec#4",
};

static struct audio_dec_msi *ulaw_decoders[ULAW_DECODER_MAX];

static struct audio_dec_mgr g_ulawdec = {
    .chans      = ulaw_decoders,
    .chan_names = ulawdec_names,
    .msi_size   = sizeof(struct ulaw_decoder_msi),
    .chan_cnt   = ULAW_DECODER_MAX,
    .plyQ_size  = 8,
};
#endif

#if ULAW_ENC_CTRL
static const char *ulawenc_default_names[ULAW_ENCODER_MAX] = {
    "ulawenc#1", "ulawenc#2", "ulawenc#3", "ulawenc#4",
};

static const char *ulawenc_names[ULAW_ENCODER_MAX] = {
    [enc_name_1] = NULL,
    [enc_name_2] = NULL,
    [enc_name_3] = NULL,
    [enc_name_4] = NULL,
};

static struct audio_enc_msi *ulaw_encoders[ULAW_ENCODER_MAX];

static struct audio_enc_mgr g_ulawenc = {
    .chans      = ulaw_encoders,
    .chan_names = ulawenc_names,
    .chan_default_names = ulawenc_default_names,
    .msi_size   = sizeof(struct ulaw_encoder_msi),
    .chan_cnt   = ULAW_ENCODER_MAX,
    .outQ_size  = 8,
    .fb_type    = MEDIA_DATA_AUDIO << 8 | AUDIO_CODEC_ULAW,
};
#endif

int ulaw_dec_msi_init(void)
{
#if ULAW_DEC_CTRL
    uint8 inited = 0;
    struct acodec_device *dev = (struct acodec_device *)dev_get(HG_ULAW_DEC_DEVID);
    struct auchange_device *ac_dev = (struct auchange_device *)dev_get(HG_AUDIO_CHANGER_DEVID);

    ASSERT(dev);
    g_ulawdec.msi = msi_new(ULAWDEC_MSI, 0, &inited);
    if (g_ulawdec.msi == NULL) {
        adec_err("ulaw_decoder: no memory!\r\n");
        return -ENOMEM;
    }

    if (inited) {
        g_ulawdec.dev           = dev;
        g_ulawdec.ac_dev        = ac_dev;
        g_ulawdec.msi->priv     = &g_ulawdec;
        g_ulawdec.msi->action   = (msi_action)adec_mgr_action;
        g_ulawdec.msi->type     = MEDIA_DATA_AUDIO << 8 | AUDIO_CODEC_ULAW;
        g_ulawdec.msi->chan_mgr = 1;
        g_ulawdec.msi->enable   = 1;
    }
    return RET_OK;
#else
    adec_err("ulaw_decoder: no enable!\r\n");
    return -ESRCH;
#endif   
}

int ulaw_enc_msi_init(void)
{
#if ULAW_ENC_CTRL
    uint8 inited = 0;
    struct acodec_device *dev = (struct acodec_device *)dev_get(HG_ULAW_ENC_DEVID);

    ASSERT(dev);
    g_ulawenc.msi = msi_new(ULAWENC_MSI, 0, &inited);
    if (g_ulawenc.msi == NULL) {
        aenc_err("ulaw_encoder: no memory!\r\n");
        return -ENOMEM;
    }

    if (inited) {
        g_ulawenc.dev           = dev;
        g_ulawenc.msi->priv     = &g_ulawenc;
        g_ulawenc.msi->action   = (msi_action)aenc_mgr_action;
        g_ulawenc.msi->type     = MEDIA_DATA_AUDIO << 8 | AUDIO_CODEC_INVALID;
        g_ulawenc.msi->chan_mgr = 1;
        g_ulawenc.msi->enable   = 1;
    }
    return RET_OK;
#else
    aenc_err("ulaw_encoder: no enable!\r\n");
    return -ESRCH;
#endif   
}
