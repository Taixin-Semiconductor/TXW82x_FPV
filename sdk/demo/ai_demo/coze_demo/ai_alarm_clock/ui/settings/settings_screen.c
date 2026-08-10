/****************************************************************************
 * 文件名：settings_ui.c
 *
 * UI 整体结构（自上而下）：
 *   ┌────────────────────────────────────────┐
 *   │ [ < 返回 ]            设置              │  ← 顶部黄色 header
 *   ├────────────────────────────────────────┤
 *   │  📶 WIFI设置                           │  ← 设置列表项
 *   │  🔵 蓝牙设置                           │  ← 设置列表项
 *   │  ☀ 屏幕亮度                           │  ← 设置列表项
 *   │  🌙 定时休眠                           │  ← 设置列表项
 *   └────────────────────────────────────────┘
 *
 * 适配 Screen Manager 框架：
 *   - ctx 放在 screen->user_data（owns_self 控制释放）
 *   - 返回按钮：screen_finish(ui_s->screen)
 *   - 本期不支持物理按键（无 group / indev_keypad）
 *
 * 入口：settings_ui_create()
 *   直接创建并压栈设置页面 Screen
 ****************************************************************************/

#include "basic_include.h"
#include "ui_manager.h"

#include "screen_manager.h"
#include "screen_memory.h"
#include "screen_common.h"

#include "ui_theme.h"
#include "lvgl/lvgl.h"

extern lv_style_t g_style;

/* ===== 文案常量：集中管理用户可见字符串，方便后续调整 ===== */
static const char *const settings_str_title = "设置";
static const char *const settings_str_back  = "返回";

/* ===== 颜色宏：统一管理本页面用到的所有颜色 ===== */
#define SETTINGS_BG_DARK       0x000000  /* 屏幕根背景（黑色） */
#define SETTINGS_HEADER_BG     0xFFCC33  /* 顶部黄色 header 背景 */
#define SETTINGS_HEADER_TEXT   0x000000  /* header 上文字色（黑色） */
#define SETTINGS_LIST_BG       0x1A1A1A  /* 列表项背景（深色） */
#define SETTINGS_LIST_ACTIVE   0xFFCC33  /* 选中列表项背景（黄色） */
#define SETTINGS_TEXT_WHITE    0xFFFFFF  /* 白色文字 */
#define SETTINGS_TEXT_BLACK    0x000000  /* 黑色文字（选中时） */
#define SETTINGS_ICON_COLOR    0x4CAF50  /* 图标颜色（绿色） */

/* ===== 设置列表数据：静态数组定义 ===== */
typedef void (*settings_item_cb_t)(void); /* 列表项点击回调类型 */

typedef struct
{
    const char     *name;        /* 设置项名称 */
    const char     *icon;        /* 图标（LVGL symbol） */
    settings_item_cb_t callback; /* 点击回调函数 */
} settings_item_t;



/* 前向声明回调函数 */
static void settings_on_wifi_click(void);
static void settings_on_bluetooth_click(void);
static void settings_on_brightness_click(void);
static void settings_on_sleep_click(void);

/* 设置列表固定数组：包含名称、图标、回调函数 */
static const settings_item_t settings_list_data[] = {
        {"WIFI设置", LV_SYMBOL_WIFI, settings_on_wifi_click},
        {"蓝牙设置", LV_SYMBOL_LOOP, settings_on_bluetooth_click},
        {"屏幕亮度", LV_SYMBOL_IMAGE, settings_on_brightness_click},
        {"定时休眠", LV_SYMBOL_POWER, settings_on_sleep_click},

        {"百度网盘", LV_SYMBOL_WIFI, NULL},
        {"音量设置", LV_SYMBOL_LOOP, NULL},
        {"勿扰设置", LV_SYMBOL_IMAGE, NULL},
        {"夜间息屏模式", LV_SYMBOL_POWER, NULL},

        {"微信绑定设置", LV_SYMBOL_WIFI, NULL},
        {"关于设备", LV_SYMBOL_LOOP, NULL},
        {"系统升级", LV_SYMBOL_IMAGE, NULL},
        {"恢复出厂设置", LV_SYMBOL_POWER, NULL},
};
#define SETTINGS_LIST_COUNT (sizeof(settings_list_data) / sizeof(settings_list_data[0]))

