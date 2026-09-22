#include "basic_include.h"
#include "csi_kernel.h"
#include "syscfg.h"
#include "lwip/sockets.h"
#include "netif/ethernetif.h"
#include "lib/umac/ieee80211.h"
#include "audio_msi/audio_adc.h"
#include "lib/multimedia/audio.h"
#include "magic_voice/magic_voice.h"
#include "intercom.h"

#define MAX_INTERCOM_RXBUF      14
#define MAX_INTERCOM_TXBUF      8

#define CHANGE_PLAY_SPEED       1
#define PLC_PROCESS             1
#define BITRATE_ADJUST			0

#define CODEC_SAMPLERATE        8000

#define HIGH_BITRATE            16000
#define LOW_BITRATE             8000

#define FRAME_TIME              20

#define INTERCOM_PORT          	5008

#define MAX_ENCODED_LEN        	280
#define ENCODED_BUF_NUM        	15
#define ENCODED_RINGBUF_LEN    	1500

#define SUBLIST_NUM            	40
#define SOFTBUF_LEN            	4000
#define NODE_DATA_LEN          	80

#define HEAD_RESERVE_BYTE       18
#define NUM_OF_FRAME            1

#define LOSE_STATISTICAL        1

#define TIMEOUT_COUNT           500

static uint32_t last_statistical_time[MAX_INTERCOM_SLAVE_DEVICE] = {0};
static uint32_t lose_total[MAX_INTERCOM_SLAVE_DEVICE] = {0};
static uint32_t max_lose_cnt[MAX_INTERCOM_SLAVE_DEVICE] = {0};

static struct msi *global_intercom_msi = NULL;
static uint8_t send_start_flag = 3;  //BIT(0)可供外部控制是否发送，BIT(1)为内部逻辑判断
static uint8_t play_start_flag = BIT(MAX_INTERCOM_SLAVE_DEVICE);  //BIT(0~(MAX_INTERCOM_SLAVE_DEVICE-1))是否播放，BIT(MAX_INTERCOM_SLAVE_DEVICE)是否接收;   //BIT(0)是否播放，BIT(1)是否接收

enum {
    clear_useList_event = BIT(0),
    clear_useList_finish_event = BIT(1),
};

void intercom_deinit();

static void intercom_task_state_init(INTERCOM_STRUCT *intercom_s)
{
	intercom_s->run_state = intercom_run;
	intercom_s->run_task = 0;
}

static void intercom_task_increase(INTERCOM_STRUCT *intercom_s)
{
	os_mutex_lock(&intercom_s->state_mutex, osWaitForever);
	intercom_s->run_task++;
	os_mutex_unlock(&intercom_s->state_mutex);
}

static void intercom_task_decrease(INTERCOM_STRUCT *intercom_s)
{
	os_mutex_lock(&intercom_s->state_mutex, osWaitForever);
	intercom_s->run_task--;
	os_mutex_unlock(&intercom_s->state_mutex);
}

static uint8_t intercom_task_state(INTERCOM_STRUCT *intercom_s)
{
	uint8_t state = 0;
	os_mutex_lock(&intercom_s->state_mutex, osWaitForever);
	state = intercom_s->run_task;
	os_mutex_unlock(&intercom_s->state_mutex);
	return state;
}

static void intercom_close_socket(INTERCOM_STRUCT *intercom_s)
{
	if(intercom_s->local_trans_fd >= 0) {
		close(intercom_s->local_trans_fd);
	}
	if(intercom_s->local_ret_fd >= 0) {
		close(intercom_s->local_ret_fd);
	}
}

static int intercom_room_init(INTERCOM_STRUCT *intercom_s)
{	
	intercom_s->encoded_ringbuf = (RINGBUF*)ringbuf_Init(1, ENCODED_RINGBUF_LEN);
	if(!(intercom_s->encoded_ringbuf)) {
		os_printf("intercom encode_ringbuf init fail!\n");
		return RET_ERR;
	}	
	intercom_s->sort_buf = (uint8_t*)INTERCOM_MALLOC(SOFTBUF_LEN * sizeof(uint8_t));
	if(!intercom_s->sort_buf) {
		os_printf("intercom malloc sort_buf fail!\n");
		return RET_ERR;
	}	
	intercom_s->send_buf = (uint8_t*)INTERCOM_MALLOC(1400 * sizeof(uint8_t));
	if(!intercom_s->send_buf) {
		os_printf("intercom malloc send_buf fail!\n");
		return RET_ERR;
	}		
	intercom_s->recv_buf = (uint8_t*)INTERCOM_MALLOC(1400 * sizeof(uint8_t));
	if(!intercom_s->recv_buf) {
		os_printf("intercom malloc recv_buf fail!\n");
		return RET_ERR;
	}		

    for(uint32_t i=0; i<MAX_INTERCOM_SLAVE_DEVICE; i++) {
		INIT_LIST_HEAD((struct list_head *)&intercom_s->checkList_head[i]);
        INIT_LIST_HEAD((struct list_head *)&intercom_s->useList_head[i]);
    }
	INIT_LIST_HEAD(&intercom_s->nodeList_head);
	INIT_LIST_HEAD(&intercom_s->sublist_head);
	INIT_LIST_HEAD(&intercom_s->ringbuf_manage_empty);
	INIT_LIST_HEAD(&intercom_s->ringbuf_manage_used);
	INIT_LIST_HEAD(&intercom_s->device_head);

	audio_node *audio_node_src = (audio_node*)INTERCOM_ZALLOC(sizeof(audio_node) * (SOFTBUF_LEN / NODE_DATA_LEN));
	if(!audio_node_src) {
		os_printf("intercom malloc audio_node_src fail!\n");
		return RET_ERR;		
	}
	for(uint32_t i=0; i<(SOFTBUF_LEN/NODE_DATA_LEN); i++) {
		audio_node_src[i].buf_addr = (uint8_t*)(intercom_s->sort_buf+(NODE_DATA_LEN * i));
		list_add_tail(&(audio_node_src[i].list), &intercom_s->nodeList_head); 
	}
	intercom_s->audio_node_src = audio_node_src;

	sublist *sublist_src = (sublist*)INTERCOM_ZALLOC(sizeof(sublist) * SUBLIST_NUM);
	if(!sublist_src) {
		os_printf("intercom malloc sublist_src fail!\n");
		return RET_ERR;		
	}
	for(uint32_t i=0; i<SUBLIST_NUM; i++) {
		sublist_src[i].node_head.next = &(sublist_src[i].node_head);
		sublist_src[i].node_head.prev = &(sublist_src[i].node_head);
		list_add_tail(&sublist_src[i].list, &intercom_s->sublist_head); 
	}
	intercom_s->sublist_src = sublist_src;

	ringbuf_manage *ringbuf_manage_src = (ringbuf_manage*)INTERCOM_ZALLOC(sizeof(ringbuf_manage) * ENCODED_BUF_NUM);
	if(!ringbuf_manage_src) {
		os_printf("intercom malloc ringbuf_manage_src fail!\n");
		return RET_ERR;		
	}
	for(uint32_t i=0; i<ENCODED_BUF_NUM; i++) {
		list_add_tail(&ringbuf_manage_src[i].list, &intercom_s->ringbuf_manage_empty); 
	}
	intercom_s->ringbuf_manage_src = ringbuf_manage_src;
	intercom_s->manage_cur_pop = &intercom_s->ringbuf_manage_used;
	intercom_s->manage_cur_push = &intercom_s->ringbuf_manage_used;

	os_printf("intercom_room_init succes\n");
	return RET_OK;
}

static void intercom_room_free(INTERCOM_STRUCT *intercom_s)
{
	if(intercom_s->encoded_ringbuf) 
		ringbuf_del(intercom_s->encoded_ringbuf);
	if(intercom_s->sort_buf) 
		INTERCOM_FREE(intercom_s->sort_buf);
	if(intercom_s->send_buf) 
		INTERCOM_FREE(intercom_s->send_buf);
	if(intercom_s->recv_buf) 
		INTERCOM_FREE(intercom_s->recv_buf);
	if(intercom_s->audio_node_src) 
		INTERCOM_FREE(intercom_s->audio_node_src);
	if(intercom_s->sublist_src) 
		INTERCOM_FREE(intercom_s->sublist_src);
	if(intercom_s->ringbuf_manage_src) 
		INTERCOM_FREE(intercom_s->ringbuf_manage_src);
}

static void *get_ringbuf_manage_addr(struct list_head *list)
{
	ringbuf_manage *ringbuf_mana;
	ringbuf_mana = list_entry(list, ringbuf_manage, list);
	return ringbuf_mana->buf_addr;
}
static uint32_t get_ringbuf_manage_datalen(struct list_head *list)
{
	ringbuf_manage *ringbuf_mana;
	ringbuf_mana = list_entry(list, ringbuf_manage, list);
	return (uint32_t)ringbuf_mana->data_len;
}
static int32_t get_ringbuf_manage_count(INTERCOM_STRUCT *intercom_s)
{
	int count = 0;
	struct list_head *list_n = intercom_s->manage_cur_pop;
	struct list_head *head = intercom_s->manage_cur_push;
	while(list_n != head) {
		list_n = list_n->next;
		count++;
	}
	return count;			
}
static struct list_head *get_ringbuf_manage(INTERCOM_STRUCT *intercom_s, uint8_t grab)
{
	if(list_empty_careful((const struct list_head *)&intercom_s->ringbuf_manage_empty)) {
		if(grab) {
			list_move_tail(intercom_s->ringbuf_manage_used.next, &intercom_s->ringbuf_manage_used);
		}
		else {		
			return 0;
		}
	}
	else {
		list_move_tail(intercom_s->ringbuf_manage_empty.next, &intercom_s->ringbuf_manage_used);
	}
	return intercom_s->ringbuf_manage_used.prev;				
}
static void del_ringbuf_manage(INTERCOM_STRUCT *intercom_s)
{
	list_move_tail(intercom_s->ringbuf_manage_used.next, &intercom_s->ringbuf_manage_empty);
}

