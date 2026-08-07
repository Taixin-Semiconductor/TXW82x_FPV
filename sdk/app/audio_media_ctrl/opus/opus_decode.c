#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/audio/audio_code/audio_code.h"
#include "autpc_msi/autpc_msi.h"
#include "opus_code.h"

#define MAX_OPUS_DECODE_RXBUF    4
#define MAX_OPUS_DECODE_TXBUF    4

struct opus_decode_struct {
    AUDIO_TRACK audio_track;
    struct fbpool tx_pool;
    struct os_event event;
    struct msi *msi;
    struct msi *src_msi;
    struct msi *autpc_msi;
    char *msi_name;
    void *task_hdl;
    uint8_t direct_to_dac;
	uint8_t use_tpc;
	uint8_t speed;
	uint8_t pitch;
    uint8_t destroy_self;
    uint8_t next_status;
    uint8_t current_status;
    int16_t dec_buf[960];  //按照16k、60ms的最大长度，若超过该长度则需修改数组大小;
    uint32_t coder_sampleRate;
};

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
        os_event_wait(&s->event, coder_clear_event, &clear_flag, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
        if(clear_flag & coder_clear_event) {
            clear_flag = 0;
            clear_finish = 0;
        }
        if(s->next_status == AUCODEC_PAUSE) {
            if(s->current_status == AUCODEC_RUN) {
                audio_coder_close(opus_dec);
                msi_output_cmd(s->msi,MSI_CMD_AUTPC,MSI_AUTPC_END_STREAM,(uint32_t)(&(s->audio_track)));
                msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,(uint32_t)(&(s->audio_track)));
                opus_dec = audio_coder_open(OPUS_DEC, s->coder_sampleRate, 1);
                if(!opus_dec) 
                    return;
            }
            s->current_status = AUCODEC_PAUSE;
        }   
        recv_frame_buf = msi_get_fb(s->msi, 0);
        if(recv_frame_buf) {
            if(clear_finish == 0) {
                goto opus_decode_frame_end;
            }
            if(s->next_status == AUCODEC_PAUSE) {
                goto opus_decode_frame_end;
            }   
            else {
                s->current_status = AUCODEC_RUN;
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
                send_frame_buf->priv = &(s->audio_track);
                send_frame_buf->len = dec_samples*2;
                send_frame_buf->mtype = F_AUDIO;
                send_frame_buf->stype = FSTYPE_AUDIO_PCM;
                s->audio_track.samplerate = opus_info.samplerate;
				if(s->use_tpc && !s->autpc_msi) {
					s->autpc_msi = autpc_msi_init(s->audio_track.samplerate, s->speed, s->pitch, dec_samples, &(s->audio_track));
					if(s->autpc_msi == NULL) {
						goto opus_decode_end;
					}
					if(s->direct_to_dac) {
						msi_add_output(s->autpc_msi, NULL, "R_AUDAC");
					}
					msi_add_output(s->msi, NULL, s->autpc_msi->name);
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
				os_event_set(&s->event, coder_clear_finish_event, NULL);
			}
            os_sleep_ms(1);
        }

        if(s->next_status == AUCODEC_EXIT) {
            s->current_status = AUCODEC_EXIT;
            goto opus_decode_end;
        }
    }    
opus_decode_end:
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
    if(opus_dec)
		audio_coder_close(opus_dec);
}

static void opus_decode_thread(void *d)
{
    struct opus_decode_struct *s = (struct opus_decode_struct *)d;

    msi_get(s->msi);

    if(s->direct_to_dac) {
        msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TRACK,(uint32_t)(&(s->audio_track)));
    }

    s->msi->enable = 1;
    opus_decode(s);

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

