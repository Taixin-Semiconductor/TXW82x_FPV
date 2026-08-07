#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/audio/audio_code/audio_code.h"
#include "autpc_msi/autpc_msi.h"
#include "osal_file.h"
#include "aac_code.h"

#define BUFF_SIZE   2048
#define MAX_AAC_DECODE_RXBUF    4
#define MAX_AAC_DECODE_TXBUF    4

struct aac_decode_struct {
    AUDIO_TRACK audio_track;
    struct fbpool tx_pool;
    struct os_event event;
    struct msi *msi; 
    struct msi *src_msi;
    struct msi *autpc_msi;
    char *msi_name;
    void *task_hdl;
    void *aac_fp;
    uint8_t loop_mode;
    uint8_t direct_to_dac;
	uint8_t use_tpc;
	uint8_t speed;
	uint8_t pitch;
    uint8_t destroy_self;
    uint8_t next_status;
    uint8_t current_status;
    uint8_t inbuf[BUFF_SIZE];
    int16_t dec_buf[1024*2];
};

static int32_t aac_file_read(struct aac_decode_struct *aac_decode_s, uint8_t *buf, uint32_t size)
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
    uint32_t interval_time = 0;
    struct framebuff *frame_buf = NULL;
    AUCODE_HDL *aac_dec = NULL;
    AUCODE_FRAME_INFO aac_info;
	AUDIO_TRACK *audac_fiter_track = NULL;
     
    aac_dec = audio_coder_open(AAC_DEC, 0, 0);
    if(!aac_dec) 
        return;
    enc_ptr = s->inbuf;
    while(1) {
        if(s->next_status == AUCODEC_PAUSE) {
            if(s->current_status == AUCODEC_RUN) {
                msi_output_cmd(s->msi,MSI_CMD_AUTPC,MSI_AUTPC_END_STREAM,(uint32_t)(&(s->audio_track)));
                msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,(uint32_t)(&(s->audio_track)));
            }
            s->current_status = AUCODEC_PAUSE;
            while(s->next_status == AUCODEC_PAUSE) {
                os_sleep_ms(1);
            }
        }

        if(s->next_status == AUCODEC_EXIT) {
            s->current_status = AUCODEC_EXIT;
            goto aac_decode_end;
        }
        s->current_status = AUCODEC_RUN;

        if(unproc_data_size < 1536 && !endOfRead) {
            bytes_to_read = BUFF_SIZE - unproc_data_size;
			if(unproc_data_size)
				os_memcpy(s->inbuf, enc_ptr+enc_ptr_offset, unproc_data_size);
            read_len = aac_file_read(s, s->inbuf+unproc_data_size, bytes_to_read);
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
            frame_buf->priv = &s->audio_track;
            frame_buf->len = dec_samples*2;
            frame_buf->mtype = F_AUDIO;
            frame_buf->stype = FSTYPE_AUDIO_PCM;
            s->audio_track.samplerate = aac_info.samplerate;
            if(s->use_tpc && !s->autpc_msi) {
                s->autpc_msi = autpc_msi_init(s->audio_track.samplerate, s->speed, s->pitch, dec_samples, &(s->audio_track));
                if(s->autpc_msi == NULL) {
                    goto aac_decode_end;
                }
                if(s->direct_to_dac) {
                    msi_add_output(s->autpc_msi, NULL, "R_AUDAC");
                }
                msi_add_output(s->msi, NULL, s->autpc_msi->name);
            }
            ret = msi_output_fb(s->msi, frame_buf);  
            AAC_DEBUG("aac decode send framebuff:%p,ret:%d\r\n",frame_buf,ret);
            msi_cmd("R_AUDAC", MSI_CMD_AUDAC, MSI_AUDAC_GET_FILTER_TRACK, (uint32_t)(&audac_fiter_track));
            if(s->direct_to_dac && (!s->use_tpc) && (audac_fiter_track != (&(s->audio_track)))) {
                interval_time = (frame_buf->len>>1)*1000/(aac_info.samplerate);
                os_sleep_ms(interval_time);
            }  
            frame_buf = NULL;                 
        }
        else if(endOfRead) {
            goto aac_decode_end;
        }      
    }
