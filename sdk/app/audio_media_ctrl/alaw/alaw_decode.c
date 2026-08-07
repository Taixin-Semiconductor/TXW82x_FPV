#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "osal_file.h"
#include "lib/audio/audio_code/audio_code.h"
#include "audio_code_ctrl.h"
#include "alaw_code.h"

#define MAX_ALAW_DECODE_RXBUF    4
#define MAX_ALAW_DECODE_TXBUF    4

struct alaw_decode_struct {
    struct fbpool tx_pool;
    struct os_event event;
    struct msi *msi;
    void *task_hdl;
    uint8_t direct_to_dac;
    int16_t dec_buf[960];  //按照16k、60ms的最大长度，若超过该长度则需修改数组大小;
    uint32_t cur_sampleRate; 
};
static struct alaw_decode_struct *alaw_decode_s = NULL; 

static uint8_t next_status = AUDIO_STOP;
static uint8_t current_status = AUDIO_STOP;
static void alaw_decode_destroy(void);

uint8_t get_alaw_decode_status(void)
{
    return current_status;
}

static void set_alaw_decode_status(uint8_t status)
{
    next_status = status; 
}

static void alaw_decode(struct alaw_decode_struct *s)
{
    uint8_t clear_finish = 1;
    uint8_t *enc_ptr = NULL;
	int32_t ret = 0;
    int32_t data_len = 0;
    int32_t dec_samples = 0;
    uint32_t clear_flag = 0;
    struct framebuff *recv_frame_buf = NULL;
    struct framebuff *send_frame_buf = NULL;
    AUCODE_HDL *alaw_dec = NULL;
    AUCODE_FRAME_INFO alaw_info;
     
    alaw_dec = audio_coder_open(ALAW_DEC, 8000, 1);
    if(!alaw_dec) 
        return;
    while(1) {
        os_event_wait(&s->event, clear_event, &clear_flag, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
        if(clear_flag & clear_event) {
            clear_flag = 0;
            clear_finish = 0;
        }
        if(next_status == AUDIO_PAUSE) {
            if(current_status == AUDIO_RUN) {
                if(s->direct_to_dac)
                    msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,0);
                audio_coder_close(alaw_dec);  
                alaw_dec = audio_coder_open(ALAW_DEC, 8000, 1);
                if(!alaw_dec) 
                    return;  
            }  
            current_status = AUDIO_PAUSE;          
        }
        recv_frame_buf = msi_get_fb(s->msi, 0);
        if(recv_frame_buf) {
            if(clear_finish == 0) {
                goto alaw_decode_frame_end;
            }
            if(next_status == AUDIO_PAUSE) {
                goto alaw_decode_frame_end; 
            }
            else {
                current_status = AUDIO_RUN;
                while(!send_frame_buf) {
                    send_frame_buf = fbpool_get(&s->tx_pool, 0, s->msi);
                    if(!send_frame_buf)
                        os_sleep_ms(1); 
                }
                data_len = recv_frame_buf->len;
                if(data_len > 0) {
                    enc_ptr = recv_frame_buf->data;
                    dec_samples = audio_decode_data(alaw_dec, enc_ptr, data_len, s->dec_buf, &alaw_info); 
                }
                if((dec_samples <= 0) || (data_len <= 0)) {
                    msi_delete_fb(s->msi, send_frame_buf);
                    send_frame_buf = NULL;
                    goto alaw_decode_frame_end;
                }
                send_frame_buf->data = (uint8_t*)ALAW_CODE_MALLOC(dec_samples*2);
                if(!send_frame_buf->data) {
                    ALAW_INFO("alaw decode malloc send_frame_buf->data fail!\n");
                    msi_delete_fb(s->msi, send_frame_buf);
                    send_frame_buf = NULL;
                    goto alaw_decode_frame_end;                    
                }
                os_memcpy(send_frame_buf->data, s->dec_buf, dec_samples*2);
                send_frame_buf->len = dec_samples*2;
                send_frame_buf->mtype = F_AUDIO;
                send_frame_buf->stype = FSTYPE_AUDIO_ALAW_DECODER;
                if((s->direct_to_dac) && (s->cur_sampleRate != alaw_info.samplerate)) {
                    s->cur_sampleRate = alaw_info.samplerate;
                    msi_output_cmd(s->msi, MSI_CMD_AUDAC, MSI_AUDAC_SET_SAMPLING_RATE, s->cur_sampleRate);
                }
                ret = msi_output_fb(s->msi, send_frame_buf);   
                ALAW_DEBUG("alaw decode send framebuff:%p,ret:%d\r\n",send_frame_buf,ret);
                send_frame_buf = NULL;                 
            }
alaw_decode_frame_end:
            ALAW_DEBUG("alaw decode delete framebuff:%p,len:%d,\r\n",recv_frame_buf,recv_frame_buf->len);
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
            goto alaw_decode_end;
        }
    }
alaw_decode_end:
    if(s->direct_to_dac)
	    msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,0);
	if(alaw_dec)
		audio_coder_close(alaw_dec);
}

