#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "osal_file.h"
#include "wave_code.h"

#define MAX_WAVE_ENCODE_RXBUF    4

struct wave_encode_struct {
    struct os_event event;
    struct msi *msi;
    struct msi *src_msi;
    void *task_hdl;   
    void *wave_fp; 
    uint8_t destroy_self;
    uint8_t next_status;
    uint8_t current_status;
    uint32_t samplerate;
    uint32_t data_size; 
    TYPE_WAVE_HEAD wave_head; 
};

const unsigned char wav_header[] = {  
    'R', 'I', 'F', 'F',      // "RIFF" 标志  
    0, 0, 0, 0,              // 文件长度  
    'W', 'A', 'V', 'E',      // "WAVE" 标志  
    'f', 'm', 't', ' ',      // "fmt" 标志  
    16, 0, 0, 0,             // 过渡字节（不定）  
    0x01, 0x00,              // 格式类别  
    0x01, 0x00,              // 声道数      
    0, 0, 0, 0,              // 采样率  
    0, 0, 0, 0,              // 位速  
    0x01, 0x00,              // 一个采样多声道数据块大小  
    0x10, 0x00,              // 一个采样占的 bit 数  
    'd', 'a', 't', 'a',      // 数据标记符＂data ＂  
    0, 0, 0, 0               // 语音数据的长度，比文件长度小42一般。这个是计算音频播放时长的关键参数~  
};  

static void wave_encode_thread(void *d)
{
    uint8_t *data = NULL;
	uint32_t data_len = 0;
    struct wave_encode_struct *s = (struct wave_encode_struct *)d;
    struct framebuff *frame_buf = NULL;
    
    s->msi->enable = 1; 
    msi_get(s->msi);

    while(1)
	{
		frame_buf = msi_get_fb(s->msi, 0);
        if(frame_buf) 
        {
            if(s->next_status == AUCODEC_PAUSE) 
                s->current_status = AUCODEC_PAUSE;
            else {
                s->current_status = AUCODEC_RUN;
                data = frame_buf->data;
                data_len = frame_buf->len;
                osal_fwrite(data, 2, data_len/2, s->wave_fp);
                s->data_size += data_len;
            }
            msi_delete_fb(s->msi, frame_buf);
            WAVE_DEBUG("wave encode delete framebuff:%p,len:%d,\r\n",frame_buf,frame_buf->len);
            frame_buf = NULL;
        }
        else
            os_sleep_ms(1);

        if(s->next_status == AUCODEC_EXIT) {
            s->wave_head.riff_chunk.ChunkSize = s->data_size + sizeof(TYPE_WAVE_HEAD) - 8;
            s->wave_head.fmt_chunk.SampleRate = s->samplerate;
            s->wave_head.fmt_chunk.ByteRate = s->samplerate*2;
            s->wave_head.data_chunk.DataSize = s->data_size;
			osal_fseek(s->wave_fp, 0);
			osal_fwrite(&(s->wave_head), 1, sizeof(TYPE_WAVE_HEAD), s->wave_fp);
            goto wave_encode_thread_end;
        }
    }
wave_encode_thread_end:
    os_event_set(&s->event, coder_exit_event, NULL);

    while((s->next_status != AUCODEC_EXIT) && (s->destroy_self == 0)) {
        s->current_status = AUCODEC_END;
        os_sleep_ms(5);
    }

    if(s->src_msi) {
        msi_del_output(s->src_msi, NULL, s->msi->name);
        s->src_msi = NULL;
    }

    if(s->next_status != AUCODEC_EXIT)
        msi_destroy(s->msi);

    msi_put(s->msi);    
}

