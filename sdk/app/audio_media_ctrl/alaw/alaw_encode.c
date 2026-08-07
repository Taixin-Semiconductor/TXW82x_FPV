#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "osal_file.h"
#include "lib/audio/audio_code/audio_code.h"
#include "audio_code_ctrl.h"
#include "alaw_code.h"

#define MAX_ALAW_ENCODE_RXBUF    4
#define MAX_ALAW_ENCODE_TXBUF    4

struct alaw_encode_struct {
    struct fbpool tx_pool;
    struct os_event event;
    struct msi *msi;
    void *task_hdl; 
    uint8_t enc_buf[1024];
    uint32_t samplerate;
};
static struct alaw_encode_struct *alaw_encode_s = NULL;

static uint8_t next_status = AUDIO_STOP;
static uint8_t current_status = AUDIO_STOP;

static void alaw_encode_destroy(void);

uint8_t get_alaw_encode_status(void)
{
    return current_status;
}

static void set_alaw_encode_status(uint8_t status)
{
    next_status = status; 
}

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

    alaw_enc = audio_coder_open(ALAW_ENC, s->samplerate, 1);
    if (alaw_enc == NULL) 
        goto alaw_encode_thread_end;

    msi_get(s->msi);

    while(1) {
        os_event_wait(&s->event, clear_event, &clear_flag, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
        if(clear_flag & clear_event) {
            clear_flag = 0;
            clear_finish = 0;
        }
        if(next_status == AUDIO_PAUSE) {
            if(current_status == AUDIO_RUN) {
                audio_coder_close(alaw_enc);
                alaw_enc = audio_coder_open(ALAW_ENC, s->samplerate, 1);
                if (alaw_enc == NULL) 
                    goto alaw_encode_thread_end;
            }
            current_status = AUDIO_PAUSE;
        }
        recv_frame_buf = msi_get_fb(s->msi, 0);
        if(recv_frame_buf) {
            if(clear_finish == 0) {
                goto alaw_encode_frame_end;
            }
            if(next_status == AUDIO_PAUSE) {
                goto alaw_encode_frame_end;
            }
            else {
                current_status = AUDIO_RUN;     
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
                os_event_set(&s->event, clear_finish_event, NULL);
            }
            os_sleep_ms(1);
        }

        if(next_status == AUDIO_STOP) {
            goto alaw_encode_thread_end;
        }
    }
alaw_encode_thread_end:
    if(alaw_enc)
        audio_coder_close(alaw_enc);
    os_event_set(&s->event, exit_event, NULL);

    msi_put(s->msi);

    if(next_status != AUDIO_STOP)
        alaw_encode_destroy();    
}

