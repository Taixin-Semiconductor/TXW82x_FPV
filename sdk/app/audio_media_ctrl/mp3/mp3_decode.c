#include "basic_include.h"
#include "csi_kernel.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/audio/audio_code/audio_code.h"
#include "autpc_msi/autpc_msi.h"
#include "osal_file.h"
#include "mp3_decode.h"
#include "mp3_getInfo.h"

#define BUFF_SIZE   2048
#define MAX_MP3_DECODE_RXBUF    4
#define MAX_MP3_DECODE_TXBUF    4

struct mp3_decode_struct {
    AUDIO_TRACK audio_track;
    struct fbpool tx_pool;
    struct os_event event;
    struct msi *msi;
    struct msi *src_msi;
    struct msi *autpc_msi;
    char *msi_name;
    void *task_hdl;   
    void *mp3_fp;
    uint8_t mp3_filename[20];
    uint8_t loop_mode;
    uint8_t direct_to_dac;
	uint8_t use_tpc;
	uint8_t speed;
	uint8_t pitch;
    uint8_t destroy_self;
    uint8_t get_first_frame;
    uint8_t next_status;
    uint8_t current_status;
    uint8_t inbuf[BUFF_SIZE];
    int16_t dec_buf[1152*2];
    uint32_t file_size;
    uint32_t buf_offset;
    uint32_t buf_size;
};

static int32_t mp3_file_read(struct mp3_decode_struct *mp3_decode_s, uint8_t *buf, uint32_t size)
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
    uint32_t interval_time = 0;
    struct framebuff *frame_buf = NULL;
    AUCODE_HDL *mp3_dec = NULL;
    AUCODE_FRAME_INFO mp3_info;
	AUDIO_TRACK *audac_fiter_track = NULL;

    while(!s->get_first_frame) {
        bytes_to_read = BUFF_SIZE - unproc_data_size;
        read_len = mp3_file_read(s, s->inbuf+unproc_data_size,bytes_to_read);
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
                read_len = mp3_file_read(s, s->inbuf,BUFF_SIZE);
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
        if(s->next_status == AUCODEC_PAUSE) {
            if(s->current_status == AUCODEC_RUN) {
                msi_output_cmd(s->msi,MSI_CMD_AUTPC,MSI_AUTPC_END_STREAM,(uint32_t)(&(s->audio_track)));
                msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,(uint32_t)(&(s->audio_track)));
            }
            s->current_status = AUCODEC_PAUSE;
            while(s->next_status == AUCODEC_PAUSE) {
                os_sleep_ms(1);
            }
        }
        if(s->next_status == AUCODEC_EXIT) {
            goto mp3_decode_end;
        }
        s->current_status = AUCODEC_RUN;
        if(unproc_data_size < BUFF_SIZE && !endOfRead) {
            bytes_to_read = BUFF_SIZE - unproc_data_size;
			if(unproc_data_size)
				os_memcpy(s->inbuf, enc_ptr+enc_ptr_offset, unproc_data_size);	
            read_len = mp3_file_read(s, s->inbuf+unproc_data_size, bytes_to_read);
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
            frame_buf->stype = FSTYPE_AUDIO_PCM;
            s->audio_track.samplerate = mp3_info.samplerate;
            if(s->use_tpc && !s->autpc_msi) {
                s->autpc_msi = autpc_msi_init(s->audio_track.samplerate, s->speed, s->pitch, dec_samples, &(s->audio_track));
                if(s->autpc_msi == NULL) {
                    goto mp3_decode_end;
                }
                if(s->direct_to_dac) {
                    msi_add_output(s->autpc_msi, NULL, "R_AUDAC");
                }
                msi_add_output(s->msi, NULL, s->autpc_msi->name);
            }
            ret = msi_output_fb(s->msi, frame_buf);  
            MP3_DEBUG("mp3 decode send framebuff:%p,ret:%d\r\n",frame_buf,ret);
            msi_cmd("R_AUDAC", MSI_CMD_AUDAC, MSI_AUDAC_GET_FILTER_TRACK, (uint32_t)(&audac_fiter_track));
            if(s->direct_to_dac && (!s->use_tpc) && (audac_fiter_track != (&(s->audio_track)))) {
                interval_time = (frame_buf->len>>1)*1000/(mp3_info.samplerate);
                os_sleep_ms(interval_time);
            } 
            frame_buf = NULL;                 
        }        
        else if(endOfRead){
            goto mp3_decode_end;
        }
    }
