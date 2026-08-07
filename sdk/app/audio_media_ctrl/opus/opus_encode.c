#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "osal_file.h"
#include "lib/audio/audio_code/audio_code.h"
#include "audio_code_ctrl.h"
#include "opus_code.h"

#define MAX_OPUS_ENCODE_RXBUF    4
#define MAX_OPUS_ENCODE_TXBUF    14

#define FRAME_SIZE               160

struct opus_encode_struct {
    struct fbpool tx_pool;
    struct os_event event;
    struct msi *msi;
    void *task_hdl;
    uint8_t enc_buf[1024];
    int16_t inbuf[FRAME_SIZE];
    uint32_t samplerate;
    uint32_t new_bitrate;
    uint32_t cur_bitrate;
};
static struct opus_encode_struct *opus_encode_s = NULL;

static uint8_t next_status = AUDIO_STOP;
static uint8_t current_status = AUDIO_STOP;

static void opus_encode_destroy(void);

uint8_t get_opus_encode_status(void)
{
    return current_status;
}

static void set_opus_encode_status(uint8_t status)
{
    next_status = status; 
}

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

    opus_enc = audio_coder_open(OPUS_ENC, s->samplerate, 1);
    if (opus_enc == NULL) 
        goto opus_encode_thread_end;

    msi_get(s->msi);

    while(1) {
        os_event_wait(&s->event, clear_event, &clear_flag, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
        if(clear_flag & clear_event) {
            clear_flag = 0;
            clear_finish = 0;
        }
        if(next_status == AUDIO_PAUSE) {
            if(current_status == AUDIO_RUN) {
                audio_coder_close(opus_enc);
                opus_enc = audio_coder_open(OPUS_ENC, s->samplerate, 1);
                if(opus_enc == NULL) 
                    goto opus_encode_thread_end;
            }
            current_status = AUDIO_PAUSE;
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
            if(next_status == AUDIO_PAUSE) { 
                goto opus_encode_frame_end;
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
                os_event_set(&s->event, clear_finish_event, NULL);
            }
            os_sleep_ms(1);
        }

        if(next_status == AUDIO_STOP) {
            goto opus_encode_thread_end;
        }
    }
opus_encode_thread_end:
    if(opus_enc)
        audio_coder_close(opus_enc);
    os_event_set(&s->event, exit_event, NULL);

    msi_put(s->msi);

    if(next_status != AUDIO_STOP)
        opus_encode_destroy();    
}

