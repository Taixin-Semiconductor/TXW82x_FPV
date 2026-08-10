#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "audio_adc.h"

struct auadc_struct
{
	uint8_t auadc_stop;
	uint8_t channels;
    uint32_t data_len;
    uint32_t sampleRate;
    struct msi *msi;
    struct os_msgqueue msg;
    struct os_task *task_hdl;    
    struct fbpool tx_pool;	
	enum ausys_ad_platform platform;
	txAudioInfo_t audio_info;
};

void auadc_read_task(void *d)
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
		ret = ausys_ad_get_msg(auadc_s->platform, &ausys_msg, 10);
		if((ret == RET_OK) && (ausys_msg.type == AUSYS_AD_MSG_PLAY_DONE)) {
get_frame_buf:
			frame_buf = fbpool_get(&auadc_s->tx_pool, 0, auadc_s->msi);
			if(frame_buf) {
				data = (volatile int16_t *)frame_buf->data;
				os_memcpy((void*)data, (const void*)(ausys_msg.ad_content.fifo_cur_addr), ausys_msg.ad_content.fifo_cur_len);
				frame_buf->len = ausys_msg.ad_content.fifo_cur_len;
				frame_buf->time = os_jiffies();
				samples_len = frame_buf->len/2;
				frame_buf->mtype = MEDIA_DATA_AUDIO;	
				frame_buf->stype = AUDIO_CODEC_ADC_READ;
				frame_buf->codec_info = &(auadc_s->audio_info);
				AUADC_DEBUG("auadc send framebuff:%p\r\n",frame_buf); 
				msi_output_fb(auadc_s->msi, frame_buf, 0);
			}
			else {
                AUADC_INFO("ad loss:%d\n",auadc_s->platform);
				os_sleep_ms(10);
				goto get_frame_buf;
			}
		}
    }
	auadc_s->auadc_stop = 2;
}

int32_t auadc_start(struct auadc_struct *s)
{
    struct auadc_struct *auadc_s = (struct auadc_struct*)s;
    struct framebuff *frame_buf;
    uint8_t *data;
    uint32_t data_size;
    int32_t ret = 0;

    fbpool_init(&auadc_s->tx_pool, MAX_AUADC_TXBUF, NULL, NULL);
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
	ausys_ad_register_msg(auadc_s->platform,AUSYS_AD_MSG_PLAY_DONE);
	auadc_s->task_hdl = os_task_create("auadc_read_task", auadc_read_task, auadc_s, AUADC_TASK_PRIORITY, 0, NULL, 1024);
	return RET_OK;
}

int32_t auadc_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
	struct auadc_struct *auadc_s = (struct auadc_struct*)msi->priv;
    switch(cmd_id) {
        case MSI_CMD_FREE_FB:
		{
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
	ret = msi_add_output(msi, NULL, NULL, msi_name);
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
	ret = msi_del_output(msi, NULL, NULL, msi_name);
	return ret;    
}

int32_t audio_adc_set_gain(enum ausys_ad_platform platform, uint32_t gain)
{
	int32_t ret = RET_ERR;
	struct msi *msi = NULL;
	struct auadc_struct *auadc_s = NULL;

	switch(platform) {
		case AUSYS_AUAD:msi = msi_find("S_AUADC", 1);break;
		default:break;
	}
	if(msi) {
		msi_put(msi);
		auadc_s = (struct auadc_struct*)msi->priv;
	}	
	if(auadc_s) {
		ausys_ad_change_volume(auadc_s->platform, gain);
		ret = RET_OK;
	}	
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

int32_t audio_adc_init(enum ausys_ad_platform platform, uint32_t sampleRate, uint32_t channels, uint32_t gain)
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
        auadc_s->sampleRate = sampleRate;
		auadc_s->data_len = sampleRate/1000*channels*2*AUADC_TIME_INTERVAL;
		auadc_s->platform = platform;
		auadc_s->audio_info.sample_rate = sampleRate;
		auadc_s->audio_info.channels = channels;
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
        ret = auadc_start(auadc_s);
		if(ret != RET_OK) {
			msi_destroy(msi);
			ausys_ad_deinit(auadc_s->platform);
			return RET_ERR;
		}
		ausys_ad_change_volume(auadc_s->platform, gain);
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

void audio_adc_dev_out(struct dev_hotplug_info *info)
{
	if(info && info->priv) {
		os_free_psram(info->priv);
	}
	if(info) {
		os_free_psram(info);
	}
}