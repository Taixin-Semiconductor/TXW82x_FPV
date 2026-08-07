#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "autpc_msi.h"
#include "magic_voice.h"

#define MAX_MAGIC_VOICE_RXBUF    1
#define MAX_MAGIC_VOICE_TXBUF    1

#define SETW32TOW16(a)           (a>32767)?32767:(a<-32768)?-32768:a   

typedef struct {
    struct msi *msi;
    struct msi *autpc_msi;
    struct fbpool tx_pool;
    int16_t *buf;
    uint8_t new_type;
    uint8_t current_type;
    uint32_t table_index;
}magic_voice_struct;

static const uint8_t delaySamples_table[500] = {
	0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,
	0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,
	0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,
	0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,
	0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
	0x18,0x18,0x18,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x1a,0x1a,0x1a,0x1a,
	0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1c,0x1c,
	0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1e,0x1e,
	0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x22,0x22,0x22,0x22,0x22,0x22,
	0x22,0x22,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x25,0x25,
	0x25,0x25,0x25,0x25,0x25,0x25,0x25,0x26,0x26,0x26,0x26,0x26,0x26,0x26,0x26,0x27,0x27,0x27,0x27,0x27,
	0x27,0x27,0x27,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x2a,0x2a,
	0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2b,0x2b,0x2b,0x2b,0x2b,0x2b,0x2b,0x2b,0x2c,0x2c,0x2c,0x2c,0x2c,0x2c,
	0x2c,0x2c,0x2c,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2e,0x2e,0x2e,0x2e,0x2e,0x2e,0x2e,0x2e,0x2f,
	0x2f,0x2f,0x2f,0x2f,0x2f,0x2f,0x2f,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x31,0x31,0x31,0x31,
	0x31,0x31,0x31,0x31,0x31,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x33,0x33,0x33,0x33,0x33,0x33,
	0x33,0x33,0x33,0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
	0x35,0x35,0x35,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x37,0x37,0x37,0x37,0x37,0x37,
	0x37,0x37,0x37,0x37,0x37,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x39,0x39,
	0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,
	0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,
	0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,0x3b,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,
	0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,
	0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c
};

