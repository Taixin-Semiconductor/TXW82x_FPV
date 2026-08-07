#include "basic_include.h"
#include "csi_kernel.h"
#include "syscfg.h"
#include "lwip/sockets.h"
#include "netif/ethernetif.h"
#include "intercom.h"
#include "audio_code_ctrl.h"
#include "audio_msi/audio_adc.h"
#include "audio_msi/audio_dac.h"
#include "autpc_msi/autpc_msi.h"
#include "magic_voice/magic_voice.h"

#define MAX_INTERCOM_RXBUF      14
#define MAX_INTERCOM_TXBUF      4

#define MAGIC_VOICE_EN          0
#define CHANGE_PLAY_SPEED       1
#define PLC_PROCESS             1

#define FSTYPE_AUDIO_DECODER    FSTYPE_AUDIO_OPUS_DECODER

#define FRAME_TIME              20

#define INTERCOM_PORT          	5008

#define MAX_ENCODED_LEN        	280
#define ENCODED_BUF_NUM        	15
#define ENCODED_RINGBUF_LEN    	1500

#define SUBLIST_NUM            	40
#define SOFTBUF_LEN            	4000
#define NODE_DATA_LEN          	60

#define HEAD_RESERVE_BYTE       18
#define NUM_OF_FRAME            2

#define LOSE_STATISTICAL        0

#define ONE_TO_MANY             0
#define TIMEOUT_COUNT           2000

static uint16_t g_play_sort = 0;
static uint16_t g_current_sort = 1;
static uint32_t g_numofcached = 0;
static uint32_t g_s_identify_num = 0;
static uint32_t g_r_identify_num = 0;
static uint32_t cur_r_identify_num = 0;

static uint32_t play_start_wait = 10;

static uint8_t send_start_flag = 1;
static uint8_t play_start_flag = 2;   //BIT(0)是否播放，BIT(1)是否接收          

static uint32_t now_dac_filter_type = 0;

static uint8_t connected_num = 0;

static uint32_t lose_total = 0;
static uint32_t max_lose_cnt = 0;
static uint32_t slow_speed_cnt = 0;
static uint32_t fast_speed_cnt = 0;

enum {
    clear_useList_event = BIT(0),
    clear_useList_finish_event = BIT(1),
};

void intercom_deinit(struct msi *msi);

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