mp3_decode_end:
    msi_output_cmd(s->msi,MSI_CMD_AUTPC,MSI_AUTPC_END_STREAM,(uint32_t)(&(s->audio_track)));
	msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,(uint32_t)(&(s->audio_track)));
    if(frame_buf) {
        msi_delete_fb(s->msi, frame_buf);
        frame_buf = NULL;
    }
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
        os_event_wait(&s->event, coder_clear_event, &clear_flag, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, 0);
        if(clear_flag & coder_clear_event) {
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
                os_event_set(&s->event, coder_clear_finish_event, NULL);
            }
            os_sleep_ms(1);
        }
mp3_get_first_frame:
        if(s->next_status == AUCODEC_EXIT) {
            goto mp3_decode_end;
        }
        if(s->next_status == AUCODEC_PAUSE) {
            if(recv_frame_buf)
                msi_delete_fb(s->msi, recv_frame_buf);
            recv_frame_buf = NULL;
            enc_ptr_offset = 0;
            unproc_data_size = 0;
            if(s->current_status == AUCODEC_RUN) {
                audio_coder_close(mp3_dec);
                if(send_frame_buf) {
                    msi_delete_fb(s->msi, send_frame_buf);
                }
                send_frame_buf = NULL;
                msi_output_cmd(s->msi,MSI_CMD_AUTPC,MSI_AUTPC_END_STREAM,(uint32_t)(&(s->audio_track)));
                msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,(uint32_t)(&(s->audio_track)));
                mp3_dec = audio_coder_open(MP3_DEC, 0, 0);
                if(mp3_dec == NULL)
                    return;  
            }
            s->current_status = AUCODEC_PAUSE;
        }
        else
            s->current_status = AUCODEC_RUN;
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
            send_frame_buf->stype = FSTYPE_AUDIO_PCM;
            s->audio_track.samplerate = mp3_info.samplerate;
            if(s->use_tpc && !s->autpc_msi) {
                s->autpc_msi = autpc_msi_init(s->audio_track.samplerate, s->speed, s->pitch, dec_samples, &(s->audio_track));
                if(s->autpc_msi == NULL) {
                    goto mp3_decode_end;    
                }
                if(s->direct_to_dac) {
                    msi_add_output(s->autpc_msi, NULL, "R_AUDAC");
                }
                msi_add_output(s->msi, NULL, s->autpc_msi->name);
            }
            ret = msi_output_fb(s->msi, send_frame_buf);  
            MP3_DEBUG("mp3 decode send framebuff:%p,ret:%d\r\n",send_frame_buf,ret);
            send_frame_buf = NULL;                           
        }
        if(endOfRead)
            goto mp3_decode_end;               
    }  
mp3_decode_end:
    msi_output_cmd(s->msi,MSI_CMD_AUTPC,MSI_AUTPC_END_STREAM,(uint32_t)(&(s->audio_track)));
	msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_END_STREAM,(uint32_t)(&(s->audio_track)));
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
    struct mp3_decode_struct *s = (struct mp3_decode_struct *)d;

	if(s->mp3_fp) {
		curmp3_info_init(s->mp3_filename);
		s->file_size = osal_fsize(s->mp3_fp);
		get_curmp3_size(s->file_size);	
		find_first_frame(s->mp3_fp);
		if(cur_mp3_info && cur_mp3_info->normal_frame_offset)
			s->get_first_frame = 1;		
	}

    msi_get(s->msi);

    if(s->direct_to_dac) {
        msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TRACK,(uint32_t)(&(s->audio_track)));
    }

    s->msi->enable = 1;
    if(s->mp3_fp) {
        do {
            if(cur_mp3_info)
                osal_fseek(s->mp3_fp, cur_mp3_info->first_frame_offset);
            else
                osal_fseek(s->mp3_fp, 0);
            mp3_file_decode(s);
        }while(s->loop_mode);
    }
    else
        mp3_msi_decode(s);

    if(s->direct_to_dac) {
        s->audio_track.priority = play_disabled;
        msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TRACK,(uint32_t)(&(s->audio_track)));
    }

    os_event_set(&s->event, coder_exit_event, NULL);

    while((s->next_status != AUCODEC_EXIT) && (s->destroy_self == 0)) {
        s->current_status = AUCODEC_END;
        os_sleep_ms(5);
    } 
    if(cur_mp3_info)
        clear_curmp3_info();

    if(s->src_msi) {
        msi_del_output(s->src_msi, NULL, s->msi->name);
        s->src_msi = NULL;
    }

    if(s->next_status != AUCODEC_EXIT)
        msi_destroy(s->msi);

    msi_put(s->msi);
}

