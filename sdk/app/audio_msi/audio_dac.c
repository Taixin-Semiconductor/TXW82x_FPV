#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "audio_dac.h"
#include "dev/audio/ausys.h"
#include "dev/audio/ausys_da.h"
#include "lib/audio/wsola/wsola_process.h"
#include "lib/audio/audio_proc/audio_proc.h"
#include "audio_media_ctrl/audio_code_ctrl.h"

extern AUPROC_HDL *global_auproc_hdl;

typedef int32_t (*audac_write_func)(int16_t *audac_buf, uint32_t len);

enum {
	end_msg = 1,
	clear_msg,  
};
struct filter_track_struct {
	AUDIO_TRACK *audio_track[15];
	uint32_t audio_track_num;
	struct os_mutex mutex;
};
struct audac_cache_struct {
	int16_t *buf;
	uint32_t s_offset;
	uint32_t d_offset;
	uint32_t res_nsamples;
	struct os_msgqueue cache_msg;
};
struct audac_struct
{   
	uint8_t is_empty;
	uint8_t hold_empty;
	uint8_t delete_mode;
	uint8_t audac_stop;
	int16_t *empty_buf;
	int16_t *resample_buf;
    uint32_t data_nbytes;
	uint32_t data_nsamples;
    uint32_t filter_type;
    uint32_t sampleRate; 
    uint32_t audac_volume; 
	uint32_t call_volume;
	uint32_t media_volume;
	uint32_t bell_volume;
	uint32_t target_volume;
	float cur_soft_volume;
	struct msi *msi;
	struct audac_cache_struct cache_struct;
	struct filter_track_struct filter_track_s;
	audac_write_func write_func;
	struct os_task deal_task_hdl;
	struct os_task msg_task_hdl;
};
static struct audac_struct *audac_s = NULL;

void audac_cache_s_clear(struct audac_cache_struct *audac_cache_s)
{
	audac_cache_s->res_nsamples = audac_s->data_nsamples;
	audac_cache_s->d_offset = 0;
	audac_cache_s->s_offset = 0;
}

