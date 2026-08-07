#ifndef _AUDIO_CODE_H_
#define _AUDIO_CODE_H_

#include "basic_include.h"

#define AUCODER_NO_RUN                 0
#define AUCODER_RUN_IN_CPU0            1
#define AUCODER_RUN_IN_CPU1            2

#define AAC_ENC_CTRL                   AUCODER_RUN_IN_CPU0
#define AAC_DEC_CTRL                   AUCODER_NO_RUN
#define AMRNB_DEC_CTRL                 AUCODER_NO_RUN
#define AMRWB_DEC_CTRL                 AUCODER_NO_RUN
#define MP3_DEC_CTRL                   AUCODER_NO_RUN
#define ALAW_ENC_CTRL                  AUCODER_NO_RUN
#define ALAW_DEC_CTRL                  AUCODER_NO_RUN
#define ULAW_ENC_CTRL                  AUCODER_NO_RUN
#define ULAW_DEC_CTRL                  AUCODER_NO_RUN
#define OPUS_ENC_CTRL                  AUCODER_NO_RUN
#define OPUS_DEC_CTRL                  AUCODER_NO_RUN

#define AUCODE_INFO                    os_printf

#define aucode_memcpy                  os_memcpy
#define aucode_memset                  os_memset
#define aucode_memmove                 os_memmove

#define AUCODE_OK                      0
#define AUCODE_ERR                     -1

#define AUCODER_NUM                    12

#if MP3_DEC_CTRL
#define AUCODE_STACK_SIZE              16384
#elif OPUS_ENC_CTRL
#define AUCODE_STACK_SIZE              10240
#elif OPUS_DEC_CTRL
#define AUCODE_STACK_SIZE              4096
#else
#define AUCODE_STACK_SIZE              0
#endif   

enum {
    AAC_ENC = 1,
    AAC_DEC,
    AMRNB_DEC,
    AMRWB_DEC,
    MP3_DEC,
    ALAW_ENC,
    ALAW_DEC,
    ULAW_ENC,
    ULAW_DEC,
    OPUS_ENC,
    OPUS_DEC,
};

enum {
    codec_start   = BIT(0),
    codec_finish  = BIT(1),
    coder_close   = BIT(2),
    coder_exit    = BIT(3),
    decode_plc    = BIT(4),
};

typedef struct {
    void *coder;
    void *priv;
    uint8_t coder_type;
    uint8_t fade_type;
    uint32_t frame_nsamples;
} AUCODE_HDL;

typedef struct {
    int32_t frame_bytes;
    uint32_t samplerate;
    uint32_t channels;
} AUCODE_FRAME_INFO;

typedef int32_t (*audio_encode_func)(void *coder, int16_t *in_data, uint32_t samples, uint8_t *out_data);
typedef int32_t (*audio_decode_func)(void *coder, uint8_t *in_data, uint32_t bytes, int16_t *out_data, AUCODE_FRAME_INFO *frame_info);
typedef int32_t (*decode_plc_func)(void *coder, int16_t *out_data, uint32_t samples);
typedef int32_t (*aucoder_close_func)(void *coder);

typedef void *(*AUCODE_MALLOC)(int size);
typedef void *(*AUCODE_ZALLOC)(int size);
typedef void *(*AUCODE_CALLOC)(int nmemb, int size);
typedef void *(*AUCODE_REALLOC)(void *ptr, int size);
typedef void (*AUCODE_FREE)(void *ptr);

typedef struct {
    void *coder;
    void *in_data;
    void *out_data;
    uint32_t nbytes;
    uint32_t nsamples;
    uint32_t coder_state;
    int32_t ret;
    AUCODE_FRAME_INFO *frame_info;
    audio_encode_func encode_func;
    audio_decode_func decode_func;
    decode_plc_func plc_func;
    aucoder_close_func close_func;
} RPC_AUCODE_STRUCT;

typedef struct {
    void *task_hdl;
    int8_t *stack_priv;
    uint32_t used_size;
    uint32_t total_size;
    uint32_t state;
    RPC_AUCODE_STRUCT *rpc_aucode_s[AUCODER_NUM];
} AUCODE_MANAGE;

extern AUCODE_MANAGE *g_aucode_manage;

extern AUCODE_MALLOC  aucode_malloc;
extern AUCODE_ZALLOC  aucode_zalloc;
extern AUCODE_CALLOC  aucode_calloc;
extern AUCODE_REALLOC aucode_realloc;
extern AUCODE_FREE    aucode_free;

