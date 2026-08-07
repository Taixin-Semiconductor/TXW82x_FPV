#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "autpc_msi/autpc_msi.h"
#include "osal_file.h"
#include "wave_code.h"

#define BUFF_SIZE   1024
#define MAX_WAVE_DECODE_TXBUF    4

struct wave_decode_struct {
	AUDIO_TRACK audio_track;
    struct fbpool tx_pool;
	struct os_event event;
    struct msi *msi;
	struct msi *autpc_msi;
	char *msi_name;
	void *task_hdl;
	void *wave_fp;
	uint8_t loop_mode;
	uint8_t direct_to_dac;
	uint8_t use_tpc;
	uint8_t speed;
	uint8_t pitch;
	uint8_t destroy_self;
    uint8_t get_wave_head;
	uint8_t next_status;
	uint8_t current_status;
    uint8_t buf[BUFF_SIZE];
    uint32_t buf_size;
    TYPE_WAVE_HEAD wave_head;
};

static void get_wave_head(struct wave_decode_struct *s)
{
    int32_t read_len = 0;
    uint32_t offset = 0;
    uint32_t head_seek = 0;

    read_len = osal_fread(s->buf, 512, 1, s->wave_fp);
    if(read_len <= 0) {
        WAVE_INFO("wave read fail,ret:%d,line:%d!\r\n",read_len,__LINE__);
        return;
    }
    s->buf_size = read_len;
    os_memcpy((uint8_t*)(&(s->wave_head)), s->buf, sizeof(TYPE_RIFF_CHUNK)+sizeof(TYPE_FMT_CHUNK));
    if(os_strncmp(&(s->wave_head.riff_chunk), "RIFF", 4) != 0) {
        WAVE_INFO("wave head err!\n");
        return;
    }
    offset = sizeof(TYPE_RIFF_CHUNK)+sizeof(TYPE_FMT_CHUNK);
    while(os_strncmp(s->buf+offset, "data", 4) != 0) {
        offset++;
        if(offset >= s->buf_size-(sizeof(TYPE_DATA_CHUNK)-1)) {
            os_memcpy(s->buf, s->buf+s->buf_size+(sizeof(TYPE_DATA_CHUNK)-1), sizeof(TYPE_DATA_CHUNK)-1);
            head_seek += read_len; 
			read_len = osal_fread(s->buf+(sizeof(TYPE_DATA_CHUNK)-1), 512, 1, s->wave_fp);
            if(read_len <= 0) {
                WAVE_INFO("wave read fail,ret:%d,line:%d!\r\n",read_len,__LINE__);
                return;
            } 
            s->buf_size = read_len + (sizeof(TYPE_DATA_CHUNK)-1);                    
            offset = 0; 
        }
    }
    head_seek += (offset+sizeof(TYPE_DATA_CHUNK));
    os_memcpy((uint8_t*)(&(s->wave_head.data_chunk)), s->buf+offset, sizeof(TYPE_DATA_CHUNK));
    osal_fseek(s->wave_fp, head_seek);
    s->get_wave_head = 1;
}

