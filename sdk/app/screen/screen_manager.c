#include "basic_include.h"
#include "screen_manager.h"
#include "screen_memory.h"
#include "screen_common.h"
#include <string.h>

typedef struct
{
    screen_t           *stack[SCREEN_STACK_MAX_DEPTH];
    screen_t           *pending_destroy;
    screen_t           *registry[SCREEN_STACK_MAX_DEPTH];
    screen_transition_t transition;
    int16_t             reg_count;
    int16_t             top;
    uint32_t            next_id;
    uint8_t             animating;
} screen_manager_t;

static screen_manager_t *manager;

#ifndef SCREEN_MANAGER_DEBUG
#define SCREEN_MANAGER_DEBUG 1
#endif

#ifndef SCREEN_MANAGER_LOG
#define SCREEN_MANAGER_LOG(...) os_printf(__VA_ARGS__)
#endif

#if SCREEN_MANAGER_DEBUG
static void screen_manager_log_callback(const char *callback, screen_t *screen)
{
    if (!screen)
    {
        return;
    }

    if (screen->name)
    {
        SCREEN_MANAGER_LOG("screen callback: %s id=%u name=%s\r\n", callback, screen->id, screen->name);
    }
    else
    {
        SCREEN_MANAGER_LOG("screen callback: %s id=%u\r\n", callback, screen->id);
    }
}
#define SCREEN_MANAGER_LOG_CALLBACK(callback, screen) screen_manager_log_callback((callback), (screen))
#else
#define SCREEN_MANAGER_LOG_CALLBACK(callback, screen) ((void) 0)
#endif

/* ── lifecycle helpers ─────────────────────────────────────────── */

/* 触发 on_create：只允许 NONE -> CREATED，params 会透传给页面创建回调。 */
static void call_on_create(screen_t *s, void *params)
{
    if (s->state != SCREEN_STATE_NONE)
    {
        return;
    }
    s->state = SCREEN_STATE_CREATED;
    if (s->lifecycle.on_create)
    {
        SCREEN_MANAGER_LOG_CALLBACK("on_create", s);
        if (s->cb)
        {
            s->cb(s->id, CALLBACK_CMD_CREATE, s->cb_priv, 0);
        }
        s->lifecycle.on_create(s, params);
    }
}

/* 触发 on_start：页面进入可见阶段，s 为目标页面。 */
static void call_on_start(screen_t *s)
{
    if (s->state == SCREEN_STATE_NONE)
    {
        return;
    }
    s->state = SCREEN_STATE_STARTED;
    if (s->lifecycle.on_start)
    {
        SCREEN_MANAGER_LOG_CALLBACK("on_start", s);
        if (s->cb)
        {
            s->cb(s->id, CALLBACK_CMD_START, s->cb_priv, 0);
        }
        s->lifecycle.on_start(s);
    }
}

/* 触发 on_resume：页面成为栈顶并可交互，s 必须处于 STARTED。 */
static void call_on_resume(screen_t *s)
{
    if (s->state != SCREEN_STATE_STARTED)
    {
        return;
    }
    s->state = SCREEN_STATE_RESUMED;
    if (s->lifecycle.on_resume)
    {
        SCREEN_MANAGER_LOG_CALLBACK("on_resume", s);
        if (s->cb)
        {
            s->cb(s->id, CALLBACK_CMD_RESUME, s->cb_priv, 0);
        }
        s->lifecycle.on_resume(s);
    }
}

/* 触发 on_pause：页面失去焦点，s 必须处于 RESUMED。 */
static void call_on_pause(screen_t *s)
{
    if (s->state != SCREEN_STATE_RESUMED)
    {
        return;
    }
    s->state = SCREEN_STATE_PAUSED;
    if (s->lifecycle.on_pause)
    {
        SCREEN_MANAGER_LOG_CALLBACK("on_pause", s);
        if (s->cb)
        {
            s->cb(s->id, CALLBACK_CMD_PAUSE, s->cb_priv, 0);
        }
        s->lifecycle.on_pause(s);
    }
}

