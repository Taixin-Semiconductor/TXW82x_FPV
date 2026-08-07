#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "audio_adc.h"
#include "lib/audio/audio_proc/audio_proc.h"

#define LIMIT(x,y,z) 		({\
								int16_t tmp16;\
								if(x>y) tmp16=y;\
								else if(x<z) tmp16=z;\
								else tmp16=x;\
								tmp16;\
								}) 

#if AUADC_OUTPUT_SIN
static const int16_t sin1khz_table[8] = {
	5792,8191,5792,0,-5792,-8191,-5792,0,
};
#endif 

AUPROC_HDL *global_auproc_hdl = NULL;

struct auadc_struct
{
	uint8_t auadc_stop;
	uint8_t channels;
	uint16_t soft_gain;
    uint32_t data_len;
    uint32_t sampleRate;
	uint32_t energy;
    struct msi *msi;
    struct os_msgqueue msg;
    struct os_task *task_hdl;    
    struct fbpool tx_pool;
    type_ausys_ad_user_cb callback_func;	
	enum ausys_ad_platform platform;
#if AUDIO_PROCESS
	AUPROC_HDL *auproc_hdl;
#endif
};

extern uint32_t auadc_calc_energy_asm(volatile int16_t *data, uint16_t soft_gain, uint32_t nsamples);

void auadc_deal_task(void *d)
{
    volatile int16_t *data = NULL;  
	int32_t ret = 0;
    uint32_t samples_len;
	struct framebuff *frame_buf = NULL;
    struct auadc_struct *auadc_s = (struct auadc_struct*)d;
	struct ausys_ad_msg ausys_msg;

	auadc_s->auadc_stop = 0;
    while(1) {
		if(auadc_s->auadc_stop)
			break;
		ret = ausys_ad_get_msg(auadc_s->platform, &ausys_msg, osWaitForever);
		if((ret == RET_OK) && (ausys_msg.type == AUSYS_AD_MSG_PLAY_DONE)) {
get_frame_buf:
			frame_buf = fbpool_get(&auadc_s->tx_pool, 0, auadc_s->msi);
			if(frame_buf) {
				data = (volatile int16_t *)frame_buf->data;
				os_memcpy((void*)data, (const void*)(ausys_msg.ad_content.fifo_cur_addr), ausys_msg.ad_content.fifo_cur_len);
				frame_buf->len = ausys_msg.ad_content.fifo_cur_len;
				frame_buf->time = os_jiffies();
				samples_len = frame_buf->len/2;
#if AUADC_OUTPUT_SIN
				for(uint32_t i=0; i<samples_len; i++) {
					*(data+i) = sin1khz_table[i%8];
				}
#else
#if AUDIO_PROCESS
				if(auadc_s->auproc_hdl) {
					audio_process_data(auadc_s->auproc_hdl, (int16_t*)data, samples_len);
				}
#endif
				auadc_s->energy = auadc_calc_energy_asm(data, auadc_s->soft_gain, samples_len);
#endif
				frame_buf->mtype = F_AUDIO;	
				frame_buf->stype = FSTYPE_AUDIO_ADC;
				AUADC_DEBUG("auadc send framebuff:%p\r\n",frame_buf);   
				msi_output_fb(auadc_s->msi, frame_buf);
			}
			else {
				os_sleep_ms(1);
				goto get_frame_buf;
			}
		}
    }
	auadc_s->auadc_stop = 2;
}

int32_t auadc_start(struct auadc_struct *s, uint32_t auproc_enable)
{
    struct auadc_struct *auadc_s = (struct auadc_struct*)s;
    struct framebuff *frame_buf;
    uint8_t *data;
    uint32_t data_size;
    int32_t ret = 0;

    fbpool_init(&auadc_s->tx_pool, MAX_AUADC_TXBUF);
    data_size = auadc_s->data_len;
    for(uint32_t i=0; i<MAX_AUADC_TXBUF; i++) {
        data = (uint8_t*)AUADC_MALLOC(data_size * sizeof(uint8_t));
        if(!data) {
            AUADC_INFO("auadc malloc framebuff data fail!\r\n");
            return RET_ERR;           
        }  
		frame_buf = (auadc_s->tx_pool.pool)+i;
        frame_buf->data = data;
    }
	ausys_ad_record(auadc_s->platform);
    ret = os_msgq_init(&auadc_s->msg, MAX_AUADC_TXBUF);
    if(ret != RET_OK) {
		AUADC_INFO("auadc create msg fail!\r\n");
        return RET_ERR;
	}
#if AUDIO_PROCESS
	if(auproc_enable && global_auproc_hdl == NULL) {
		auadc_s->auproc_hdl = audio_process_init(auadc_s->sampleRate, 1, (auadc_s->sampleRate/1000*AUPROC_FRAME_MS));
		if(auadc_s->auproc_hdl == NULL) {
			AUADC_INFO("audio_process_init fail!\r\n");
			return RET_ERR;
		}
		global_auproc_hdl = auadc_s->auproc_hdl;
	}
#endif
	ausys_ad_register_msg(auadc_s->platform,AUSYS_AD_MSG_PLAY_DONE);
	auadc_s->task_hdl = os_task_create("auadc_deal_task", auadc_deal_task, auadc_s, AUADC_TASK_PRIORITY, 0, NULL, 1024);
	return RET_OK;
}