void set_audac_filter_track(struct audac_struct *audac_s, uint8_t fiter_none, AUDIO_TRACK *audio_track)
{
	struct filter_track_struct *filter_track_s = &(audac_s->filter_track_s);
	uint32_t wait_empty_cnt = 0;
	os_mutex_lock(&filter_track_s->mutex, osWaitForever);
	if(fiter_none == 1) {
		audac_s->filter_type = FSTYPE_NONE;
		os_mutex_unlock(&filter_track_s->mutex);
		return;
	}
	if((fiter_none==0) && (audio_track==NULL)) {
		goto set_audac_filter_type_end;
	}
	if((audio_track->priority&0xC0) == play_disabled) {
		uint8_t del_priority = audio_track->priority;
		for(uint32_t i=0; i<filter_track_s->audio_track_num; i++) {
			AUDIO_TRACK *cur_audio_track = filter_track_s->audio_track[i];
			if(cur_audio_track == audio_track) {
				os_memmove(&(filter_track_s->audio_track[i]), &(filter_track_s->audio_track[i+1]), (filter_track_s->audio_track_num-(i+1))*sizeof(AUDIO_TRACK*));
				cur_audio_track = filter_track_s->audio_track[i];
			}
			if(((cur_audio_track->priority&0x30)==del_priority) && (cur_audio_track->priority>del_priority)) {
				cur_audio_track->priority--;
			}
		}
		if(filter_track_s->audio_track[filter_track_s->audio_track_num-1] == audio_track) {
			audac_s->filter_type = FSTYPE_NONE;
			while((!audac_s->is_empty) && (wait_empty_cnt++<1000))
				os_sleep_ms(1);					
		}
		filter_track_s->audio_track_num--;
	}
	else if((audio_track->priority&0xC0) == play_interruptible) {
		if(filter_track_s->audio_track_num) {
			for(uint32_t i=(filter_track_s->audio_track_num-1); i>=0; i--) {
				AUDIO_TRACK *cur_audio_track = filter_track_s->audio_track[i];
				if((cur_audio_track->priority&0x30) == 0x10) {
					os_memmove(&(filter_track_s->audio_track[i+2]), &(filter_track_s->audio_track[i+1]), (filter_track_s->audio_track_num-(i+1))*sizeof(AUDIO_TRACK*));
					filter_track_s->audio_track[i+1] = audio_track;
					AUDIO_TRACK *new_audio_track = filter_track_s->audio_track[i+1];
					new_audio_track->priority = cur_audio_track->priority+1;
					filter_track_s->audio_track_num++;
					goto set_audac_filter_type_end;
				}
				else if(i == 0) {
					os_memmove(&(filter_track_s->audio_track[1]), &(filter_track_s->audio_track[0]), (filter_track_s->audio_track_num)*sizeof(AUDIO_TRACK*));
					filter_track_s->audio_track[0] = audio_track;
					AUDIO_TRACK *new_audio_track = filter_track_s->audio_track[0];
					new_audio_track->priority = 0x11;
					filter_track_s->audio_track_num++;
					goto set_audac_filter_type_end;
				}
			}
		}
		filter_track_s->audio_track[0] = audio_track;
		AUDIO_TRACK *new_audio_track = filter_track_s->audio_track[0];
		new_audio_track->priority = 0x11;
		filter_track_s->audio_track_num++;
	}
	else if((audio_track->priority&0xC0) == play_nonInterruptible) {
		for(uint32_t i=0; i<filter_track_s->audio_track_num; i++) {
			AUDIO_TRACK *cur_audio_track = filter_track_s->audio_track[i];
			if((cur_audio_track->priority&0x30) == 0x20) {
				os_memmove(&(filter_track_s->audio_track[i+1]), &(filter_track_s->audio_track[i]), (filter_track_s->audio_track_num-i)*sizeof(AUDIO_TRACK*));
				filter_track_s->audio_track[i] = audio_track;
				AUDIO_TRACK *new_audio_track = filter_track_s->audio_track[i];
				new_audio_track->priority = 0x21;
				filter_track_s->audio_track_num++;
				for(uint32_t j=(i+1); j<filter_track_s->audio_track_num; j++) {
					cur_audio_track = filter_track_s->audio_track[i];
					cur_audio_track->priority++;
				}
				goto set_audac_filter_type_end;
			}
			else if(i == (filter_track_s->audio_track_num-1)) {
				filter_track_s->audio_track[i+1] = audio_track;
				AUDIO_TRACK *new_audio_track = filter_track_s->audio_track[i];
				new_audio_track->priority = 0x21;
				filter_track_s->audio_track_num++;
				goto set_audac_filter_type_end;
			}
		}
		filter_track_s->audio_track[0] = audio_track;
		AUDIO_TRACK *new_audio_track = filter_track_s->audio_track[0];
		new_audio_track->priority = 0x21;
		filter_track_s->audio_track_num++;
	}
set_audac_filter_type_end:

	if(filter_track_s->audio_track_num) {
		audac_s->filter_type = FSTYPE_AUDIO_PCM;
	}
	os_mutex_unlock(&filter_track_s->mutex);
}

AUDIO_TRACK *get_audac_filter_track(struct audac_struct *audac_s)
{
	struct filter_track_struct *filter_track_s = &(audac_s->filter_track_s);
	if(filter_track_s->audio_track_num && (audac_s->filter_type!=FSTYPE_NONE)) {
		return filter_track_s->audio_track[filter_track_s->audio_track_num-1];
	}
	else {
		return NULL;
	}
}
  
void set_audac_samplingrate(struct audac_struct *audac_s, uint32_t sampling_rate)
{
	uint32_t wait_empty_cnt = 0;
    int32_t ret = 0;

	AUDAC_INFO("change audio dac samplingRate,last:%d,current:%d\r\n",audac_s->sampleRate,sampling_rate);
	if((sampling_rate == audac_s->sampleRate) || (sampling_rate < 0))
		return;
	set_audac_filter_track(audac_s, 1, NULL);  
	while((!audac_s->is_empty) && (wait_empty_cnt++<5000))
		os_sleep_ms(1);  
	if(global_auproc_hdl && global_auproc_hdl->aec) {
		ret = audio_resample_config(global_auproc_hdl, sampling_rate, AUDAC_SAMPLERATE);
		if(ret == RET_OK) {
			audac_s->sampleRate = sampling_rate; 
		}  
		os_printf("audio_resample_config:%d\n",ret);
	}
	else {
		ret = ausys_da_change_sample_rate(sampling_rate);
		if(ret == RET_OK) {
			audac_s->sampleRate = sampling_rate; 
		}  
	}
	audac_s->data_nbytes = sampling_rate*AUDAC_TIME_INTERVAL*2/1000;  
	audac_s->data_nsamples = audac_s->data_nbytes / 2;    
	set_audac_filter_track(audac_s, 0, NULL);   
}

