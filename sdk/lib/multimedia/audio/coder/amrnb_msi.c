#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "../audio_coder.h"
#include "adec_msi.h"

#define AMRNB_DECODER_MAX (4)

struct amrnb_decoder_msi {
    struct audio_dec_msi dec;
    //可以扩展私有信息
};

#if AMRNB_DEC_CTRL
static const char *amrnbdec_names[AMRNB_DECODER_MAX] = {
    "amrnbdec#1", "amrnbdec#2", "amrnbdec#3", "amrnbdec#4",
};

static struct audio_dec_msi *amrnb_decoders[AMRNB_DECODER_MAX];

static struct audio_dec_mgr g_amrnbdec = {
    .chans      = amrnb_decoders,
    .chan_names = amrnbdec_names,
    .msi_size   = sizeof(struct amrnb_decoder_msi),
    .chan_cnt   = AMRNB_DECODER_MAX,
    .plyQ_size  = 8,
};
#endif

int amrnb_dec_msi_init(void)
{
#if AMRNB_DEC_CTRL
    uint8 inited = 0;
    struct acodec_device *dev = (struct acodec_device *)dev_get(HG_AMRNB_DEC_DEVID);
    struct auchange_device *ac_dev = (struct auchange_device *)dev_get(HG_AUDIO_CHANGER_DEVID);

    ASSERT(dev);
    g_amrnbdec.msi = msi_new(AMRNBDEC_MSI, 0, &inited);
    if (g_amrnbdec.msi == NULL) {
        adec_err("amrnb_decoder: no memory!\r\n");
        return -ENOMEM;
    }

    if (inited) {
        g_amrnbdec.dev           = dev;
        g_amrnbdec.ac_dev        = ac_dev;
        g_amrnbdec.msi->priv     = &g_amrnbdec;
        g_amrnbdec.msi->action   = (msi_action)adec_mgr_action;
        g_amrnbdec.msi->type     = MEDIA_DATA_AUDIO << 8 | AUDIO_CODEC_AMR_NB;
        g_amrnbdec.msi->chan_mgr = 1;
        g_amrnbdec.msi->enable   = 1;
    }
    return RET_OK;
#else
    adec_err("amrnb_decoder: no enable!\r\n");
    return -ESRCH;
#endif 
}


