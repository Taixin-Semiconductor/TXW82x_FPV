#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "osal_file.h"
#include "lib/audio/audio_code/audio_code.h"
#include "audio_code_ctrl.h"
#include "opus_code.h"

#define MAX_OPUS_DECODE_RXBUF    4
#define MAX_OPUS_DECODE_TXBUF    4

struct opus_decode_struct {
    struct fbpool tx_pool;
    struct os_event event;
    struct msi *msi;
    void *task_hdl;
    uint8_t direct_to_dac;
    int16_t dec_buf[960];  //按照16k、60ms的最大长度，若超过该长度则需修改数组大小;
    uint32_t cur_sampleRate;
    uint32_t coder_sampleRate;
};
static struct opus_decode_struct *opus_decode_s = NULL; 

static uint8_t next_status = AUDIO_STOP;
static uint8_t current_status = AUDIO_STOP;
static void opus_decode_destroy(void);

uint8_t get_opus_decode_status(void)
{
    return current_status;
}

static void set_opus_decode_status(uint8_t status)
{
    next_status = status; 
}

static void opus_decode(struct opus_decode_struct *s)
{
    uint8_t clear_finish = 1;
    uint8_t *enc_ptr = NULL;
	int32_t ret = 0;
    int32_t data_len = 0;
    int32_t dec_samples = 0;
    uint32_t dec_operation = 0;
    uint32_t clear_flag = 0;
    struct framebuff *recv_frame_buf = NULL;
    struct framebuff *send_frame_buf = NULL;
    AUCODE_HDL *opus_dec = NULL;
    AUCODE_FRAME_INFO opus_info;
     
    opus_dec = audio_coder_open(OPUS_DEC, s->coder_sampleRate, 1);
    if(!opus_dec) 
        return;
    while(1) {
        os_event_wait(&s->event, clear_event, &clear_flag, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
        if(clear_flag & clear_event) {
            clear_flag = 0;
            clear_finish = 0;
        }
        if(next_status == AUDIO_PAUSE) {
            if(current_status == AUDIO_RUN) {
                audio_coder_close(opus_dec);
                if(s->direct_to_dac)
                    msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,0);
                opus_dec = audio_coder_open(OPUS_DEC, s->coder_sampleRate, 1);
                if(!opus_dec) 
                    return;
            }
            current_status = AUDIO_PAUSE;
        }   
        recv_frame_buf = msi_get_fb(s->msi, 0);
        if(recv_frame_buf) {
            if(clear_finish == 0) {
                goto opus_decode_frame_end;
            }
            if(next_status == AUDIO_PAUSE) {
                goto opus_decode_frame_end;
            }   
            else {
                current_status = AUDIO_RUN;
                while(!send_frame_buf) {
                    send_frame_buf = fbpool_get(&s->tx_pool, 0, s->msi);
                    if(!send_frame_buf)
                        os_sleep_ms(1); 
                }
                if(recv_frame_buf->priv)
                    dec_operation = ((AUDECODER_OPERATION*)recv_frame_buf->priv)->decode_operation;
                if(dec_operation == PACKET_LOSS_CONCEALMENT)
                    dec_samples = audio_decode_do_plc(opus_dec, s->dec_buf);
                else if(dec_operation == OUTPUT_MUTE_DATA)
                    dec_samples = audio_decode_output_mute(opus_dec, s->dec_buf);
                else {
                    if(dec_operation == DECODE_ADD_FADE_IN)
                        audio_decode_fade_in(opus_dec);
                    else if(dec_operation == DECODE_ADD_FADE_OUT)
                        audio_decode_fade_out(opus_dec);
                    data_len = recv_frame_buf->len;
                    if(data_len > 0) {
                        enc_ptr = recv_frame_buf->data;
                        dec_samples = audio_decode_data(opus_dec, enc_ptr, data_len, s->dec_buf, &opus_info); 
                    }
                    else {
                        msi_delete_fb(s->msi, send_frame_buf);
                        send_frame_buf = NULL;
                        goto opus_decode_frame_end;              
                    }
                }
                if(dec_samples <= 0) {
                    msi_delete_fb(s->msi, send_frame_buf);
                    send_frame_buf = NULL;
                    goto opus_decode_frame_end;
                }
                send_frame_buf->data = (uint8_t*)OPUS_CODE_MALLOC(dec_samples*2);
                if(!send_frame_buf->data) {
                    OPUS_INFO("opus decode malloc send_frame_buf->data fail!\n");
                    msi_delete_fb(s->msi, send_frame_buf);
                    send_frame_buf = NULL;
                    goto opus_decode_frame_end;                    
                }
                os_memcpy(send_frame_buf->data, s->dec_buf, dec_samples*2);
                send_frame_buf->len = dec_samples*2;
                send_frame_buf->mtype = F_AUDIO;
                send_frame_buf->stype = FSTYPE_AUDIO_OPUS_DECODER;
                if((s->direct_to_dac) && (s->cur_sampleRate != opus_info.samplerate)) {
                    s->cur_sampleRate = opus_info.samplerate;
                    msi_output_cmd(s->msi, MSI_CMD_AUDAC, MSI_AUDAC_SET_SAMPLING_RATE, s->cur_sampleRate);
                }
                ret = msi_output_fb(s->msi, send_frame_buf);   
                OPUS_DEBUG("opus decode send framebuff:%p,ret:%d\r\n",send_frame_buf,ret);
                send_frame_buf = NULL;              
            }
opus_decode_frame_end:
            OPUS_DEBUG("opus decode delete framebuff:%p,len:%d,\r\n",recv_frame_buf,recv_frame_buf->len);
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
            goto opus_decode_end;
        }
    }    
opus_decode_end:
    if(s->direct_to_dac)
	    msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,0);
	if(opus_dec)
		audio_coder_close(opus_dec);
}

