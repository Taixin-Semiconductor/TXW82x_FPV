#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "osal_file.h"
#include "lib/audio/audio_code/audio_code.h"
#include "audio_code_ctrl.h"
#include "mp3_decode.h"
#include "mp3_getInfo.h"

#define BUFF_SIZE   2048
#define MAX_MP3_DECODE_RXBUF    4
#define MAX_MP3_DECODE_TXBUF    4

struct mp3_decode_struct {
    struct fbpool tx_pool;
    struct os_event event;
    struct msi *msi;
    void *task_hdl;   
    void *mp3_fp;
    uint8_t *mp3_filename;
    uint8_t direct_to_dac;
    uint8_t get_first_frame;
    uint8_t inbuf[BUFF_SIZE];
    int16_t dec_buf[1152*2];
    uint32_t file_size;
    uint32_t buf_offset;
    uint32_t buf_size;
    uint32_t cur_sampleRate;
};
static struct mp3_decode_struct *mp3_decode_s = NULL;

static uint8_t next_status = AUDIO_STOP;
static uint8_t current_status = AUDIO_STOP;
static void mp3_decode_destroy(void);

uint8_t get_mp3_decode_status(void)
{
    return current_status;
}

static void set_mp3_decode_status(uint8_t status)
{
    next_status = status; 
}

static int32_t mp3_file_read(uint8_t *buf, uint32_t size)
{
    int32_t read_len = 0;
	
    read_len = osal_fread(buf, size, 1, mp3_decode_s->mp3_fp);
    return read_len;
}

