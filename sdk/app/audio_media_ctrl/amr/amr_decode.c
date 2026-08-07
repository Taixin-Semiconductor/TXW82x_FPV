#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/audio/audio_code/audio_code.h"
#include "autpc_msi/autpc_msi.h"
#include "osal_file.h"
#include "amr_decode.h"

#define BUFF_SIZE   1024
#define MAX_AMR_DECODE_TXBUF    4

#define AMRNB_HEADER "#!AMR\n"
#define AMRWB_HEADER "#!AMR-WB\n"

enum {
    AMR_NB = 1,
    AMR_WB,  
};

struct amr_decode_struct {
    AUDIO_TRACK audio_track;
    struct fbpool tx_pool;
    struct os_event event;
    struct msi *msi;
    struct msi *autpc_msi;
    char *msi_name;
    void *task_hdl;
    void *amr_fp;
    uint8_t loop_mode;
    uint8_t direct_to_dac;
	uint8_t use_tpc;
	uint8_t speed;
	uint8_t pitch;
    uint8_t destroy_self;
    uint8_t amr_type;
    uint8_t next_status;
    uint8_t current_status;
    uint8_t inbuf[BUFF_SIZE];    
	uint32_t buf_size;
	uint32_t buf_offset;
};

static int32_t amr_file_read(struct amr_decode_struct *amr_decode_s, uint8_t *buf, uint32_t size)
{
    int32_t read_len = 0;
	
    read_len = osal_fread(buf, size, 1, amr_decode_s->amr_fp);
    return read_len;
}

static void amr_decode(struct amr_decode_struct *s)
{
    uint8_t endOfRead = 0;
    uint8_t *enc_ptr = NULL;
    int16_t *data = NULL;
    int32_t ret = 0;
    int32_t read_len = 0;
    int32_t dec_samples = 0;
    uint32_t unproc_data_size = 0;
    uint32_t enc_ptr_offset = 0;
    uint32_t bytes_to_read = 0;
    uint32_t interval_time = 0;
    struct framebuff *frame_buf = NULL;
    AUCODE_HDL *amr_dec = NULL;
    AUCODE_FRAME_INFO amr_info;
	AUDIO_TRACK *audac_fiter_track = NULL;

    while(!s->amr_type) {
        read_len = amr_file_read(s, s->inbuf+unproc_data_size,BUFF_SIZE);
        if(read_len <= 0) {
            AMR_INFO("amr read fail,ret:%d,line:%d!\r\n",read_len,__LINE__);
            return;
        }
        s->buf_size = read_len + unproc_data_size;
        if(s->buf_size < os_strlen(AMRWB_HEADER)) {
            unproc_data_size = s->buf_size;
            os_sleep_ms(1);
            continue;            
        } 
        if(os_strncmp(s->inbuf, AMRWB_HEADER, os_strlen(AMRWB_HEADER)) == 0) {
            s->amr_type = AMR_WB;
            s->buf_size -= os_strlen(AMRWB_HEADER);
            s->buf_offset = 0;
            os_memcpy(s->inbuf, s->inbuf+os_strlen(AMRWB_HEADER), s->buf_size-os_strlen(AMRWB_HEADER));
        }
        else if(os_strncmp(s->inbuf, AMRNB_HEADER, os_strlen(AMRNB_HEADER)) == 0) {
            s->amr_type = AMR_NB;
            s->buf_size -= os_strlen(AMRNB_HEADER);  
            s->buf_offset = 0;
            os_memcpy(s->inbuf, s->inbuf+os_strlen(AMRNB_HEADER), s->buf_size-os_strlen(AMRNB_HEADER));        
        }
        else {
            AMR_INFO("amr header err!\n");
            return;
        }
    }
	unproc_data_size = s->buf_size;
    if(s->amr_type == AMR_NB)
        amr_dec = audio_coder_open(AMRNB_DEC, 0, 0);
    else if(s->amr_type == AMR_WB)
        amr_dec = audio_coder_open(AMRWB_DEC, 0, 0);
    else {
        AMR_INFO("unknow amr type!\r\n");
        return;
    }
    if(!amr_dec) 
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
            goto amr_decode_end;
        }
        s->current_status = AUCODEC_RUN;
        if(unproc_data_size < 128 && !endOfRead) {
            bytes_to_read = BUFF_SIZE - unproc_data_size;
			if(unproc_data_size)
				os_memcpy(s->inbuf, enc_ptr+enc_ptr_offset, unproc_data_size);
            read_len = amr_file_read(s, s->inbuf+unproc_data_size, bytes_to_read);
            if(read_len <= 0) {
                AMR_INFO("amr read fail,ret:%d,line:%d!\r\n",read_len,__LINE__);
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
            dec_samples = audio_decode_data(amr_dec, enc_ptr+enc_ptr_offset, unproc_data_size, data, &amr_info);
            if(dec_samples <= 0) {
                enc_ptr_offset += 1;
                unproc_data_size -= 1;
                msi_delete_fb(s->msi, frame_buf);;
                frame_buf = NULL;
                continue;
            }
            enc_ptr_offset += amr_info.frame_bytes;
            unproc_data_size -= amr_info.frame_bytes;
            frame_buf->priv = &s->audio_track;
            frame_buf->len = dec_samples*2;
            frame_buf->mtype = F_AUDIO;
            frame_buf->stype = FSTYPE_AUDIO_PCM;
            s->audio_track.samplerate = amr_info.samplerate;
            ret = msi_output_fb(s->msi, frame_buf); 
            if(s->use_tpc && !s->autpc_msi) {
                s->autpc_msi = autpc_msi_init(s->audio_track.samplerate, s->speed, s->pitch, dec_samples, &(s->audio_track));
                if(s->autpc_msi == NULL) {
                    goto amr_decode_end;
                }
                msi_add_output(s->msi, NULL, s->autpc_msi->name);
                if(s->direct_to_dac) {
                    msi_add_output(s->autpc_msi, NULL, "R_AUDAC");
                }
            }  
            AMR_DEBUG("amr decode send framebuff:%p,ret:%d\r\n",frame_buf,ret);
            msi_cmd("R_AUDAC", MSI_CMD_AUDAC, MSI_AUDAC_GET_FILTER_TRACK, (uint32_t)(&audac_fiter_track));
            if(s->direct_to_dac && (!s->use_tpc) && (audac_fiter_track != (&(s->audio_track)))) {
                interval_time = (frame_buf->len>>1)*1000/(amr_info.samplerate);
                os_sleep_ms(interval_time);
            } 
            frame_buf = NULL;              
        }
        else if(endOfRead) {
            goto amr_decode_end;
        }      
    }
