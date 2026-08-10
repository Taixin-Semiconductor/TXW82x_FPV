#ifndef SCREEN_H
#define SCREEN_H

#include "lvgl.h"
#include "screen_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SCREEN_STATE_NONE = 0,
    SCREEN_STATE_CREATED,
    SCREEN_STATE_STARTED,
    SCREEN_STATE_RESUMED,
    SCREEN_STATE_PAUSED,
    SCREEN_STATE_STOPPED,
    SCREEN_STATE_DESTROYED,
} screen_state_t;

enum
{
    SCREEN_TYPE_NONE,
    SCREEN_TYPE_CHAT_MSG, // 支持聊天对话类型
};

typedef struct screen_t screen_t;

typedef struct
{
    /* 首次进入生命周期时触发；params 为 screen_push/screen_replace 传入的启动参数。 */
    void (*on_create)(screen_t *screen, void *params);
    /* 页面即将可见时触发；screen 为当前生命周期所属页面。 */
    void (*on_start)(screen_t *screen);
    /* 页面成为栈顶并可交互时触发；screen 为当前生命周期所属页面。 */
    void (*on_resume)(screen_t *screen);
    /* 页面失去焦点时触发；screen 为当前生命周期所属页面。 */
    void (*on_pause)(screen_t *screen);
    /* 页面不可见或退出前台时触发；screen 为当前生命周期所属页面。 */
    void (*on_stop)(screen_t *screen);
    /* 页面对象即将销毁时触发；用于释放页面私有资源。 */
    void (*on_destroy)(screen_t *screen);
    /* 页面从 STOPPED 状态重新回到前台前触发。 */
    void (*on_restart)(screen_t *screen);
    /* 页面业务明确结束时触发；用于移除业务列表、回传结果，不释放 UI 资源。 */
    void (*on_finish)(screen_t *screen);
    uint32_t screen_id;
    uint32_t only;
} screen_lifecycle_t;

/*************************************************************************
 * state: 当前生命周期状态。
 * keep_alive: 非 0 表示退出栈时保留页面对象作为后台页。
 * finished: 非 0 表示 on_finish 已触发，避免重复回调。
 * in_stack: 非 0 表示当前页面在导航栈中。
 * is_extern_destroyed: 非 0 表示外部销毁了页面对象
 **************************************************************** */
struct screen_t
{
    lv_obj_t          *root;      /* 页面根容器，必须是 lv_scr_act() 的直接子对象。 */
    uint32_t           id;        /* screen_register 自动分配的唯一 ID。 */
    uint32_t           only;      /* 非 0 表示仅支持一个实例 */
    const char        *name;      /* 可选页面名称；可为 NULL。 */
    screen_lifecycle_t lifecycle; /* 页面生命周期回调集合。 */
    void              *user_data; /* 页面私有数据，由业务层管理和释放。 */
    ui_extern_cb       cb;
    void              *cb_priv;
    uint8_t            state : 3, keep_alive : 1, finished : 1, in_stack : 1, is_extern_destroyed : 1;
    uint8_t            type; // 类型,screen_manager内部的类型,比如是否可以支持接收数据之类,需要类型匹配才可以
};

/* 创建 Screen 对象和隐藏 root；name 为可选名称，lifecycle 为生命周期回调集合。 */
screen_t *screen_create(const char *name, screen_lifecycle_t *lifecycle);
/* 销毁 Screen 对象；会触发 on_destroy 并删除 root。 */
void      screen_destroy(screen_t *screen);
/* 设置页面私有数据；screen 为目标页面，user_data 为业务层自定义指针。 */
void      screen_set_user_data(screen_t *screen, void *user_data);
/* 获取页面私有数据；screen 为目标页面，返回此前设置的 user_data。 */
void     *screen_get_user_data(screen_t *screen);

#ifdef __cplusplus
}
#endif

#endif