static void mp3_file_decode(struct mp3_decode_struct *s)
{
    uint8_t endOfRead = 0;
    uint8_t *enc_ptr = NULL;
    int16_t *data = NULL;
    int32_t ret = 0;
    int32_t dec_samples = 0;
    int32_t read_len = 0;
    int32_t unproc_data_size = 0;
    int32_t ID3V2_offset = 0;
    uint32_t ID3V2_len = 0;  
    uint32_t enc_ptr_offset = 0;
    uint32_t bytes_to_read = 0;
    struct framebuff *frame_buf = NULL;
    AUCODE_HDL *mp3_dec = NULL;
    AUCODE_FRAME_INFO mp3_info;

    while(!s->get_first_frame) {
        bytes_to_read = BUFF_SIZE - unproc_data_size;
        read_len = mp3_file_read(s->inbuf+unproc_data_size,bytes_to_read);
        if(read_len <= 0) {
            MP3_INFO("mp3 read fail,ret:%d,line:%d!\r\n",read_len,__LINE__);
            goto mp3_decode_end;
        }
        s->buf_size = read_len + unproc_data_size;
        if(os_strncmp(s->inbuf, "ID3", 3) == 0) {
            if (s->buf_size < 10) {
                unproc_data_size = s->buf_size;
                os_sleep_ms(1);
                continue;
            }
            ID3V2_len = (s->inbuf[6]&0x7F)*0x200000+ (s->inbuf[7]&0x7F)*0x4000 + 
                                            (s->inbuf[8]&0x7F)*0x80 +(s->inbuf[9]&0x7F);
            ID3V2_offset = (ID3V2_len+10);
            while(ID3V2_offset >= s->buf_size) {
                os_sleep_ms(1);
                ID3V2_offset -= s->buf_size;
                read_len = mp3_file_read(s->inbuf,BUFF_SIZE);
                if(read_len <= 0) {
                    MP3_INFO("mp3 read fail,ret:%d,line:%d!\r\n",read_len,__LINE__);
                    goto mp3_decode_end;
                }
                s->buf_size = read_len;
            } 
            if(ID3V2_offset) {
                s->buf_size -= ID3V2_offset;
                os_memcpy(s->inbuf, s->inbuf+ID3V2_offset, s->buf_size);
            }
            if(s->buf_size <= 1) {
                os_sleep_ms(1);
                unproc_data_size = s->buf_size;
                continue;
            }
        } 

        for(uint32_t i=0; i<(s->buf_size-1); i++) {
            if( ( (s->inbuf[i] << 8) | (s->inbuf[i+1] & 0xE0) ) == 0xFFE0 ) {
                s->get_first_frame = 1;
                unproc_data_size = s->buf_size - i;
                break;
            }
        }
        if(!s->get_first_frame) {
            unproc_data_size = 1;
            s->inbuf[0] = s->inbuf[s->buf_size-1];
        }
        os_sleep_ms(1);
    }

    mp3_dec = audio_coder_open(MP3_DEC, 0, 0);
    if(mp3_dec == NULL)
        return;
    enc_ptr = s->inbuf;
    while(1) {
        if(next_status == AUDIO_PAUSE) {
            current_status = AUDIO_PAUSE;
            while(next_status == AUDIO_PAUSE) {
                os_sleep_ms(1);
            }
        }

        if(next_status == AUDIO_STOP) {
            goto mp3_decode_end;
        }
        current_status = AUDIO_RUN;

        if(unproc_data_size < BUFF_SIZE && !endOfRead) {
            bytes_to_read = BUFF_SIZE - unproc_data_size;
			if(unproc_data_size)
				os_memcpy(s->inbuf, enc_ptr+enc_ptr_offset, unproc_data_size);	
            read_len = mp3_file_read(s->inbuf+unproc_data_size, bytes_to_read);
            if(read_len <= 0) {
                MP3_INFO("mp3 read fail,ret:%d,line:%d!\r\n",read_len,__LINE__);
                endOfRead = 1;
            }   
            else       
                unproc_data_size += read_len;
            enc_ptr = s->inbuf;
            enc_ptr_offset = 0;
        }
        
        if(unproc_data_size > 0) {
            while(!frame_buf) {
                frame_buf = fbpool_get(&s->tx_pool, 0, s->msi);
                if(!frame_buf)
                    os_sleep_ms(1);
            }
            data = (int16_t*)frame_buf->data;
            dec_samples = audio_decode_data(mp3_dec, enc_ptr+enc_ptr_offset, unproc_data_size, s->dec_buf, &mp3_info);
			if(dec_samples <= 0) {
				enc_ptr_offset += 1;
				unproc_data_size -= 1;
                msi_delete_fb(s->msi, frame_buf);
                frame_buf = NULL;
				continue;
			}
            if(mp3_info.channels == 2) {
                for(uint32_t i=0; i<dec_samples; i++) 
                    data[i] = s->dec_buf[2*i];
            }
			else {
				for(uint32_t i=0; i<dec_samples; i++)
					data[i] = s->dec_buf[i];
			}
            enc_ptr_offset += mp3_info.frame_bytes;
            unproc_data_size -= mp3_info.frame_bytes;
            frame_buf->len = dec_samples*2;
            frame_buf->mtype = F_AUDIO;
            frame_buf->stype = FSTYPE_AUDIO_MP3_DECODER;
            if((s->direct_to_dac) && (s->cur_sampleRate != mp3_info.samplerate)) {
                s->cur_sampleRate = mp3_info.samplerate;
                msi_output_cmd(s->msi, MSI_CMD_AUDAC, MSI_AUDAC_SET_SAMPLING_RATE, s->cur_sampleRate);
            }
            ret = msi_output_fb(s->msi, frame_buf);  
            MP3_DEBUG("mp3 decode send framebuff:%p,ret:%d\r\n",frame_buf,ret);
            frame_buf = NULL;                 
        }        
        else if(endOfRead){
            goto mp3_decode_end;
        }
    }
mp3_decode_end:
    if(s->direct_to_dac)
	    msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,0);
    if(mp3_dec) {
        audio_coder_close(mp3_dec);
	}
}

static void update_frame_buf(struct msi *msi, struct framebuff * frame_buf)
{
    msi_delete_fb(NULL, frame_buf);
    frame_buf = NULL;
    while(!frame_buf) {
        frame_buf = msi_get_fb(msi, 0);
        os_sleep_ms(1);
    }    
}