static void output_sema_up(uint32_t *args)
{
	INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)args;
	if(intercom_s->output_sema.hdl) {
		os_sema_up(&intercom_s->output_sema);
	}
	return;
}

void losePacket_retransfer(INTERCOM_STRUCT *intercom_s, uint8_t *addr, uint32_t len)
{
	int32_t slen = 0;
	if((addr + len) > ((uint8_t*)(intercom_s->encoded_ringbuf->data +
								intercom_s->encoded_ringbuf->elementcount))) {
		uint32_t residue_len = (uint8_t*)(intercom_s->encoded_ringbuf->data + 
								intercom_s->encoded_ringbuf->elementcount) - addr;
		os_memcpy(intercom_s->send_buf, addr, residue_len);
		os_memcpy(intercom_s->send_buf + residue_len, intercom_s->encoded_ringbuf->data, 
																		len - residue_len);
	}
	else {
		os_memcpy(intercom_s->send_buf, addr, len);
	}	
	slen = sendto(intercom_s->local_trans_fd, intercom_s->send_buf, len, 0,
		(struct sockaddr*)&(intercom_s->remote_retrans_addr), sizeof(intercom_s->remote_trans_addr));
}

static void intercom_retransfer_check(INTERCOM_STRUCT *intercom_s)
{
	uint8_t *addr = NULL;
	uint8_t sequence = 0;
	uint8_t cnt = 0;
	int32_t recv_len = 0;	
	uint32_t lose_packet = 0;
	uint32_t send_totallen = 0;
	struct list_head *manage_p = NULL;
	socklen_t addrlen = sizeof(struct sockaddr_in);

	os_mutex_lock(&intercom_s->send_mutex, osWaitForever);
	recv_len = recvfrom(intercom_s->local_ret_fd, &lose_packet, 4, 0, 
			(struct sockaddr*)&(intercom_s->remote_retrans_addr), &addrlen);
	intercom_s->remote_retrans_addr.sin_port = htons(INTERCOM_PORT);
	if(send_start_flag != 0x03)
		goto intercom_retransfer_check_end;
	if(recv_len > 0) {
		// os_printf("ret:%x %d\n",intercom_s->remote_retrans_addr.sin_addr.s_addr,lose_packet);
		manage_p = intercom_s->ringbuf_manage_used.next;
		while(manage_p != &intercom_s->ringbuf_manage_used) {
			cnt++;
			send_totallen = 0;
			addr = (uint8_t*)get_ringbuf_manage_addr(manage_p);
			sequence = *addr;
			if(BIT(sequence) & lose_packet) {	
				send_totallen = get_ringbuf_manage_datalen(manage_p);
				losePacket_retransfer(intercom_s, addr, send_totallen);		
				os_sleep_ms(1);
			}
			manage_p = manage_p->next;
			if(cnt >= ENCODED_BUF_NUM)
				break;
		}	
#if BITRATE_ADJUST == ADJUST_BY_LOSS
		if(lose_packet & BIT(31)) {
			intercom_s->new_bitrate_mode = low_bitrate_mode;
		}	
		else {
			intercom_s->new_bitrate_mode = high_bitrate_mode;
		}
#endif
	}
intercom_retransfer_check_end:
	os_mutex_unlock(&intercom_s->send_mutex);
}

static uint16_t calulate_sum(uint8_t * buf, uint16_t len) 
{
	uint16_t sum = 0;
	for(uint16_t i=0; i<len; i++) {
		sum += *(buf+i);
	}
	return sum;
}

void ringbuf_write_pre(INTERCOM_STRUCT *intercom_s, uint32_t size)
{
	uint32_t last_front = 0;
	uint32_t cur_front = 0;
	uint32_t lookback = 0;
	ringbuf_manage *ringbuf_mana_n = NULL;
	struct list_head *ringbuf_mana_l = NULL;
	ringbuf_manage *manage_n_del = NULL;
	ringbuf_manage *npos = NULL;

	ringbuf_mana_l = get_ringbuf_manage(intercom_s, 0);
	if(!ringbuf_mana_l) {
		if(intercom_s->manage_cur_pop == intercom_s->ringbuf_manage_used.next)
			intercom_s->manage_cur_pop = intercom_s->manage_cur_pop->next;
		ringbuf_mana_l = get_ringbuf_manage(intercom_s, 1);
	}
	intercom_s->manage_cur_push = ringbuf_mana_l;
	ringbuf_mana_n = list_entry(ringbuf_mana_l, ringbuf_manage, list);
	ringbuf_mana_n->buf_addr = (uint8_t*)intercom_s->encoded_ringbuf->data + 
											intercom_s->encoded_ringbuf->rear;
	ringbuf_mana_n->data_len = size;
	if(ringbuf_write_available(intercom_s->encoded_ringbuf) < size) {
		last_front = ringbuf_cur_front(intercom_s->encoded_ringbuf);
		ringbuf_move_readptr(intercom_s->encoded_ringbuf, size, &lookback);
		cur_front = ringbuf_cur_front(intercom_s->encoded_ringbuf);
		list_for_each_entry_safe(manage_n_del, npos, &(intercom_s->ringbuf_manage_used), list) {
			if(lookback == 1) {
				if(((manage_n_del->buf_addr+manage_n_del->data_len) > (intercom_s->encoded_ringbuf->data + last_front)) ||
					(manage_n_del->buf_addr <= (intercom_s->encoded_ringbuf->data + cur_front))) {
					if(intercom_s->manage_cur_pop == &manage_n_del->list) {
						intercom_s->manage_cur_pop = intercom_s->manage_cur_pop->next;
					}
					del_ringbuf_manage(intercom_s);
				}
			}
			else {
				if(((manage_n_del->buf_addr+manage_n_del->data_len) > (intercom_s->encoded_ringbuf->data + last_front)) &&
					(manage_n_del->buf_addr <= (intercom_s->encoded_ringbuf->data + cur_front))) {
					if(intercom_s->manage_cur_pop == &manage_n_del->list) {
						intercom_s->manage_cur_pop = intercom_s->manage_cur_pop->next;
					}
					del_ringbuf_manage(intercom_s);
				}
			}
		}
	}
}

static void intercom_send_data(INTERCOM_STRUCT *intercom_s, uint8_t num)
{
	uint8_t *addr = NULL;
	int32_t slen = 0;
	uint32_t send_totallen = 0;
	uint32_t data_len = 0;
    socklen_t addrlen = sizeof(struct sockaddr_in);	
	struct list_head *manage_p = NULL;
 
	intercom_s->manage_cur_pop = intercom_s->manage_cur_pop->next;
	manage_p = intercom_s->manage_cur_pop;
	addr = (uint8_t*)get_ringbuf_manage_addr(manage_p);
	for(uint32_t i=0; i<num; i++) {
		data_len = get_ringbuf_manage_datalen(manage_p);
		send_totallen += data_len;
		manage_p = manage_p->next;
	}
	if((addr + send_totallen) > ((uint8_t*)(intercom_s->encoded_ringbuf->data +
									intercom_s->encoded_ringbuf->elementcount))) {
		uint32_t residue_len = (uint8_t*)(intercom_s->encoded_ringbuf->data +
									intercom_s->encoded_ringbuf->elementcount) - addr;
		os_memcpy(intercom_s->send_buf, addr, residue_len);
		os_memcpy(intercom_s->send_buf + residue_len, intercom_s->encoded_ringbuf->data, 
															send_totallen - residue_len);
	}
	else
		os_memcpy(intercom_s->send_buf, addr, send_totallen);
	os_mutex_lock(&intercom_s->send_mutex, osWaitForever);
	slen = sendto(intercom_s->local_trans_fd, intercom_s->send_buf, send_totallen, 0, 
				(struct sockaddr*)&(intercom_s->remote_trans_addr), addrlen);
	// os_printf("send:%d\n",*((uint16_t*)(intercom_s->send_buf+2)));		
	os_mutex_unlock(&intercom_s->send_mutex);			
}