/* ===== 自适应缩放宏 ===== */
#define SETTINGS_BASE_H   480
#define SETTINGS_SCALE(v) ((lv_coord_t) LV_MAX((int32_t) (v) * (LV_VER_RES) / SETTINGS_BASE_H, 2))

/* ===== 页面私有上下文，挂在 screen->user_data 上 ===== */
typedef struct
{
    screen_t *screen;           /* 关联的 Screen 对象 */

    /* UI 控件指针 */
    lv_obj_t *header_panel;     /* 顶部黄色 header 容器 */
    lv_obj_t *back_btn;         /* 返回按钮 */
    lv_obj_t *settings_list;    /* 设置列表容器 */

} settings_ctx_t;

/* ===== 函数前置声明 ===== */
static void settings_get_lifecycle(screen_lifecycle_t *lifecycle);
static void settings_create(screen_t *screen, void *params);
static void settings_destroy(screen_t *screen);
static void settings_start(screen_t *screen);
static void settings_resume(screen_t *screen);
static void settings_pause(screen_t *screen);
static void settings_stop(screen_t *screen);
static void settings_restart(screen_t *screen);
static void settings_finish(screen_t *screen);

static void settings_on_back_click(lv_event_t *e);
static void settings_on_item_click(lv_event_t *e);

static void settings_create_header(lv_obj_t *page, settings_ctx_t *ui_s);
static void settings_create_list(lv_obj_t *page, settings_ctx_t *ui_s);

/* ===== 各设置项点击回调函数（占位，后续可扩展跳转逻辑） ===== */
static void settings_on_wifi_click(void)
{
    os_printf("settings: click WIFI设置\n");
    /* TODO: 后续可扩展跳转到 WIFI 设置页面 */
    extern void wifi_setting_ui_create(void);
    wifi_setting_ui_create();
}

static void settings_on_bluetooth_click(void)
{
    os_printf("settings: click 蓝牙设置\n");
    /* TODO: 后续可扩展跳转到蓝牙设置页面 */
}

static void settings_on_brightness_click(void)
{
    os_printf("settings: click 屏幕亮度\n");
    /* TODO: 后续可扩展跳转到屏幕亮度设置页面 */
}

static void settings_on_sleep_click(void)
{
    os_printf("settings: click 定时休眠\n");
    /* TODO: 后续可扩展跳转到定时休眠设置页面 */
}

/****************************************************************************
 * 返回按钮点击回调：触发 screen_finish()
 ****************************************************************************/
static void settings_on_back_click(lv_event_t *e)
{
    settings_ctx_t *ui_s = (settings_ctx_t *) lv_event_get_user_data(e);
    if (ui_s == NULL || ui_s->screen == NULL)
    {
        return;
    }
    os_printf("settings: click back btn\n");
    screen_finish(ui_s->screen);
}

/****************************************************************************
 * 设置列表项点击回调：根据索引调用对应的回调函数
 ****************************************************************************/
static void settings_on_item_click(lv_event_t *e)
{
    uint32_t index = (uint32_t)(uintptr_t) lv_event_get_user_data(e);

    if (index < SETTINGS_LIST_COUNT && settings_list_data[index].callback != NULL)
    {
        settings_list_data[index].callback();
    }
}

/****************************************************************************
 * 构建顶部黄色 header：包含左侧"< 返回"按钮和居中标题
 ****************************************************************************/
