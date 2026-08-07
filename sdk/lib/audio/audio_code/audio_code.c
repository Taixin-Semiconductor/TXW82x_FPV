#include "basic_include.h"
#include "lib/audio/audio_code/audio_code.h"

AUCODE_MANAGE *g_aucode_manage = NULL;
struct os_mutex g_aucode_mutex;

AUCODE_MALLOC 	aucode_malloc  = NULL;
AUCODE_ZALLOC 	aucode_zalloc  = NULL;
AUCODE_CALLOC 	aucode_calloc  = NULL;
AUCODE_REALLOC  aucode_realloc = NULL;
AUCODE_FREE     aucode_free    = NULL;

void reg_aucoder_alloc(AUCODE_MALLOC m, AUCODE_ZALLOC z, AUCODE_CALLOC c, AUCODE_REALLOC r, AUCODE_FREE f)
{
    aucode_malloc  = m;
    aucode_zalloc  = z;
    aucode_calloc  = c;
    aucode_realloc = r;
    aucode_free    = f;
}

void aucode_mutex_init(void)
{
    os_mutex_init(&g_aucode_mutex);
}

void *aucode_stack_alloc(uint32_t size)
{
    void *return_addr = NULL;
	if((size%32)!=0) {
		size = (size+32)/32*32;
	}
//	os_printf("%s %d %d %x\n",__FUNCTION__,g_aucode_manage->used_size,size,RETURN_ADDR());
    if((g_aucode_manage->used_size+sizeof(uint32_t)+size)>g_aucode_manage->total_size) {
        while(1) {
            _os_printf("aucode_stack_alloc fail\n");
            os_sleep_ms(1000);
        }
    }
    *((uint32_t*)(g_aucode_manage->stack_priv+g_aucode_manage->used_size+size)) = size;
    return_addr = g_aucode_manage->stack_priv+g_aucode_manage->used_size;
//	os_printf("%s %p %p\n",__FUNCTION__,return_addr,(g_aucode_manage->stack_priv+g_aucode_manage->used_size+size));
    g_aucode_manage->used_size += (sizeof(uint32_t)+size);
    return return_addr;
}

void aucode_stack_free(void *ptr)
{
    uint32_t last_size = *((uint32_t*)(g_aucode_manage->stack_priv+g_aucode_manage->used_size-4));
//	os_printf("%s %p %p %d\n",__FUNCTION__,ptr,(g_aucode_manage->stack_priv+g_aucode_manage->used_size-4),last_size);
    if(ptr != (g_aucode_manage->stack_priv+g_aucode_manage->used_size-last_size-4)) {
        while(1) {
            _os_printf("aucode_stack_free fail\n");
            os_sleep_ms(1000);
        }
    }
    g_aucode_manage->used_size -= (sizeof(uint32_t)+last_size);
//	os_printf("%s %d\n",__FUNCTION__,g_aucode_manage->used_size);
}

