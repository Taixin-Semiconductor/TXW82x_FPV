#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "hal/auproc.h"
#include "hal/aures.h"
#include "magic_voice/magic_voice.h"
#include "auadc_proc.h"

typedef struct {
    uint16 data_size;
    uint16 data_offset;
    int16  *buf;
    uint32 buf_size;
} DATA_STRUCT;

typedef struct {
    uint8 stop;
    uint8 soft_gain;
    uint16 frame_size;
    uint32 energy;
    DATA_STRUCT adc_s;
    DATA_STRUCT dac_s;
    struct msi *msi;
    struct os_task *task_hdl;
	txAudioInfo_t audio_info;
    struct aures_device *aures_dev;
    void *aures_chan;
#if AUDIO_PROCESS
	struct auproc_device *auproc_dev;
#endif
#if MAGIC_VOICE_EN
    magic_voice_struct *magVo_s;
#endif
} AUADC_PROC_STRUCT;

extern uint32 auadc_calc_energy_asm(int16 *data, uint16 soft_gain, uint32 nsamples);

static int32 encoder_adaption_channels(AUADC_PROC_STRUCT *auproc_s, DATA_STRUCT *data_s, uint32 actual_channels, int16 *data, uint32 nsamples)
{
    int16 *buf = NULL;
    uint32 new_nsamples = nsamples * auproc_s->audio_info.channels;
    if(data_s->buf_size - data_s->data_size < new_nsamples){
        buf = (int16*)os_malloc_psram((new_nsamples + data_s->data_size) * sizeof(int16));
        if(buf == NULL) {
            return RET_ERR;
        }
        if(data_s->buf) {
            hw_memcpy(buf, data_s->buf, data_s->data_size * sizeof(int16));
            os_free_psram(data_s->buf);
        }  
        data_s->buf = buf;
        data_s->buf_size = new_nsamples + data_s->data_size;
    }
    buf = data_s->buf + data_s->data_size;
    if(actual_channels == 2) {
        for(uint32 i=0; i<new_nsamples; i++) {
            buf[i] = ((int32)data[2*i] + data[2*i+1]) >> 1; 
        }            
    }
    if(actual_channels == 1) {
        for(uint32 i=0; i<nsamples; i++) {
            buf[2 * i] = data[i];   
            buf[2 * i + 1] = data[i];
        }      
    }
    return RET_OK;
}

static int32 encoder_adaption_samplerate(AUADC_PROC_STRUCT *auproc_s, DATA_STRUCT *data_s, uint32 actual_sample_rate, int16 *data, uint32 *nsamples)
{
    int32 ret = RET_ERR;
    int16 *buf = NULL;
    uint32 last_actual_sample_rate = 0;
    uint32 new_nsamples = (*nsamples) * auproc_s->audio_info.sample_rate / actual_sample_rate + 10;
    struct aures_req req;
    if(auproc_s->aures_chan == NULL) {
        auproc_s->aures_chan = aures_open(auproc_s->aures_dev, actual_sample_rate, auproc_s->audio_info.sample_rate, auproc_s->audio_info.channels);
    }
    if(auproc_s->aures_chan == NULL) {
        return RET_ERR;
    }
    aures_ioctl(auproc_s->aures_dev, auproc_s->aures_chan, AURES_IOCTL_GET_SRC_SAMPLERATE, (uint32)(&last_actual_sample_rate));
    if(actual_sample_rate != last_actual_sample_rate) {
        aures_ioctl(auproc_s->aures_dev, auproc_s->aures_chan, AURES_IOCTL_SET_SRC_SAMPLERATE, actual_sample_rate);
    }
    if(data_s->buf_size - data_s->data_size < new_nsamples){
        buf = (int16*)os_malloc_psram((new_nsamples + data_s->data_size) * sizeof(int16));
        if(buf == NULL) {
            return RET_ERR;
        }
        if(data_s->buf) {
            hw_memcpy(buf, data_s->buf, data_s->data_size * sizeof(int16));
            os_free_psram(data_s->buf);
        }
        data_s->buf = buf; 
        data_s->buf_size = new_nsamples + data_s->data_size;
    }
    buf = data_s->buf + data_s->data_size;
    new_nsamples = data_s->buf_size - data_s->data_size;
    req.chan = auproc_s->aures_chan;
    req.in_data = data;
    req.in_nsamples = *nsamples;
    req.outbuf = buf;
    req.out_nsamples = new_nsamples;
    ret = aures_resample(auproc_s->aures_dev, &req);
    *nsamples = req.out_nsamples; 
    return ret;
}

