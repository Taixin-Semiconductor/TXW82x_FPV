#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "osal_file.h"
#include "audio_code_ctrl.h"
#include "wave_code.h"

#define BUFF_SIZE   1024
#define MAX_WAVE_DECODE_TXBUF    4

struct wave_decode_struct {
    struct fbpool tx_pool;
	struct os_event event;
    struct msi *msi;
	void *task_hdl;
	void *wave_fp;
	uint8_t direct_to_dac;
    uint8_t get_wave_head;
    uint8_t buf[BUFF_SIZE];
    uint32_t buf_size;
	uint32_t cur_sampleRate;
    TYPE_WAVE_HEAD wave_head;
};
static struct wave_decode_struct *wave_decode_s = NULL;

static uint8_t next_status = AUDIO_STOP;
static uint8_t current_status = AUDIO_STOP;
static void wave_decode_destroy(void);

uint8_t get_wave_decode_status(void)
{
    return current_status;
}

static void set_wave_decode_status(uint8_t status)
{
    next_status = status; 
}

static void get_wave_head(void *fp, struct wave_decode_struct *s)
{
    int32_t read_len = 0;
    uint32_t offset = 0;
    uint32_t head_seek = 0;

    read_len = osal_fread(s->buf, 512, 1, fp);
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
	s->cur_sampleRate = s->wave_head.fmt_chunk.SampleRate;
    offset = sizeof(TYPE_RIFF_CHUNK)+sizeof(TYPE_FMT_CHUNK);
    while(os_strncmp(s->buf+offset, "data", 4) != 0) {
        offset++;
        if(offset >= s->buf_size-(sizeof(TYPE_DATA_CHUNK)-1)) {
            os_memcpy(s->buf, s->buf+s->buf_size+(sizeof(TYPE_DATA_CHUNK)-1), sizeof(TYPE_DATA_CHUNK)-1);
            head_seek += read_len; 
			read_len = osal_fread(s->buf+(sizeof(TYPE_DATA_CHUNK)-1), 512, 1, fp);
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
    osal_fseek(fp, head_seek);
    s->get_wave_head = 1;
}

static void wave_decode(void *fp, struct wave_decode_struct *s)
{
    int16_t *data = NULL;
	int32_t ret = 0;
    int32_t read_len = 0;
    int32_t audio_temp = 0;
    uint32_t once_read_size = 0;
	uint32_t read_total_size = 0;
    struct framebuff *frame_buf = NULL;

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
        if(next_status == AUDIO_PAUSE) {
            current_status = AUDIO_PAUSE;
            while(next_status == AUDIO_PAUSE) {
                os_sleep_ms(1);
            }
        }
        if(next_status == AUDIO_STOP)            
            break;

        current_status = AUDIO_RUN;

		frame_buf = fbpool_get(&s->tx_pool, 0, s->msi);
		if(frame_buf) {
			data = (int16_t*)(frame_buf->data);
			if(read_total_size >= s->wave_head.data_chunk.DataSize) {
				msi_delete_fb(s->msi, frame_buf);
				frame_buf = NULL;
				break;
			}
			if((read_total_size+once_read_size) > s->wave_head.data_chunk.DataSize) {
				read_len = osal_fread((uint8_t*)data, 1, (s->wave_head.data_chunk.DataSize-read_total_size), fp);
			}
			else
				read_len = osal_fread((uint8_t*)data, 1, once_read_size, fp);			
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
				frame_buf->len = read_len;
				frame_buf->mtype = F_AUDIO;
				frame_buf->stype = FSTYPE_AUDIO_WAVE_DECODER;
				ret = msi_output_fb(s->msi, frame_buf);
				WAVE_DEBUG("wave decode send framebuff:%p,ret:%d\r\n",frame_buf,ret);
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
    if(s->direct_to_dac)
	    msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,0);
}

static void wave_decode_thread(void *d)
{
	int32_t ret = 0;
    uint32_t former_dac_filter_type = 0;
    uint32_t former_dac_samplingrate = 0;
    struct wave_decode_struct *s = (struct wave_decode_struct *)d;

    if(s->wave_fp) {
		get_wave_head(s->wave_fp, s);
        if(!s->get_wave_head) {
            WAVE_INFO("wave head err!\n");
            goto wave_decode_thread_end;
        }
	}
  
	msi_get(s->msi);

	if(s->direct_to_dac) {
		ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_GET_FILTER_TYPE,(uint32_t)(&former_dac_filter_type));
		ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TYPE,FSTYPE_AUDIO_WAVE_DECODER);
		ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_GET_SAMPLING_RATE,(uint32_t)(&former_dac_samplingrate));
		ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_SAMPLING_RATE,s->cur_sampleRate);
	}

    wave_decode(s->wave_fp, s);

	if(s->direct_to_dac) {
		ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_SAMPLING_RATE,former_dac_samplingrate);
		ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TYPE,former_dac_filter_type);
	}
	
