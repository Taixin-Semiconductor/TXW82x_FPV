#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "../audio_coder.h"
#include "adec_msi.h"
#include "aenc_msi.h"

#define AAC_DECODER_MAX (4)
#define AAC_ENCODER_MAX (4)

#ifndef AAC_OUTQ_SIZE
#define AAC_OUTQ_SIZE    8
#endif

struct aac_decoder_msi {
    struct audio_dec_msi dec;
    //可以扩展私有信息
};
struct aac_encoder_msi {
    struct audio_enc_msi enc;
    //可以扩展私有信息
};

#if AAC_DEC_CTRL
static const char *aacdec_names[AAC_DECODER_MAX] = {
    "aacdec#1", "aacdec#2", "aacdec#3", "aacdec#4",
};

static struct audio_dec_msi *aac_decoders[AAC_DECODER_MAX];

static struct audio_dec_mgr g_aacdec = {
    .chans      = aac_decoders,
    .chan_names = aacdec_names,
    .msi_size   = sizeof(struct aac_decoder_msi),
    .chan_cnt   = AAC_DECODER_MAX,
    .plyQ_size  = 8,
};
#endif

#if AAC_ENC_CTRL
static const char *aacenc_default_names[AAC_DECODER_MAX] = {
    "aacenc#1", "aacenc#2", "aacenc#3", "aacenc#4",
};

static const char *aacenc_names[AAC_ENCODER_MAX] = {
    [enc_name_1] = NULL, 
    [enc_name_2] = NULL, 
    [enc_name_3] = NULL, 
    [enc_name_4] = NULL,
};

static struct audio_enc_msi *aac_encoders[AAC_ENCODER_MAX];

static struct audio_enc_mgr g_aacenc = {
    .chans      = aac_encoders,
    .chan_names = aacenc_names,
    .chan_default_names = aacenc_default_names,
    .msi_size   = sizeof(struct aac_encoder_msi),
    .chan_cnt   = AAC_ENCODER_MAX,
    .outQ_size  = AAC_OUTQ_SIZE,
    .fb_type    = MEDIA_DATA_AUDIO << 8 | AUDIO_CODEC_AAC,
};
#endif

int aac_dec_msi_init(void)
{
#if AAC_DEC_CTRL
    uint8 inited = 0;
    struct acodec_device *dev = (struct acodec_device *)dev_get(HG_AAC_DEC_DEVID);
    struct auchange_device *ac_dev = (struct auchange_device *)dev_get(HG_AUDIO_CHANGER_DEVID);

    ASSERT(dev);
    g_aacdec.msi = msi_new(AACDEC_MSI, 0, &inited);
    if (g_aacdec.msi == NULL) {
        adec_err("aac_decoder: no memory!\r\n");
        return -ENOMEM;
    }

    if (inited) {
        g_aacdec.dev           = dev;
        g_aacdec.ac_dev        = ac_dev;
        g_aacdec.msi->priv     = &g_aacdec;
        g_aacdec.msi->action   = (msi_action)adec_mgr_action;
        g_aacdec.msi->type     = MEDIA_DATA_AUDIO << 8 | AUDIO_CODEC_AAC;
        g_aacdec.msi->chan_mgr = 1;
        g_aacdec.msi->enable   = 1;
    }
    return RET_OK;
#else
    adec_err("aac_decoder: no enable!\r\n");
    return -ESRCH;
#endif   
}

int aac_enc_msi_init(void)
{
#if AAC_ENC_CTRL
    uint8 inited = 0;
    struct acodec_device *dev = (struct acodec_device *)dev_get(HG_AAC_ENC_DEVID);

    ASSERT(dev);
    g_aacenc.msi = msi_new(AACENC_MSI, 0, &inited);
    if (g_aacenc.msi == NULL) {
        aenc_err("aac_encoder: no memory!\r\n");
        return -ENOMEM;
    }

    if (inited) {
        g_aacenc.dev           = dev;
        g_aacenc.msi->priv     = &g_aacenc;
        g_aacenc.msi->action   = (msi_action)aenc_mgr_action;
        g_aacenc.msi->type     = MEDIA_DATA_AUDIO << 8 | AUDIO_CODEC_INVALID;
        g_aacenc.msi->chan_mgr = 1;
        g_aacenc.msi->enable   = 1;
    }
    return RET_OK;
#else
    adec_err("aac_encoder: no enable!\r\n");
    return -ESRCH;
#endif   
}