static void intercom_send_task(void *d)
{
#if BITRATE_ADJUST == ADJUST_BY_MCS
	uint8_t avg_mcs = 7;
	uint8_t avg_cnt = 0;
#endif
	uint8_t send_sequence = 0;
	uint8_t encoded_buf[MAX_ENCODED_LEN] = {0};
	uint8_t *data = NULL;
	uint16_t send_sort = 0;
	uint32_t data_len = 0;
	uint32_t timestamp = 0;
	INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)d;
	struct framebuff *frame_buf = NULL;
	txAudioInfo_t codec_info;

	intercom_task_increase(intercom_s);
	os_random_bytes((uint8_t*)(&intercom_s->g_s_identify_num), 4);
	os_printf("\n**********intercom ID:%d***********\n",intercom_s->g_s_identify_num);

	codec_info.sample_rate = CODEC_SAMPLERATE;
	codec_info.channels = 1;
	codec_info.frame_size = FRAME_TIME * CODEC_SAMPLERATE / 1000;
	intercom_s->encoder_msi = aenc_get_msi(AUDIO_CODEC_OPUS, "adc", &codec_info);
	if(!intercom_s->encoder_msi) {
		os_printf("intercom audio encoder init fail!\n");
		intercom_task_decrease(intercom_s);
		return;
	}
	msi_add_output(NULL, "auadc_proc", intercom_s->encoder_msi, NULL);
	msi_add_output(intercom_s->encoder_msi, NULL, intercom_s->msi, NULL);
	msi_do_cmd(intercom_s->encoder_msi, MSI_CMD_START, 0, 0);
	intercom_s->send_stream_type = intercom_live_audio;
	while(1) {
		if(intercom_s->run_state == intercom_stop) 
			break;
		intercom_retransfer_check(intercom_s);
#if BITRATE_ADJUST == ADJUST_BY_MCS
		if(sys_cfgs.wifi_mode == WIFI_MODE_STA) {
			avg_mcs += (ieee80211_conf_get_tx_mcs(WIFI_MODE_STA, NULL, 0) & 0x0f);
		}
		else {
			avg_mcs += (ieee80211_conf_get_tx_mcs(WIFI_MODE_AP, NULL, 1) & 0x0f);
		}
		if(avg_cnt >= 20) {
			avg_mcs /= 20;
			avg_cnt = 0;
			if(avg_mcs >= 4) {
				intercom_s->new_bitrate_mode = high_bitrate_mode;
			}
			else {
				intercom_s->new_bitrate_mode = low_bitrate_mode;
			}
		}
		avg_cnt++;
#endif
#if BITRATE_ADJUST
		if((intercom_s->cur_bitrate_mode != intercom_s->new_bitrate_mode) && (intercom_s->new_bitrate_mode == low_bitrate_mode)) {
			msi_do_cmd(intercom_s->encoder_msi, MSI_CMD_SET_BITRATE, LOW_BITRATE, 0);
			intercom_s->cur_bitrate_mode = intercom_s->new_bitrate_mode;
			os_printf("audio set bitrate:8000\n");
		}
		else if((intercom_s->cur_bitrate_mode != intercom_s->new_bitrate_mode) && (intercom_s->new_bitrate_mode == high_bitrate_mode)) {
			msi_do_cmd(intercom_s->encoder_msi, MSI_CMD_SET_BITRATE, HIGH_BITRATE, 0);
			intercom_s->cur_bitrate_mode = intercom_s->new_bitrate_mode;
			os_printf("audio set bitrate:16000\n");
		}
#endif
		if(send_start_flag == 0x03) {
			frame_buf = msi_get_fb(intercom_s->msi, 0);
			if(frame_buf) {
				data = frame_buf->data;
				data_len = frame_buf->len;	
				send_sequence = (send_sequence % (ENCODED_BUF_NUM*2))+1;
				send_sort++;
				timestamp = os_jiffies();
				encoded_buf[0] = send_sequence;		
				if(data_len) {
					if(intercom_s->send_stream_type == intercom_live_audio)
						encoded_buf[1] = (intercom_live_audio << 4) | intercom_s->cur_bitrate_mode;
					else if(intercom_s->send_stream_type == intercom_playback_audio)
						encoded_buf[1] = (intercom_playback_audio << 4) | intercom_s->cur_bitrate_mode;
				}
				else
					encoded_buf[1] = 0;
				*((uint16_t*)(encoded_buf+2)) = send_sort;
				*((uint32_t*)(encoded_buf+4)) = timestamp;
				*((uint32_t*)(encoded_buf+8)) = data_len;
				*((uint32_t*)(encoded_buf+12)) = intercom_s->g_s_identify_num;
				*((uint16_t*)(encoded_buf+16)) = calulate_sum(encoded_buf,HEAD_RESERVE_BYTE-2);
				if(data_len)
					os_memcpy(encoded_buf + HEAD_RESERVE_BYTE, data, data_len);
				data_len += HEAD_RESERVE_BYTE;
				ringbuf_write_pre(intercom_s, data_len);
				ringbuf_write(intercom_s->encoded_ringbuf, encoded_buf, data_len);
				if(get_ringbuf_manage_count(intercom_s) >= NUM_OF_FRAME)
					intercom_send_data(intercom_s, NUM_OF_FRAME);
				msi_delete_fb(NULL, frame_buf);
				frame_buf = NULL;
			}
			else
				os_sleep_ms(1);
		}
		else {
			frame_buf = msi_get_fb(intercom_s->msi, 0);
			if(frame_buf) {
				msi_delete_fb(NULL, frame_buf);
				frame_buf = NULL;
			}
			if(get_ringbuf_manage_count(intercom_s) >= NUM_OF_FRAME)
				intercom_send_data(intercom_s, NUM_OF_FRAME);
			os_sleep_ms(10);
		}		
	}
	intercom_task_decrease(intercom_s);
}

static int32_t get_audio_node_count(struct list_head *head)
{
	int count = 0;
	struct list_head *list_n = head;
	while(list_n->next != head) {
		list_n = list_n->next;
		count++;
	}
	return count;			
}
static void *get_audio_node_addr(struct list_head *list)
{
	audio_node *audio_n;
	audio_n = list_entry(list, audio_node, list);
	return audio_n->buf_addr;
}
static struct list_head *get_audio_node(INTERCOM_STRUCT *intercom_s, struct list_head *head, uint32_t node_num)
{
	os_mutex_lock(&intercom_s->list_mutex, osWaitForever);
	if(get_audio_node_count(&intercom_s->nodeList_head) < node_num) {
		os_mutex_unlock(&intercom_s->list_mutex);
		os_printf("nodeList_head empty\n");
		return 0;
	}
	for(uint32_t i=0; i<node_num; i++)
		list_move_tail(intercom_s->nodeList_head.next, head);
	os_mutex_unlock(&intercom_s->list_mutex);
	return head->next;			
}
static void del_audio_node(INTERCOM_STRUCT *intercom_s, struct list_head *del)
{
	del->next->prev = intercom_s->nodeList_head.prev;
	del->prev->next = &(intercom_s->nodeList_head);
	intercom_s->nodeList_head.prev->next = del->next;
	intercom_s->nodeList_head.prev = del->prev;
	del->next = del;
	del->prev = del;
}

static int32_t get_audio_sublist_count(INTERCOM_STRUCT *intercom_s, struct list_head *head)
{
	int count = 0;
	struct list_head *list_n = head;
	os_mutex_lock(&intercom_s->list_mutex, osWaitForever);
	while(list_n->next != head) {
		list_n = list_n->next;
		count++;
	}
	os_mutex_unlock(&intercom_s->list_mutex);
	return count;			
}
static struct list_head *get_audio_sublist(INTERCOM_STRUCT *intercom_s)
{
	struct list_head *get_list = NULL;
	os_mutex_lock(&intercom_s->list_mutex, osWaitForever);
	if(list_empty_careful((const struct list_head *)&intercom_s->sublist_head)) {
		os_mutex_unlock(&intercom_s->list_mutex);
		os_printf("sublist_head empty\n");
		return NULL;
	}
	get_list = intercom_s->sublist_head.next;
	list_del_init(get_list);
	os_mutex_unlock(&intercom_s->list_mutex);
	return get_list;			
}
static void del_audio_sublist(INTERCOM_STRUCT *intercom_s, struct list_head *del)
{
	os_mutex_lock(&intercom_s->list_mutex, osWaitForever);	
	sublist *sublist_n = list_entry(del, sublist, list);
	if(sublist_n->node_cnt)
		del_audio_node(intercom_s, &(sublist_n->node_head));
	list_move_tail(del, &intercom_s->sublist_head);
	os_mutex_unlock(&intercom_s->list_mutex);	
}
static void insert_into_checkList(INTERCOM_STRUCT *intercom_s, uint8_t dev_id, struct list_head *del)
{
	os_mutex_lock(&intercom_s->list_mutex, osWaitForever);
	list_move_tail(del, &intercom_s->checkList_head[dev_id]);
	os_mutex_unlock(&intercom_s->list_mutex);
}
static void insert_into_useList(INTERCOM_STRUCT *intercom_s, uint8_t dev_id, struct list_head *del)
{
	sublist *sublist_n;
	sublist *sublist_n_pos;
	sublist *npos;
	sublist_n = list_entry(del, sublist, list);
	int32_t prev_sort = 0;
	uint16_t new_sort = 0;
	uint16_t next_sort = 0;
	struct list_head *head = &intercom_s->useList_head[dev_id];

	new_sort = sublist_n->sort;
	prev_sort = (play_start_flag&BIT(dev_id))?intercom_s->g_current_sort[dev_id]:-1;
	if((prev_sort >= new_sort) && ((prev_sort - new_sort) < 60000)) {
		if(sublist_n->node_cnt) {
			del_audio_node(intercom_s, &(sublist_n->node_head));
		}
		list_move_tail(del, &intercom_s->sublist_head);
		return;					
	}
	// os_printf("insert:%d\n",new_sort);
	if(list_empty_careful((const struct list_head *)head)) {	
		list_move(del, head);
		goto insert_into_useList_end;	
	}
	list_for_each_entry_safe(sublist_n_pos, npos, head, list) {
		next_sort = sublist_n_pos->sort;
		if(new_sort == next_sort) {
			if(sublist_n->node_cnt) {
				del_audio_node(intercom_s, &(sublist_n->node_head));
			}
			list_move_tail(del, &intercom_s->sublist_head);
			goto insert_into_useList_end;
		}
		else if( (prev_sort < new_sort ) && (new_sort < next_sort) && ((next_sort-new_sort) < 60000) ) {
			list_move(del, sublist_n_pos->list.prev);	
			goto insert_into_useList_end;
		}
		else if( (prev_sort>60000) && ((prev_sort - new_sort)>60000) && (new_sort < next_sort) && (prev_sort - next_sort > 60000) ) {	
			list_move(del, sublist_n_pos->list.prev);
			goto insert_into_useList_end;			
		}
		prev_sort = next_sort;
	}		
	list_move_tail(del, head);
insert_into_useList_end:
	return;	
}