static void intercom_close_socket(INTERCOM_STRUCT *intercom_s)
{
	close(intercom_s->local_trans_fd);
	close(intercom_s->local_ret_fd);
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

	INIT_LIST_HEAD((struct list_head *)&intercom_s->checkList_head);
	INIT_LIST_HEAD((struct list_head *)&intercom_s->useList_head);
	INIT_LIST_HEAD((struct list_head *)&intercom_s->nodeList_head);
	INIT_LIST_HEAD((struct list_head *)&intercom_s->sublist_head);
	INIT_LIST_HEAD((struct list_head *)&intercom_s->ringbuf_manage_empty);
	INIT_LIST_HEAD((struct list_head *)&intercom_s->ringbuf_manage_used);
	INIT_LIST_HEAD((struct list_head *)&intercom_s->device_head);

	audio_node *audio_node_src = (audio_node*)INTERCOM_ZALLOC(sizeof(audio_node) * (SOFTBUF_LEN / NODE_DATA_LEN));
	if(!audio_node_src) {
		os_printf("intercom malloc audio_node_src fail!\n");
		return RET_ERR;		
	}
	for(uint32_t i=0; i<(SOFTBUF_LEN/NODE_DATA_LEN); i++) {
		audio_node_src[i].buf_addr = (uint8_t*)(intercom_s->sort_buf+(NODE_DATA_LEN * i));
		list_add_tail((struct list_head *)&(audio_node_src[i].list),(struct list_head *)&intercom_s->nodeList_head); 
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
		list_add_tail((struct list_head *)&sublist_src[i].list, (struct list_head *)&intercom_s->sublist_head); 
	}
	intercom_s->sublist_src = sublist_src;

	ringbuf_manage *ringbuf_manage_src = (ringbuf_manage*)INTERCOM_ZALLOC(sizeof(ringbuf_manage) * SUBLIST_NUM);
	if(!ringbuf_manage_src) {
		os_printf("intercom malloc ringbuf_manage_src fail!\n");
		return RET_ERR;		
	}
	for(uint32_t i=0; i<ENCODED_BUF_NUM; i++) {
		list_add_tail((struct list_head *)&ringbuf_manage_src[i].list,(struct list_head *)&intercom_s->ringbuf_manage_empty); 
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
	ringbuf_mana = list_entry((struct list_head *)list,ringbuf_manage,list);
	return ringbuf_mana->buf_addr;
}
static uint32_t get_ringbuf_manage_datalen(struct list_head *list)
{
	ringbuf_manage *ringbuf_mana;
	ringbuf_mana = list_entry((struct list_head *)list,ringbuf_manage,list);
	return (uint32_t)ringbuf_mana->data_len;
}
static int32_t get_ringbuf_manage_count(INTERCOM_STRUCT *intercom_s)
{
	int count = 0;
	struct list_head *list_n = (struct list_head *)intercom_s->manage_cur_pop;
	struct list_head *head = (struct list_head *)intercom_s->manage_cur_push;
	while(list_n != head) {
		list_n = list_n->next;
		count++;
		if(count > ENCODED_BUF_NUM)
		{
			count = 0;
			break;
		}
	}
	return count;			
}
static struct list_head *get_ringbuf_manage(INTERCOM_STRUCT *intercom_s, uint8_t grab)
{
	if(list_empty_careful((const struct list_head *)&intercom_s->ringbuf_manage_empty)) {
		if(grab) {
			list_move_tail((struct list_head *)intercom_s->ringbuf_manage_used.next,
						   (struct list_head *)&intercom_s->ringbuf_manage_used);
		}
		else {		
			return 0;
		}
	}
	else {
		list_move_tail((struct list_head *)intercom_s->ringbuf_manage_empty.next,
					   (struct list_head *)&intercom_s->ringbuf_manage_used);
	}
	return intercom_s->ringbuf_manage_used.prev;				
}
static void del_ringbuf_manage(INTERCOM_STRUCT *intercom_s)
{
	list_move_tail((struct list_head *)intercom_s->ringbuf_manage_used.next,
						(struct list_head*)&intercom_s->ringbuf_manage_empty);
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
	sendto(intercom_s->local_trans_fd, intercom_s->send_buf, len, 0,
		(struct sockaddr*)&(intercom_s->remote_trans_addr), sizeof(intercom_s->remote_trans_addr));
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
	struct sockaddr_in remote_ret_addr;

	os_mutex_lock(&intercom_s->send_mutex, osWaitForever);
	recv_len = recvfrom(intercom_s->local_ret_fd, &lose_packet, 4, 0, 
			(struct sockaddr*)&(remote_ret_addr), &addrlen);
	if(os_memcmp(&remote_ret_addr.sin_addr, &intercom_s->remote_ret_addr.sin_addr, sizeof(struct in_addr)) != 0)
		goto intercom_retransfer_check_end;
	if(!send_start_flag)
		goto intercom_retransfer_check_end;
	if(recv_len > 0) {
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
	ringbuf_manage *ringbuf_mana_n = NULL;
	struct list_head *ringbuf_mana_l = NULL;
	struct list_head *manage_l_del = NULL;
	ringbuf_manage *manage_n_del = NULL;

	ringbuf_mana_l = get_ringbuf_manage(intercom_s, 0);
	if(!ringbuf_mana_l) {
		ringbuf_mana_l = intercom_s->ringbuf_manage_used.next;
		ringbuf_mana_n = list_entry(ringbuf_mana_l, ringbuf_manage, list);
		ringbuf_move_readptr(intercom_s->encoded_ringbuf, size);
		if(intercom_s->manage_cur_pop == intercom_s->ringbuf_manage_used.next)
			intercom_s->manage_cur_pop = intercom_s->manage_cur_pop->next;
		ringbuf_mana_l = get_ringbuf_manage(intercom_s, 1);
	}
	intercom_s->manage_cur_push = ringbuf_mana_l;
	ringbuf_mana_n = list_entry(ringbuf_mana_l, ringbuf_manage, list);
	ringbuf_mana_n->buf_addr = (uint8_t*)intercom_s->encoded_ringbuf->data + 
											intercom_s->encoded_ringbuf->rear;
	ringbuf_mana_n->data_len = size;
	while(ringbuf_write_available(intercom_s->encoded_ringbuf) < size) {
		manage_l_del = intercom_s->ringbuf_manage_used.next;
		manage_n_del = list_entry(manage_l_del, ringbuf_manage, list);
		ringbuf_move_readptr(intercom_s->encoded_ringbuf, size);
		if(intercom_s->manage_cur_pop == intercom_s->ringbuf_manage_used.next)
			intercom_s->manage_cur_pop = intercom_s->manage_cur_pop->next;
		del_ringbuf_manage(intercom_s);
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
	os_mutex_unlock(&intercom_s->send_mutex);			
}

static void intercom_send_task(void *d)
{
	uint8_t send_sequence = 0;
	uint8_t encoded_buf[MAX_ENCODED_LEN] = {0};
	uint8_t *data = NULL;
	uint16_t send_sort = 0;
	uint32_t data_len = 0;
	uint32_t timestamp = 0;
	INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)d;
	struct framebuff *frame_buf = NULL;

	intercom_task_increase(intercom_s);
	g_s_identify_num = 0;
	os_random_bytes((uint8_t*)(&g_s_identify_num), 4);
	os_printf("\n**********intercom ID:%d***********\n",g_s_identify_num);

	intercom_s->encoder_msi = audio_encode_init(AUDIO_ENCODER, AUADC_SAMPLERATE);
	if(!intercom_s->encoder_msi) {
		os_printf("intercom audio encoder init fail!\n");
		intercom_task_decrease(intercom_s);
		return;
	}
#if MAGIC_VOICE_EN
	auadc_msi_add_output(intercom_s->magic_voice_msi->name);
	magic_voice_msi_add_output(audio_code_msi_name(AUDIO_ENCODER));
#else
	auadc_msi_add_output(audio_code_msi_name(AUDIO_ENCODER));
#endif
	audio_code_add_output(AUDIO_ENCODER, intercom_s->msi->name);
	intercom_s->send_stream_type = intercom_live_audio;
	while(1) {
		if(intercom_s->run_state == intercom_stop) 
			break;
		intercom_retransfer_check(intercom_s);
		if(send_start_flag) {
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
						encoded_buf[1] = intercom_live_audio;
					else if(intercom_s->send_stream_type == intercom_playback_audio)
						encoded_buf[1] = intercom_playback_audio;
				}
				else
					encoded_buf[1] = 0;
				*((uint16_t*)(encoded_buf+2)) = send_sort;
				*((uint32_t*)(encoded_buf+4)) = timestamp;
				*((uint32_t*)(encoded_buf+8)) = data_len;
				*((uint32_t*)(encoded_buf+12)) = g_s_identify_num;
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
	struct list_head *list_n = (struct list_head *)head;
	while(list_n->next != head) {
		list_n = list_n->next;
		count++;
		if(count > (SOFTBUF_LEN / NODE_DATA_LEN)) {
			count = 0;
			break;
		}
	}
	return count;			
}
static void *get_audio_node_addr(struct list_head *list)
{
	audio_node *audio_n;
	audio_n = list_entry((struct list_head *)list,audio_node,list);
	return audio_n->buf_addr;
}
static struct list_head *get_audio_node(INTERCOM_STRUCT *intercom_s, struct list_head *head, uint32_t node_num)
{
	int ret = os_mutex_lock(&intercom_s->list_mutex, osWaitForever);
	if(ret < 0)
		return 0;
	if(get_audio_node_count((struct list_head *)&intercom_s->nodeList_head) < node_num) {
		os_mutex_unlock(&intercom_s->list_mutex);
		os_printf("nodeList_head empty\n");
		return 0;
	}

	for(uint32_t i=0; i<node_num; i++)
		list_move_tail((struct list_head *)intercom_s->nodeList_head.next, (struct list_head *)head);
	os_mutex_unlock(&intercom_s->list_mutex);
	return head->next;			
}
static void del_audio_node(INTERCOM_STRUCT *intercom_s, struct list_head *del)
{
	del->next->prev = intercom_s->nodeList_head.prev;
	del->prev->next = (struct list_head*)(&(intercom_s->nodeList_head));
	intercom_s->nodeList_head.prev->next = del->next;
	intercom_s->nodeList_head.prev = del->prev;
	del->next = (struct list_head *)del;
	del->prev = (struct list_head *)del;
}

static int32_t get_audio_sublist_count(INTERCOM_STRUCT *intercom_s, struct list_head *head)
{
	int count = 0;
	struct list_head *list_n = (struct list_head *)head;
	int ret = os_mutex_lock(&intercom_s->list_mutex, osWaitForever);
	if(ret < 0) {
		return 0;
	}
	while(list_n->next != head) {
		list_n = list_n->next;
		count++;
		if(count > SUBLIST_NUM) {
			count = 0;
			break;
		}
	}
	os_mutex_unlock(&intercom_s->list_mutex);
	return count;			
}
static struct list_head *get_audio_sublist(INTERCOM_STRUCT *intercom_s, struct list_head *head)
{
	int ret = os_mutex_lock(&intercom_s->list_mutex, osWaitForever);
	if(ret < 0)
		return 0;
	if(list_empty_careful((const struct list_head *)&intercom_s->sublist_head)) {
		os_mutex_unlock(&intercom_s->list_mutex);
		os_printf("sublist_head empty\n");
		return 0;
	}
	list_move_tail((struct list_head *)intercom_s->sublist_head.next,(struct list_head *)head);
	os_mutex_unlock(&intercom_s->list_mutex);
	return head->prev;			
}
static void del_audio_sublist(INTERCOM_STRUCT *intercom_s, struct list_head *del)
{
	int32_t ret = os_mutex_lock(&intercom_s->list_mutex, osWaitForever);
	if(ret < 0)
		return;		
	sublist *sublist_n = list_entry((struct list_head *)del,sublist,list);
	if(sublist_n->node_cnt)
		del_audio_node(intercom_s, &(sublist_n->node_head));
	list_move_tail((struct list_head *)del,(struct list_head*)&intercom_s->sublist_head);
	os_mutex_unlock(&intercom_s->list_mutex);	
}
static void insert_into_useList(INTERCOM_STRUCT *intercom_s, struct list_head *del, volatile struct list_head *head)
{
	sublist *sublist_n;
	sublist *sublist_n_pos;
	sublist *npos;
	sublist_n = list_entry((struct list_head *)del,sublist,list);
	int32_t prev_sort = 0;
	uint16_t new_sort = 0;
	uint16_t next_sort = 0;
	int32_t ret;

	new_sort = sublist_n->sort;
	prev_sort = (play_start_flag&BIT(0))?g_play_sort:-1;
	if((prev_sort >= new_sort) && ((prev_sort - new_sort) < 60000)) {
		del_audio_sublist(intercom_s, del);
		return;					
	}
	// os_printf("insert:%d\n",new_sort);
	ret = os_mutex_lock(&intercom_s->list_mutex, osWaitForever);
	if(ret < 0)
		return;
	if(list_empty_careful((struct list_head *)head)) {	
		list_move((struct list_head *)del, (struct list_head *)head);
		goto insert_into_useList_end;	
	}
	list_for_each_entry_safe(sublist_n_pos, npos, (struct list_head *)head, list) {
		next_sort = sublist_n_pos->sort;
		if(new_sort == next_sort) {
			if(sublist_n->node_cnt)
				del_audio_node(intercom_s, &(sublist_n->node_head));
			list_move_tail((struct list_head *)del,(struct list_head*)&intercom_s->sublist_head);
			goto insert_into_useList_end;
		}
		else if( (prev_sort < new_sort ) && (new_sort < next_sort) && ((next_sort-new_sort) < 60000) ) {
			list_move((struct list_head *)del, (struct list_head *)(sublist_n_pos->list.prev));	
			goto insert_into_useList_end;
		}
		else if( (prev_sort>60000) && ((prev_sort - new_sort)>60000) && (new_sort < next_sort) && (prev_sort - next_sort > 60000) ) {	
			list_move((struct list_head *)del, (struct list_head *)(sublist_n_pos->list.prev));
			goto insert_into_useList_end;			
		}
		prev_sort = next_sort;
	}		
	list_move_tail((struct list_head *)del, (struct list_head *)head);
insert_into_useList_end:
	os_mutex_unlock(&intercom_s->list_mutex);
	return;	
}

static int32_t recv_repeat_check(uint16_t seq, uint16_t sort)
{
	static uint16_t seq_sort[ENCODED_BUF_NUM*2+1] = {0};

	if(sort == seq_sort[seq]) {
		return 1;
	}
	else {
		seq_sort[seq] = sort;
		return 0;
	}
}

static void lose_packet_check(INTERCOM_STRUCT *intercom_s)
{ 
	static char sequence = 0;
	char temp = 0;
	static uint32_t lose_packet = 0;
	static char retrans_num[ENCODED_BUF_NUM*2+1] = {0};
	socklen_t addrlen = sizeof(struct sockaddr_in);
	uint32_t last_loop_lose = 0;
	uint32_t new_loop_lose = 0;
	uint8_t seq = 0;
	uint16_t sort = 0;
	struct list_head *sublist_l = NULL;
	sublist *sublist_n = NULL;
	uint32_t i = 0;
	int32_t prev_sort = 0;

	for(i=1; i<(ENCODED_BUF_NUM*2+1); i++)
	{
		if(lose_packet & BIT(i)) {
			retrans_num[i] +=1;
			if(retrans_num[i] >= 2) {
				retrans_num[i] = 0;
				lose_packet &= ~BIT(i);
			}
		}
	}

	sublist_l = intercom_s->checkList_head.next;
	sublist_n = list_entry((struct list_head *)sublist_l, sublist, list);

	temp = (sequence % (ENCODED_BUF_NUM*2))+1;
	seq = sublist_n->seq;
	if(os_abs(seq-temp) >= 8)
		temp = seq;
	sort = sublist_n->sort;
	prev_sort = (play_start_flag&BIT(0))?g_play_sort:-1;
	/*收到重复包，丢弃*/
	if(recv_repeat_check(seq, sort)) {
		del_audio_sublist(intercom_s, sublist_l);
	}
	else if((sort <= prev_sort) && ((prev_sort-sort) < 60000)) {
		del_audio_sublist(intercom_s, sublist_l);
		lose_packet &= ~BIT(seq);
		retrans_num[seq] = 0;
	}	
	else
	{
		/*收到丢包，清掉该丢包位*/
		if( lose_packet & BIT(seq) )  {
			lose_packet &= ~BIT(seq);		
			retrans_num[seq] = 0;
		}
		else if( temp != seq ) 
		{
			if(BIT(seq) > BIT(temp))
				lose_packet |= (BIT(seq) - BIT(temp));   
			else {
				last_loop_lose = BIT(ENCODED_BUF_NUM*2+1) - BIT(temp);
				new_loop_lose = BIT(seq) - BIT(1);
				lose_packet |= (last_loop_lose|new_loop_lose);
			}
			sequence = seq;
		}
		else
			sequence = temp;
		insert_into_useList(intercom_s, sublist_l, &intercom_s->useList_head);
	}

	if(lose_packet) {
		sendto(intercom_s->local_ret_fd, &lose_packet, 4, 0, (struct sockaddr*)&(intercom_s->remote_ret_addr), addrlen);
	}
}

static uint8_t is_new_device(INTERCOM_STRUCT *intercom_s, uint32_t new_identify_num)
{
	intercom_device *device;
	intercom_device *npos;
	list_for_each_entry_safe(device, npos, (struct list_head *)&(intercom_s->device_head), list) {
		if(device->identify_num == new_identify_num) {
			return 0;
		}
	}
	os_printf("\n*****intercom find new device*****\n");
	return 1;
}

static void add_new_device(INTERCOM_STRUCT *intercom_s, uint32_t new_identify_num, struct sockaddr_in *remote_trans_addr, uint8_t switch_new)
{
	intercom_device *device = (intercom_device*)INTERCOM_MALLOC(sizeof(intercom_device));
	if(!device) {
		os_printf("intercom add new device fail\n");
		return;
	}
	os_printf("\n*****intercom add new device:%d*****\n",new_identify_num);
	device->identify_num = new_identify_num;
	os_memcpy(&device->trans_addr, remote_trans_addr, sizeof(struct sockaddr_in));
	os_memcpy(&device->ret_addr, remote_trans_addr, sizeof(struct sockaddr_in));
	device->ret_addr.sin_port = htons(INTERCOM_PORT + 1);
	list_move_tail((struct list_head *)device, (struct list_head *)&(intercom_s->device_head));
	connected_num++;
	device->online = 1;
	device->timeout_cnt = TIMEOUT_COUNT;
	if(switch_new) {
		send_start_flag = 0;
		g_r_identify_num = new_identify_num;
		os_memcpy(&intercom_s->remote_trans_addr, &device->trans_addr, sizeof(struct sockaddr_in));
		os_memcpy(&intercom_s->remote_ret_addr, &device->ret_addr, sizeof(struct sockaddr_in));
		send_start_flag = 1;
	}
}

static intercom_device *find_device(INTERCOM_STRUCT *intercom_s, uint32_t identify_num)
{
	intercom_device *device;
	intercom_device *npos;
	list_for_each_entry_safe(device, npos, (struct list_head *)&(intercom_s->device_head), list) {
		if(device->identify_num == identify_num) {
			return device;
		}
	}
	return NULL;	
}

static void switch_device(INTERCOM_STRUCT *intercom_s, uint32_t switch_index)
{
	intercom_device *device;
	intercom_device *npos;
	uint32_t i = 0;
	os_mutex_lock(&intercom_s->send_mutex, osWaitForever);
	list_for_each_entry_safe(device, npos, (struct list_head *)&(intercom_s->device_head), list) {
		if(i == switch_index) {
			g_r_identify_num = device->identify_num;
			os_memcpy(&intercom_s->remote_trans_addr, &device->trans_addr, sizeof(struct sockaddr_in));
			os_memcpy(&intercom_s->remote_ret_addr, &device->ret_addr, sizeof(struct sockaddr_in));		
		}
		i++;
	}	
	os_mutex_unlock(&intercom_s->send_mutex);
}

static void clear_disconnect_device(INTERCOM_STRUCT *intercom_s)
{
	intercom_device *device;
	intercom_device *npos;
	list_for_each_entry_safe(device, npos, (struct list_head *)&(intercom_s->device_head), list) {
		if(device->online) {
			device->timeout_cnt = TIMEOUT_COUNT;
		}
		else {
			device->timeout_cnt--;
		}
		device->online = 0;
		if(device->timeout_cnt <= 0) {
			os_printf("\n*****intercom del device:%d*****\n",device->identify_num);
			list_del(&device->list);
			INTERCOM_FREE(device);
			switch_device(intercom_s, 0);
		}
	}
}

static void clear_all_device(INTERCOM_STRUCT *intercom_s)
{
	intercom_device *device;
	intercom_device *npos;
	list_for_each_entry_safe(device, npos, (struct list_head *)&(intercom_s->device_head), list) {
		list_del(&device->list);
		INTERCOM_FREE(device);
	}
}

static void intercom_recv_task(void *d)
{
	uint8_t *node_addr = NULL;
	int32_t rlen = 0;
	uint16_t check_sum = 0;
	uint32_t code_len = 0;
	uint32_t offset = 0;
	uint32_t node_num = 0;
	uint32_t recv_timeout_cnt = 0;
	struct sockaddr_in remote_trans_addr;
	socklen_t addrlen = sizeof(struct sockaddr_in);
#if ONE_TO_MANY
	uint8_t is_new = 0;
	intercom_device *device;
#endif
	INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)d;

	intercom_task_increase(intercom_s);
	intercom_s->recv_stream_type = intercom_live_audio;
	while(1) {
		if(intercom_s->run_state == intercom_stop) 
			break;	
recv_data_again:	
		rlen = recvfrom(intercom_s->local_trans_fd, intercom_s->recv_buf, 1400, 0, 
					   (struct sockaddr*)&(remote_trans_addr), &addrlen);
		if((play_start_flag&BIT(1)) == 0) {
			continue;
		}	
		clear_disconnect_device(intercom_s);
		if(rlen >= HEAD_RESERVE_BYTE) {
			offset = 0;
			while(rlen > offset) {
				check_sum = ((uint16_t)(intercom_s->recv_buf[offset + HEAD_RESERVE_BYTE - 1]) << 8)
												| intercom_s->recv_buf[offset + HEAD_RESERVE_BYTE - 2];
				if(check_sum != calulate_sum(intercom_s->recv_buf + offset, HEAD_RESERVE_BYTE - 2)) {
					os_printf("intercom abnormal packet\n");
					goto recv_data_again;
				}
				recv_timeout_cnt = 0;
				//os_printf("recv:%d\n",*((uint16_t*)(intercom_s->recv_buf + offset + 2)));
				struct list_head *sublist_l = get_audio_sublist(intercom_s, (struct list_head*)(&intercom_s->checkList_head));
				if(!sublist_l)
					goto recv_data_again;
				sublist *sublist_n = list_entry((struct list_head *)sublist_l, sublist, list);
				os_memcpy(&(sublist_n->seq), intercom_s->recv_buf + offset, HEAD_RESERVE_BYTE - 2);
#if ONE_TO_MANY
				is_new = is_new_device(intercom_s, sublist_n->identify_num);
				os_mutex_lock(&intercom_s->send_mutex, osWaitForever);
				if(is_new) {
					add_new_device(intercom_s, sublist_n->identify_num, &remote_trans_addr, 1);
				}
				else {
					device = find_device(intercom_s, sublist_n->identify_num);
					if(device && os_memcmp(&remote_trans_addr.sin_addr, &device->trans_addr.sin_addr, sizeof(struct in_addr))) {
						os_memcpy(&device->trans_addr.sin_addr, &remote_trans_addr.sin_addr, sizeof(struct in_addr));
						os_memcpy(&device->ret_addr.sin_addr, &remote_trans_addr.sin_addr, sizeof(struct in_addr));
						if(sublist_n->identify_num == cur_r_identify_num) {
							os_memcpy(&intercom_s->remote_trans_addr.sin_addr, &remote_trans_addr.sin_addr, sizeof(struct in_addr));
							os_memcpy(&intercom_s->remote_ret_addr.sin_addr, &remote_trans_addr.sin_addr, sizeof(struct in_addr));							
						}
					}
					if(device)
						device->online = 1;
				}
				os_mutex_unlock(&intercom_s->send_mutex);
				if(cur_r_identify_num != g_r_identify_num) {
					os_event_set(&intercom_s->clear_event, clear_useList_event, NULL);
					os_event_wait(&intercom_s->clear_event, clear_useList_finish_event, 
								  NULL, OS_EVENT_WMODE_CLEAR|OS_EVENT_WMODE_OR, osWaitForever);	
					cur_r_identify_num = g_r_identify_num;
					os_printf("\n******change intercom identify_num****\n");				
				}
				if(cur_r_identify_num != sublist_n->identify_num) {
					sublist_n->node_cnt = 0;
					del_audio_sublist(intercom_s, sublist_l);
					goto recv_data_again;					
				}
#else
				os_mutex_lock(&intercom_s->send_mutex, osWaitForever);
				if(os_memcmp(&remote_trans_addr.sin_addr, &intercom_s->remote_trans_addr.sin_addr, sizeof(struct in_addr))) {
					os_memcpy(&intercom_s->remote_trans_addr.sin_addr, &remote_trans_addr.sin_addr, sizeof(struct in_addr));
					os_memcpy(&intercom_s->remote_ret_addr.sin_addr, &remote_trans_addr.sin_addr, sizeof(struct in_addr));
				}
				os_mutex_unlock(&intercom_s->send_mutex);
				if(cur_r_identify_num != sublist_n->identify_num) {
					os_event_set(&intercom_s->clear_event, clear_useList_event, NULL);
					os_event_wait(&intercom_s->clear_event, clear_useList_finish_event, 
								  NULL, OS_EVENT_WMODE_CLEAR|OS_EVENT_WMODE_OR, osWaitForever);
					g_r_identify_num = sublist_n->identify_num;
					cur_r_identify_num = g_r_identify_num;
					os_printf("\n******change intercom identify_num****\n");
				}
#endif
				offset += HEAD_RESERVE_BYTE;
				if(sublist_n->type != intercom_s->recv_stream_type) {
					sublist_n->node_cnt = 0;
					del_audio_sublist(intercom_s, sublist_l);
					goto recv_data_again;						
				}
				code_len = sublist_n->code_len;
				node_num = (code_len % NODE_DATA_LEN)?(code_len / NODE_DATA_LEN + 1):(code_len / NODE_DATA_LEN);
				sublist_n->node_cnt = node_num;
				// os_printf("recv:%d %d\n",sublist_n->sort,sublist_n->timestamp);
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
					lose_packet_check(intercom_s);
				}
				else {
					sublist_n->node_cnt = 0;
					del_audio_sublist(intercom_s, sublist_l);
					goto recv_data_again;
				}
			}
		}
		else {
			recv_timeout_cnt++;
			if(recv_timeout_cnt > TIMEOUT_COUNT) {
				recv_timeout_cnt = TIMEOUT_COUNT;
				if((play_start_flag&BIT(0)) == 1) {
					os_event_set(&intercom_s->clear_event, clear_useList_event, NULL);
					os_event_wait(&intercom_s->clear_event, clear_useList_finish_event, 
								  NULL, OS_EVENT_WMODE_CLEAR|OS_EVENT_WMODE_OR, osWaitForever);	
					play_start_flag &= ~BIT(0);	
					os_printf("intercom stop\n");
				}
			}
		}
	}
	intercom_task_decrease(intercom_s);
}

static void intercom_output_framebuf(INTERCOM_STRUCT *intercom_s, uint32_t cached)
{	
	uint8_t encode_data[MAX_ENCODED_LEN] = {0};
	uint16_t new_sort = 0;
	int32_t output_res = 0;
	uint32_t timestamp = 0;
	uint32_t offset = 0;
	uint32_t code_len = 0;
	static uint8_t play_speed_sta = 2;
	static uint32_t play_speed = 100;
	static uint32_t last_timestamp = 0;
	static uint32_t plc_cnt = 0;
	struct framebuff *frame_buf = NULL;
	struct list_head *sublist_l = NULL;
	sublist *sublist_n = NULL;	
#if LOSE_STATISTICAL
	static uint32_t last_statistical_time = 0;
#endif

output_framebuf_again:
	frame_buf = fbpool_get(&intercom_s->tx_pool, 0, intercom_s->msi);
	if(frame_buf) {
		g_play_sort = g_current_sort;
		((AUDECODER_OPERATION*)frame_buf->priv)->decode_operation = 0;
		if(list_empty((struct list_head*)&intercom_s->useList_head) == 0) {
			sublist_l = intercom_s->useList_head.next;
			sublist_n = list_entry((struct list_head*)sublist_l, sublist, list);
			new_sort = sublist_n->sort;
			// os_printf("decode info:%d %d %d\n",cached, g_current_sort, new_sort);
			if((g_current_sort>new_sort) && ((g_current_sort-new_sort)<60000)) {
				del_audio_sublist(intercom_s, sublist_l);
			}
			if((SUBLIST_NUM-cached<4) && (((new_sort>g_current_sort)&&(new_sort-g_current_sort<60000)) || 
										((g_current_sort>new_sort)&&(g_current_sort-new_sort>=60000)))) {
				g_current_sort = new_sort;
			} 
			if(g_current_sort == new_sort) {
				timestamp = sublist_n->timestamp;
				last_timestamp = timestamp;
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
					g_current_sort += 1;
					return;
				}
				os_memcpy(frame_buf->data, encode_data, sublist_n->code_len);
				frame_buf->len = sublist_n->code_len;
				timestamp = sublist_n->timestamp;	
				last_timestamp = timestamp;	
				del_audio_sublist(intercom_s, sublist_l);	
#if !PLC_PROCESS
				if(plc_cnt)	
					((AUDECODER_OPERATION*)frame_buf->priv)->decode_operation = DECODE_ADD_FADE_IN;
#endif
				plc_cnt = 0;
			}	
			else {	
#if PLC_PROCESS
				((AUDECODER_OPERATION*)frame_buf->priv)->decode_operation = PACKET_LOSS_CONCEALMENT;
#else
				((AUDECODER_OPERATION*)frame_buf->priv)->decode_operation = OUTPUT_MUTE_DATA;	
#endif
				timestamp = last_timestamp + FRAME_TIME;
				last_timestamp = timestamp;
				plc_cnt++;
				lose_total++;
				if(plc_cnt > max_lose_cnt)
					max_lose_cnt = plc_cnt;
			}
		}
		else {
#if PLC_PROCESS
			((AUDECODER_OPERATION*)frame_buf->priv)->decode_operation = PACKET_LOSS_CONCEALMENT;
#else
			((AUDECODER_OPERATION*)frame_buf->priv)->decode_operation = OUTPUT_MUTE_DATA;	
#endif	
			timestamp = last_timestamp + FRAME_TIME;
			last_timestamp = timestamp;
			plc_cnt++;
			lose_total++;
			if(plc_cnt > max_lose_cnt)
				max_lose_cnt = plc_cnt;			
		}
#if CHANGE_PLAY_SPEED
		if((cached > 0) && (cached <= (play_start_wait - 5))) {
			if(play_speed_sta != 0) {
				play_speed = 90;
				msi_do_cmd(intercom_s->autpc_msi, MSI_CMD_AUTPC, MSI_AUTPC_SET_SPEED, play_speed);
				play_speed_sta = 0;
			}
		}
		else if(cached >= (play_start_wait + 5)) {
			if(play_speed_sta != 1) {
				play_speed = 110;
				msi_do_cmd(intercom_s->autpc_msi, MSI_CMD_AUTPC, MSI_AUTPC_SET_SPEED, play_speed);
				play_speed_sta = 1;
			}
		}
		else if((cached == 0) || (cached == play_start_wait)) {
			if(play_speed_sta != 2) {
				play_speed = 100;
				msi_do_cmd(intercom_s->autpc_msi, MSI_CMD_AUTPC, MSI_AUTPC_SET_SPEED, play_speed);
				play_speed_sta = 2;
			}
		}	
		if(play_speed == 90) 
			slow_speed_cnt++;
		if(play_speed == 110)
			fast_speed_cnt++;
#endif
		frame_buf->time = timestamp;
		frame_buf->mtype = F_AUDIO;	
		output_res = msi_output_fb(intercom_s->msi, frame_buf);
		// os_printf("out:%d %d\n",g_current_sort,(int32_t)(os_jiffies()-timestamp));
		if((now_dac_filter_type==FSTYPE_AUDIO_DECODER) && (cached>=1) && (cached >= (play_start_wait-2))) {    
			cached -= 1;
			g_current_sort += 1;
			os_sleep_ms(5);
			goto output_framebuf_again;     
		}
		g_current_sort += 1;
	}
#if LOSE_STATISTICAL
	if(os_jiffies()-last_statistical_time > 5000) {
		_os_printf("\n");
		os_printf("---intercom loss statistical---\n");
		_os_printf("total loss:%d, max loss:%d, slow speed:%d, fast speed:%d\n",lose_total, max_lose_cnt, slow_speed_cnt, fast_speed_cnt);
		_os_printf("\n");
		lose_total = 0;
		max_lose_cnt = 0;
		slow_speed_cnt = 0;
		fast_speed_cnt = 0;
		last_statistical_time = os_jiffies();
	}
#endif
}

