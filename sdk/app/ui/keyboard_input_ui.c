#include "basic_include.h"
#include "ui_manager.h"

#include "screen_manager.h"
#include "screen_memory.h"
#include "screen_common.h"

#include "ui_theme.h"
#include "lvgl/lvgl.h"

extern lv_style_t  g_style;
extern lv_indev_t *indev_keypad;

/****************************************************************************
 * 文件名：keyboard_input_ui.c
 *
 * UI 整体结构（自上而下）：
 *   ┌────────────────────────────────────────┐
 *   │ [ < 返回 ]        输入密码              │  ← 顶部 header
 *   ├────────────────────────────────────────┤
 *   │  请输入密码                             │  ← 文本输入框
 *   ├────────────────────────────────────────┤
 *   │  [a][b][c][d][e][f][g][h]              │  ← LVGL 内置键盘
 *   │  [i][j][k][l][m][n][o][p]              │
 *   │  [q][r][s][t][u][v][w][x]              │
 *   │  [y][z]                                │
 *   └────────────────────────────────────────┘
 *
 * 适配 Screen Manager 框架：
 *   - ctx 放在 screen->user_data（owns_self 控制释放）
 *   - 返回按钮：screen_finish(ui_s->screen)
 *   - 支持物理按键（group / indev_keypad）
 *
 * 入口：keyboard_input_ui_create()
 *   直接创建并压栈键盘输入页面 Screen
 ****************************************************************************/

/* ===== 文案常量：集中管理用户可见字符串，方便后续调整 ===== */
static const char *const kb_str_back = "返回";
static const char *const kb_str_hint = "请输入密码";

/* ===== 颜色宏：统一管理本页面用到的所有颜色 ===== */
#define KB_BG_DARK      0x000000 /* 屏幕根背景（黑色） */
#define KB_HEADER_BG    0xFFCC33 /* 顶部 header 背景 */
#define KB_HEADER_TEXT  0x000000 /* header 上文字色（黑色） */
#define KB_INPUT_BG     0xFFFFFF /* 输入框背景 */
#define KB_INPUT_BORDER 0x333333 /* 输入框边框 */
#define KB_INPUT_TEXT   0x000000 /* 输入框文字色 */
#define KB_HINT_TEXT    0x000000 /* 提示文字灰色 */
#define KB_KB_BG        0x222222 /* 键盘背景 */
#define KB_KB_CONFIRM_ACTIVE 0x4CAF50 /* 确认按钮激活颜色（绿色） */
#define KB_KB_CONFIRM_DISABLED 0x888888 /* 确认按钮禁用颜色（灰色） */

/* ===== 自适应缩放宏 ===== */
#define KB_BASE_H   480
#define KB_SCALE(v) ((lv_coord_t) LV_MAX((int32_t) (v) * (LV_VER_RES) / KB_BASE_H, 2))

/* ===== 最小输入长度限制 ===== */
#define KB_MIN_INPUT_LENGTH 8

/* ===== 页面私有上下文，挂在 screen->user_data 上 ===== */
typedef struct
{
    screen_t   *screen; /* 关联的 Screen 对象 */
    lv_group_t *group;  /* 按键组 */

    /* UI 控件指针 */
    lv_obj_t *header_panel; /* 顶部 header 容器 */
    lv_obj_t *back_btn;     /* 返回按钮 */
    lv_obj_t *textarea;     /* 文本输入框 */
    lv_obj_t *keyboard;     /* LVGL 内置键盘 */

    /* 配置参数 */
    const char *title;      /* 页面标题 */
    uint8_t     min_length; /* 最小输入长度 */

    /* 输入结果回调 */
    void (*on_confirm)(void *user_data, const char *text); /* 确认回调 */
    void (*on_cancel)(void *user_data);                    /* 取消回调 */
    void *user_data;                                       /* 用户数据指针 */
} keyboard_input_ctx_t;

/* ===== 函数前置声明 ===== */
static void keyboard_input_get_lifecycle(screen_lifecycle_t *lifecycle);
static void keyboard_input_create(screen_t *screen, void *params);
static void keyboard_input_destroy(screen_t *screen);
static void keyboard_input_start(screen_t *screen);
static void keyboard_input_resume(screen_t *screen);
static void keyboard_input_pause(screen_t *screen);
static void keyboard_input_stop(screen_t *screen);
static void keyboard_input_restart(screen_t *screen);
static void keyboard_input_finish(screen_t *screen);