static void alaw_decode_thread(void *d)
{
	int32_t ret = 0;
    uint32_t former_dac_filter_type = 0;
    uint32_t former_dac_samplingrate = 0;
    struct alaw_decode_struct *s = (struct alaw_decode_struct *)d;

    msi_get(s->msi);

    if(s->direct_to_dac) {
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_GET_FILTER_TYPE,(uint32_t)(&former_dac_filter_type));
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TYPE,FSTYPE_AUDIO_ALAW_DECODER);
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_GET_SAMPLING_RATE,(uint32_t)(&former_dac_samplingrate));
        s->cur_sampleRate = former_dac_samplingrate;
    }

    alaw_decode(s);

    if(s->direct_to_dac) {
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_SAMPLING_RATE,former_dac_samplingrate);
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TYPE,former_dac_filter_type);
    }

    os_event_set(&s->event, exit_event, NULL);

    msi_put(s->msi);

    if(next_status != AUDIO_STOP)
        alaw_decode_destroy();
}

static int32_t alaw_decode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
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
            if(alaw_decode_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                if(frame_buf->data) {
                    ALAW_CODE_FREE(frame_buf->data);
                    frame_buf->data = NULL;
                }
                fbpool_put(&alaw_decode_s->tx_pool, frame_buf);
            }
            ret = RET_OK+1;
            break; 
        }  
        case MSI_CMD_PRE_DESTROY:
        {
            if(alaw_decode_s) {
                if(alaw_decode_s->task_hdl) {
                    set_alaw_decode_status(AUDIO_STOP);
                }
            }
            ret = RET_OK;
            break;
        }      
		case MSI_CMD_POST_DESTROY:
        {
            if(alaw_decode_s) {
                if(alaw_decode_s->task_hdl) {
                    os_event_wait(&alaw_decode_s->event, exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
                for(uint32_t i=0; i<MAX_ALAW_DECODE_TXBUF; i++) {
                    struct framebuff *frame_buf = (alaw_decode_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        ALAW_CODE_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&alaw_decode_s->tx_pool);
                if(alaw_decode_s->event.hdl)
                    os_event_del(&alaw_decode_s->event);
                ALAW_CODE_FREE(alaw_decode_s);
                alaw_decode_s = NULL;
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

int32_t alaw_decode_add_output(const char *msi_name)
{
    int32_t ret = RET_ERR;
	if(!msi_name) {
		ALAW_INFO("alaw decode add output fail,msi_name is null\n");
		return RET_ERR;
	}
	if(!alaw_decode_s) {
		ALAW_INFO("alaw decode add output fail,alaw_decode_s is null\n");
		return RET_ERR;
	}
    if(next_status == AUDIO_STOP) {
        ALAW_INFO("alaw decode add output fail,next_status is stop\n");
        return RET_ERR;
    }
	ret = msi_add_output(alaw_decode_s->msi, NULL, msi_name);	
    return ret;	
}

int32_t alaw_decode_del_output(const char *msi_name)
{
    int32_t ret = RET_ERR;
	if(!msi_name) {
		ALAW_INFO("alaw decode del output fail,msi_name is null\n");
		return RET_ERR;
	}
	if(!alaw_decode_s) {
		ALAW_INFO("alaw decode del output fail,alaw_decode_s is null\n");
		return RET_ERR;
	}
	ret = msi_del_output(alaw_decode_s->msi, NULL, msi_name);
    return ret;	
}

static void alaw_decode_destroy(void)
{
	msi_destroy(alaw_decode_s->msi);
}

void alaw_decode_pause(void)
{
    set_alaw_decode_status(AUDIO_PAUSE);
}

void alaw_decode_continue(void)
{
    if(get_alaw_decode_status() == AUDIO_PAUSE)
        set_alaw_decode_status(AUDIO_RUN);
}

void alaw_decode_clear(void)
{
    if(alaw_decode_s) {
        os_event_set(&alaw_decode_s->event, clear_event, NULL);
        os_event_wait(&alaw_decode_s->event, clear_finish_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
    }
}

int32_t alaw_decode_deinit(void)
{
    if(!alaw_decode_s) {
        ALAW_INFO("alaw_decode_deinit fail,alaw_decode_s is null!\r\n");
        return RET_ERR;
    }
    alaw_decode_destroy();
    return RET_OK;
}

struct msi *alaw_decode_init(uint8_t direct_to_dac)
{
#if AUDIO_EN
    uint8_t msi_isnew = 0;
	uint32_t count = 0;
	struct framebuff *frame_buf = NULL;

    while((get_alaw_decode_status() != AUDIO_STOP) && count < 2000) {
        os_sleep_ms(1);
        count++;
    }
    if(count >= 2000) {
        ALAW_INFO("alaw decode init timeout!\r\n");
        return NULL;
    }
	struct msi *msi = msi_new("SR_ALAW_DECODE", MAX_ALAW_DECODE_RXBUF, &msi_isnew);
	if(!msi) {
		ALAW_INFO("create alaw decode msi fail!\r\n");
		return NULL;
	}
    else if(!msi_isnew) {
        ALAW_INFO("alaw decode msi has been create!\r\n");
        msi_destroy(msi);
        return NULL;
    }
    if(direct_to_dac)
	    msi_add_output(msi, NULL, "R_AUDAC");
	msi->enable = 1;
	msi->action = (msi_action)alaw_decode_msi_action;        
	alaw_decode_s = (struct alaw_decode_struct *)ALAW_CODE_ZALLOC(sizeof(struct alaw_decode_struct));
	if(!alaw_decode_s) {
		ALAW_INFO("alaw_decode_s malloc fail!\r\n");
		goto alaw_decode_init_err;
	}
	alaw_decode_s->msi = msi;
    alaw_decode_s->direct_to_dac = direct_to_dac;
	fbpool_init(&alaw_decode_s->tx_pool, MAX_ALAW_DECODE_TXBUF);
	for(uint32_t i=0; i<MAX_ALAW_DECODE_TXBUF; i++) {
		frame_buf->data = NULL;
	}
    if(os_event_init(&alaw_decode_s->event) != RET_OK) {
        AAC_INFO("create alaw decode event fail!\r\n");
        goto alaw_decode_init_err;
    }
	next_status = AUDIO_RUN;
	current_status = AUDIO_RUN;
    alaw_decode_s->task_hdl = os_task_create("alaw_decode_thread", alaw_decode_thread, (void*)alaw_decode_s, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 512);
	if(alaw_decode_s->task_hdl == NULL)  {
		ALAW_INFO("create opus decode task fail!\r\n");
		goto alaw_decode_init_err;
	}
	return msi;
	
alaw_decode_init_err:
	msi_destroy(msi);
#endif
    return NULL;
}