static void mp3_msi_decode(struct mp3_decode_struct *s)
{
    uint8_t clear_finish = 1;
    uint8_t find_ID3 = 0;
    uint8_t endOfRead = 0;
    uint8_t *recv_data = NULL;
    int16_t *send_data = NULL;
    uint8_t *enc_ptr = NULL;
	int32_t ret = 0;
    int32_t data_len = 0;
    int32_t dec_samples = 0; 
    int32_t unproc_data_size = 0;  
    int32_t ID3V2_offset = 0;
    uint32_t ID3V2_len = 0;  
    uint32_t bytes_to_copy = 0;
    uint32_t recv_data_offset = 0;
    uint32_t enc_ptr_offset = 0;
    uint32_t clear_flag = 0;
    struct framebuff *recv_frame_buf = NULL;
    struct framebuff *send_frame_buf = NULL;
    AUCODE_HDL *mp3_dec = NULL;
    AUCODE_FRAME_INFO mp3_info; 

    mp3_dec = audio_coder_open(MP3_DEC, 0, 0);
    if(mp3_dec == NULL)
        return;  
    enc_ptr = s->inbuf;
    while(1) {
get_recv_frame_buf_again:
        os_event_wait(&s->event, clear_event, &clear_flag, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
        if(clear_flag & clear_event) {
            clear_flag = 0;
            clear_finish = 0;
        }
        recv_frame_buf = msi_get_fb(s->msi, 0);
        if(recv_frame_buf) {
            data_len = recv_frame_buf->len;
            recv_data = recv_frame_buf->data;
            recv_data_offset = 0;
            if(data_len <= 0)
                endOfRead = 1;
            if(!s->get_first_frame) {  
                if(data_len <= 0)
                    goto mp3_decode_end;
                if((os_strncmp(recv_data, "ID3", 3) == 0) && !find_ID3) {
                    ID3V2_len = (recv_data[6]&0x7F)*0x200000+ (recv_data[7]&0x7F)*0x4000 + 
                                                (recv_data[8]&0x7F)*0x80 +(recv_data[9]&0x7F);
                    ID3V2_offset = (ID3V2_len+10);
                    while(ID3V2_offset >= data_len) {
                        ID3V2_offset -= data_len;
                        update_frame_buf(s->msi, recv_frame_buf);
                        data_len = recv_frame_buf->len;
                        if(data_len <= 0)
                            goto mp3_decode_end;
                    }
                    recv_data_offset = ID3V2_offset;
                    data_len -= ID3V2_offset;
                    if(data_len <= 1) {
                        os_memcpy(s->inbuf, recv_data+recv_data_offset, data_len);
                        s->buf_size = data_len;
                        unproc_data_size = data_len;
                        update_frame_buf(s->msi, recv_frame_buf);
                        recv_data_offset = 0;
                        data_len = recv_frame_buf->len;
                        if(data_len <= 0)
                            goto mp3_decode_end;
                    }
                }
                find_ID3 = 1;  
                while(data_len) {
                    bytes_to_copy = data_len>(BUFF_SIZE-unproc_data_size)?(BUFF_SIZE-unproc_data_size):data_len;
                    os_memcpy(s->inbuf+unproc_data_size, recv_data+recv_data_offset, bytes_to_copy);
                    s->buf_size += bytes_to_copy;
                    data_len -= bytes_to_copy;
                    recv_data_offset += bytes_to_copy;
                    for(uint32_t i=0; i<(s->buf_size-1); i++) {
                        if( ( (s->inbuf[i] << 8) | (s->inbuf[i+1] & 0xE0) ) == 0xFFE0 ) {
                            s->get_first_frame = 1;
                            unproc_data_size = s->buf_size - i;
                            s->buf_size -= i; 
                            enc_ptr_offset += i;
                            goto mp3_get_first_frame;
                        }
                    } 
                }
                if(!s->get_first_frame) {
                    unproc_data_size = 1;
                    s->inbuf[0] = s->inbuf[s->buf_size-1];
                    s->buf_size = 1;
                    continue;
                }
            }
        } 
        else {
            if(clear_finish == 0) {
                clear_finish = 1;
                os_event_set(&s->event, clear_finish_event, NULL);
            }
            os_sleep_ms(1);
        }
mp3_get_first_frame:
        if(next_status == AUDIO_STOP) {
            goto mp3_decode_end;
        }
        if(next_status == AUDIO_PAUSE) {
            if(recv_frame_buf)
                msi_delete_fb(s->msi, recv_frame_buf);
            recv_frame_buf = NULL;
            enc_ptr_offset = 0;
            unproc_data_size = 0;
            if(current_status == AUDIO_RUN) {
                audio_coder_close(mp3_dec);
                if(send_frame_buf) {
                    msi_delete_fb(s->msi, send_frame_buf);
                }
                send_frame_buf = NULL;
                if(s->direct_to_dac)
                    msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,0);
                mp3_dec = audio_coder_open(MP3_DEC, 0, 0);
                if(mp3_dec == NULL)
                    return;  
            }
            current_status = AUDIO_PAUSE;
        }
        else
            current_status = AUDIO_RUN;
        if(clear_finish == 0) {
            if(recv_frame_buf)
                msi_delete_fb(s->msi, recv_frame_buf);
            recv_frame_buf = NULL;
            enc_ptr_offset = 0;
            unproc_data_size = 0;
            continue;
        }
        while((data_len>0) || (unproc_data_size>0)) {
            if((data_len) >= (BUFF_SIZE-unproc_data_size)) {
                os_memcpy(enc_ptr, enc_ptr+enc_ptr_offset, unproc_data_size);
                bytes_to_copy = BUFF_SIZE - unproc_data_size;
                os_memcpy(enc_ptr+unproc_data_size, recv_data+recv_data_offset, bytes_to_copy);
                data_len -= bytes_to_copy;
                recv_data_offset += bytes_to_copy;
                enc_ptr_offset = 0;
                unproc_data_size += bytes_to_copy;
            }
            else if(!endOfRead) {
                bytes_to_copy = data_len;
                if(bytes_to_copy) {
                    os_memcpy(enc_ptr, enc_ptr+enc_ptr_offset, unproc_data_size);
                    os_memcpy(enc_ptr+unproc_data_size, recv_data+recv_data_offset, bytes_to_copy);
                    data_len -= bytes_to_copy;
                    recv_data_offset += bytes_to_copy;
                    enc_ptr_offset = 0;
                    unproc_data_size += bytes_to_copy;
                }
                msi_delete_fb(NULL, recv_frame_buf);
                recv_frame_buf = NULL;
                goto get_recv_frame_buf_again;
            }  
            while(!send_frame_buf) {
                send_frame_buf = fbpool_get(&s->tx_pool, 0, s->msi);
                if(!send_frame_buf)
                    os_sleep_ms(1);
            }
            send_data = (int16_t*)send_frame_buf->data;
            dec_samples = audio_decode_data(mp3_dec, enc_ptr+enc_ptr_offset, unproc_data_size, s->dec_buf, &mp3_info);
			if(dec_samples <= 0) {
				enc_ptr_offset += 1;
				unproc_data_size -= 1;
                msi_delete_fb(s->msi, send_frame_buf);
                send_frame_buf = NULL;
				continue;
			}  
            if(mp3_info.channels == 2) {
                for(uint32_t i=0; i<dec_samples; i++) 
                    send_data[i] = s->dec_buf[2*i];
            }
			else {
				for(uint32_t i=0; i<dec_samples; i++)
					send_data[i] = s->dec_buf[i];
			}
            enc_ptr_offset += mp3_info.frame_bytes;
            unproc_data_size -= mp3_info.frame_bytes;
            send_frame_buf->len = dec_samples*2;
            send_frame_buf->mtype = F_AUDIO;
            send_frame_buf->stype = FSTYPE_AUDIO_MP3_DECODER;
            if((s->direct_to_dac) && (s->cur_sampleRate != mp3_info.samplerate)) {
                s->cur_sampleRate = mp3_info.samplerate;
                msi_output_cmd(s->msi, MSI_CMD_AUDAC, MSI_AUDAC_SET_SAMPLING_RATE, s->cur_sampleRate);
            }
            ret = msi_output_fb(s->msi, send_frame_buf);  
            MP3_DEBUG("mp3 decode send framebuff:%p,ret:%d\r\n",send_frame_buf,ret);
            send_frame_buf = NULL;                           
        }
        if(endOfRead)
            goto mp3_decode_end;               
    }  
mp3_decode_end:
    if(s->direct_to_dac)
	    msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,0);
    if(recv_frame_buf) {
        msi_delete_fb(NULL, recv_frame_buf);
		recv_frame_buf = NULL;
	}
    if(send_frame_buf) {
        msi_delete_fb(s->msi, send_frame_buf);
        send_frame_buf = NULL;
    }
    if(mp3_dec) {
        audio_coder_close(mp3_dec);
	} 
}

