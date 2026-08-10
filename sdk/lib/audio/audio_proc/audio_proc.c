#include "basic_include.h"
#include "lib/audio/audio_proc/audio_proc.h"
#include "heap/aurpc_heap.h"
#include "csi_common_tables.h"

AUPROC_CFG auproc_cfg = {
    /********** AEC **********/
    .magnSum_MaxMin_ratio                = 5.0f,
    .farend_vad_threshold_ratio          = 0.1f,
    .max_filter_coef                     = 5.0f,
    .far_magnSum_min                     = 10000.f,
    .far_magn_valid                      = 1000.0f,
    .nearend_min_snr                     = 5.0f,
    .update_filter_snr                   = 5.0f,
    .delta1_threshold                    = 0.7f,
    .delta2_threshold                    = 10.0f,

    /********** AHS **********/
    .hs_min_ratio                        = 5.0f,
    .hs_threshold                        = 1.0f,
    .decrease_speed                      = 0.8f,
    .increase_speed                      = 0.01f,

    /********** ANS **********/
    .quantile                            = 0.25f,
    .suppression_level                   = SUPPRESSION_LEVEL_3,

    /********** VAD **********/
    .vad_min_snr                         = 5.0f,
    .vad_min_pr                          = 10.0f,
    .min_band_valid_cnt                  = 5,
    .detect_mode                         = DETECT_ENERGY,

    /********** AGC **********/
    .target_db                           = -6,
    .max_increase_db                     = 18,
    .max_output_db                       = -3,
    .sub_frame_time_ms                   = 20,
    .gain_mute_decay                     = 0.9995f,
    .env_decay_voiced                    = 0.9f,
    .env_decay_mute                      = 0.8f,
    .update_step                         = 0.04f,
    .min_env_value                       = 100.0f,
};

extern void *audio_mem_alloc(uint32_t size, void *priv);
extern void audio_mem_free(void *ptr, void *priv);

typedef struct {
    malloc_cb_t mem_alloc;         
    mfree_cb_t  mem_free;           
    void *priv; 
    void *rpc_priv; 
} AUPROC_MEM_STRUCT;

AUPROC_MEM_STRUCT g_auproc_mem_s = {
    .mem_alloc = audio_mem_alloc,
    .mem_free = audio_mem_free,
    .priv = &psram_heap,
    .rpc_priv = &aurpc_psram_heap,
};

void *auproc_alloc(uint32 size)
{
    void *ptr = NULL;
    if(sysctrl_get_cpu_id() == 0) {
        ptr = g_auproc_mem_s.mem_alloc(size, g_auproc_mem_s.priv);
    }
    else {
        ptr = g_auproc_mem_s.mem_alloc(size, g_auproc_mem_s.rpc_priv);
    }
    ASSERT(ptr != NULL);
    return ptr;
}

void auproc_free(void *ptr)
{
    if(sysctrl_get_cpu_id() == 0) {
        g_auproc_mem_s.mem_free(ptr, g_auproc_mem_s.priv);
    }
    else {
        g_auproc_mem_s.mem_free(ptr, g_auproc_mem_s.rpc_priv);
    }
}

