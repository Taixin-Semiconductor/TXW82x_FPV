#include "basic_include.h"
#include "lib/audio/audio_code/audio_code_core.h"
#include "lib/audio/audio_code/audio_code.h"
#include "heap/aurpc_heap.h"


#define AURPC_MEM_DEBUG(fmt, args...)     		//os_printf(fmt, ##args)

extern void *audio_mem_alloc(uint32_t size, void *priv);
extern void audio_mem_free(void *ptr, void *priv);

typedef struct {
    malloc_cb_t mem_alloc;         ///< 自定义 Framebuff 分配接口
    mfree_cb_t  mem_free;          ///< 自定义 Framebuff 释放接口  
    void *priv; 
    void *rpc_priv; 
} AUCODE_MEM_STRUCT;

AUCODE_MEM_STRUCT g_aucode_mem_s = {
    .mem_alloc = audio_mem_alloc,
    .mem_free = audio_mem_free,
#ifndef AURPC_PSRAM_HEAP_SIZE
    .priv = &sram_heap,
    .rpc_priv = &aurpc_sram_heap,
#else
    .priv = &psram_heap,
    .rpc_priv = &aurpc_psram_heap,
#endif
};

AUCODE_MANAGE_RPC *g_aucode_manage_rpc = NULL;

void *aucoder_alloc(uint32 size)
{
    void *ptr = NULL;
    if(sysctrl_get_cpu_id() == 0) {
        ptr = g_aucode_mem_s.mem_alloc(size, g_aucode_mem_s.priv);
    }
    else {
        ptr = g_aucode_mem_s.mem_alloc(size, g_aucode_mem_s.rpc_priv);
    }
    ASSERT(ptr != NULL);
    return ptr;
}

void aucoder_free(void *ptr)
{
    if(sysctrl_get_cpu_id() == 0) {
        g_aucode_mem_s.mem_free(ptr, g_aucode_mem_s.priv);
    }
    else {
        g_aucode_mem_s.mem_free(ptr, g_aucode_mem_s.rpc_priv);
    }
}

int32 audio_code_rpc_init(void)
{
    g_aucode_manage_rpc = (AUCODE_MANAGE_RPC*)aucoder_alloc(sizeof(AUCODE_MANAGE_RPC));
    if(g_aucode_manage_rpc == NULL) {
        goto audio_codec_rpc_init_fail;
    }
	aucode_memset(g_aucode_manage_rpc, 0, sizeof(AUCODE_MANAGE_RPC));
    g_aucode_manage_rpc->req_buf = (uint8*)aucoder_alloc(sizeof(AUCODER_HDL*) * 20);
    if(g_aucode_manage_rpc->req_buf == NULL) {
        goto audio_codec_rpc_init_fail;
    }
    g_aucode_manage_rpc->memfree_buf = aucoder_alloc(sizeof(void*) * 20);
    if(g_aucode_manage_rpc->memfree_buf == NULL) {
        goto audio_codec_rpc_init_fail;
    }
    rbuffer_init(&g_aucode_manage_rpc->req_rbuf, sizeof(AUCODER_HDL*)*20, g_aucode_manage_rpc->req_buf);
    rbuffer_init(&g_aucode_manage_rpc->memfree_rbuf, sizeof(void*)*20, g_aucode_manage_rpc->memfree_buf);
    g_aucode_manage_rpc->rb_read = rbuffer_get;
    g_aucode_manage_rpc->rb_write = rbuffer_set;
    g_aucode_manage_rpc->mem_free = aucoder_free;
    os_event_init(&g_aucode_manage_rpc->finish_event);
    if(g_aucode_manage_rpc->finish_event.hdl == NULL) {
        goto audio_codec_rpc_init_fail;
    }
    audio_coder_run_rpc(g_aucode_manage_rpc);
    if(g_aucode_manage_rpc->task_hdl == NULL || g_aucode_manage_rpc->code_sema.hdl == NULL) {
        goto audio_codec_rpc_init_fail;
    }
    return AUCODE_OK;
audio_codec_rpc_init_fail:
    if(g_aucode_manage_rpc) {
        if(g_aucode_manage_rpc->req_buf) {
            aucoder_free(g_aucode_manage_rpc->req_buf);
        }
        if(g_aucode_manage_rpc->memfree_buf) {
            aucoder_free(g_aucode_manage_rpc->memfree_buf);
        }
        if(g_aucode_manage_rpc->finish_event.hdl) {
            os_event_del(&g_aucode_manage_rpc->finish_event);
        }
        if(g_aucode_manage_rpc->task_hdl) {
            g_aucode_manage_rpc->state = codec_rpc_stop;
            while(g_aucode_manage_rpc->state != codec_rpc_exit) {
                os_sleep_ms(10);
            }
        }
        aucoder_free(g_aucode_manage_rpc);
        g_aucode_manage_rpc = NULL;
    }
	return AUCODE_ERR;
}

