#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "osal_file.h"
#include "lib/audio/audio_code/audio_code.h"
#include "audio_code_ctrl.h"
#include "aac_code.h"

#define BUFF_SIZE   2048
#define MAX_AAC_DECODE_RXBUF    4
#define MAX_AAC_DECODE_TXBUF    4

struct aac_decode_struct {
    struct fbpool tx_pool;
    struct os_event event;
    struct msi *msi; 
    void *task_hdl;
    void *aac_fp;
    uint8_t direct_to_dac;
    uint8_t inbuf[BUFF_SIZE];
    int16_t dec_buf[1024*2];
    uint32_t cur_sampleRate;
};
static struct aac_decode_struct *aac_decode_s = NULL;

static uint8_t next_status = AUDIO_STOP;
static uint8_t current_status = AUDIO_STOP;
static void aac_decode_destroy(void);

uint8_t get_aac_decode_status(void)
{
    return current_status;
}

static void set_aac_decode_status(uint8_t status)
{
    next_status = status; 
}

static int32_t aac_file_read(uint8_t *buf, uint32_t size)
{
    int32_t read_len = 0;
	
    read_len = osal_fread(buf, size, 1, aac_decode_s->aac_fp);
    return read_len;
}

static void aac_file_decode(struct aac_decode_struct *s)
{
    uint8_t endOfRead = 0;
    uint8_t *enc_ptr = NULL;
    int16_t *data = NULL;
	int32_t ret = 0;
    int32_t read_len = 0;
    int32_t dec_samples = 0;
    int32_t unproc_data_size = 0;
    uint32_t enc_ptr_offset = 0;
    uint32_t bytes_to_read = 0;
    struct framebuff *frame_buf = NULL;
    AUCODE_HDL *aac_dec = NULL;
    AUCODE_FRAME_INFO aac_info;
     
    aac_dec = audio_coder_open(AAC_DEC, 0, 0);
    if(!aac_dec) 
        return;
    enc_ptr = s->inbuf;
    while(1) {
        if(next_status == AUDIO_PAUSE) {
            current_status = AUDIO_PAUSE;
            while(next_status == AUDIO_PAUSE) {
                os_sleep_ms(1);
            }
        }

        if(next_status == AUDIO_STOP) {
            goto aac_decode_end;
        }
        current_status = AUDIO_RUN;

        if(unproc_data_size < 1536 && !endOfRead) {
            bytes_to_read = BUFF_SIZE - unproc_data_size;
			if(unproc_data_size)
				os_memcpy(s->inbuf, enc_ptr+enc_ptr_offset, unproc_data_size);
            read_len = aac_file_read(s->inbuf+unproc_data_size, bytes_to_read);
            if(read_len <= 0) {
                AAC_INFO("aac read fail,ret:%d,line:%d!\r\n",read_len,__LINE__);
                endOfRead = 1;
            }   
            else
                unproc_data_size += read_len;
            enc_ptr = s->inbuf;
            enc_ptr_offset = 0;
        }

        if(unproc_data_size > 0) {
            while(!frame_buf) {
                frame_buf = fbpool_get(&s->tx_pool, 0, s->msi);
                if(!frame_buf)
                    os_sleep_ms(1); 
            }
            data = (int16_t*)frame_buf->data;
            dec_samples = audio_decode_data(aac_dec, enc_ptr+enc_ptr_offset, unproc_data_size, s->dec_buf, &aac_info);
            if(dec_samples <= 0) {
                enc_ptr_offset += 1;
                unproc_data_size -= 1;
                msi_delete_fb(s->msi, frame_buf);
                frame_buf = NULL;
                continue;
            }
            if(aac_info.channels == 2) {
                for(uint32_t i=0; i<dec_samples; i++) 
                    data[i] = s->dec_buf[2*i];
            }
			else {
				for(uint32_t i=0; i<dec_samples; i++)
					data[i] = s->dec_buf[i];
			}
            enc_ptr_offset += aac_info.frame_bytes;
            unproc_data_size -= aac_info.frame_bytes;
            frame_buf->len = dec_samples*2;
            frame_buf->mtype = F_AUDIO;
            frame_buf->stype = FSTYPE_AUDIO_AAC_DECODER;
            if((s->direct_to_dac) && (s->cur_sampleRate != aac_info.samplerate)) {
                s->cur_sampleRate = aac_info.samplerate;
                msi_output_cmd(s->msi, MSI_CMD_AUDAC, MSI_AUDAC_SET_SAMPLING_RATE, s->cur_sampleRate);
            }
            ret = msi_output_fb(s->msi, frame_buf);   
			AAC_DEBUG("aac decode send framebuff:%p,ret:%d\r\n",frame_buf,ret);
            frame_buf = NULL;                 
        }
        else if(endOfRead) {
            goto aac_decode_end;
        }      
    }
aac_decode_end:
    if(s->direct_to_dac)
	    msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,0);
	if(aac_dec)
		audio_coder_close(aac_dec);
}