static void opus_decode_thread(void *d)
{
	int32_t ret = 0;
    uint32_t former_dac_filter_type = 0;
    uint32_t former_dac_samplingrate = 0;
    struct opus_decode_struct *s = (struct opus_decode_struct *)d;

    msi_get(s->msi);

    if(s->direct_to_dac) {
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_GET_FILTER_TYPE,(uint32_t)(&former_dac_filter_type));
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TYPE,FSTYPE_AUDIO_OPUS_DECODER);
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_GET_SAMPLING_RATE,(uint32_t)(&former_dac_samplingrate));
        s->cur_sampleRate = former_dac_samplingrate;
    }

    opus_decode(s);

    if(s->direct_to_dac) {
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_SAMPLING_RATE,former_dac_samplingrate);
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TYPE,former_dac_filter_type);
    }

    os_event_set(&s->event, exit_event, NULL);

    msi_put(s->msi);

    if(next_status != AUDIO_STOP)
        opus_decode_destroy();
}

static int32_t opus_decode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
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
            if(opus_decode_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                if(frame_buf->data) {
                    OPUS_CODE_FREE(frame_buf->data);
                    frame_buf->data = NULL;
                }
                fbpool_put(&opus_decode_s->tx_pool, frame_buf);
            }
            ret = RET_OK+1;
            break; 
        }
        case MSI_CMD_PRE_DESTROY:
        {
            if(opus_decode_s) {
                if(opus_decode_s->task_hdl) {
                    set_opus_decode_status(AUDIO_STOP);
                }
            }
            ret = RET_OK;
            break;
        }         
		case MSI_CMD_POST_DESTROY:
        {
            if(opus_decode_s) {
                if(opus_decode_s->task_hdl) {
                    os_event_wait(&opus_decode_s->event, exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
                for(uint32_t i=0; i<MAX_OPUS_DECODE_TXBUF; i++) {
                    struct framebuff *frame_buf = (opus_decode_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        OPUS_CODE_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&opus_decode_s->tx_pool);
                if(opus_decode_s->event.hdl)
                    os_event_del(&opus_decode_s->event);
                OPUS_CODE_FREE(opus_decode_s);
                opus_decode_s = NULL;
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

int32_t opus_decode_add_output(const char *msi_name)
{
    int32_t ret = RET_ERR;
	if(!msi_name) {
		OPUS_INFO("opus decode add output fail,msi_name is null\n");
		return RET_ERR;
	}
	if(!opus_decode_s) {
		OPUS_INFO("opus decode add output fail,opus_decode_s is null\n");
		return RET_ERR;
	}
    if(next_status == AUDIO_STOP) {
        OPUS_INFO("opus decode add output fail,next_status is stop\n");
        return RET_ERR;
    }
	ret = msi_add_output(opus_decode_s->msi, NULL, msi_name);	
    return ret;	
}

int32_t opus_decode_del_output(const char *msi_name)
{
    int32_t ret = RET_ERR;
	if(!msi_name) {
		OPUS_INFO("opus decode del output fail,msi_name is null\n");
		return RET_ERR;
	}
	if(!opus_decode_s) {
		OPUS_INFO("opus decode del output fail,opus_decode_s is null\n");
		return RET_ERR;
	}
	ret = msi_del_output(opus_decode_s->msi, NULL, msi_name);	
    return ret;	
}

static void opus_decode_destroy(void)
{
	msi_destroy(opus_decode_s->msi);
}

void opus_decode_pause(void)
{
    set_opus_decode_status(AUDIO_PAUSE);
}

void opus_decode_continue(void)
{
    if(get_opus_decode_status() == AUDIO_PAUSE)
        set_opus_decode_status(AUDIO_RUN);
}

void opus_decode_clear(void)
{
    if(opus_decode_s) {
        os_event_set(&opus_decode_s->event, clear_event, NULL);
        os_event_wait(&opus_decode_s->event, clear_finish_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
    }
}

int32_t opus_decode_deinit(void)
{
    if(!opus_decode_s) {
        OPUS_INFO("opus_decode_deinit fail,opus_decode_s is null!\r\n");
        return RET_ERR;
    }
    opus_decode_destroy();
    return RET_OK;
}

struct msi *opus_decode_init(uint32_t samplerate, uint8_t direct_to_dac)
{
#if AUDIO_EN
    uint8_t msi_isnew = 0;
	uint32_t count = 0;

    while((get_opus_decode_status() != AUDIO_STOP) && count < 2000) {
        os_sleep_ms(1);
        count++;
    }
    if(count >= 2000) {
        OPUS_INFO("opus decode init timeout!\r\n");
        return NULL;
    }

	struct msi *msi = msi_new("SR_OPUS_DECODE", MAX_OPUS_DECODE_RXBUF, &msi_isnew);
	if(!msi) {
		OPUS_INFO("create opus decode msi fail!\r\n");
		return NULL;
	}
    else if(!msi_isnew) {
		OPUS_INFO("opus decode msi has been create!\r\n");
        msi_destroy(msi);
		return NULL;        
    }
    if(direct_to_dac)
	    msi_add_output(msi, NULL, "R_AUDAC");
	msi->enable = 1;
	msi->action = (msi_action)opus_decode_msi_action;        
	opus_decode_s = (struct opus_decode_struct *)OPUS_CODE_ZALLOC(sizeof(struct opus_decode_struct));
	if(!opus_decode_s) {
		OPUS_INFO("opus_decode_s malloc fail!\r\n");
		goto opus_decode_init_err;
	}
	opus_decode_s->msi = msi;
    opus_decode_s->direct_to_dac = direct_to_dac;
    opus_decode_s->coder_sampleRate = samplerate;
	fbpool_init(&opus_decode_s->tx_pool, MAX_OPUS_DECODE_TXBUF);
	for(uint32_t i=0; i<MAX_OPUS_DECODE_TXBUF; i++) {
        struct framebuff *frame_buf = (opus_decode_s->tx_pool.pool)+i;
		frame_buf->data = NULL;
	}
    if(os_event_init(&opus_decode_s->event) != RET_OK) {
        AAC_INFO("create opus decode event fail!\r\n");
        goto opus_decode_init_err;
    }
    next_status = AUDIO_RUN;
	current_status = AUDIO_RUN;
#if OPUS_DEC_CTRL == AUCODER_RUN_IN_CPU1
    opus_decode_s->task_hdl = os_task_create("opus_decode_thread", opus_decode_thread, (void*)opus_decode_s, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 1024);
#else
    opus_decode_s->task_hdl = os_task_create("opus_decode_thread", opus_decode_thread, (void*)opus_decode_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);
#endif
    if(opus_decode_s->task_hdl == NULL)  {
		OPUS_INFO("create opus decode task fail!\r\n");
		goto opus_decode_init_err;
	}
	return msi;
	
opus_decode_init_err:
	msi_destroy(msi);
#endif
    return NULL;
}