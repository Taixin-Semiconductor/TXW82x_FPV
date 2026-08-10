#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "../audio_coder.h"
#include "adec_msi.h"
#include "aenc_msi.h"

#define ALAW_DECODER_MAX (4)
#define ALAW_ENCODER_MAX (4)

struct alaw_decoder_msi {
    struct audio_dec_msi dec;
    //可以扩展私有信息
};
struct alaw_encoder_msi {
    struct audio_enc_msi enc;
    //可以扩展私有信息
};

#if ALAW_DEC_CTRL
static const char *alawdec_names[ALAW_DECODER_MAX] = {
    "alawdec#1", "alawdec#2", "alawdec#3", "alawdec#4",
};

static struct audio_dec_msi *alaw_decoders[ALAW_DECODER_MAX];

static struct audio_dec_mgr g_alawdec = {
    .chans      = alaw_decoders,
    .chan_names = alawdec_names,
    .msi_size   = sizeof(struct alaw_decoder_msi),
    .chan_cnt   = ALAW_DECODER_MAX,
    .plyQ_size  = 8,
};
#endif

#if ALAW_ENC_CTRL
static const char *alawenc_default_names[ALAW_ENCODER_MAX] = {
    "alawenc#1", "alawenc#2", "alawenc#3", "alawenc#4",
};

static const char *alawenc_names[ALAW_ENCODER_MAX] = {
    [enc_name_1] = NULL,
    [enc_name_2] = NULL,
    [enc_name_3] = NULL,
    [enc_name_4] = NULL,
};

static struct audio_enc_msi *alaw_encoders[ALAW_ENCODER_MAX];

static struct audio_enc_mgr g_alawenc = {
    .chans      = alaw_encoders,
    .chan_names = alawenc_names,
    .chan_default_names = alawenc_default_names,
    .msi_size   = sizeof(struct alaw_encoder_msi),
    .chan_cnt   = ALAW_ENCODER_MAX,
    .outQ_size  = 8,
    .fb_type    = MEDIA_DATA_AUDIO << 8 | AUDIO_CODEC_ALAW,
};
#endif

int alaw_dec_msi_init(void)
{
#if ALAW_DEC_CTRL
    uint8 inited = 0;
    struct acodec_device *dev = (struct acodec_device *)dev_get(HG_ALAW_DEC_DEVID);
    struct auchange_device *ac_dev = (struct auchange_device *)dev_get(HG_AUDIO_CHANGER_DEVID);

    ASSERT(dev);
    g_alawdec.msi = msi_new(ALAWDEC_MSI, 0, &inited);
    if (g_alawdec.msi == NULL) {
        adec_err("alaw_decoder: no memory!\r\n");
        return -ENOMEM;
    }

    if (inited) {
        g_alawdec.dev           = dev;
        g_alawdec.ac_dev        = ac_dev;
        g_alawdec.msi->priv     = &g_alawdec;
        g_alawdec.msi->action   = (msi_action)adec_mgr_action;
        g_alawdec.msi->type     = MEDIA_DATA_AUDIO << 8 | AUDIO_CODEC_ALAW;
        g_alawdec.msi->chan_mgr = 1;
        g_alawdec.msi->enable   = 1;
    }
    return RET_OK;
#else
    adec_err("alaw_decoder: no enable!\r\n");
    return -ESRCH;
#endif   
}

int alaw_enc_msi_init(void)
{
#if ALAW_ENC_CTRL
    uint8 inited = 0;
    struct acodec_device *dev = (struct acodec_device *)dev_get(HG_ALAW_ENC_DEVID);

    ASSERT(dev);
    g_alawenc.msi = msi_new(ALAWENC_MSI, 0, &inited);
    if (g_alawenc.msi == NULL) {
        aenc_err("alaw_encoder: no memory!\r\n");
        return -ENOMEM;
    }

    if (inited) {
        g_alawenc.dev           = dev;
        g_alawenc.msi->priv     = &g_alawenc;
        g_alawenc.msi->action   = (msi_action)aenc_mgr_action;
        g_alawenc.msi->type     = MEDIA_DATA_AUDIO << 8 | AUDIO_CODEC_INVALID;
        g_alawenc.msi->chan_mgr = 1;
        g_alawenc.msi->enable   = 1;
    }
    return RET_OK;
#else
    aenc_err("alaw_encoder: no enable!\r\n");
    return -ESRCH;
#endif   
}
