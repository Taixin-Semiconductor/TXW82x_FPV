#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "audio_dac.h"
#include "dev/audio/ausys.h"
#include "dev/audio/ausys_da.h"
#include "lib/audio/wsola/wsola_process.h"
#include "lib/audio/audio_proc/audio_proc.h"

extern AUPROC_HDL *global_auproc_hdl;

typedef int32_t (*audac_write_func)(int16_t *audac_buf, uint32_t len);

enum {
	end_msg = 1,
	clear_msg,  
};
struct audac_tpc_struct {
    float speed;
    float pitch;
    uint32_t sampleRate;
    WsolaStream *wsola_stream;   
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
	uint8_t audac_stop;
	int16_t *empty_buf;
	int16_t *resample_buf;
    uint32_t data_nbytes;
	uint32_t data_nsamples;
    uint32_t filter_type;
    uint32_t sampleRate; 
    uint32_t volume;  
	struct msi *msi;
	struct audac_cache_struct cache_struct;
#if AUDAC_TPC_PROCESS
    struct audac_tpc_struct tpc_struct;
#endif
	audac_write_func write_func;
	struct os_task deal_task_hdl;
	struct os_task msg_task_hdl;
};
static struct audac_struct *audac_s = NULL;

int32_t audac_write_data(int16_t *audac_buf, uint32_t len)
{
	int32_t res = 0;
	res = ausys_da_put(audac_buf, len);
	return res;
}

void audac_cache_s_clear(struct audac_cache_struct *audac_cache_s)
{
	audac_cache_s->res_nsamples = audac_s->data_nsamples;
	audac_cache_s->d_offset = 0;
	audac_cache_s->s_offset = 0;
}

static void set_audac_tpc_speed(struct audac_struct *audac_s, float speed)
{
#if AUDAC_TPC_PROCESS
	if((speed > 2.0f) || (speed < 0.5f)) {
		AUDAC_INFO("audac not support speed:%f!\r\n",speed);
		return;
	}
    audac_s->tpc_struct.speed = speed;
#endif
}

static void get_audac_tpc_speed(struct audac_struct *audac_s, float *speed)
{
#if AUDAC_TPC_PROCESS
	*speed = audac_s->tpc_struct.speed;
#endif
}

static void set_audac_tpc_pitch(struct audac_struct *audac_s, float pitch)
{
#if AUDAC_TPC_PROCESS
	if((pitch > 2.0f) || (pitch < 0.5f)) {
		AUDAC_INFO("audac not support pitch:%f!\r\n",pitch);
		return;
	}
    audac_s->tpc_struct.pitch = pitch;
#endif    
}

static void get_audac_tpc_pitch(struct audac_struct *audac_s, float *pitch)
{
#if AUDAC_TPC_PROCESS
	*pitch = audac_s->tpc_struct.pitch;
#endif
}

static void set_audac_tpc_sampleRate(struct audac_struct *audac_s, uint32_t sampleRate)
{
#if AUDAC_TPC_PROCESS
    audac_s->tpc_struct.sampleRate = sampleRate;
#endif    
}

void set_audac_filter_type(struct audac_struct *audac_s, uint32_t filter_type)
{
	uint32_t wait_empty_cnt = 0;
    if(audac_s->filter_type == filter_type) {
        return;
	}
	audac_s->filter_type = FSTYPE_NONE;
    while((!audac_s->is_empty) && (wait_empty_cnt++<1000))
		os_sleep_ms(1);     	
    audac_s->filter_type = filter_type;
}

int get_audac_filter_type(struct audac_struct *audac_s)
{
    return audac_s->filter_type;
}
  