AUCODE_HDL *audio_coder_open(uint32_t coder, uint32_t samplerate, uint32_t channel)
{
	AUCODE_HDL *aucode_hdl = NULL;
    if(g_aucode_mutex.hdl) {
        os_mutex_lock(&g_aucode_mutex, osWaitForever);
    }
    else {
        AUCODE_INFO("g_aucode_mutex.hdl is null\n");
        return NULL;
    }
    if(g_aucode_manage == NULL) {
        g_aucode_manage = (AUCODE_MANAGE*)aucode_zalloc(sizeof(AUCODE_MANAGE));
    }
    if(g_aucode_manage == NULL) {
        AUCODE_INFO("alloc aucode_manage fail!\n");
        goto audio_coder_open_err;
    }
    aucode_hdl = (AUCODE_HDL*)aucode_zalloc(sizeof(AUCODE_HDL));
    if(!aucode_hdl) {
        AUCODE_INFO("alloc aucode_hdl fail!\n");
        goto audio_coder_open_err;
    }
    switch(coder) {
#if AAC_ENC_CTRL
        case AAC_ENC:
        {  
            void *aac_enc = NULL;
#if AAC_ENC_CTRL == AUCODER_RUN_IN_CPU0      
            aac_enc = aac_encoder_open(samplerate, channel);
#else
            if(g_aucode_manage->task_hdl == NULL) {
                audio_coder_run_rpc(g_aucode_manage);
            }
            if(g_aucode_manage->task_hdl == NULL) {
                goto audio_coder_open_err;
            }
            aac_enc = aac_encoder_open_rpc(samplerate, channel);
            if(aac_enc) {
                g_aucode_manage->rpc_aucode_s[g_aucode_manage->rpc_coder_num] = aac_enc;
                g_aucode_manage->rpc_coder_num++; 
            }
#endif
            if (!aac_enc) {
                AUCODE_INFO("aac encoder open fail!\n");
                goto audio_coder_open_err;
            }      
            g_aucode_manage->coder_num++;
            aucode_hdl->coder = aac_enc;
            aucode_hdl->coder_type = AAC_ENC;
            break;
        }
#endif
#if AAC_DEC_CTRL
        case AAC_DEC:
        {
            void *aac_dec = NULL;
#if AAC_DEC_CTRL == AUCODER_RUN_IN_CPU0
            aac_dec = aac_decoder_open();
#else
            if(g_aucode_manage->task_hdl == NULL) {
                audio_coder_run_rpc(g_aucode_manage);
            }
            if(g_aucode_manage->task_hdl == NULL) {
                goto audio_coder_open_err;
            }
            aac_dec = aac_decoder_open_rpc();
            if(aac_dec) {
                g_aucode_manage->rpc_aucode_s[g_aucode_manage->rpc_coder_num] = aac_dec;
                g_aucode_manage->rpc_coder_num++;
            }
#endif
            if(!aac_dec) {
                AUCODE_INFO("aac decoder open fail!\n");
                goto audio_coder_open_err;
            }
            g_aucode_manage->coder_num++;
            aucode_hdl->coder = aac_dec;
            aucode_hdl->coder_type = AAC_DEC;
            break;
        }
#endif
#if AMRNB_DEC_CTRL
        case AMRNB_DEC:
        {
            void *amrnb_dec = NULL;
#if AMRNB_DEC_CTRL == AUCODER_RUN_IN_CPU0
            amrnb_dec = amrnb_decoder_open();
#else
            if(g_aucode_manage->task_hdl == NULL) {
                audio_coder_run_rpc(g_aucode_manage);
            }
            if(g_aucode_manage->task_hdl == NULL) {
                goto audio_coder_open_err;
            }
            amrnb_dec = amrnb_decoder_open_rpc();
            if(amrnb_dec) {
                g_aucode_manage->rpc_aucode_s[g_aucode_manage->rpc_coder_num] = amrnb_dec;
                g_aucode_manage->rpc_coder_num++;
            }
#endif
            if(!amrnb_dec) {
                AUCODE_INFO("amrnb decoder open fail!\n");
                goto audio_coder_open_err;
            }
            g_aucode_manage->coder_num++;
            aucode_hdl->coder = amrnb_dec;
            aucode_hdl->coder_type = AMRNB_DEC; 
            break;           
        }
#endif
#if AMRWB_DEC_CTRL
        case AMRWB_DEC:
        {
            void *amrwb_dec = NULL;
#if AMRWB_DEC_CTRL == AUCODER_RUN_IN_CPU0
            amrwb_dec = amrwb_decoder_open();
#else
            if(g_aucode_manage->task_hdl == NULL) {
                audio_coder_run_rpc(g_aucode_manage);
            }
            if(g_aucode_manage->task_hdl == NULL) {
                goto audio_coder_open_err;
            }
            amrwb_dec = amrwb_decoder_open_rpc();
            if(amrwb_dec) {
                g_aucode_manage->rpc_aucode_s[g_aucode_manage->rpc_coder_num] = amrwb_dec;
                g_aucode_manage->rpc_coder_num++;
            }
#endif
            if(!amrwb_dec) {
                AUCODE_INFO("amrwb decoder open fail!\n");
                goto audio_coder_open_err;
            }
            g_aucode_manage->coder_num++;
            aucode_hdl->coder = amrwb_dec;
            aucode_hdl->coder_type = AMRWB_DEC;
            break;
        }
#endif
#if MP3_DEC_CTRL
        case MP3_DEC:
        {
            void *mp3_dec = NULL;
            mp3_dec = mp3_decoder_open();
            if(!mp3_dec) {
                AUCODE_INFO("mp3 decoder open fail!\n");
                goto audio_coder_open_err;
            }
            g_aucode_manage->coder_num++;
            aucode_hdl->coder = mp3_dec;
            aucode_hdl->coder_type = MP3_DEC;  
            break;  
        }
#endif
#if ALAW_ENC_CTRL
        case ALAW_ENC:
        {
            void *alaw_enc = NULL;
#if ALAW_ENC_CTRL == AUCODER_RUN_IN_CPU0
            alaw_enc = alaw_encoder_open();
else
            if(g_aucode_manage->task_hdl == NULL) {
                audio_coder_run_rpc(g_aucode_manage);
            }
            if(g_aucode_manage->task_hdl == NULL) {
                goto audio_coder_open_err;
            }
            alaw_enc = alaw_encoder_open_rpc();
            if(alaw_enc) {
                g_aucode_manage->rpc_aucode_s[g_aucode_manage->rpc_coder_num] = alaw_enc;
                g_aucode_manage->rpc_coder_num++;
            }
#endif
            if(!alaw_enc) {
                AUCODE_INFO("alaw encoder open fail!\n");
                goto audio_coder_open_err;
            }
            g_aucode_manage->coder_num++;
            aucode_hdl->coder = alaw_enc;
            aucode_hdl->coder_type = ALAW_ENC;
            break;
        }
#endif
#if ALAW_DEC_CTRL
        case ALAW_DEC:
        {
            void *alaw_dec = NULL;
#if ALAW_ENC_CTRL == AUCODER_RUN_IN_CPU0
            alaw_dec = alaw_decoder_open();
else
            if(g_aucode_manage->task_hdl == NULL) {
                audio_coder_run_rpc(g_aucode_manage);
            }
            if(g_aucode_manage->task_hdl == NULL) {
                goto audio_coder_open_err;
            }
            alaw_dec = alaw_decoder_open_rpc();
            if(alaw_dec) {
                g_aucode_manage->rpc_aucode_s[g_aucode_manage->rpc_coder_num] = alaw_dec;
                g_aucode_manage->rpc_coder_num++;
            }
#endif
            if(!alaw_dec) {
                AUCODE_INFO("alaw decoder open fail!\n");
                goto audio_coder_open_err;
            }
            g_aucode_manage->coder_num++;
            aucode_hdl->coder = alaw_dec;
            aucode_hdl->coder_type = ALAW_DEC;
            break;
        }
#endif
#if ULAW_ENC_CTRL
        case ULAW_ENC:
        {
            void *ulaw_enc = NULL;
#if ALAW_ENC_CTRL == AUCODER_RUN_IN_CPU0
            ulaw_enc = ulaw_encoder_open();
else
            if(g_aucode_manage->task_hdl == NULL) {
                audio_coder_run_rpc(g_aucode_manage);
            }
            if(g_aucode_manage->task_hdl == NULL) {
                goto audio_coder_open_err;
            }
            ulaw_enc = ulaw_encoder_open_rpc();
            if(ulaw_enc) {
                g_aucode_manage->rpc_aucode_s[g_aucode_manage->rpc_coder_num] = ulaw_enc;
                g_aucode_manage->rpc_coder_num++;
            }
#endif
            if(!ulaw_enc) {
                AUCODE_INFO("ulaw encoder open fail!\n");
                goto audio_coder_open_err;
            }
            g_aucode_manage->coder_num++;
            aucode_hdl->coder = ulaw_enc;
            aucode_hdl->coder_type = ULAW_ENC;
            break;
        }
#endif
#if ULAW_DEC_CTRL
        case ULAW_DEC:
        {
            void *ulaw_dec = NULL;
#if ULAW_ENC_CTRL == AUCODER_RUN_IN_CPU0
            ulaw_dec = ulaw_decoder_open();
else
            if(g_aucode_manage->task_hdl == NULL) {
                audio_coder_run_rpc(g_aucode_manage);
            }
            if(g_aucode_manage->task_hdl == NULL) {
                goto audio_coder_open_err;
            }
            ulaw_dec = ulaw_decoder_open_rpc();
            if(ulaw_dec) {
                g_aucode_manage->rpc_aucode_s[g_aucode_manage->rpc_coder_num] = ulaw_dec;
                g_aucode_manage->rpc_coder_num++;
            }
#endif
            if(!ulaw_dec) {
                AUCODE_INFO("ulaw decoder open fail!\n");
                goto audio_coder_open_err;
            }
            g_aucode_manage->coder_num++;
            aucode_hdl->coder = ulaw_dec;
            aucode_hdl->coder_type = ULAW_DEC;
            break;
        }
#endif
#if OPUS_ENC_CTRL
        case OPUS_ENC:
        {
            void *opus_enc = NULL;
#if OPUS_ENC_CTRL == AUCODER_RUN_IN_CPU0
            opus_enc = opus_encoder_open(samplerate, channel);
#else
            if(g_aucode_manage->task_hdl == NULL) {
                audio_coder_run_rpc(g_aucode_manage);
            }
            if(g_aucode_manage->task_hdl == NULL) {
                goto audio_coder_open_err;
            }
            opus_enc = opus_encoder_open_rpc(samplerate, channel);
            if(opus_enc) {
                g_aucode_manage->rpc_aucode_s[g_aucode_manage->rpc_coder_num] = opus_enc;
                g_aucode_manage->rpc_coder_num++;
            }
#endif
            if(!opus_enc) {
                AUCODE_INFO("opus encoder open fail!\n");
                goto audio_coder_open_err;                
            }
            if(g_aucode_manage->stack_priv == NULL) {
                g_aucode_manage->total_size = AUCODE_STACK_SIZE;
                g_aucode_manage->used_size = 0;
                g_aucode_manage->stack_priv = (int8_t*)aucode_malloc(sizeof(int8_t) * g_aucode_manage->total_size);
                if(g_aucode_manage->stack_priv == NULL) {
#if OPUS_ENC_CTRL == AUCODER_RUN_IN_CPU1
                    g_aucode_manage->rpc_coder_num--;
                    g_aucode_manage->rpc_aucode_s[g_aucode_manage->rpc_coder_num] = NULL;
#endif
                    goto audio_coder_open_err;
                }  
            }
            g_aucode_manage->stack_used_num++;
            g_aucode_manage->coder_num++;
            aucode_hdl->coder = opus_enc;
            aucode_hdl->coder_type = OPUS_ENC;
            break;
        }
#endif
#if OPUS_DEC_CTRL
        case OPUS_DEC:
        {
            void *opus_dec = NULL;
#if OPUS_DEC_CTRL == AUCODER_RUN_IN_CPU0
            opus_dec = opus_decoder_open(samplerate, channel);
#else
            if(g_aucode_manage->task_hdl == NULL) {
                audio_coder_run_rpc(g_aucode_manage);
            }
            if(g_aucode_manage->task_hdl == NULL) {
                goto audio_coder_open_err;
            }
            opus_dec = opus_decoder_open_rpc(samplerate, channel);
            if(opus_dec) {
                g_aucode_manage->rpc_aucode_s[g_aucode_manage->rpc_coder_num] = opus_dec;
                g_aucode_manage->rpc_coder_num++;
            }
#endif
            if (!opus_dec) {
                AUCODE_INFO("opus decoder open fail!\n");
                goto audio_coder_open_err;
            }
            if(g_aucode_manage->stack_priv == NULL) {
                g_aucode_manage->total_size = AUCODE_STACK_SIZE;
                g_aucode_manage->used_size = 0;
                g_aucode_manage->stack_priv = (int8_t*)aucode_malloc(sizeof(int8_t) * g_aucode_manage->total_size);
                if(g_aucode_manage->stack_priv == NULL) {
#if OPUS_DEC_CTRL == AUCODER_RUN_IN_CPU1
                    g_aucode_manage->rpc_coder_num--;
                    g_aucode_manage->rpc_aucode_s[g_aucode_manage->rpc_coder_num] = NULL;
#endif
                    goto audio_coder_open_err;
                }  
            }
            g_aucode_manage->stack_used_num++;
            g_aucode_manage->coder_num++;
            aucode_hdl->coder = opus_dec;
            aucode_hdl->coder_type = OPUS_DEC;
            break;
        }
#endif
        default:
            AUCODE_INFO("audio coder open fail,unsupport coder type!\r\n");
            goto audio_coder_open_err;
    }
    os_mutex_unlock(&g_aucode_mutex);
    return aucode_hdl;
audio_coder_open_err:
    if(g_aucode_manage) {
        if(g_aucode_manage->stack_priv && (g_aucode_manage->stack_used_num == 0)) {
           aucode_free(g_aucode_manage->stack_priv);
           g_aucode_manage->stack_priv = NULL;
        }
        if(g_aucode_manage->rpc_coder_num == 0) {
            if(g_aucode_manage->task_hdl) {
                g_aucode_manage->state = coder_close;
                while(g_aucode_manage->state != coder_exit) {
                    os_sleep_ms(10);
                }
            }
        }
        if(g_aucode_manage->coder_num == 0) {
            aucode_free(g_aucode_manage);
            g_aucode_manage = NULL;
        }
    }
    if(aucode_hdl) {
        aucode_free(aucode_hdl);
    }
    os_mutex_unlock(&g_aucode_mutex);
    return NULL;
}