/* 触发 on_stop：页面退出前台并不可见，s 必须处于 PAUSED。 */
static void call_on_stop(screen_t *s)
{
    if (s->state != SCREEN_STATE_PAUSED)
    {
        return;
    }
    s->state = SCREEN_STATE_STOPPED;
    if (s->lifecycle.on_stop)
    {
        SCREEN_MANAGER_LOG_CALLBACK("on_stop", s);
        if (s->cb)
        {
            s->cb(s->id, CALLBACK_CMD_STOP, s->cb_priv, 0);
        }
        s->lifecycle.on_stop(s);
    }
}

/* 触发 on_restart：STOPPED 页面准备重新回到前台。 */
static void call_on_restart(screen_t *s)
{
    if (s->state != SCREEN_STATE_STOPPED)
    {
        return;
    }
    if (s->lifecycle.on_restart)
    {
        SCREEN_MANAGER_LOG_CALLBACK("on_restart", s);
        if (s->cb)
        {
            s->cb(s->id, CALLBACK_CMD_RESTART, s->cb_priv, 0);
        }
        s->lifecycle.on_restart(s);
    }
}

/* 触发 on_finish：页面业务结束回调，不改变生命周期状态，且同一页面只触发一次。 */
static void call_on_finish(screen_t *s)
{
    if (!s || s->finished)
    {
        return;
    }

    s->finished = 1;
    if (s->lifecycle.on_finish)
    {
        SCREEN_MANAGER_LOG_CALLBACK("on_finish", s);
        if (s->cb)
        {
            s->cb(s->id, CALLBACK_CMD_FINISH, s->cb_priv, 0);
        }
        s->lifecycle.on_finish(s);
    }
}

static void call_on_destroy(screen_t *s)
{
    if (s->lifecycle.on_destroy)
    {
        SCREEN_MANAGER_LOG_CALLBACK("on_finish", s);
        if (s->cb)
        {
            s->cb(s->id, CALLBACK_CMD_DESTROY, s->cb_priv, s->is_extern_destroyed);
        }
        s->lifecycle.on_destroy(s);
    }
}

/* 将页面 root 移到 LVGL 绘制层最前；s 为需要显示到前台的页面。 */
static void bring_screen_to_front(screen_t *s)
{
    if (s && s->root)
    {
        lv_obj_move_foreground(s->root);
    }
}

static void show_screen_root(screen_t *s)
{
    if (s && s->root)
    {
        lv_obj_clear_flag(s->root, LV_OBJ_FLAG_HIDDEN);
    }
}

static void hide_screen_root(screen_t *s)
{
    if (s && s->root)
    {
        lv_obj_add_flag(s->root, LV_OBJ_FLAG_HIDDEN);
    }
}

/* ── transition direction helpers ──────────────────────────────── */

/* 返回转场方向的反向类型；t 为原始进入动画类型，主要用于 pop 返回动画。 */
static screen_transition_type_t trans_reverse(screen_transition_type_t t)
{
    switch (t)
    {
        case SCREEN_TRANS_SLIDE_LEFT:
            return SCREEN_TRANS_SLIDE_RIGHT;
        case SCREEN_TRANS_SLIDE_RIGHT:
            return SCREEN_TRANS_SLIDE_LEFT;
        case SCREEN_TRANS_SLIDE_UP:
            return SCREEN_TRANS_SLIDE_DOWN;
        case SCREEN_TRANS_SLIDE_DOWN:
            return SCREEN_TRANS_SLIDE_UP;
        default:
            return t;
    }
}

/* ── helpers ───────────────────────────────────────────────────── */

/* 销毁页面并从 registry 移除；s 为即将释放的页面。 */
static void destroy_screen(screen_t *s)
{
    if (!s)
    {
        return;
    }

    /* 兜底触发 on_finish，保证任何销毁路径都能先清理业务状态。 */
    call_on_finish(s);

    /* 从 registry 移除，避免 screen_find_by_id/name 返回已销毁页面。 */
    if (manager)
    {
        for (int16_t i = 0; i < manager->reg_count; i++)
        {
            if (manager->registry[i] == s)
            {
                for (int16_t j = i; j < manager->reg_count - 1; j++)
                {
                    manager->registry[j] = manager->registry[j + 1];
                }
                manager->registry[--manager->reg_count] = NULL;
                break;
            }
        }
    }
    call_on_destroy(s);
    screen_destroy(s);
}

