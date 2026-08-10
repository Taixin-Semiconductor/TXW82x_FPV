#include "basic_include.h"
#include "screen_msg_queue.h"
#include "screen_memory.h"
#include "screen_manager.h"
#include "osal/string.h"

/* 分发总队列，最后由 LVGL 线程分发到目标 screen 队列。 */
static screen_msg_queue_t *dispatch_queue = NULL;

static void screen_msg_queue_push_node(screen_msg_queue_t *queue, screen_msg_node_t *node)
{
    if (queue == NULL || node == NULL)
    {
        return;
    }

    node->next = NULL;
    if (queue->tail != NULL)
    {
        queue->tail->next = node;
    }
    else
    {
        queue->head = node;
    }
    queue->tail = node;
}

static screen_msg_node_t *screen_msg_node_create(uint16_t screen_id, const char *text, uint8_t is_user)
{
    screen_msg_node_t *node;
    size_t             len;

    if (text == NULL)
    {
        return NULL;
    }

    len  = os_strlen(text);
    node = (screen_msg_node_t *) SCREEN_MALLOC(sizeof(screen_msg_node_t) + len + 1);
    if (node == NULL)
    {
        return NULL;
    }
    node->text = (char *)(node + 1);
    os_memcpy(node->text, text, len + 1);
    node->screen_id = screen_id;
    node->is_user   = is_user;
    node->next      = NULL;
    return node;
}

void screen_msg_queue_dispatch(void)
{
    screen_msg_node_t *node;
    screen_msg_queue_t dispatch_local;
    uint32_t           flag;

    if (dispatch_queue == NULL)
    {
        screen_msg_queue_t *queue;
        queue = (screen_msg_queue_t *) SCREEN_MALLOC(sizeof(screen_msg_queue_t));
        if (queue == NULL)
        {
            return;
        }
        screen_msg_queue_init(queue);
        dispatch_queue = queue;
        return;
    }

    flag = SCREEN_MSG_QUEUE_LOCK();
    if (screen_msg_queue_is_empty(dispatch_queue))
    {
        SCREEN_MSG_QUEUE_UNLOCK(flag);
        return;
    }
    dispatch_local = *dispatch_queue;
    screen_msg_queue_init(dispatch_queue);
    SCREEN_MSG_QUEUE_UNLOCK(flag);

    while ((node = screen_msg_queue_pop(&dispatch_local)) != NULL)
    {
        (void) screen_msg_queue_push_by_id(node);
    }
}

int32_t screen_msg_queue_push_dispatch(uint16_t screen_id, const char *text, uint8_t is_user)
{
    screen_msg_node_t *node;
    uint32_t           flag;

    if (dispatch_queue == NULL)
    {
        return -1;
    }

    node = screen_msg_node_create(screen_id, text, is_user);
    if (node == NULL)
    {
        return -1;
    }

    flag = SCREEN_MSG_QUEUE_LOCK();
    screen_msg_queue_push_node(dispatch_queue, node);
    SCREEN_MSG_QUEUE_UNLOCK(flag);
    return 0;
}

void screen_msg_queue_init(screen_msg_queue_t *queue)
{
    if (queue == NULL)
    {
        return;
    }
    queue->head = NULL;
    queue->tail = NULL;
}

int32_t screen_msg_queue_push(screen_msg_queue_t *queue, const char *text, uint8_t is_user)
{
    screen_msg_node_t *node;

    if (queue == NULL)
    {
        return -1;
    }

    node = screen_msg_node_create(0, text, is_user);
    if (node == NULL)
    {
        return -1;
    }

    screen_msg_queue_push_node(queue, node);
    return 0;
}

screen_msg_node_t *screen_msg_queue_pop(screen_msg_queue_t *queue)
{
    screen_msg_node_t *node;

    if (queue == NULL || queue->head == NULL)
    {
        return NULL;
    }

    node        = queue->head;
    queue->head = node->next;
    if (queue->head == NULL)
    {
        queue->tail = NULL;
    }
    node->next = NULL;
    return node;
}

void screen_msg_queue_clear(screen_msg_queue_t *queue)
{
    screen_msg_node_t *node;

    if (queue == NULL)
    {
        return;
    }
    while (queue->head != NULL)
    {
        node        = queue->head;
        queue->head = node->next;
        screen_msg_node_free(node);
    }
    queue->tail = NULL;
}

uint8_t screen_msg_queue_is_empty(screen_msg_queue_t *queue)
{
    return (queue == NULL || queue->head == NULL) ? 1 : 0;
}

void screen_msg_node_free(screen_msg_node_t *node)
{
    if (node == NULL)
    {
        return;
    }
    SCREEN_FREE(node);
}

int32_t screen_msg_queue_push_by_id(screen_msg_node_t *node)
{
    screen_t           *screen;
    screen_msg_queue_t *queue;

    if (node == NULL)
    {
        return -1;
    }
    screen = screen_find_by_id(node->screen_id);
    if (screen == NULL)
    {
        screen_msg_node_free(node);
        return -1;
    }
    queue = (screen_msg_queue_t *) screen_get_user_data(screen);
    if (queue == NULL)
    {
        screen_msg_node_free(node);
        return -1;
    }
    screen_msg_queue_push_node(queue, node);
    return 0;
}