int32 audio_code_rpc_deinit(void)
{
    if(g_aucode_manage_rpc) {
        if(g_aucode_manage_rpc->req_buf) {
            aucoder_free(g_aucode_manage_rpc->req_buf);
        }
        if(g_aucode_manage_rpc->memfree_buf) {
            aucoder_free(g_aucode_manage_rpc->memfree_buf);
        }
        if(g_aucode_manage_rpc->task_hdl) {
            g_aucode_manage_rpc->state = codec_rpc_stop;
            while(g_aucode_manage_rpc->state != codec_rpc_exit) {
                os_sleep_ms(10);
            }
        }
        aucoder_free(g_aucode_manage_rpc);
        g_aucode_manage_rpc = NULL;
    }
	return AUCODE_OK;
}

int32 audio_coder_module_init()
{
	if(audio_code_rpc_init() != AUCODE_OK) {
		return AUCODE_ERR;
	}
    return AUCODE_OK;
}

int32 audio_coder_module_deinit(void)
{
	audio_code_rpc_deinit();
	return AUCODE_OK;
}

static void audio_decode_done(void *req, int32 status)
{
    AUDECODE_REQ *audecode_req = (AUDECODE_REQ*)req;
    struct acodec_decode_req *req_priv = (struct acodec_decode_req*)audecode_req->req_priv;
    req_priv->info.actual_sample_rate = audecode_req->actual_sample_rate;
    req_priv->info.actual_channels = audecode_req->actual_channels;
    req_priv->outbuf = audecode_req->outbuf;
    req_priv->outbuf_len = audecode_req->outbuf_len;
    req_priv->done(req_priv, status);
}

static void audio_decode_bind_req(AUDECODE_REQ *audecode_req, struct acodec_decode_req *req)
{
    if(req->done) {
        audecode_req->done = audio_decode_done;
    }
    else {
        audecode_req->done = NULL;
    }
    audecode_req->target_sample_rate = req->info.target_sample_rate;
    audecode_req->target_channels = req->info.target_channels;
    audecode_req->flags = req->info.flags;
    audecode_req->data = req->fb?req->fb->data:NULL;
    audecode_req->data_len = req->fb?req->fb->len:0;
    audecode_req->outbuf = req->outbuf;
    audecode_req->outbuf_len = req->outbuf?req->outbuf_len:0;
    audecode_req->req_priv = req;
    if(req->info.flags == ADEC_FLAG_PLC) {
        audecode_req->flags = DECODER_PLC;
    }
}

static void audio_encode_done(void *req, int32 status)
{
    AUENCODE_REQ *auencode_req = (AUENCODE_REQ*)req;
    struct acodec_encode_req *req_priv = (struct acodec_encode_req*)auencode_req->req_priv;
    req_priv->outbuf = auencode_req->outbuf;
    req_priv->outbuf_len = auencode_req->outbuf_len;
    req_priv->info.need_more = auencode_req->need_more;
    req_priv->done(req_priv, status);
}