uint32_t get_audac_samplingrate(struct audac_struct *audac_s)
{
	return audac_s->sampleRate;     
}

void set_audac_volume(struct audac_struct *audac_s, uint32_t volume)
{
    int32_t ret = 0;
    if(audac_s->audac_volume == volume)
        return;
    ret = ausys_da_change_volume(volume);    
    if(ret == RET_OK) 
        audac_s->audac_volume = volume;       
}

uint32_t get_audac_volume(struct audac_struct *audac_s)
{
	return audac_s->audac_volume;         
}

void audac_soft_volume_adjust(struct audac_struct *audac_s, int16_t *buf, uint32_t nsamples)
{
	for(uint32_t i=0; i<nsamples; i++) {
		buf[i] = (int16_t)(((float)buf[i]) * audac_s->cur_soft_volume);
		if(((uint32_t)(audac_s->cur_soft_volume * 100.0f)) > audac_s->target_volume) {
			audac_s->cur_soft_volume -= 0.01;
		}
		else if(((uint32_t)(audac_s->cur_soft_volume * 100.0f)) < audac_s->target_volume) {
			audac_s->cur_soft_volume += 0.01;
		}
	}
}

int32_t audac_write_data(struct audac_struct *audac_s, void* buf, uint32_t nbytes)
{
	int32_t ret = ausys_da_put(buf, nbytes);
	audac_s->hold_empty = 1;
	audac_s->is_empty = 0;
	return ret;
}

void audac_deal_task(void *d)
{
	uint8_t end_stream = 0;
	uint8_t clear_finish = 1;
	int16_t *data = NULL;
	int32_t ret = 0;
	uint32_t data_nsamples = 0;
	uint32_t resample_nsamples = 0;
	uint32_t cache_msg = 0;

	struct framebuff *frame_buf = NULL;
	struct audac_struct *audac_s = (struct audac_struct*)d;
	struct audac_cache_struct *audac_cache_s = &audac_s->cache_struct;

	audac_s->audac_stop = 0;
	audac_cache_s_clear(audac_cache_s);
	ausys_da_play();
	while(1) {
		if(audac_s->audac_stop)
			break;
		cache_msg = os_msgq_get2(&audac_cache_s->cache_msg, 0, &ret);
		if(ret)
			cache_msg = 0;
		if(cache_msg == end_msg)
			end_stream = 1;
		else if(cache_msg == clear_msg) {
			clear_finish = 0;
			audac_cache_s_clear(audac_cache_s);
		}

		if(!(global_auproc_hdl && global_auproc_hdl->aec)) {
			if(get_audac_volume(audac_s) != audac_s->target_volume) {
				set_audac_volume(audac_s, audac_s->target_volume);
			}
		}

		frame_buf = msi_get_fb(audac_s->msi,0);
		if(frame_buf) {
			data = (int16_t*)frame_buf->data;
			data_nsamples = (frame_buf->len)/2;
			audac_cache_s->s_offset = 0;		
			while(data_nsamples >= audac_cache_s->res_nsamples) {
				if(!clear_finish) {
					data_nsamples = 0;
					break;					
				}
				os_memcpy(audac_cache_s->buf + audac_cache_s->d_offset, 
						  data + audac_cache_s->s_offset, audac_cache_s->res_nsamples * sizeof(int16_t));
				data_nsamples -= audac_cache_s->res_nsamples;
				audac_cache_s->s_offset += audac_cache_s->res_nsamples;
				if(global_auproc_hdl && global_auproc_hdl->aec) {
					audio_resample_data(global_auproc_hdl, audac_cache_s->buf, audac_s->data_nsamples, audac_s->resample_buf, &resample_nsamples);
					audac_soft_volume_adjust(audac_s, audac_s->resample_buf, resample_nsamples);
					audac_write_data(audac_s, audac_s->resample_buf, (resample_nsamples << 1));
				}
				else {
					audac_write_data(audac_s, audac_cache_s->buf, audac_s->data_nbytes);
				}
				audac_cache_s->res_nsamples = audac_s->data_nsamples;
				audac_cache_s->d_offset = 0;
			}
			if(data_nsamples) {
				os_memcpy(audac_cache_s->buf + audac_cache_s->d_offset, 
						  data + audac_cache_s->s_offset, data_nsamples * sizeof(int16_t));
				audac_cache_s->res_nsamples -= data_nsamples;
				audac_cache_s->d_offset += data_nsamples;
				audac_cache_s->s_offset = 0;
				data_nsamples = 0;
			}
			AUDAC_DEBUG("audac delete framebuff:%p,len:%d\r\n",frame_buf, frame_buf->len);  
			msi_delete_fb(audac_s->msi, frame_buf);
			frame_buf = NULL;
		}
		else {
			clear_finish = 1;
			if((audac_cache_s->d_offset > 0) && end_stream) {	
				os_memset(audac_cache_s->buf + audac_cache_s->d_offset, 0, 
						 (audac_s->data_nsamples - audac_cache_s->d_offset) * sizeof(int16_t));
				if(global_auproc_hdl && global_auproc_hdl->aec) {
					audio_resample_data(global_auproc_hdl, audac_cache_s->buf, audac_s->data_nsamples, audac_s->resample_buf, &resample_nsamples);
					audac_soft_volume_adjust(audac_s, audac_s->resample_buf, resample_nsamples);
					audac_write_data(audac_s, audac_s->resample_buf, (resample_nsamples << 1));
				}
				else {
					audac_write_data(audac_s, audac_cache_s->buf, audac_s->data_nbytes);
				}
				audac_cache_s_clear(audac_cache_s);
				data_nsamples = 0;
			}
			else 
				os_sleep_ms(1);
			end_stream = 0;
		}
	}
	audac_s->audac_stop = 2;
}

