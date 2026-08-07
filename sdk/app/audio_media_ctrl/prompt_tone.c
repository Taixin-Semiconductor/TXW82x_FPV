#include "prompt_tone.h"
#include "audio_code_ctrl.h"

#define MAX_PROMPTTONE_TXBUF  4

static int32_t prompt_tone_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
	PROMPT_TONE_STRUCT *tone_s = (PROMPT_TONE_STRUCT*)msi->priv;
    switch(cmd_id) {
        case MSI_CMD_FREE_FB:
        {
            if(tone_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                fbpool_put(&tone_s->tx_pool, frame_buf);
            }
            ret = RET_OK+1;
            break; 
        }        
		case MSI_CMD_POST_DESTROY:
        {
            if(tone_s) {
                for(uint32_t i=0; i<MAX_PROMPTTONE_TXBUF; i++) {
                    struct framebuff *frame_buf = (tone_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        PROMPTTONE_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&tone_s->tx_pool);
				PROMPTTONE_FREE(tone_s);
            }
            ret = RET_OK;
            break; 
        }         
        default:
            break;    
    }
    return ret;
}

void prompt_tone_task(void *d)
{
    struct framebuff *frame_buf = NULL;
    PROMPT_TONE_STRUCT *tone_s = (PROMPT_TONE_STRUCT*)d;

	os_sleep_ms(5000);
    while(1) {  
        if(get_audio_code_status(MP3_DEC) == AUDIO_STOP)
            break;
        frame_buf = fbpool_get(&tone_s->tx_pool, 0, NULL);
        if(frame_buf) {
            if(tone_s->tone_buf_offset >= tone_s->tone_buf_size) {
                frame_buf->len = 0;
            }
            else if((tone_s->tone_buf_offset+1024) < tone_s->tone_buf_size) {
                os_memcpy(frame_buf->data, tone_s->tone_buf+tone_s->tone_buf_offset, 1024);
                frame_buf->len = 1024;
                tone_s->tone_buf_offset += 1024;
            }
            else {
                os_memcpy(frame_buf->data, tone_s->tone_buf+tone_s->tone_buf_offset, 
                                            tone_s->tone_buf_size-tone_s->tone_buf_offset);
                frame_buf->len = tone_s->tone_buf_size-tone_s->tone_buf_offset;
                tone_s->tone_buf_offset = tone_s->tone_buf_size;
            }
            frame_buf->msi = tone_s->msi;
            frame_buf->mtype = F_AUDIO;	
            msi_get(frame_buf->msi);
            msi_output_fb(tone_s->msi, frame_buf);
        }
        else
            os_sleep_ms(1);
    }
    msi_destroy(tone_s->msi);
}

struct msi *play_prompt_tone(const uint8_t *tone_buf, uint32_t size)
{
    uint8_t msi_isnew = 0;
    struct framebuff *frame_buf = NULL;
	struct msi *tone_msi = NULL;
    struct msi *mp3_msi = NULL;

    tone_msi = msi_new("S_PROMPTTONE", MAX_PROMPTTONE_TXBUF, &msi_isnew);
	if(tone_msi == NULL) {
		os_printf("create prompt_tone msi fail\n");
		return NULL;	
	}
	if(tone_msi && !msi_isnew) {
		os_printf("prompt_tone msi has been created\n");
		return NULL;
	}
	PROMPT_TONE_STRUCT *tone_s = (PROMPT_TONE_STRUCT*)PROMPTTONE_ZALLOC(sizeof(PROMPT_TONE_STRUCT));
	if(tone_s == NULL) {
		os_printf("malloc tone_s fail!\n");
		goto play_prompt_tone_err;
	}
	tone_s->msi = tone_msi;
    tone_s->msi->enable = 1;
    tone_s->msi->action = prompt_tone_msi_action;
    tone_s->tone_buf = tone_buf;
    tone_s->tone_buf_size = size;
    
    fbpool_init(&tone_s->tx_pool, MAX_PROMPTTONE_TXBUF);
    for(uint32_t i=0; i<MAX_PROMPTTONE_TXBUF; i++) {
		frame_buf = (tone_s->tx_pool.pool)+i;
		frame_buf->data = (uint8_t*)PROMPTTONE_MALLOC(sizeof(uint8_t) * 1024);
		if(!frame_buf->data)
			goto play_prompt_tone_err;
    }
    tone_s->msi->priv = tone_s;
    mp3_msi = audio_decode_init(MP3_DEC, 0, 1);
	msi_add_output(tone_msi, NULL, audio_code_msi_name(MP3_DEC));
    tone_s->task_hdl = os_task_create("prompt_tone_task", prompt_tone_task, (void*)tone_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
	if(tone_s->task_hdl == NULL)  {
		os_printf("create tone_s task fail!\r\n");
		goto play_prompt_tone_err;
	}
    return tone_msi;
play_prompt_tone_err:
	if(tone_msi)
		msi_destroy(tone_msi);
	return NULL;
}

void play_prompt_tone_test(void)
{
    play_prompt_tone(connect_mp3, 2969);
}