static void audio_encode_bind_req(AUENCODE_REQ *auencode_req, struct acodec_encode_req *req)
{
    if(req->done) {
        auencode_req->done = audio_encode_done;
    }
    else {
        auencode_req->done = NULL;
    }
    auencode_req->actual_sample_rate = req->info.actual_sample_rate;
    auencode_req->actual_channels = req->info.actual_channels;
    auencode_req->target_bitrate = req->info.target_bitrate;
    auencode_req->data = req->fb?req->fb->data:NULL;
    auencode_req->data_len = req->fb?req->fb->len:0;
    auencode_req->outbuf = req->outbuf;
    auencode_req->outbuf_len = req->outbuf?req->outbuf_len:0;
    auencode_req->req_priv = req;
    auencode_req->frame_size = req->info.frame_size;
}

void *mp3_decoder_open(struct acodec_device *dec, txAudioInfo_t *info)
{
	AUCODER_HDL *coder_hdl = NULL;
    AUCODEC_INFO aucodec_info;
    aucodec_info.sample_rate = info->sample_rate;
    aucodec_info.channels = info->channels;
#if MP3_DEC_CTRL == AUCODER_RUN_IN_CPU0
    coder_hdl = mp3_decoder_init(&aucodec_info, 0);
#elif MP3_DEC_CTRL == AUCODER_RUN_IN_CPU1
    coder_hdl = mp3_decoder_init(&aucodec_info, 0);
#endif
    coder_hdl->aucode_manage_rpc = g_aucode_manage_rpc;
	return coder_hdl;
}

int32 mp3_decoder_close(struct acodec_device *dec, void *hdl)
{
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)hdl;
    aucoder_hdl->close_func(aucoder_hdl);
	return AUCODE_OK;
}

int32 mp3_decode_frame(struct acodec_device *dec, struct acodec_decode_req *req)
{
    int32 ret = AUCODE_ERR;
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)req->chan;
    AUDECODE_REQ *audecode_req = (AUDECODE_REQ*)aucoder_hdl->req;
    audio_decode_bind_req(audecode_req, req);
    ret = aucoder_hdl->codec_func(aucoder_hdl); 
    if(req->done == NULL) {
        req->info.actual_sample_rate = audecode_req->actual_sample_rate;
        req->info.actual_channels = audecode_req->actual_channels;
        req->outbuf = audecode_req->outbuf;
        req->outbuf_len = audecode_req->outbuf_len;
    } 
	return ret;
}

void *opus_decoder_open(struct acodec_device *dec, txAudioInfo_t *info)
{
	AUCODER_HDL *coder_hdl = NULL;
    AUCODEC_INFO aucodec_info;
    aucodec_info.sample_rate = info->sample_rate;
    aucodec_info.channels = info->channels;
#if OPUS_DEC_CTRL == AUCODER_RUN_IN_CPU0
    coder_hdl = opus_decoder_init(&aucodec_info, 0);
#elif OPUS_DEC_CTRL == AUCODER_RUN_IN_CPU1
    coder_hdl = opus_decoder_init(&aucodec_info, 1);
#endif
    coder_hdl->aucode_manage_rpc = g_aucode_manage_rpc;
	return coder_hdl;
}

int32 opus_decoder_close(struct acodec_device *dec, void *hdl)
{
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)hdl;
    aucoder_hdl->close_func(aucoder_hdl);
	return AUCODE_OK;
}

int32 opus_decode_frame(struct acodec_device *dec,  struct acodec_decode_req *req)
{
    int32 ret = AUCODE_ERR;
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)req->chan;
    AUDECODE_REQ *audecode_req = (AUDECODE_REQ*)aucoder_hdl->req;
    audio_decode_bind_req(audecode_req, req);
    ret = aucoder_hdl->codec_func(aucoder_hdl); 
    if(req->done == NULL) {
        req->info.actual_sample_rate = audecode_req->actual_sample_rate;
        req->info.actual_channels = audecode_req->actual_channels;
        req->outbuf = audecode_req->outbuf;
        req->outbuf_len = audecode_req->outbuf_len;
    } 
	return ret;
}