/* ── transition complete callback ──────────────────────────────── */

/* 转场完成回调；a 为 LVGL 动画对象，完成后处理延迟销毁页面。 */
static void on_transition_done(lv_anim_t *a)
{
    (void) a;
    if (!manager)
    {
        return;
    }

    manager->animating = 0;
    if (manager->pending_destroy)
    {
        screen_t *s              = manager->pending_destroy;
        manager->pending_destroy = NULL;
        destroy_screen(s);
    }
}

/* ── public API ─────────────────────────────────────────────────── */

uint32_t screen_manager_get_next_id(void)
{
    uint32_t id;
    uint32_t flag = disable_irq();
    id            = manager->next_id++;
    enable_irq(flag);
    return id;
}

void screen_manager_init(void)
{
    if (manager)
    {
        return;
    }

    manager = SCREEN_MALLOC(sizeof(screen_manager_t));
    if (!manager)
    {
        return;
    }
    memset(manager, 0, sizeof(screen_manager_t));

    manager->top                 = -1;
    manager->animating           = 0;
    manager->reg_count           = 0;
    manager->transition.enter    = SCREEN_TRANS_NONE;
    manager->transition.exit     = SCREEN_TRANS_NONE;
    manager->transition.duration = 300;
    manager->pending_destroy     = NULL;
    manager->next_id             = 1;
}

void screen_manager_deinit(void)
{
    if (!manager)
    {
        return;
    }

    while (manager->reg_count > 0)
    {
        destroy_screen(manager->registry[0]);
    }
    SCREEN_FREE(manager);
    manager = NULL;
}

screen_t *screen_register(const char *name, screen_lifecycle_t *lifecycle)
{
    if (!manager)
    {
        return NULL;
    }
    screen_t *s;
    s = screen_find_only(lifecycle->only);
    if (s)
    {
        return s;
    }
    // 检查一下only,如果非0,代表仅仅支持一个实例
    s = screen_create(name, lifecycle);
    if (!s)
    {
        return NULL;
    }
    if (s->lifecycle.screen_id)
    {
        s->id = s->lifecycle.screen_id;
    }
    else
    {
        s->id = screen_manager_get_next_id();
    }

    if (manager->reg_count < SCREEN_STACK_MAX_DEPTH)
    {
        manager->registry[manager->reg_count++] = s;
    }
    return s;
}

void screen_unregister(screen_t *screen)
{
    if (!manager || !screen)
    {
        return;
    }

    /* 后台页被移除也视为业务结束，提前触发 on_finish。 */
    call_on_finish(screen);

    uint8_t was_top = (manager->top >= 0 && manager->stack[manager->top] == screen);

    if (was_top)
    {
        call_on_pause(screen);
        call_on_stop(screen);
    }

    for (int16_t i = 0; i <= manager->top; i++)
    {
        if (manager->stack[i] == screen)
        {
            for (int16_t j = i; j < manager->top; j++)
            {
                manager->stack[j] = manager->stack[j + 1];
            }
            manager->stack[manager->top] = NULL;
            manager->top--;
            screen->in_stack = 0;
            break;
        }
    }

    if (was_top && manager->top >= 0)
    {
        screen_t *prev = manager->stack[manager->top];
        bring_screen_to_front(prev);
        show_screen_root(prev);
        call_on_restart(prev);
        call_on_start(prev);
        call_on_resume(prev);
    }

    destroy_screen(screen);
}

