#ifndef _INTERCOM_H_
#define _INTERCOM_H_

#include "basic_include.h"
#include "lwip/sockets.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "lib/audio/ring_buffer/ring_buffer.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "keyWork.h"
#include "keyScan.h"

#ifdef PSRAM_HEAP
#define INTERCOM_MALLOC os_malloc_psram
#define INTERCOM_ZALLOC os_zalloc_psram
#define INTERCOM_FREE   os_free_psram
#else
#define INTERCOM_MALLOC os_malloc
#define INTERCOM_ZALLOC os_zalloc
#define INTERCOM_FREE   os_free
#endif

#define ADJUST_BY_LOSS  1
#define ADJUST_BY_MCS   2


#ifndef ONE_TO_MANY
#define ONE_TO_MANY             0
#endif
#ifndef INTERCOM_GROUP
#define INTERCOM_GROUP          1
#endif
#ifndef INTERCOM_HALF_DUPLEX
#define INTERCOM_HALF_DUPLEX    0
#endif
#ifndef LOW_BITRATE_MODE        
#define LOW_BITRATE_MODE        0
#endif 
#if INTERCOM_GROUP
#define MAX_INTERCOM_SLAVE_DEVICE     2
#else
#define MAX_INTERCOM_SLAVE_DEVICE     1
#endif

enum {
    intercom_live_audio = 1,
    intercom_playback_audio,
};

enum {
    high_bitrate_mode,
    low_bitrate_mode,
};

enum {
    mild_loss,
    serious_loss,
};

typedef struct {     
	struct list_head list;
	uint8_t *buf_addr;
}audio_node;

typedef struct {
    struct list_head list;
    struct list_head node_head;
    uint8_t seq;
    uint8_t type; //1：实时流，2：回放流
    uint16_t sort;
    uint32_t timestamp;
    uint32_t code_len;
	uint32_t identify_num;
	uint32_t node_cnt;
} __attribute__((packed)) sublist;

typedef struct {
    struct list_head list;
    uint8_t* buf_addr;
    uint32_t data_len;
}ringbuf_manage;

typedef enum {
    SEND_ONLY,
    RECV_ONLY,
    NORMAL_MODE,
}transfer_mode;

typedef enum {
    intercom_run,
    intercom_stop,
}intercom_state;

typedef struct {
    uint32 last_time;
    uint16 time_keep;
    int16  time_diff;
}time_synth;

typedef struct {
    struct list_head list;
    uint32_t ip_addr;
    uint32_t identify_num;
    uint8_t dev_id;
    uint8_t online;
    uint16_t timeout_cnt;
} intercom_device;

typedef struct {
    uint16_t g_current_sort[MAX_INTERCOM_SLAVE_DEVICE];
    uint16_t g_numofcached[MAX_INTERCOM_SLAVE_DEVICE];
    uint32_t g_s_identify_num;
    uint32_t g_r_identify_num;

    uint8_t g_send_enable; 
    uint8_t play_start_wait;

    uint8_t run_state;
    uint8_t run_task;
    uint8_t recv_stream_type;
    uint8_t send_stream_type;

    uint8_t cur_bitrate_mode;
    uint8_t new_bitrate_mode;
    uint8_t loss_state[MAX_INTERCOM_SLAVE_DEVICE];

    uint8_t connected_num;
    uint8_t current_dev_id;
    uint8_t next_dev_id;

    int local_trans_fd;
    int local_ret_fd;

    struct sockaddr_in local_trans_addr;
    struct sockaddr_in local_ret_addr;
    struct sockaddr_in remote_trans_addr;
    struct sockaddr_in remote_ret_addr;
    struct sockaddr_in remote_retrans_addr;

    void *init_task_hdl;
	void *recv_task_hdl;
	void *send_task_hdl;
    void *output_task_hdl;

    uint8_t *recv_buf;
    uint8_t *send_buf;
    uint8_t *sort_buf;
    RINGBUF *encoded_ringbuf;
    ringbuf_manage *ringbuf_manage_src;
    audio_node *audio_node_src;
    sublist *sublist_src;

    struct list_head checkList_head[MAX_INTERCOM_SLAVE_DEVICE];
    struct list_head useList_head[MAX_INTERCOM_SLAVE_DEVICE];
    struct list_head nodeList_head;
    struct list_head sublist_head;
    struct list_head ringbuf_manage_empty;
    struct list_head ringbuf_manage_used;
    struct list_head *manage_cur_push;
    struct list_head *manage_cur_pop; 

    struct list_head device_head;

    struct msi *msi;
    struct msi *encoder_msi;
    struct msi *decoder_msi[MAX_INTERCOM_SLAVE_DEVICE];
    struct fbpool tx_pool;

    struct os_mutex state_mutex;
    struct os_mutex list_mutex;
    struct os_mutex send_mutex;
    struct os_event clear_event[MAX_INTERCOM_SLAVE_DEVICE];
    struct os_semaphore output_sema;
    struct os_timer ctl_timer;

    txAudioInfo_t codec_info;
	time_synth time_s[MAX_INTERCOM_SLAVE_DEVICE];
} INTERCOM_STRUCT;

struct msi *intercom_init(void);
void intercom_deinit(void);
void intercom_send_enable(uint8_t state);
void intercom_recv_enable(uint8_t state);
void intercom_encode_pause(uint8_t state);
void intercom_decode_pause(uint8_t state, uint8_t dev_id);
uint32_t intercom_ctrl_key(struct key_callback_list_s *callback_list,uint32_t keyvalue,uint32_t extern_value);
#endif