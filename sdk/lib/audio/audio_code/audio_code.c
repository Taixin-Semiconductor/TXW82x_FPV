#include "basic_include.h"
#include "lib/audio/audio_code/audio_code.h"

const struct acodec_hal_ops mp3dec_hal_ops = {
    .open   = mp3_decoder_open,
    .close  = mp3_decoder_close,
    .decode = mp3_decode_frame,
};

const struct acodec_hal_ops opusdec_hal_ops = {
    .open   = opus_decoder_open,
    .close  = opus_decoder_close,
    .decode = opus_decode_frame,
};

const struct acodec_hal_ops opusenc_hal_ops = {
    .open   = opus_encoder_open,
    .close  = opus_encoder_close,
    .encode = opus_encode_frame,
    .ioctl  = opus_encoder_ctrl,
};

const struct acodec_hal_ops aacdec_hal_ops = {
    .open   = aac_decoder_open,
    .close  = aac_decoder_close,
    .decode = aac_decode_frame,
};

const struct acodec_hal_ops aacenc_hal_ops = {
    .open   = aac_encoder_open,
    .close  = aac_encoder_close,
    .encode = aac_encode_frame,
};

const struct acodec_hal_ops amrnbdec_hal_ops = {
    .open   = amrnb_decoder_open,
    .close  = amrnb_decoder_close,
    .decode = amrnb_decode_frame,
};

const struct acodec_hal_ops amrwbdec_hal_ops = {
    .open   = amrwb_decoder_open,
    .close  = amrwb_decoder_close,
    .decode = amrwb_decode_frame,
};

const struct acodec_hal_ops alawdec_hal_ops = {
    .open   = alaw_decoder_open,
    .close  = alaw_decoder_close,
    .decode = alaw_decode_frame,
};

const struct acodec_hal_ops alawenc_hal_ops = {
    .open   = alaw_encoder_open,
    .close  = alaw_encoder_close,
    .encode = alaw_encode_frame,
};

const struct acodec_hal_ops ulawdec_hal_ops = {
    .open   = ulaw_decoder_open,
    .close  = ulaw_decoder_close,
    .decode = ulaw_decode_frame,
};

const struct acodec_hal_ops ulawenc_hal_ops = {
    .open   = ulaw_encoder_open,
    .close  = ulaw_encoder_close,
    .encode = ulaw_encode_frame,
};

const struct acodec_hal_ops pcmdec_hal_ops = {
    .open   = pcm_decoder_open,
    .close  = pcm_decoder_close,
    .decode = pcm_decode_frame,
};

int32 hgacodec_v1_attach(uint32 dev_id, struct hgacodec_v1 *acodec)
{
    acodec->dev.dev.ops        = (const struct devobj_ops *)acodec->ops;
    dev_register(dev_id, (struct dev_obj *)acodec);
    
    return RET_OK;
}