void screen_push(screen_t *screen, void *params)
{
    if (!manager || !screen || manager->animating)
    {
        return;
    }

    screen_t *current = (manager->top >= 0) ? manager->stack[manager->top] : NULL;
    if (current == screen)
    {
        return;
    }

    if (screen->in_stack)
    {
        int16_t existing_idx = -1;
        for (int16_t i = 0; i <= manager->top; i++)
        {
            if (manager->stack[i] == screen)
            {
                existing_idx = i;
                break;
            }
        }

        if (existing_idx < 0)
        {
            return;
        }

        if (current)
        {
            call_on_pause(current);
            call_on_stop(current);
        }

        for (int16_t i = existing_idx; i < manager->top; i++)
        {
            manager->stack[i] = manager->stack[i + 1];
        }
        manager->stack[manager->top] = screen;

        bring_screen_to_front(screen);
        show_screen_root(screen);
        call_on_restart(screen);
        call_on_start(screen);

        if (current && manager->transition.enter != SCREEN_TRANS_NONE)
        {
            manager->animating = 1;
            screen_transition_start(current, screen, &manager->transition, on_transition_done);
        }

        call_on_resume(screen);
        return;
    }

    if (manager->top >= SCREEN_STACK_MAX_DEPTH - 1)
    {
        return;
    }

    if (current)
    {
        call_on_pause(current);
        call_on_stop(current);
    }

    call_on_create(screen, params);
    call_on_restart(screen);
    call_on_start(screen);

    manager->top++;
    manager->stack[manager->top] = screen;
    screen->in_stack             = 1;

    bring_screen_to_front(screen);
    show_screen_root(screen);

    if (current && manager->transition.enter != SCREEN_TRANS_NONE)
    {
        manager->animating = 1;
        screen_transition_start(current, screen, &manager->transition, on_transition_done);
    }

    call_on_resume(screen);
}

void screen_pop(void)
{
    if (!manager || manager->top < 0 || manager->animating)
    {
        return;
    }

    screen_t *current = manager->stack[manager->top];

    if (current)
    {
        call_on_pause(current);
    }

    manager->stack[manager->top] = NULL;
    manager->top--;

    if (current)
    {
        current->in_stack = 0;
    }

    screen_t *previous = (manager->top >= 0) ? manager->stack[manager->top] : NULL;

    if (previous)
    {
        bring_screen_to_front(previous);
        show_screen_root(previous);

        call_on_restart(previous);
        call_on_start(previous);
        call_on_resume(previous);

        if (current)
        {
            call_on_stop(current);
        }

        if (manager->transition.enter != SCREEN_TRANS_NONE)
        {
            screen_transition_t rev = manager->transition;
            rev.enter               = trans_reverse(rev.enter);
            manager->animating      = 1;
            if (!current->keep_alive)
            {
                manager->pending_destroy = current;
            }
            screen_transition_start(current, previous, &rev, on_transition_done);
        }
        else
        {
            hide_screen_root(current);
            if (!current->keep_alive)
            {
                destroy_screen(current);
            }
        }
    }
    else
    {
        if (current)
        {
            call_on_stop(current);
        }
        if (!current->keep_alive)
        {
            destroy_screen(current);
        }
    }
}

/*
 * 弹出到指定页面。
 * screen: 目标页面，必须已经在栈内。
 * destroy_popped: 非 0 时允许销毁 keep_alive=0 的被移出栈页面；为 0 时全部后台保留。
 */
static void pop_to_screen(screen_t *screen, uint8_t destroy_popped)
{
    if (!manager || !screen || manager->top < 0)
    {
        return;
    }
    if (manager->stack[manager->top] == screen)
    {
        return;
    }

    screen_t *current = manager->stack[manager->top];
    if (current)
    {
        call_on_pause(current);
        call_on_stop(current);
        manager->stack[manager->top] = NULL;
        manager->top--;
        hide_screen_root(current);
        current->in_stack = 0;
        if (destroy_popped && !current->keep_alive)
        {
            destroy_screen(current);
        }
    }

    while (manager->top >= 0 && manager->stack[manager->top] != screen)
    {
        screen_t *s                  = manager->stack[manager->top];
        manager->stack[manager->top] = NULL;
        manager->top--;
        hide_screen_root(s);
        s->in_stack = 0;
        if (destroy_popped && !s->keep_alive)
        {
            destroy_screen(s);
        }
    }

    /* screen is now on top; bring it back to RESUMED */
    if (manager->top >= 0 && manager->stack[manager->top] == screen)
    {
        show_screen_root(screen);
        call_on_restart(screen);
        call_on_start(screen);
        call_on_resume(screen);
    }
}