void *opus_encoder_open(struct acodec_device *dec, txAudioInfo_t *info)
{
	AUCODER_HDL *coder_hdl = NULL;
    AUCODEC_INFO aucodec_info;
    aucodec_info.sample_rate = info->sample_rate;
    aucodec_info.channels = info->channels;
#if OPUS_ENC_CTRL == AUCODER_RUN_IN_CPU0
    coder_hdl = opus_encoder_init(&aucodec_info, 0);
#elif OPUS_ENC_CTRL == AUCODER_RUN_IN_CPU1
    coder_hdl = opus_encoder_init(&aucodec_info, 1);
#endif
    coder_hdl->aucode_manage_rpc = g_aucode_manage_rpc;
	return coder_hdl;
}

int32 opus_encoder_close(struct acodec_device *dec, void *hdl)
{
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)hdl;
    aucoder_hdl->close_func(aucoder_hdl);
	return AUCODE_OK;
}

int32 opus_encode_frame(struct acodec_device *dec,  struct acodec_encode_req *req)
{
    int32 ret = AUCODE_ERR;
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)req->chan;
    AUENCODE_REQ *auencode_req = (AUENCODE_REQ*)aucoder_hdl->req;
    audio_encode_bind_req(auencode_req, req);
    ret = aucoder_hdl->codec_func(aucoder_hdl);
    if(req->done == NULL) {
        req->outbuf = auencode_req->outbuf;
        req->outbuf_len = auencode_req->outbuf_len;
        req->info.need_more = auencode_req->need_more;
    }     
	return ret;
}

int32 opus_encoder_ctrl(struct acodec_device *dec, void *hdl, enum acodec_ioctl_cmd cmd, uint32 param)
{
    int32 ret = AUCODE_ERR;
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)hdl;
    switch(cmd) {
        case AENC_IOCTL_SET_BITRATE:
            aucoder_hdl->ctrl_func(aucoder_hdl, encoder_set_bitrate, param);
            ret = AUCODE_OK;
            break;
        default:
            break;
    }
    return ret;
}

void *aac_decoder_open(struct acodec_device *dec, txAudioInfo_t *info)
{
	AUCODER_HDL *coder_hdl = NULL;
    AUCODEC_INFO aucodec_info;
    aucodec_info.sample_rate = info->sample_rate;
    aucodec_info.channels = info->channels;
#if AAC_DEC_CTRL == AUCODER_RUN_IN_CPU0
    coder_hdl = aac_decoder_init(&aucodec_info, 0);
#elif AAC_DEC_CTRL == AUCODER_RUN_IN_CPU1
    coder_hdl = aac_decoder_init(&aucodec_info, 1);
#endif
    coder_hdl->aucode_manage_rpc = g_aucode_manage_rpc;
	return coder_hdl;
}

int32 aac_decoder_close(struct acodec_device *dec, void *hdl)
{
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)hdl;
    aucoder_hdl->close_func(aucoder_hdl);
	return AUCODE_OK;
}

int32 aac_decode_frame(struct acodec_device *dec,  struct acodec_decode_req *req)
{
    int32 ret = AUCODE_ERR;
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)req->chan;
    AUDECODE_REQ *audecode_req = (AUDECODE_REQ*)aucoder_hdl->req;
    audio_decode_bind_req(audecode_req, req);
    ret = aucoder_hdl->codec_func(aucoder_hdl); 
    if(req->done == NULL) {
        req->info.actual_sample_rate = audecode_req->actual_sample_rate;
        req->info.actual_channels = audecode_req->actual_channels;
        req->outbuf = audecode_req->outbuf;
        req->outbuf_len = audecode_req->outbuf_len;
    } 
	return ret;
}

void *aac_encoder_open(struct acodec_device *dec, txAudioInfo_t *info)
{
	AUCODER_HDL *coder_hdl = NULL;
    AUCODEC_INFO aucodec_info;
    aucodec_info.sample_rate = info->sample_rate;
    aucodec_info.channels = info->channels;
#if AAC_ENC_CTRL == AUCODER_RUN_IN_CPU0
    coder_hdl = aac_encoder_init(&aucodec_info, 0);
#elif AAC_ENC_CTRL == AUCODER_RUN_IN_CPU1
    coder_hdl = aac_encoder_init(&aucodec_info, 1);
#endif
    coder_hdl->aucode_manage_rpc = g_aucode_manage_rpc;
	return coder_hdl;
}