static int32_t alaw_encode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
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
            if(alaw_encode_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                if(frame_buf->data) {
                    ALAW_CODE_FREE(frame_buf->data);
                    frame_buf->data = NULL;
                }
                fbpool_put(&alaw_encode_s->tx_pool, frame_buf);
            }
            ret = RET_OK+1;
            break; 
        }  
        case MSI_CMD_PRE_DESTROY:
        {
            if(alaw_encode_s) {
                if(alaw_encode_s->task_hdl) {
                    set_alaw_encode_status(AUDIO_STOP);
                }
            }
            ret = RET_OK;
            break;
        }      
		case MSI_CMD_POST_DESTROY:
        {
            if(alaw_encode_s) {
                if(alaw_encode_s->task_hdl) {
                    os_event_wait(&alaw_encode_s->event, exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
                for(uint32_t i=0; i<MAX_ALAW_ENCODE_TXBUF; i++) {
                    struct framebuff *frame_buf = (alaw_encode_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        ALAW_CODE_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&alaw_encode_s->tx_pool);
                if(alaw_encode_s->event.hdl)
                    os_event_del(&alaw_encode_s->event);
                ALAW_CODE_FREE(alaw_encode_s);
                alaw_encode_s = NULL;
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

int32_t alaw_encode_add_output(const char *msi_name)
{
    int32_t ret = RET_ERR;
	if(!msi_name) {
		ALAW_INFO("alaw encode add output fail,msi_name is null\n");
		return RET_ERR;
	}
	if(!alaw_encode_s) {
		ALAW_INFO("alaw encode add output fail,alaw_encode_s is null\n");
		return RET_ERR;
	}
    if(next_status == AUDIO_STOP) {
        ALAW_INFO("alaw encode add output fail,next_status is stop\n");
        return RET_ERR;
    }
	ret = msi_add_output(alaw_encode_s->msi, NULL, msi_name);
    return ret;
}

int32_t alaw_encode_del_output(const char *msi_name)
{
    int32_t ret = RET_ERR;
	if(!msi_name) {
		ALAW_INFO("alaw encode del output fail,msi_name is null\n");
		return RET_ERR;
	}
	if(!alaw_encode_s) {
		ALAW_INFO("alaw encode del output fail,alaw_encode_s is null\n");
		return RET_ERR;
	}
	ret = msi_del_output(alaw_encode_s->msi, NULL, msi_name);	
    return ret;	
}

static void alaw_encode_destroy(void)
{
    msi_destroy(alaw_encode_s->msi);
}

void alaw_encode_pause(void)
{
    set_alaw_encode_status(AUDIO_PAUSE);
}

void alaw_encode_continue(void)
{
    if(get_alaw_encode_status() == AUDIO_PAUSE)
        set_alaw_encode_status(AUDIO_RUN);
}

void alaw_encode_clear(void)
{
    if(alaw_encode_s) {
        os_event_set(&alaw_encode_s->event, clear_event, NULL);
        os_event_wait(&alaw_encode_s->event, clear_finish_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
    }
}

int32_t alaw_encode_deinit(void)
{
    int32_t ret = RET_ERR;

    if(!alaw_encode_s) {
        ALAW_INFO("alaw_encode_deinit fail,alaw_encode_s is null!\r\n");
        return RET_ERR;
    }
    alaw_encode_destroy();
    if(!alaw_encode_s)
        ret = RET_OK;
    return ret;
}

struct msi *alaw_encode_init(uint32_t samplerate)
{ 
#if AUDIO_EN
    uint8_t msi_isnew = 0;
    uint32_t count = 0; 

    while((next_status==AUDIO_STOP) && (current_status!=AUDIO_STOP) && (count < 2000)) {
        os_sleep_ms(1);
        count++;
    }
    if(count >= 2000) {
        ALAW_INFO("alaw encode init timeout!\r\n");
        return NULL;
    }
	struct msi *msi = msi_new("SR_ALAW_ENCODE", MAX_ALAW_ENCODE_RXBUF, &msi_isnew);
	if(msi && !msi_isnew) {
        if(alaw_encode_s) {
            if(samplerate != alaw_encode_s->samplerate) {
                ALAW_INFO("alaw_encode_init conflict!\r\n");
                msi_destroy(msi);
                return NULL;
            }               
        }
        else {
            ALAW_INFO("alaw_encode_init fail,alaw_encode_s is null!\r\n");
            msi_destroy(msi);
            return NULL;
        }          		
	}
    else if(msi && msi_isnew) {
        msi->enable = 1;
        msi->action = (msi_action)alaw_encode_msi_action;   
        alaw_encode_s = (struct alaw_encode_struct *)ALAW_CODE_ZALLOC(sizeof(struct alaw_encode_struct));
        if(!alaw_encode_s) {
            ALAW_INFO("alaw_encode_s malloc fail!\r\n");
            msi_destroy(msi);
            return NULL;	
        }
        alaw_encode_s->msi = msi;
        alaw_encode_s->samplerate = samplerate;
        fbpool_init(&alaw_encode_s->tx_pool, MAX_ALAW_ENCODE_TXBUF);
        for(uint32_t i=0; i<MAX_ALAW_ENCODE_TXBUF; i++) {
            struct framebuff *frame_buf = (alaw_encode_s->tx_pool.pool)+i;
            frame_buf->data = NULL;
        }
		next_status = AUDIO_RUN;
		current_status = AUDIO_RUN;
    }
    else {
        ALAW_INFO("create alaw encode msi fail!\r\n");
        return NULL;        
    }
    if(msi_isnew) {
        if(os_event_init(&alaw_encode_s->event) != RET_OK) {
            AAC_INFO("create alaw encode event fail!\r\n");
            goto alaw_encode_init_err;
        }
        alaw_encode_s->task_hdl = os_task_create("alaw_encode_thread", alaw_encode_thread, (void*)alaw_encode_s, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 512);
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