static int32_t opus_decode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    struct opus_decode_struct *opus_decode_s = (struct opus_decode_struct *)(msi->priv);
    switch(cmd_id) {
		case MSI_CMD_AUCODER:
		{
            ret = RET_ERR;
			if(opus_decode_s) {
				uint32_t cmd_self = (uint32_t)param1;
				switch(cmd_self) {	
                    case MSI_AUCODER_PAUSE:
                    {
                        if(opus_decode_s->current_status == AUCODEC_RUN) {
                            opus_decode_s->next_status = AUCODEC_PAUSE;
                        }
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_CONTINUE:
                    {
                        if(opus_decode_s->current_status == AUCODEC_PAUSE) {
                            opus_decode_s->next_status = AUCODEC_RUN;
                        }
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_GET_STATUS:
                    {
                        *((uint32_t*)param2) = (uint32_t)(opus_decode_s->current_status);
                        ret = RET_OK;  
                        break;                      
                    }
					case MSI_AUCODER_SET_SPEED:
					{
                        opus_decode_s->speed = (uint8_t)param2;
                        ret = msi_output_cmd(opus_decode_s->msi,MSI_CMD_AUTPC,MSI_AUTPC_SET_SPEED,(uint32_t)(opus_decode_s->speed));
						break;
					}
					case MSI_AUCODER_GET_SPEED:
					{
                        *((uint32_t*)param2) = (uint32_t)(opus_decode_s->speed);
                        ret = RET_OK;
						break;
					}
					case MSI_AUCODER_SET_PITCH:
					{
						opus_decode_s->pitch = (uint8_t)param2;
                        ret = msi_output_cmd(opus_decode_s->msi,MSI_CMD_AUTPC,MSI_AUTPC_SET_PITCH,(uint32_t)(opus_decode_s->pitch));
						break;
					}
					case MSI_AUCODER_GET_PITCH:
					{
                        *((uint32_t*)param2) = (uint32_t)(opus_decode_s->pitch);
                        ret = RET_OK;
						break;
					}
                    case MSI_AUCODER_CLEAR_STREAM:
                    {
                        os_event_set(&opus_decode_s->event, coder_clear_event, NULL);
                        os_event_wait(&opus_decode_s->event, coder_clear_finish_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                        msi_output_cmd(opus_decode_s->msi,MSI_CMD_AUTPC,MSI_AUTPC_END_STREAM,(uint32_t)(&(opus_decode_s->audio_track)));
                        msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_CLEAR_STREAM,(uint32_t)(&(opus_decode_s->audio_track)));                        
                        ret = RET_OK;  
                        break;                                
                    }
                    case MSI_AUCODER_SET_SRCMSI:
                    {
                        if(opus_decode_s->src_msi) {
                            msi_del_output(opus_decode_s->src_msi, NULL, msi->name);
                        }
                        opus_decode_s->src_msi = NULL;
                        ret = msi_add_output((struct msi*)param2, NULL, msi->name);
                        if(ret == RET_OK) {
                            opus_decode_s->src_msi = (struct msi*)param2;
                        }
                        break;
                    }
                    case MSI_AUCODER_DIRECT_TO_DAC:
                    {
                        uint32_t direct_to_dac = param2;
                        if(direct_to_dac && opus_decode_s->direct_to_dac == 0) {
                            opus_decode_s->direct_to_dac = 1;
                            if(opus_decode_s->use_tpc == 0) {
                                msi_add_output(msi, NULL, "R_AUDAC");
                            }
                            else if(opus_decode_s->autpc_msi) {
                                msi_add_output(opus_decode_s->autpc_msi, NULL, "R_AUDAC");
                            }
                            msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TRACK,(uint32_t)(&(opus_decode_s->audio_track)));
                        }
                        else if(opus_decode_s->direct_to_dac == 1) {
                            opus_decode_s->direct_to_dac = 0;
                            if(opus_decode_s->use_tpc == 0) {
                                msi_del_output(msi, NULL, "R_AUDAC");
                            }
                            else if(opus_decode_s->autpc_msi) {
                                msi_del_output(opus_decode_s->autpc_msi, NULL, "R_AUDAC");
                            }
                            opus_decode_s->audio_track.priority &= 0x3F;
                            msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TRACK,(uint32_t)(&(opus_decode_s->audio_track)));
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
            if(opus_decode_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                if(frame_buf->data) {
                    OPUS_CODE_FREE(frame_buf->data);
                    frame_buf->data = NULL;
                }
                fbpool_put(&opus_decode_s->tx_pool, frame_buf);
            }
            break; 
        }
        case MSI_CMD_PRE_DESTROY:
        {
            if(opus_decode_s && opus_decode_s->task_hdl) {
                opus_decode_s->next_status = AUCODEC_EXIT;
            }
            break;
        }         
		case MSI_CMD_POST_DESTROY:
        {
            if(opus_decode_s) {
                if(opus_decode_s->task_hdl) {
                    os_event_wait(&opus_decode_s->event, coder_exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
                for(uint32_t i=0; i<MAX_OPUS_DECODE_TXBUF; i++) {
                    struct framebuff *frame_buf = (opus_decode_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        OPUS_CODE_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&opus_decode_s->tx_pool);
                if(opus_decode_s->event.hdl) {
                    os_event_del(&opus_decode_s->event);
                }
				if(opus_decode_s->autpc_msi) {
					autpc_msi_deinit(opus_decode_s->autpc_msi);
                    opus_decode_s->autpc_msi = NULL;
				}
                if(opus_decode_s->msi_name) {
                    OPUS_CODE_FREE(opus_decode_s->msi_name);
                    opus_decode_s->msi_name = NULL;
                }
                OPUS_CODE_FREE(opus_decode_s);
                opus_decode_s = NULL;
            }	
            break;
        }     
        default:
            break;    
    }
    return ret;
}

struct msi *opus_decode_init(uint32_t samplerate, AUDEC_INIT *audec_init)
{
#if AUDIO_EN
    char *msi_name = NULL;
    int32_t random_value = 0;

    msi_name = (char*)OPUS_CODE_ZALLOC(sizeof(char)*32);
    if(msi_name == NULL) {
        os_printf("alloc autpc msi namefail\n");
        return NULL;
    }
    os_random_bytes((uint8_t*)(&random_value), 4);
    os_snprintf(msi_name, 20, "SR_OPUS_DECODE_""%04d", random_value);
	struct msi *msi = msi_new(msi_name, MAX_OPUS_DECODE_RXBUF, NULL);
	if(msi == NULL) {
		OPUS_INFO("create opus decode msi fail!\r\n");
		return NULL;
	}     
	struct opus_decode_struct *opus_decode_s = (struct opus_decode_struct *)OPUS_CODE_ZALLOC(sizeof(struct opus_decode_struct));
	if(!opus_decode_s) {
		OPUS_INFO("opus_decode_s malloc fail!\r\n");
		goto opus_decode_init_err;
	}
    msi->priv = opus_decode_s;
	msi->action = (msi_action)opus_decode_msi_action;   
	fbpool_init(&opus_decode_s->tx_pool, MAX_OPUS_DECODE_TXBUF);
    if(os_event_init(&opus_decode_s->event) != RET_OK) {
        OPUS_INFO("create opus decode event fail!\r\n");
        goto opus_decode_init_err;
    }
    if(audec_init->src_msi && (msi_add_output(audec_init->src_msi, NULL, msi->name) != RET_OK)) {
        goto opus_decode_init_err;
    }
	opus_decode_s->msi = msi;
    opus_decode_s->msi_name = msi_name;
    opus_decode_s->src_msi = audec_init->src_msi;
    opus_decode_s->direct_to_dac = audec_init->direct_to_dac;
    opus_decode_s->coder_sampleRate = samplerate;
	opus_decode_s->use_tpc = audec_init->use_tpc;
	opus_decode_s->speed = audec_init->speed;
	opus_decode_s->pitch = audec_init->pitch;
    opus_decode_s->destroy_self = audec_init->destroy_self;
    opus_decode_s->audio_track.priority = audec_init->priority;
    opus_decode_s->audio_track.track_type = audec_init->track_type;
    opus_decode_s->next_status = AUCODEC_RUN;
	opus_decode_s->current_status = AUCODEC_RUN;
    if(audec_init->direct_to_dac && !opus_decode_s->use_tpc) {
	    msi_add_output(msi, NULL, "R_AUDAC");
    }
    os_printf("opus_track:%x\n",&(opus_decode_s->audio_track));
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