#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "../audio_coder.h"
#include "adec_msi.h"

#define MP3_DECODER_MAX (4)

struct mp3_decoder_msi {
    struct audio_dec_msi dec;
    //可以扩展私有信息
};

#if MP3_DEC_CTRL
static const char *mp3dec_names[MP3_DECODER_MAX] = {
    "mp3dec#1", "mp3dec#2", "mp3dec#3", "mp3dec#4",
};

static struct audio_dec_msi *mp3_decoders[MP3_DECODER_MAX];

static struct audio_dec_mgr g_mp3dec = {
    .chans      = mp3_decoders,
    .chan_names = mp3dec_names,
    .msi_size   = sizeof(struct mp3_decoder_msi),
    .chan_cnt   = MP3_DECODER_MAX,
    .plyQ_size  = 8,
};
#endif

int mp3_dec_msi_init(void)
{
#if MP3_DEC_CTRL
    uint8 inited = 0;
    struct acodec_device *dev = (struct acodec_device *)dev_get(HG_MP3_DEC_DEVID);
    struct auchange_device *ac_dev = (struct auchange_device *)dev_get(HG_AUDIO_CHANGER_DEVID);

    ASSERT(dev);
    g_mp3dec.msi = msi_new(MP3DEC_MSI, 0, &inited);
    if (g_mp3dec.msi == NULL) {
        adec_err("mp3_decoder: no memory!\r\n");
        return -ENOMEM;
    }

    if (inited) {
        g_mp3dec.dev           = dev;
        g_mp3dec.ac_dev        = ac_dev;
        g_mp3dec.msi->priv     = &g_mp3dec;
        g_mp3dec.msi->action   = (msi_action)adec_mgr_action;
        g_mp3dec.msi->type     = MEDIA_DATA_AUDIO << 8 | AUDIO_CODEC_MP3;
        g_mp3dec.msi->chan_mgr = 1;
        g_mp3dec.msi->enable   = 1;
    }
    return RET_OK;
#else
    adec_err("mp3_decoder: no enable!\r\n");
    return -ESRCH;
#endif 
}

