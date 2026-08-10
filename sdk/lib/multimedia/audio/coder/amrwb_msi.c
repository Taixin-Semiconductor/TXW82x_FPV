#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "../audio_coder.h"
#include "adec_msi.h"

#define AMRWB_DECODER_MAX (4)

struct amrwb_decoder_msi {
    struct audio_dec_msi dec;
    //可以扩展私有信息
};

#if AMRWB_DEC_CTRL
static const char *amrwbdec_names[AMRWB_DECODER_MAX] = {
    "amrwbdec#1", "amrwbdec#2", "amrwbdec#3", "amrwbdec#4",
};

static struct audio_dec_msi *amrwb_decoders[AMRWB_DECODER_MAX];

static struct audio_dec_mgr g_amrwbdec = {
    .chans      = amrwb_decoders,
    .chan_names = amrwbdec_names,
    .msi_size   = sizeof(struct amrwb_decoder_msi),
    .chan_cnt   = AMRWB_DECODER_MAX,
    .plyQ_size  = 8,
};
#endif

int amrwb_dec_msi_init(void)
{
#if AMRWB_DEC_CTRL
    uint8 inited = 0;
    struct acodec_device *dev = (struct acodec_device *)dev_get(HG_AMRWB_DEC_DEVID);
    struct auchange_device *ac_dev = (struct auchange_device *)dev_get(HG_AUDIO_CHANGER_DEVID);

    ASSERT(dev);
    g_amrwbdec.msi = msi_new(AMRWBDEC_MSI, 0, &inited);
    if (g_amrwbdec.msi == NULL) {
        adec_err("amrwb_decoder: no memory!\r\n");
        return -ENOMEM;
    }

    if (inited) {
        g_amrwbdec.dev           = dev;
        g_amrwbdec.ac_dev        = ac_dev;
        g_amrwbdec.msi->priv     = &g_amrwbdec;
        g_amrwbdec.msi->action   = (msi_action)adec_mgr_action;
        g_amrwbdec.msi->type     = MEDIA_DATA_AUDIO << 8 | AUDIO_CODEC_AMR_WB;
        g_amrwbdec.msi->chan_mgr = 1;
        g_amrwbdec.msi->enable   = 1;
    }
    return RET_OK;
#else
    adec_err("amrwb_decoder: no enable!\r\n");
    return -ESRCH;
#endif 
}