int32 aac_encoder_close(struct acodec_device *dec, void *hdl)
{
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)hdl;
    aucoder_hdl->close_func(aucoder_hdl);
	return AUCODE_OK;
}

int32 aac_encode_frame(struct acodec_device *dec,  struct acodec_encode_req *req)
{
    int32 ret = AUCODE_ERR;
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)req->chan;
    AUENCODE_REQ *auencode_req = (AUENCODE_REQ*)aucoder_hdl->req;
    audio_encode_bind_req(auencode_req, req);
    ret = aucoder_hdl->codec_func(aucoder_hdl);
    if(req->done == NULL) {
        req->outbuf = auencode_req->outbuf;
        req->outbuf_len = auencode_req->outbuf_len;
        req->info.need_more = auencode_req->need_more;
    }     
	return ret;
}

void *amrnb_decoder_open(struct acodec_device *dec, txAudioInfo_t *info)
{
	AUCODER_HDL *coder_hdl = NULL;
    AUCODEC_INFO aucodec_info;
    aucodec_info.sample_rate = info->sample_rate;
    aucodec_info.channels = info->channels;
#if AMRNB_DEC_CTRL == AUCODER_RUN_IN_CPU0
    coder_hdl = amrnb_decoder_init(&aucodec_info, 0);
#elif AMRNB_DEC_CTRL == AUCODER_RUN_IN_CPU1
    coder_hdl = amrnb_decoder_init(&aucodec_info, 1);
#endif
    coder_hdl->aucode_manage_rpc = g_aucode_manage_rpc;
	return coder_hdl;
}

int32 amrnb_decoder_close(struct acodec_device *dec, void *hdl)
{
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)hdl;
    aucoder_hdl->close_func(aucoder_hdl);
	return AUCODE_OK;
}

int32 amrnb_decode_frame(struct acodec_device *dec,  struct acodec_decode_req *req)
{
    int32 ret = AUCODE_ERR;
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)req->chan;
    AUDECODE_REQ *audecode_req = (AUDECODE_REQ*)aucoder_hdl->req;
    audio_decode_bind_req(audecode_req, req);
    ret = aucoder_hdl->codec_func(aucoder_hdl); 
    if(req->done == NULL) {
        req->info.actual_sample_rate = audecode_req->actual_sample_rate;
        req->info.actual_channels = audecode_req->actual_channels;
        req->outbuf = audecode_req->outbuf;
        req->outbuf_len = audecode_req->outbuf_len;
    } 
	return ret;
}

void *amrwb_decoder_open(struct acodec_device *dec, txAudioInfo_t *info)
{
	AUCODER_HDL *coder_hdl = NULL;
    AUCODEC_INFO aucodec_info;
    aucodec_info.sample_rate = info->sample_rate;
    aucodec_info.channels = info->channels;
#if AMRWB_DEC_CTRL == AUCODER_RUN_IN_CPU0
    coder_hdl = amrwb_decoder_init(&aucodec_info, 0);
#elif AMRWB_DEC_CTRL == AUCODER_RUN_IN_CPU1
    coder_hdl = amrwb_decoder_init(&aucodec_info, 1);
#endif
    coder_hdl->aucode_manage_rpc = g_aucode_manage_rpc;
	return coder_hdl;
}

int32 amrwb_decoder_close(struct acodec_device *dec, void *hdl)
{
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)hdl;
    aucoder_hdl->close_func(aucoder_hdl);
	return AUCODE_OK;
}

int32 amrwb_decode_frame(struct acodec_device *dec,  struct acodec_decode_req *req)
{
    int32 ret = AUCODE_ERR;
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)req->chan;
    AUDECODE_REQ *audecode_req = (AUDECODE_REQ*)aucoder_hdl->req;
    audio_decode_bind_req(audecode_req, req);
    ret = aucoder_hdl->codec_func(aucoder_hdl); 
    if(req->done == NULL) {
        req->info.actual_sample_rate = audecode_req->actual_sample_rate;
        req->info.actual_channels = audecode_req->actual_channels;
        req->outbuf = audecode_req->outbuf;
        req->outbuf_len = audecode_req->outbuf_len;
    } 
	return ret;
}