static int32_t magic_voice_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
	magic_voice_struct *magic_voice_s = (magic_voice_struct*)msi->priv;
    switch(cmd_id) {
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *recv_frame_buf = (struct framebuff *)param1;
            struct framebuff *send_frame_buf = NULL;
            if(recv_frame_buf->mtype == F_AUDIO) {
                if(magic_voice_s->current_type != magic_voice_s->new_type) {
                    uint32_t pitch = 100;
                    switch(magic_voice_s->new_type) {
                        case original_voice: pitch = 100; break; 
                        case alien_voice: pitch = 130; break; 
                        case robot_voice: pitch = 90; break;
                        case hight_voice: pitch = 150; break;
                        case deep_voice: pitch = 70; break; 
                        case etourdi_voice: pitch = 120; break; 
                        default: break;
                    }
                    msi_output_cmd(msi, MSI_CMD_AUTPC, MSI_AUTPC_SET_PITCH, pitch);
                    magic_voice_s->current_type = magic_voice_s->new_type;
                }
            }
            if(recv_frame_buf->mtype == F_AUDIO) {
                uint8_t delaySamples = 0;
                int16_t *send_buf = NULL;
                int32_t temp32 = 0;
                uint32_t nsamples = recv_frame_buf->len / 2;
                while(!send_frame_buf) {
                    send_frame_buf = fbpool_get(&magic_voice_s->tx_pool, 0, magic_voice_s->msi);
                    if(!send_frame_buf)
                        os_sleep_ms(1);
                }
                send_frame_buf->data = (uint8_t*)MAGIC_VOICE_MALLOC(sizeof(int16_t) * nsamples);
                if(send_frame_buf->data == NULL) {
                    os_printf("magic voice msi malloc send_frame_buf->data fail\n");
                    msi_delete_fb(magic_voice_s->msi, send_frame_buf);
                    break;
                }
                if(nsamples != 160) {
                    os_memcpy(send_frame_buf->data, recv_frame_buf->data, recv_frame_buf->len);
                    os_printf("magic voice nsamples abormal\n");
                    goto magic_voice_output;
                }
                send_buf = (int16_t*)(send_frame_buf->data);
                os_memcpy(magic_voice_s->buf+nsamples, recv_frame_buf->data, recv_frame_buf->len);
                if(magic_voice_s->current_type == alien_voice) {
                    for(uint32_t i=0; i<nsamples; i++) {
                        if(magic_voice_s->table_index < 500)
                            delaySamples = delaySamples_table[magic_voice_s->table_index];
                        else
                            delaySamples = delaySamples_table[999-magic_voice_s->table_index];
                        temp32 = (magic_voice_s->buf[(i + nsamples - delaySamples)]+magic_voice_s->buf[(i + nsamples)])>>1;
                        send_buf[i] = SETW32TOW16(temp32);
                        magic_voice_s->table_index = (magic_voice_s->table_index+1)%1000;
                    }
                    os_memcpy(magic_voice_s->buf,magic_voice_s->buf+nsamples,recv_frame_buf->len);   
                }
                else if(magic_voice_s->current_type == robot_voice) {
                    int16_t delay_data = 0;
                    int16_t back_data = 0;
                    delaySamples=80;
                    for(uint32_t i=0; i<nsamples; i++) {
                        delay_data = magic_voice_s->buf[(i + nsamples - delaySamples)];
                        back_data = delay_data;
                        temp32 = ((magic_voice_s->buf[nsamples+i]>>1)+delay_data)*8/10;
                        send_buf[i] = SETW32TOW16(temp32);
                        temp32 = ((magic_voice_s->buf[nsamples+i]>>1)+(back_data<<2)/5);
                        magic_voice_s->buf[nsamples+i] = SETW32TOW16(temp32);
                    }
                    os_memcpy(magic_voice_s->buf,magic_voice_s->buf+nsamples,recv_frame_buf->len);        
                }
                else {
                    os_memcpy(send_frame_buf->data, recv_frame_buf->data, recv_frame_buf->len);
                }
magic_voice_output:
                send_frame_buf->mtype = recv_frame_buf->mtype;
                send_frame_buf->stype = recv_frame_buf->stype; 
                send_frame_buf->time = recv_frame_buf->time;    
                send_frame_buf->len = recv_frame_buf->len;  
                msi_output_fb(magic_voice_s->msi, send_frame_buf);     
            }
            ret = RET_OK+1;
            break;
        }            
        case MSI_CMD_FREE_FB:
        {
            if(magic_voice_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                if(frame_buf->data) {
                    MAGIC_VOICE_FREE(frame_buf->data);
                    frame_buf->data = NULL;
                }
                fbpool_put(&magic_voice_s->tx_pool, frame_buf);
            }
            ret = RET_OK+1;
            break; 
        }        
		case MSI_CMD_POST_DESTROY:
        {
            if(magic_voice_s) {
                for(uint32_t i=0; i<MAX_MAGIC_VOICE_TXBUF; i++) {
                    struct framebuff *frame_buf = (magic_voice_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        MAGIC_VOICE_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&magic_voice_s->tx_pool);
                if(magic_voice_s->autpc_msi) {
                    msi_destroy(magic_voice_s->autpc_msi);
                    magic_voice_s->autpc_msi = NULL;
                }
                if(magic_voice_s->buf) {
                    MAGIC_VOICE_FREE(magic_voice_s->buf);
                }
				MAGIC_VOICE_FREE(magic_voice_s);
            }
            ret = RET_OK;
            break; 
        }  
        default:
            break;    
    }
    return ret;
}