void set_audac_samplingrate(struct audac_struct *audac_s, uint32_t sampling_rate)
{
    uint32_t last_filter_type;
	uint32_t wait_empty_cnt = 0;
    int32_t ret = 0;

	AUDAC_INFO("change audio dac samplingRate,last:%d,current:%d\r\n",audac_s->sampleRate,sampling_rate);
	if((sampling_rate == audac_s->sampleRate) || (sampling_rate < 0))
		return;
	last_filter_type = get_audac_filter_type(audac_s);
	set_audac_filter_type(audac_s, FSTYPE_NONE);  
	while((!audac_s->is_empty) && (wait_empty_cnt++<5000))
		os_sleep_ms(1);  
	if(global_auproc_hdl && global_auproc_hdl->aec && global_auproc_hdl->res) {
		ret = audio_resample_config(global_auproc_hdl, sampling_rate, AUDAC_SAMPLERATE);
		if(ret == RET_OK) {
			audac_s->sampleRate = sampling_rate; 
		}  
		os_printf("audio_resample_config:%d\n",ret);
	}
	else {
		set_audac_tpc_sampleRate(audac_s, sampling_rate);   
		ret = ausys_da_change_sample_rate(sampling_rate);
		if(ret == RET_OK) {
			audac_s->sampleRate = sampling_rate; 
		}  
	}
	audac_s->data_nbytes = sampling_rate*AUDAC_TIME_INTERVAL*2/1000;  
	audac_s->data_nsamples = audac_s->data_nbytes / 2;    
	set_audac_filter_type(audac_s, last_filter_type);   
}

uint32_t get_audac_samplingrate(struct audac_struct *audac_s)
{
	return audac_s->sampleRate;     
}

void set_audac_volume(struct audac_struct *audac_s, uint32_t volume)
{
    int32_t ret = 0;
    if(audac_s->volume == volume)
        return;
    ret = ausys_da_change_volume(volume);    
    if(ret == RET_OK) 
        audac_s->volume = volume;       
}

uint32_t get_audac_volume(struct audac_struct *audac_s)
{
	uint32_t volume = 0;
	ausys_da_get_cur_volume(&volume);
	return volume;         
}

#if AUDAC_TPC_PROCESS

void wsolaStream_flush_to_audac(struct audac_struct *audac_s)
{
	int32_t output_nsamples = 0;
	uint32_t output_available = 0;
	struct audac_cache_struct *audac_cache_s = &audac_s->cache_struct;
	struct audac_tpc_struct *audac_tpc_s = &audac_s->tpc_struct;	

	wsolaStream_flush(audac_tpc_s->wsola_stream);
	output_available = wsolaStream_output_available(audac_tpc_s->wsola_stream);
	while(output_available > 0) {
		if(output_available >= audac_cache_s->res_nsamples) {
			output_nsamples = wsolaStream_output_data(audac_tpc_s->wsola_stream, 
							  audac_cache_s->buf + audac_cache_s->d_offset, audac_cache_s->res_nsamples);
			if(global_auproc_hdl && global_auproc_hdl->aec && global_auproc_hdl->res) {
				audio_resample_data(global_auproc_hdl, audac_cache_s->buf, audac_s->data_nsamples, audac_s->resample_buf, &resample_nsamples);
				ausys_da_put(audac_s->resample_buf, (resample_nsamples << 1));
			}
			else {
				ausys_da_put(audac_cache_s->buf, audac_s->data_nbytes);
			}
		}
		else if(output_available) {
			output_nsamples = wsolaStream_output_data(audac_tpc_s->wsola_stream, 
							  audac_cache_s->buf + audac_cache_s->d_offset, output_available);
			audac_cache_s->res_nsamples -= output_nsamples;
			os_memset(audac_cache_s->buf + audac_cache_s->d_offset + 
						output_nsamples, 0, audac_cache_s->res_nsamples * sizeof(int16_t));
			if(global_auproc_hdl && global_auproc_hdl->aec && global_auproc_hdl->res) {
				audio_resample_data(global_auproc_hdl, audac_cache_s->buf, audac_s->data_nsamples, audac_s->resample_buf, &resample_nsamples);
				ausys_da_put(audac_s->resample_buf, (resample_nsamples << 1));
			}
			else {
				ausys_da_put(audac_cache_s->buf, audac_s->data_nbytes);
			}			
		}
		output_available -= output_nsamples;
		audac_cache_s->res_nsamples = audac_s->data_nsamples;
		audac_cache_s->d_offset = 0;
	}
	wsola_stream_clean(audac_tpc_s->wsola_stream);
}
 