static int32 encoder_adaption_input(AUADC_PROC_STRUCT *auproc_s, DATA_STRUCT *data_s, struct framebuff *frame_buf)
{
    uint8 copy_input = 1;
    int16 *input_data = NULL;
	uint32 nsamples = 0;
    txAudioInfo_t *actual_audio_info = frame_buf->codec_info;
    
    nsamples = frame_buf->len / sizeof(int16) / actual_audio_info->channels;
    if(auproc_s->audio_info.channels != actual_audio_info->channels) {
        input_data = (int16*)(frame_buf->data);
        if (encoder_adaption_channels(auproc_s, data_s, actual_audio_info->channels, input_data, nsamples) != RET_OK) {
            return RET_ERR;
        }
        input_data = data_s->buf + data_s->data_size;
        copy_input = 0;
    }
    if (auproc_s->audio_info.sample_rate != actual_audio_info->sample_rate) {
        if(input_data == NULL) {
            input_data = (int16*)(frame_buf->data);
        }
        if (encoder_adaption_samplerate(auproc_s, data_s, (uint32)(actual_audio_info->sample_rate), input_data, &nsamples) != RET_OK) {
            return RET_ERR;
        }
        input_data = data_s->buf + data_s->data_size;
        copy_input = 0;
    }
    if(copy_input) {
        if(data_s->buf_size - data_s->data_size < nsamples){
            input_data = (int16*)os_malloc_psram((nsamples + data_s->data_size) * sizeof(int16));
            if(input_data == NULL) {
                return RET_ERR;
            }
            if(data_s->buf) {
                hw_memcpy(input_data, data_s->buf, data_s->data_size * sizeof(int16));
                os_free_psram(data_s->buf);
            }
            data_s->buf = input_data;
            data_s->buf_size = nsamples + data_s->data_size;
        } 
        hw_memcpy(data_s->buf + data_s->data_size, frame_buf->data, frame_buf->len);       
    }
    data_s->data_size += nsamples;
	return RET_OK;
}