static int32_t recv_repeat_check(uint8_t dev_id, uint16_t seq, uint16_t sort)
{
	static uint16_t seq_sort[MAX_INTERCOM_SLAVE_DEVICE][ENCODED_BUF_NUM*2+1] = {0};

	if(sort == seq_sort[dev_id][seq]) {
		return 1;
	}
	else {
		seq_sort[dev_id][seq] = sort;
		return 0;
	}
}

static void lose_packet_check(INTERCOM_STRUCT *intercom_s, uint8_t dev_id, sublist *sublist_n)
{ 
	static char sequence[MAX_INTERCOM_SLAVE_DEVICE] = {0};
	char temp = 0;
	static uint32_t lose_packet[MAX_INTERCOM_SLAVE_DEVICE] = {0};
	static char retrans_num[MAX_INTERCOM_SLAVE_DEVICE][ENCODED_BUF_NUM*2+1] = {0};
	socklen_t addrlen = sizeof(struct sockaddr_in);
	uint32_t last_loop_lose = 0;
	uint32_t new_loop_lose = 0;
	uint8_t seq = 0;
	uint16_t sort = 0;
	struct list_head *sublist_l = &(sublist_n->list);
	uint32_t i = 0;
	static uint8_t param_init = 0;

    if(param_init == 0) {
        param_init = 1;
        os_memset(sequence, 0, sizeof(char)*MAX_INTERCOM_SLAVE_DEVICE);
        os_memset(lose_packet, 0, sizeof(uint32_t)*MAX_INTERCOM_SLAVE_DEVICE);
        os_memset(retrans_num, 0, sizeof(char)*MAX_INTERCOM_SLAVE_DEVICE*(ENCODED_BUF_NUM*2+1));
    }

	for(i=1; i<(ENCODED_BUF_NUM*2+1); i++)
	{
		if(lose_packet[dev_id] & BIT(i)) {
			retrans_num[dev_id][i] +=1;
			if(retrans_num[dev_id][i] >= 2) {
				retrans_num[dev_id][i] = 0;
				lose_packet[dev_id] &= ~BIT(i);
			}
		}
	}

	temp = (sequence[dev_id] % (ENCODED_BUF_NUM*2))+1;
	seq = sublist_n->seq;
	if(os_abs(seq-temp) >= 8)
		temp = seq;
	sort = sublist_n->sort;
	/*收到重复包，丢弃*/
	if(recv_repeat_check(dev_id, seq, sort)) {
		del_audio_sublist(intercom_s, sublist_l);
	}	
	else
	{
		insert_into_checkList(intercom_s, dev_id, sublist_l);
		/*收到丢包，清掉该丢包位*/
		if( lose_packet[dev_id] & BIT(seq) )  {
			// os_printf("recv loss:%d\n",seq);
			lose_packet[dev_id] &= ~BIT(seq);		
			retrans_num[dev_id][seq] = 0;
		}
		else if( temp != seq ) 
		{
			if(BIT(seq) > BIT(temp))
				lose_packet[dev_id] |= (BIT(seq) - BIT(temp));   
			else {
				last_loop_lose = BIT(ENCODED_BUF_NUM*2+1) - BIT(temp);
				new_loop_lose = BIT(seq) - BIT(1);
				lose_packet[dev_id] |= (last_loop_lose|new_loop_lose);
			}
			sequence[dev_id] = seq;
		}
		else
			sequence[dev_id] = temp;
	}
	if(intercom_s->loss_state[dev_id] == serious_loss) {
		lose_packet[dev_id] |= BIT(31);
	}
	else {
		lose_packet[dev_id] &= ~BIT(31);
	}
	if(lose_packet[dev_id] & 0x7FFFFFFF) {
		intercom_s->remote_ret_addr.sin_port = htons(INTERCOM_PORT + 1);
		sendto(intercom_s->local_ret_fd, &lose_packet[dev_id], 4, 0, (struct sockaddr*)&(intercom_s->remote_ret_addr), addrlen);
		// os_printf("send loss:%x %d\n",intercom_s->remote_ret_addr.sin_addr.s_addr, lose_packet[dev_id]);
	}
#if BITRATE_ADJUST == ADJUST_BY_LOSS
	else if(intercom_s->loss_state[dev_id] != (sublist_n->type&0xF)) {
		sendto(intercom_s->local_ret_fd, &lose_packet, 4, 0, (struct sockaddr*)&(intercom_s->remote_ret_addr), addrlen);
	}
#endif
}

static uint8_t is_new_device(INTERCOM_STRUCT *intercom_s, struct sockaddr_in *remote_trans_addr)
{
	intercom_device *device;
	intercom_device *npos;
	list_for_each_entry_safe(device, npos, &(intercom_s->device_head), list) {
		if(device->ip_addr == remote_trans_addr->sin_addr.s_addr) {
			return 0;
		}
	}
	os_printf("\n*****intercom find new device*****\n");
	return 1;
}

static void add_new_device(INTERCOM_STRUCT *intercom_s, uint32_t new_identify_num, struct sockaddr_in *remote_trans_addr, uint8_t switch_new)
{
#if INTERCOM_GROUP
	if(intercom_s->connected_num >= MAX_INTERCOM_SLAVE_DEVICE) {
		return;
	}
#endif
	intercom_device *device = (intercom_device*)INTERCOM_MALLOC(sizeof(intercom_device));
	if(!device) {
		os_printf("intercom add new device fail\n");
		return;
	}
	device->identify_num = new_identify_num;
	device->ip_addr = remote_trans_addr->sin_addr.s_addr;
	list_add_tail(&(device->list), &(intercom_s->device_head));
	device->online = 1;
	device->timeout_cnt = TIMEOUT_COUNT;
/*一对多时，为一个ap对应多个sta，sta的ip地址为.100开始，固设备id以ip地址-100设定*/
#if ONE_TO_MANY
	device->dev_id = ((remote_trans_addr->sin_addr.s_addr & 0xFF000000) >> 24) - 100;
/*当为群组时，则以连接顺序做为设备id号*/
#elif INTERCOM_GROUP
	device->dev_id = intercom_s->connected_num;
#endif
	os_printf("\n*****intercom add new device,ip:%x,id:%d*****\n",device->ip_addr,device->dev_id);
	intercom_s->connected_num++;
/*一对多时，同一时刻只播放一台设备的音频，固需要选择设备id*/
#if ONE_TO_MANY
	if(switch_new || device->dev_id == intercom_s->current_dev_id) {
		intercom_s->g_r_identify_num = new_identify_num;
		intercom_s->remote_trans_addr.sin_addr.s_addr = remote_trans_addr->sin_addr.s_addr;
		intercom_s->remote_ret_addr.sin_addr.s_addr = remote_trans_addr->sin_addr.s_addr;
		intercom_s->current_dev_id = device->dev_id;
	}
#endif
}

static intercom_device *find_device(INTERCOM_STRUCT *intercom_s, struct sockaddr_in *remote_trans_addr)
{
	intercom_device *device;
	intercom_device *npos;
	list_for_each_entry_safe(device, npos, &(intercom_s->device_head), list) {
		if(device->ip_addr == remote_trans_addr->sin_addr.s_addr) {
			return device;
		}
	}
	return NULL;	
}

static void switch_device(INTERCOM_STRUCT *intercom_s, uint32_t dev_id)
{
	intercom_device *device;
	intercom_device *npos;
	os_mutex_lock(&intercom_s->send_mutex, osWaitForever);
	list_for_each_entry_safe(device, npos, &(intercom_s->device_head), list) {
		if(device->dev_id == dev_id) {
			intercom_s->g_r_identify_num = device->identify_num;
			intercom_s->remote_trans_addr.sin_addr.s_addr = device->ip_addr;
			intercom_s->remote_ret_addr.sin_addr.s_addr = device->ip_addr;	
			intercom_s->current_dev_id = dev_id;
			break;
		}
	}	
	os_mutex_unlock(&intercom_s->send_mutex);
}

static int8 clear_disconnect_device(INTERCOM_STRUCT *intercom_s)
{
	intercom_device *device;
	intercom_device *npos;
	uint8_t switch_next_device = 0;
	int8_t del_dev_id = -1;
	list_for_each_entry_safe(device, npos, &(intercom_s->device_head), list) {
		if(device->online) {
			device->timeout_cnt = TIMEOUT_COUNT;
		}
		else {
			device->timeout_cnt--;
		}
		device->online = 0;
		if(device->timeout_cnt <= 0) {
			intercom_s->connected_num--;
			del_dev_id = device->dev_id;
			os_printf("\n*****intercom del device ip:%x,id:%d*****\n",device->ip_addr,device->dev_id);
			list_del(&device->list);
			INTERCOM_FREE(device);
			if(intercom_s->current_dev_id == del_dev_id) {
				switch_next_device = 1;
			}
		}
	}
/*一对多时，当前选择的设备掉线时则自动切换到下一台设备*/
#if ONE_TO_MANY
	int8_t next_dev_id = -1;
	if(switch_next_device) {
		list_for_each_entry_safe(device, npos, &(intercom_s->device_head), list) {
			if(device->timeout_cnt > 0) {
				next_dev_id = device->dev_id;
			}
		}
		if(next_dev_id >= 0) {
			os_printf("\n*****intercom auto switch device ip:%x,id:%d*****\n",device->ip_addr,device->dev_id);
			switch_device(intercom_s, device->dev_id);
		}
	}
#endif
	return del_dev_id;
}