static void wave_decode(struct wave_decode_struct *s)
{
    int16_t *data = NULL;
	int32_t ret = 0;
    int32_t read_len = 0;
    int32_t audio_temp = 0;
    uint32_t once_read_size = 0;
	uint32_t read_total_size = 0;
	uint32_t interval_time = 0;
    struct framebuff *frame_buf = NULL;
	AUDIO_TRACK *audac_fiter_track = NULL;

	if(s->wave_head.fmt_chunk.BitsPerSample == 24) {
		once_read_size = BUFF_SIZE*8/24;  //sample取整
		once_read_size = once_read_size*3/4;   //sample取偶
		once_read_size *= 4;
	}
	else if(s->wave_head.fmt_chunk.BitsPerSample == 16) {
		once_read_size = BUFF_SIZE;
	}
	else if(s->wave_head.fmt_chunk.BitsPerSample == 8) {
		once_read_size = BUFF_SIZE/2;
	}
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
            break;
		}
        s->current_status = AUCODEC_RUN;
		frame_buf = fbpool_get(&s->tx_pool, 0, s->msi);
		if(frame_buf) {
			data = (int16_t*)(frame_buf->data);
			if(read_total_size >= s->wave_head.data_chunk.DataSize) {
				goto wave_decode_end;
			}
			if((read_total_size+once_read_size) > s->wave_head.data_chunk.DataSize) {
				read_len = osal_fread((uint8_t*)data, 1, (s->wave_head.data_chunk.DataSize-read_total_size), s->wave_fp);
			}
			else
				read_len = osal_fread((uint8_t*)data, 1, once_read_size, s->wave_fp);			
			if(read_len > 0) {
				read_total_size += read_len;
				if(s->wave_head.fmt_chunk.BitsPerSample == 24) {
					for(uint32_t i=0; i<(read_len/3); i++) {						
						audio_temp = (int32_t)((*((uint8_t*)data+3*i))|(*((uint8_t*)data+3*i+1)<<8)|(*((uint8_t*)data+3*i+2))<<16)&0x00FFFFFF;
						data[i] = audio_temp>>8;
					}
					read_len = read_len*2/3;
				}
				else if(s->wave_head.fmt_chunk.BitsPerSample == 8) {
					os_memcpy(data+read_len/2, data, read_len);
					for(uint32_t i=0; i<read_len; i++) {
						data[i] = (int16_t)(*(((uint8_t*)data)+read_len+i)-128)<<8;
					}
					read_len = read_len*2;                
				}

				if(s->wave_head.fmt_chunk.FmtChannels == 2) {
					for(uint32_t i=0; i<(read_len/4); i++) {
						data[i] = (( ((int32_t)data[2*i]) + data[2*i+1] ) >>1 );
					}
					read_len = read_len/2;
				}
				frame_buf->priv = &(s->audio_track);
				frame_buf->len = read_len;
				frame_buf->mtype = F_AUDIO;
				frame_buf->stype = FSTYPE_AUDIO_PCM;
                s->audio_track.samplerate = s->wave_head.fmt_chunk.SampleRate;
				if(s->use_tpc && !s->autpc_msi) {
					s->autpc_msi = autpc_msi_init(s->audio_track.samplerate, s->speed, s->pitch, read_len/2, &(s->audio_track));
					if(s->autpc_msi == NULL) {
						goto wave_decode_end;
					}
					msi_add_output(s->msi, NULL, s->autpc_msi->name);
					if(s->direct_to_dac) {
						msi_add_output(s->autpc_msi, NULL, "R_AUDAC");
					}
				}
				ret = msi_output_fb(s->msi, frame_buf);
				WAVE_DEBUG("wave decode send framebuff:%p,ret:%d\r\n",frame_buf,ret);
                msi_cmd("R_AUDAC", MSI_CMD_AUDAC, MSI_AUDAC_GET_FILTER_TRACK, (uint32_t)(&audac_fiter_track));
                if(s->direct_to_dac && (!s->use_tpc) && (audac_fiter_track != (&(s->audio_track)))) {
                    interval_time = (frame_buf->len>>1)*1000/(s->wave_head.fmt_chunk.SampleRate);
                    os_sleep_ms(interval_time);
                } 
				frame_buf = NULL;
			}	
			else if(read_len <= 0 ) {
				msi_delete_fb(s->msi, frame_buf);
				frame_buf = NULL;
				WAVE_INFO("wave read fail,ret:%d,line:%d!\r\n",read_len,__LINE__);
				break;
			}
        }
		else
			os_sleep_ms(1);
    }
wave_decode_end:
    msi_output_cmd(s->msi,MSI_CMD_AUTPC,MSI_AUTPC_END_STREAM,(uint32_t)(&(s->audio_track)));
	msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,(uint32_t)(&(s->audio_track)));
    if(frame_buf) {
        msi_delete_fb(s->msi, frame_buf);
        frame_buf = NULL;
    }
}

static void wave_decode_thread(void *d)
{
    struct wave_decode_struct *s = (struct wave_decode_struct *)d;

    if(s->wave_fp) {
		get_wave_head(s);
        if(!s->get_wave_head) {
            WAVE_INFO("wave head err!\n");
            goto wave_decode_thread_end;
        }
	}
  
	msi_get(s->msi);

	if(s->direct_to_dac) {
		msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TRACK,(uint32_t)(&(s->audio_track)));
	}

	s->msi->enable = 1;
	do {
		osal_fseek(s->wave_fp, sizeof(TYPE_WAVE_HEAD));
    	wave_decode(s);
		if(s->current_status == AUCODEC_EXIT) {
			break;
		}
	}while(s->loop_mode);

	if(s->direct_to_dac) {
		s->audio_track.priority &= 0x3F;
		msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TRACK,(uint32_t)(&(s->audio_track)));
	}
	