aac_decode_end:
    msi_output_cmd(s->msi,MSI_CMD_AUTPC,MSI_AUTPC_END_STREAM,(uint32_t)(&(s->audio_track)));
	msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,(uint32_t)(&(s->audio_track)));
    if(frame_buf) {
        msi_delete_fb(s->msi, frame_buf);
        frame_buf = NULL;
    }
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
        os_event_wait(&s->event, coder_clear_event, &clear_flag, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
        if(clear_flag & coder_clear_event) {
            clear_flag = 0;
            clear_finish = 0;
        }
        if(s->next_status == AUCODEC_PAUSE) {
            if(s->current_status == AUCODEC_RUN) {
                msi_output_cmd(s->msi,MSI_CMD_AUTPC,MSI_AUTPC_END_STREAM,(uint32_t)(&(s->audio_track)));
                msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,(uint32_t)(&(s->audio_track)));
                aac_dec = audio_coder_open(AAC_DEC, 0, 1);
                if(!aac_dec) 
                    return;
            }
            s->current_status = AUCODEC_PAUSE;
        }
        recv_frame_buf = msi_get_fb(s->msi, 0);
        if(recv_frame_buf) {
            if(clear_finish == 0) {
                goto aac_decode_frame_end;
            }
            if(s->current_status == AUCODEC_PAUSE) {
                goto aac_decode_frame_end;
            }
            else {
                s->current_status = AUCODEC_RUN;
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
                send_frame_buf->priv = &s->audio_track;
                send_frame_buf->len = dec_samples*2;
                send_frame_buf->mtype = F_AUDIO;
                send_frame_buf->stype = FSTYPE_AUDIO_PCM;
                s->audio_track.samplerate = aac_info.samplerate;
                if(s->use_tpc && !s->autpc_msi) {
                    s->autpc_msi = autpc_msi_init(s->audio_track.samplerate, s->speed, s->pitch, dec_samples, &(s->audio_track));
                    if(s->autpc_msi == NULL) {
                        goto aac_decode_end;    
                    }
                    if(s->direct_to_dac) {
                        msi_add_output(s->autpc_msi, NULL, "R_AUDAC");
                    }
                    msi_add_output(s->msi, NULL, s->autpc_msi->name);
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
                os_event_set(&s->event, coder_clear_finish_event, NULL);
            }
            os_sleep_ms(1);
        }

        if(s->next_status == AUCODEC_EXIT) {
            s->current_status = AUCODEC_EXIT;
            goto aac_decode_end;
        }
    }    
aac_decode_end:
    msi_output_cmd(s->msi,MSI_CMD_AUTPC,MSI_AUTPC_END_STREAM,(uint32_t)(&(s->audio_track)));
	msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,(uint32_t)(&(s->audio_track)));
    if(recv_frame_buf) {
        msi_delete_fb(NULL, recv_frame_buf);
		recv_frame_buf = NULL;
	}
    if(send_frame_buf) {
        msi_delete_fb(s->msi, send_frame_buf);
        send_frame_buf = NULL;
    }
    if(aac_dec)
		audio_coder_close(aac_dec);   
}
static void aac_decode_thread(void *d)
{
    struct aac_decode_struct *s = (struct aac_decode_struct *)d;

    msi_get(s->msi);

    if(s->direct_to_dac) {
        msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TRACK,(uint32_t)(&(s->audio_track)));
    }

    s->msi->enable = 1;
    if(s->aac_fp) {
        do {
            osal_fseek(s->aac_fp, 0);
            aac_file_decode(s);
            if(s->current_status == AUCODEC_EXIT) {
                break;
            }
        }while(s->loop_mode);
    }
    else
        aac_msi_decode(s);

    if(s->direct_to_dac) {
        s->audio_track.priority &= 0x3F;
        msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TRACK,(uint32_t)(&(s->audio_track)));
    }

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