extern void reg_aucoder_alloc(AUCODE_MALLOC m, AUCODE_ZALLOC z, AUCODE_CALLOC c, AUCODE_REALLOC r, AUCODE_FREE f);
extern void aucode_mutex_init(void);
extern void *aucode_stack_alloc(uint32_t size);
extern void aucode_stack_free(void *ptr);
extern AUCODE_HDL *audio_coder_open(uint32_t coder, uint32_t samplerate, uint32_t channel);
extern int32_t audio_encoder_set_bitrate(AUCODE_HDL *aucode_hdl, uint32_t bitrate);
extern int32_t audio_encode_data(AUCODE_HDL *aucode_hdl, int16_t *in_data, uint32_t samples, uint8_t *out_data);
extern int32_t audio_decode_fade_in(AUCODE_HDL *aucode_hdl);
extern int32_t audio_decode_fade_out(AUCODE_HDL *aucode_hdl);
extern int32_t audio_decode_data(AUCODE_HDL *aucode_hdl, uint8_t *in_data, uint32_t bytes, int16_t *out_data, AUCODE_FRAME_INFO *frame_info);
extern int32_t audio_decode_output_mute(AUCODE_HDL *aucode_hdl, int16_t *out_data);
extern int32_t audio_decode_do_plc(AUCODE_HDL *aucode_hdl, int16_t *out_data);
extern void audio_coder_close(AUCODE_HDL *aucode_hdl);

extern void *aac_encoder_open(uint32_t samplerate, uint32_t channel);
extern void *aac_decoder_open(void);
extern void *amrnb_decoder_open(void);
extern void *amrnb_decoder_open(void);
extern void *mp3_decoder_open(void);
extern void *alaw_encoder_open(void);
extern void *alaw_decoder_open(void);
extern void *ulaw_encoder_open(void);
extern void *ulaw_decoder_open(void);
extern void *opus_encoder_open(uint32_t samplerate, uint32_t channel);
extern void *opus_decoder_open(uint32_t samplerate, uint32_t channel);
extern int32_t opus_encoder_set_bitrate(void *coder, uint32_t bitrate);
extern int32_t aac_encode_data(void *coder, int16_t *in_data, uint32_t samples, uint8_t *out_data);
extern int32_t alaw_encode_data(void *coder, int16_t *in_data, uint32_t samples, uint8_t *out_data);
extern int32_t ulaw_encode_data(void *coder, int16_t *in_data, uint32_t samples, uint8_t *out_data);
extern int32_t opus_encode_data(void *coder, int16_t *in_data, uint32_t samples, uint8_t *out_data);
extern int32_t aac_decode_data(void *coder, uint8_t *in_data, uint32_t bytes, int16_t *out_data, AUCODE_FRAME_INFO *frame_info);
extern int32_t amrnb_decode_data(void *coder, uint8_t *in_data, uint32_t bytes, int16_t *out_data, AUCODE_FRAME_INFO *frame_info);
extern int32_t amrwb_decode_data(void *coder, uint8_t *in_data, uint32_t bytes, int16_t *out_data, AUCODE_FRAME_INFO *frame_info);
extern int32_t mp3_decode_data(void *coder, uint8_t *in_data, uint32_t bytes, int16_t *out_data, AUCODE_FRAME_INFO *frame_info);
extern int32_t alaw_decode_data(void *coder, uint8_t *in_data, uint32_t bytes, int16_t *out_data, AUCODE_FRAME_INFO *frame_info);
extern int32_t ulaw_decode_data(void *coder, uint8_t *in_data, uint32_t bytes, int16_t *out_data, AUCODE_FRAME_INFO *frame_info);
extern int32_t opus_decode_data(void *coder, uint8_t *in_data, uint32_t bytes, int16_t *out_data, AUCODE_FRAME_INFO *frame_info);
extern int32_t opus_decode_plc(void *coder, int16_t *out_data, uint32_t samples);
extern int32_t aac_encoder_close(void *coder);
extern int32_t aac_decoder_close(void *coder);
extern int32_t amrnb_decoder_close(void *coder);
extern int32_t amrwb_decoder_close(void *coder);
extern int32_t mp3_decoder_close(void *coder);
extern int32_t opus_encoder_close(void *coder);
extern int32_t opus_decoder_close(void *coder);

extern void *aac_encoder_open_rpc(uint32_t samplerate, uint32_t channel);
extern void *aac_decoder_open_rpc(void);
extern void *amrnb_decoder_open_rpc(void);
extern void *amrwb_decoder_open_rpc(void);
extern void *mp3_decoder_open_rpc(void);
extern void *alaw_encoder_open_rpc(void);
extern void *alaw_decoder_open_rpc(void);
extern void *ulaw_encoder_open_rpc(void);
extern void *ulaw_decoder_open_rpc(void);
extern void *opus_encoder_open_rpc(uint32_t samplerate, uint32_t channel);
extern void *opus_decoder_open_rpc(uint32_t samplerate, uint32_t channel);
extern int32_t opus_encoder_set_bitrate_rpc(void *coder, uint32_t bitrate);
extern int32_t audio_encode_data_rpc(void *coder, int16_t *in_data, uint32_t samples, uint8_t *out_data, int32_t *enc_bytes);
extern int32_t audio_decode_data_rpc(void *coder, uint8_t *in_data, uint32_t bytes, int16_t *out_data, AUCODE_FRAME_INFO *frame_info, int32_t *dec_samples);
extern int32_t audio_decode_plc_rpc(void *coder, int16_t *out_data, uint32_t samples, int32_t *dec_samples);
extern int32_t audio_coder_close_rpc(void *coder);

extern int32_t audio_coder_run(AUCODE_MANAGE *aucode_manage);
extern int32_t audio_coder_run_rpc(AUCODE_MANAGE *aucode_manage);

#endif