static void aac_msi_decode(struct aac_decode_struct *s)
{
    uint8_t clear_finish = 1;
    uint8_t *enc_ptr = NULL;
    int16_t *send_data = NULL;
	int32_t ret = 0;
    int32_t data_len = 0;
    int32_t dec_samples = 0;
    uint32_t clear_flag = 0;
    struct framebuff *recv_frame_buf = NULL;
    struct framebuff *send_frame_buf = NULL;
    AUCODE_HDL *aac_dec = NULL;
    AUCODE_FRAME_INFO aac_info;
     
    aac_dec = audio_coder_open(AAC_DEC, 0, 1);
    if(!aac_dec) 
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
                aac_dec = audio_coder_open(AAC_DEC, 0, 1);
                if(!aac_dec) 
                    return;
            }
            current_status = AUDIO_PAUSE;
        }
        recv_frame_buf = msi_get_fb(s->msi, 0);
        if(recv_frame_buf) {
            if(clear_finish == 0) {
                goto aac_decode_frame_end;
            }
            if(current_status == AUDIO_PAUSE) {
                goto aac_decode_frame_end;
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
                    dec_samples = audio_decode_data(aac_dec, enc_ptr, data_len, s->dec_buf, &aac_info); 
                }
                if((dec_samples <= 0) || (data_len <= 0)) {
                    msi_delete_fb(s->msi, send_frame_buf);
                    send_frame_buf = NULL;
                    goto aac_decode_frame_end;
                }
                send_data = (int16_t*)send_frame_buf->data;
                if(aac_info.channels == 2) {
                    for(uint32_t i=0; i<dec_samples; i++) 
                        send_data[i] = s->dec_buf[2*i];
                }
                else {
                    for(uint32_t i=0; i<dec_samples; i++)
                        send_data[i] = s->dec_buf[i];
                }
                send_frame_buf->len = dec_samples*2;
                send_frame_buf->mtype = F_AUDIO;
                send_frame_buf->stype = FSTYPE_AUDIO_AAC_DECODER;
                if((s->direct_to_dac) && (s->cur_sampleRate != aac_info.samplerate)) {
                    s->cur_sampleRate = aac_info.samplerate;
                    msi_output_cmd(s->msi, MSI_CMD_AUDAC, MSI_AUDAC_SET_SAMPLING_RATE, s->cur_sampleRate);
                }
                ret = msi_output_fb(s->msi, send_frame_buf); 
                AAC_DEBUG("aac decode send framebuff:%p,ret:%d\r\n",send_frame_buf,ret);
                send_frame_buf = NULL;      
            }        
aac_decode_frame_end:
            AAC_DEBUG("aac decode delete framebuff:%p,len:%d,\r\n",recv_frame_buf,recv_frame_buf->len);
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
            goto aac_decode_end;
        }
    }    