void auadc_proc_task(void *d)
{
    int16 *buf = NULL;
    uint32 nsamples = 0;
	struct framebuff *recv_frame_buf = NULL;
    struct framebuff *send_frame_buf = NULL;
    AUADC_PROC_STRUCT *auadc_proc_s = (AUADC_PROC_STRUCT*)d;
#if AUDIO_PROCESS
    struct auproc_req req;
#endif

	auadc_proc_s->stop = 0;
    while(1) {
		if(auadc_proc_s->stop)
			break;

        recv_frame_buf = msi_get_fb(auadc_proc_s->msi, 0);
        if(recv_frame_buf) {
            if(recv_frame_buf->stype == AUDIO_CODEC_DAC_LOOPBACK) {
#if AUDIO_PROCESS
                encoder_adaption_input(auadc_proc_s, &(auadc_proc_s->dac_s), recv_frame_buf);
                auadc_proc_s->dac_s.data_offset = 0;
                while(auadc_proc_s->dac_s.data_size - auadc_proc_s->dac_s.data_offset >= auadc_proc_s->frame_size) {
                    buf = auadc_proc_s->dac_s.buf + auadc_proc_s->dac_s.data_offset;
                    req.near_data = NULL;
                    req.far_data = buf;
                    req.nsamples = auadc_proc_s->frame_size;
                    auproc_process(auadc_proc_s->auproc_dev, &req);
                    auadc_proc_s->dac_s.data_offset += auadc_proc_s->frame_size;
                }
                if(auadc_proc_s->dac_s.data_size > auadc_proc_s->dac_s.data_offset) {
                    os_memmove(auadc_proc_s->dac_s.buf, auadc_proc_s->dac_s.buf + auadc_proc_s->dac_s.data_offset, 
                                (auadc_proc_s->dac_s.data_size - auadc_proc_s->dac_s.data_offset) * sizeof(int16));
                }  
                auadc_proc_s->dac_s.data_size -= auadc_proc_s->dac_s.data_offset; 
                auadc_proc_s->adc_s.data_offset = 0;             
#endif
                msi_delete_fb(auadc_proc_s->msi, recv_frame_buf);
            }
            else if(recv_frame_buf->stype == AUDIO_CODEC_ADC_READ) {
                encoder_adaption_input(auadc_proc_s, &(auadc_proc_s->adc_s), recv_frame_buf);
                msi_delete_fb(auadc_proc_s->msi, recv_frame_buf);
            }
        }
        else {
            os_sleep_ms(5);
        }

auadc_proc_again:
        if(auadc_proc_s->adc_s.data_size - auadc_proc_s->adc_s.data_offset >= auadc_proc_s->frame_size) {        
            nsamples = auadc_proc_s->frame_size;
            buf = auadc_proc_s->adc_s.buf + auadc_proc_s->adc_s.data_offset;

#if AUDIO_PROCESS
            req.near_data = buf;
            req.far_data = NULL;
            req.out_data = buf;
            req.nsamples = nsamples; 
            auproc_process(auadc_proc_s->auproc_dev, &req);               
#endif
			
            auadc_proc_s->energy = auadc_calc_energy_asm(buf, auadc_proc_s->soft_gain, nsamples);

#if MAGIC_VOICE_EN
            magic_voice_write(auadc_proc_s->magVo_s, buf, &nsamples);
magic_voice_read_again:
            nsamples = magic_voice_read_available(auadc_proc_s->magVo_s);
#endif

            if(nsamples >= auadc_proc_s->frame_size) {
                send_frame_buf = msi_alloc_fb(auadc_proc_s->msi, NULL, NULL, nsamples * sizeof(int16), 0, 0);
                if(send_frame_buf) {

#if MAGIC_VOICE_EN
                    magic_voice_read(auadc_proc_s->magVo_s, (int16*)(send_frame_buf->data), &nsamples);
#else
                    hw_memcpy(send_frame_buf->data, buf, nsamples * sizeof(int16));
#endif

                    send_frame_buf->mtype = MEDIA_DATA_AUDIO;	
                    send_frame_buf->stype = AUDIO_CODEC_PCM_S16LE;
                    send_frame_buf->time = os_jiffies();
                    send_frame_buf->codec_info = &(auadc_proc_s->audio_info);
                    msi_output_fb(auadc_proc_s->msi, send_frame_buf, 0);
                }
#if MAGIC_VOICE_EN
                goto magic_voice_read_again;
#endif
            }
            auadc_proc_s->adc_s.data_offset += auadc_proc_s->frame_size;
            goto auadc_proc_again;
        }
        if(auadc_proc_s->adc_s.data_size > auadc_proc_s->adc_s.data_offset) {
            os_memmove(auadc_proc_s->adc_s.buf, auadc_proc_s->adc_s.buf + auadc_proc_s->adc_s.data_offset, 
                        (auadc_proc_s->adc_s.data_size - auadc_proc_s->adc_s.data_offset) * sizeof(int16));
        }
        auadc_proc_s->adc_s.data_size -= auadc_proc_s->adc_s.data_offset;
        auadc_proc_s->adc_s.data_offset = 0;
    }
	auadc_proc_s->stop = 2;
}