static void settings_create_header(lv_obj_t *page, settings_ctx_t *ui_s)
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

    /* 黄色胶囊状 header 容器 */
    header_panel = lv_obj_create(page);
    if (header_panel == NULL)
    {
        return;
    }
    lv_obj_remove_style_all(header_panel);
    lv_obj_set_size(header_panel, LV_PCT(96), SETTINGS_SCALE(50));
    lv_obj_set_style_radius(header_panel, SETTINGS_SCALE(25), 0);
    lv_obj_align(header_panel, LV_ALIGN_TOP_MID, 0, SETTINGS_SCALE(10));
    lv_obj_set_style_bg_color(header_panel, lv_color_hex(SETTINGS_HEADER_BG), 0);
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
        lv_obj_set_size(back_btn, SETTINGS_SCALE(110), SETTINGS_SCALE(40));
        lv_obj_set_style_radius(back_btn, SETTINGS_SCALE(20), 0);
        lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(back_btn, 0, 0);
        lv_obj_set_style_shadow_width(back_btn, 0, 0);
        lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_flex_flow(back_btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(back_btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(back_btn, SETTINGS_SCALE(4), 0);
        lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, SETTINGS_SCALE(8), 0);
        lv_obj_add_event_cb(back_btn, settings_on_back_click, LV_EVENT_CLICKED, ui_s);
        ui_s->back_btn = back_btn;

        /* "<" 左箭头图标 */
        back_icon = lv_label_create(back_btn);
        if (back_icon != NULL)
        {
            lv_label_set_text(back_icon, LV_SYMBOL_LEFT);
            lv_obj_set_style_text_font(back_icon, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(back_icon, lv_color_hex(SETTINGS_HEADER_TEXT), 0);
        }

        /* "返回" 文字 */
        back_label = lv_label_create(back_btn);
        if (back_label != NULL)
        {
            lv_label_set_text(back_label, settings_str_back);
            lv_obj_set_style_text_font(back_label, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(back_label, lv_color_hex(SETTINGS_HEADER_TEXT), 0);
        }
    }

    /* 居中标题：设置（向右偏移避开左侧返回按钮） */
    title_label = lv_label_create(header_panel);
    if (title_label != NULL)
    {
        lv_label_set_text(title_label, settings_str_title);
        lv_obj_set_style_text_font(title_label, UI_FONT_BODY, 0);
        lv_obj_set_style_text_color(title_label, lv_color_hex(SETTINGS_HEADER_TEXT), 0);
        lv_obj_align(title_label, LV_ALIGN_CENTER, SETTINGS_SCALE(40), 0);
    }
}

/****************************************************************************
 * 构建单个设置列表项：
 *   左侧：图标
 *   中间：设置项名称
 * 返回创建的列表项对象
 ****************************************************************************/
static lv_obj_t *settings_create_list_item(lv_obj_t *parent, const settings_item_t *item, uint32_t index)
{
    lv_obj_t *list_item;
    lv_obj_t *icon_label;
    lv_obj_t *name_label;

    if (parent == NULL || item == NULL)
    {
        return NULL;
    }

    /* 列表项容器 */
    list_item = lv_obj_create(parent);
    if (list_item == NULL)
    {
        return NULL;
    }
    lv_obj_remove_style_all(list_item);
    lv_obj_set_size(list_item, LV_PCT(95), SETTINGS_SCALE(50));
    lv_obj_set_style_radius(list_item, SETTINGS_SCALE(10), 0);
    lv_obj_set_style_bg_opa(list_item, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(list_item, lv_color_hex(SETTINGS_LIST_BG), 0);
    lv_obj_set_style_border_width(list_item, 0, 0);
    lv_obj_set_style_shadow_width(list_item, 0, 0);
    lv_obj_set_flex_flow(list_item, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(list_item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(list_item, SETTINGS_SCALE(12), 0);
    lv_obj_set_style_pad_left(list_item, SETTINGS_SCALE(15), 0);
    lv_obj_set_style_pad_right(list_item, SETTINGS_SCALE(15), 0);
    lv_obj_clear_flag(list_item, LV_OBJ_FLAG_SCROLLABLE);

    /* 左侧：图标 */
    icon_label = lv_label_create(list_item);
    if (icon_label != NULL)
    {
        lv_label_set_text(icon_label, item->icon);
        lv_obj_set_style_text_font(icon_label, UI_FONT_BODY, 0);
        lv_obj_set_style_text_color(icon_label, lv_color_hex(SETTINGS_ICON_COLOR), 0);
    }

    /* 中间：设置项名称 */
    name_label = lv_label_create(list_item);
    if (name_label != NULL)
    {
        lv_label_set_text(name_label, item->name);
        lv_obj_set_style_text_font(name_label, UI_FONT_BODY, 0);
        lv_obj_set_style_text_color(name_label, lv_color_hex(SETTINGS_TEXT_WHITE), 0);
        lv_obj_set_flex_grow(name_label, 1); /* 占满剩余空间 */
    }

    /* 添加点击事件：传递索引 */
    lv_obj_add_flag(list_item, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(list_item, settings_on_item_click, LV_EVENT_CLICKED, (void *)(uintptr_t) index);

    return list_item;
}

/****************************************************************************
 * 构建设置列表：遍历 settings_list_data 数组创建列表项
 ****************************************************************************/
static void settings_create_list(lv_obj_t *page, settings_ctx_t *ui_s)
{
    lv_obj_t *settings_list;
    uint32_t  i;

    if (page == NULL || ui_s == NULL)
    {
        return;
    }

    /* 列表容器（垂直方向，支持滚动） */
    settings_list = lv_obj_create(page);
    if (settings_list == NULL)
    {
        return;
    }
    lv_obj_remove_style_all(settings_list);
    lv_obj_set_size(settings_list, LV_PCT(100), LV_VER_RES - SETTINGS_SCALE(80));
    lv_obj_align(settings_list, LV_ALIGN_TOP_MID, 0, SETTINGS_SCALE(70));
    lv_obj_set_flex_flow(settings_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(settings_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(settings_list, SETTINGS_SCALE(10), 0);
    lv_obj_set_style_pad_top(settings_list, SETTINGS_SCALE(10), 0);
    lv_obj_set_scroll_dir(settings_list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(settings_list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_snap_y(settings_list, LV_SCROLL_SNAP_CENTER);
    ui_s->settings_list = settings_list;

    /* 遍历设置列表数据，创建每个列表项 */
    for (i = 0; i < SETTINGS_LIST_COUNT; i++)
    {
        settings_create_list_item(settings_list, &settings_list_data[i], i);
    }
    
}

/****************************************************************************
 * on_create：首次进入页面时调用
 *   1. 申请 ctx 上下文并挂到 screen->user_data
 *   2. 配置 screen->root 黑色背景
 *   3. 创建 header（返回按钮 + 标题）
 *   4. 创建设置列表
 ****************************************************************************/
static void settings_create(screen_t *screen, void *params)
{
    settings_ctx_t *ui_s;

    (void) params; /* 暂不使用入参 */

    if (screen == NULL)
    {
        return;
    }

    /* 申请上下文内存 */
    ui_s = (settings_ctx_t *) SCREEN_MALLOC(sizeof(settings_ctx_t));
    if (ui_s == NULL)
    {
        return;
    }
    memset(ui_s, 0, sizeof(settings_ctx_t));
    ui_s->screen = screen;
    screen_set_user_data(screen, ui_s);

    /* 配置根容器：全屏黑色背景 */
    lv_obj_add_style(screen->root, &g_style, 0);
    lv_obj_set_size(screen->root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(screen->root, lv_color_hex(SETTINGS_BG_DARK), 0);
    lv_obj_set_style_bg_opa(screen->root, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen->root, LV_OBJ_FLAG_SCROLLABLE);

    /* 顶部 header：黄色胶囊状，包含返回按钮 + 标题 */
    settings_create_header(screen->root, ui_s);

    /* 设置列表 */
    settings_create_list(screen->root, ui_s);
}

/****************************************************************************
 * on_start：页面即将可见时调用
 *   - 本页无硬件资源，空实现
 ****************************************************************************/
static void settings_start(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_resume：页面成为栈顶可交互时调用
 *   - 本页不支持物理按键，空实现
 ****************************************************************************/
static void settings_resume(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_pause：页面失去焦点时调用
 *   - 本页无硬件资源/按键钩子，空实现
 ****************************************************************************/
static void settings_pause(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_stop：页面不可见 / 退出前台时调用
 *   - 本页无硬件资源，空实现
 ****************************************************************************/
static void settings_stop(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_destroy：页面对象销毁时调用
 *   - 释放 ui_s
 *   - 不释放 LVGL 对象，由 screen_manager 统一销毁
 ****************************************************************************/
static void settings_destroy(screen_t *screen)
{
    settings_ctx_t *ui_s;

    if (screen == NULL)
    {
        return;
    }
    ui_s = (settings_ctx_t *) screen_get_user_data(screen);
    if (ui_s == NULL)
    {
        return;
    }

    /* 清空 ctx 内部指针（不释放 LVGL 对象，由 screen_manager 统一销毁） */
    ui_s->screen       = NULL;
    ui_s->header_panel = NULL;
    ui_s->back_btn     = NULL;
    ui_s->settings_list = NULL;

    SCREEN_FREE(ui_s);
    screen_set_user_data(screen, NULL);
}

/****************************************************************************
 * on_restart：STOPPED → 重新前台前调用
 ****************************************************************************/
static void settings_restart(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_finish：screen_finish() 调用时触发
 *   - 本页无业务结果回传，空实现
 ****************************************************************************/
static void settings_finish(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * 组装生命周期回调集合
 ****************************************************************************/
static void settings_get_lifecycle(screen_lifecycle_t *lifecycle)
{
    if (lifecycle == NULL)
    {
        return;
    }

    *lifecycle = (screen_lifecycle_t) {
            .on_create  = settings_create,
            .on_start   = settings_start,
            .on_resume  = settings_resume,
            .on_pause   = settings_pause,
            .on_stop    = settings_stop,
            .on_destroy = settings_destroy,
            .on_restart = settings_restart,
            .on_finish  = settings_finish,
            .screen_id  = 0,
            .only       = 0,
    };
}


/****************************************************************************
 * push 新页面到屏幕栈
 ****************************************************************************/
static void settings_push_new_screen(void)
{
    screen_lifecycle_t lifecycle;
    screen_t          *screen;

    os_printf("settings: create and push settings screen\n");
    settings_get_lifecycle(&lifecycle);
    screen = screen_register("settings", &lifecycle);
    if (screen != NULL)
    {
        screen_push(screen, NULL);
    }
}


/****************************************************************************
 * 菜单入口点击回调：触发 push 新页面
 ****************************************************************************/
static void settings_on_entry_click(lv_event_t *event)
{
    (void) event;
    settings_push_new_screen();
}



/****************************************************************************
 * 外部入口：直接创建并压栈设置页面
 * 用法示例：
 *   settings_ui_create();
 ****************************************************************************/
void settings_ui_create(void)
{
    settings_push_new_screen();
}

/****************************************************************************
 * 外部入口：在主菜单列表项上调用，绑定入口图标和触摸回调
 *   parent - 菜单列表的某个 item
 *   path   - 入口图标图片路径（由 ui_image_show 显示）
 * 用法示例（在 main_ui.c 等列表中）：
 *   settings_ui_create_entry(list_item, icon_path);
 ****************************************************************************/
void settings_ui_create_entry(lv_obj_t *parent, const char *path)
{
    if (parent == NULL)
    {
        return;
    }

    ui_image_show(parent, path);
    lv_obj_add_flag(parent, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(parent, settings_on_entry_click, LV_EVENT_CLICKED, NULL);
}
