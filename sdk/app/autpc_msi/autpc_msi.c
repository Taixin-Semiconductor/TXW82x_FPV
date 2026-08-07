#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/audio/wsola/wsola_process.h"
#include "autpc_msi.h"

#define MAX_AUTPC_RXBUF      1
#define MAX_AUTPC_TXBUF      4

typedef struct {
    struct msi *msi;
    struct fbpool tx_pool;
    uint32_t have_next_time;
    uint32_t next_time;
    float speed;
    float pitch;
    WsolaStream *wsola_stream;
} autpc_struct;

static int32_t autpc_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
	autpc_struct *autpc_s = (autpc_struct*)msi->priv;
    switch(cmd_id) {
        case MSI_CMD_TRANS_FB:
        {
            struct framebuff *recv_frame_buf = (struct framebuff *)param1;
            struct framebuff *send_frame_buf = NULL;
            if(recv_frame_buf->mtype == F_AUDIO) {
                uint32_t data_nsamples = (recv_frame_buf->len) / 2;
                uint32_t nsamples = 0;
                uint32_t offset = 0;
                float current_pitch = 1.0f;
                float current_speed = 1.0f;
                current_speed = wsolaStream_get_speed(autpc_s->wsola_stream);
                current_pitch = wsolaStream_get_pitch(autpc_s->wsola_stream);
                if(current_pitch != autpc_s->pitch) {
                    msi_do_cmd(msi, MSI_CMD_AUTPC, MSI_AUTPC_END_STREAM, 0);
                    wsolaStream_set_pitch(autpc_s->wsola_stream, autpc_s->pitch);
                }
                if(current_speed != autpc_s->speed) {
                    wsolaStream_set_speed(autpc_s->wsola_stream, autpc_s->speed);  
                }
                while(data_nsamples > 0) {
                    nsamples = wsolaStream_input_data(autpc_s->wsola_stream, (int16_t*)(recv_frame_buf->data+offset), data_nsamples);
                    data_nsamples -= nsamples;
                    offset += (nsamples * 2);
                    nsamples = wsolaStream_output_available(autpc_s->wsola_stream);
                    if(nsamples > 0) {
                        while(!send_frame_buf) {
                            send_frame_buf = fbpool_get(&autpc_s->tx_pool, 0, autpc_s->msi);
                            if(!send_frame_buf)
                                os_sleep_ms(1); 
                        }
                        send_frame_buf->data = (uint8_t*)AUTPC_MALLOC(sizeof(int16_t) * nsamples);
                        if(send_frame_buf->data == NULL) {
                            os_printf("autpc msi malloc send_frame_buf->data fail\n");
                            msi_delete_fb(autpc_s->msi, send_frame_buf);
                            break;
                        }
                        nsamples = wsolaStream_output_data(autpc_s->wsola_stream, (int16_t*)(send_frame_buf->data), nsamples);
                        send_frame_buf->len = nsamples * 2;
                        send_frame_buf->mtype = recv_frame_buf->mtype;
                        send_frame_buf->stype = recv_frame_buf->stype;
                        if(autpc_s->have_next_time == 0) {
                            send_frame_buf->time = recv_frame_buf->time;
                            autpc_s->have_next_time = 1;
                            autpc_s->next_time = send_frame_buf->time;
                        }
                        else {
                            send_frame_buf->time = autpc_s->next_time;
                        }
                        msi_output_fb(autpc_s->msi, send_frame_buf); 
                        send_frame_buf = NULL;
                        autpc_s->next_time += nsamples * 1000 / wsolaStream_get_sampleRate(autpc_s->wsola_stream);
                    }     
                }   
            } 
            ret = RET_OK+1;
            break;
        }            
        case MSI_CMD_FREE_FB:
        {
            if(autpc_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                if(frame_buf->data) {
                    AUTPC_FREE(frame_buf->data);
                    frame_buf->data = NULL;
                }
                fbpool_put(&autpc_s->tx_pool, frame_buf);
            }
            ret = RET_OK+1;
            break; 
        }        
		case MSI_CMD_POST_DESTROY:
        {
            if(autpc_s) {
                for(uint32_t i=0; i<MAX_AUTPC_TXBUF; i++) {
                    struct framebuff *frame_buf = (autpc_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        AUTPC_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&autpc_s->tx_pool);
                if(autpc_s->wsola_stream) {
                    wsolaStream_deinit(autpc_s->wsola_stream);
                    autpc_s->wsola_stream = NULL;
                }
				AUTPC_FREE(autpc_s);
            }
            ret = RET_OK;
            break; 
        }  
        case MSI_CMD_AUTPC:
        {
            if(!autpc_s) {
                break;
            }
			uint32_t cmd_self = (uint32_t)param1;
			switch(cmd_self) {
                case MSI_AUTPC_SET_SPEED:
                {
                    autpc_s->speed = ((float)param2)/100.0f;
                    break;
                }
                case MSI_AUTPC_GET_SPEED:
                {
					*((uint32_t*)param2) = (uint32_t)(autpc_s->speed*100);
                    break;
                }
                case MSI_AUTPC_SET_PITCH:
                {
					autpc_s->pitch = ((float)param2)/100.0f;  
					break;
                }
                case MSI_AUTPC_GET_PITCH:
                {
					*((uint32_t*)param2) = (uint32_t)(autpc_s->pitch*100);
					break;
                }
                case MSI_AUTPC_END_STREAM:
                {
                    struct framebuff *send_frame_buf = NULL;
                    uint32_t nsamples = 0;
                    wsolaStream_flush(autpc_s->wsola_stream);
                    nsamples = wsolaStream_output_available(autpc_s->wsola_stream);
                    if(nsamples > 0) {
                        while(!send_frame_buf) {
                            send_frame_buf = fbpool_get(&autpc_s->tx_pool, 0, autpc_s->msi);
                            if(!send_frame_buf)
                                os_sleep_ms(1); 
                        }
                        send_frame_buf->data = (uint8_t*)AUTPC_MALLOC(sizeof(int16_t) * nsamples);
                        if(send_frame_buf->data == NULL) {
                            os_printf("autpc msi malloc send_frame_buf->data fail\n");
                            msi_delete_fb(autpc_s->msi, send_frame_buf);
                            break;
                        }
                        nsamples = wsolaStream_output_data(autpc_s->wsola_stream, (int16_t*)(send_frame_buf->data), nsamples);
                        msi_output_fb(autpc_s->msi, send_frame_buf); 
                        autpc_s->next_time += nsamples * 1000 / wsolaStream_get_sampleRate(autpc_s->wsola_stream);   
                    }   
                    autpc_s->have_next_time = 0;  
                    wsola_stream_clean(autpc_s->wsola_stream);
                    break;            
                }
                default:
                    break; 
            }
            ret = RET_OK;
            break;
        }       
        default:
            break;    
    }
    return ret;
}