void audac_msg_task(void *d)
{
	int32_t ret = 0;
	struct ausys_da_msg ausys_msg;

	while(1) {
		if(audac_s->audac_stop == 2)
			break;
		ret = ausys_da_get_msg(&ausys_msg, osWaitForever);
        if((ret == RET_OK) && (ausys_msg.type & AUSYS_DA_MSG_FIFO_EMPTY)) {
			audac_s->is_empty = 1;	
		}
		if((ret == RET_OK) && (ausys_msg.type & AUSYS_DA_MSG_PLAY_HALF)) {
			if(ausys_msg.da_content.fifo_next_len == 0)
				audio_process_fardata(global_auproc_hdl, audac_s->empty_buf, audac_s->data_nsamples);
			else
				audio_process_fardata(global_auproc_hdl, (int16_t*)(ausys_msg.da_content.fifo_next_addr),
																ausys_msg.da_content.fifo_next_len / 2);
		}
	}
	audac_s->audac_stop = 3;
}

int32_t audac_start(struct audac_struct *s)
{
    int32_t ret = 0;
    struct audac_struct *audac_s = (struct audac_struct*)s;
	struct audac_cache_struct *audac_cache_s = &audac_s->cache_struct;
	struct filter_track_struct *filter_track_s = &audac_s->filter_track_s;

	audac_cache_s->buf = (int16_t *)AUDAC_ZALLOC(AUDAC_LEN);
    if(!audac_cache_s->buf) {
		AUDAC_INFO("audac malloc cache_s buf fail!\r\n");
		return RET_ERR;
	}
	ret = os_msgq_init(&audac_cache_s->cache_msg, 1);
	if(ret != RET_OK) {
		AUDAC_INFO("audac create cache_s msg fail!\r\n");
		return RET_ERR;
	}
	ret = os_mutex_init(&filter_track_s->mutex);
	if(ret != RET_OK) {
		AUDAC_INFO("audac create filter_track_s mutex fail!\r\n");
		return RET_ERR;		
	}
	ausys_da_register_msg(AUSYS_DA_MSG_PLAY_HALF | AUSYS_DA_MSG_FIFO_EMPTY);
    OS_TASK_INIT("audac_deal_task", &audac_s->deal_task_hdl, audac_deal_task, audac_s, AUDAC_TASK_PRIORITY, NULL, 1024);
	OS_TASK_INIT("audac_msg_task", &audac_s->msg_task_hdl, audac_msg_task, audac_s, AUDAC_TASK_PRIORITY, NULL, 512);
	return RET_OK;
}