int32_t auadc_proc_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
	AUADC_PROC_STRUCT *auadc_proc_s = (AUADC_PROC_STRUCT*)msi->priv;
    switch(cmd_id) {
        case MSI_CMD_TRANS_FB:
        {
            ret = RET_ERR;
            struct framebuff *frame_buf = (struct framebuff *)param1;
            if(frame_buf->mtype == MEDIA_DATA_AUDIO) {
                ret = RET_OK;
            } 
            break;
        }
        case MSI_CMD_FREE_FB:
		{
			break;
		}
		case MSI_CMD_POST_DESTROY:
		{
			if(auadc_proc_s) {
                if(auadc_proc_s->task_hdl) {
                    auadc_proc_s->stop = 1;
                    while(auadc_proc_s->stop != 2) {
                        os_sleep_ms(1);
                    }
                }
                if(auadc_proc_s->adc_s.buf) {
                    os_free_psram(auadc_proc_s->adc_s.buf);
                }
                if(auadc_proc_s->dac_s.buf) {
                    os_free_psram(auadc_proc_s->dac_s.buf);
                }
#if AUDIO_PROCESS
				if(auadc_proc_s->auproc_dev) {
					auproc_close(auadc_proc_s->auproc_dev);
				}
#endif
#if MAGIC_VOICE_EN
                if(auadc_proc_s->magVo_s) {
                    magic_voice_deinit(auadc_proc_s->magVo_s);
                }
#endif
				os_free_psram(auadc_proc_s);
			}
            msi_del_output(NULL, "S_AUADC", msi, NULL);
            msi_del_output(NULL, "dac_msg", msi, NULL);
			break; 
		} 
        default:
            break;    
    }
    return ret;
}

struct msi *auadc_proc_init(uint32 samplerate, uint32 channels, uint32 frame_size, uint32 soft_gain)
{
    uint8 isnew = 0;
    int32 ret = RET_ERR;
    struct msi *msi = NULL;
    AUADC_PROC_STRUCT *auadc_proc_s = NULL;

    msi = msi_new("auadc_proc", 8, &isnew);
    if(msi == NULL) {
        return NULL;
    }
    if(isnew) {
        auadc_proc_s = (AUADC_PROC_STRUCT*)os_zalloc_psram(sizeof(AUADC_PROC_STRUCT));
        if(auadc_proc_s == NULL) {
            os_printf("malloc auadc_proc_s fail!\r\n");
            goto auadc_proc_init_fail;
        }
        msi->enable = 1;
        msi->action = (msi_action)auadc_proc_msi_action;
		msi->priv = auadc_proc_s;
        msi->fb_limits.counter = 16;
        auadc_proc_s->msi = msi;
        auadc_proc_s->soft_gain = soft_gain;
		msi_add_output(NULL, "S_AUADC", msi, NULL);
#if AUDIO_PROCESS
        samplerate = 8000;     //only support 8k now
        channels = 1;          //only support 1 now
        frame_size = 160;      //only support 160 now
		msi_add_output(NULL, "dac_msg", msi, NULL);
        auadc_proc_s->auproc_dev = (struct auproc_device*)dev_get(HG_AUDIO_PROCESS_DEVID);
        if(auadc_proc_s->auproc_dev == NULL) {
            goto auadc_proc_init_fail;
        }
        ret = auproc_open(auadc_proc_s->auproc_dev, samplerate, channels);
        if(ret == RET_ERR) {
            os_printf("audio_process_init fail!\r\n");
            goto auadc_proc_init_fail;
        }
#endif
#if MAGIC_VOICE_EN
        samplerate = 8000;     //only support 8k now
        channels = 1;          //only support 1 now
        frame_size = 160;      //only support 160 now
        auadc_proc_s->magVo_s = magic_voice_init(samplerate, frame_size);
        if(auadc_proc_s->magVo_s == NULL) {
            goto auadc_proc_init_fail;
        }
#endif
		auadc_proc_s->audio_info.sample_rate = samplerate;  
		auadc_proc_s->audio_info.channels = channels;   
        auadc_proc_s->audio_info.frame_size = frame_size; 
		auadc_proc_s->frame_size = frame_size;
        auadc_proc_s->task_hdl = os_task_create("auadc_proc_task", auadc_proc_task, auadc_proc_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
    }
	ret = RET_OK;
    return msi;
auadc_proc_init_fail:
    msi_destroy(msi);
    return NULL;
}

int32 auadc_proc_deinit(struct msi *msi)
{
    msi_destroy(msi);
    return RET_OK;
}