struct msi *autpc_msi_init(uint32_t samplerate, uint32_t speed, uint32_t pitch, uint32_t max_inputSamples)
{
    struct framebuff *frame_buf = NULL;
	struct msi *msi = NULL;
    char msi_name[16] = {0};

    os_snprintf(msi_name, 16, "SR_AUTPC_""%04d", (int)(os_jiffies()));
	msi = msi_new(msi_name, MAX_AUTPC_RXBUF, NULL);
	if(msi == NULL) {
		os_printf("create autpc msi fail\n");
		return NULL;	
	}   
    autpc_struct *autpc_s = (autpc_struct*)AUTPC_ZALLOC(sizeof(autpc_struct));
    if(autpc_s == NULL) {
		msi_destroy(msi);
		os_printf("alloc autpc_s fail!\n");
		return NULL;        
    }
    fbpool_init(&autpc_s->tx_pool, MAX_AUTPC_TXBUF); 
    for(uint32_t i=0; i<MAX_AUTPC_TXBUF; i++) {
		frame_buf = (autpc_s->tx_pool.pool)+i;
        frame_buf->data = NULL;
    } 
    msi->priv = autpc_s;
	autpc_s->msi = msi;
    autpc_s->msi->enable = 1;
    autpc_s->msi->action = autpc_msi_action;
    autpc_s->wsola_stream = wsolaStream_init(samplerate, 1, (float)speed/100.0f, (float)pitch/100.0f, max_inputSamples*2, max_inputSamples*4);
    if(autpc_s->wsola_stream == NULL) {
        msi_destroy(msi);
        os_printf("wsolaStream init fail!\n");
        return NULL;
    }
    autpc_s->speed = ((float)speed)/100.0f;
    autpc_s->pitch = ((float)pitch)/100.0f;
    return msi;
}