int32_t audio_encoder_set_bitrate(AUCODE_HDL *aucode_hdl, uint32_t bitrate)
{
    int32_t ret = AUCODE_ERR;
    switch(aucode_hdl->coder_type) {
#if OPUS_ENC_CTRL
        case OPUS_ENC:
        {
#if OPUS_ENC_CTRL == AUCODER_RUN_IN_CPU0
            ret = opus_encoder_set_bitrate(aucode_hdl->coder, bitrate);
#else
            ret = opus_encoder_set_bitrate_rpc(aucode_hdl->coder, bitrate);
#endif
            break;
        }
#endif
        default:
            AUCODE_INFO("audio encoder unsupport set bitrate!\r\n");
            break;
    }   
    return ret;
}

int32_t audio_encode_data(AUCODE_HDL *aucode_hdl, int16_t *in_data, uint32_t samples, uint8_t *out_data)
{
    int32_t encode_bytes = 0;
    
    switch(aucode_hdl->coder_type) {
#if AAC_ENC_CTRL
        case AAC_ENC:
        {
#if AAC_ENC_CTRL == AUCODER_RUN_IN_CPU0
            encode_bytes = aac_encode_data(aucode_hdl->coder, in_data, samples, out_data);
#else
            os_mutex_lock(&g_aucode_mutex, osWaitForever);
            audio_encode_data_rpc(aucode_hdl->coder, in_data, samples, out_data, &encode_bytes);
            os_mutex_unlock(&g_aucode_mutex);
#endif
            break;
        }
#endif
#if ALAW_ENC_CTRL
        case ALAW_ENC:
        {
#if ALAW_ENC_CTRL == AUCODER_RUN_IN_CPU0
            encode_bytes = alaw_encode_data(aucode_hdl->coder, in_data, samples, out_data);
#else
            os_mutex_lock(&g_aucode_mutex, osWaitForever);
            audio_encode_data_rpc(aucode_hdl->coder, in_data, samples, out_data, &encode_bytes);
            os_mutex_unlock(&g_aucode_mutex);
#endif
            break;
        }
#endif
#if ULAW_ENC_CTRL
        case ULAW_ENC:
        {
#if ALAW_ENC_CTRL == AUCODER_RUN_IN_CPU0
            encode_bytes = ulaw_encode_data(aucode_hdl->coder, in_data, samples, out_data);
#else
            os_mutex_lock(&g_aucode_mutex, osWaitForever);
            audio_encode_data_rpc(aucode_hdl->coder, in_data, samples, out_data, &encode_bytes);
            os_mutex_unlock(&g_aucode_mutex);
#endif
            break;
        }
#endif
#if OPUS_ENC_CTRL
        case OPUS_ENC:
        {
            os_mutex_lock(&g_aucode_mutex, osWaitForever);
#if OPUS_ENC_CTRL == AUCODER_RUN_IN_CPU0
            encode_bytes = opus_encode_data(aucode_hdl->coder, in_data, samples, out_data);
#else
            audio_encode_data_rpc(aucode_hdl->coder, in_data, samples, out_data, &encode_bytes);
#endif
            os_mutex_unlock(&g_aucode_mutex);
	        break;
        }
#endif
        default:
            AUCODE_INFO("audio encode data fail,unknow coder type!\r\n");
            break;
    }
    os_mutex_unlock(&g_aucode_mutex);
    return encode_bytes;
}

