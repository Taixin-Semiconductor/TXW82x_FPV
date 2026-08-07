#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "osal_file.h"
#include "lib/audio/audio_code/audio_code.h"
#include "audio_code_ctrl.h"
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
    struct fbpool tx_pool;
    struct os_event event;
    struct msi *msi;
    void *task_hdl;
    void *amr_fp;
    uint8_t direct_to_dac;
    uint8_t inbuf[BUFF_SIZE];    
    uint8_t amr_type;
	uint32_t buf_size;
	uint32_t buf_offset;
    uint32_t cur_sampleRate;
};
static struct amr_decode_struct *amr_decode_s = NULL;

static uint8_t next_status = AUDIO_STOP;
static uint8_t current_status = AUDIO_STOP;

static void amr_decode_destroy(void);

uint8_t get_amr_decode_status(void)
{
    return current_status;
}

static void set_amr_decode_status(uint8_t status)
{
    next_status = status; 
}

static int32_t amr_file_read(uint8_t *buf, uint32_t size)
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
    struct framebuff *frame_buf = NULL;
    AUCODE_HDL *amr_dec = NULL;
    AUCODE_FRAME_INFO amr_info;

    while(!s->amr_type) {
        read_len = amr_file_read(s->inbuf+unproc_data_size,BUFF_SIZE);
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
        if(next_status == AUDIO_PAUSE) {
            current_status = AUDIO_PAUSE;
            while(next_status == AUDIO_PAUSE) {
                os_sleep_ms(1);
            }
        }

        if(next_status == AUDIO_STOP) {
            goto amr_decode_end;
        }
        current_status = AUDIO_RUN;

        if(unproc_data_size < 128 && !endOfRead) {
            bytes_to_read = BUFF_SIZE - unproc_data_size;
			if(unproc_data_size)
				os_memcpy(s->inbuf, enc_ptr+enc_ptr_offset, unproc_data_size);
            read_len = amr_file_read(s->inbuf+unproc_data_size, bytes_to_read);
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
            frame_buf->len = dec_samples*2;
            frame_buf->mtype = F_AUDIO;
            frame_buf->stype = FSTYPE_AUDIO_AMR_DECODER;
            if((s->direct_to_dac) && (s->cur_sampleRate != amr_info.samplerate)) {
                s->cur_sampleRate = amr_info.samplerate;
                msi_output_cmd(s->msi, MSI_CMD_AUDAC, MSI_AUDAC_SET_SAMPLING_RATE, s->cur_sampleRate);
            }
            ret = msi_output_fb(s->msi, frame_buf);   
			AMR_DEBUG("amr decode send framebuff:%p,ret:%d\r\n",frame_buf,ret);
            frame_buf = NULL;              
        }
        else if(endOfRead) {
            goto amr_decode_end;
        }      
    }
amr_decode_end:
    if(s->direct_to_dac)
	    msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,0);
	if(amr_dec)
		audio_coder_close(amr_dec);
}

static void amr_decode_thread(void *d)
{
	int32_t ret = 0;
    uint32_t former_dac_filter_type = 0;
    uint32_t former_dac_samplingrate = 0;
    struct amr_decode_struct *s = (struct amr_decode_struct *)d;
    
    msi_get(s->msi);

    if(s->direct_to_dac) {
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_GET_FILTER_TYPE,(uint32_t)(&former_dac_filter_type));
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TYPE,FSTYPE_AUDIO_AMR_DECODER);
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_GET_SAMPLING_RATE,(uint32_t)(&former_dac_samplingrate));
        s->cur_sampleRate = former_dac_samplingrate;
    }

    amr_decode(s);

    if(s->direct_to_dac) {
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_SAMPLING_RATE,former_dac_samplingrate);
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TYPE,former_dac_filter_type);
    }

    os_event_set(&s->event, exit_event, NULL);

    msi_put(s->msi);

    if(next_status != AUDIO_STOP)
        amr_decode_destroy();
}