static void mp3_decode_thread(void *d)
{
	int32_t ret = 0;
    uint32_t former_dac_filter_type = 0;
    uint32_t former_dac_samplingrate = 0;
    struct mp3_decode_struct *s = (struct mp3_decode_struct *)d;

	if(s->mp3_fp) {
		curmp3_info_init(s->mp3_filename);
		s->file_size = osal_fsize(s->mp3_fp);
		get_curmp3_size(s->file_size);	
		find_first_frame(s->mp3_fp);
		if(cur_mp3_info->normal_frame_offset)
			s->get_first_frame = 1;	
        if(cur_mp3_info) {
            osal_fseek(s->mp3_fp, cur_mp3_info->first_frame_offset);
        }	
	}

    msi_get(s->msi);

    if(s->direct_to_dac) {
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_GET_FILTER_TYPE,(uint32_t)(&former_dac_filter_type));
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TYPE,FSTYPE_AUDIO_MP3_DECODER);
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_GET_SAMPLING_RATE,(uint32_t)(&former_dac_samplingrate));
        s->cur_sampleRate = former_dac_samplingrate;
    }

    if(s->mp3_fp)
        mp3_file_decode(s);
    else
        mp3_msi_decode(s);

    if(s->direct_to_dac) {
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_SAMPLING_RATE,former_dac_samplingrate);
        ret = msi_output_cmd(s->msi,MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TYPE,former_dac_filter_type);
    }

    os_event_set(&s->event, exit_event, NULL);

    msi_put(s->msi);

    if(next_status != AUDIO_STOP)
        mp3_decode_destroy();
}