static void clear_one_device(INTERCOM_STRUCT *intercom_s, uint32_t dev_id)
{
	intercom_device *device;
	intercom_device *npos;
	if(intercom_s->connected_num) {
		list_for_each_entry_safe(device, npos, &(intercom_s->device_head), list) {
			if(device->dev_id == dev_id) {
				list_del(&device->list);
				INTERCOM_FREE(device);
				intercom_s->connected_num--;
			}
		}
	}
}

static void clear_all_device(INTERCOM_STRUCT *intercom_s)
{
	intercom_device *device;
	intercom_device *npos;
	if(intercom_s->connected_num) {
		list_for_each_entry_safe(device, npos, &(intercom_s->device_head), list) {
			list_del(&device->list);
			INTERCOM_FREE(device);
		}
	}
	intercom_s->connected_num = 0;
}

static void intercom_recv_task(void *d)
{
	uint8_t dev_id = 0;
	uint8_t *node_addr = NULL;
	int32_t rlen = 0;
	uint16_t check_sum = 0;
	uint32_t code_len = 0;
	uint32_t offset = 0;
	uint32_t node_num = 0;
	uint32_t recv_timeout_cnt[MAX_INTERCOM_SLAVE_DEVICE] = {0};
	uint32_t cur_r_identify_num[MAX_INTERCOM_SLAVE_DEVICE] = {0};
	socklen_t addrlen = sizeof(struct sockaddr_in);
#if ONE_TO_MANY || INTERCOM_GROUP
	uint8_t is_new = 0;
	intercom_device *device;
#endif
	uint32_t ipaddr;
	ip_addr_t ip;

	INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)d;

	intercom_task_increase(intercom_s);
	intercom_s->recv_stream_type = intercom_live_audio;
	while(1) {
		if(intercom_s->run_state == intercom_stop) 
			break;	
		rlen = recvfrom(intercom_s->local_trans_fd, intercom_s->recv_buf, 1400, 0, 
					   (struct sockaddr*)&(intercom_s->remote_ret_addr), &addrlen);
		if((rlen <= 0) || !(play_start_flag&BIT(MAX_INTERCOM_SLAVE_DEVICE))) {
			goto recv_data_end;
		}	
#if ONE_TO_MANY
		clear_disconnect_device(intercom_s);
#endif
		ip = lwip_netif_get_ip2("w0");
		ipaddr = ip_addr_get_ip4_u32(&ip);
		if(intercom_s->remote_ret_addr.sin_addr.s_addr == ipaddr) {
			goto recv_data_end;
		}
		if(rlen >= HEAD_RESERVE_BYTE) {
			offset = 0;
			while(rlen > offset) {
				check_sum = ((uint16_t)(intercom_s->recv_buf[offset + HEAD_RESERVE_BYTE - 1]) << 8)
												| intercom_s->recv_buf[offset + HEAD_RESERVE_BYTE - 2];
				if(check_sum != calulate_sum(intercom_s->recv_buf + offset, HEAD_RESERVE_BYTE - 2)) {
					os_printf("intercom abnormal packet\n");
					goto recv_data_end;
				}
				struct list_head *sublist_l = get_audio_sublist(intercom_s);
				if(!sublist_l)
					goto recv_data_end;
				sublist *sublist_n = list_entry(sublist_l, sublist, list);
				os_memcpy(&(sublist_n->seq), intercom_s->recv_buf + offset, HEAD_RESERVE_BYTE - 2);
#if ONE_TO_MANY
				if(intercom_s->current_dev_id != intercom_s->next_dev_id) {
					os_mutex_lock(&intercom_s->send_mutex, osWaitForever);
					switch_device(intercom_s, intercom_s->next_dev_id);
					os_mutex_unlock(&intercom_s->send_mutex);
				}
				is_new = is_new_device(intercom_s, &intercom_s->remote_ret_addr);
				if(is_new) {
					os_mutex_lock(&intercom_s->send_mutex, osWaitForever);
					add_new_device(intercom_s, sublist_n->identify_num, &intercom_s->remote_ret_addr, 0);
					os_mutex_unlock(&intercom_s->send_mutex);
				}
				device = find_device(intercom_s, &intercom_s->remote_ret_addr);
				if(device) {
					device->online = 1;
					if((device->identify_num == intercom_s->g_r_identify_num) && (device->identify_num != sublist_n->identify_num)) {
						intercom_s->g_r_identify_num = sublist_n->identify_num;
					}
					device->identify_num = sublist_n->identify_num;
				}
				else {
					sublist_n->node_cnt = 0;
					del_audio_sublist(intercom_s, sublist_l);
					goto recv_data_end;
				}
				if(cur_r_identify_num[dev_id] != intercom_s->g_r_identify_num) {
					os_event_set(&intercom_s->clear_event[dev_id], clear_useList_event, NULL);
					os_event_wait(&intercom_s->clear_event[dev_id], clear_useList_finish_event, 
								  NULL, OS_EVENT_WMODE_CLEAR|OS_EVENT_WMODE_OR, osWaitForever);	
					cur_r_identify_num[dev_id] = intercom_s->g_r_identify_num;				
				}
				if(cur_r_identify_num[dev_id] != sublist_n->identify_num) {
					sublist_n->node_cnt = 0;
					del_audio_sublist(intercom_s, sublist_l);
					goto recv_data_end;					
				}
#elif INTERCOM_GROUP
				is_new = is_new_device(intercom_s, &intercom_s->remote_ret_addr);
				if(is_new) {
					add_new_device(intercom_s, sublist_n->identify_num, &intercom_s->remote_ret_addr, 0);
				}
				device = find_device(intercom_s, &intercom_s->remote_ret_addr);
				if(device) {
					device->online = 1;
					device->identify_num = sublist_n->identify_num;
					dev_id = device->dev_id;
				}
				else {
					sublist_n->node_cnt = 0;
					del_audio_sublist(intercom_s, sublist_l);
					goto recv_data_end;
				}
				if(cur_r_identify_num[dev_id] != device->identify_num) {
					os_event_set(&intercom_s->clear_event[dev_id], clear_useList_event, NULL);
					os_event_wait(&intercom_s->clear_event[dev_id], clear_useList_finish_event, 
								  NULL, OS_EVENT_WMODE_CLEAR|OS_EVENT_WMODE_OR, osWaitForever);	
					cur_r_identify_num[dev_id] = device->identify_num;
				}
#else
				os_mutex_lock(&intercom_s->send_mutex, osWaitForever);
				if(os_memcmp(&intercom_s->remote_ret_addr.sin_addr, &intercom_s->remote_trans_addr.sin_addr, sizeof(struct in_addr))) {
					os_memcpy(&intercom_s->remote_trans_addr.sin_addr, &intercom_s->remote_ret_addr.sin_addr, sizeof(struct in_addr));
				}
				os_mutex_unlock(&intercom_s->send_mutex);
				if(cur_r_identify_num[dev_id] != sublist_n->identify_num) {
					os_event_set(&intercom_s->clear_event[dev_id], clear_useList_event, NULL);
					os_event_wait(&intercom_s->clear_event[dev_id], clear_useList_finish_event, 
								  NULL, OS_EVENT_WMODE_CLEAR|OS_EVENT_WMODE_OR, osWaitForever);
					intercom_s->g_r_identify_num = sublist_n->identify_num;
					cur_r_identify_num[dev_id] = intercom_s->g_r_identify_num;
				}
#endif
#if INTERCOM_HALF_DUPLEX
				if(intercom_s->g_r_identify_num < intercom_s->g_s_identify_num) {
					send_start_flag &= ~BIT(1);
				}
#else
				send_start_flag |= BIT(1);
#endif
				recv_timeout_cnt[dev_id] = 0;
				offset += HEAD_RESERVE_BYTE;
				// if((sublist_n->type>>4) != intercom_s->recv_stream_type) {
				// 	sublist_n->node_cnt = 0;
				// 	del_audio_sublist(intercom_s, sublist_l);
				// 	goto recv_data_again;						
				// }
				code_len = sublist_n->code_len;
				node_num = (code_len % NODE_DATA_LEN)?(code_len / NODE_DATA_LEN + 1):(code_len / NODE_DATA_LEN);
				sublist_n->node_cnt = node_num;
				// os_printf("recv%d:%d\n",dev_id,sublist_n->sort);
				struct list_head *audio_n = get_audio_node(intercom_s, &(sublist_n->node_head), node_num);
				if(audio_n) {
					while(code_len > NODE_DATA_LEN) {
						node_addr = (uint8_t*)get_audio_node_addr(audio_n);
						os_memcpy(node_addr, intercom_s->recv_buf + offset, NODE_DATA_LEN);
						audio_n = audio_n->next;
						offset += NODE_DATA_LEN;
						code_len -= NODE_DATA_LEN;
					}
					if(code_len > 0) {
						node_addr = (uint8_t*)get_audio_node_addr(audio_n);
						os_memcpy(node_addr, intercom_s->recv_buf + offset, code_len);
						offset += code_len;
						code_len = 0;
					}
					lose_packet_check(intercom_s, dev_id, sublist_n);
				}
				else {
					sublist_n->node_cnt = 0;
					del_audio_sublist(intercom_s, sublist_l);
					goto recv_data_end;
				}
			}
		}
recv_data_end:
		for(uint32_t i=0; i<MAX_INTERCOM_SLAVE_DEVICE; i++) {
            recv_timeout_cnt[i]++;
            if(recv_timeout_cnt[i] > TIMEOUT_COUNT) {
                recv_timeout_cnt[i] = TIMEOUT_COUNT;
				if((play_start_flag&BIT(i)) == 1) {
                    os_event_set(&intercom_s->clear_event[i], clear_useList_event, NULL);
                    os_event_wait(&intercom_s->clear_event[i], clear_useList_finish_event, 
                                    NULL, OS_EVENT_WMODE_CLEAR|OS_EVENT_WMODE_OR, osWaitForever);	
                    os_printf("intercom device id %d stop\n",i);
				}
			}
		}
	}
	intercom_task_decrease(intercom_s);
}

