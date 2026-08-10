#ifndef SCREEN_MSG_QUEUE_H
#define SCREEN_MSG_QUEUE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef SCREEN_MSG_QUEUE_LOCK
#define SCREEN_MSG_QUEUE_LOCK() disable_irq()
#endif

#ifndef SCREEN_MSG_QUEUE_UNLOCK
#define SCREEN_MSG_QUEUE_UNLOCK(f) enable_irq(f)
#endif



/* 消息队列节点：保存一条待显示消息
 * text 为 malloc 分配，free 释放 */
typedef struct screen_msg_node_s
{
    uint16_t screen_id;                     /* 目标 screen ID */
    uint8_t is_user;                         /* 1=用户消息，0=AI 回复 */
    char *text;                              /* 文本副本 */
    struct screen_msg_node_s *next;
} screen_msg_node_t;

/* 消息队列（单向链表，head/tail 快速入队出队） */
typedef struct
{
    screen_msg_node_t *head;
    screen_msg_node_t *tail;
} screen_msg_queue_t;

/* 初始化队列头尾为 NULL */
void screen_msg_queue_init(screen_msg_queue_t *queue);

/* 推入一条消息，text 会被复制，调用方可立即释放自己的副本。 */
int32_t screen_msg_queue_push(screen_msg_queue_t *queue, const char *text, uint8_t is_user);

/* 推入一条消息到分发总队列，由 LVGL 线程调用 screen_msg_queue_dispatch 分发。 */
int32_t screen_msg_queue_push_dispatch(uint16_t screen_id, const char *text, uint8_t is_user);

/* 分发总队列中的所有消息到对应 screen 队列。 */
void screen_msg_queue_dispatch(void);

/* 弹出队头节点，队列为空返回 NULL */
screen_msg_node_t *screen_msg_queue_pop(screen_msg_queue_t *queue);

/* 释放弹出的节点（内部 free text 和自身），pop 后用完调用 */
void screen_msg_node_free(screen_msg_node_t *node);

/* 清空队列并释放所有节点 */
void screen_msg_queue_clear(screen_msg_queue_t *queue);

/* 队列是否为空，返回 1=空 */
uint8_t screen_msg_queue_is_empty(screen_msg_queue_t *queue);

/* 将已创建的节点推入目标 screen 的队列；找不到目标时释放 node。 */
int32_t screen_msg_queue_push_by_id(screen_msg_node_t *node);

#ifdef __cplusplus
}
#endif

#endif