static int32_t mp3_decode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
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
        case MSI_CMD_FREE_FB:
        {
            if(mp3_decode_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                fbpool_put(&mp3_decode_s->tx_pool, frame_buf);
            }
            ret = RET_OK+1;
            break; 
        }  
        case MSI_CMD_PRE_DESTROY:
        {
            if(mp3_decode_s) {
                if(mp3_decode_s->task_hdl) {
                    set_mp3_decode_status(AUDIO_STOP);
                }
            }
            ret = RET_OK;
            break;
        }      
		case MSI_CMD_POST_DESTROY:
        {
            if(mp3_decode_s) {
                if(mp3_decode_s->task_hdl) {
                    os_event_wait(&mp3_decode_s->event, exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
                for(uint32_t i=0; i<MAX_MP3_DECODE_TXBUF; i++) {
                    struct framebuff *frame_buf = (mp3_decode_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        MP3_DECODE_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&mp3_decode_s->tx_pool);
                if(mp3_decode_s->mp3_fp) {
                    osal_fclose(mp3_decode_s->mp3_fp);
                    mp3_decode_s->mp3_fp = NULL;
                }
                if(mp3_decode_s->event.hdl)
                    os_event_del(&mp3_decode_s->event);
                MP3_DECODE_FREE(mp3_decode_s);
                mp3_decode_s = NULL;
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

int32_t mp3_decode_add_output(const char *msi_name)
{
    int32_t ret = RET_ERR;
	if(!msi_name) {
		MP3_INFO("mp3 decode add output fail,msi_name is null\n");
		return RET_ERR;
	}
	if(!mp3_decode_s) {
		MP3_INFO("mp3 decode add output fail,mp3_decode_s is null\n");
		return RET_ERR;
	}
    if(next_status == AUDIO_STOP) {
        MP3_INFO("mp3 decode add output fail,next_status is stop\n");
        return RET_ERR;
    }
	ret = msi_add_output(mp3_decode_s->msi, NULL, msi_name);	
    return RET_OK;	
}

int32_t mp3_decode_del_output(const char *msi_name)
{
    int32_t ret = RET_ERR;
	if(!msi_name) {
		MP3_INFO("mp3 decode del output fail,msi_name is null\n");
		return RET_ERR;
	}
	if(!mp3_decode_s) {
		MP3_INFO("mp3 decode del output fail,mp3_decode_s is null\n");
		return RET_ERR;
	}
	ret = msi_del_output(mp3_decode_s->msi, NULL, msi_name);	
    return ret;	
}

static void mp3_decode_destroy(void)
{
	msi_destroy(mp3_decode_s->msi);
}

void mp3_decode_pause(void)
{
    set_mp3_decode_status(AUDIO_PAUSE);
}

void mp3_decode_continue(void)
{
    if(get_mp3_decode_status() == AUDIO_PAUSE)
        set_mp3_decode_status(AUDIO_RUN);
}

void mp3_decode_clear(void)
{
    if(mp3_decode_s) {
        os_event_set(&mp3_decode_s->event, clear_event, NULL);
        os_event_wait(&mp3_decode_s->event, clear_finish_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
    }
}

int32_t mp3_decode_deinit(void)
{
    if(!mp3_decode_s) {
        MP3_INFO("mp3_decode_deinit fail,mp3_decode_s is null!\r\n");
        return RET_ERR;
    }
    mp3_decode_destroy();
    return RET_OK;
}

struct msi *mp3_decode_init(uint8_t *filename, uint8_t direct_to_dac)
{
#if AUDIO_EN
    uint8_t msi_isnew = 0;
    uint8_t *data = NULL;
	uint32_t count = 0;
	struct framebuff *frame_buf = NULL;
    void *mp3_fp = NULL;

    while((get_mp3_decode_status() != AUDIO_STOP) && count < 2000) {
        os_sleep_ms(1);
        count ++;
    }
    if(count >= 2000) {
        MP3_INFO("mp3 decode init timeout!\r\n");
        return NULL;
    }
	struct msi *msi = msi_new("SR_MP3_DECODE", MAX_MP3_DECODE_RXBUF, &msi_isnew);
	if(!msi) {
		MP3_INFO("create mp3 decode msi fail!\r\n");
		return NULL;
	}
    else if(!msi_isnew) {
		MP3_INFO("mp3 decode msi has been create!\r\n");
        msi_destroy(msi);
		return NULL;        
    }
    if(direct_to_dac)
	    msi_add_output(msi, NULL, "R_AUDAC");
	msi->enable = 1;
	msi->action = (msi_action)mp3_decode_msi_action;        
	mp3_decode_s = (struct mp3_decode_struct *)MP3_DECODE_ZALLOC(sizeof(struct mp3_decode_struct));
	if(!mp3_decode_s) {
		MP3_INFO("mp3_decode_s malloc fail!\r\n");
		goto mp3_decode_init_err;
	}
	fbpool_init(&mp3_decode_s->tx_pool, MAX_MP3_DECODE_TXBUF);
	for(uint32_t i=0; i<MAX_MP3_DECODE_TXBUF; i++) {
		frame_buf = (mp3_decode_s->tx_pool.pool)+i;
		frame_buf->data = NULL;
		data = (uint8_t*)MP3_DECODE_MALLOC(1152 * sizeof(int16_t));
		if(!data)
		{
			MP3_INFO("mp3 decode malloc framebuff data fail!\r\n");
			goto mp3_decode_init_err;       
		}  
		frame_buf->data = data;  
	}
	mp3_decode_s->mp3_filename = (uint8_t*)filename;
	if(mp3_decode_s->mp3_filename)    {
		mp3_fp = osal_fopen((const char*)mp3_decode_s->mp3_filename, "rb");
		if(!mp3_fp) {
            MP3_INFO("open mp3 file %s fail!\r\n", mp3_decode_s->mp3_filename);
            goto mp3_decode_init_err;
        }	
        mp3_decode_s->mp3_fp = mp3_fp;
	}
	mp3_decode_s->msi = msi;
    mp3_decode_s->direct_to_dac = direct_to_dac;
    if(os_event_init(&mp3_decode_s->event) != RET_OK) {
        AAC_INFO("create mp3 decode event fail!\r\n");
        goto mp3_decode_init_err;
    }
	next_status = AUDIO_RUN;
	current_status = AUDIO_RUN;
	mp3_decode_s->task_hdl = os_task_create("mp3_decode_thread", mp3_decode_thread, (void*)mp3_decode_s, OS_TASK_PRIORITY_BELOW_NORMAL, 0, NULL, 3072);
	if(mp3_decode_s->task_hdl == NULL)  {
		MP3_INFO("create mp3 decode task fail!\r\n");
		goto mp3_decode_init_err;
	}
	return msi;
	
mp3_decode_init_err:
	if(mp3_fp) {
		osal_fclose(mp3_fp);
		mp3_fp = NULL;
        mp3_decode_s->mp3_fp = NULL;
	}
	msi_destroy(msi);
#endif
    return NULL;
}