static void intercom_output_framebuf(INTERCOM_STRUCT *intercom_s, uint32_t dev_id, uint32_t cached)
{	
	uint8 del_frame = 0;
	static uint8_t param_init = 0;
	uint8_t encode_data[MAX_ENCODED_LEN] = {0};
	uint16_t new_sort = 0;
	int32_t output_ret = 0;
	uint32_t timestamp = 0;
	uint32_t offset = 0;
	uint32_t code_len = 0;
    uint32_t play_speed = 100;
	static uint8_t play_speed_sta[MAX_INTERCOM_SLAVE_DEVICE] = {0};
	static uint32_t last_timestamp[MAX_INTERCOM_SLAVE_DEVICE] = {0};
	static uint32_t plc_cnt[MAX_INTERCOM_SLAVE_DEVICE] = {0};
	struct framebuff *frame_buf = NULL;
	struct list_head *sublist_l = NULL;
	sublist *sublist_n = NULL;	

    if(param_init == 0) {
        param_init = 1;
        for(uint32_t i=0; i<MAX_INTERCOM_SLAVE_DEVICE; i++) {
            play_speed_sta[i] = 2;
            last_timestamp[i] = 0;
            plc_cnt[i] = 0;
        }
    }

	del_frame = 0;
	frame_buf = fbpool_get(&intercom_s->tx_pool, 0, intercom_s->msi);
	if(frame_buf) {
		if(list_empty(&intercom_s->useList_head[dev_id]) == 0) {
			sublist_l = intercom_s->useList_head[dev_id].next;
			sublist_n = list_entry(sublist_l, sublist, list);
			new_sort = sublist_n->sort;
			// os_printf("dec%d:%d %d %d\n",dev_id, cached, intercom_s->g_current_sort[dev_id], new_sort);
			if((intercom_s->g_current_sort[dev_id]>new_sort) && ((intercom_s->g_current_sort[dev_id]-new_sort)<60000)) {
				del_audio_sublist(intercom_s, sublist_l);
			}
			if((SUBLIST_NUM-cached<4) && (((new_sort>intercom_s->g_current_sort[dev_id])&&(new_sort-intercom_s->g_current_sort[dev_id]<60000)) || 
										((intercom_s->g_current_sort[dev_id]>new_sort)&&(intercom_s->g_current_sort[dev_id]-new_sort>=60000)))) {
				intercom_s->g_current_sort[dev_id] = new_sort;
			} 
			if(intercom_s->g_current_sort[dev_id] == new_sort) {
				timestamp = sublist_n->timestamp;
				last_timestamp[dev_id] = timestamp;
				code_len = sublist_n->code_len;
				struct list_head *audio_n = sublist_n->node_head.next;
				uint8_t *addr = NULL;
				offset = 0;
				while(code_len > NODE_DATA_LEN) {
					addr = (uint8_t*)get_audio_node_addr(audio_n);
					os_memcpy(encode_data+offset, addr, NODE_DATA_LEN);
					audio_n = audio_n->next;
					offset += NODE_DATA_LEN;
					code_len -= NODE_DATA_LEN;
				}
				if(code_len > 0) {
					addr = (uint8_t*)get_audio_node_addr(audio_n);
					os_memcpy(encode_data+offset, addr, code_len);
					offset += code_len;
					code_len = 0;
				}
				frame_buf->data = (uint8_t*)INTERCOM_MALLOC(sublist_n->code_len);
				if(!frame_buf->data) {
					os_printf("intercom malloc frame_buf fail\n");
					msi_delete_fb(intercom_s->msi, frame_buf);
					del_audio_sublist(intercom_s, sublist_l);	
					frame_buf = NULL;
					intercom_s->g_current_sort[dev_id] += 1;
					return;
				}
				os_memcpy(frame_buf->data, encode_data, sublist_n->code_len);
				frame_buf->len = sublist_n->code_len;
				timestamp = sublist_n->timestamp;	
				last_timestamp[dev_id] = timestamp;	
				del_audio_sublist(intercom_s, sublist_l);	
				plc_cnt[dev_id] = 0;
			}	
			else {	
				del_frame = 1;
				if(plc_cnt[dev_id] <= 5) {
					frame_buf->len = 0;
					del_frame = 0;
				}
				timestamp = last_timestamp[dev_id] + FRAME_TIME;
				last_timestamp[dev_id] = timestamp;
				plc_cnt[dev_id]++;
				lose_total[dev_id]++;
				if(plc_cnt[dev_id] > max_lose_cnt[dev_id])
					max_lose_cnt[dev_id] = plc_cnt[dev_id];
			}
		}
		else {
			del_frame = 1;
			if(plc_cnt[dev_id] <= 5) {
				frame_buf->len = 0;
				del_frame = 0;
			}
			timestamp = last_timestamp[dev_id] + FRAME_TIME;
			last_timestamp[dev_id] = timestamp;
			plc_cnt[dev_id]++;
			lose_total[dev_id]++;
			if(plc_cnt[dev_id] > max_lose_cnt[dev_id])
				max_lose_cnt[dev_id] = plc_cnt[dev_id];			
		}
#if CHANGE_PLAY_SPEED
		if(cached < (intercom_s->play_start_wait - 2)) {
			if(play_speed_sta[dev_id] != 0) {
				play_speed = 90;
				msi_do_cmd(intercom_s->decoder_msi[dev_id], MSI_CMD_SET_SPEED, play_speed, 0);
				play_speed_sta[dev_id] = 0;
				intercom_s->time_s[dev_id].time_keep = FRAME_TIME*100/90;
			}
		}
		else if(cached > (intercom_s->play_start_wait + 2)) {
			if(play_speed_sta[dev_id] != 1) {
				play_speed = 110;
				msi_do_cmd(intercom_s->decoder_msi[dev_id], MSI_CMD_SET_SPEED, play_speed, 0);
				play_speed_sta[dev_id] = 1;
				intercom_s->time_s[dev_id].time_keep = FRAME_TIME*100/110;
			}
		}
		else if(cached == intercom_s->play_start_wait) {
			if(play_speed_sta[dev_id] != 2) {
				play_speed = 100;
				msi_do_cmd(intercom_s->decoder_msi[dev_id], MSI_CMD_SET_SPEED, play_speed, 0);
				play_speed_sta[dev_id] = 2;
				intercom_s->time_s[dev_id].time_keep = FRAME_TIME;
			}
		}	
#endif
		if(del_frame) {
			msi_delete_fb(intercom_s->msi, frame_buf);
		}
		else {
			frame_buf->time = timestamp;
			frame_buf->mtype = MEDIA_DATA_AUDIO;	
			frame_buf->stype = AUDIO_CODEC_OPUS;
			frame_buf->codec_info = &(intercom_s->codec_info);
			output_ret = msi_recv_fb(intercom_s->decoder_msi[dev_id], frame_buf);
			if(output_ret == RET_OK) {
				fb_put(frame_buf);
			}
			// os_printf("out%d:%d\n",dev_id, intercom_s->g_current_sort[dev_id]);
		}
		intercom_s->g_current_sort[dev_id] += 1;
	}
	if(os_jiffies()-last_statistical_time[dev_id] > 5000) {
#if LOSE_STATISTICAL
		os_printf("\r\naudio info %d :total loss:%d, max loss:%d\r\n",dev_id, lose_total[dev_id], max_lose_cnt[dev_id]);
#endif
		if((lose_total[dev_id] > 50) || (lose_total[dev_id]>25 && max_lose_cnt[dev_id]>3)) {
			intercom_s->loss_state[dev_id] = serious_loss;
		}
		else if((lose_total[dev_id] < 20) && (max_lose_cnt[dev_id]<=2) && (intercom_s->loss_state[dev_id] == serious_loss)) {
			intercom_s->loss_state[dev_id] = mild_loss;
		}
		lose_total[dev_id] = 0;
		max_lose_cnt[dev_id] = 0;
		last_statistical_time[dev_id] = os_jiffies();
	}
}

static void clear_useList_func(INTERCOM_STRUCT *intercom_s, uint32_t dev_id)
{
	sublist *sublist_n_pos;
	sublist *npos;
	os_mutex_lock(&intercom_s->list_mutex, osWaitForever);
	list_for_each_entry_safe(sublist_n_pos, npos, &intercom_s->useList_head[dev_id], list) {
		if(sublist_n_pos->node_cnt)
			del_audio_node(intercom_s, &(sublist_n_pos->node_head));
		list_move_tail(&(sublist_n_pos->list), &intercom_s->sublist_head);
	}
	os_mutex_unlock(&intercom_s->list_mutex);
	intercom_s->g_numofcached[dev_id] = 0;
	play_start_flag &= ~BIT(dev_id);
}

static void update_useList_func(INTERCOM_STRUCT *intercom_s, uint32_t dev_id)
{
	sublist *sublist_n_pos;
	sublist *npos;	
	os_mutex_lock(&intercom_s->list_mutex, osWaitForever);
	list_for_each_entry_safe(sublist_n_pos, npos, &intercom_s->checkList_head[dev_id], list) {
		insert_into_useList(intercom_s, dev_id, &(sublist_n_pos->list));
	}
	os_mutex_unlock(&intercom_s->list_mutex);
}