wave_decode_thread_end: 
	os_event_set(&s->event, coder_exit_event, NULL);

    while((s->next_status != AUCODEC_EXIT) && (s->destroy_self == 0)) {
        s->current_status = AUCODEC_END;
        os_sleep_ms(5);
    } 

	if(s->next_status != AUCODEC_EXIT)
    	msi_destroy(s->msi);

	msi_put(s->msi);
}

static int32_t wave_decode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
	struct wave_decode_struct *wave_decode_s = (struct wave_decode_struct *)(msi->priv);
    switch(cmd_id) {
		case MSI_CMD_AUCODER:
		{
			ret = RET_ERR;
			if(wave_decode_s) {
				uint32_t cmd_self = (uint32_t)param1;
				switch(cmd_self) {	
                    case MSI_AUCODER_PAUSE:
                    {
                        if(wave_decode_s->current_status == AUCODEC_RUN) {
                            wave_decode_s->next_status = AUCODEC_PAUSE;
						}
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_CONTINUE:
                    {
                        if(wave_decode_s->current_status == AUCODEC_PAUSE) {
                            wave_decode_s->next_status = AUCODEC_RUN;
						}
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_GET_STATUS:
                    {
                        *((uint32_t*)param2) = (uint32_t)(wave_decode_s->current_status);
                        ret = RET_OK;  
                        break;                      
                    }
					case MSI_AUCODER_SET_SPEED:
					{
                        wave_decode_s->speed = (uint8_t)param2;
                        ret = msi_output_cmd(wave_decode_s->msi,MSI_CMD_AUTPC,MSI_AUTPC_SET_SPEED,(uint32_t)(wave_decode_s->speed));
						break;
					}
					case MSI_AUCODER_GET_SPEED:
					{
                        *((uint32_t*)param2) = (uint32_t)(wave_decode_s->speed);
                        ret = RET_OK;
						break;
					}
					case MSI_AUCODER_SET_PITCH:
					{
						wave_decode_s->pitch = (uint8_t)param2;
                        ret = msi_output_cmd(wave_decode_s->msi,MSI_CMD_AUTPC,MSI_AUTPC_SET_PITCH,(uint32_t)(wave_decode_s->pitch));
						break;
					}
					case MSI_AUCODER_GET_PITCH:
					{
                        *((uint32_t*)param2) = (uint32_t)(wave_decode_s->pitch);
                        ret = RET_OK;
						break;
					}
                    case MSI_AUCODER_DIRECT_TO_DAC:
                    {
                        uint32_t direct_to_dac = param2;
                        if(direct_to_dac && wave_decode_s->direct_to_dac == 0) {
                            wave_decode_s->direct_to_dac = 1;
                            if(wave_decode_s->use_tpc == 0) {
                                msi_add_output(msi, NULL, "R_AUDAC");
                            }
                            else if(wave_decode_s->autpc_msi) {
                                msi_add_output(wave_decode_s->autpc_msi, NULL, "R_AUDAC");
                            }
                            msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TRACK,(uint32_t)(&(wave_decode_s->audio_track)));
                        }
                        else if(wave_decode_s->direct_to_dac == 1) {
                            wave_decode_s->direct_to_dac = 0;
                            if(wave_decode_s->use_tpc == 0) {
                                msi_del_output(msi, NULL, "R_AUDAC");
                            }
                            else if(wave_decode_s->autpc_msi) {
                                msi_del_output(wave_decode_s->autpc_msi, NULL, "R_AUDAC");
                            }
                            wave_decode_s->audio_track.priority &= 0x3F;
                            msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TRACK,(uint32_t)(&(wave_decode_s->audio_track)));
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
			if(wave_decode_s) {
				struct framebuff *frame_buf = (struct framebuff *)param1;
				fbpool_put(&wave_decode_s->tx_pool, frame_buf);
			}
			break;
		}   
        case MSI_CMD_PRE_DESTROY:
        {
			if(wave_decode_s && wave_decode_s->task_hdl) {
                wave_decode_s->next_status = AUCODEC_EXIT;
			}
            break;
        }          
		case MSI_CMD_POST_DESTROY:
		{
			if(wave_decode_s) {
                if(wave_decode_s->task_hdl) {
                    os_event_wait(&wave_decode_s->event, coder_exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
				for(uint32_t i=0; i<MAX_WAVE_DECODE_TXBUF; i++) {
					struct framebuff *frame_buf = (wave_decode_s->tx_pool.pool)+i;
					if(frame_buf->data) {
						WAVE_CODE_FREE(frame_buf->data);
						frame_buf->data = NULL;
					}
				}
				fbpool_destroy(&wave_decode_s->tx_pool);
                if(wave_decode_s->event.hdl) {
                    os_event_del(&wave_decode_s->event);
				}
				if(wave_decode_s->wave_fp) {
					osal_fclose(wave_decode_s->wave_fp);
					wave_decode_s->wave_fp = NULL;
				}
				if(wave_decode_s->autpc_msi) {
					autpc_msi_deinit(wave_decode_s->autpc_msi);
					wave_decode_s->autpc_msi = NULL;
				}
                if(wave_decode_s->msi_name) {
                    WAVE_CODE_FREE(wave_decode_s->msi_name);
					wave_decode_s->msi_name = NULL;
                }
				WAVE_CODE_FREE(wave_decode_s);
				wave_decode_s = NULL;
			}
			break;
		}         
        default:
            break;    
    }
    return ret;
}

struct msi *wave_decode_init(char *filename, uint8_t loop_mode, AUDEC_INIT *audec_init)
{
#if AUDIO_EN	
    char *msi_name = NULL;

    msi_name = (char*)WAVE_CODE_ZALLOC(sizeof(char)*32);
    if(msi_name == NULL) {
        os_printf("alloc autpc msi namefail\n");
        return NULL;
    }
    os_snprintf(msi_name, 20, "S_WAVE_DECODE_""%04d", (int)(os_jiffies()));
	struct msi *msi = msi_new(msi_name, 0, NULL);
	if(!msi) {
		WAVE_INFO("create wave decode msi fail!\r\n");
		return NULL; 
	}
	struct wave_decode_struct *wave_decode_s = (struct wave_decode_struct *)WAVE_CODE_ZALLOC(sizeof(struct wave_decode_struct));
	if(!wave_decode_s) {
		WAVE_INFO("wave_decode_s malloc fail!\r\n");
		goto wave_decode_init_err;
	}
	msi->priv = wave_decode_s;
	msi->action = (msi_action)wave_decode_msi_action;     
	fbpool_init(&wave_decode_s->tx_pool, MAX_WAVE_DECODE_TXBUF);
	for(uint32_t i=0; i<MAX_WAVE_DECODE_TXBUF; i++) {
		struct framebuff *frame_buf = (wave_decode_s->tx_pool.pool)+i;
		frame_buf->data = (uint8_t*)WAVE_CODE_MALLOC(BUFF_SIZE);  
		if(frame_buf->data == NULL) {
			WAVE_INFO("wave decode malloc framebuff data fail!\r\n");
			goto wave_decode_init_err;       
		}  
	}
	if(filename) {
		wave_decode_s->wave_fp = osal_fopen((const char*)filename, "rb");
		if(wave_decode_s->wave_fp == NULL) {
            WAVE_INFO("open wave file %s fail!\r\n", filename);
			goto wave_decode_init_err;
        }
	}
	else {
        WAVE_INFO("wave filename is null!\r\n");
		goto wave_decode_init_err;
	}
    if(os_event_init(&wave_decode_s->event) != RET_OK) {
        WAVE_INFO("create wave decode event fail!\r\n");
        goto wave_decode_init_err;
    }
	wave_decode_s->msi = msi;
	wave_decode_s->msi_name = msi_name;
	wave_decode_s->loop_mode = loop_mode;
	wave_decode_s->direct_to_dac = audec_init->direct_to_dac;
	wave_decode_s->use_tpc = audec_init->use_tpc;
	wave_decode_s->speed = audec_init->speed;
	wave_decode_s->pitch = audec_init->pitch;
	wave_decode_s->destroy_self = audec_init->destroy_self;
	wave_decode_s->audio_track.priority = audec_init->priority;
	wave_decode_s->audio_track.track_type = audec_init->track_type;
	wave_decode_s->next_status = AUCODEC_RUN;
	wave_decode_s->current_status = AUCODEC_RUN;
	if(audec_init->direct_to_dac && !wave_decode_s->use_tpc) {
		msi_add_output(msi, NULL, "R_AUDAC"); 
	}  
	wave_decode_s->task_hdl = os_task_create("wave_decode_thread", wave_decode_thread, (void*)wave_decode_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
	if(wave_decode_s->task_hdl == NULL)  {
		WAVE_INFO("create wave decode task fail!\r\n");
		goto wave_decode_init_err;
	}
	return msi;
	
wave_decode_init_err:
	msi_destroy(msi);
#endif
	return NULL;
}