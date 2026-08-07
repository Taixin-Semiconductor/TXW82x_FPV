#ifndef __AUSYS_QUEUE_H
#define __AUSYS_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif


struct __ausys_queue_elem {
	uint32 addr;
	uint32 bytes;
};

typedef struct __ausys_queue_ctrl{
	uint32   elem_num;
	uint16   head;
	uint16   tail;
	uint16   per_bytes;

	uint16   sta_init:1,
		     sta_none:1;

	void    *p_malloc;
	void    *p_use;
	struct __ausys_queue_elem  *p_queue;
}TYPE_AUSYS_QUEUE;



int32 ausys_queue_create(TYPE_AUSYS_QUEUE *ausys_queue, uint32 total_bytes, uint32 per_bytes);
int32 ausys_queue_destroy(TYPE_AUSYS_QUEUE *ausys_queue);
int32 ausys_queue_elem_ent(TYPE_AUSYS_QUEUE *ausys_queue, void *buf, uint32 bytes, uint32 need_cpy);
int32 ausys_queue_elem_del(TYPE_AUSYS_QUEUE *ausys_queue);
void *ausys_queue_get_tail_addr(TYPE_AUSYS_QUEUE *ausys_queue);
uint32 ausys_queue_get_tail_len(TYPE_AUSYS_QUEUE *ausys_queue);
void ausys_queue_clr_head_first_half(TYPE_AUSYS_QUEUE *ausys_queue);
void ausys_queue_clr_head_second_half(TYPE_AUSYS_QUEUE *ausys_queue);
void ausys_queue_clr_head_all(TYPE_AUSYS_QUEUE *ausys_queue);
void *ausys_queue_get_head_addr(TYPE_AUSYS_QUEUE *ausys_queue);
uint32 ausys_queue_get_head_len(TYPE_AUSYS_QUEUE *ausys_queue);
void *ausys_queue_get_index_addr(TYPE_AUSYS_QUEUE *ausys_queue, uint32 index);
uint32 ausys_queue_get_index_len(TYPE_AUSYS_QUEUE *ausys_queue, uint32 index);
uint32 ausys_queue_get_head_index(TYPE_AUSYS_QUEUE *ausys_queue);
uint32 ausys_queue_get_tail_index(TYPE_AUSYS_QUEUE *ausys_queue);
void *ausys_queue_get_headnext_addr(TYPE_AUSYS_QUEUE *ausys_queue);
uint32 ausys_queue_get_headnext_len(TYPE_AUSYS_QUEUE *ausys_queue);
int32 ausys_queue_is_empty(TYPE_AUSYS_QUEUE *ausys_queue);
int32 ausys_queue_is_full(TYPE_AUSYS_QUEUE *ausys_queue);


#ifdef __cplusplus
}
#endif


#endif