amr_decode_end:
    msi_output_cmd(s->msi,MSI_CMD_AUTPC,MSI_AUTPC_END_STREAM,(uint32_t)(&(s->audio_track)));
	msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,(uint32_t)(&(s->audio_track)));
    if(frame_buf) {
        msi_delete_fb(s->msi, frame_buf);
        frame_buf = NULL;
    }
	if(amr_dec)
		audio_coder_close(amr_dec);
}

static void amr_decode_thread(void *d)
{
    struct amr_decode_struct *s = (struct amr_decode_struct *)d;
    
    msi_get(s->msi);

    if(s->direct_to_dac) {
        msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TRACK,(uint32_t)(&(s->audio_track)));
    }

    s->msi->enable = 1;
    do {
        osal_fseek(s->amr_fp, 0);
        amr_decode(s);
        if(s->current_status == AUCODEC_EXIT) {
            break;
        }
    }while(s->loop_mode);

    if(s->direct_to_dac) {
        s->audio_track.priority &= 0x3F;
        msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TRACK,(uint32_t)(&(s->audio_track)));
    }

    os_event_set(&s->event, coder_exit_event, NULL);

    while((s->next_status != AUCODEC_EXIT) && (s->destroy_self == 0)) {
        s->current_status = AUCODEC_END;
        os_sleep_ms(5);
    } 

    if(s->next_status != AUCODEC_EXIT)
        msi_destroy(s->msi);

    msi_put(s->msi);
}