int32_t audio_decode_fade_in(AUCODE_HDL *aucode_hdl)
{
    aucode_hdl->fade_type = 1;
    return RET_OK;
}

int32_t audio_decode_fade_out(AUCODE_HDL *aucode_hdl)
{
    aucode_hdl->fade_type = 2;
    return RET_OK;
}

int32_t audio_decode_data(AUCODE_HDL *aucode_hdl, uint8_t *in_data, uint32_t bytes, int16_t *out_data, AUCODE_FRAME_INFO *frame_info)
{
    int32_t decode_samples = 0;
    float fade_init = 0.0001f;
    float fade_speed = 0.0001f;

    switch(aucode_hdl->coder_type) {
#if AAC_DEC_CTRL
        case AAC_DEC:
        {
#if AAC_DEC_CTRL == AUCODER_RUN_IN_CPU0
            decode_samples = aac_decode_data(aucode_hdl->coder, in_data, bytes, out_data, frame_info);
#else
            os_mutex_lock(&g_aucode_mutex, osWaitForever);
            audio_decode_data_rpc(aucode_hdl->coder, in_data, bytes, out_data, frame_info, &decode_samples);
            os_mutex_unlock(&g_aucode_mutex);
#endif
            break;
        }
#endif
#if AMRNB_DEC_CTRL
        case AMRNB_DEC:
        {
#if AMRNB_DEC_CTRL == AUCODER_RUN_IN_CPU0
            decode_samples = amrnb_decode_data(aucode_hdl->coder, in_data, bytes, out_data, frame_info);
#else
            os_mutex_lock(&g_aucode_mutex, osWaitForever);
            audio_decode_data_rpc(aucode_hdl->coder, in_data, bytes, out_data, frame_info, &decode_samples);
            os_mutex_unlock(&g_aucode_mutex);
#endif
            break;
        }
#endif
#if AMRWB_DEC_CTRL
        case AMRWB_DEC:
        {
#if AMRWB_DEC_CTRL == AUCODER_RUN_IN_CPU0
            decode_samples = amrwb_decode_data(aucode_hdl->coder, in_data, bytes, out_data, frame_info);
#else
            os_mutex_lock(&g_aucode_mutex, osWaitForever);
            audio_decode_data_rpc(aucode_hdl->coder, in_data, bytes, out_data, frame_info, &decode_samples);
            os_mutex_unlock(&g_aucode_mutex);
#endif
            break;
        }
#endif
#if MP3_DEC_CTRL
        case MP3_DEC:
        {
            decode_samples = mp3_decode_data(aucode_hdl->coder, in_data, bytes, out_data, frame_info);
            break;
        }
#endif
#if ALAW_DEC_CTRL
        case ALAW_DEC:
        {
#if ALAW_DEC_CTRL == AUCODER_RUN_IN_CPU0
            decode_samples = alaw_decode_data(aucode_hdl->coder, in_data, bytes, out_data, frame_info);
#else
            os_mutex_lock(&g_aucode_mutex, osWaitForever);
            audio_decode_data_rpc(aucode_hdl->coder, in_data, bytes, out_data, frame_info, &decode_samples);
            os_mutex_unlock(&g_aucode_mutex);
#endif
            break;
        }
#endif
#if ULAW_DEC_CTRL
        case ULAW_DEC:
        {
#if ULAW_DEC_CTRL == AUCODER_RUN_IN_CPU0
            decode_samples = ulaw_decode_data(aucode_hdl->coder, in_data, bytes, out_data, frame_info);
#else
            os_mutex_lock(&g_aucode_mutex, osWaitForever);
            audio_decode_data_rpc(aucode_hdl->coder, in_data, bytes, out_data, frame_info, &decode_samples);
            os_mutex_unlock(&g_aucode_mutex);
#endif
            break;
        }
#endif
#if OPUS_DEC_CTRL
        case OPUS_DEC:
        {
            os_mutex_lock(&g_aucode_mutex, osWaitForever);
#if OPUS_DEC_CTRL == AUCODER_RUN_IN_CPU0
            decode_samples = opus_decode_data(aucode_hdl->coder, in_data, bytes, out_data, frame_info);
#else
            audio_decode_data_rpc(aucode_hdl->coder, in_data, bytes, out_data, frame_info, &decode_samples);
#endif
            os_mutex_unlock(&g_aucode_mutex);
	        break;
        }
#endif
        default:
            AUCODE_INFO("audio decode data fail,unknow coder type!\r\n");
            break;
    }
    if(aucode_hdl->fade_type == 1) {
        fade_init = 0.0001f;
        fade_speed = 1.0f / decode_samples;
        for(uint32_t i=0; i<decode_samples; i++) {
           out_data[i] = (int16_t)((float)out_data[i] * fade_init);
           fade_init += fade_speed;
        } 
    }
    else if(aucode_hdl->fade_type == 2) {
        fade_init = 1.0f;
        fade_speed = 1.0f / decode_samples;
        for(uint32_t i=0; i<decode_samples; i++) {
           out_data[i] = (int16_t)((float)out_data[i] * fade_init);
           fade_init -= fade_speed;
        } 
    }
    aucode_hdl->fade_type = 0;
    aucode_hdl->frame_nsamples = decode_samples;
    return decode_samples;
}