int32_t audac_msi_action(struct msi *msi,uint32_t cmd_id,uint32_t param1,uint32_t param2)
{
    int32_t ret = RET_OK;
    switch(cmd_id) {
        case MSI_CMD_TRANS_FB:
		{
			ret = RET_ERR;
			struct framebuff *frame_buf = (struct framebuff*)param1;
			if(audac_s && (frame_buf->mtype == F_AUDIO)) {
				AUDIO_TRACK *audac_filter_track = get_audac_filter_track(audac_s);
				AUDIO_TRACK *audio_track = (AUDIO_TRACK*)(frame_buf->priv);
				if(audac_filter_track == audio_track) {
					switch(audio_track->track_type) {
						case CALL_TRACK:audac_s->target_volume = audac_s->call_volume;break;
						case MEDIA_TRACK:audac_s->target_volume = audac_s->media_volume;break;
						case BELL_TRACK:audac_s->target_volume = audac_s->bell_volume;break;
						default:break;
					}
					if(audio_track->samplerate != get_audac_samplingrate(audac_s)) {
						set_audac_samplingrate(audac_s, audio_track->samplerate);
					}
					ret = RET_OK;
				}
			} 
			break;
		}
        case MSI_CMD_TRANS_FB_END:
        {
            struct framebuff *frame_buf = (struct framebuff *)param1;
			if(audac_s->delete_mode) {
            	frame_buf = msi_get_fb(msi, 0);
				msi_delete_fb(msi, frame_buf);
			}
            break;
        }            
        case MSI_CMD_AUDAC:
		{
			ret = RET_ERR;
			if(!audac_s) {
				AUDAC_INFO("audac do cmd:%d fail!\r\n",param1);
				break;
			}
			uint32_t cmd_self = (uint32_t)param1;
			switch(cmd_self) {			
				case MSI_AUDAC_SET_FILTER_TRACK:
				{
					AUDIO_TRACK *audio_track = (AUDIO_TRACK*)param2;
					set_audac_filter_track(audac_s, 0, audio_track);
					ret = RET_OK;
					break;
				}
				case MSI_AUDAC_GET_FILTER_TRACK:
				{
					*((uint32_t*)param2) = (uint32_t)get_audac_filter_track(audac_s);
					ret = RET_OK;
					break;
				}		
				case MSI_AUDAC_SET_CALL_VOLUME:
				{
					audac_s->call_volume = param2;
					ret = RET_OK;
					break;
				}
				case MSI_AUDAC_GET_CALL_VOLUME:
				{
					*((uint32_t*)param2) = audac_s->call_volume;
					ret = RET_OK;
					break;		
				}		
				case MSI_AUDAC_SET_MEDIA_VOLUME:
				{
					audac_s->media_volume = param2;
					ret = RET_OK;
					break;
				}
				case MSI_AUDAC_GET_MEDIA_VOLUME:
				{
					*((uint32_t*)param2) = audac_s->media_volume;
					ret = RET_OK;
					break;		
				}		
				case MSI_AUDAC_SET_BELL_VOLUME:
				{
					audac_s->bell_volume = param2;
					ret = RET_OK;
					break;
				}
				case MSI_AUDAC_GET_BELL_VOLUME:
				{
					*((uint32_t*)param2) = audac_s->bell_volume;
					ret = RET_OK;
					break;		
				}		
				case MSI_AUDAC_CLEAR_STREAM:
				{
					if(param2 == (uint32_t)get_audac_filter_track(audac_s)) {
						os_msgq_put(&audac_s->cache_struct.cache_msg, clear_msg, osWaitForever);
						ret = RET_OK;
					}	
					break;
				}
				case MSI_AUDAC_END_STREAM:
				{
					if(param2 == (uint32_t)get_audac_filter_track(audac_s)) {
						os_msgq_put(&audac_s->cache_struct.cache_msg, end_msg, osWaitForever);
						ret = RET_OK;
					}
					break;
				}
				case MSI_AUDAC_DELETE_MODE:
				{
					audac_s->delete_mode = (uint8_t)param2;
				}
				case MSI_AUDAC_GET_EMPTY:
				{
					*((uint32_t*)param2) = (uint32_t)(audac_s->is_empty && audac_s->hold_empty);
					ret = RET_OK;
					break;
				}
				case MSI_AUDAC_HOLD_EMPTY:
				{
					audac_s->hold_empty = (uint8_t)param2;
					ret = RET_OK;
					break;
				}
				case MSI_AUDAC_TEST_MODE:
				{
					if(param2 == 1) {
						set_audac_filter_track(audac_s, 1, NULL);
						set_audac_samplingrate(audac_s, 48000);
						ausys_da_test_mode(AUSYS_DA_TEST_OPEN, 0, 0, 0);
						ausys_da_test_mode(AUSYS_DA_TEST_PLAY_SINE, 0, 0, 0);
					}
					else if(param2 == 0) {
						ausys_da_test_mode(AUSYS_DA_TEST_CLOSE, 0, 0, 0);
					}
					ret = RET_OK;
					break;
				}
				default:
					AUDAC_INFO("audac invalid commond!\r\n");
					break;
			}
			break;
		}		
        case MSI_CMD_POST_DESTROY:
		{
			if(audac_s) {
				if(audac_s->cache_struct.buf) 
					AUDAC_FREE(audac_s->cache_struct.buf);
				if(audac_s->cache_struct.cache_msg.hdl)
					os_msgq_del(&audac_s->cache_struct.cache_msg);
				if(audac_s->filter_track_s.mutex.hdl)
					os_mutex_del(&audac_s->filter_track_s.mutex);
				if(audac_s->empty_buf) {
					AUDAC_FREE(audac_s->empty_buf);
				}
				if(audac_s->resample_buf) {
					AUDAC_FREE(audac_s->resample_buf);
				}
				AUDAC_FREE(audac_s);
				audac_s = NULL;
			}
			break;
		}
        default:
            break;    
    }
    return ret;
}