void *alaw_decoder_open(struct acodec_device *dec, txAudioInfo_t *info)
{
	AUCODER_HDL *coder_hdl = NULL;
    AUCODEC_INFO aucodec_info;
    aucodec_info.sample_rate = info->sample_rate;
    aucodec_info.channels = info->channels;
#if ALAW_DEC_CTRL == AUCODER_RUN_IN_CPU0
    coder_hdl = alaw_decoder_init(&aucodec_info, 0);
#elif ALAW_DEC_CTRL == AUCODER_RUN_IN_CPU1
    coder_hdl = alaw_decoder_init(&aucodec_info, 1);
#endif
    coder_hdl->aucode_manage_rpc = g_aucode_manage_rpc;
	return coder_hdl;
}

int32 alaw_decoder_close(struct acodec_device *dec, void *hdl)
{
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)hdl;
    aucoder_hdl->close_func(aucoder_hdl);
	return AUCODE_OK;
}

int32 alaw_decode_frame(struct acodec_device *dec, struct acodec_decode_req *req)
{
    int32 ret = AUCODE_ERR;
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)req->chan;
    AUDECODE_REQ *audecode_req = (AUDECODE_REQ*)aucoder_hdl->req;
    audio_decode_bind_req(audecode_req, req);
    ret = aucoder_hdl->codec_func(aucoder_hdl); 
    if(req->done == NULL) {
        req->info.actual_sample_rate = audecode_req->actual_sample_rate;
        req->info.actual_channels = audecode_req->actual_channels;
        req->outbuf = audecode_req->outbuf;
        req->outbuf_len = audecode_req->outbuf_len;
    } 
	return ret;
}

void *alaw_encoder_open(struct acodec_device *dec, txAudioInfo_t *info)
{
	AUCODER_HDL *coder_hdl = NULL;
    AUCODEC_INFO aucodec_info;
    aucodec_info.sample_rate = info->sample_rate;
    aucodec_info.channels = info->channels;
#if ALAW_ENC_CTRL == AUCODER_RUN_IN_CPU0
    coder_hdl = alaw_encoder_init(&aucodec_info, 0);
#elif ALAW_ENC_CTRL == AUCODER_RUN_IN_CPU1
    coder_hdl = alaw_encoder_init(&aucodec_info, 1);
#endif
    coder_hdl->aucode_manage_rpc = g_aucode_manage_rpc;
	return coder_hdl;
}

int32 alaw_encoder_close(struct acodec_device *dec, void *hdl)
{
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)hdl;
    aucoder_hdl->close_func(aucoder_hdl);
	return AUCODE_OK;
}

int32 alaw_encode_frame(struct acodec_device *dec, struct acodec_encode_req *req)
{
    int32 ret = AUCODE_ERR;
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)req->chan;
    AUENCODE_REQ *auencode_req = (AUENCODE_REQ*)aucoder_hdl->req;
    audio_encode_bind_req(auencode_req, req);
    ret = aucoder_hdl->codec_func(aucoder_hdl);
    if(req->done == NULL) {
        req->outbuf = auencode_req->outbuf;
        req->outbuf_len = auencode_req->outbuf_len;
        req->info.need_more = auencode_req->need_more;
    }     
	return ret;
}

void *ulaw_decoder_open(struct acodec_device *dec, txAudioInfo_t *info)
{
	AUCODER_HDL *coder_hdl = NULL;
    AUCODEC_INFO aucodec_info;
    aucodec_info.sample_rate = info->sample_rate;
    aucodec_info.channels = info->channels;
#if ULAW_DEC_CTRL == AUCODER_RUN_IN_CPU0
    coder_hdl = ulaw_decoder_init(&aucodec_info, 0);
#elif ULAW_DEC_CTRL == AUCODER_RUN_IN_CPU1
    coder_hdl = ulaw_decoder_init(&aucodec_info, 1);
#endif
    coder_hdl->aucode_manage_rpc = g_aucode_manage_rpc;
	return coder_hdl;
}