static void keyboard_input_on_back_click(lv_event_t *e);
static void keyboard_input_on_keyboard_ready(lv_event_t *e);
static void keyboard_input_on_textarea_changed(lv_event_t *e);
static void keyboard_input_update_confirm_btn(keyboard_input_ctx_t *ui_s);
static void keyboard_input_create_header(lv_obj_t *page, keyboard_input_ctx_t *ui_s);
static void keyboard_input_create_textarea(lv_obj_t *page, keyboard_input_ctx_t *ui_s);
static void keyboard_input_create_keyboard_area(lv_obj_t *page, keyboard_input_ctx_t *ui_s);

/****************************************************************************
 * 返回按钮点击回调：触发取消回调并退出
 ****************************************************************************/
static void keyboard_input_on_back_click(lv_event_t *e)
{
    keyboard_input_ctx_t *ui_s = (keyboard_input_ctx_t *) lv_event_get_user_data(e);
    if (ui_s == NULL || ui_s->screen == NULL)
    {
        return;
    }

    os_printf("keyboard_input: click back btn (cancel)\n");
    os_printf("ui_s:%X\tscreen:%X\n", ui_s, ui_s->screen);
    /* 调用取消回调 */
    if (ui_s->on_cancel != NULL)
    {
        os_printf("keyboard_input1: call cancel callback\n");
        os_printf("ui_s->on_cancel:%X\n", ui_s->on_cancel);
        ui_s->on_cancel(ui_s->user_data);
        os_printf("keyboard_input2: call cancel callback\n");
    }

    screen_finish(ui_s->screen);
}

/****************************************************************************
 * 键盘确认回调：当用户点击键盘上的确认键时触发
 ****************************************************************************/
static void keyboard_input_on_keyboard_ready(lv_event_t *e)
{
    keyboard_input_ctx_t *ui_s = (keyboard_input_ctx_t *) lv_event_get_user_data(e);
    lv_obj_t             *kb   = lv_event_get_target(e);
    const char           *text;
    uint32_t              len;

    if (ui_s == NULL || kb == NULL)
    {
        return;
    }

    /* 获取输入框中的文本 */
    text = lv_textarea_get_text(ui_s->textarea);
    len  = strlen(text);

    /* 检查输入长度是否满足最小要求 */
    if (len < ui_s->min_length)
    {
        os_printf("keyboard_input: input too short (%d < %d), reject\n", (int) len, (int) ui_s->min_length);
        return;
    }

    os_printf("keyboard_input: confirm input: %s\n", text);

    /* 调用确认回调 */
    if (ui_s->on_confirm != NULL)
    {
        ui_s->on_confirm(ui_s->user_data, text);
    }

    screen_finish(ui_s->screen);
}

/****************************************************************************
 * textarea 内容变化回调：更新确认按键状态
 ****************************************************************************/
static void keyboard_input_on_textarea_changed(lv_event_t *e)
{
    keyboard_input_ctx_t *ui_s = (keyboard_input_ctx_t *) lv_event_get_user_data(e);

    if (ui_s == NULL)
    {
        return;
    }

    keyboard_input_update_confirm_btn(ui_s);
}

/****************************************************************************
 * 更新确认按键状态：根据输入长度启用/禁用确认按键
 * 注意：由于 LVGL keyboard 没有公开 get_btnmatrix API，
 * 我们通过在确认回调中检查长度来实现限制
 ****************************************************************************/
static void keyboard_input_update_confirm_btn(keyboard_input_ctx_t *ui_s)
{
    const char *text;
    uint32_t    len;

    if (ui_s == NULL || ui_s->textarea == NULL)
    {
        return;
    }

    text = lv_textarea_get_text(ui_s->textarea);
    len  = strlen(text);

    /* 仅记录状态，实际限制在 on_keyboard_ready 回调中实现 */
    if (len < ui_s->min_length)
    {
        os_printf("keyboard_input: input length %d < min %d\n", (int) len, (int) ui_s->min_length);
    }
}

/****************************************************************************
 * 构建顶部 header：包含左侧"< 返回"按钮和居中标题
 ****************************************************************************/