int32_t audio_decode_output_mute(AUCODE_HDL *aucode_hdl, int16_t *out_data)
{
    int32_t decode_samples = 0;
    decode_samples = aucode_hdl->frame_nsamples;
    os_memset(out_data, 0, decode_samples * sizeof(int16_t));
    return decode_samples;
}

int32_t audio_decode_do_plc(AUCODE_HDL *aucode_hdl, int16_t *out_data)
{
    int32_t decode_samples = 0;
    switch(aucode_hdl->coder_type) {
#if OPUS_DEC_CTRL
        case OPUS_DEC:
        {
            os_mutex_lock(&g_aucode_mutex, osWaitForever);
#if OPUS_DEC_CTRL == AUCODER_RUN_IN_CPU0
            decode_samples = opus_decode_plc(aucode_hdl->coder, out_data, aucode_hdl->frame_nsamples);
#else
            audio_decode_plc_rpc(aucode_hdl->coder, out_data, aucode_hdl->frame_nsamples, &decode_samples);
#endif
            os_mutex_unlock(&g_aucode_mutex);
            break;
        }
#endif
        default:
            AUCODE_INFO("audio decoder unsupport do plc!\r\n");
            break;
    }
    return decode_samples;
}

void audio_coder_close(AUCODE_HDL *aucode_hdl)
{
    if(aucode_hdl == NULL) {
	    return;
    }
    os_mutex_lock(&g_aucode_mutex, osWaitForever);
    for(uint32_t i=0; i<g_aucode_manage->rpc_coder_num; i++) {
        if(g_aucode_manage->rpc_aucode_s[i] == aucode_hdl->coder) {
            g_aucode_manage->rpc_aucode_s[i] = NULL;
            os_memmove(&(g_aucode_manage->rpc_aucode_s[i]), &(g_aucode_manage->rpc_aucode_s[i+1]), 
                                    (g_aucode_manage->rpc_coder_num-1-i)*sizeof(RPC_AUCODE_STRUCT*));
            break;
        }
    }
    switch(aucode_hdl->coder_type) {
#if AAC_ENC_CTRL
        case AAC_ENC:
        {
#if AAC_ENC_CTRL == AUCODER_RUN_IN_CPU0
            aac_encoder_close(aucode_hdl->coder);
#else
            audio_coder_close_rpc(aucode_hdl->coder);
            g_aucode_manage->rpc_coder_num--;
#endif
            g_aucode_manage->coder_num--;
            break;
        }
#endif
#if AAC_DEC_CTRL
        case AAC_DEC:
        {
#if AAC_DEC_CTRL == AUCODER_RUN_IN_CPU0
            aac_decoder_close(aucode_hdl->coder);
#else
            audio_coder_close_rpc(aucode_hdl->coder);
            g_aucode_manage->rpc_coder_num--;
#endif
            g_aucode_manage->coder_num--;
            break;
        }
#endif
#if AMRNB_DEC_CTRL
        case AMRNB_DEC:
        {
#if AMRNB_DEC_CTRL == AUCODER_RUN_IN_CPU0
            amrnb_decoder_close(aucode_hdl->coder);
#else
            audio_coder_close_rpc(aucode_hdl->coder);
            g_aucode_manage->rpc_coder_num--;
#endif
            g_aucode_manage->coder_num--;
            break;
        }
#endif
#if AMRWB_DEC_CTRL
        case AMRWB_DEC:
        {
#if AMRWB_DEC_CTRL == AUCODER_RUN_IN_CPU0
            amrwb_decoder_close(aucode_hdl->coder);
#else
            audio_coder_close_rpc(aucode_hdl->coder);
            g_aucode_manage->rpc_coder_num--;
#endif
            g_aucode_manage->coder_num--;
            break;
        }
#endif
#if MP3_DEC_CTRL
        case MP3_DEC:
        {
            mp3_decoder_close(aucode_hdl->coder);
            g_aucode_manage->coder_num--;
            break;
        }
#endif
#if ALAW_ENC_CTRL
        case ALAW_ENC:
        {
#if ALAW_ENC_CTRL == AUCODER_RUN_IN_CPU0
            alaw_encoder_close(aucode_hdl->coder);
#else
            audio_coder_close_rpc(aucode_hdl->coder);
            g_aucode_manage->rpc_coder_num--;
#endif
            g_aucode_manage->coder_num--;
            break;
        }
#endif
#if ALAW_DEC_CTRL
        case ALAW_DEC: 
        {
#if ALAW_DEC_CTRL == AUCODER_RUN_IN_CPU0
            alaw_decoder_close(aucode_hdl->coder);
#else
            audio_coder_close_rpc(aucode_hdl->coder);
            g_aucode_manage->rpc_coder_num--;
#endif
            g_aucode_manage->coder_num--;
            break;
        }
#endif
#if ULAW_ENC_CTRL
        case ULAW_ENC:  
        {
#if ULAW_ENC_CTRL == AUCODER_RUN_IN_CPU0
            ulaw_encoder_close(aucode_hdl->coder);
#else
            audio_coder_close_rpc(aucode_hdl->coder);
            g_aucode_manage->rpc_coder_num--;
#endif
            g_aucode_manage->coder_num--;
            break;
        }  
#endif 
#if ULAW_DEC_CTRL
        case ULAW_DEC:
        {
#if ULAW_DEC_CTRL == AUCODER_RUN_IN_CPU0
            ulaw_decoder_close(aucode_hdl->coder);
#else
            audio_coder_close_rpc(aucode_hdl->coder);
            g_aucode_manage->rpc_coder_num--;
#endif
            g_aucode_manage->coder_num--;
            break;
        }
#endif
#if OPUS_ENC_CTRL
        case OPUS_ENC:
        {
#if OPUS_ENC_CTRL == AUCODER_RUN_IN_CPU0
            opus_encoder_close(aucode_hdl->coder);
#else
            audio_coder_close_rpc(aucode_hdl->coder);
            g_aucode_manage->rpc_coder_num--;
#endif
            g_aucode_manage->coder_num--;
            g_aucode_manage->stack_used_num--;
            break;
        }
#endif
#if OPUS_DEC_CTRL
        case OPUS_DEC:
        {
#if OPUS_DEC_CTRL == AUCODER_RUN_IN_CPU0
            opus_decoder_close(aucode_hdl->coder);
#else
            audio_coder_close_rpc(aucode_hdl->coder);
            g_aucode_manage->rpc_coder_num--;
#endif
            g_aucode_manage->coder_num--;
            g_aucode_manage->stack_used_num--;
            break;
        }
#endif
        default:
            AUCODE_INFO("audio coder close fail,unknow coder type!\r\n");
            break;
    }
    aucode_free(aucode_hdl);
    if(g_aucode_manage->stack_priv && (g_aucode_manage->stack_used_num == 0)) {
        aucode_free(g_aucode_manage->stack_priv);
        g_aucode_manage->stack_priv = NULL;
    }
    if(g_aucode_manage->rpc_coder_num == 0) {
        if(g_aucode_manage->task_hdl) {
            g_aucode_manage->state = coder_close;
            while(g_aucode_manage->state != coder_exit) {
                os_sleep_ms(10);
            }
        }
    }
    if(g_aucode_manage->coder_num == 0) {
        aucode_free(g_aucode_manage);
        g_aucode_manage = NULL;
    }
    os_mutex_unlock(&g_aucode_mutex);
}