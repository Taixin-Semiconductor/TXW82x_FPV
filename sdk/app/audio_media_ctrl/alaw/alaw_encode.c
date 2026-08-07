#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/audio/audio_code/audio_code.h"
#include "alaw_code.h"

#define MAX_ALAW_ENCODE_RXBUF    4
#define MAX_ALAW_ENCODE_TXBUF    4

struct alaw_encode_struct {
    struct fbpool tx_pool;
    struct os_event event;
    struct msi *msi;
    struct msi *src_msi;
    void *task_hdl; 
    uint8_t destroy_self;
    uint8_t next_status;
    uint8_t current_status;
    uint8_t enc_buf[1024];
    uint32_t samplerate;
    AUDIO_INFO audio_info;
};

static void alaw_encode_thread(void *d)
{
    uint8_t clear_finish = 1;
    uint8_t *send_data = NULL;
	int16_t *recv_data = NULL;
    int32_t enc_bytes = 0;
    int32_t ret = 0;
    uint32_t data_len = 0;
    uint32_t clear_flag = 0;
    struct framebuff *send_frame_buf = NULL;
	struct framebuff *recv_frame_buf = NULL;
    struct alaw_encode_struct *s = (struct alaw_encode_struct *)d;
    AUCODE_HDL *alaw_enc = NULL;

    s->msi->enable = 1;
    msi_get(s->msi);

    alaw_enc = audio_coder_open(ALAW_ENC, s->samplerate, 1);
    if (alaw_enc == NULL) 
        goto alaw_encode_thread_end;

    while(1) {
        os_event_wait(&s->event, coder_clear_event, &clear_flag, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
        if(clear_flag & coder_clear_event) {
            clear_flag = 0;
            clear_finish = 0;
        }
        if(s->next_status == AUCODEC_PAUSE) {
            if(s->current_status == AUCODEC_RUN) {
                audio_coder_close(alaw_enc);
                alaw_enc = audio_coder_open(ALAW_ENC, s->samplerate, 1);
                if (alaw_enc == NULL) 
                    goto alaw_encode_thread_end;
            }
            s->current_status = AUCODEC_PAUSE;
        }
        recv_frame_buf = msi_get_fb(s->msi, 0);
        if(recv_frame_buf) { 
            if(s->audio_info.nsamples == 0) {
                s->audio_info.nsamples = recv_frame_buf->len / 2;
                s->audio_info.time_interval = s->audio_info.nsamples * 1000 / s->samplerate;
                s->audio_info.samplerate = s->samplerate;
            }
            if(clear_finish == 0) {
                goto alaw_encode_frame_end;
            }
            if(s->next_status == AUCODEC_PAUSE) {
                goto alaw_encode_frame_end;
            }
            else {
                s->current_status = AUCODEC_RUN;     
                recv_data = (int16_t*)recv_frame_buf->data;
                data_len = recv_frame_buf->len;
                enc_bytes = audio_encode_data(alaw_enc, (int16_t*)recv_data, data_len/2, s->enc_buf);   
                send_frame_buf = fbpool_get(&s->tx_pool, 0, s->msi);
                if(send_frame_buf) {
                    send_frame_buf->data = (uint8_t*)ALAW_CODE_MALLOC(enc_bytes);
                    if(!send_frame_buf->data) {
                        ALAW_INFO("alaw encode malloc send_frame_buf->data fail!\n");
                        msi_delete_fb(s->msi, send_frame_buf);
                        send_frame_buf = NULL;
                        goto alaw_encode_frame_end;
                    }
                    send_data = send_frame_buf->data;
                    os_memcpy(send_data, s->enc_buf, enc_bytes);
                    send_frame_buf->len = enc_bytes;
                    send_frame_buf->mtype = F_AUDIO;
                    send_frame_buf->time = recv_frame_buf->time;
                    send_frame_buf->priv = &(s->audio_info);
                    ret = msi_output_fb(s->msi, send_frame_buf); 
                    ALAW_DEBUG("alaw encode send framebuff:%p,ret:%d\r\n",send_frame_buf,ret);  
                    send_frame_buf = NULL;   
                }
            }
alaw_encode_frame_end:
            ALAW_DEBUG("alaw encode delete framebuff:%p,len:%d,\r\n",recv_frame_buf,recv_frame_buf->len);
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
            s->current_status = AUCODEC_EXIT;
            goto alaw_encode_thread_end;
        }
    }
alaw_encode_thread_end:
    if(alaw_enc)
        audio_coder_close(alaw_enc);
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

static int32_t alaw_encode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    struct alaw_encode_struct *alaw_encode_s = (struct alaw_encode_struct*)(msi->priv);
    switch(cmd_id) {
		case MSI_CMD_AUCODER:
		{
            ret = RET_ERR;
			if(alaw_encode_s) {
				uint32_t cmd_self = (uint32_t)param1;
				switch(cmd_self) {	
                    case MSI_AUCODER_PAUSE:
                    {
                        if(alaw_encode_s->current_status == AUCODEC_RUN) {
                            alaw_encode_s->next_status = AUCODEC_PAUSE;
                        }
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_CONTINUE:
                    {
                        if(alaw_encode_s->current_status == AUCODEC_PAUSE) {
                            alaw_encode_s->next_status = AUCODEC_RUN;
                        }
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_GET_STATUS:
                    {
                        *((uint32_t*)param2) = (uint32_t)(alaw_encode_s->current_status);
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_CLEAR_STREAM:
                    {
                        os_event_set(&alaw_encode_s->event, coder_clear_event, NULL);
                        os_event_wait(&alaw_encode_s->event, coder_clear_finish_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                        ret = RET_OK;  
                        break;                                
                    }
                    case MSI_AUCODER_SET_SRCMSI:
                    {
                        if(alaw_encode_s->src_msi) {
                            msi_del_output(alaw_encode_s->src_msi, NULL, msi->name);
                        }
                        alaw_encode_s->src_msi = NULL;
                        ret = msi_add_output((struct msi*)param2, NULL, msi->name);
                        if(ret == RET_OK) {
                            alaw_encode_s->src_msi = (struct msi*)param2;
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
            if(alaw_encode_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                if(frame_buf->data) {
                    ALAW_CODE_FREE(frame_buf->data);
                    frame_buf->data = NULL;
                }
                fbpool_put(&alaw_encode_s->tx_pool, frame_buf);
            }
            break; 
        }  
        case MSI_CMD_PRE_DESTROY:
        {
            if(alaw_encode_s && alaw_encode_s->task_hdl) {
                alaw_encode_s->next_status = AUCODEC_EXIT;
            }
            break;
        }      
		case MSI_CMD_POST_DESTROY:
        {
            if(alaw_encode_s) {
                if(alaw_encode_s->task_hdl) {
                    os_event_wait(&alaw_encode_s->event, coder_exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
                for(uint32_t i=0; i<MAX_ALAW_ENCODE_TXBUF; i++) {
                    struct framebuff *frame_buf = (alaw_encode_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        ALAW_CODE_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&alaw_encode_s->tx_pool);
                if(alaw_encode_s->event.hdl) {
                    os_event_del(&alaw_encode_s->event);
                }
                ALAW_CODE_FREE(alaw_encode_s);
                alaw_encode_s = NULL;
            }
            break; 
        }         
        default:
            break;    
    }
    return ret;
}

struct msi *alaw_encode_init(uint32_t samplerate, AUENC_INIT *auenc_init)
{ 
#if AUDIO_EN
    uint8_t msi_isnew = 0;
    struct alaw_encode_struct *alaw_encode_s = NULL;

	struct msi *msi = msi_new("SR_ALAW_ENCODE", MAX_ALAW_ENCODE_RXBUF, &msi_isnew);
	if(msi && !msi_isnew) {
        alaw_encode_s = (struct alaw_encode_struct*)(msi->priv);
        if(alaw_encode_s) {
            if(samplerate != alaw_encode_s->samplerate) {
                ALAW_INFO("alaw_encode_init conflict!\r\n");
                goto alaw_encode_init_err;
            }               
        }        		
	}
    else if(msi && msi_isnew) {   
        alaw_encode_s = (struct alaw_encode_struct *)ALAW_CODE_ZALLOC(sizeof(struct alaw_encode_struct));
        if(!alaw_encode_s) {
            ALAW_INFO("alaw_encode_s malloc fail!\r\n");
            goto alaw_encode_init_err;
        }
        msi->priv = alaw_encode_s;
        msi->action = (msi_action)alaw_encode_msi_action;
        fbpool_init(&alaw_encode_s->tx_pool, MAX_ALAW_ENCODE_TXBUF);
        for(uint32_t i=0; i<MAX_ALAW_ENCODE_TXBUF; i++) {
            struct framebuff *frame_buf = (alaw_encode_s->tx_pool.pool)+i;
            frame_buf->data = NULL;
        }
        if(os_event_init(&alaw_encode_s->event) != RET_OK) {
            ALAW_INFO("create alaw encode event fail!\r\n");
            goto alaw_encode_init_err;
        }
        if(auenc_init->src_msi && (msi_add_output(auenc_init->src_msi, NULL, msi->name) != RET_OK)) {
            goto alaw_encode_init_err;
        }
        alaw_encode_s->msi = msi;
        alaw_encode_s->src_msi = auenc_init->src_msi;
        alaw_encode_s->samplerate = samplerate;
        alaw_encode_s->destroy_self = auenc_init->destroy_self;
		alaw_encode_s->next_status = AUCODEC_RUN;
		alaw_encode_s->current_status = AUCODEC_RUN;
    }
    else {
        ALAW_INFO("create alaw encode msi fail!\r\n");
        return NULL;        
    }
    if(msi_isnew) {
        alaw_encode_s->task_hdl = os_task_create("alaw_encode_thread", alaw_encode_thread, (void*)alaw_encode_s, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 1024);
		if(alaw_encode_s->task_hdl == NULL)  {
			ALAW_INFO("create alaw encode task fail!\r\n");
			goto alaw_encode_init_err;
		}
	}
	return msi;
    
alaw_encode_init_err:
	msi_destroy(msi);
#endif
	return NULL; 
}