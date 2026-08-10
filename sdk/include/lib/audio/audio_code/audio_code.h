#ifndef _AUDIO_CODE_H_
#define _AUDIO_CODE_H_

#include "typesdef.h"
#include "hal/acodec.h"
#include "lib/multimedia/audio.h"
#include "audio_code_core.h"

extern const struct acodec_hal_ops mp3dec_hal_ops;
extern const struct acodec_hal_ops opusdec_hal_ops;
extern const struct acodec_hal_ops aacdec_hal_ops;
extern const struct acodec_hal_ops amrnbdec_hal_ops;
extern const struct acodec_hal_ops amrwbdec_hal_ops;
extern const struct acodec_hal_ops alawdec_hal_ops;
extern const struct acodec_hal_ops ulawdec_hal_ops;
extern const struct acodec_hal_ops pcmdec_hal_ops;

extern const struct acodec_hal_ops opusenc_hal_ops;
extern const struct acodec_hal_ops aacenc_hal_ops;
extern const struct acodec_hal_ops alawenc_hal_ops;
extern const struct acodec_hal_ops ulawenc_hal_ops;

struct hgacodec_v1 {
    struct acodec_device   dev;
    const struct acodec_hal_ops *ops;   ///< 硬件抽象层操作接口表.
};

int32 hgacodec_v1_attach(uint32 dev_id, struct hgacodec_v1 *acodec);
int32 audio_coder_module_init();
int32 audio_coder_module_deinit();

void *mp3_decoder_open(struct acodec_device *dec, txAudioInfo_t *info);
int32 mp3_decoder_close(struct acodec_device *dec, void *hdl);
int32 mp3_decode_frame(struct acodec_device *dec, struct acodec_decode_req *req);

void *opus_decoder_open(struct acodec_device *dec, txAudioInfo_t *info);
int32 opus_decoder_close(struct acodec_device *dec, void *hdl);
int32 opus_decode_frame(struct acodec_device *dec, struct acodec_decode_req *req);
void *opus_encoder_open(struct acodec_device *dec, txAudioInfo_t *info);
int32 opus_encoder_close(struct acodec_device *dec, void *hdl);
int32 opus_encode_frame(struct acodec_device *dec, struct acodec_encode_req *req);
int32 opus_encoder_ctrl(struct acodec_device *dec, void *hdl, enum acodec_ioctl_cmd cmd, uint32 param);

void *aac_decoder_open(struct acodec_device *dec, txAudioInfo_t *info);
int32 aac_decoder_close(struct acodec_device *dec, void *hdl);
int32 aac_decode_frame(struct acodec_device *dec, struct acodec_decode_req *req);
void *aac_encoder_open(struct acodec_device *dec, txAudioInfo_t *info);
int32 aac_encoder_close(struct acodec_device *dec, void *hdl);
int32 aac_encode_frame(struct acodec_device *dec, struct acodec_encode_req *req);

void *amrnb_decoder_open(struct acodec_device *dec, txAudioInfo_t *info);
int32 amrnb_decoder_close(struct acodec_device *dec, void *hdl);
int32 amrnb_decode_frame(struct acodec_device *dec, struct acodec_decode_req *req);

void *amrwb_decoder_open(struct acodec_device *dec, txAudioInfo_t *info);
int32 amrwb_decoder_close(struct acodec_device *dec, void *hdl);
int32 amrwb_decode_frame(struct acodec_device *dec, struct acodec_decode_req *req);

void *alaw_decoder_open(struct acodec_device *dec, txAudioInfo_t *info);
int32 alaw_decoder_close(struct acodec_device *dec, void *hdl);
int32 alaw_decode_frame(struct acodec_device *dec, struct acodec_decode_req *req);
void *alaw_encoder_open(struct acodec_device *dec, txAudioInfo_t *info);
int32 alaw_encoder_close(struct acodec_device *dec, void *hdl);
int32 alaw_encode_frame(struct acodec_device *dec, struct acodec_encode_req *req);

void *ulaw_decoder_open(struct acodec_device *dec, txAudioInfo_t *info);
int32 ulaw_decoder_close(struct acodec_device *dec, void *hdl);
int32 ulaw_decode_frame(struct acodec_device *dec, struct acodec_decode_req *req);
void *ulaw_encoder_open(struct acodec_device *dec, txAudioInfo_t *info);
int32 ulaw_encoder_close(struct acodec_device *dec, void *hdl);
int32 ulaw_encode_frame(struct acodec_device *dec, struct acodec_encode_req *req);

void *pcm_decoder_open(struct acodec_device *dec, txAudioInfo_t *info);
int32 pcm_decoder_close(struct acodec_device *dec, void *hdl);
int32 pcm_decode_frame(struct acodec_device *dec, struct acodec_decode_req *req);

#endif