aac_decode_end:
    if(s->direct_to_dac)
	    msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,0);
	if(aac_dec)
		audio_coder_close(aac_dec);    
}

static void aac_decode_thread(void *d)
{
	int32_t ret = 0;
    uint32_t former_dac_filter_type = 0;
    uint32_t former_dac_samplingrate = 0;
    struct aac_decode_struct *s = (struct aac_decode_struct *)d;

    msi_get(s->msi);

    if(s->direct_to_dac) {
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_GET_FILTER_TYPE,(uint32_t)(&former_dac_filter_type));
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TYPE,FSTYPE_AUDIO_AAC_DECODER);
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_GET_SAMPLING_RATE,(uint32_t)(&former_dac_samplingrate));
        s->cur_sampleRate = former_dac_samplingrate;
    }

    if(s->aac_fp)
        aac_file_decode(s);
    else
        aac_msi_decode(s);

    if(s->direct_to_dac) {
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_SAMPLING_RATE,former_dac_samplingrate);
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TYPE,former_dac_filter_type);
    }

    os_event_set(&s->event, exit_event, NULL);

    msi_put(s->msi);

    if(next_status != AUDIO_STOP)
        aac_decode_destroy();
}

static int32_t aac_decode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
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
            if(aac_decode_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                fbpool_put(&aac_decode_s->tx_pool, frame_buf);
            }
            ret = RET_OK+1;
            break; 
        }  
        case MSI_CMD_PRE_DESTROY:
        {
            if(aac_decode_s) {
                if(aac_decode_s->task_hdl) {
                    set_aac_decode_status(AUDIO_STOP);
                }
            }
            ret = RET_OK;
            break;
        }   
		case MSI_CMD_POST_DESTROY:
        {
            if(aac_decode_s) {
                if(aac_decode_s->task_hdl) {
                    os_event_wait(&aac_decode_s->event, exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
                for(uint32_t i=0; i<MAX_AAC_DECODE_TXBUF; i++) {
                    struct framebuff *frame_buf = (aac_decode_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        AAC_CODE_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&aac_decode_s->tx_pool);
                if(aac_decode_s->event.hdl)
                    os_event_del(&aac_decode_s->event);
                if(aac_decode_s->aac_fp) {
                    osal_fclose(aac_decode_s->aac_fp);
                    aac_decode_s->aac_fp = NULL;
                }
                AAC_CODE_FREE(aac_decode_s);
                aac_decode_s = NULL;
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

int32_t aac_decode_add_output(const char *msi_name)
{
    int32_t ret = RET_ERR;
	if(!msi_name) {
		AAC_INFO("aac decode add output fail,msi_name is null\n");
		return RET_ERR;
	}
	if(!aac_decode_s) {
		AAC_INFO("aac decode add output fail,aac_decode_s is null\n");
		return RET_ERR;
	}
    if(next_status == AUDIO_STOP) {
        AAC_INFO("aac decode add output fail,next_status is stop\n");
        return RET_ERR;
    }
	ret = msi_add_output(aac_decode_s->msi, NULL, msi_name);	
    return ret;	
}

int32_t aac_decode_del_output(const char *msi_name)
{
    int32_t ret = RET_ERR;
	if(!msi_name) {
		AAC_INFO("aac decode del output fail,msi_name is null\n");
		return RET_ERR;
	}
	if(!aac_decode_s) {
		AAC_INFO("aac decode del output fail,aac_decode_s is null\n");
		return RET_ERR;
	}
	ret = msi_del_output(aac_decode_s->msi, NULL, msi_name);
    return ret;	
}

static void aac_decode_destroy(void)
{
	msi_destroy(aac_decode_s->msi);
}

void aac_decode_pause(void)
{
    set_aac_decode_status(AUDIO_PAUSE);
}

void aac_decode_continue(void)
{
    if(get_aac_decode_status() == AUDIO_PAUSE)
        set_aac_decode_status(AUDIO_RUN);
}

void aac_decode_clear(void)
{
    if(aac_decode_s) {
        os_event_set(&aac_decode_s->event, clear_event, NULL);
        os_event_wait(&aac_decode_s->event, clear_finish_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
    }
}

int32_t aac_decode_deinit(void)
{
    if(!aac_decode_s) {
        AAC_INFO("aac_decode_deinit fail,aac_decode_s is null!\r\n");
        return RET_ERR;
    }
    aac_decode_destroy();
    return RET_OK;
}

struct msi *aac_decode_init(uint8_t *filename, uint8_t direct_to_dac)
{
#if AUDIO_EN
    uint8_t msi_isnew = 0;
    uint8_t *data = NULL;
	uint32_t count = 0;
	struct framebuff *frame_buf = NULL;
    void *aac_fp = NULL;

    while((get_aac_decode_status() != AUDIO_STOP) && count < 2000) {
        os_sleep_ms(1);
        count++;
    }
    if(count >= 2000) {
        AAC_INFO("aac decode init timeout!\r\n");
        return NULL;
    }
	struct msi *msi = msi_new("SR_AAC_DECODE", MAX_AAC_DECODE_RXBUF, &msi_isnew);
	if(!msi) {
		AAC_INFO("create aac decode msi fail!\r\n");
		return NULL;
	}
    else if(!msi_isnew) {
        AAC_INFO("aac decode msi has been create!\r\n");
        msi_destroy(msi);
        return NULL;
    }
    if(direct_to_dac)
	    msi_add_output(msi, NULL, "R_AUDAC");
	msi->enable = 1;
	msi->action = (msi_action)aac_decode_msi_action;        
	aac_decode_s = (struct aac_decode_struct *)AAC_CODE_ZALLOC(sizeof(struct aac_decode_struct));
	if(!aac_decode_s) {
		AAC_INFO("aac_decode_s malloc fail!\r\n");
		goto aac_decode_init_err;
	}
	fbpool_init(&aac_decode_s->tx_pool, MAX_AAC_DECODE_TXBUF);
	for(uint32_t i=0; i<MAX_AAC_DECODE_TXBUF; i++) {
		frame_buf = (aac_decode_s->tx_pool.pool)+i;
		frame_buf->data = NULL;
		data = (uint8_t*)AAC_CODE_MALLOC(1024 * sizeof(int16_t));
		if(!data)
		{
			AAC_INFO("aac decode malloc framebuff data fail!\r\n");
			goto aac_decode_init_err;       
		}  
		frame_buf->data = data;  
	}
	if(filename) {
		aac_fp = osal_fopen((const char*)filename, "rb");
		if(!aac_fp) {
            AAC_INFO("open aac file %s fail!\r\n", filename);
			return NULL;
        }	
        aac_decode_s->aac_fp = aac_fp;
	}
	aac_decode_s->msi = msi;
    aac_decode_s->direct_to_dac = direct_to_dac;
    if(os_event_init(&aac_decode_s->event) != RET_OK) {
        AAC_INFO("create aac decode event fail!\r\n");
        goto aac_decode_init_err;
    }
    next_status = AUDIO_RUN;
	current_status = AUDIO_RUN;
#if AAC_DEC == AUCODER_RUN_IN_CPU1
    aac_decode_s->task_hdl = os_task_create("aac_decode_thread", aac_decode_thread, (void*)aac_decode_s, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 512);
#else
    aac_decode_s->task_hdl = os_task_create("aac_decode_thread", aac_decode_thread, (void*)aac_decode_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);
#endif
    if(aac_decode_s->task_hdl == NULL)  {
		AAC_INFO("create aac decode task fail!\r\n");
		goto aac_decode_init_err;
	}
	return msi;
	
aac_decode_init_err:
	if(aac_fp) {
		osal_fclose(aac_fp);
		aac_fp = NULL;
        aac_decode_s->aac_fp = NULL;
	}
	msi_destroy(msi);
#endif
    return NULL;
}