static void clear_useList_func(INTERCOM_STRUCT *intercom_s)
{
	g_numofcached = get_audio_sublist_count(intercom_s, (struct list_head *)&intercom_s->useList_head);
	for(uint8_t i=0; i<g_numofcached; i++) {
		struct list_head *sublist_l = intercom_s->useList_head.next;
		del_audio_sublist(intercom_s, sublist_l);
	}
	g_numofcached = 0;
	play_start_flag &= ~BIT(0);
}

static void intercom_output_task(void *d)
{
	uint8_t numofwait = 0;
	uint32_t useList_clear = 0;
    uint32_t former_dac_filter_type = 0;
    uint32_t former_dac_samplingrate = 0;
	struct list_head *sublist_l = NULL;
	sublist *sublist_n = NULL;
	INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)d;
	struct os_semaphore *sem = &intercom_s->output_sema;

	intercom_task_increase(intercom_s);
#if CHANGE_PLAY_SPEED
	intercom_s->decoder_msi = audio_decode_init(AUDIO_DECODER, AUADC_SAMPLERATE, 0);
	audio_code_add_output(AUDIO_DECODER, intercom_s->autpc_msi->name);
	msi_add_output(intercom_s->autpc_msi, NULL, "R_AUDAC");
	msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_GET_FILTER_TYPE,(uint32_t)(&former_dac_filter_type));
	msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TYPE,FSTYPE_AUDIO_OPUS_DECODER);
	msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_GET_SAMPLING_RATE,(uint32_t)(&former_dac_samplingrate));
	msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_SAMPLING_RATE,AUADC_SAMPLERATE);