static int32_t amr_decode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    struct amr_decode_struct *amr_decode_s = (struct amr_decode_struct*)(msi->priv);
    switch(cmd_id) {
		case MSI_CMD_AUCODER:
		{
            ret = RET_ERR;
			if(amr_decode_s) {
				uint32_t cmd_self = (uint32_t)param1;
				switch(cmd_self) {	
                    case MSI_AUCODER_PAUSE:
                    {
                        if(amr_decode_s->current_status == AUCODEC_RUN) {
                            amr_decode_s->next_status = AUCODEC_PAUSE;
                        }
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_CONTINUE:
                    {
                        if(amr_decode_s->current_status == AUCODEC_PAUSE) {
                            amr_decode_s->next_status = AUCODEC_RUN;
                        }
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_GET_STATUS:
                    {
                        *((uint32_t*)param2) = (uint32_t)(amr_decode_s->current_status);
                        ret = RET_OK;  
                        break;                      
                    }
					case MSI_AUCODER_SET_SPEED:
					{
                        amr_decode_s->speed = (uint8_t)param2;
                        ret = msi_output_cmd(amr_decode_s->msi,MSI_CMD_AUTPC,MSI_AUTPC_SET_SPEED,(uint32_t)(amr_decode_s->speed));
						break;
					}
					case MSI_AUCODER_GET_SPEED:
					{
                        *((uint32_t*)param2) = (uint32_t)(amr_decode_s->speed);
                        ret = RET_OK;
						break;
					}
					case MSI_AUCODER_SET_PITCH:
					{
						amr_decode_s->pitch = (uint8_t)param2;
                        ret = msi_output_cmd(amr_decode_s->msi,MSI_CMD_AUTPC,MSI_AUTPC_SET_PITCH,(uint32_t)(amr_decode_s->pitch));
						break;
					}
					case MSI_AUCODER_GET_PITCH:
					{
                        *((uint32_t*)param2) = (uint32_t)(amr_decode_s->pitch);
                        ret = RET_OK;
						break;
					}
                    case MSI_AUCODER_CLEAR_STREAM:
                    {
                        ret = RET_OK;  
                        break;                                
                    }
                    case MSI_AUCODER_DIRECT_TO_DAC:
                    {
                        uint32_t direct_to_dac = param2;
                        if(direct_to_dac && amr_decode_s->direct_to_dac == 0) {
                            amr_decode_s->direct_to_dac = 1;
                            if(amr_decode_s->use_tpc == 0) {
                                msi_add_output(msi, NULL, "R_AUDAC");
                            }
                            else if(amr_decode_s->autpc_msi) {
                                msi_add_output(amr_decode_s->autpc_msi, NULL, "R_AUDAC");
                            }
                            msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TRACK,(uint32_t)(&(amr_decode_s->audio_track)));
                        }
                        else if(amr_decode_s->direct_to_dac == 1) {
                            amr_decode_s->direct_to_dac = 0;
                            if(amr_decode_s->use_tpc == 0) {
                                msi_del_output(msi, NULL, "R_AUDAC");
                            }
                            else if(amr_decode_s->autpc_msi) {
                                msi_del_output(amr_decode_s->autpc_msi, NULL, "R_AUDAC");
                            }
                            amr_decode_s->audio_track.priority &= 0x3F;
                            msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TRACK,(uint32_t)(&(amr_decode_s->audio_track)));
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
        case MSI_CMD_FREE_FB:
        {
            ret = RET_ERR;
            if(amr_decode_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                fbpool_put(&amr_decode_s->tx_pool, frame_buf);
            }
            break;
        }
        case MSI_CMD_PRE_DESTROY:
        {
            if(amr_decode_s && amr_decode_s->task_hdl) {
                amr_decode_s->next_status = AUCODEC_EXIT;
            }
            break;
        }     
		case MSI_CMD_POST_DESTROY:
        {
            if(amr_decode_s) {
                if(amr_decode_s->task_hdl) {
                    os_event_wait(&amr_decode_s->event, coder_exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
                for(uint32_t i=0; i<MAX_AMR_DECODE_TXBUF; i++) {
                    struct framebuff *frame_buf = (amr_decode_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        AMR_DECODE_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&amr_decode_s->tx_pool);
                if(amr_decode_s->event.hdl) {
                    os_event_del(&amr_decode_s->event);
                }
                if(amr_decode_s->amr_fp) {
                    osal_fclose(amr_decode_s->amr_fp);
                    amr_decode_s->amr_fp = NULL;
                }
				if(amr_decode_s->autpc_msi) {
					autpc_msi_deinit(amr_decode_s->autpc_msi);
                    amr_decode_s->autpc_msi = NULL;
				}
                if(amr_decode_s->msi_name) {
                    AMR_DECODE_FREE(amr_decode_s->msi_name);
                    amr_decode_s->msi_name = NULL;
                }
                AMR_DECODE_FREE(amr_decode_s);
                amr_decode_s = NULL;
            }
            break;
        } 
        default:
            break;    
    }
    return ret;
}

struct msi *amr_decode_init(char *filename, uint8_t loop_mode, AUDEC_INIT *audec_init)
{
#if AUDIO_EN
    char *msi_name = NULL;

    msi_name = (char*)AMR_DECODE_ZALLOC(sizeof(char)*32);
    if(msi_name == NULL) {
        os_printf("alloc autpc msi namefail\n");
        return NULL;
    }
    os_snprintf(msi_name, 20, "SR_AMR_DECODE_""%04d", (int)(os_jiffies()));
    struct msi *msi = msi_new(msi_name, 0, NULL);
	if(msi == NULL) {
		AMR_INFO("create amr decode msi fail!\r\n");
		return NULL;
	}
	struct amr_decode_struct *amr_decode_s = (struct amr_decode_struct*)AMR_DECODE_ZALLOC(sizeof(struct amr_decode_struct));
	if(!amr_decode_s) {
		AMR_INFO("amr_decode_s malloc fail!\r\n");
		goto amr_decode_init_err;
	}
    msi->priv = amr_decode_s;
	msi->action = (msi_action)amr_decode_msi_action;    
	fbpool_init(&amr_decode_s->tx_pool, MAX_AMR_DECODE_TXBUF);
	for(uint32_t i=0; i<MAX_AMR_DECODE_TXBUF; i++) {
		struct framebuff *frame_buf = (amr_decode_s->tx_pool.pool)+i; 
		frame_buf->data = (uint8_t*)AMR_DECODE_MALLOC(320 * sizeof(int16_t));
		if(frame_buf->data == NULL) {
			AMR_INFO("amr decode malloc framebuff data fail!\r\n");
			goto amr_decode_init_err;       
		}  	
	}
    if(filename) {
        amr_decode_s->amr_fp = osal_fopen((const char*)filename, "rb");
        if(amr_decode_s->amr_fp == NULL) {
            AMR_INFO("open amr file %s fail!\r\n", filename);
            goto amr_decode_init_err;
        }
    }
    else {
        AMR_INFO("arm filename is null!\r\n");
		goto amr_decode_init_err;        
    }
    if(os_event_init(&amr_decode_s->event) != RET_OK) {
        AMR_INFO("create amr decode event fail!\r\n");
        goto amr_decode_init_err;
    }
	amr_decode_s->msi = msi;
    amr_decode_s->msi_name = msi_name;
    amr_decode_s->loop_mode = loop_mode;
    amr_decode_s->direct_to_dac = audec_init->direct_to_dac;
	amr_decode_s->use_tpc = audec_init->use_tpc;
	amr_decode_s->speed = audec_init->speed;
	amr_decode_s->pitch = audec_init->pitch;
    amr_decode_s->destroy_self = audec_init->destroy_self;
    amr_decode_s->audio_track.priority = audec_init->priority;
    amr_decode_s->audio_track.track_type = audec_init->track_type;
	amr_decode_s->next_status = AUCODEC_RUN;
	amr_decode_s->current_status = AUCODEC_RUN;
    if(audec_init->direct_to_dac && !amr_decode_s->use_tpc) {
	    msi_add_output(msi, NULL, "R_AUDAC");
    }  
#if AMRNB_DEC_CTRL == AUCODER_RUN_IN_CPU1
    amr_decode_s->task_hdl = os_task_create("amr_decode_thread", amr_decode_thread, (void*)amr_decode_s, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 1024);
#else
	amr_decode_s->task_hdl = os_task_create("amr_decode_thread", amr_decode_thread, (void*)amr_decode_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 4096);
#endif
	if(amr_decode_s->task_hdl == NULL)  {
		AMR_INFO("create amr decode task fail!\r\n");
		goto amr_decode_init_err;
	}
    return msi;
	
amr_decode_init_err:
	msi_destroy(msi);
#endif
    return NULL;
}