wave_decode_thread_end: 
	os_event_set(&s->event, exit_event, NULL);

	msi_put(s->msi);

	if(next_status != AUDIO_STOP)
    	wave_decode_destroy();
}

static int32_t wave_decode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;

    switch(cmd_id) {
        case MSI_CMD_FREE_FB:
		{
			if(wave_decode_s) {
				struct framebuff *frame_buf = (struct framebuff *)param1;
				fbpool_put(&wave_decode_s->tx_pool, frame_buf);
			}
			ret = RET_OK+1;
			break;
		}   
        case MSI_CMD_PRE_DESTROY:
        {
			if(wave_decode_s) {
                if(wave_decode_s->task_hdl) {
                    set_wave_decode_status(AUDIO_STOP);
                }
			}
			ret = RET_OK;
            break;
        }          
		case MSI_CMD_POST_DESTROY:
		{
			if(wave_decode_s) {
                if(wave_decode_s->task_hdl) {
                    os_event_wait(&wave_decode_s->event, exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
				for(uint32_t i=0; i<MAX_WAVE_DECODE_TXBUF; i++) {
					struct framebuff *frame_buf = (wave_decode_s->tx_pool.pool)+i;
					if(frame_buf->data) {
						WAVE_CODE_FREE(frame_buf->data);
						frame_buf->data = NULL;
					}
				}
				fbpool_destroy(&wave_decode_s->tx_pool);
                if(wave_decode_s->event.hdl)
                    os_event_del(&wave_decode_s->event);
				if(wave_decode_s->wave_fp) {
					osal_fclose(wave_decode_s->wave_fp);
					wave_decode_s->wave_fp = NULL;
				}
				WAVE_CODE_FREE(wave_decode_s);
				wave_decode_s = NULL;
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

int32_t wave_decode_add_output(char *msi_name)
{
	int32_t ret = RET_ERR;
	if(!msi_name) {
		WAVE_INFO("wave decode add output fail,msi_name is null!\r\n");
		return RET_ERR;
	}
	if(!wave_decode_s) {
		WAVE_INFO("wave decode add output fail,wave_decode_s is null!\r\n");
		return RET_ERR;
	}
	ret = msi_add_output(wave_decode_s->msi, NULL, msi_name);	
	return ret;
}

int32_t wave_decode_del_output(char *msi_name)
{
	int32_t ret = RET_ERR;
	if(!msi_name) {
		WAVE_INFO("wave decode del output fail,msi_name is null!\r\n");
		return RET_ERR;
	}
	if(!wave_decode_s) {
		WAVE_INFO("wave decode del output fail,wave_decode_s is null!\r\n");
		return RET_ERR;
	}
	ret = msi_del_output(wave_decode_s->msi, NULL, msi_name);	
	return ret;	
}

static void wave_decode_destroy(void)
{
	msi_destroy(wave_decode_s->msi);
}

void wave_decode_pause(void)
{
    set_wave_decode_status(AUDIO_PAUSE);
}

void wave_decode_continue(void)
{
    if(get_wave_decode_status() == AUDIO_PAUSE)
        set_wave_decode_status(AUDIO_RUN);
}

int32_t wave_decode_deinit(void)
{
    if(!wave_decode_s) {
        WAVE_INFO("wave_decode_deinit fail,wave_decode_s is null!\r\n");
        return RET_ERR;
    }
	wave_decode_destroy();
	return RET_OK;
}

struct msi *wave_decode_init(uint8_t *filename, uint8_t direct_to_dac)
{
#if AUDIO_EN
	uint8_t msi_isnew = 0;
    uint8_t *data = NULL;
	uint32_t count = 0;
	struct framebuff *frame_buf = NULL;
	void *wave_fp = NULL;

    while((get_wave_decode_status() != AUDIO_STOP) && count < 2000) {
        os_sleep_ms(1);
        count++;
    }
    if(count >= 2000) {
        WAVE_INFO("wave decode init timeout!\r\n");
        return NULL;
    }
		
	struct msi *msi = msi_new("S_WAVE_DECODE", 0, &msi_isnew);
	if(!msi) {
		WAVE_INFO("create wave decode msi fail!\r\n");
		return NULL; 
	}
	else if(!msi_isnew) {
		WAVE_INFO("wave decode msi has been create!\r\n");
		msi_destroy(msi);
		return NULL;		
	}
	if(direct_to_dac)
		msi_add_output(msi, NULL, "R_AUDAC");
	msi->enable = 1;
	msi->action = (msi_action)wave_decode_msi_action;        
	wave_decode_s = (struct wave_decode_struct *)WAVE_CODE_ZALLOC(sizeof(struct wave_decode_struct));
	if(!wave_decode_s) {
		WAVE_INFO("wave_decode_s malloc fail!\r\n");
		goto wave_decode_init_err;
	}
	fbpool_init(&wave_decode_s->tx_pool, MAX_WAVE_DECODE_TXBUF);
	for(uint32_t i=0; i<MAX_WAVE_DECODE_TXBUF; i++) {
		frame_buf = (wave_decode_s->tx_pool.pool)+i;
		frame_buf->data = NULL;
		data = (uint8_t*)WAVE_CODE_MALLOC(BUFF_SIZE);  
		if(!data)
		{
			WAVE_INFO("wave decode malloc framebuff data fail!\r\n");
			goto wave_decode_init_err;       
		}  
		frame_buf->data = data;  
	}
	if(filename) {
		wave_fp = osal_fopen((const char*)filename, "rb");
		if(!wave_fp) {
            WAVE_INFO("open wave file %s fail!\r\n", filename);
			goto wave_decode_init_err;
        }
		wave_decode_s->wave_fp = wave_fp;
	}
	else {
        WAVE_INFO("wave filename is null!\r\n");
		goto wave_decode_init_err;
	}
	wave_decode_s->msi = msi;
	wave_decode_s->direct_to_dac = direct_to_dac;
    if(os_event_init(&wave_decode_s->event) != RET_OK) {
        AAC_INFO("create wave decode event fail!\r\n");
        goto wave_decode_init_err;
    }
	next_status = AUDIO_RUN;
	current_status = AUDIO_RUN;
	wave_decode_s->task_hdl = os_task_create("wave_decode_thread", wave_decode_thread, (void*)wave_decode_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
	if(wave_decode_s->task_hdl == NULL)  {
		WAVE_INFO("create wave decode task fail!\r\n");
		goto wave_decode_init_err;
	}
	return msi;
	
wave_decode_init_err:
	if(wave_fp) {
		osal_fclose(wave_fp);
		wave_fp = NULL;
		wave_decode_s->wave_fp = NULL;
	}
	msi_destroy(msi);
#endif
	return NULL;
}