#else
	intercom_s->decoder_msi = audio_decode_init(AUDIO_DECODER, AUADC_SAMPLERATE, 1);
#endif
	if(!intercom_s->decoder_msi) {
		os_printf("intercom audio decoder init fail!\n");
		intercom_task_decrease(intercom_s);
		return;
	}
	msi_add_output(intercom_s->msi, NULL, intercom_s->decoder_msi->name);
	while(1) {
		if(intercom_s->run_state == intercom_stop) 
			break;	
		os_event_wait(&intercom_s->clear_event, clear_useList_event, &useList_clear, OS_EVENT_WMODE_CLEAR|OS_EVENT_WMODE_OR, 0);
		if(useList_clear & clear_useList_event) {
			useList_clear = 0;
			clear_useList_func(intercom_s);
			os_event_set(&intercom_s->clear_event, clear_useList_finish_event, NULL);
			numofwait = 0;
			continue;
		}
		if(os_sema_down(sem, 1) == 1) {	
			g_numofcached = get_audio_sublist_count(intercom_s, (struct list_head *)&intercom_s->useList_head);
			msi_cmd("R_AUDAC", MSI_CMD_AUDAC, MSI_AUDAC_GET_FILTER_TYPE, (uint32_t)(&now_dac_filter_type));
			if(now_dac_filter_type != FSTYPE_AUDIO_DECODER) {
				clear_useList_func(intercom_s);
				numofwait = 0;
				continue;
			}
			if(((play_start_flag&BIT(0)) == 0) && (g_numofcached > play_start_wait)) {
				play_start_flag |= BIT(0);
				numofwait = 0;
				sublist_l = intercom_s->useList_head.next;
				sublist_n = list_entry((struct list_head*)sublist_l, sublist, list);
				g_current_sort = sublist_n->sort;
				os_printf("intercom start\n");			
			}
			if( ((play_start_flag&BIT(0)) == 0) && (g_numofcached > 0) ) 
				numofwait++;
			if(play_start_flag&BIT(0)) {
				lose_total = 0;
				max_lose_cnt = 0;
				slow_speed_cnt = 0;
				fast_speed_cnt = 0;
				intercom_output_framebuf(intercom_s, g_numofcached);
			}
		}
	}