int32_t auadc_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
	struct auadc_struct *auadc_s = (struct auadc_struct*)msi->priv;
    switch(cmd_id) {
        case MSI_CMD_FREE_FB:
		{
			ret = RET_ERR;
			if(auadc_s) {
				struct framebuff *frame_buf = (struct framebuff *)param1;                
				fbpool_put(&auadc_s->tx_pool, frame_buf);
			}
			break;
		}
		case MSI_CMD_POST_DESTROY:
		{
			if(auadc_s) {
				for(uint32_t i=0; i<MAX_AUADC_TXBUF; i++) {
					struct framebuff *frame_buf = (auadc_s->tx_pool.pool)+i;
					if(frame_buf->data) {
						AUADC_FREE(frame_buf->data);
						frame_buf->data = NULL;
					}
				}
				fbpool_destroy(&auadc_s->tx_pool);
				if(auadc_s->msg.hdl)
					os_msgq_del(&auadc_s->msg);
#if AUDIO_PROCESS
				if(auadc_s->auproc_hdl) {
					audio_process_deinit(auadc_s->auproc_hdl);
					global_auproc_hdl = NULL;
				}
#endif
				AUADC_FREE(auadc_s);
			}
			break; 
		} 
        default:
            break;    
    }
    return ret;
}

struct msi *get_auadc_msi(enum ausys_ad_platform platform)
{
	struct msi *msi = NULL;

	switch(platform) {
		case AUSYS_AUAD:msi = msi_find("S_AUADC", 1);break;
		case AUSYS_PDM:msi = msi_find("S_AUPDM", 1);break;
		case AUSYS_IIS_SLAVER0:msi = msi_find("S_AUIIS0", 1);break;
		case AUSYS_IIS_SLAVER1:msi = msi_find("S_AUIIS1", 1);break;
		default:break;
	}
	if(msi) {
		msi_put(msi);
	}	
	return msi;
}

int32_t auadc_msi_add_output(enum ausys_ad_platform platform, const char *msi_name)
{
	int32_t ret = RET_ERR;
	struct msi *msi = NULL;

	switch(platform) {
		case AUSYS_AUAD:msi = msi_find("S_AUADC", 1);break;
		case AUSYS_PDM:msi = msi_find("S_AUPDM", 1);break;
		case AUSYS_IIS_SLAVER0:msi = msi_find("S_AUIIS0", 1);break;
		case AUSYS_IIS_SLAVER1:msi = msi_find("S_AUIIS1", 1);break;
		default:break;
	}
	if(msi) {
		msi_put(msi);
	}
	else {
		AUADC_INFO("auadc msi add output fail,auadc msi is null\n");
		return RET_ERR;
	}
	ret = msi_add_output(msi, NULL, msi_name);
	return ret;    
}

int32_t auadc_msi_del_output(enum ausys_ad_platform platform, const char *msi_name)
{
	int32_t ret = RET_ERR;
	struct msi *msi = NULL;

	switch(platform) {
		case AUSYS_AUAD:msi = msi_find("S_AUADC", 1);break;
		case AUSYS_PDM:msi = msi_find("S_AUPDM", 1);break;
		case AUSYS_IIS_SLAVER0:msi = msi_find("S_AUIIS0", 1);break;
		case AUSYS_IIS_SLAVER1:msi = msi_find("S_AUIIS1", 1);break;
		default:break;
	}
	if(msi) {
		msi_put(msi);
	}
	else {
		AUADC_INFO("auadc msi add output fail,auadc msi is null\n");
		return RET_ERR;
	}
	ret = msi_del_output(msi, NULL, msi_name);
	return ret;    
}

int32_t audio_adc_get_samplerate(enum ausys_ad_platform platform)
{
	uint32_t samplerate = 8000;
	struct msi *msi = NULL;
	struct auadc_struct *auadc_s = NULL;

	switch(platform) {
		case AUSYS_AUAD:msi = msi_find("S_AUADC", 1);break;
		case AUSYS_PDM:msi = msi_find("S_AUPDM", 1);break;
		case AUSYS_IIS_SLAVER0:msi = msi_find("S_AUIIS0", 1);break;
		case AUSYS_IIS_SLAVER1:msi = msi_find("S_AUIIS1", 1);break;
		default:break;
	}
	if(msi) {
		msi_put(msi);
		auadc_s = (struct auadc_struct*)msi->priv;
	}	
	if(auadc_s) {
		samplerate = auadc_s->sampleRate;
	}
	return samplerate;
}