int32 ulaw_decoder_close(struct acodec_device *dec, void *hdl)
{
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)hdl;
    aucoder_hdl->close_func(aucoder_hdl);
	return AUCODE_OK;
}

int32 ulaw_decode_frame(struct acodec_device *dec, struct acodec_decode_req *req)
{
    int32 ret = AUCODE_ERR;
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)req->chan;
    AUDECODE_REQ *audecode_req = (AUDECODE_REQ*)aucoder_hdl->req;
    audio_decode_bind_req(audecode_req, req);
    ret = aucoder_hdl->codec_func(aucoder_hdl); 
    if(req->done == NULL) {
        req->info.actual_sample_rate = audecode_req->actual_sample_rate;
        req->info.actual_channels = audecode_req->actual_channels;
        req->outbuf = audecode_req->outbuf;
        req->outbuf_len = audecode_req->outbuf_len;
    } 
	return ret;
}

void *ulaw_encoder_open(struct acodec_device *dec, txAudioInfo_t *info)
{
	AUCODER_HDL *coder_hdl = NULL;
    AUCODEC_INFO aucodec_info;
    aucodec_info.sample_rate = info->sample_rate;
    aucodec_info.channels = info->channels;
#if ULAW_ENC_CTRL == AUCODER_RUN_IN_CPU0
    coder_hdl = ulaw_encoder_init(&aucodec_info, 0);
#elif ULAW_ENC_CTRL == AUCODER_RUN_IN_CPU1
    coder_hdl = ulaw_encoder_init(&aucodec_info, 1);
#endif
    coder_hdl->aucode_manage_rpc = g_aucode_manage_rpc;
	return coder_hdl;
}

int32 ulaw_encoder_close(struct acodec_device *dec, void *hdl)
{
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)hdl;
    aucoder_hdl->close_func(aucoder_hdl);
	return AUCODE_OK;
}

int32 ulaw_encode_frame(struct acodec_device *dec, struct acodec_encode_req *req)
{
    int32 ret = AUCODE_ERR;
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)req->chan;
    AUENCODE_REQ *auencode_req = (AUENCODE_REQ*)aucoder_hdl->req;
    audio_encode_bind_req(auencode_req, req);
    ret = aucoder_hdl->codec_func(aucoder_hdl);
    if(req->done == NULL) {
        req->outbuf = auencode_req->outbuf;
        req->outbuf_len = auencode_req->outbuf_len;
        req->info.need_more = auencode_req->need_more;
    }     
	return ret;
}

void *pcm_decoder_open(struct acodec_device *dec, txAudioInfo_t *info)
{
	AUCODER_HDL *coder_hdl = NULL;
    AUCODEC_INFO aucodec_info;
    aucodec_info.sample_rate = info->sample_rate;
    aucodec_info.channels = info->channels;
#if PCM_DEC_CTRL == AUCODER_RUN_IN_CPU0
    coder_hdl = pcm_decoder_init(&aucodec_info, 0);
#elif PCM_DEC_CTRL == AUCODER_RUN_IN_CPU1
    coder_hdl = pcm_decoder_init(&aucodec_info, 1);
#endif
    coder_hdl->aucode_manage_rpc = g_aucode_manage_rpc;
	return coder_hdl;
}

int32 pcm_decoder_close(struct acodec_device *dec, void *hdl)
{
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)hdl;
	aucoder_hdl->close_func(aucoder_hdl);
	return AUCODE_OK;
}

int32 pcm_decode_frame(struct acodec_device *dec,  struct acodec_decode_req *req)
{
    AUCODER_HDL *aucoder_hdl = (AUCODER_HDL*)req->chan;
    AUDECODE_REQ *audecode_req = (AUDECODE_REQ*)aucoder_hdl->req;
    audio_decode_bind_req(audecode_req, req);
    aucoder_hdl->codec_func(aucoder_hdl); 
    if(req->done == NULL) {
        req->info.actual_sample_rate = audecode_req->actual_sample_rate;
        req->info.actual_channels = audecode_req->actual_channels;
        req->outbuf = audecode_req->outbuf;
        req->outbuf_len = audecode_req->outbuf_len;
    } 
	return AUCODE_OK;
}