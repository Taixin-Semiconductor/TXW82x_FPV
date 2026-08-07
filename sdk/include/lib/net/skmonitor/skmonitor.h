#ifndef _SDK_SKMONITOR_H_
#define _SDK_SKMONITOR_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SOCK_MONITOR_READ   = BIT(0),
    SOCK_MONITOR_WRITE  = BIT(1),
    SOCK_MONITOR_ERROR  = BIT(2),
} skmonitor_flags;

typedef void (*skmonitor_cb)(uint16 sock, skmonitor_flags flags, uint32 priv);

int32 sock_monitor_init(void);
void sock_monitor_dump(void);
/* 注册到skmonitor的socket，不能再由模块task自己主动读取，只能由skmonitor触发执行读取 */
int32 sock_monitor_add(uint16 sock, skmonitor_flags flags, skmonitor_cb cb, uint32 priv);
void sock_monitor_del(uint16 sock);
void sock_monitor_disable(uint16 sock, uint8 disable);

#ifdef __cplusplus
}
#endif
#endif