int32_t audio_adc_set_soft_gain(enum ausys_ad_platform platform, uint32_t soft_gain)
{
	int32_t ret = RET_ERR;
	struct msi *msi = NULL;
	struct auadc_struct *auadc_s = NULL;

	switch(platform) {
		case AUSYS_AUAD:msi = msi_find("S_AUADC", 1);break;
		case AUSYS_PDM:msi = msi_find("S_AUPDM", 1);break;
		case AUSYS_IIS_SLAVER0:msi = msi_find("S_AUIIS0", 1);break;
		case AUSYS_IIS_SLAVER1:msi = msi_find("S_AUIIS1", 1);break;
		default:break;
	}
	if(msi) {
		msi_put(msi);
		auadc_s = (struct auadc_struct*)msi->priv;
	}	
	if(auadc_s) {
		auadc_s->soft_gain = soft_gain;
		ret = RET_OK;
	}	
	return ret;
}

uint32_t audio_adc_get_energy(enum ausys_ad_platform platform)
{
	struct msi *msi = NULL;
	struct auadc_struct *auadc_s = NULL;

	switch(platform) {
		case AUSYS_AUAD:msi = msi_find("S_AUADC", 1);break;
		case AUSYS_PDM:msi = msi_find("S_AUPDM", 1);break;
		case AUSYS_IIS_SLAVER0:msi = msi_find("S_AUIIS0", 1);break;
		case AUSYS_IIS_SLAVER1:msi = msi_find("S_AUIIS1", 1);break;
		default:break;
	}
	if(msi) {
		msi_put(msi);
		auadc_s = (struct auadc_struct*)msi->priv;
	}	
	if(auadc_s) {
		return auadc_s->energy;
	}	
	return 0;	
}

int32_t audio_adc_init(enum ausys_ad_platform platform, uint32_t sampleRate, uint32_t channels, uint32_t soft_gain, uint32_t auproc_enable)
{
	int32_t ret = 0;
	struct msi *msi = NULL;

	switch(platform) {
		case AUSYS_AUAD:msi = msi_new("S_AUADC", 0, NULL);break;
		case AUSYS_PDM:msi = msi_new("S_AUPDM", 0, NULL);break;
		case AUSYS_IIS_SLAVER0:msi = msi_new("S_AUIIS0", 0, NULL);break;
		case AUSYS_IIS_SLAVER1:msi = msi_new("S_AUIIS1", 0, NULL);break;
		default:break;
	}  
	if(msi) {
        struct auadc_struct *auadc_s = (struct auadc_struct*)AUADC_ZALLOC(sizeof(struct auadc_struct));
        if(!auadc_s) {
            AUADC_INFO("malloc auadc_struct fail!\r\n");
            msi_destroy(msi);
            return RET_ERR;
        }
        msi->enable = 1;
        msi->action = (msi_action)auadc_msi_action;
		msi->priv = auadc_s;
        auadc_s->msi = msi;
		auadc_s->channels = channels;
		auadc_s->soft_gain = soft_gain;
        auadc_s->sampleRate = sampleRate;
		auadc_s->data_len = sampleRate/1000*channels*2*AUADC_TIME_INTERVAL;
		auadc_s->platform = platform;
        ausys_ad_init(
            auadc_s->platform,
            auadc_s->sampleRate,
            16,
            auadc_s->channels,
            auadc_s->data_len*AUADC_QUEUE_NUM,
            auadc_s->data_len,
            NULL, 
            (uint32_t)auadc_s
        );
        ret = auadc_start(auadc_s, auproc_enable);
		if(ret != RET_OK) {
			msi_destroy(msi);
			ausys_ad_deinit(auadc_s->platform);
			return RET_ERR;
		}
		AUADC_INFO("auadc init success!\r\n");    
		return RET_OK;
	}
	else {
		AUADC_INFO("create auadc msi fail!\r\n");
		return RET_ERR;
	}
}

int32_t audio_adc_deinit(enum ausys_ad_platform platform)
{
	struct msi *msi = NULL;
	struct auadc_struct *auadc_s = NULL;

	switch(platform) {
		case AUSYS_AUAD:msi = msi_find("S_AUADC", 1);break;
		case AUSYS_PDM:msi = msi_find("S_AUPDM", 1);break;
		case AUSYS_IIS_SLAVER0:msi = msi_find("S_AUIIS0", 1);break;
		case AUSYS_IIS_SLAVER1:msi = msi_find("S_AUIIS1", 1);break;
		default:break;
	}
	if(msi) {
		msi_put(msi);
		auadc_s = (struct auadc_struct*)msi->priv;
	}
	if(!auadc_s) {
		AUADC_INFO("auadc deinit fail,auadc_s is null!\r\n");
		return RET_ERR;
	}
	auadc_s->auadc_stop = 1;
	while(auadc_s->auadc_stop != 2)
		os_sleep_ms(1);
	ausys_ad_deinit(auadc_s->platform);
	msi_destroy(msi);
	return RET_OK;
}
