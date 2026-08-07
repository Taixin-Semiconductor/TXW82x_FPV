#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "osal_file.h"
#include "audio_code_ctrl.h"
#include "wave_code.h"

#define MAX_WAVE_ENCODE_RXBUF    4

struct wave_encode_struct {
    struct os_event event;
    struct msi *msi;
    void *task_hdl;   
    void *wave_fp; 
    uint32_t samplerate;
    uint32_t data_size; 
    TYPE_WAVE_HEAD wave_head; 
};
static struct wave_encode_struct *wave_encode_s = NULL;

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

static uint8_t next_status = AUDIO_STOP;
static uint8_t current_status = AUDIO_STOP;

static void wave_encode_destroy(void);

uint8_t get_wave_encode_status(void)
{
    return current_status;
}

static void set_wave_encode_status(uint8_t status)
{
    next_status = status; 
}

static void wave_encode_thread(void *d)
{
    uint8_t *data = NULL;
	uint32_t data_len = 0;
    struct wave_encode_struct *s = (struct wave_encode_struct *)d;
    struct framebuff *frame_buf = NULL;

    msi_get(s->msi);

    while(1)
	{
		frame_buf = msi_get_fb(s->msi, 0);
        if(frame_buf) 
        {
            if(next_status == AUDIO_PAUSE) 
                current_status = AUDIO_PAUSE;
            else {
                current_status = AUDIO_RUN;
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

        if(next_status == AUDIO_STOP) {
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
    os_event_set(&s->event, exit_event, NULL);

    msi_put(s->msi);

    if(next_status != AUDIO_STOP)
        wave_encode_destroy();    
}

static int32_t wave_encode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    switch(cmd_id) {
        case MSI_CMD_TRANS_FB:
        {
            ret = RET_OK+1;
            struct framebuff *frame_buf = (struct framebuff *)param1;
            if(frame_buf->mtype == F_AUDIO) {
                ret = RET_OK;
            } 
            break;
        }    
        case MSI_CMD_PRE_DESTROY:
        {
            if(wave_encode_s) {
                if(wave_encode_s->task_hdl) {
                    set_wave_encode_status(AUDIO_STOP);
                }
            }
            ret = RET_OK;
            break;
        }     
        case MSI_CMD_POST_DESTROY:
        {
            if(wave_encode_s) {
                if(wave_encode_s->task_hdl) {
                    os_event_wait(&wave_encode_s->event, exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
                if(wave_encode_s->event.hdl)
                    os_event_del(&wave_encode_s->event);
                if(wave_encode_s->wave_fp) {
                    osal_fclose(wave_encode_s->wave_fp);
                    wave_encode_s->wave_fp = NULL;
                }
                WAVE_CODE_FREE(wave_encode_s);
                wave_encode_s = NULL;
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

static void wave_encode_destroy(void)
{
	msi_destroy(wave_encode_s->msi);
}

void wave_encode_pause(void)
{
    set_wave_encode_status(AUDIO_PAUSE);
}

void wave_encode_continue(void)
{
    if(get_wave_encode_status() == AUDIO_PAUSE)
        set_wave_encode_status(AUDIO_RUN);
}

int32_t wave_encode_deinit(void)
{
    if(!wave_encode_s) {
        WAVE_INFO("wave_encode_deinit fail,wave_encode_s is null!\r\n");
        return RET_ERR;
    }
    wave_encode_destroy();
    return RET_OK;
}

struct msi *wave_encode_init(uint8_t *filename, uint32_t samplerate)
{  
#if AUDIO_EN
    uint8_t msi_isnew = 0;
    uint32_t count = 0; 
    void *wave_fp = NULL;
	
    while((get_wave_encode_status() != AUDIO_STOP) && count < 2000) {
        os_sleep_ms(1);
        count++;
    }
    if(count >= 2000) {
        WAVE_INFO("wave encode init timeout!\r\n");
        return NULL;
    }
	struct msi *msi = msi_new("R_WAVE_ENCODE", MAX_WAVE_ENCODE_RXBUF, &msi_isnew);
	if(!msi) {
		WAVE_INFO("create wave encode msi fail!\r\n");
		return NULL;
	}
    else if(!msi_isnew) {
		WAVE_INFO("wave encode msi has been create!\r\n");
        msi_destroy(msi);
		return NULL;        
    }
	msi->enable = 1;
	msi->action = (msi_action)wave_encode_msi_action;   
	wave_encode_s = (struct wave_encode_struct *)WAVE_CODE_ZALLOC(sizeof(struct wave_encode_struct));
	if(!wave_encode_s) {
		WAVE_INFO("wave_encode_s malloc fail!\r\n");
		goto wave_encode_init_err;
	}
	if(filename) {
		wave_fp = osal_fopen((const char*)filename, "wb+");
		if(!wave_fp) {
            WAVE_INFO("open wave record file %s fail!\r\n", filename);
			goto wave_encode_init_err;	
        }
        wave_encode_s->wave_fp = wave_fp;
	}
    else {
        WAVE_INFO("wave record filename is null!\r\n");
        goto wave_encode_init_err;	
    }
	wave_encode_s->msi = msi;
    wave_encode_s->samplerate = samplerate;
    os_memcpy(&(wave_encode_s->wave_head), wav_header, sizeof(TYPE_WAVE_HEAD));
    osal_fseek(wave_fp, sizeof(TYPE_WAVE_HEAD));
    if(os_event_init(&wave_encode_s->event) != RET_OK) {
        AAC_INFO("create wave encode event fail!\r\n");
        goto wave_encode_init_err;
    }
	next_status = AUDIO_RUN;
    current_status = AUDIO_RUN;
    wave_encode_s->task_hdl = os_task_create("wave_encode_thread", wave_encode_thread, (void*)wave_encode_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
	if(wave_encode_s->task_hdl == NULL)  {
		WAVE_INFO("create wave encode task fail!\r\n");
		goto wave_encode_init_err;
	}
	return msi;

wave_encode_init_err:
	if(wave_fp) {
		osal_fclose(wave_fp);
		wave_fp= NULL;
        wave_encode_s->wave_fp = NULL;
	}
	msi_destroy(msi);
#endif
	return NULL;
}