void audac_deal_task(void *d)
{
    uint8_t msi_empty = 1;
	uint8_t last_msi_empty = 1;
	uint8_t end_stream = 0;
	uint8_t clear_finish = 1;
	int16_t *data = NULL;
	int32_t ret = 0;
	int32_t output_nsamples = 0;
	int32_t input_nsamples = 0;
    uint32_t output_available = 0;
	uint32_t data_nsamples = 0;
    uint32_t current_sampleRate = 0;
	uint32_t cache_msg = 0;
    float current_pitch = 1.0f;
    float current_speed = 1.0f;
	struct framebuff *frame_buf = NULL;
	struct audac_struct *audac_s = (struct audac_struct*)d;
	struct audac_cache_struct *audac_cache_s = &audac_s->cache_struct;
	struct audac_tpc_struct *audac_tpc_s = &audac_s->tpc_struct;

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
		else if(cache_msg == clear_msg)
			clear_finish = 0;

        current_speed = wsolaStream_get_speed(audac_tpc_s->wsola_stream);
        current_pitch = wsolaStream_get_pitch(audac_tpc_s->wsola_stream);
        current_sampleRate = (uint32_t)wsolaStream_get_sampleRate(audac_tpc_s->wsola_stream);
		if((current_sampleRate != audac_tpc_s->sampleRate) && msi_empty) {
			wsolaStream_flush_to_audac(audac_s);
            wsolaStream_deinit(audac_tpc_s->wsola_stream);
            audac_tpc_s->wsola_stream = wsolaStream_init(audac_tpc_s->sampleRate, 1, 
                                          audac_tpc_s->speed, audac_tpc_s->pitch, 320, 640);
		}
        else if((current_speed != audac_tpc_s->speed) || (current_pitch != audac_tpc_s->pitch)) { 
			wsolaStream_set_speed(audac_tpc_s->wsola_stream, audac_tpc_s->speed);
			wsolaStream_set_pitch(audac_tpc_s->wsola_stream, audac_tpc_s->pitch);
		}
		else {
            if(msi_empty && (last_msi_empty==0) && end_stream) {
                wsolaStream_flush_to_audac(audac_s);
				if(audac_cache_s->d_offset) {
					os_memset(audac_cache_s->buf + audac_cache_s->d_offset, 0, 
							  audac_cache_s->res_nsamples * sizeof(int16_t));
					if(global_auproc_hdl && global_auproc_hdl->aec && global_auproc_hdl->res) {
						audio_resample_data(global_auproc_hdl, audac_cache_s->buf, audac_s->data_nsamples, audac_s->resample_buf, &resample_nsamples);
						ausys_da_put(audac_s->resample_buf, (resample_nsamples << 1));
					}
					else {
						ausys_da_put(audac_cache_s->buf, audac_s->data_nbytes);
					}
					audac_cache_s->res_nsamples = audac_s->data_nsamples;
					audac_cache_s->d_offset = 0;						
				}
			}
			else if(!msi_empty) {
				output_available = wsolaStream_output_available(audac_tpc_s->wsola_stream);
				if(output_available >= audac_cache_s->res_nsamples) {
					output_nsamples = wsolaStream_output_data(audac_tpc_s->wsola_stream, 
									  audac_cache_s->buf + audac_cache_s->d_offset, audac_cache_s->res_nsamples);
					if(global_auproc_hdl && global_auproc_hdl->aec && global_auproc_hdl->res) {
						audio_resample_data(global_auproc_hdl, audac_cache_s->buf, audac_s->data_nsamples, audac_s->resample_buf, &resample_nsamples);
						ausys_da_put(audac_s->resample_buf, (resample_nsamples << 1));
					}
					else {
						ausys_da_put(audac_cache_s->buf, audac_s->data_nbytes);
					}
					output_available -= output_nsamples;
					audac_cache_s->res_nsamples = audac_s->data_nsamples;
					audac_cache_s->d_offset = 0;
				}
				else if(output_available) {
					output_nsamples = wsolaStream_output_data(audac_tpc_s->wsola_stream, 
									  audac_cache_s->buf + audac_cache_s->d_offset, output_available);
					audac_cache_s->res_nsamples -= output_nsamples;
					audac_cache_s->d_offset += output_nsamples;	
					output_available = 0;				
				}
			}
			else if(msi_empty)
				end_stream = 0;
		}

		last_msi_empty = msi_empty;
		if(!frame_buf) {
			frame_buf = msi_get_fb(audac_s->msi, 0);
			if(frame_buf) {
				data = (int16_t*)frame_buf->data;
				data_nsamples = frame_buf->len / 2;
				audac_cache_s->s_offset = 0;
			}
			else {
				clear_finish = 1;
				msi_empty = 1;
				os_sleep_ms(1);
			}		 
		}
		if(frame_buf) {
			if(clear_finish) {
				msi_empty = 0;
				audac_s->is_empty = 0;
				input_nsamples = wsolaStream_input_data(audac_tpc_s->wsola_stream, data+audac_cache_s->s_offset, data_nsamples);
				data_nsamples -= input_nsamples;
				audac_cache_s->s_offset += input_nsamples;
			}
			else {
				msi_empty = 1; 
				data_nsamples = 0;
				wsolaStream_clean(audac_tpc_s->wsola_stream);
				audac_cache_s_clear(audac_cache_s);
			}
			if(data_nsamples==0) {
				msi_delete_fb(audac_s->msi, frame_buf);
				AUDAC_DEBUG("audac delete framebuff:%p,type:%x\r\n",
							 frame_buf,(frame_buf->mtype<<16|frame_buf->stype));
				frame_buf = NULL;				
			}
		}
	}	
	audac_s->audac_stop = 2;
}
#else
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
		else if(cache_msg == clear_msg)
			clear_finish = 0;

		frame_buf = msi_get_fb(audac_s->msi,0);
		if(frame_buf) {
			data = (int16_t*)frame_buf->data;
			data_nsamples = (frame_buf->len)/2;
			audac_cache_s->s_offset = 0;		
			while(data_nsamples >= audac_cache_s->res_nsamples) {
				if(!clear_finish) {
					audac_cache_s_clear(audac_cache_s);
					data_nsamples = 0;
					break;					
				}
				os_memcpy(audac_cache_s->buf + audac_cache_s->d_offset, 
						  data + audac_cache_s->s_offset, audac_cache_s->res_nsamples * sizeof(int16_t));
				data_nsamples -= audac_cache_s->res_nsamples;
				audac_cache_s->s_offset += audac_cache_s->res_nsamples;
				audac_s->is_empty = 0;
				if(global_auproc_hdl && global_auproc_hdl->aec && global_auproc_hdl->res) {
					audio_resample_data(global_auproc_hdl, audac_cache_s->buf, audac_s->data_nsamples, audac_s->resample_buf, &resample_nsamples);
					ausys_da_put(audac_s->resample_buf, (resample_nsamples << 1));
				}
				else {
					ausys_da_put(audac_cache_s->buf, audac_s->data_nbytes);
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
			AUDAC_DEBUG("audac delete framebuff:%p,len:%d,type:%x\r\n",
						frame_buf, frame_buf->len, (frame_buf->mtype<<16|frame_buf->stype));  
			msi_delete_fb(audac_s->msi, frame_buf);
			frame_buf = NULL;
		}
		else {
			clear_finish = 1;
			if((audac_cache_s->d_offset > 0) && end_stream) {	
				os_memset(audac_cache_s->buf + audac_cache_s->d_offset, 0, 
						 (audac_s->data_nsamples - audac_cache_s->d_offset) * sizeof(int16_t));
				if(global_auproc_hdl && global_auproc_hdl->aec && global_auproc_hdl->res) {
					audio_resample_data(global_auproc_hdl, audac_cache_s->buf, audac_s->data_nsamples, audac_s->resample_buf, &resample_nsamples);
					ausys_da_put(audac_s->resample_buf, (resample_nsamples << 1));
				}
				else {
					ausys_da_put(audac_cache_s->buf, audac_s->data_nbytes);
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
#endif

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
	ausys_da_register_msg(AUSYS_DA_MSG_PLAY_HALF | AUSYS_DA_MSG_FIFO_EMPTY);
#if AUDAC_TPC_PROCESS
    audac_s->tpc_struct.sampleRate = audac_s->sampleRate;
    audac_s->tpc_struct.speed = 1.0f;
    audac_s->tpc_struct.pitch = 1.0f;
    audac_s->tpc_struct.wsola_stream = wsolaStream_init(audac_s->tpc_struct.sampleRate, 1, 
                                         audac_s->tpc_struct.speed, audac_s->tpc_struct.pitch, 320, 640);
	if(!audac_s->tpc_struct.wsola_stream) {
		AUDAC_INFO("audac tpc init fail!\r\n");
		return RET_ERR;
	}
#endif
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
			ret = RET_OK+1;
			struct framebuff *frame_buf = (struct framebuff *)param1;
			if(frame_buf->mtype == F_AUDIO) {
				if(audac_s && (frame_buf->stype == get_audac_filter_type(audac_s))) {
					ret = RET_OK;
				}
			} 
			break;
		}
        case MSI_CMD_AUDAC:
		{
			if(!audac_s) {
				AUDAC_INFO("audac do cmd:%d fail!\r\n",param1);
				break;
			}
			uint32_t cmd_self = (uint32_t)param1;
			switch(cmd_self) {
				case MSI_AUDAC_SET_SAMPLING_RATE:
				{
					uint32_t sampling_rate = (uint32_t)param2;
					set_audac_samplingrate(audac_s, sampling_rate);
					break;	
				}		
				case MSI_AUDAC_GET_SAMPLING_RATE:
				{
					*((uint32_t*)param2) = get_audac_samplingrate(audac_s);
					break;
				}				
				case MSI_AUDAC_SET_FILTER_TYPE:
				{
					uint32_t filter_type = (uint32_t)param2;
					set_audac_filter_type(audac_s, filter_type);
					break;
				}
				case MSI_AUDAC_GET_FILTER_TYPE:
				{
					*((uint32_t*)param2) = get_audac_filter_type(audac_s);
					break;
				}
				case MSI_AUDAC_SET_VOLUME:
				{
					uint32_t volume = (uint32_t)param2;
					set_audac_volume(audac_s, volume);
					break;
				}
				case MSI_AUDAC_GET_VOLUME:
				{
					*((uint32_t*)param2) = get_audac_volume(audac_s);
					break;		
				}		
				case MSI_AUDAC_SET_SPEED:
				{
					float speed = ((float)param2)/100;
					set_audac_tpc_speed(audac_s, speed); 
					break; 
				}                          
				case MSI_AUDAC_GET_SPEED:
				{
					float speed = 1.0f;
					get_audac_tpc_speed(audac_s, &speed);
					*((uint32_t*)param2) = (uint32_t)(speed*100);
					break;
				}			
				case MSI_AUDAC_SET_PITCH:
				{
					float pitch = ((float)param2)/100;
					set_audac_tpc_pitch(audac_s, pitch);    
					break;
				}
				case MSI_AUDAC_GET_PITCH:
				{
					float pitch = 1.0f;
					get_audac_tpc_pitch(audac_s, &pitch);
					*((uint32_t*)param2) = (uint32_t)(pitch*100);
					break;
				}
				case MSI_AUDAC_CLEAR_STREAM:
				{
					if(audac_s->cache_struct.cache_msg.hdl)
						os_msgq_put(&audac_s->cache_struct.cache_msg, clear_msg, osWaitForever);
					break;
				}
				case MSI_AUDAC_END_STREAM:
				{
					if(audac_s->cache_struct.cache_msg.hdl)
						os_msgq_put(&audac_s->cache_struct.cache_msg, end_msg, osWaitForever);
					break;
				}
				case MSI_AUDAC_GET_EMPTY:
				{
					*((uint32_t*)param2) = (uint32_t)(audac_s->is_empty);
					break;
				}
				case MSI_AUDAC_TEST_MODE:
				{
					if(param2 == 1) {
						set_audac_filter_type(audac_s, FSTYPE_NONE);
						set_audac_samplingrate(audac_s, 48000);
						ausys_da_test_mode(AUSYS_DA_TEST_OPEN, 0, 0, 0);
						ausys_da_test_mode(AUSYS_DA_TEST_PLAY_SINE, 0, 0, 0);
					}
					else if(param2 == 0) {
						ausys_da_test_mode(AUSYS_DA_TEST_CLOSE, 0, 0, 0);
					}
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
			#if AUDAC_TPC_PROCESS
				if(audac_s->tpc_struct.wsola_stream)
					wsolaStream_deinit(audac_s->tpc_struct.wsola_stream);
			#endif
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
        ausys_da_init(audac_s->sampleRate, 16, AUSYS_AUDA, AUDAC_LEN*AUDAC_QUEUE_NUM, AUDAC_LEN, audac_s->data_nbytes);
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