#if CHANGE_PLAY_SPEED
	msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_FILTER_TYPE,former_dac_filter_type);
	msi_cmd("R_AUDAC",MSI_CMD_AUDAC,MSI_AUDAC_SET_SAMPLING_RATE,former_dac_samplingrate);
#endif
	intercom_task_decrease(intercom_s);
}

static void intercom_handel_task(void *d)
{
	uint32_t ipaddr;  
    int32_t err = -1; 
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int32_t trans_time_out = FRAME_TIME * 2;	
	int32_t ret_time_out = 1;
	INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)d;

	intercom_task_increase(intercom_s);
	if(sys_cfgs.wifi_mode == WIFI_MODE_STA) {
		do{
			if(sys_cfgs.wifi_mode == WIFI_MODE_AP)
				break;
			if(intercom_s->run_state == intercom_stop) {
				intercom_task_decrease(intercom_s);
				return;
			}
			ipaddr = lwip_netif_get_ip2("w0").addr;
			os_sleep_ms(100);
		}while((ipaddr&0xff000000) == 0x1000000);		
	}

    intercom_s->local_trans_fd = socket(AF_INET,SOCK_DGRAM, 0);
    intercom_s->local_ret_fd = socket(AF_INET,SOCK_DGRAM, 0);
	if((intercom_s->local_trans_fd==-1) || (intercom_s->local_ret_fd==-1)) {
		os_printf("create intercom socket fail!\n");
		goto intercom_handel_task_err;
	}

	if(setsockopt(intercom_s->local_trans_fd,SOL_SOCKET,SO_RCVTIMEO,&trans_time_out,sizeof(int32_t)) == -1) {
		os_printf("intercom setsockopt fail!\n");
		goto intercom_handel_task_err;
	}

	if(setsockopt(intercom_s->local_ret_fd,SOL_SOCKET,SO_RCVTIMEO,&ret_time_out,sizeof(int32_t)) == -1) {
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
        if(sys_cfgs.wifi_mode == WIFI_MODE_STA) {
		    (*((&intercom_s->remote_trans_addr)+i)).sin_addr.s_addr = inet_addr("192.168.169.1");
		}
        else {
#if ONE_TO_MANY
			send_start_flag = 0;
#else
            (*((&intercom_s->remote_trans_addr)+i)).sin_addr.s_addr = inet_addr("192.168.169.100");
#endif
		}
	}

	err = intercom_room_init(intercom_s);
	if(err == RET_ERR) {
		goto intercom_handel_task_err;			
	}

	os_mutex_init(&intercom_s->list_mutex);
	os_mutex_init(&intercom_s->send_mutex);
	os_event_init(&intercom_s->clear_event);
	os_sema_init(&intercom_s->output_sema, 0);
	os_timer_init(&intercom_s->ctl_timer, (os_timer_func_t)output_sema_up, OS_TIMER_MODE_PERIODIC, intercom_s);

	if(!intercom_s->list_mutex.hdl || !intercom_s->send_mutex.hdl || 
	   !intercom_s->clear_event.hdl || !intercom_s->output_sema.hdl || !intercom_s->ctl_timer.hdl) {
		os_printf("intercom malloc synth fail!\n");
		goto intercom_handel_task_err;
	}

	intercom_s->recv_task_hdl = os_task_create("intercom_recv_task", intercom_recv_task, (void*)intercom_s, OS_TASK_PRIORITY_ABOVE_NORMAL-1, 0, NULL, 1024);
	intercom_s->send_task_hdl = os_task_create("intercom_send_task", intercom_send_task, (void*)intercom_s, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 1024);
	intercom_s->output_task_hdl = os_task_create("intercom_output_task", intercom_output_task, (void*)intercom_s, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 1024);
	if(!intercom_s->recv_task_hdl || !intercom_s->send_task_hdl || !intercom_s->output_task_hdl) {
		os_printf("intercom create task fail!\n");
		goto intercom_handel_task_err;
	}

	os_timer_start(&intercom_s->ctl_timer, FRAME_TIME);
	intercom_task_decrease(intercom_s);
	return;	
