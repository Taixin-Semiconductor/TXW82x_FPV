#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/audio/audio_code/audio_code.h"
#include "opus_code.h"

#define MAX_OPUS_ENCODE_RXBUF    4
#define MAX_OPUS_ENCODE_TXBUF    14

#define FRAME_SIZE               160

struct opus_encode_struct {
    struct fbpool tx_pool;
    struct os_event event;
    struct msi *msi;
    struct msi *src_msi;
    void *task_hdl;
    uint8_t destroy_self;
    uint8_t next_status;
    uint8_t current_status;
    uint8_t enc_buf[1024];
    int16_t inbuf[FRAME_SIZE];
    uint32_t samplerate;
    uint32_t new_bitrate;
    uint32_t cur_bitrate;
    AUDIO_INFO audio_info;
};

static void opus_encode_thread(void *d)
{
    uint8_t clear_finish = 1;
    uint8_t get_audio_time = 0;
    uint8_t update_audio_time = 0;
    uint8_t *send_data = NULL;
	int16_t *recv_data = NULL;
    int32_t enc_bytes = 0;
    int32_t ret = 0;
    uint32_t data_len = 0;
    uint32_t data_offset = 0;
    uint32_t inbuf_offset = 0;
    uint32_t inbuf_reslen = (FRAME_SIZE << 1);
    uint32_t audio_time = 0;
    uint32_t clear_flag = 0;
    struct framebuff *send_frame_buf = NULL;
	struct framebuff *recv_frame_buf = NULL;
    struct opus_encode_struct *s = (struct opus_encode_struct *)d;
    AUCODE_HDL *opus_enc = NULL;

    s->msi->enable = 1; 
    msi_get(s->msi);

    opus_enc = audio_coder_open(OPUS_ENC, s->samplerate, 1);
    if (opus_enc == NULL) 
        goto opus_encode_thread_end;

    s->audio_info.nsamples = FRAME_SIZE;
    s->audio_info.time_interval = s->audio_info.nsamples * FRAME_SIZE / s->samplerate;
    s->audio_info.samplerate = s->samplerate;

    while(1) {
        os_event_wait(&s->event, coder_clear_event, &clear_flag, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
        if(clear_flag & coder_clear_event) {
            clear_flag = 0;
            clear_finish = 0;
        }
        if(s->next_status == AUCODEC_PAUSE) {
            // if(s->current_status == AUCODEC_RUN) {
            //     audio_coder_close(opus_enc);
            //     opus_enc = audio_coder_open(OPUS_ENC, s->samplerate, 1);
            //     if(opus_enc == NULL) 
            //         goto opus_encode_thread_end;
            // }
            s->current_status = AUCODEC_PAUSE;
            inbuf_reslen = (FRAME_SIZE << 1);
            inbuf_offset = 0; 
        }
        if(s->cur_bitrate != s->new_bitrate) {
            if(audio_encoder_set_bitrate(opus_enc, s->new_bitrate) == RET_OK) {
                s->cur_bitrate = s->new_bitrate;
            }
        }
		recv_frame_buf = msi_get_fb(s->msi, 0);
        if(recv_frame_buf) {
            if(clear_finish == 0) {
                inbuf_reslen = (FRAME_SIZE << 1);
                inbuf_offset = 0;  
                goto opus_encode_frame_end;
            }
            if(s->next_status == AUCODEC_PAUSE) { 
                goto opus_encode_frame_end;
            }
            else {
                s->current_status = AUCODEC_RUN;
                recv_data = (int16_t*)recv_frame_buf->data;
                data_len = recv_frame_buf->len;
                data_offset = 0;
                if(!get_audio_time) {
                    get_audio_time = 1;
                    audio_time = recv_frame_buf->time;
                }
                if(update_audio_time) {
                    audio_time = recv_frame_buf->time;
                    update_audio_time = 0;
                }
                while(data_len >= inbuf_reslen) {
                    os_memcpy(s->inbuf + (inbuf_offset/2), recv_data + (data_offset/2), inbuf_reslen);
                    data_len -= inbuf_reslen;
                    data_offset += inbuf_reslen;   
                    enc_bytes = audio_encode_data(opus_enc, s->inbuf, FRAME_SIZE, s->enc_buf); 
                    inbuf_reslen = (FRAME_SIZE << 1);
                    inbuf_offset = 0;  
                    while(!send_frame_buf) {
                        send_frame_buf = fbpool_get(&s->tx_pool, 0, s->msi);
                        if(!send_frame_buf)
                            os_sleep_ms(1); 
                    }     
                    if(send_frame_buf) {
                        send_frame_buf->data = (uint8_t*)OPUS_CODE_MALLOC(enc_bytes);
                        if(!send_frame_buf->data) {
                            OPUS_INFO("opus encode malloc send_frame_buf->data fail!\n");
                            msi_delete_fb(s->msi, send_frame_buf);
                            send_frame_buf = NULL;
                            goto opus_encode_frame_end;
                        }
                        send_data = send_frame_buf->data;
                        os_memcpy(send_data, s->enc_buf, enc_bytes);
                        send_frame_buf->len = enc_bytes;
                        send_frame_buf->mtype = F_AUDIO;
                        send_frame_buf->time = audio_time;
                        send_frame_buf->priv = &(s->audio_info);
                        ret = msi_output_fb(s->msi, send_frame_buf); 
                        OPUS_DEBUG("opus encode send framebuff:%p,ret:%d\r\n",send_frame_buf,ret);  
                        send_frame_buf = NULL;   
                    }
                    audio_time += (FRAME_SIZE * 1000 / s->samplerate);
                    if(data_len == 0)
                        update_audio_time = 1;
opus_encode_frame_end:
                    os_sleep_ms(1);
                }
                if(data_len) {
                    os_memcpy(s->inbuf + (inbuf_offset/2), recv_data + (data_offset/2), data_len);
                    inbuf_reslen -= data_len;
                    inbuf_offset += data_len;
                    data_offset = 0;
                    data_len = 0;
                }
            }
            OPUS_DEBUG("opus encode delete framebuff:%p,len:%d,\r\n",recv_frame_buf,recv_frame_buf->len);
            msi_delete_fb(s->msi, recv_frame_buf);
            recv_frame_buf = NULL; 
        }
        else {
            if(clear_finish == 0) {
                clear_finish = 1;
                os_event_set(&s->event, coder_clear_finish_event, NULL);
            }
            os_sleep_ms(1);
        }

        if(s->next_status == AUCODEC_EXIT) {
            goto opus_encode_thread_end;
        }
    }
opus_encode_thread_end:
    if(opus_enc)
        audio_coder_close(opus_enc);
    os_event_set(&s->event, coder_exit_event, NULL);

    while((s->next_status != AUCODEC_EXIT) && (s->destroy_self == 0)) {
        s->current_status = AUCODEC_END;
        os_sleep_ms(5);
    }

    if(s->src_msi) {
        msi_del_output(s->src_msi, NULL, s->msi->name);
        s->src_msi = NULL;
    }

    if(s->next_status != AUCODEC_EXIT)
        msi_destroy(s->msi);

    msi_put(s->msi);    
}

static int32_t opus_encode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    struct opus_encode_struct *opus_encode_s = (struct opus_encode_struct*)(msi->priv);
    switch(cmd_id) {
		case MSI_CMD_AUCODER:
		{
            ret = RET_ERR;
			if(opus_encode_s) {
				uint32_t cmd_self = (uint32_t)param1;
				switch(cmd_self) {	
                    case MSI_AUCODER_PAUSE:
                    {
                        if(opus_encode_s->current_status == AUCODEC_RUN) {
                            opus_encode_s->next_status = AUCODEC_PAUSE;
                        }
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_CONTINUE:
                    {
                        if(opus_encode_s->current_status == AUCODEC_PAUSE) {
                            opus_encode_s->next_status = AUCODEC_RUN;
                        }
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_GET_STATUS:
                    {
                        *((uint32_t*)param2) = (uint32_t)(opus_encode_s->current_status);
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_SET_BITRATE:
                    {
                        opus_encode_s->new_bitrate = param2;
                        ret = RET_OK;  
                        break;     
                    }
                    case MSI_AUCODER_CLEAR_STREAM:
                    {
                        os_event_set(&opus_encode_s->event, coder_clear_event, NULL);
                        os_event_wait(&opus_encode_s->event, coder_clear_finish_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                        ret = RET_OK;  
                        break;                                
                    }
                    case MSI_AUCODER_SET_SRCMSI:
                    {
                        if(opus_encode_s->src_msi) {
                            msi_del_output(opus_encode_s->src_msi, NULL, msi->name);
                        }
                        opus_encode_s->src_msi = NULL;
                        ret = msi_add_output((struct msi*)param2, NULL, msi->name);
                        if(ret == RET_OK) {
                            opus_encode_s->src_msi = (struct msi*)param2;
                        }
                        break;
                    }
					case MSI_AUCODER_DEINIT:
					{	
						msi_destroy(msi);
                        ret = RET_OK; 	
						break;				
					}
					default:
						break;
				}
			}
			break;
		}
        case MSI_CMD_TRANS_FB:
        {
            ret = RET_ERR;
            struct framebuff *frame_buf = (struct framebuff *)param1;
            if(frame_buf->mtype == F_AUDIO) {
                ret = RET_OK;
            } 
            break;
        }            
        case MSI_CMD_FREE_FB:
        {
            ret = RET_ERR;
            if(opus_encode_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                if(frame_buf->data) {
                    OPUS_CODE_FREE(frame_buf->data);
                    frame_buf->data = NULL;
                }
                fbpool_put(&opus_encode_s->tx_pool, frame_buf);
            }
            break; 
        }   
        case MSI_CMD_PRE_DESTROY:
        {
            if(opus_encode_s && opus_encode_s->task_hdl) {
                opus_encode_s->next_status = AUCODEC_EXIT;
            }
            break;
        }       
		case MSI_CMD_POST_DESTROY:
        {
            if(opus_encode_s) {
                if(opus_encode_s->task_hdl) {
                    os_event_wait(&opus_encode_s->event, coder_exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
                for(uint32_t i=0; i<MAX_OPUS_ENCODE_TXBUF; i++) {
                    struct framebuff *frame_buf = (opus_encode_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        OPUS_CODE_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&opus_encode_s->tx_pool);
                if(opus_encode_s->event.hdl) {
                    os_event_del(&opus_encode_s->event);  
                } 
                OPUS_CODE_FREE(opus_encode_s);
                opus_encode_s = NULL;
            }
            break; 
        }         
        default:
            break;    
    }
    return ret;
}

struct msi *opus_encode_init(uint32_t samplerate, AUENC_INIT *auenc_init)
{ 
#if AUDIO_EN
    uint8_t msi_isnew = 0;
    struct opus_encode_struct *opus_encode_s = NULL;

	struct msi *msi = msi_new("SR_OPUS_ENCODE", MAX_OPUS_ENCODE_RXBUF, &msi_isnew);
	if(msi && !msi_isnew) {
        opus_encode_s = (struct opus_encode_struct*)(msi->priv);
        if(opus_encode_s) {
            if(samplerate != opus_encode_s->samplerate) {
                OPUS_INFO("opus_encode_init conflict!\r\n");
                goto opus_encode_init_err;
            }               
        }      		
	}
    else if(msi && msi_isnew) {  
        opus_encode_s = (struct opus_encode_struct*)OPUS_CODE_ZALLOC(sizeof(struct opus_encode_struct));
        if(!opus_encode_s) {
            OPUS_INFO("opus_encode_s malloc fail!\r\n");
            goto opus_encode_init_err;
        }
        msi->priv = opus_encode_s;
        msi->action = (msi_action)opus_encode_msi_action;
        fbpool_init(&opus_encode_s->tx_pool, MAX_OPUS_ENCODE_TXBUF);
        for(uint32_t i=0; i<MAX_OPUS_ENCODE_TXBUF; i++) {
            struct framebuff *frame_buf = (opus_encode_s->tx_pool.pool)+i;
            frame_buf->data = NULL;
        }
        if(os_event_init(&opus_encode_s->event) != RET_OK) {
            OPUS_INFO("create opus encode event fail!\r\n");
            goto opus_encode_init_err;
        }
        if(auenc_init->src_msi && (msi_add_output(auenc_init->src_msi, NULL, msi->name) != RET_OK)) {
            goto opus_encode_init_err;
        }
        opus_encode_s->msi = msi;
        opus_encode_s->src_msi = auenc_init->src_msi;
        opus_encode_s->samplerate = samplerate;
        opus_encode_s->destroy_self = auenc_init->destroy_self;
		opus_encode_s->next_status = AUCODEC_RUN;
		opus_encode_s->current_status = AUCODEC_RUN;
    }
    else {
		OPUS_INFO("create opus encode msi fail!\r\n");
        return NULL;            
    }
    if(msi_isnew) {
#if OPUS_ENC_CTRL == AUCODER_RUN_IN_CPU1
        opus_encode_s->task_hdl = os_task_create("opus_encode_thread", opus_encode_thread, (void*)opus_encode_s, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 1024);
#else
        opus_encode_s->task_hdl = os_task_create("opus_encode_thread", opus_encode_thread, (void*)opus_encode_s, OS_TASK_PRIORITY_BELOW_NORMAL, 0, NULL, 3072);
#endif
        if(opus_encode_s->task_hdl == NULL)  {
			OPUS_INFO("create opus encode task fail!\r\n");
			goto opus_encode_init_err;
		}
	}
	return msi;
    
opus_encode_init_err:
	msi_destroy(msi);
#endif
	return NULL; 
}