static int32_t opus_encode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
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
            if(opus_encode_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                if(frame_buf->data) {
                    OPUS_CODE_FREE(frame_buf->data);
                    frame_buf->data = NULL;
                }
                fbpool_put(&opus_encode_s->tx_pool, frame_buf);
            }
            ret = RET_OK+1;
            break; 
        }   
        case MSI_CMD_PRE_DESTROY:
        {
            if(opus_encode_s) {
                if(opus_encode_s->task_hdl) {
                    set_opus_encode_status(AUDIO_STOP);
                }
            }
            ret = RET_OK;
            break;
        }       
		case MSI_CMD_POST_DESTROY:
        {
            if(opus_encode_s) {
                if(opus_encode_s->task_hdl) {
                    os_event_wait(&opus_encode_s->event, exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
                for(uint32_t i=0; i<MAX_OPUS_ENCODE_TXBUF; i++) {
                    struct framebuff *frame_buf = (opus_encode_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        OPUS_CODE_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&opus_encode_s->tx_pool);
                if(opus_encode_s->event.hdl)
                    os_event_del(&opus_encode_s->event);   
                OPUS_CODE_FREE(opus_encode_s);
                opus_encode_s = NULL;
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

int32_t opus_encode_add_output(const char *msi_name)
{
    int32_t ret = RET_ERR;
	if(!msi_name) {
		OPUS_INFO("opus encode add output fail,msi_name is null\n");
		return RET_ERR;
	}
	if(!opus_encode_s) {
		OPUS_INFO("opus encode add output fail,opus_encode_s is null\n");
		return RET_ERR;
	}
    if(next_status == AUDIO_STOP) {
        OPUS_INFO("opus encode add output fail,next_status is stop\n");
        return RET_ERR;
    }
	ret = msi_add_output(opus_encode_s->msi, NULL, msi_name);
    return ret;
}

int32_t opus_encode_del_output(const char *msi_name)
{
    int32_t ret = RET_ERR;
	if(!msi_name) {
		OPUS_INFO("opus encode del output fail,msi_name is null\n");
		return RET_ERR;
	}
	if(!opus_encode_s) {
		OPUS_INFO("opus encode del output fail,opus_encode_s is null\n");
		return RET_ERR;
	}
	ret = msi_del_output(opus_encode_s->msi, NULL, msi_name);	
    return ret;	
}

static void opus_encode_destroy(void)
{
    msi_destroy(opus_encode_s->msi);
}

void opus_encode_pause(void)
{
    set_opus_encode_status(AUDIO_PAUSE);
}

void opus_encode_continue(void)
{
    if(get_opus_encode_status() == AUDIO_PAUSE)
        set_opus_encode_status(AUDIO_RUN);
}

void opus_encode_clear(void)
{
    if(opus_encode_s) {
        os_event_set(&opus_encode_s->event, clear_event, NULL);
        os_event_wait(&opus_encode_s->event, clear_finish_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
    }
}

int32_t opus_encode_set_bitrate(uint32_t bitrate)
{
    if(!opus_encode_s) {
        OPUS_INFO("opus_encode_set_bitrate fail,opus_encode_s is null!\r\n");
        return RET_ERR;
    } 
    opus_encode_s->new_bitrate = bitrate;
    return RET_OK;
}

int32_t opus_encode_deinit(void)
{
    int32_t ret = RET_ERR;

    if(!opus_encode_s) {
        OPUS_INFO("opus_encode_deinit fail,opus_encode_s is null!\r\n");
        return RET_ERR;
    }
    opus_encode_destroy();
    if(!opus_encode_s)
        ret = RET_OK;
    return ret;
}

struct msi *opus_encode_init(uint32_t samplerate)
{ 
#if AUDIO_EN
    uint8_t msi_isnew = 0;
    uint32_t count = 0; 

    while((next_status==AUDIO_STOP) && (current_status!=AUDIO_STOP) && (count < 2000)) {
        os_sleep_ms(1);
        count++;
    }
    if(count >= 2000) {
        OPUS_INFO("opus encode init timeout!\r\n");
        return NULL;
    }
	struct msi *msi = msi_new("SR_OPUS_ENCODE", MAX_OPUS_ENCODE_RXBUF, &msi_isnew);
	if(msi && !msi_isnew) {
        if(opus_encode_s) {
            if(samplerate != opus_encode_s->samplerate) {
                OPUS_INFO("opus_encode_init conflict!\r\n");
                msi_destroy(msi);
                return NULL;
            }               
        }
        else {
            OPUS_INFO("opus_encode_init fail,opus_encode_s is null!\r\n");
            msi_destroy(msi);
            return NULL;
        }          		
	}
    else if(msi && msi_isnew) {
        msi->enable = 1;
        msi->action = (msi_action)opus_encode_msi_action;   
        opus_encode_s = (struct opus_encode_struct *)OPUS_CODE_ZALLOC(sizeof(struct opus_encode_struct));
        if(!opus_encode_s) {
            OPUS_INFO("opus_encode_s malloc fail!\r\n");
            msi_destroy(msi);
            return NULL;	
        }
        opus_encode_s->msi = msi;
        opus_encode_s->samplerate = samplerate;
        fbpool_init(&opus_encode_s->tx_pool, MAX_OPUS_ENCODE_TXBUF);
        for(uint32_t i=0; i<MAX_OPUS_ENCODE_TXBUF; i++) {
            struct framebuff *frame_buf = (opus_encode_s->tx_pool.pool)+i;
            frame_buf->data = NULL;
        }
		next_status = AUDIO_RUN;
		current_status = AUDIO_RUN;
    }
    else {
		OPUS_INFO("create opus encode msi fail!\r\n");
        return NULL;            
    }
    if(msi_isnew) {
        if(os_event_init(&opus_encode_s->event) != RET_OK) {
            AAC_INFO("create opus encode event fail!\r\n");
            goto opus_encode_init_err;
        }
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