intercom_handel_task_err:	
	intercom_task_decrease(intercom_s);
	intercom_deinit(intercom_s->msi);
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
            if(frame_buf->mtype == F_AUDIO) {
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
                fbpool_put(&intercom_s->tx_pool, frame_buf);
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
					if(frame_buf->priv) {
                        INTERCOM_FREE(frame_buf->priv);
                        frame_buf->priv = NULL;						
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
	struct framebuff *frame_buf = NULL;
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
	intercom_s->msi = msi;
    intercom_s->msi->enable = 1;
    intercom_s->msi->action = intercom_msi_action;
    fbpool_init(&intercom_s->tx_pool, MAX_INTERCOM_TXBUF);
    for(uint32_t i=0; i<MAX_INTERCOM_TXBUF; i++) {
		frame_buf = (intercom_s->tx_pool.pool)+i;
        frame_buf->data = NULL;
		frame_buf->priv = NULL;
		frame_buf->priv = (AUDECODER_OPERATION*)INTERCOM_ZALLOC(sizeof(AUDECODER_OPERATION));
		if(!frame_buf->priv)
			goto intercom_init_err;
    }
    intercom_s->msi->priv = intercom_s;
#if MAGIC_VOICE_EN
	intercom_s->magic_voice_msi = magic_voice_init(AUADC_SAMPLERATE, 1, FRAME_TIME*AUADC_SAMPLERATE/1000);
	if(intercom_s->magic_voice_msi == NULL) {
        os_printf("magic voice create magic voice msi fail\n");
        goto intercom_init_err;   		
	}
#endif
#if CHANGE_PLAY_SPEED
	intercom_s->autpc_msi = autpc_msi_init(AUADC_SAMPLERATE, 100, 100, FRAME_TIME*AUADC_SAMPLERATE/1000);
    if(intercom_s->autpc_msi == NULL) {
        os_printf("magic voice create autpc msi fail\n");
        goto intercom_init_err;       
    }	
#endif
	intercom_s->local_trans_fd = -1;
	intercom_s->local_ret_fd = -1;
	if(os_mutex_init(&intercom_s->state_mutex) != RET_OK)
		goto intercom_init_err;
	intercom_task_state_init(intercom_s);
	intercom_s->init_task_hdl = os_task_create("intercom_handel_task", intercom_handel_task, (void*)intercom_s, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
	if(!intercom_s->init_task_hdl)
		goto intercom_init_err;
	return intercom_s->msi;
intercom_init_err:
	intercom_deinit(intercom_s->msi);
	os_printf("intercom init fail\n");
	return NULL;
}

void intercom_deinit(struct msi *msi)
{
	INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)(msi->priv);
	intercom_s->run_state = intercom_stop;	
	while(intercom_s->run_task > 0)
		os_sleep_ms(1);
	clear_all_device(intercom_s);
	if(intercom_s->autpc_msi) {
		msi_destroy(intercom_s->autpc_msi);
	}
	if(intercom_s->magic_voice_msi) {
		magic_voice_deinit();
	}
	if(intercom_s->encoder_msi) {
		if(audio_encode_deinit(AUDIO_ENCODER) == RET_OK)
			auadc_msi_del_output(audio_code_msi_name(AUDIO_ENCODER));		
	}
	if(intercom_s->decoder_msi)
		audio_decode_deinit(AUDIO_DECODER);
	if(intercom_s->ctl_timer.hdl) {
		os_timer_stop(&intercom_s->ctl_timer);
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
	if(intercom_s->clear_event.hdl)
		os_event_del(&intercom_s->clear_event);
	intercom_close_socket(intercom_s);
	intercom_room_free(intercom_s);
	msi_destroy(intercom_s->msi);

	g_play_sort = 0;
	g_current_sort = 1;
	g_numofcached = 0;
	g_s_identify_num = 0;
	send_start_flag = 0;
	play_start_flag = 2;
}

void intercom_send_enable(uint8_t state)
{
	send_start_flag = state;
}

void intercom_recv_enable(uint8_t state)
{
	if(state == 1)
		play_start_flag |= BIT(1);
	else if(state == 0)
		play_start_flag &= ~BIT(1);
}

void intercom_encode_pause(uint8_t state, uint8_t clear)
{
	if(state == 1) {
		audio_code_pause(AUDIO_ENCODER);
		if(clear) {
			audio_code_clear(AUDIO_ENCODER);
		}
	}
	else if(state == 0)
		audio_code_continue(AUDIO_ENCODER);
}

void intercom_decode_pause(uint8_t state, uint8_t clear)
{
	if(state == 1) {
		audio_code_pause(AUDIO_DECODER);
		if(clear) {
			audio_code_clear(AUDIO_DECODER);
		}
		struct msi *msi = msi_find("SR_INTERCOM", 1);
		if(msi) {
			msi_put(msi);
			INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)(msi->priv);
			if(intercom_s->autpc_msi) {
				msi_do_cmd(intercom_s->autpc_msi, MSI_CMD_AUTPC, MSI_AUTPC_END_STREAM, 0);
			}
		}
	}
	else if(state == 0)
		audio_code_continue(AUDIO_DECODER);
}

void intercom_reset_play(void)
{
	struct msi *msi = msi_find("SR_INTERCOM", 1);
	if(msi) {
		msi_put(msi);
		play_start_flag &= ~BIT(1);
		INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)(msi->priv);
		os_event_set(&intercom_s->clear_event, clear_useList_event, NULL);
		os_event_wait(&intercom_s->clear_event, clear_useList_finish_event, 
						NULL, OS_EVENT_WMODE_CLEAR|OS_EVENT_WMODE_OR, osWaitForever);
		play_start_flag |= BIT(1);
	}
}

void intercom_set_stream_type(uint8_t recv_type, uint8_t send_type)
{
	struct msi *msi = msi_find("SR_INTERCOM", 1);
	if(msi) {
		msi_put(msi);
		INTERCOM_STRUCT *intercom_s = (INTERCOM_STRUCT*)(msi->priv);
		intercom_s->recv_stream_type = recv_type;
		intercom_s->send_stream_type = send_type;
	}
}