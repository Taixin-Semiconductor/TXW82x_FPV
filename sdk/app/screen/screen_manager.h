#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include "lvgl.h"
#include "screen.h"
#include "screen_transition.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_STACK_MAX_DEPTH 16

/* 初始化屏幕管理器；必须在注册和导航页面前调用。 */
void screen_manager_init(void);
/* 反初始化屏幕管理器；销毁 registry 中仍存在的 Screen。 */
void screen_manager_deinit(void);

/* 注册页面；name 为可选名称，lifecycle 为回调集合，返回已分配唯一 ID 的 Screen。 */
screen_t *screen_register(const char *name, screen_lifecycle_t *lifecycle);
/* 注销并销毁指定页面；如果它在栈内，会先从栈中移除。 */
void screen_unregister(screen_t *screen);

/* 将页面压入栈顶；screen 为目标页面，params 会传给首次 on_create。 */
void screen_push(screen_t *screen, void *params);
/* 弹出当前栈顶页面；按 Android 风格先恢复上一页，再 stop/destroy 当前页。 */
void screen_pop(void);
/* 弹出到指定页面；screen 为目标页面，被移出栈的页面隐藏并后台保留。 */
void screen_pop_to(screen_t *screen);
/* 弹出到栈底根页面；被移出栈的页面隐藏并后台保留。 */
void screen_pop_to_root(void);
/* 弹出到指定页面；仅销毁 keep_alive=0 的被移出栈页面。 */
void screen_pop_to_destroy(screen_t *screen);
/* 弹出到栈底根页面；仅销毁 keep_alive=0 的被移出栈页面。 */
void screen_pop_to_root_destroy(void);
/* 当前页面业务结束并返回上一页；screen 必须是当前栈顶，会先触发 on_finish。 */
void screen_finish(screen_t *screen);
/* 当前页面业务结束并返回根页面；screen 必须是当前栈顶，会先触发 on_finish。 */
void screen_finish_to_root(screen_t *screen);
/* 替换当前栈顶页面；screen 为新页面，params 会传给首次 on_create。 */
void screen_replace(screen_t *screen, void *params);

/* 设置默认转场动画；transition 为进入/退出动画和时长配置。 */
void screen_set_default_transition(screen_transition_t *transition);

/* 获取当前栈顶页面；栈为空时返回 NULL。 */
screen_t *screen_current(void);
/* 按唯一 ID 查找已注册页面；id 为 screen_register 分配的页面 ID。 */
screen_t *screen_find_by_id(uint16_t id);
/* 按名称查找已注册页面；name 为 screen_register 时传入的名称。 */
screen_t *screen_find_only(uint32_t only);

int32_t screen_pop_id(uint32_t id);

#ifdef __cplusplus
}
#endif

#endif