int32 audio_process_open(struct auproc_device *auproc, uint32 sample_rate, uint32 channels)
{
    int32_t ret = RET_ERR;
    struct hgauproc_v1 *auproc_dev = (struct hgauproc_v1*)auproc;
	AUPROC_HDL *auproc_hdl = NULL;
    auproc_hdl = audio_process_init(sample_rate, channels, &auproc_cfg);
    if(!auproc_hdl) {
        os_printf("audio_process_open fail!\n");
        goto audio_process_init_err;
    }
#if ANF_PROCESSING
    ret = auproc_attach_anf(auproc_hdl);
    if(ret != RET_OK) {
        os_printf("auproc_attach_anf fail!\n");
        goto audio_process_init_err;        
    }
#endif
#if AEC_PROCESSING
    ret = auproc_attach_aec(auproc_hdl);
    if(ret != RET_OK) {
        os_printf("auproc_attach_aec fail!\n");
        goto audio_process_init_err;        
    }
#endif
#if AHS_PROCESSING
    ret = auproc_attach_ahs(auproc_hdl);
    if(ret != RET_OK) {
        os_printf("auproc_attach_ahs fail!\n");
        goto audio_process_init_err;        
    }
#endif
#if ANS_PROCESSING
    ret = auproc_attach_ans(auproc_hdl);
    if(ret != RET_OK) {
        os_printf("auproc_attach_ans fail!\n");
        goto audio_process_init_err;        
    }
#endif
#if VAD_PROCESSING
    ret = auproc_attach_vad(auproc_hdl);
    if(ret != RET_OK) {
        os_printf("auproc_attach_vad fail!\n");
        goto audio_process_init_err;        
    }
#endif
#if AGC_PROCESSING
    ret = auproc_attach_agc(auproc_hdl);
    if(ret != RET_OK) {
        os_printf("auproc_attach_vad fail!\n");
        goto audio_process_init_err;        
    }
#endif   
    ret = audio_process_prepare(auproc_hdl);
    if(ret != RET_OK) {
        os_printf("audio_process_prepare fail!\n");
        goto audio_process_init_err;        
    }
    auproc_dev->auproc_hdl = auproc_hdl;
    return RET_OK;
audio_process_init_err:
    if(auproc_hdl) {
        audio_process_deinit(auproc_hdl);
    }
    return RET_ERR;
}

int32 audio_process_frame(struct auproc_device *auproc, struct auproc_req *req)
{
    int32 ret = RET_ERR;
    struct hgauproc_v1 *auproc_dev = (struct hgauproc_v1*)auproc;
    AUPROC_HDL *auproc_hdl = auproc_dev->auproc_hdl;
    if(auproc_hdl) {
        ret = RET_OK;
        if(req->far_data) {
            ret |= auproc_put_fardata(auproc_hdl, req->far_data, req->nsamples);
        }
        if(req->near_data) {
            ret |= auproc_put_neardata(auproc_hdl, req->near_data, req->out_data, req->nsamples);
        }
    }
    return ret;
}

int32 audio_process_close(struct auproc_device *auproc)
{
    struct hgauproc_v1 *auproc_dev = (struct hgauproc_v1*)auproc;
    AUPROC_HDL *auproc_hdl = auproc_dev->auproc_hdl;
    if(auproc_hdl) {
        return audio_process_deinit(auproc_hdl);
    }
	return RET_ERR;
}

int32 audio_process_ctrl(struct auproc_device *auproc, enum auproc_ioctl_cmd cmd, uint32 param1, uint32 param2)
{
    struct hgauproc_v1 *auproc_dev = (struct hgauproc_v1*)auproc;
    AUPROC_HDL *auproc_hdl = auproc_dev->auproc_hdl;
    if(auproc_hdl) {
        return RET_ERR;
    }    
    return RET_ERR;
}

const struct auproc_hal_ops auproc_ops = {
    .open     = audio_process_open,
    .close    = audio_process_close,
    .process  = audio_process_frame,
	.ioctl    = audio_process_ctrl,
};

int32 hgauproc_v1_attach(uint32 dev_id, struct hgauproc_v1 *auproc)
{
    auproc->dev.dev.ops        = (const struct devobj_ops *)auproc->ops;
    auproc->auproc_hdl         = NULL;
    dev_register(dev_id, (struct dev_obj *)auproc);
    
    return RET_OK;
}