void screen_pop_to(screen_t *screen)
{
    pop_to_screen(screen, 0);
}

void screen_pop_to_root(void)
{
    if (!manager)
    {
        return;
    }

    if (manager->top > 0)
    {
        screen_t *root = manager->stack[0];
        screen_pop_to(root);
    }
}

void screen_pop_to_destroy(screen_t *screen)
{
    pop_to_screen(screen, 1);
}

void screen_pop_to_root_destroy(void)
{
    if (!manager)
    {
        return;
    }

    if (manager->top > 0)
    {
        screen_t *root = manager->stack[0];
        screen_pop_to_destroy(root);
    }
}

/*
 * 结束当前业务页面并返回上一页。
 * screen: 必须是当前栈顶；函数会先触发 on_finish，再设置 keep_alive=0 并 pop。
 */
void screen_finish(screen_t *screen)
{
    if (!manager || !screen || screen_current() != screen || manager->animating)
    {
        return;
    }

    call_on_finish(screen);
    screen->keep_alive = 0;
    screen_pop();
}

/*
 * 结束当前业务页面并返回根页面。
 * screen: 必须是当前栈顶；函数会先触发 on_finish，再设置 keep_alive=0 并 pop_to_root_destroy。
 */
void screen_finish_to_root(screen_t *screen)
{
    if (!manager || !screen || screen_current() != screen || manager->animating)
    {
        return;
    }
    if (manager->top <= 0)
    {
        return;
    }

    call_on_finish(screen);
    screen->keep_alive = 0;
    screen_pop_to_root_destroy();
}

void screen_replace(screen_t *screen, void *params)
{
    if (!manager || !screen || manager->top < 0 || manager->animating)
    {
        return;
    }

    screen_t *current = manager->stack[manager->top];

    call_on_pause(current);
    call_on_stop(current);

    manager->stack[manager->top] = screen;
    screen->in_stack             = 1;
    current->in_stack            = 0;

    call_on_create(screen, params);
    call_on_restart(screen);
    call_on_start(screen);
    bring_screen_to_front(screen);
    show_screen_root(screen);

    if (manager->transition.enter != SCREEN_TRANS_NONE)
    {
        manager->animating = 1;
        if (!current->keep_alive)
        {
            manager->pending_destroy = current;
        }
        screen_transition_start(current, screen, &manager->transition, on_transition_done);
    }
    else
    {
        hide_screen_root(current);
        if (!current->keep_alive)
        {
            destroy_screen(current);
        }
    }

    call_on_resume(screen);
}

void screen_set_default_transition(screen_transition_t *transition)
{
    if (manager && transition)
    {
        lv_memcpy(&manager->transition, transition, sizeof(screen_transition_t));
    }
}

screen_t *screen_current(void)
{
    if (!manager)
    {
        return NULL;
    }

    if (manager->top >= 0)
    {
        return manager->stack[manager->top];
    }
    return NULL;
}

screen_t *screen_find_by_id(uint16_t id)
{
    if (!manager)
    {
        return NULL;
    }

    for (int16_t i = 0; i < manager->reg_count; i++)
    {
        if (manager->registry[i] && manager->registry[i]->id == id)
        {
            return manager->registry[i];
        }
    }
    return NULL;
}

screen_t *screen_find_only(uint32_t only)
{
    if (!manager)
    {
        return NULL;
    }
    if (!only)
    {
        return NULL;
    }

    for (int16_t i = 0; i < manager->reg_count; i++)
    {
        if (manager->registry[i]->only == only)
        {
            return manager->registry[i];
        }
    }
    return NULL;
}

int32_t screen_pop_id(uint32_t id)
{
    screen_t *screen = screen_find_by_id(id);
    os_printf("screen:%X\n", screen);
    if (screen)
    {
        screen->is_extern_destroyed = 1;
        screen_finish(screen);
        return 0;
    }
    else
    {
        os_printf("%s:%d\tno found screen id:%d\n", __func__, __LINE__, id);
    }
    return -1;
}