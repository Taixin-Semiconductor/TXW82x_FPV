#ifndef SCREEN_COMMAND_H
#define SCREEN_COMMAND_H
#include "basic_include.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct screen_command_t
{
    struct os_event evt;
    uint32_t        id;
    uint32_t        screen_id;
    void           *params;
} screen_command_t;

/* 初始化命令队列；由外部在使用 send/recv 前显式调用。 */
void screen_command_init(void);
/* 释放队列中尚未接收的命令节点；不释放各命令携带的 params。 */
void screen_command_deinit(void);

/* 发送创建 UI 命令；返回值为命令节点句柄，失败返回 NULL。 */
uint32_t screen_command_send(uint16_t id, uint32_t screen_id, void *params);
/* 从队列头取出一条命令；返回值可转换为 screen_command_t * 读取 id/params。 */
void    *screen_command_recv(void);
void screen_command_finish(screen_command_t *node);

#ifdef __cplusplus
}
#endif

#endif