static int32_t mp3_decode_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
    struct mp3_decode_struct *mp3_decode_s = (struct mp3_decode_struct*)(msi->priv);
    switch(cmd_id) {
		case MSI_CMD_AUCODER:
		{
            ret = RET_ERR;
			if(mp3_decode_s) {
				uint32_t cmd_self = (uint32_t)param1;
				switch(cmd_self) {	
                    case MSI_AUCODER_PAUSE:
                    {
                        if(mp3_decode_s->current_status == AUCODEC_RUN) {
                            mp3_decode_s->next_status = AUCODEC_PAUSE;
                        }
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_CONTINUE:
                    {
                        if(mp3_decode_s->current_status == AUCODEC_PAUSE) {
                            mp3_decode_s->next_status = AUCODEC_RUN;
                        }
                        ret = RET_OK;  
                        break;                      
                    }
                    case MSI_AUCODER_GET_STATUS:
                    {
                        *((uint32_t*)param2) = (uint32_t)(mp3_decode_s->current_status);
                        ret = RET_OK;  
                        break;                      
                    }
					case MSI_AUCODER_SET_SPEED:
					{
                        mp3_decode_s->speed = (uint8_t)param2;
                        ret = msi_output_cmd(mp3_decode_s->msi,MSI_CMD_AUTPC,MSI_AUTPC_SET_SPEED,(uint32_t)(mp3_decode_s->speed));
						break;
					}
					case MSI_AUCODER_GET_SPEED:
					{
                        *((uint32_t*)param2) = (uint32_t)(mp3_decode_s->speed);
                        ret = RET_OK;
						break;
					}
					case MSI_AUCODER_SET_PITCH:
					{
						mp3_decode_s->pitch = (uint8_t)param2;
                        ret = msi_output_cmd(mp3_decode_s->msi,MSI_CMD_AUTPC,MSI_AUTPC_SET_PITCH,(uint32_t)(mp3_decode_s->pitch));
						break;
					}
					case MSI_AUCODER_GET_PITCH:
					{
                        *((uint32_t*)param2) = (uint32_t)(mp3_decode_s->pitch);
                        ret = RET_OK;
						break;
					}
                    case MSI_AUCODER_CLEAR_STREAM:
                    {
                        os_event_set(&mp3_decode_s->event, coder_clear_event, NULL);
                        os_event_wait(&mp3_decode_s->event, coder_clear_finish_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                        msi_output_cmd(mp3_decode_s->msi,MSI_CMD_AUTPC,MSI_AUTPC_END_STREAM,(uint32_t)(&(mp3_decode_s->audio_track)));
                        msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_CLEAR_STREAM,(uint32_t)(&(mp3_decode_s->audio_track)));                        
                        ret = RET_OK;  
                        break;                                
                    }
                    case MSI_AUCODER_SET_SRCMSI:
                    {
                        if(mp3_decode_s->src_msi) {
                            msi_del_output(mp3_decode_s->src_msi, NULL, msi->name);
                        }
                        mp3_decode_s->src_msi = NULL;
                        ret = msi_add_output((struct msi*)param2, NULL, msi->name);
                        if(ret == RET_OK) {
                            mp3_decode_s->src_msi = (struct msi*)param2;
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
        case MSI_CMD_FREE_FB:
        {
            ret = RET_ERR;
            if(mp3_decode_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                fbpool_put(&mp3_decode_s->tx_pool, frame_buf);
            }
            break; 
        }  
        case MSI_CMD_PRE_DESTROY:
        {
            if(mp3_decode_s && mp3_decode_s->task_hdl) {
                mp3_decode_s->next_status = AUCODEC_EXIT;
            }
            break;
        }      
		case MSI_CMD_POST_DESTROY:
        {
            if(mp3_decode_s) {
                if(mp3_decode_s->task_hdl) {
                    os_event_wait(&mp3_decode_s->event, coder_exit_event, NULL, OS_EVENT_WMODE_OR | OS_EVENT_WMODE_CLEAR, osWaitForever);
                }
                for(uint32_t i=0; i<MAX_MP3_DECODE_TXBUF; i++) {
                    struct framebuff *frame_buf = (mp3_decode_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        MP3_DECODE_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&mp3_decode_s->tx_pool);
                if(mp3_decode_s->event.hdl) {
                    os_event_del(&mp3_decode_s->event);
                }
                if(mp3_decode_s->mp3_fp) {
                    osal_fclose(mp3_decode_s->mp3_fp);
                    mp3_decode_s->mp3_fp = NULL;
                }
				if(mp3_decode_s->autpc_msi) {
					autpc_msi_deinit(mp3_decode_s->autpc_msi);
                    mp3_decode_s->autpc_msi = NULL;
				}
                if(mp3_decode_s->msi_name) {
                    MP3_DECODE_FREE(mp3_decode_s->msi_name);
                    mp3_decode_s->msi_name = NULL;
                }
                MP3_DECODE_FREE(mp3_decode_s);
                mp3_decode_s = NULL;
            }
            break;
        }    
        default:
            break;    
    }
    return ret;
}

struct msi *mp3_decode_init(char *filename, uint8_t loop_mode, AUDEC_INIT *audec_init)
{
#if AUDIO_EN
    char *msi_name = NULL;

    msi_name = (char*)MP3_DECODE_ZALLOC(sizeof(char)*32);
    if(msi_name == NULL) {
        os_printf("alloc autpc msi namefail\n");
        return NULL;
    }
    os_snprintf(msi_name, 20, "SR_MP3_DECODE_""%04d", (int)(os_jiffies()));
	struct msi *msi = msi_new(msi_name, MAX_MP3_DECODE_RXBUF, NULL);
	os_printf("mp3 msi name:%s\n",msi_name);
	if(msi == NULL) {
		MP3_INFO("create mp3 decode msi fail!\r\n");
		return NULL;
	}      
	struct mp3_decode_struct *mp3_decode_s = (struct mp3_decode_struct*)MP3_DECODE_ZALLOC(sizeof(struct mp3_decode_struct));
	if(!mp3_decode_s) {
		MP3_INFO("mp3_decode_s malloc fail!\r\n");
		goto mp3_decode_init_err;
	}
    msi->priv = mp3_decode_s;
	msi->action = (msi_action)mp3_decode_msi_action; 
	fbpool_init(&mp3_decode_s->tx_pool, MAX_MP3_DECODE_TXBUF);
	for(uint32_t i=0; i<MAX_MP3_DECODE_TXBUF; i++) {
		struct framebuff *frame_buf = (mp3_decode_s->tx_pool.pool)+i;
		frame_buf->data = (uint8_t*)MP3_DECODE_MALLOC(1152 * sizeof(int16_t));
		if(frame_buf->data == NULL)
		{
			MP3_INFO("mp3 decode malloc framebuff data fail!\r\n");
			goto mp3_decode_init_err;       
		}  
		frame_buf->priv = &(mp3_decode_s->audio_track);	
	}
    if(os_event_init(&mp3_decode_s->event) != RET_OK) {
        MP3_INFO("create mp3 decode event fail!\r\n");
        goto mp3_decode_init_err;
    }
	if(filename) {
        os_memcpy(mp3_decode_s->mp3_filename, filename, 20);
		mp3_decode_s->mp3_fp = osal_fopen((const char*)mp3_decode_s->mp3_filename, "rb");
		if(mp3_decode_s->mp3_fp == NULL) {
            MP3_INFO("open mp3 file %s fail!\r\n", mp3_decode_s->mp3_filename);
            goto mp3_decode_init_err;
        }	
	}
    if(audec_init->src_msi && (msi_add_output(audec_init->src_msi, NULL, msi->name) != RET_OK)) {
        goto mp3_decode_init_err;
    }
	mp3_decode_s->msi = msi;
    mp3_decode_s->msi_name = msi_name;
    mp3_decode_s->src_msi = audec_init->src_msi;
    mp3_decode_s->loop_mode = loop_mode;
    mp3_decode_s->direct_to_dac = audec_init->direct_to_dac;
	mp3_decode_s->use_tpc = audec_init->use_tpc;
	mp3_decode_s->speed = audec_init->speed;
	mp3_decode_s->pitch = audec_init->pitch;
    mp3_decode_s->destroy_self = audec_init->destroy_self;
    mp3_decode_s->audio_track.priority = audec_init->priority;
    mp3_decode_s->audio_track.track_type = audec_init->track_type;
	mp3_decode_s->next_status = AUCODEC_RUN;
	mp3_decode_s->current_status = AUCODEC_RUN;
    if(audec_init->direct_to_dac && !mp3_decode_s->use_tpc) {
	    msi_add_output(msi, NULL, "R_AUDAC");
    }
	mp3_decode_s->task_hdl = os_task_create("mp3_decode_thread", mp3_decode_thread, (void*)mp3_decode_s, OS_TASK_PRIORITY_BELOW_NORMAL, 0, NULL, 3072);
	if(mp3_decode_s->task_hdl == NULL)  {
		MP3_INFO("create mp3 decode task fail!\r\n");
		goto mp3_decode_init_err;
	}
	return msi;
	
mp3_decode_init_err:
	msi_destroy(msi);
#endif
    return NULL;
}