static int32_t amr_decode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;

    switch(cmd_id) {
        case MSI_CMD_FREE_FB:
        {
            if(amr_decode_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                fbpool_put(&amr_decode_s->tx_pool, frame_buf);
            }
            ret = RET_OK+1;
            break;
        }
        case MSI_CMD_PRE_DESTROY:
        {
            if(amr_decode_s) {
                if(amr_decode_s->task_hdl) {
                    set_amr_decode_status(AUDIO_STOP);
                }
            }
            ret = RET_OK;
            break;
        }     
		case MSI_CMD_POST_DESTROY:
        {
            if(amr_decode_s) {
                if(amr_decode_s->task_hdl) {
                    os_event_wait(&amr_decode_s->event, exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
                for(uint32_t i=0; i<MAX_AMR_DECODE_TXBUF; i++) {
                    struct framebuff *frame_buf = (amr_decode_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        AMR_DECODE_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&amr_decode_s->tx_pool);
                if(amr_decode_s->amr_fp) {
                    osal_fclose(amr_decode_s->amr_fp);
                    amr_decode_s->amr_fp = NULL;
                }
                if(amr_decode_s->event.hdl)
                    os_event_del(&amr_decode_s->event);
                AMR_DECODE_FREE(amr_decode_s);
                amr_decode_s = NULL;
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

int32_t amr_decode_add_output(const char *msi_name)
{
    int32_t ret = RET_ERR;
	if(!msi_name) {
		AMR_INFO("amr decode add output fail,msi_name is null!\r\n");
		return RET_ERR;
	}
	if(!amr_decode_s) {
		AMR_INFO("amr decode add output fail,amr_decode_s is null!\r\n");
		return RET_ERR;
	}
    if(next_status == AUDIO_STOP) {
        AMR_INFO("amr decode add output fail,next_status is stop\n");
        return RET_ERR;
    }
	ret = msi_add_output(amr_decode_s->msi, NULL, msi_name);	
    return ret;
}

int32_t amr_decode_del_output(const char *msi_name)
{
    int32_t ret = RET_ERR;
	if(!msi_name) {
		AMR_INFO("amr decode del output fail,msi_name is null!\r\n");
		return RET_ERR;
	}
	if(!amr_decode_s) {
		AMR_INFO("amr decode del output fail,amr_decode_s is null!\r\n");
		return RET_ERR;
	}
	ret = msi_del_output(amr_decode_s->msi, NULL, msi_name);	
    return ret;	
}

static void amr_decode_destroy(void)
{
	msi_destroy(amr_decode_s->msi);
}

void amr_decode_pause(void)
{
    set_amr_decode_status(AUDIO_PAUSE);
}

void amr_decode_continue(void)
{
    if(get_amr_decode_status() == AUDIO_PAUSE)
        set_amr_decode_status(AUDIO_RUN);
}

void amr_decode_clear(void)
{

}

int32_t amr_decode_deinit(void)
{
    if(!amr_decode_s) {
        AMR_INFO("amr_decode_deinit fail,amr_decode_s is null!\r\n");
        return RET_ERR;
    }
    amr_decode_destroy();
    return RET_OK;
}

struct msi *amr_decode_init(uint8_t *filename, uint8_t direct_to_dac)
{
#if AUDIO_EN
    uint8_t msi_isnew = 0;
    uint8_t *data = NULL;
	uint32_t count = 0;
	struct framebuff *frame_buf = NULL;
    void *amr_fp = NULL;

    while((get_amr_decode_status() != AUDIO_STOP) && count < 2000) {
        os_sleep_ms(1);
        count++;
    }
    if(count >= 2000) {
        AMR_INFO("amr decode init timeout!\r\n");
        return NULL;
    }	
	struct msi *msi = msi_new("SR_AMR_DECODE", 0, &msi_isnew);
	if(!msi) {
		AMR_INFO("create amr decode msi fail!\r\n");
		return NULL;
	}
    else if(!msi_isnew) {
		AMR_INFO("amr decode msi has been create!\r\n");
        msi_destroy(msi);
		return NULL;        
    }
    if(direct_to_dac)
	    msi_add_output(msi, NULL, "R_AUDAC");
	msi->enable = 1;
	msi->action = (msi_action)amr_decode_msi_action;        
	amr_decode_s = (struct amr_decode_struct *)AMR_DECODE_ZALLOC(sizeof(struct amr_decode_struct));
	if(!amr_decode_s) {
		AMR_INFO("amr_decode_s malloc fail!\r\n");
		goto amr_decode_init_err;
	}
	fbpool_init(&amr_decode_s->tx_pool, MAX_AMR_DECODE_TXBUF);
	for(uint32_t i=0; i<MAX_AMR_DECODE_TXBUF; i++) {
		frame_buf = (amr_decode_s->tx_pool.pool)+i;
		frame_buf->data = NULL;  
		data = (uint8_t*)AMR_DECODE_MALLOC(320 * sizeof(int16_t));
		if(!data) {
			AMR_INFO("amr decode malloc framebuff data fail!\r\n");
			goto amr_decode_init_err;       
		}  
		frame_buf->data = data;  
	}
    if(filename) {
        amr_fp = osal_fopen((const char*)filename, "rb");
        if(!amr_fp) {
            AMR_INFO("open amr file %s fail!\r\n", filename);
            goto amr_decode_init_err;
        }
        amr_decode_s->amr_fp = amr_fp;
    }
    else {
        AMR_INFO("arm filename is null!\r\n");
		goto amr_decode_init_err;        
    }
	amr_decode_s->msi = msi;
    amr_decode_s->direct_to_dac = direct_to_dac;
    if(os_event_init(&amr_decode_s->event) != RET_OK) {
        AAC_INFO("create amr decode event fail!\r\n");
        goto amr_decode_init_err;
    }
	next_status = AUDIO_RUN;
	current_status = AUDIO_RUN;
#if AAC_ENC == AUCODER_RUN_IN_CPU1
    amr_decode_s->task_hdl = os_task_create("amr_decode_thread", amr_decode_thread, (void*)amr_decode_s, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 512);
#else
	amr_decode_s->task_hdl = os_task_create("amr_decode_thread", amr_decode_thread, (void*)amr_decode_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 4096);
#endif
	if(amr_decode_s->task_hdl == NULL)  {
		AMR_INFO("create amr decode task fail!\r\n");
		goto amr_decode_init_err;
	}
    return msi;
	
amr_decode_init_err:
	if(amr_fp) {
		osal_fclose(amr_fp);
		amr_fp = NULL;
        amr_decode_s->amr_fp = NULL;
	}
	msi_destroy(msi);
#endif
    return NULL;
}