static void intercom_output_task(void *d)
{
	int16_t mixer_list_cnt = 0;
	uint32_t useList_clear = 0;
	struct msi *mixer_msi[MAX_INTERCOM_SLAVE_DEVICE] = {0};
	struct list_head *sublist_l = NULL;
	sublist *sublist_n = NULL;
	INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)d;
	struct os_semaphore *sem = &intercom_s->output_sema;

	intercom_task_increase(intercom_s);
	for(uint32_t i=0; i<MAX_INTERCOM_SLAVE_DEVICE; i++) {
		intercom_s->decoder_msi[i] = msi_find2(NULL, MEDIA_DATA_AUDIO << 8 | AUDIO_CODEC_OPUS, 1, (void*)(&(intercom_s->codec_info)));
		if(!intercom_s->decoder_msi[i]) {
			os_printf("intercom audio decoder init fail!\n");
			intercom_task_decrease(intercom_s);
			return;
		}
		mixer_msi[i] = msi_find2(MIXER_MSI, 0, 0, 0);
		msi_add_output(intercom_s->decoder_msi[i], NULL, mixer_msi[i], NULL);
		msi_do_cmd(intercom_s->decoder_msi[i], MSI_CMD_START, 0, 0);

		intercom_s->time_s[i].time_keep = FRAME_TIME;
		intercom_s->time_s[i].last_time = os_jiffies();
		intercom_s->time_s[i].time_diff = 0;
	}
	
	while(1) {
		if(intercom_s->run_state == intercom_stop) {
			if(intercom_s->run_task == 1) {     //需要等到其他线程退出才退出此线程，因为recv线程可能会通知此线程清除缓存，要确保recv线程退出了再退出此线程。
				break;	
			}
		}
		for(uint32_t i=0; i<MAX_INTERCOM_SLAVE_DEVICE; i++) {
			update_useList_func(intercom_s, i);
			os_event_wait(&intercom_s->clear_event[i], clear_useList_event, &useList_clear, OS_EVENT_WMODE_CLEAR|OS_EVENT_WMODE_OR, 0);
			if(useList_clear & clear_useList_event) {
				useList_clear = 0;
				clear_useList_func(intercom_s, i);
				os_event_set(&intercom_s->clear_event[i], clear_useList_finish_event, NULL);
				continue;
			}
		}
		if(os_sema_down(sem, osWaitForever) == 1) {	
			for(uint32_t i=0; i<MAX_INTERCOM_SLAVE_DEVICE; i++) {
				if(os_jiffies()-intercom_s->time_s[i].last_time >= intercom_s->time_s[i].time_keep + intercom_s->time_s[i].time_diff) {
					intercom_s->time_s[i].last_time += intercom_s->time_s[i].time_keep + intercom_s->time_s[i].time_diff;
				}
				else {
					continue;
				}
				intercom_s->g_numofcached[i] = get_audio_sublist_count(intercom_s, &intercom_s->useList_head[i]);
#if INTERCOM_HALF_DUPLEX
				if(intercom_s->g_numofcached[i] == 0) {
					send_start_flag |= BIT(1);
				}
				if((send_start_flag == 0x03) && (intercom_s->g_r_identify_num > intercom_s->g_s_identify_num) && (intercom_s->g_numofcached > 0)) {
					clear_useList_func(intercom_s);
					intercom_s->g_numofcached[i] = 0;
				}
#endif
				if(((play_start_flag&BIT(i)) == 0) && (intercom_s->g_numofcached[i] > intercom_s->play_start_wait)) {
					play_start_flag |= BIT(i);
					sublist_l = intercom_s->useList_head[i].next;
					sublist_n = list_entry(sublist_l, sublist, list);
					intercom_s->g_current_sort[i] = sublist_n->sort;
					lose_total[i] = 0;
					max_lose_cnt[i] = 0;
					os_printf("intercom dev_id %d start\n", i);			
				}
				if(play_start_flag&BIT(i)) {
					intercom_output_framebuf(intercom_s, i, intercom_s->g_numofcached[i]);   
					mixer_list_cnt = fbq_count(&mixer_msi[i]->fbQ);
					if(mixer_list_cnt <= 1) {
						intercom_s->time_s[i].time_diff = -1;
					}
					else if(mixer_list_cnt > 2) {
						intercom_s->time_s[i].time_diff = 1;
					}
					else {
						intercom_s->time_s[i].time_diff = 0;
					}
				}
			}
		}
	}
	intercom_task_decrease(intercom_s);
}

static void intercom_handel_task(void *d)
{
	uint32_t ipaddr;  
    int32_t err = -1; 
    socklen_t addrlen = sizeof(struct sockaddr_in);
	struct timeval timeout_t;
    int32_t trans_time_out = FRAME_TIME * 2;	
	int32_t ret_time_out = 1;
	INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)d;
    ip_addr_t ip;
	int32_t tos = 0xE0;   //28

	intercom_task_increase(intercom_s);
	if(sys_cfgs.wifi_mode == WIFI_MODE_STA) {
		do{
			if(sys_cfgs.wifi_mode == WIFI_MODE_AP)
				break;
			if(intercom_s->run_state == intercom_stop) {
				intercom_task_decrease(intercom_s);
				return;
			}
            ip = lwip_netif_get_ip2("w0");
			ipaddr = ip_addr_get_ip4_u32(&ip);
			os_sleep_ms(100);
		}while((ipaddr&0xff000000) == 0x1000000);		
	}

	intercom_s->codec_info.sample_rate = CODEC_SAMPLERATE;
	intercom_s->codec_info.channels = 1;
	intercom_s->play_start_wait = 5;

    intercom_s->local_trans_fd = socket(AF_INET,SOCK_DGRAM, 0);
    intercom_s->local_ret_fd = socket(AF_INET,SOCK_DGRAM, 0);
	if((intercom_s->local_trans_fd==-1) || (intercom_s->local_ret_fd==-1)) {
		os_printf("create intercom socket fail!\n");
		goto intercom_handel_task_err;
	}

	timeout_t.tv_sec = 0;
	timeout_t.tv_usec = trans_time_out * 1000;
	if(setsockopt(intercom_s->local_trans_fd,SOL_SOCKET,SO_RCVTIMEO,&timeout_t,sizeof(struct timeval)) == -1) {
		os_printf("intercom setsockopt fail!\n");
		goto intercom_handel_task_err;
	}

	timeout_t.tv_sec = 0;
	timeout_t.tv_usec = ret_time_out * 1000;
	if(setsockopt(intercom_s->local_ret_fd,SOL_SOCKET,SO_RCVTIMEO,&timeout_t,sizeof(struct timeval)) == -1) {
		os_printf("intercom setsockopt fail!\n");
		goto intercom_handel_task_err;
	}

	if(setsockopt(intercom_s->local_trans_fd,IPPROTO_IP,IP_TOS,&tos,sizeof(int32_t)) == -1) {
		os_printf("intercom setsockopt fail!\n");
		goto intercom_handel_task_err;
	}

	if(setsockopt(intercom_s->local_ret_fd,IPPROTO_IP,IP_TOS,&tos,sizeof(int32_t)) == -1) {
		os_printf("intercom setsockopt fail!\n");
		goto intercom_handel_task_err;
	}

	for(uint8_t i=0; i<2; i++) {
		(*((&intercom_s->local_trans_addr)+i)).sin_family = AF_INET;
		(*((&intercom_s->local_trans_addr)+i)).sin_port = htons(INTERCOM_PORT + i);
		(*((&intercom_s->local_trans_addr)+i)).sin_addr.s_addr = 0;	
		
		err = bind(*((&intercom_s->local_trans_fd)+i), (struct sockaddr *)((&intercom_s->local_trans_addr)+i), addrlen);
		if(err == -1) {
			os_printf("intercom socket bind fail!\n");
			goto intercom_handel_task_err;
		}					
	}

	for(uint8_t i=0; i<2; i++) {
		(*((&intercom_s->remote_trans_addr)+i)).sin_family = AF_INET;
		(*((&intercom_s->remote_trans_addr)+i)).sin_port = htons(INTERCOM_PORT + i);
#if INTERCOM_GROUP
		(*((&intercom_s->remote_trans_addr)+i)).sin_addr.s_addr = inet_addr("255.255.255.255");
#else
		if(sys_cfgs.wifi_mode == WIFI_MODE_STA) {
		    (*((&intercom_s->remote_trans_addr)+i)).sin_addr.s_addr = inet_addr("192.168.1.1");
		}
#endif
	}
	if(sys_cfgs.wifi_mode == WIFI_MODE_AP) {
		send_start_flag &= ~BIT(1);  
	}

	err = intercom_room_init(intercom_s);
	if(err == RET_ERR) {
		goto intercom_handel_task_err;			
	}

	for(uint32_t i=0; i<MAX_INTERCOM_SLAVE_DEVICE; i++) {	
		err |= os_event_init(&intercom_s->clear_event[i]);
	}
	err |= os_mutex_init(&intercom_s->list_mutex);
	err |= os_mutex_init(&intercom_s->send_mutex);
	err |= os_sema_init(&intercom_s->output_sema, 0);
	err |= os_timer_init(&intercom_s->ctl_timer, (os_timer_func_t)output_sema_up, OS_TIMER_MODE_PERIODIC, intercom_s);

	if(err != RET_OK) {
		os_printf("intercom malloc synth fail!\n");
		goto intercom_handel_task_err;
	}

	intercom_s->recv_task_hdl = os_task_create("intercom_recv_task", intercom_recv_task, (void*)intercom_s, OS_TASK_PRIORITY_ABOVE_NORMAL-1, 0, NULL, 1536);
	intercom_s->send_task_hdl = os_task_create("intercom_send_task", intercom_send_task, (void*)intercom_s, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 1024);
	intercom_s->output_task_hdl = os_task_create("intercom_output_task", intercom_output_task, (void*)intercom_s, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 1024);
	if(!intercom_s->recv_task_hdl || !intercom_s->send_task_hdl || !intercom_s->output_task_hdl) {
		os_printf("intercom create task fail!\n");
		goto intercom_handel_task_err;
	}

	os_timer_start(&intercom_s->ctl_timer, 2);
	intercom_task_decrease(intercom_s);
	return;	