static int32_t wave_encode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    struct wave_encode_struct *wave_encode_s = (struct wave_encode_struct*)(msi->priv);
    switch(cmd_id) {
		case MSI_CMD_AUCODER:
		{
            ret = RET_ERR;
			if(wave_encode_s) {
				uint32_t cmd_self = (uint32_t)param1;
				switch(cmd_self) {	
                    case MSI_AUCODER_PAUSE:
                    {
                        if(wave_encode_s->current_status == AUCODEC_RUN) {
                            wave_encode_s->next_status = AUCODEC_PAUSE;
                        }
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_CONTINUE:
                    {
                        if(wave_encode_s->current_status == AUCODEC_PAUSE) {
                            wave_encode_s->next_status = AUCODEC_RUN;
                        }
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_GET_STATUS:
                    {
                        *((uint32_t*)param2) = (uint32_t)(wave_encode_s->current_status);
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_SET_SRCMSI:
                    {
                        if(wave_encode_s->src_msi) {
                            msi_del_output(wave_encode_s->src_msi, NULL, msi->name);
                        }
                        wave_encode_s->src_msi = NULL;
                        ret = msi_add_output((struct msi*)param2, NULL, msi->name);
                        if(ret == RET_OK) {
                            wave_encode_s->src_msi = (struct msi*)param2;
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
        case MSI_CMD_TRANS_FB:
        {
            ret = RET_ERR;
            struct framebuff *frame_buf = (struct framebuff *)param1;
            if(frame_buf->mtype == F_AUDIO) {
                ret = RET_OK;
            } 
            break;
        }    
        case MSI_CMD_PRE_DESTROY:
        {
            if(wave_encode_s && wave_encode_s->task_hdl) {
                wave_encode_s->next_status = AUCODEC_EXIT;
            }
            break;
        }     
        case MSI_CMD_POST_DESTROY:
        {
            if(wave_encode_s) {
                if(wave_encode_s->task_hdl) {
                    os_event_wait(&wave_encode_s->event, coder_exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
                if(wave_encode_s->event.hdl) {
                    os_event_del(&wave_encode_s->event);
                }
                if(wave_encode_s->wave_fp) {
                    osal_fclose(wave_encode_s->wave_fp);
                    wave_encode_s->wave_fp = NULL;
                }
                WAVE_CODE_FREE(wave_encode_s);
                wave_encode_s = NULL;
            }
            break;                
        }			
        default:
            break;    
    }
    return ret;
}

struct msi *wave_encode_init(char *filename, uint32_t samplerate, AUENC_INIT *auenc_init)
{  
#if AUDIO_EN
    uint8_t msi_isnew = 0;
	
	struct msi *msi = msi_new("R_WAVE_ENCODE", MAX_WAVE_ENCODE_RXBUF, &msi_isnew);
	if(!msi) {
		WAVE_INFO("create wave encode msi fail!\r\n");
		return NULL;
	}
    else if(!msi_isnew) {
		WAVE_INFO("wave encode msi has been create!\r\n");
        goto wave_encode_init_err;     
    } 
	struct wave_encode_struct *wave_encode_s = (struct wave_encode_struct*)WAVE_CODE_ZALLOC(sizeof(struct wave_encode_struct));
	if(!wave_encode_s) {
		WAVE_INFO("wave_encode_s malloc fail!\r\n");
		goto wave_encode_init_err;
	}
    msi->priv = wave_encode_s;
    msi->action = (msi_action)wave_encode_msi_action; 
    if(os_event_init(&wave_encode_s->event) != RET_OK) {
        WAVE_INFO("create wave encode event fail!\r\n");
        goto wave_encode_init_err;
    }
	if(filename) {
		wave_encode_s->wave_fp = osal_fopen((const char*)filename, "wb+");
		if(wave_encode_s->wave_fp == NULL) {
            WAVE_INFO("open wave record file %s fail!\r\n", filename);
			goto wave_encode_init_err;	
        }
	}
    else {
        WAVE_INFO("wave record filename is null!\r\n");
        goto wave_encode_init_err;	
    }
    os_memcpy(&(wave_encode_s->wave_head), wav_header, sizeof(TYPE_WAVE_HEAD));
    osal_fseek(wave_encode_s->wave_fp, sizeof(TYPE_WAVE_HEAD));
    if(auenc_init->src_msi && (msi_add_output(auenc_init->src_msi, NULL, msi->name) != RET_OK)) {
        goto wave_encode_init_err;
    }
	wave_encode_s->msi = msi;
    wave_encode_s->src_msi = auenc_init->src_msi;
    wave_encode_s->samplerate = samplerate;
    wave_encode_s->destroy_self = auenc_init->destroy_self;
	wave_encode_s->next_status = AUCODEC_RUN;
    wave_encode_s->current_status = AUCODEC_RUN;
    wave_encode_s->task_hdl = os_task_create("wave_encode_thread", wave_encode_thread, (void*)wave_encode_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
	if(wave_encode_s->task_hdl == NULL)  {
		WAVE_INFO("create wave encode task fail!\r\n");
		goto wave_encode_init_err;
	}
	return msi;

wave_encode_init_err:
	msi_destroy(msi);
#endif
	return NULL;
}