static void keyboard_input_create_header(lv_obj_t *page, keyboard_input_ctx_t *ui_s)
{
    lv_obj_t *header_panel;
    lv_obj_t *back_btn;
    lv_obj_t *back_icon;
    lv_obj_t *back_label;
    lv_obj_t *title_label;

    if (page == NULL || ui_s == NULL)
    {
        return;
    }

    /* header 容器 */
    header_panel = lv_obj_create(page);
    if (header_panel == NULL)
    {
        return;
    }
    lv_obj_remove_style_all(header_panel);
    lv_obj_set_size(header_panel, LV_PCT(96), KB_SCALE(50));
    lv_obj_set_style_radius(header_panel, KB_SCALE(25), 0);
    lv_obj_align(header_panel, LV_ALIGN_TOP_MID, 0, KB_SCALE(10));
    lv_obj_set_style_bg_color(header_panel, lv_color_hex(KB_HEADER_BG), 0);
    lv_obj_set_style_bg_opa(header_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header_panel, 0, 0);
    lv_obj_set_style_shadow_width(header_panel, 0, 0);
    lv_obj_clear_flag(header_panel, LV_OBJ_FLAG_SCROLLABLE);
    ui_s->header_panel = header_panel;

    /* 左上角"返回"按钮："< 返回" */
    back_btn = lv_obj_create(header_panel);
    if (back_btn != NULL)
    {
        lv_obj_remove_style_all(back_btn);
        lv_obj_set_size(back_btn, KB_SCALE(110), KB_SCALE(40));
        lv_obj_set_style_radius(back_btn, KB_SCALE(20), 0);
        lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(back_btn, 0, 0);
        lv_obj_set_style_shadow_width(back_btn, 0, 0);
        lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_flex_flow(back_btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(back_btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(back_btn, KB_SCALE(4), 0);
        lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, KB_SCALE(8), 0);
        lv_obj_add_event_cb(back_btn, keyboard_input_on_back_click, LV_EVENT_CLICKED, ui_s);
        ui_s->back_btn = back_btn;

        /* "<" 左箭头图标 */
        back_icon = lv_label_create(back_btn);
        if (back_icon != NULL)
        {
            lv_label_set_text(back_icon, LV_SYMBOL_LEFT);
            lv_obj_set_style_text_font(back_icon, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(back_icon, lv_color_hex(KB_HEADER_TEXT), 0);
        }

        /* "返回" 文字 */
        back_label = lv_label_create(back_btn);
        if (back_label != NULL)
        {
            lv_label_set_text(back_label, kb_str_back);
            lv_obj_set_style_text_font(back_label, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(back_label, lv_color_hex(KB_HEADER_TEXT), 0);
        }
    }

    /* 居中标题 */
    title_label = lv_label_create(header_panel);
    if (title_label != NULL)
    {
        lv_label_set_text(title_label, ui_s->title);
        lv_obj_set_style_text_font(title_label, UI_FONT_BODY, 0);
        lv_obj_set_style_text_color(title_label, lv_color_hex(KB_HEADER_TEXT), 0);
        lv_obj_align(title_label, LV_ALIGN_CENTER, KB_SCALE(40), 0);
    }
}

/****************************************************************************
 * 构建文本输入框
 ****************************************************************************/
static void keyboard_input_create_textarea(lv_obj_t *page, keyboard_input_ctx_t *ui_s)
{
    lv_obj_t *textarea;
    lv_obj_t *label;

    if (page == NULL || ui_s == NULL)
    {
        return;
    }

    /* 创建文本输入框 */
    textarea = lv_textarea_create(page);
    if (textarea == NULL)
    {
        return;
    }

    /* 设置输入框属性 */
    lv_textarea_set_placeholder_text(textarea, kb_str_hint);
    lv_textarea_set_one_line(textarea, true);
    lv_textarea_set_password_bullet(textarea, "*");
    lv_textarea_set_password_mode(textarea, true);      /* 密码模式，显示星号 */
    lv_textarea_set_password_show_time(textarea, 1000); /* 密码显示时间 1 秒 */
    lv_textarea_set_max_length(textarea, 32);           /* 最大输入长度 */
    
    lv_textarea_set_align(textarea, LV_TEXT_ALIGN_LEFT);

    /* 设置输入框样式 - 使用 LV_PART_MAIN 确保覆盖默认主题 */
    lv_obj_set_size(textarea, LV_PCT(90), KB_SCALE(50));
    lv_obj_align(textarea, LV_ALIGN_TOP_MID, 0, KB_SCALE(70));

    /* 背景和边框 */
    lv_obj_set_style_bg_color(textarea, lv_color_hex(KB_INPUT_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(textarea, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(textarea, lv_color_hex(KB_INPUT_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(textarea, KB_SCALE(2), LV_PART_MAIN);
    lv_obj_set_style_radius(textarea, KB_SCALE(8), LV_PART_MAIN);

    /* 文字样式 - 在 LV_PART_MAIN 上设置，确保内部 label 继承 */
    lv_obj_set_style_text_color(textarea, lv_color_hex(KB_INPUT_TEXT), LV_PART_MAIN);
    lv_obj_set_style_text_font(textarea, UI_FONT_BODY, LV_PART_MAIN);

    /* 内边距 */
    lv_obj_set_style_pad_left(textarea, KB_SCALE(10), LV_PART_MAIN);
    lv_obj_set_style_pad_right(textarea, KB_SCALE(10), LV_PART_MAIN);
    lv_obj_set_style_pad_top(textarea, KB_SCALE(5), LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(textarea, KB_SCALE(5), LV_PART_MAIN);

    /* 设置 textarea 内部 label 的样式，确保文字可见 */
    label = lv_textarea_get_label(textarea);
    if (label != NULL)
    {
        lv_obj_set_style_text_color(label, lv_color_hex(KB_INPUT_TEXT), 0);
        lv_obj_set_style_text_font(label, UI_FONT_BODY, 0);
        lv_obj_set_style_pad_left(label, 0, 0);
        lv_obj_set_style_pad_right(label, 0, 0);
    }

    /* 监听 textarea 内容变化事件，用于更新确认按钮状态 */
    lv_obj_add_event_cb(textarea, keyboard_input_on_textarea_changed, LV_EVENT_VALUE_CHANGED, ui_s);

    ui_s->textarea = textarea;
}

/****************************************************************************
 * 构建 LVGL 内置键盘区域
 ****************************************************************************/
static void keyboard_input_create_keyboard_area(lv_obj_t *page, keyboard_input_ctx_t *ui_s)
{
    lv_obj_t *keyboard;

    if (page == NULL || ui_s == NULL)
    {
        return;
    }

    /* 创建 LVGL 内置键盘 */
    keyboard = lv_keyboard_create(page);
    if (keyboard == NULL)
    {
        return;
    }

    /* 将键盘与输入框关联 */
    lv_keyboard_set_textarea(keyboard, ui_s->textarea);

    /* 设置键盘大小和位置 */
    lv_obj_set_size(keyboard, LV_PCT(95), LV_VER_RES - KB_SCALE(140));
    lv_obj_align(keyboard, LV_ALIGN_TOP_MID, 0, KB_SCALE(130));

    /* 设置键盘样式 */
    lv_obj_set_style_bg_color(keyboard, lv_color_hex(KB_KB_BG), 0);
    lv_obj_set_style_bg_opa(keyboard, LV_OPA_COVER, 0);

    /* 添加确认事件回调 */
    lv_obj_add_event_cb(keyboard, keyboard_input_on_keyboard_ready, LV_EVENT_READY, ui_s);

    ui_s->keyboard = keyboard;
}

/****************************************************************************
 * on_create：首次进入页面时调用
 ****************************************************************************/
static void keyboard_input_create(screen_t *screen, void *params)
{
    keyboard_input_ctx_t *ui_s;
    keyboard_input_ctx_t *init_params = (keyboard_input_ctx_t *) params;

    if (screen == NULL)
    {
        return;
    }

    /* 申请上下文内存 */
    ui_s = (keyboard_input_ctx_t *) SCREEN_MALLOC(sizeof(keyboard_input_ctx_t));
    if (ui_s == NULL)
    {
        return;
    }
    memset(ui_s, 0, sizeof(keyboard_input_ctx_t));
    ui_s->screen = screen;

    /* 复制初始化参数中的回调函数和配置 */
    if (init_params != NULL)
    {
        ui_s->on_confirm = init_params->on_confirm;
        ui_s->on_cancel  = init_params->on_cancel;
        ui_s->user_data  = init_params->user_data;
        ui_s->title      = init_params->title;
        ui_s->min_length = init_params->min_length;
    }

    /* 如果标题为空，使用默认标题 */
    if (ui_s->title == NULL)
    {
        ui_s->title = kb_str_hint;
    }

    /* 如果最小长度为0，使用默认值 KB_MIN_INPUT_LENGTH */
    if (ui_s->min_length == 0)
    {
        ui_s->min_length = KB_MIN_INPUT_LENGTH;
    }

    screen_set_user_data(screen, ui_s);

    /* 创建 group */
    ui_s->group = lv_group_create();
    if (ui_s->group != NULL && indev_keypad != NULL)
    {
        lv_indev_set_group(indev_keypad, ui_s->group);
    }

    /* 配置根容器：全屏黑色背景 */
    lv_obj_add_style(screen->root, &g_style, 0);
    lv_obj_set_size(screen->root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(screen->root, lv_color_hex(KB_BG_DARK), 0);
    lv_obj_set_style_bg_opa(screen->root, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen->root, LV_OBJ_FLAG_SCROLLABLE);

    /* 顶部 header */
    keyboard_input_create_header(screen->root, ui_s);

    /* 文本输入框 */
    keyboard_input_create_textarea(screen->root, ui_s);

    /* LVGL 内置键盘 */
    keyboard_input_create_keyboard_area(screen->root, ui_s);

    /* 将返回按钮添加到 group */
    if (ui_s->group != NULL && ui_s->back_btn != NULL)
    {
        lv_group_add_obj(ui_s->group, ui_s->back_btn);
    }
}

/****************************************************************************
 * on_start：页面即将可见时调用
 ****************************************************************************/
static void keyboard_input_start(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_resume：页面成为栈顶可交互时调用
 ****************************************************************************/
static void keyboard_input_resume(screen_t *screen)
{
    keyboard_input_ctx_t *ui_s;

    if (screen == NULL)
    {
        return;
    }

    ui_s = (keyboard_input_ctx_t *) screen_get_user_data(screen);
    if (ui_s != NULL && ui_s->group != NULL && indev_keypad != NULL)
    {
        lv_indev_set_group(indev_keypad, ui_s->group);
    }
}

/****************************************************************************
 * on_pause：页面失去焦点时调用
 ****************************************************************************/
static void keyboard_input_pause(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_stop：页面不可见 / 退出前台时调用
 ****************************************************************************/
static void keyboard_input_stop(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_destroy：页面对象销毁时调用
 ****************************************************************************/
static void keyboard_input_destroy(screen_t *screen)
{
    keyboard_input_ctx_t *ui_s;

    if (screen == NULL)
    {
        return;
    }
    ui_s = (keyboard_input_ctx_t *) screen_get_user_data(screen);
    if (ui_s == NULL)
    {
        return;
    }

    /* 销毁 group */
    if (ui_s->group != NULL)
    {
        lv_group_del(ui_s->group);
        ui_s->group = NULL;
    }

    /* 清空 ctx 内部指针 */
    ui_s->screen       = NULL;
    ui_s->header_panel = NULL;
    ui_s->back_btn     = NULL;
    ui_s->textarea     = NULL;
    ui_s->keyboard     = NULL;

    SCREEN_FREE(ui_s);
    screen_set_user_data(screen, NULL);
}

/****************************************************************************
 * on_restart：STOPPED → 重新前台前调用
 ****************************************************************************/
static void keyboard_input_restart(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_finish：screen_finish() 调用时触发
 ****************************************************************************/
static void keyboard_input_finish(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * 组装生命周期回调集合
 ****************************************************************************/
static void keyboard_input_get_lifecycle(screen_lifecycle_t *lifecycle)
{
    if (lifecycle == NULL)
    {
        return;
    }

    *lifecycle = (screen_lifecycle_t) {
            .on_create  = keyboard_input_create,
            .on_start   = keyboard_input_start,
            .on_resume  = keyboard_input_resume,
            .on_pause   = keyboard_input_pause,
            .on_stop    = keyboard_input_stop,
            .on_destroy = keyboard_input_destroy,
            .on_restart = keyboard_input_restart,
            .on_finish  = keyboard_input_finish,
            .screen_id  = 0,
            .only       = 0,
    };
}

/****************************************************************************
 * 外部入口：直接创建并压栈键盘输入页面
 * 用法示例：
 *   keyboard_input_ui_create("输入密码", 8, on_confirm, on_cancel, user_data);
 ****************************************************************************/
void keyboard_input_ui_create(const char *title, uint8_t min_length, void (*on_confirm)(void *user_data, const char *text), void (*on_cancel)(void *user_data), void *user_data)
{
    screen_lifecycle_t   lifecycle;
    screen_t            *screen;
    keyboard_input_ctx_t init_params;

    os_printf("keyboard_input: create and push keyboard_input screen\n");

    /* 设置初始化参数 */
    memset(&init_params, 0, sizeof(keyboard_input_ctx_t));
    init_params.title      = title;
    init_params.min_length = min_length;
    init_params.on_confirm = on_confirm;
    init_params.on_cancel  = on_cancel;
    init_params.user_data  = user_data;

    keyboard_input_get_lifecycle(&lifecycle);
    screen = screen_register("keyboard_input", &lifecycle);
    if (screen != NULL)
    {
        screen_push(screen, &init_params);
    }
}

/****************************************************************************
 * 外部入口：获取输入框中的文本（用于外部获取输入结果）
 ****************************************************************************/
const char *keyboard_input_get_text(keyboard_input_ctx_t *ui_s)
{
    if (ui_s == NULL || ui_s->textarea == NULL)
    {
        return "";
    }
    return lv_textarea_get_text(ui_s->textarea);
}
