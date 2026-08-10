#ifndef _AUDIO_CODE_CORE_H_
#define _AUDIO_CODE_CORE_H_

#include "typesdef.h"
#include "osal/string.h"
#include "osal/sleep.h"
#include "lib/common/rbuffer.h"
#include "osal/semaphore.h"
#include "osal/event.h"

#define AUCODER_NO_RUN                 0
#define AUCODER_RUN_IN_CPU0            1
#define AUCODER_RUN_IN_CPU1            2

#ifndef AAC_ENC_CTRL
#define AAC_ENC_CTRL                   AUCODER_NO_RUN
#endif
#ifndef AAC_DEC_CTRL
#define AAC_DEC_CTRL                   AUCODER_NO_RUN
#endif
#ifndef AMRNB_DEC_CTRL
#define AMRNB_DEC_CTRL                 AUCODER_NO_RUN
#endif
#ifndef AMRWB_DEC_CTRL
#define AMRWB_DEC_CTRL                 AUCODER_NO_RUN
#endif
#ifndef MP3_DEC_CTRL
#define MP3_DEC_CTRL                   AUCODER_NO_RUN
#endif
#ifndef ALAW_ENC_CTRL
#define ALAW_ENC_CTRL                  AUCODER_NO_RUN
#endif
#ifndef ALAW_DEC_CTRL
#define ALAW_DEC_CTRL                  AUCODER_NO_RUN
#endif
#ifndef ULAW_ENC_CTRL
#define ULAW_ENC_CTRL                  AUCODER_NO_RUN
#endif
#ifndef ULAW_DEC_CTRL
#define ULAW_DEC_CTRL                  AUCODER_NO_RUN
#endif
#ifndef OPUS_ENC_CTRL
#define OPUS_ENC_CTRL                  AUCODER_NO_RUN
#endif
#ifndef OPUS_DEC_CTRL
#define OPUS_DEC_CTRL                  AUCODER_NO_RUN
#endif
#ifndef PCM_DEC_CTRL
#define PCM_DEC_CTRL                   AUCODER_NO_RUN
#endif

#define AUCODE_INFO                    os_printf

#define aucode_memcpy                  os_memcpy
#define aucode_memset                  os_memset
#define aucode_memmove                 os_memmove

#define AUCODE_OK                      0
#define AUCODE_ERR                     -1

enum {
    codec_finish = 1,
    codec_rpc_stop,
    codec_rpc_exit,
};

enum {
    DECODER_PLC = 1,
    DECODER_EOS,
    DECODER_FLUSH,
};

enum {
    encoder_set_bitrate,
};

typedef int32 (*audio_codec_func)(void *hdl);
typedef int32 (*coder_close_func)(void *hdl);
typedef int32 (*coder_ctrl_func)(void *hdl, int32 cmd, int32 param);
typedef int32 (*RB_READ)(struct rbuffer *rb, void *data, uint32 length);
typedef int32 (*RB_WRITE)(struct rbuffer *rb, void *buff, uint32 size);
typedef void  (*MEM_FREE)(void *ptr);

typedef struct {
    uint16 sample_rate;
    uint16  channels;
} AUCODEC_INFO;

typedef struct {
    void     (*done)(void *req, int32 status);
    uint8     *data;
    void     *outbuf;
    uint16   data_len;
    uint16   outbuf_len;
    uint16   actual_sample_rate;
    uint16   target_sample_rate;  
    uint8    actual_channels; 
    uint8    target_channels;           
    uint16    flags; 
    void     *req_priv;  
    int16    *codec_buf; 
    uint32   codec_buf_size;             
} AUDECODE_REQ;

typedef struct {
    void     (*done)(void *req, int32 status);
    void     *data;
    void     *outbuf;
    uint16   data_len;
    uint16   outbuf_len; 
    uint16   actual_sample_rate;
    uint16   target_sample_rate;  
    uint8    actual_channels; 
    uint8    target_channels;
    int16    target_bitrate;
    void     *req_priv;
    int16    *encode_buf;
    uint16   encode_buf_size;
    uint16   encode_data_size;
    uint16   encode_data_offset;
    uint16   frame_size;
    uint32    need_more;
    uint8    *codec_buf;
} AUENCODE_REQ;

typedef struct {
    uint32 state;
    void *task_hdl;
    uint8 *req_buf;
    void *memfree_buf;
    struct rbuffer req_rbuf;
    struct rbuffer memfree_rbuf;
    RB_READ rb_read;
    RB_WRITE rb_write;
    MEM_FREE mem_free;
    struct os_event finish_event;
    struct os_semaphore code_sema;
} AUCODE_MANAGE_RPC;

typedef struct {
    uint8 codec_rpc;
    uint8 state;
    int16 frame_size;
    audio_codec_func codec_func;
    audio_codec_func codec_func_rpc;
    coder_close_func close_func;
    coder_ctrl_func  ctrl_func;
    void *req;
    void *coder;
    void *res_hdl;
    AUCODE_MANAGE_RPC *aucode_manage_rpc;
} AUCODER_HDL;

extern AUCODE_MANAGE_RPC *g_aucode_manage_rpc;

void *aucoder_alloc(uint32 size);
void aucoder_free(void *ptr);

int32 audio_coder_run(AUCODE_MANAGE_RPC *aucode_manage);
int32 audio_coder_run_rpc(AUCODE_MANAGE_RPC *aucode_manage);

int32 audio_code_frame(AUCODE_MANAGE_RPC *aucode_manage);
int32 audio_code_frame_rpc(AUCODE_MANAGE_RPC *aucode_manage);

int32 audio_code_finish(AUCODE_MANAGE_RPC *aucode_manage);
int32 audio_code_finish_rpc(AUCODE_MANAGE_RPC *aucode_manage);
int32 audio_code_wait_finish_rpc(AUCODE_MANAGE_RPC *aucode_manage);

void *mp3_decoder_init(AUCODEC_INFO *info, uint8 codec_rpc);
void *opus_decoder_init(AUCODEC_INFO *info, uint8 codec_rpc);
void *opus_encoder_init(AUCODEC_INFO *info, uint8 codec_rpc);
void *aac_decoder_init(AUCODEC_INFO *info, uint8 codec_rpc);
void *aac_encoder_init(AUCODEC_INFO *info, uint8 codec_rpc);
void *amrnb_decoder_init(AUCODEC_INFO *info, uint8 codec_rpc);
void *amrwb_decoder_init(AUCODEC_INFO *info, uint8 codec_rpc);
void *alaw_decoder_init(AUCODEC_INFO *info, uint8 codec_rpc);
void *alaw_encoder_init(AUCODEC_INFO *info, uint8 codec_rpc);
void *ulaw_decoder_init(AUCODEC_INFO *info, uint8 codec_rpc);
void *ulaw_encoder_init(AUCODEC_INFO *info, uint8 codec_rpc);
void *pcm_decoder_init(AUCODEC_INFO *info, uint8 codec_rpc);

#endif