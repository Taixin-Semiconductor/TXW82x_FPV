#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/audio/audio_code/audio_code.h"
#include "osal_file.h"
#include "aac_code.h"

#define MAX_AAC_ENCODE_RXBUF    4
#define MAX_AAC_ENCODE_TXBUF    20

struct aac_encode_struct {
    struct fbpool tx_pool;
    struct os_event event;
    struct msi *msi;
    struct msi *src_msi;
    void *task_hdl;
    void *aac_fp;    
    uint8_t direct_to_record;
    uint8_t stop_record;
    uint8_t destroy_self;
    uint8_t next_status;
    uint8_t current_status;
    uint8_t enc_buf[1536];
	int16_t inbuf[1024];
    uint32_t samplerate;
    AUDIO_INFO audio_info;
};

static void aac_encode_thread(void *d)
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
    uint32_t inbuf_reslen = 2048;
    uint32_t audio_time = 0;
    uint32_t clear_flag = 0;
    struct framebuff *send_frame_buf = NULL;
	struct framebuff *recv_frame_buf = NULL;
    struct aac_encode_struct *s = (struct aac_encode_struct *)d;
    AUCODE_HDL *aac_enc = NULL;

    s->msi->enable = 1;
    msi_get(s->msi);

    aac_enc = audio_coder_open(AAC_ENC, s->samplerate, 1);
    if (aac_enc == NULL) 
        goto aac_encode_thread_end;

    s->audio_info.nsamples = 1024;
    s->audio_info.time_interval = s->audio_info.nsamples * 1000 / s->samplerate;
    s->audio_info.samplerate = s->samplerate;

    while(1) {
        os_event_wait(&s->event, coder_clear_event, &clear_flag, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
        if(clear_flag & coder_clear_event) {
            clear_flag = 0;
            clear_finish = 0;
        }
        if(s->next_status == AUCODEC_PAUSE) { 
            if(s->current_status == AUCODEC_RUN) {
                audio_coder_close(aac_enc);
                aac_enc = audio_coder_open(AAC_ENC, s->samplerate, 1);
                if (aac_enc == NULL) 
                    goto aac_encode_thread_end;
            }
            s->current_status = AUCODEC_PAUSE;
            inbuf_reslen = 2048;
            inbuf_offset = 0;
        }
		recv_frame_buf = msi_get_fb(s->msi, 0);
        if(recv_frame_buf) {
            if(clear_finish == 0) {
                inbuf_reslen = 2048;
                inbuf_offset = 0;                
                goto aac_encode_frame_end;
            }
            if(s->next_status == AUCODEC_PAUSE) {          
                goto aac_encode_frame_end;
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
                    enc_bytes = audio_encode_data(aac_enc, s->inbuf, 1024, s->enc_buf);   
                    inbuf_reslen = 2048;
                    inbuf_offset = 0;
                    if(enc_bytes < 0) {
                        AAC_INFO("aac encode failed\n");
                        break;
                    }
                    if(enc_bytes > 0) {
						if(s->aac_fp)
							osal_fwrite(s->enc_buf, 1, enc_bytes, s->aac_fp);
                        send_frame_buf = fbpool_get(&s->tx_pool, 0, s->msi);
                        if(send_frame_buf) {
                            send_frame_buf->data = (uint8_t*)AAC_CODE_MALLOC(enc_bytes);
                            if(!send_frame_buf->data) {
                                AAC_INFO("aac encode malloc send_frame_buf->data fail!\n");
                                msi_delete_fb(s->msi, send_frame_buf);
                                send_frame_buf = NULL;
                                goto aac_encode_frame_end;
                            }
                            send_data = send_frame_buf->data;
                            os_memcpy(send_data, s->enc_buf, enc_bytes);
                            send_frame_buf->len = enc_bytes;
                            send_frame_buf->mtype = F_AUDIO;
                            send_frame_buf->time = audio_time;
                            send_frame_buf->priv = &(s->audio_info);
                            ret = msi_output_fb(s->msi, send_frame_buf); 
                            AAC_DEBUG("aac encode send framebuff:%p,ret:%d\r\n",send_frame_buf,ret);  
                            send_frame_buf = NULL;   
                        }
                        audio_time += (1024 * 1000 / s->samplerate);
                        if(data_len == 0)
                            update_audio_time = 1;
                    }
aac_encode_frame_end:
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
            AAC_DEBUG("aac encode delete framebuff:%p,len:%d,\r\n",recv_frame_buf,recv_frame_buf->len);
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

        if(s->stop_record) {
            if(s->aac_fp) {
                osal_fclose(s->aac_fp);
                s->aac_fp = NULL;
            }
            s->direct_to_record = 0;
            s->stop_record = 0;
        }

        if(s->next_status == AUCODEC_EXIT) {
            goto aac_encode_thread_end;
        }
    }
aac_encode_thread_end:
    if(aac_enc)
        audio_coder_close(aac_enc);
    os_event_set(&s->event, coder_exit_event, NULL);

    while((s->next_status != AUCODEC_EXIT) && (s->destroy_self == 0)) {
        s->current_status = AUCODEC_END;
        os_sleep_ms(5);
    }

    if(s->src_msi) {
        msi_del_output(s->src_msi, NULL, s->msi->name);
        s->src_msi = NULL;
    }

    if(s->next_status != AUCODEC_EXIT) {
        msi_destroy(s->msi);    
    }

    msi_put(s->msi);
}

static int32_t aac_encode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    struct aac_encode_struct *aac_encode_s = (struct aac_encode_struct*)(msi->priv);
    switch(cmd_id) {
		case MSI_CMD_AUCODER:
		{
            ret = RET_ERR;
			if(aac_encode_s) {
				uint32_t cmd_self = (uint32_t)param1;
				switch(cmd_self) {	
                    case MSI_AUCODER_PAUSE:
                    {
                        if(aac_encode_s->current_status == AUCODEC_RUN) {
                            aac_encode_s->next_status = AUCODEC_PAUSE;
                        }
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_CONTINUE:
                    {
                        if(aac_encode_s->current_status == AUCODEC_PAUSE) {
                            aac_encode_s->next_status = AUCODEC_RUN;
                        }
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_GET_STATUS:
                    {
                        *((uint32_t*)param2) = (uint32_t)(aac_encode_s->current_status);
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_CLEAR_STREAM:
                    {
                        os_event_set(&aac_encode_s->event, coder_clear_event, NULL);
                        os_event_wait(&aac_encode_s->event, coder_clear_finish_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                        ret = RET_OK;  
                        break;                                
                    }
                    case MSI_AUCODER_SET_SRCMSI:
                    {
                        if(aac_encode_s->src_msi) {
                            msi_del_output(aac_encode_s->src_msi, NULL, msi->name);
                        }
                        aac_encode_s->src_msi = NULL;
                        ret = msi_add_output((struct msi*)param2, NULL, msi->name);
                        if(ret == RET_OK) {
                            aac_encode_s->src_msi = (struct msi*)param2;
                        }
                        break;
                    }
					case MSI_AUCODER_DEINIT:
					{
                        aac_encode_s->stop_record = param2;
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
            if(aac_encode_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                if(frame_buf->data) {
                    AAC_CODE_FREE(frame_buf->data);
                    frame_buf->data = NULL;
                }
                fbpool_put(&aac_encode_s->tx_pool, frame_buf);
            }
            break; 
        } 
        case MSI_CMD_PRE_DESTROY:
        {
            if(aac_encode_s && aac_encode_s->task_hdl) {
                aac_encode_s->next_status = AUCODEC_EXIT;
            }
            break;
        }       
		case MSI_CMD_POST_DESTROY:
        {
            if(aac_encode_s) {
                if(aac_encode_s->task_hdl) {
                    os_event_wait(&aac_encode_s->event, coder_exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
                for(uint32_t i=0; i<MAX_AAC_ENCODE_TXBUF; i++) {
                    struct framebuff *frame_buf = (aac_encode_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        AAC_CODE_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&aac_encode_s->tx_pool);
                if(aac_encode_s->event.hdl) {
                    os_event_del(&aac_encode_s->event);
                }
                if(aac_encode_s->aac_fp) {
                    osal_fclose(aac_encode_s->aac_fp);
                    aac_encode_s->aac_fp = NULL;
                }
                AAC_CODE_FREE(aac_encode_s);
                aac_encode_s = NULL;
            }
            break; 
        }         
        default:
            break;    
    }
    return ret;
}

struct msi *aac_encode_init(char *filename, uint32_t samplerate, uint8_t direct_to_record, AUENC_INIT *auenc_init)
{ 
#if AUDIO_EN
    uint8_t msi_isnew = 0;
    struct aac_encode_struct *aac_encode_s = NULL;

	struct msi *msi = msi_new("SR_AAC_ENCODE", MAX_AAC_ENCODE_RXBUF, &msi_isnew);
	if(msi && !msi_isnew) {
        aac_encode_s = (struct aac_encode_struct*)(msi->priv);
        if(aac_encode_s) {
            if((samplerate != aac_encode_s->samplerate) || (direct_to_record && aac_encode_s->direct_to_record)) {
                AAC_INFO("aac_encode_init conflict!\r\n");
                goto aac_encode_init_err;
            }               
        }      		
	}
    else if(msi && msi_isnew) {  
        aac_encode_s = (struct aac_encode_struct *)AAC_CODE_ZALLOC(sizeof(struct aac_encode_struct));
        if(!aac_encode_s) {
            AAC_INFO("aac_encode_s malloc fail!\r\n");
            goto aac_encode_init_err;	
        }
        msi->priv = aac_encode_s;
        msi->action = (msi_action)aac_encode_msi_action; 
        fbpool_init(&aac_encode_s->tx_pool, MAX_AAC_ENCODE_TXBUF);
        for(uint32_t i=0; i<MAX_AAC_ENCODE_TXBUF; i++) {
            struct framebuff *frame_buf = (aac_encode_s->tx_pool.pool)+i;
            frame_buf->data = NULL;
        }
        if(os_event_init(&aac_encode_s->event) != RET_OK) {
            AAC_INFO("create aac encode event fail!\r\n");
            goto aac_encode_init_err;
        }
        if(auenc_init->src_msi && (msi_add_output(auenc_init->src_msi, NULL, msi->name) != RET_OK)) {
            goto aac_encode_init_err;
        }
        aac_encode_s->msi = msi;
        aac_encode_s->src_msi = auenc_init->src_msi;
        aac_encode_s->samplerate = samplerate;
        aac_encode_s->destroy_self = auenc_init->destroy_self;
		aac_encode_s->next_status = AUCODEC_RUN;
		aac_encode_s->current_status = AUCODEC_RUN;
    }
    else {
        AAC_INFO("create aac encode msi fail!\r\n");
        return NULL;
    }
	if(direct_to_record && filename) {
		aac_encode_s->aac_fp = osal_fopen((const char*)filename, "wb+");
		if(aac_encode_s->aac_fp == NULL) {
            AAC_INFO("open aac record file %s fail!\r\n", filename);
			goto aac_encode_init_err;
        }
        aac_encode_s->direct_to_record = 1;
	}
    else if(direct_to_record && !filename) {
        AAC_INFO("aac record file is null!\r\n");
        goto aac_encode_init_err;	
    }
    else if(!direct_to_record && filename) {
        AAC_INFO("aac record direct_to_record is 0!\r\n");
        goto aac_encode_init_err;
    }

    if(msi_isnew) {
#if AAC_ENC_CTRL == AUCODER_RUN_IN_CPU1
        aac_encode_s->task_hdl = os_task_create("aac_encode_thread", aac_encode_thread, (void*)aac_encode_s, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 1024);
#else
        aac_encode_s->task_hdl = os_task_create("aac_encode_thread", aac_encode_thread, (void*)aac_encode_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 3072);
#endif
        if(aac_encode_s->task_hdl == NULL)  {
			AAC_INFO("create aac encode task fail!\r\n");
			goto aac_encode_init_err;
		}
	}
	return msi;
    
aac_encode_init_err:
	msi_destroy(msi);
#endif
	return NULL;
}