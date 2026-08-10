#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "../audio_coder.h"
#include "adec_msi.h"

#define PCM_DECODER_MAX (4)

struct pcm_decoder_msi {
    struct audio_dec_msi dec;
};

#if PCM_DEC_CTRL
static const char *pcmdec_names[PCM_DECODER_MAX] = {
    "pcmdec#1", "pcmdec#2", "pcmdec#3", "pcmdec#4",
};

static struct audio_dec_msi *pcm_decoders[PCM_DECODER_MAX];

static struct audio_dec_mgr g_pcmdec = {
    .chans      = pcm_decoders,
    .chan_names = pcmdec_names,
    .msi_size   = sizeof(struct pcm_decoder_msi),
    .chan_cnt   = PCM_DECODER_MAX,
    .plyQ_size  = 16,
};
#endif

int pcm_dec_msi_init(void)
{
#if PCM_DEC_CTRL
    uint8 inited = 0;
    struct acodec_device *dev = (struct acodec_device *)dev_get(HG_PCM_DEC_DEVID);
    struct auchange_device *ac_dev = (struct auchange_device *)dev_get(HG_AUDIO_CHANGER_DEVID);

    ASSERT(dev);
    g_pcmdec.msi = msi_new(PCMDEC_MSI, 0, &inited);
    if (g_pcmdec.msi == NULL) {
        adec_err("pcm_decoder: no memory!\r\n");
        return -ENOMEM;
    }

    if (inited) {
        g_pcmdec.dev           = dev;
        g_pcmdec.ac_dev        = ac_dev;
        g_pcmdec.msi->priv     = &g_pcmdec;
        g_pcmdec.msi->action   = (msi_action)adec_mgr_action;
        g_pcmdec.msi->type     = MEDIA_DATA_AUDIO << 8 | AUDIO_CODEC_PCM_S16LE;
        g_pcmdec.msi->chan_mgr = 1;
        g_pcmdec.msi->enable   = 1;
    }
    return RET_OK;
#else
    adec_err("pcm_decoder: no enable!\r\n");
    return -ESRCH;
#endif 
}