int32_t audio_dac_init(void)
{
	int32_t ret = 0;
    struct msi *msi = msi_new("R_AUDAC", MAX_AUDAC_RXBUF,NULL);
    if(msi)
    {
        audac_s = (struct audac_struct *)AUDAC_ZALLOC(sizeof(struct audac_struct));
        if(!audac_s) {
            AUDAC_INFO("malloc audac_struct fail!\r\n");
            msi_destroy(msi);
            return RET_ERR;
        } 
		msi->enable = 1;
        msi->action = (msi_action)audac_msi_action;
        audac_s->msi = msi; 
        audac_s->sampleRate = AUDAC_SAMPLERATE;
		audac_s->data_nbytes = audac_s->sampleRate*AUDAC_TIME_INTERVAL*2/1000; 
		audac_s->data_nsamples = audac_s->data_nbytes / 2;
		audac_s->empty_buf = (int16_t*)AUDAC_ZALLOC(sizeof(int16_t) * audac_s->data_nsamples);
		audac_s->resample_buf = (int16_t*)AUDAC_ZALLOC(sizeof(int16_t) * audac_s->data_nsamples);
		if((audac_s->empty_buf==NULL) || (audac_s->resample_buf==NULL)) {
			AUDAC_INFO("malloc audac empty_buf fail!\r\n");
			msi_destroy(audac_s->msi);
			return RET_ERR;
		}
        audac_s->is_empty = 1;
		audac_s->hold_empty = 1;
		audac_s->call_volume = 100;
		audac_s->media_volume = 100;
		audac_s->bell_volume = 100;
		audac_s->target_volume = 100;
		audac_s->audac_volume = 100;
        ausys_da_init(
            audac_s->sampleRate,
            16,
            1,
            AUSYS_AUDA,
            AUDAC_LEN*AUDAC_QUEUE_NUM,
            AUDAC_LEN,
            audac_s->data_nbytes
        );
        ret = audac_start(audac_s);
		if(ret != RET_OK) {
			ausys_da_deinit();
			msi_destroy(audac_s->msi);
			return RET_ERR;
		}
		AUDAC_INFO("audio dac msi init!\r\n");
		return RET_OK;
    }
	else {
		AUDAC_INFO("create auadc msi fail!\r\n");
		return RET_ERR;
	}
}     

int32_t audio_dac_deinit(void)
{
	if(!audac_s) {
		AUDAC_INFO("audac deinit fail,auadc_s is null!\r\n");
		return RET_ERR;
	}	
	audac_s->audac_stop = 1;
	while(audac_s->audac_stop != 3)
		os_sleep_ms(1);
	ausys_da_deinit();
	msi_destroy(audac_s->msi);
	return RET_OK;
}