intercom_handel_task_err:	
	intercom_task_decrease(intercom_s);
	intercom_deinit();
	return;
}

static int32_t intercom_msi_action(struct msi *msi, uint32_t cmd_id, uint32_t param1, uint32_t param2)
{
    int32_t ret = RET_OK;
	INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)msi->priv;
    switch(cmd_id) {
        case MSI_CMD_TRANS_FB:
        {
            ret = RET_OK+1;
            struct framebuff *frame_buf = (struct framebuff *)param1;
            if(frame_buf->mtype == MEDIA_DATA_AUDIO) {
                ret = RET_OK;
            } 
            break;
        }            
        case MSI_CMD_FREE_FB:
        {
            if(intercom_s) {
                struct framebuff *frame_buf = (struct framebuff *)param1;
                if(frame_buf->data) {
                    INTERCOM_FREE(frame_buf->data);
                    frame_buf->data = NULL;
                }
            }
            ret = RET_OK+1;
            break; 
        }        
		case MSI_CMD_POST_DESTROY:
        {
            if(intercom_s) {
                for(uint32_t i=0; i<MAX_INTERCOM_TXBUF; i++) {
                    struct framebuff *frame_buf = (intercom_s->tx_pool.pool)+i;
                    if(frame_buf->data) {
                        INTERCOM_FREE(frame_buf->data);
                        frame_buf->data = NULL;
                    }
                }
                fbpool_destroy(&intercom_s->tx_pool);
				INTERCOM_FREE(intercom_s);
            }
            ret = RET_OK;
            break; 
        }         
        default:
            break;    
    }
    return ret;
}

struct msi *intercom_init(void)
{
	uint8_t msi_isnew = 0;
	struct msi *msi = NULL;

	msi = msi_new("SR_INTERCOM", MAX_INTERCOM_RXBUF, &msi_isnew);
	if(msi == NULL) {
		os_printf("create intercom msi fail\n");
		return NULL;	
	}
	if(msi && !msi_isnew) {
		os_printf("intercom has been created\n");
		return NULL;
	}
	INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)INTERCOM_ZALLOC(sizeof(INTERCOM_STRUCT));
	if(intercom_s == NULL) {
		msi_destroy(msi);
		os_printf("malloc intercom_s fail!\n");
		return NULL;
	}
	intercom_s->local_trans_fd = -1;
	intercom_s->local_ret_fd = -1;
	intercom_s->msi = msi;
    intercom_s->msi->enable = 1;
    intercom_s->msi->action = intercom_msi_action;
    fbpool_init(&intercom_s->tx_pool, MAX_INTERCOM_TXBUF, NULL, NULL);
    intercom_s->msi->priv = intercom_s;
	if(os_mutex_init(&intercom_s->state_mutex) != RET_OK)
		goto intercom_init_err;
	intercom_task_state_init(intercom_s);
	intercom_s->init_task_hdl = os_task_create("intercom_handel_task", intercom_handel_task, (void*)intercom_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
	if(!intercom_s->init_task_hdl)
		goto intercom_init_err;
	global_intercom_msi = intercom_s->msi;
	return intercom_s->msi;
intercom_init_err:
	intercom_deinit();
	os_printf("intercom init fail\n");
	return NULL;
}

void intercom_deinit(void)
{
	global_intercom_msi = NULL;
	struct msi *msi = msi_find("SR_INTERCOM", 1);
	if(msi) {
		msi_put(msi);
		INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)(msi->priv);
		intercom_s->run_state = intercom_stop;	
		while(intercom_task_state(intercom_s) > 0)
			os_sleep_ms(1);
		clear_all_device(intercom_s);
		if(intercom_s->encoder_msi) {
			msi_put(intercom_s->encoder_msi);		
		}
		for(uint32_t i=0; i<MAX_INTERCOM_SLAVE_DEVICE; i++) {
			if(intercom_s->decoder_msi[i]) {
				msi_do_cmd(intercom_s->decoder_msi[i], MSI_CMD_STOP, 0, 0);
				msi_put(intercom_s->decoder_msi[i]);
			}
		}
		if(intercom_s->ctl_timer.hdl) {
			os_timer_stop(&intercom_s->ctl_timer);
			os_sleep_ms(50);
			os_timer_del(&intercom_s->ctl_timer);
		}
		if(intercom_s->output_sema.hdl)
			os_sema_del(&intercom_s->output_sema);
		if(intercom_s->state_mutex.hdl)
			os_mutex_del(&intercom_s->state_mutex);
		if(intercom_s->list_mutex.hdl)
			os_mutex_del(&intercom_s->list_mutex);
		if(intercom_s->send_mutex.hdl)
			os_mutex_del(&intercom_s->send_mutex);
		for(uint32_t i=0; i<MAX_INTERCOM_SLAVE_DEVICE; i++) {
			if(intercom_s->clear_event[i].hdl)
				os_event_del(&intercom_s->clear_event[i]);
		}
		intercom_close_socket(intercom_s);
		intercom_room_free(intercom_s);
		msi_destroy(intercom_s->msi);
	}
}

void intercom_send_enable(uint8_t state)
{
	if(state == 1) {
		send_start_flag |= BIT(0);
	}
	else if(state == 0) {
		send_start_flag &= ~BIT(0);
	}
	if(global_intercom_msi) {
		INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)(global_intercom_msi->priv);
		if(state == 1) {
			intercom_s->g_s_identify_num = 0;
			os_random_bytes((uint8_t*)(&intercom_s->g_s_identify_num), 4);
		}		
	}
}

void intercom_recv_enable(uint8_t state)
{
	if(state == 1) {
		play_start_flag |= BIT(MAX_INTERCOM_SLAVE_DEVICE);
	}
	else if(state == 0) {
		play_start_flag &= ~BIT(MAX_INTERCOM_SLAVE_DEVICE);
	}
}

void intercom_encode_pause(uint8_t state)
{
	uint32_t encoder_run = 0;
	if(global_intercom_msi) {
		INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)(global_intercom_msi->priv);
		if(intercom_s->encoder_msi == NULL) {
			return;
		}
		if(state == 1) {
			msi_do_cmd(intercom_s->encoder_msi, MSI_CMD_GET_RUNNING, (uint32)(&encoder_run), 0);
			if(encoder_run == 1) {
				msi_do_cmd(intercom_s->encoder_msi, MSI_CMD_PAUSE, 0, 0);
			}
		}
		else if(state == 0) {
			intercom_s->g_s_identify_num = 0;
			os_random_bytes((uint8_t*)(&intercom_s->g_s_identify_num), 4);
			msi_do_cmd(intercom_s->encoder_msi, MSI_CMD_GET_RUNNING, (uint32)(&encoder_run), 0);
			if(encoder_run == 0) {
				msi_do_cmd(intercom_s->encoder_msi, MSI_CMD_START, 0, 0);	
			}
		}
	}
}

void intercom_decode_pause(uint8_t state, uint8_t dev_id)
{
	if(global_intercom_msi) {
		INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)(global_intercom_msi->priv);
		if(intercom_s->decoder_msi[dev_id] == NULL) {
			return;
		}
		if(state == 1) {
			msi_do_cmd(intercom_s->decoder_msi[dev_id], MSI_CMD_PAUSE, 0, 0);
		}
		else if(state == 0)
			msi_do_cmd(intercom_s->decoder_msi[dev_id], MSI_CMD_START, 0, 0);
	}
}

uint32_t intercom_ctrl_key(struct key_callback_list_s *callback_list,uint32_t keyvalue,uint32_t extern_value)
{
	if(global_intercom_msi) {
		INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)(global_intercom_msi->priv);
#ifdef SYS_APP_WALKIE_TALKIE
		if( (keyvalue>>8) != AD_SPEACH)
			return 0;
#else
		if( (keyvalue>>8) != AD_DOWN)
			return 0;
#endif

		uint32 key_val = (keyvalue & 0xff);
		if((send_start_flag & BIT(1)) == 0) {
			if(send_start_flag & BIT(0)) {
				intercom_encode_pause(1);
				send_start_flag &= ~BIT(0);
			}	
			return 0;	
		}
		if((key_val == KEY_EVENT_DOWN) || (key_val == KEY_EVENT_LDOWN) || (key_val == KEY_EVENT_REPEAT)) {
			if((send_start_flag & BIT(0)) == 0) {
				intercom_encode_pause(0);
				intercom_s->g_s_identify_num = 0;
				os_random_bytes((uint8_t*)(&intercom_s->g_s_identify_num), 4);
				send_start_flag |= BIT(0);
			}
		}
		else if((key_val == KEY_EVENT_SUP) || (key_val == KEY_EVENT_LUP)) {
			if(send_start_flag & BIT(0)) {
				intercom_encode_pause(1);
				send_start_flag &= ~BIT(0);
			}
		}
	}
	return 0;
}

void intercom_switch_device(uint8_t dev_id)
{
	if(global_intercom_msi) {
		INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)(global_intercom_msi->priv);
		intercom_s->next_dev_id = dev_id;
	}	
}

int32_t atcmd_intercom_switch_device(const char *cmd, char *argv[], uint32 argc)
{
	uint8_t dev_id = 0;
	if(argv[0]) {
		dev_id = os_atoi(argv[0]);
		intercom_switch_device(dev_id);
		return RET_OK;
	}
	return RET_ERR;
}