csi_status csi_rfft_fast_init_f32(csi_rfft_fast_instance_f32 *S, uint16_t fftLen)
{
    uint16_t fftLenBy2 = fftLen >> 1;

    S->Sint.fftLen = fftLenBy2;
    S->fftLenRFFT = fftLen;

    switch (fftLenBy2) {
		case 16:    /* RFFT 32 */
			S->Sint.bitRevLength = CSIBITREVINDEXTABLE_16_TABLE_LENGTH;   /* 20 */
			S->Sint.pBitRevTable = csiBitRevIndexTable16;
			S->Sint.pTwiddle     = (const float32_t *)twiddleCoef_16;
			S->pTwiddleRFFT      = twiddleCoef_rfft_32;
			break;

		case 32:    /* RFFT 64 */
			S->Sint.bitRevLength = CSIBITREVINDEXTABLE_32_TABLE_LENGTH;   /* 48 */
			S->Sint.pBitRevTable = csiBitRevIndexTable32;
			S->Sint.pTwiddle     = (const float32_t *)twiddleCoef_32;
			S->pTwiddleRFFT      = twiddleCoef_rfft_64;
			break;

		case 64:    /* RFFT 128 */
			S->Sint.bitRevLength = CSIBITREVINDEXTABLE_64_TABLE_LENGTH;   /* 56 */
			S->Sint.pBitRevTable = csiBitRevIndexTable64;
			S->Sint.pTwiddle     = (const float32_t *)twiddleCoef_64;
			S->pTwiddleRFFT      = twiddleCoef_rfft_128;
			break;

		case 128:   /* RFFT 256 */
			S->Sint.bitRevLength = CSIBITREVINDEXTABLE_128_TABLE_LENGTH;   /* 208 */
			S->Sint.pBitRevTable = csiBitRevIndexTable128;
			S->Sint.pTwiddle     = (const float32_t *)twiddleCoef_128;
			S->pTwiddleRFFT      = twiddleCoef_rfft_256;
			break;

		case 256:   /* RFFT 512 */
			S->Sint.bitRevLength = CSIBITREVINDEXTABLE_256_TABLE_LENGTH;   /* 440 */
			S->Sint.pBitRevTable = csiBitRevIndexTable256;
			S->Sint.pTwiddle     = (const float32_t *)twiddleCoef_256;
			S->pTwiddleRFFT      = twiddleCoef_rfft_512;
			break;

		case 512:   /* RFFT 1024 */
			S->Sint.bitRevLength = CSIBITREVINDEXTABLE_512_TABLE_LENGTH;   /* 448 */
			S->Sint.pBitRevTable = csiBitRevIndexTable512;
			S->Sint.pTwiddle     = (const float32_t *)twiddleCoef_512;
			S->pTwiddleRFFT      = twiddleCoef_rfft_1024;
			break;
			
#if CSI_RFFT_FAST_F32_MAX_LEN >= 2048
		case 1024:  /* RFFT 2048 */
			S->Sint.bitRevLength = CSIBITREVINDEXTABLE_1024_TABLE_LENGTH;   /* 1800 */
			S->Sint.pBitRevTable = csiBitRevIndexTable1024;
			S->Sint.pTwiddle     = (const float32_t *)twiddleCoef_1024;
			S->pTwiddleRFFT      = twiddleCoef_rfft_2048;
			break;
#endif

#if CSI_RFFT_FAST_F32_MAX_LEN >= 4096
		case 2048:  /* RFFT 4096 */
			S->Sint.bitRevLength = CSIBITREVINDEXTABLE_2048_TABLE_LENGTH;   /* 3808 */
			S->Sint.pBitRevTable = csiBitRevIndexTable2048;
			S->Sint.pTwiddle     = (const float32_t *)twiddleCoef_2048;
			S->pTwiddleRFFT      = twiddleCoef_rfft_4096;
			break;
#endif

#if CSI_RFFT_FAST_F32_MAX_LEN >= 8192
		case 4096:  /* RFFT 8192 */
			S->Sint.bitRevLength = CSIBITREVINDEXTABLE_4096_TABLE_LENGTH;   /* 4032 */
			S->Sint.pBitRevTable = csiBitRevIndexTable4096;
			S->Sint.pTwiddle     = (const float32_t *)twiddleCoef_4096;
			S->pTwiddleRFFT      = twiddleCoef_rfft_8192;
			break;
#endif

		default:
			return CSI_MATH_ARGUMENT_ERROR;
    }

    return CSI_MATH_SUCCESS;
}