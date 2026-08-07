#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "dev/audio/ausys.h"
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
    uint32_t data_len;
    uint32_t sampleRate;
    struct msi *msi;
    struct os_msgqueue msg;
    struct os_task task_hdl;    
    struct fbpool tx_pool;
    type_ausys_ad_user_cb callback_func;	
#if AUDIO_PROCESS
	AUPROC_HDL *auproc_hdl;
#endif
};
static struct auadc_struct *auadc_s = NULL;

void auadc_deal_task(void *d)
{
    int16_t *data;  
    int32_t tmp32;
	int32_t ret = 0;
    uint32_t samples_len;
	struct framebuff *frame_buf = NULL;
    struct auadc_struct *auadc_s = (struct auadc_struct*)d;
	struct ausys_ad_msg ausys_msg;

	auadc_s->auadc_stop = 0;
    while(1) {
		if(auadc_s->auadc_stop)
			break;
		ret = ausys_ad_get_msg(&ausys_msg, osWaitForever);
		if((ret == RET_OK) && (ausys_msg.type == AUSYS_AD_MSG_PLAY_DONE)) {
get_frame_buf:
			frame_buf = fbpool_get(&auadc_s->tx_pool, 0, auadc_s->msi);
			if(frame_buf) {
				data = (int16_t *)frame_buf->data;
				os_memcpy(data, (const void*)(ausys_msg.ad_content.fifo_cur_addr), ausys_msg.ad_content.fifo_cur_len);
				frame_buf->len = ausys_msg.ad_content.fifo_cur_len;
				frame_buf->time = os_jiffies();
				samples_len = frame_buf->len/2;
#if AUDIO_PROCESS
				audio_process_data(auadc_s->auproc_hdl, data, samples_len);
#endif
				for(uint32_t i=0; i<samples_len; i++) {
					tmp32 = *(data+i) * AUADC_SOFT_GAIN;
					*(data+i) = LIMIT(tmp32, 32767, -32768);
				#if AUADC_OUTPUT_SIN
					*(data+i) = sin1khz_table[i%8];
				#endif
				}
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

int32_t auadc_start(struct auadc_struct *s)
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
	ausys_ad_record();
    ret = os_msgq_init(&auadc_s->msg, MAX_AUADC_TXBUF);
    if(ret != RET_OK) {
		AUADC_INFO("auadc create msg fail!\r\n");
        return RET_ERR;
	}
#if AUDIO_PROCESS
	auadc_s->auproc_hdl = audio_process_init(auadc_s->sampleRate, 1, (AUADC_SAMPLERATE/1000*AUPROC_FRAME_MS));
	if(auadc_s->auproc_hdl == NULL) {
		AUADC_INFO("audio_process_init fail!\r\n");
		return RET_ERR;
	}
	global_auproc_hdl = auadc_s->auproc_hdl;
#endif
	ausys_ad_register_msg(AUSYS_AD_MSG_PLAY_DONE);

    OS_TASK_INIT("auadc_deal_task", &auadc_s->task_hdl, auadc_deal_task, auadc_s, AUADC_TASK_PRIORITY, NULL, 1024);

	return RET_OK;
}

int32_t auadc_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK + 1;
    switch(cmd_id) {
        case MSI_CMD_FREE_FB:
		{
			if(auadc_s) {
				struct framebuff *frame_buf = (struct framebuff *)param1;                
				fbpool_put(&auadc_s->tx_pool, frame_buf);
			}
			ret = RET_OK+1;
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
				if(auadc_s->auproc_hdl)
					audio_process_deinit(auadc_s->auproc_hdl);
				global_auproc_hdl = NULL;
#endif
				AUADC_FREE(auadc_s);
				auadc_s = NULL;
			}
 			ret = RET_OK;
			break; 
		} 
        default:
            break;    
    }
    return ret;
}

int32 auadc_msi_add_output(const char *msi_name)
{
	int32 ret = RET_ERR;
	if(!msi_name) {
		AUADC_INFO("auadc msi add output fail,msi_name is null\n");
		return RET_ERR;
	}
	if(!auadc_s) {
		AUADC_INFO("auadc msi add output fail,auadc msi is null\n");
		return RET_ERR;
	}
	ret = msi_add_output(auadc_s->msi, NULL, msi_name);
	return ret;    
}

int32 auadc_msi_del_output(const char *msi_name)
{
	int32 ret = RET_ERR;
	if(!msi_name) {
		AUADC_INFO("auadc msi add output fail,msi_name is null\n");
		return RET_ERR;
	}
	if(!auadc_s) {
		AUADC_INFO("auadc msi add output fail,auadc msi is null\n");
		return RET_ERR;
	}
	ret = msi_del_output(auadc_s->msi, NULL, msi_name);
	return ret;    
}

int32_t audio_adc_init(void)
{
	int32_t ret = 0;

    struct msi *msi = msi_new("S_AUADC", 0,NULL);
	if(msi) {
        auadc_s = (struct auadc_struct*)AUADC_ZALLOC(sizeof(struct auadc_struct));
        if(!auadc_s) {
            AUADC_INFO("malloc auadc_struct fail!\r\n");
            msi_destroy(msi);
            return RET_ERR;
        }
        msi->enable = 1;
        msi->action = (msi_action)auadc_msi_action;
        auadc_s->msi = msi;
        auadc_s->data_len = AUADC_LEN;
        auadc_s->sampleRate = AUADC_SAMPLERATE;
        ausys_ad_init(auadc_s->sampleRate, 16, AUSYS_AUAD, auadc_s->data_len*AUADC_QUEUE_NUM, auadc_s->data_len, NULL, (uint32)auadc_s);
        ret = auadc_start(auadc_s);
		if(ret != RET_OK) {
			msi_destroy(msi);
			ausys_ad_deinit();
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

int32_t audio_adc_deinit(void)
{
	if(!auadc_s) {
		AUADC_INFO("auadc deinit fail,auadc_s is null!\r\n");
		return RET_ERR;
	}
	auadc_s->auadc_stop = 1;
	while(auadc_s->auadc_stop != 2)
		os_sleep_ms(1);
	ausys_ad_deinit();
	msi_destroy(auadc_s->msi);
	return RET_OK;
}