#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "osal_file.h"
#include "lib/audio/audio_code/audio_code.h"
#include "audio_code_ctrl.h"
#include "aac_code.h"

#define MAX_AAC_ENCODE_RXBUF    4
#define MAX_AAC_ENCODE_TXBUF    20

struct aac_encode_struct {
    struct fbpool tx_pool;
    struct os_mutex mutex;
    struct os_event event;
    struct msi *msi;
    void *task_hdl;
    void *aac_fp;    
    uint8_t direct_to_record;
    uint8_t stop_record;
    uint8_t output_msi_num;
    uint8_t enc_buf[1536];
	int16_t inbuf[1024];
    uint32_t samplerate;
};
static struct aac_encode_struct *aac_encode_s = NULL;

static uint8_t next_status = AUDIO_STOP;
static uint8_t current_status = AUDIO_STOP;

static void aac_encode_destroy(void);

uint8_t get_aac_encode_status(void)
{
    return current_status;
}

static void set_aac_encode_status(uint8_t status)
{
    next_status = status; 
}

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

    aac_enc = audio_coder_open(AAC_ENC, s->samplerate, 1);
    if (aac_enc == NULL) 
        goto aac_encode_thread_end;

    msi_get(s->msi);

    while(1) {
        os_event_wait(&s->event, clear_event, &clear_flag, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
        if(clear_flag & clear_event) {
            clear_flag = 0;
            clear_finish = 0;
        }
        if(next_status == AUDIO_PAUSE) { 
            if(current_status == AUDIO_RUN) {
                audio_coder_close(aac_enc);
                aac_enc = audio_coder_open(AAC_ENC, s->samplerate, 1);
                if (aac_enc == NULL) 
                    goto aac_encode_thread_end;
            }
            current_status = AUDIO_PAUSE;
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
            if(next_status == AUDIO_PAUSE) {          
                goto aac_encode_frame_end;
            }
            else {
                current_status = AUDIO_RUN;
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
						if(s->output_msi_num) {
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
								ret = msi_output_fb(s->msi, send_frame_buf); 
                                AAC_DEBUG("aac encode send framebuff:%p,ret:%d\r\n",send_frame_buf,ret);  
								send_frame_buf = NULL;   
							}
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
                os_event_set(&s->event, clear_finish_event, NULL);
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

        if(next_status == AUDIO_STOP) {
            goto aac_encode_thread_end;
        }
    }
aac_encode_thread_end:
    if(aac_enc)
        audio_coder_close(aac_enc);
    os_event_set(&s->event, exit_event, NULL);

    msi_put(s->msi);

    if(next_status != AUDIO_STOP)
        aac_encode_destroy();    
}

static int32_t aac_encode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;

    switch(cmd_id) {
        case MSI_CMD_TRANS_FB:
        {
            ret = RET_OK+1;
            struct framebuff *frame_buf = (struct framebuff *)param1;
            if(frame_buf->mtype == F_AUDIO) {
                ret = RET_OK;
            } 
            break;
        }            
        case MSI_CMD_FREE_FB:
        {
            if(aac_encode_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                if(frame_buf->data) {
                    AAC_CODE_FREE(frame_buf->data);
                    frame_buf->data = NULL;
                }
                fbpool_put(&aac_encode_s->tx_pool, frame_buf);
            }
            ret = RET_OK+1;
            break; 
        } 
        case MSI_CMD_PRE_DESTROY:
        {
            if(aac_encode_s) {
                if(aac_encode_s->task_hdl) {
                    set_aac_encode_status(AUDIO_STOP);
                }
            }
            ret = RET_OK;
            break;
        }       
		case MSI_CMD_POST_DESTROY:
        {
            if(aac_encode_s) {
                if(aac_encode_s->task_hdl) {
                    os_event_wait(&aac_encode_s->event, exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
                for(uint32_t i=0; i<MAX_AAC_ENCODE_TXBUF; i++) {
                    struct framebuff *frame_buf = (aac_encode_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        AAC_CODE_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&aac_encode_s->tx_pool);
                if(aac_encode_s->mutex.hdl)
                    os_mutex_del(&aac_encode_s->mutex);
                if(aac_encode_s->event.hdl)
                    os_event_del(&aac_encode_s->event);
                if(aac_encode_s->aac_fp) {
                    osal_fclose(aac_encode_s->aac_fp);
                    aac_encode_s->aac_fp = NULL;
                }
                AAC_CODE_FREE(aac_encode_s);
                aac_encode_s = NULL;
            }
            next_status = AUDIO_STOP;
            current_status = AUDIO_STOP;
            ret = RET_OK;
            break; 
        }         
        default:
            break;    
    }
    return ret;
}

int32_t aac_encode_add_output(const char *msi_name)
{
    int32_t ret = RET_ERR;
	if(!msi_name) {
		AAC_INFO("aac encode add output fail,msi_name is null\n");
		return RET_ERR;
	}
	if(!aac_encode_s) {
		AAC_INFO("aac encode add output fail,aac_encode_s is null\n");
		return RET_ERR;
	}
    if(next_status == AUDIO_STOP) {
        AAC_INFO("aac encode add output fail,next_status is stop\n");
        return RET_ERR;
    }
    if(aac_encode_s->mutex.hdl) {
        os_mutex_lock(&aac_encode_s->mutex, osWaitForever);
	}
	ret = msi_add_output(aac_encode_s->msi, NULL, msi_name);
    if(ret == RET_OK)
        aac_encode_s->output_msi_num++;	
    if(aac_encode_s->mutex.hdl)
        os_mutex_unlock(&aac_encode_s->mutex);
    return ret;
}

int32_t aac_encode_del_output(const char *msi_name)
{
    int32_t ret = RET_ERR;
	if(!msi_name) {
		AAC_INFO("aac encode del output fail,msi_name is null\n");
		return RET_ERR;
	}
	if(!aac_encode_s) {
		AAC_INFO("aac encode del output fail,aac_encode_s is null\n");
		return RET_ERR;
	}
    if(aac_encode_s->mutex.hdl) {
        os_mutex_lock(&aac_encode_s->mutex, osWaitForever);
	}
	ret = msi_del_output(aac_encode_s->msi, NULL, msi_name);	
    aac_encode_s->output_msi_num--;
    if(aac_encode_s->mutex.hdl) {
        os_mutex_unlock(&aac_encode_s->mutex);
    }
    return ret;	
}

static void aac_encode_destroy(void)
{
    msi_destroy(aac_encode_s->msi);
}

void aac_encode_pause(void)
{
    set_aac_encode_status(AUDIO_PAUSE);
}

void aac_encode_continue(void)
{
    if(get_aac_encode_status() == AUDIO_PAUSE)
        set_aac_encode_status(AUDIO_RUN);
}

void aac_encode_clear(void)
{
    if(aac_encode_s) {
        os_event_set(&aac_encode_s->event, clear_event, NULL);
        os_event_wait(&aac_encode_s->event, clear_finish_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
    }
}

int32_t aac_encode_deinit(uint8_t stop_record)
{
    int32_t ret = RET_ERR;

    if(!aac_encode_s) {
        AAC_INFO("aac_encode_deinit fail,aac_encode_s is null!\r\n");
        return RET_ERR;
    }
    if(stop_record) {
        aac_encode_s->stop_record = 1;
    }
    aac_encode_destroy();
    if(!aac_encode_s)
        ret = RET_OK;
    return ret;
}

struct msi *aac_encode_init(uint8_t *filename, uint32_t samplerate, uint8_t direct_to_record)
{ 
#if AUDIO_EN
    uint8_t msi_isnew = 0;
    uint32_t count = 0; 
    void *aac_fp = NULL;

    while((next_status==AUDIO_STOP) && (current_status!=AUDIO_STOP) && (count < 2000)) {
        os_sleep_ms(1);
        count++;
    }
    if(count >= 2000) {
        AAC_INFO("aac encode init timeout!\r\n");
        return NULL;
    }
	struct msi *msi = msi_new("SR_AAC_ENCODE", MAX_AAC_ENCODE_RXBUF, &msi_isnew);
	if(msi && !msi_isnew) {
        if(aac_encode_s) {
            if((samplerate != aac_encode_s->samplerate) || (direct_to_record && aac_encode_s->direct_to_record)) {
                AAC_INFO("aac_encode_init conflict!\r\n");
                msi_destroy(msi);
                return NULL;
            }               
        }
        else {
            AAC_INFO("aac_encode_init fail,aac_encode_s is null!\r\n");
            msi_destroy(msi);
            return NULL;
        }          		
	}
    else if(msi && msi_isnew) {
        msi->enable = 1;
        msi->action = (msi_action)aac_encode_msi_action;   
        aac_encode_s = (struct aac_encode_struct *)AAC_CODE_ZALLOC(sizeof(struct aac_encode_struct));
        if(!aac_encode_s) {
            AAC_INFO("aac_encode_s malloc fail!\r\n");
            msi_destroy(msi);
            return NULL;	
        }
        aac_encode_s->msi = msi;
        aac_encode_s->samplerate = samplerate;
        fbpool_init(&aac_encode_s->tx_pool, MAX_AAC_ENCODE_TXBUF);
        for(uint32_t i=0; i<MAX_AAC_ENCODE_TXBUF; i++) {
            struct framebuff *frame_buf = (aac_encode_s->tx_pool.pool)+i;
            frame_buf->data = NULL;
        }
		next_status = AUDIO_RUN;
		current_status = AUDIO_RUN;
    }
    else {
        AAC_INFO("create aac encode msi fail!\r\n");
        return NULL;
    }
	if(direct_to_record && filename) {
		aac_fp = osal_fopen((const char*)filename, "wb+");
		if(!aac_fp) {
            AAC_INFO("open aac record file %s fail!\r\n", filename);
			goto aac_encode_init_err;
        }
        aac_encode_s->direct_to_record = 1;
        aac_encode_s->aac_fp = aac_fp;
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
        if(os_mutex_init(&aac_encode_s->mutex) != RET_OK) {
            AAC_INFO("create aac encode mutex fail!\r\n");
            goto aac_encode_init_err;
        }
        if(os_event_init(&aac_encode_s->event) != RET_OK) {
            AAC_INFO("create aac encode event fail!\r\n");
            goto aac_encode_init_err;
        }
#if AAC_ENC == AUCODER_RUN_IN_CPU1
        aac_encode_s->task_hdl = os_task_create("aac_encode_thread", aac_encode_thread, (void*)aac_encode_s, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 512);
#else
        aac_encode_s->task_hdl = os_task_create("aac_encode_thread", aac_encode_thread, (void*)aac_encode_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 3072);
#endif
        if(aac_encode_s->task_hdl == NULL)  {
			AAC_INFO("create aac encode task fail!\r\n");
			goto aac_encode_init_err;
		}
	}
	os_printf("aac_encode_init success:%p\n",aac_encode_s);
	return msi;
    
aac_encode_init_err:
	if(aac_fp) { 
		osal_fclose(aac_fp);
		aac_fp = NULL;
        aac_encode_s->aac_fp = NULL;
	}
	msi_destroy(msi);
#endif
	return NULL;
}