int32_t magic_voice_set_type(uint8_t type)
{
    int ret = RET_ERR;
    struct msi *msi = msi_find("SR_MAGIC_VOICE", 1);
    if(msi) {
        msi_put(msi);
        magic_voice_struct *magic_voice_s = (magic_voice_struct*)msi->priv;
        magic_voice_s->new_type = type;
        ret = RET_OK;
    }
    return ret;    
}

int32_t magic_voice_msi_add_output(const char *msi_name)
{
    int ret = RET_ERR;
    struct msi *msi = msi_find("SR_MAGIC_VOICE", 1);
    if(msi) {
        msi_put(msi);
        magic_voice_struct *magic_voice_s = (magic_voice_struct*)msi->priv;
        if(magic_voice_s->autpc_msi) {
            ret = msi_add_output(magic_voice_s->autpc_msi, NULL, msi_name);
        }
    }
    return ret;
}

int32_t magic_voice_msi_del_output(const char *msi_name)
{
    int ret = RET_ERR;
    struct msi *msi = msi_find("SR_MAGIC_VOICE", 1);
    if(msi) {
        msi_put(msi);
        magic_voice_struct *magic_voice_s = (magic_voice_struct*)msi->priv;
        if(magic_voice_s->autpc_msi) {
            ret = msi_del_output(magic_voice_s->autpc_msi, NULL, msi_name);
        }
    }
    return ret;
}

int32_t magic_voice_deinit(void)
{
    int ret = RET_ERR;
    struct msi *msi = msi_find("SR_MAGIC_VOICE", 1);
    if(msi) {
        msi_put(msi);
        msi_destroy(msi);
        ret = RET_OK;
    }
    return ret;    
}

struct msi *magic_voice_init(uint32_t samplerate, uint32_t size)
{
    uint8_t msi_isnew = 0;
    struct framebuff *frame_buf = NULL;
	struct msi *msi = NULL;

    if((samplerate != 8000) || (size != 160)) {
        os_printf("magic voice param abormal\n");
        return NULL;
    }

    msi = msi_new("SR_MAGIC_VOICE", MAX_MAGIC_VOICE_RXBUF, &msi_isnew);
	if(msi == NULL) {
		os_printf("create magic voice msi fail\n");
		return NULL;	
	}   
    if(msi_isnew == 0) {
        return NULL;
    }
    magic_voice_struct *magic_voice_s = (magic_voice_struct*)MAGIC_VOICE_ZALLOC(sizeof(magic_voice_struct));
    if(magic_voice_s == NULL) {
        msi_destroy(msi);
        os_printf("alloc magic voice struct fail\n");
        return NULL;
    }
    fbpool_init(&magic_voice_s->tx_pool, MAX_MAGIC_VOICE_TXBUF); 
    for(uint32_t i=0; i<MAX_MAGIC_VOICE_TXBUF; i++) {
		frame_buf = (magic_voice_s->tx_pool.pool)+i;
        frame_buf->data = NULL;
    } 
    msi->priv = magic_voice_s;
	magic_voice_s->msi = msi;
    magic_voice_s->msi->enable = 1;
    magic_voice_s->msi->action = magic_voice_msi_action;
    magic_voice_s->autpc_msi = autpc_msi_init(samplerate, 100, 100, size);
    if(magic_voice_s->autpc_msi == NULL) {
        msi_destroy(msi);
        os_printf("magic voice create autpc msi fail\n");
        return NULL;        
    }

    magic_voice_s->buf = (int16_t*)MAGIC_VOICE_MALLOC(size*2+320);
    if(magic_voice_s->buf == NULL) {
        msi_destroy(msi);
        os_printf("magic voice alloc buf fail\n");
        return NULL;        
    }
    msi_add_output(msi, NULL, magic_voice_s->autpc_msi->name);
    magic_voice_s->new_type = original_voice;
    magic_voice_s->current_type = original_voice;
	return msi;
}