static int32_t aac_decode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    struct aac_decode_struct *aac_decode_s = (struct aac_decode_struct*)(msi->priv);
    switch(cmd_id) {
		case MSI_CMD_AUCODER:
		{
            ret = RET_ERR;
			if(aac_decode_s) {
				uint32_t cmd_self = (uint32_t)param1;
				switch(cmd_self) {	
                    case MSI_AUCODER_PAUSE:
                    {
                        if(aac_decode_s->current_status == AUCODEC_RUN) {
                            aac_decode_s->next_status = AUCODEC_PAUSE;
                        }
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_CONTINUE:
                    {
                        if(aac_decode_s->current_status == AUCODEC_PAUSE) {
                            aac_decode_s->next_status = AUCODEC_RUN;
                        }
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_GET_STATUS:
                    {
                        *((uint32_t*)param2) = (uint32_t)(aac_decode_s->current_status);
                        ret = RET_OK;  
                        break;                      
                    }
					case MSI_AUCODER_SET_SPEED:
					{
                        aac_decode_s->speed = (uint8_t)param2;
                        ret = msi_output_cmd(aac_decode_s->msi,MSI_CMD_AUTPC,MSI_AUTPC_SET_SPEED,(uint32_t)(aac_decode_s->speed));
						break;
					}
					case MSI_AUCODER_GET_SPEED:
					{
                        *((uint32_t*)param2) = (uint32_t)(aac_decode_s->speed);
                        ret = RET_OK;
						break;
					}
					case MSI_AUCODER_SET_PITCH:
					{
						aac_decode_s->pitch = (uint8_t)param2;
                        ret = msi_output_cmd(aac_decode_s->msi,MSI_CMD_AUTPC,MSI_AUTPC_SET_PITCH,(uint32_t)(aac_decode_s->pitch));
						break;
					}
					case MSI_AUCODER_GET_PITCH:
					{
                        *((uint32_t*)param2) = (uint32_t)(aac_decode_s->pitch);
                        ret = RET_OK;
						break;
					}
                    case MSI_AUCODER_CLEAR_STREAM:
                    {
                        os_event_set(&aac_decode_s->event, coder_clear_event, NULL);
                        os_event_wait(&aac_decode_s->event, coder_clear_finish_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                        msi_output_cmd(aac_decode_s->msi,MSI_CMD_AUTPC,MSI_AUTPC_END_STREAM,(uint32_t)(&(aac_decode_s->audio_track)));
                        msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_CLEAR_STREAM,(uint32_t)(&(aac_decode_s->audio_track)));
                        ret = RET_OK;  
                        break;                                
                    }
                    case MSI_AUCODER_SET_SRCMSI:
                    {
                        if(aac_decode_s->src_msi) {
                            msi_del_output(aac_decode_s->src_msi, NULL, msi->name);
                        }
                        aac_decode_s->src_msi = NULL;
                        ret = msi_add_output((struct msi*)param2, NULL, msi->name);
                        if(ret == RET_OK) {
                            aac_decode_s->src_msi = (struct msi*)param2;
                        }
                        break;
                    }
                    case MSI_AUCODER_DIRECT_TO_DAC:
                    {
                        uint32_t direct_to_dac = param2;
                        if(direct_to_dac && aac_decode_s->direct_to_dac == 0) {
                            aac_decode_s->direct_to_dac = 1;
                            if(aac_decode_s->use_tpc == 0) {
                                msi_add_output(msi, NULL, "R_AUDAC");
                            }
                            else if(aac_decode_s->autpc_msi) {
                                msi_add_output(aac_decode_s->autpc_msi, NULL, "R_AUDAC");
                            }
                            msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TRACK,(uint32_t)(&(aac_decode_s->audio_track)));
                        }
                        else if(aac_decode_s->direct_to_dac == 1) {
                            aac_decode_s->direct_to_dac = 0;
                            if(aac_decode_s->use_tpc == 0) {
                                msi_del_output(msi, NULL, "R_AUDAC");
                            }
                            else if(aac_decode_s->autpc_msi) {
                                msi_del_output(aac_decode_s->autpc_msi, NULL, "R_AUDAC");
                            }
                            aac_decode_s->audio_track.priority &= 0x3F;
                            msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TRACK,(uint32_t)(&(aac_decode_s->audio_track)));
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
            if(aac_decode_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                fbpool_put(&aac_decode_s->tx_pool, frame_buf);
            }
            break; 
        }  
        case MSI_CMD_PRE_DESTROY:
        {
            if(aac_decode_s && aac_decode_s->task_hdl) {
                aac_decode_s->next_status = AUCODEC_EXIT;
            }
            break;
        }   
		case MSI_CMD_POST_DESTROY:
        {
            if(aac_decode_s) {
                if(aac_decode_s->task_hdl) {
                    os_event_wait(&aac_decode_s->event, coder_exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
                for(uint32_t i=0; i<MAX_AAC_DECODE_TXBUF; i++) {
                    struct framebuff *frame_buf = (aac_decode_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        AAC_CODE_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&aac_decode_s->tx_pool);
                if(aac_decode_s->event.hdl)  {
                    os_event_del(&aac_decode_s->event);
                }
                if(aac_decode_s->aac_fp) {
                    osal_fclose(aac_decode_s->aac_fp);
                    aac_decode_s->aac_fp = NULL;
                }
				if(aac_decode_s->autpc_msi) {
					autpc_msi_deinit(aac_decode_s->autpc_msi);
                    aac_decode_s->autpc_msi = NULL;
				}
                if(aac_decode_s->msi_name) {
                    AAC_CODE_FREE(aac_decode_s->msi_name);
                    aac_decode_s->msi_name = NULL;
                }
                AAC_CODE_FREE(aac_decode_s);
                aac_decode_s = NULL;
            }
            break;
        }     
        default:
            break;    
    }
    return ret;
}

struct msi *aac_decode_init(char *filename, uint8_t loop_mode, AUDEC_INIT *audec_init)
{
#if AUDIO_EN
    char *msi_name = NULL;

    msi_name = (char*)AAC_CODE_ZALLOC(sizeof(char)*32);
    if(msi_name == NULL) {
        os_printf("alloc autpc msi namefail\n");
        return NULL;
    }
    os_snprintf(msi_name, 20, "SR_AAC_DECODE_""%04d", (int)(os_jiffies()));
	struct msi *msi = msi_new(msi_name, MAX_AAC_DECODE_RXBUF, NULL);
	if(msi == NULL) {
		AAC_INFO("create aac decode msi fail!\r\n");
		return NULL;
	}
	struct aac_decode_struct *aac_decode_s = (struct aac_decode_struct*)AAC_CODE_ZALLOC(sizeof(struct aac_decode_struct));
	if(!aac_decode_s) {
		AAC_INFO("aac_decode_s malloc fail!\r\n");
		goto aac_decode_init_err;
	}
    msi->priv = aac_decode_s;
	msi->action = (msi_action)aac_decode_msi_action;   
	fbpool_init(&aac_decode_s->tx_pool, MAX_AAC_DECODE_TXBUF);
	for(uint32_t i=0; i<MAX_AAC_DECODE_TXBUF; i++) {
		struct framebuff *frame_buf = (aac_decode_s->tx_pool.pool)+i;
		frame_buf->data = (uint8_t*)AAC_CODE_MALLOC(1024 * sizeof(int16_t));
		if(frame_buf->data == NULL) {
			AAC_INFO("aac decode malloc framebuff data fail!\r\n");
			goto aac_decode_init_err;       
		}   
	}
    if(os_event_init(&aac_decode_s->event) != RET_OK) {
        AAC_INFO("create aac decode event fail!\r\n");
        goto aac_decode_init_err;
    }
	if(filename) {
		aac_decode_s->aac_fp = osal_fopen((const char*)filename, "rb");
		if(aac_decode_s->aac_fp == NULL) {
            AAC_INFO("open aac file %s fail!\r\n", filename);
			goto aac_decode_init_err;
        }	
	}
    if(audec_init->src_msi && (msi_add_output(audec_init->src_msi, NULL, msi->name) != RET_OK)) {
        goto aac_decode_init_err;
    }
	aac_decode_s->msi = msi;
    aac_decode_s->msi_name = msi_name;
    aac_decode_s->src_msi = audec_init->src_msi;
    aac_decode_s->loop_mode = loop_mode;
    aac_decode_s->direct_to_dac = audec_init->direct_to_dac;
	aac_decode_s->use_tpc = audec_init->use_tpc;
	aac_decode_s->speed = audec_init->speed;
	aac_decode_s->pitch = audec_init->pitch;
    aac_decode_s->destroy_self = audec_init->destroy_self;
    aac_decode_s->audio_track.priority = audec_init->priority;
    aac_decode_s->audio_track.track_type = audec_init->track_type;
    aac_decode_s->next_status = AUCODEC_RUN;
	aac_decode_s->current_status = AUCODEC_RUN;
    if(audec_init->direct_to_dac && !aac_decode_s->use_tpc) {
	    msi_add_output(msi, NULL, "R_AUDAC");
    }    
#if AAC_DEC_CTRL == AUCODER_RUN_IN_CPU1
    aac_decode_s->task_hdl = os_task_create("aac_decode_thread", aac_decode_thread, (void*)aac_decode_s, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 1024);
#else
    aac_decode_s->task_hdl = os_task_create("aac_decode_thread", aac_decode_thread, (void*)aac_decode_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 2048);
#endif
    if(aac_decode_s->task_hdl == NULL)  {
		AAC_INFO("create aac decode task fail!\r\n");
		goto aac_decode_init_err;
	}
	return msi;
	
aac_decode_init_err:
	msi_destroy(msi);
#endif
    return NULL;
}