#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "../audio_coder.h"
#include "adec_msi.h"
#include "aenc_msi.h"

#define OPUS_DECODER_MAX (4)
#define OPUS_ENCODER_MAX (4)

struct opus_decoder_msi {
    struct audio_dec_msi dec;
    //可以扩展私有信息
};
struct opus_encoder_msi {
    struct audio_enc_msi enc;
    //可以扩展私有信息
};

#if OPUS_DEC_CTRL
static const char *opusdec_names[OPUS_DECODER_MAX] = {
    "opusdec#1", "opusdec#2", "opusdec#3", "opusdec#4",
};

static struct audio_dec_msi *opus_decoders[OPUS_DECODER_MAX];

static struct audio_dec_mgr g_opusdec = {
    .chans      = opus_decoders,
    .chan_names = opusdec_names,
    .msi_size   = sizeof(struct opus_decoder_msi),
    .chan_cnt   = OPUS_DECODER_MAX,
    .plyQ_size  = 8,
};
#endif

#if OPUS_ENC_CTRL
static const char *opusenc_default_names[OPUS_ENCODER_MAX] = {
    "opusenc#1", "opusenc#2", "opusenc#3", "opusenc#4",
};

static const char *opusenc_names[OPUS_ENCODER_MAX] = {
    [enc_name_1] = NULL,
    [enc_name_2] = NULL,
    [enc_name_3] = NULL,
    [enc_name_4] = NULL,
};

static struct audio_enc_msi *opus_encoders[OPUS_ENCODER_MAX];

static struct audio_enc_mgr g_opusenc = {
    .chans      = opus_encoders,
    .chan_names = opusenc_names,
    .chan_default_names = opusenc_default_names,
    .msi_size   = sizeof(struct opus_encoder_msi),
    .chan_cnt   = OPUS_ENCODER_MAX,
    .outQ_size  = 8,
    .fb_type    = MEDIA_DATA_AUDIO << 8 | AUDIO_CODEC_OPUS,
};
#endif

int opus_dec_msi_init(void)
{
#if OPUS_DEC_CTRL
    uint8 inited = 0;
    struct acodec_device *dev = (struct acodec_device *)dev_get(HG_OPUS_DEC_DEVID);
    struct auchange_device *ac_dev = (struct auchange_device *)dev_get(HG_AUDIO_CHANGER_DEVID);

    ASSERT(dev);
    g_opusdec.msi = msi_new(OPUSDEC_MSI, 0, &inited);
    if (g_opusdec.msi == NULL) {
        adec_err("opus_decoder: no memory!\r\n");
        return -ENOMEM;
    }

    if (inited) {
        g_opusdec.dev           = dev;
        g_opusdec.ac_dev        = ac_dev;
        g_opusdec.msi->priv     = &g_opusdec;
        g_opusdec.msi->action   = (msi_action)adec_mgr_action;
        g_opusdec.msi->type     = MEDIA_DATA_AUDIO << 8 | AUDIO_CODEC_OPUS;
        g_opusdec.msi->chan_mgr = 1;
        g_opusdec.msi->enable   = 1;
    }
    return RET_OK;
#else
    adec_err("opus_decoder: no enable!\r\n");
    return -ESRCH;
#endif   
}

int opus_enc_msi_init(void)
{
#if OPUS_ENC_CTRL
    uint8 inited = 0;
    struct acodec_device *dev = (struct acodec_device *)dev_get(HG_OPUS_ENC_DEVID);

    ASSERT(dev);
    g_opusenc.msi = msi_new(OPUSENC_MSI, 0, &inited);
    if (g_opusenc.msi == NULL) {
        aenc_err("opus_encoder: no memory!\r\n");
        return -ENOMEM;
    }

    if (inited) {
        g_opusenc.dev           = dev;
        g_opusenc.msi->priv     = &g_opusenc;
        g_opusenc.msi->action   = (msi_action)aenc_mgr_action;
        g_opusenc.msi->type     = MEDIA_DATA_AUDIO << 8 | AUDIO_CODEC_INVALID;
        g_opusenc.msi->chan_mgr = 1;
        g_opusenc.msi->enable   = 1;
    }
    return RET_OK;
#else
    aenc_err("opus_encoder: no enable!\r\n");
    return -ESRCH;
#endif   
}

