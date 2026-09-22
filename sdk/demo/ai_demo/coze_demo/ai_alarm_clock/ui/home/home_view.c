/****************************************************************************
 * 文件名：home_screen.c
 *
 * UI 整体结构（自上而下）：
 *   ┌────────────────────────────────────────┐
 *   │            周一 07月06日                 │  ← 顶部日期
 *   │              WiFi [电池]                │  ← 右上角图标
 *   ├────────────────────────────────────────┤
 *   │                                        │
 *   │              18:23                     │  ← 大号时间显示
 *   │                                        │
 *   │          27~29°C 中雨                  │  ← 底部天气信息
 *   │              🌸                        │  ← 天气图标
 *   └────────────────────────────────────────┘
 *
 * 功能说明：
 *   纯显示页面，无按钮交互事件
 *   - 显示当前时间（每秒自动更新）
 *   - 显示当前日期
 *   - 显示天气信息
 *   - 显示WiFi和电池状态图标
 *
 * 入口：home_screen_create(parent)
 ****************************************************************************/
#include "basic_include.h"
#include "lvgl/lvgl.h"
#include "screen_memory.h"
#include "screen_msg_queue.h"
#include "osal/string.h"
#include "ui_theme.h"
// wifi相关
#include "syscfg.h"
#include "lib/umac/ieee80211.h"

// 时间相关
#include "common/timezone.h"
/* ===== 颜色常量 ===== */
#define HOME_BG_COLOR          0x0F172A /* 深色背景 */
#define HOME_DATE_COLOR        0xFFFFFF /* 日期文字白色 */
#define HOME_TIME_COLOR        0xFFCC33 /* 时间文字黄色 */
#define HOME_WEATHER_COLOR     0xFFFFFF /* 天气文字白色 */
#define HOME_ICON_COLOR        0xFFFFFF /* 图标白色 */
#define WIFI_ICON_COLOR        0xFF0000 /* 图标红色 */
#define BATTERY_ICON_COLOR_MID 0xFFCC33 /* 图标黄色 */
#define BATTERY_ICON_COLOR_LOW 0xFF0000 /* 图标红色 */

/* ===== 定时器周期 ===== */
#define HOME_UPDATE_INTERVAL 300 /* 1秒更新一次 */
#define HOME_ERR_MSG_MAX_LEN    128

/* 错误消息通过 home 页自己的 LVGL 定时器消费，避免跨线程调用 LVGL。 */
static volatile uint8_t home_err_pending;
static volatile uint8_t home_err_active;
static char home_err_msg[HOME_ERR_MSG_MAX_LEN];

/* 星期几字符串数组 */
static const char *weekday_str[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};

/* 页面私有上下文 */
typedef struct {
    lv_obj_t   *parent;        /* 父容器 */
    lv_obj_t   *root;          /* 根容器 */
    lv_obj_t   *date_label;    /* 日期显示 */
    lv_obj_t   *time_label;    /* 时间显示 */
    lv_obj_t   *weather_label; /* 天气信息 */
    lv_obj_t   *wifi_icon;     /* WiFi图标 */
    lv_obj_t   *battery_icon;  /* 电池图标 */
    lv_obj_t   *weather_icon;  /* 天气装饰图标 */
    lv_obj_t   *err_popup;     /* 错误弹窗 */
    lv_obj_t   *err_label;     /* 错误文本 */
    lv_timer_t *update_timer;  /* 每秒更新时间 */
    uint32_t    hour : 5, mon : 4, mday : 5, wday : 3, min : 6, power_percent : 7, rev : 2;
    uint8_t     env_update : 1, battery_update : 1, last_connected : 1, power_level : 3;
    char       *env_str;
    char        time_buf[8];  /* HH:MM */
    char        date_buf[32]; /* 周X MM月DD日 */
} home_ctx_t;

// 监听环境事件,当前只响应天气事件
static sysevt_hdl_res ENV_event(uint32 event_id, uint32 data, uint32 priv)
{
    home_ctx_t *ctx;
    ctx = (home_ctx_t *) priv;
    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_ENV, SYSEVT_ENV_WEATHER): {
            ctx->env_str    = (char *) data;
            ctx->env_update = 1;
        }
        break;
    }
    return SYSEVT_CONTINUE;
}

void ui_home_view_set_error(int32_t err_code, const char *err_msg)
{
    uint32_t flag;
    size_t   len;

    (void) err_code;

    flag = SCREEN_MSG_QUEUE_LOCK();
    if (err_msg == NULL && !home_err_active) {
        SCREEN_MSG_QUEUE_UNLOCK(flag);
        return;
    }

    if (err_msg == NULL) {
        home_err_msg[0] = '\0';
    } else {
        len = os_strlen(err_msg);
        if (len >= sizeof(home_err_msg)) {
            len = sizeof(home_err_msg) - 1;
        }
        os_memcpy(home_err_msg, err_msg, len);
        home_err_msg[len] = '\0';
    }
    home_err_active = (home_err_msg[0] != '\0');
    home_err_pending = 1;
    SCREEN_MSG_QUEUE_UNLOCK(flag);
}

// 监听系统事件,当前只响应电池事件
static sysevt_hdl_res SYSTEM_event(uint32 event_id, uint32 data, uint32 priv)
{
    home_ctx_t *ctx;
    ctx = (home_ctx_t *) priv;
    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_SYSTEM, SYSEVT_SYSTEM_BATTERY_LEVEL): {
            // 计算百分比
            ctx->power_percent  = data;
            ctx->battery_update = 1;
        }
        break;
    }
    return SYSEVT_CONTINUE;
}

/****************************************************************************
 * 定时器回调：每秒检查时间变化，仅在变化时更新显示
 ****************************************************************************/
static void home_update_timer_cb(lv_timer_t *timer)
{
    home_ctx_t *ctx;
    struct tm   t;
    uint8_t     hour;
    uint8_t     min;
    uint8_t     mon;
    uint8_t     mday;
    uint8_t     wday;
    uint32_t    flag;
    char        err_msg[HOME_ERR_MSG_MAX_LEN];

    if (timer == NULL) {
        return;
    }

    ctx = (home_ctx_t *) timer->user_data;
    if (ctx == NULL || ctx->time_label == NULL) {
        return;
    }

    /* 获取系统实际时间 */
    localtime_tz(time(NULL), &t);
    hour = t.tm_hour;
    min  = t.tm_min;
    mon  = t.tm_mon;
    mday = t.tm_mday;
    wday = t.tm_wday;

    /* 检测时间是否有变化 */
    if (hour != ctx->hour || min != ctx->min) {
        lv_snprintf(ctx->time_buf, sizeof(ctx->time_buf), "%02d:%02d", hour, min);
        lv_label_set_text(ctx->time_label, ctx->time_buf);
        ctx->hour = hour;
        ctx->min  = min;
    }

    flag = SCREEN_MSG_QUEUE_LOCK();
    if (home_err_pending) {
        os_memcpy(err_msg, home_err_msg, sizeof(err_msg));
        home_err_pending = 0;
        SCREEN_MSG_QUEUE_UNLOCK(flag);

        if (ctx->err_popup != NULL && ctx->err_label != NULL) {
            if (err_msg[0] == '\0') {
                lv_obj_add_flag(ctx->err_popup, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_label_set_text(ctx->err_label, err_msg);
                lv_obj_clear_flag(ctx->err_popup, LV_OBJ_FLAG_HIDDEN);
                lv_obj_move_foreground(ctx->err_popup);
            }
        }
    } else {
        SCREEN_MSG_QUEUE_UNLOCK(flag);
    }

    /* 检测日期是否有变化 */
    if (mon != ctx->mon || mday != ctx->mday || wday != ctx->wday) {
        lv_snprintf(ctx->date_buf, sizeof(ctx->date_buf), "%s %d月%d日", weekday_str[wday], mon, mday);
        lv_label_set_text(ctx->date_label, ctx->date_buf);
        ctx->mon  = mon;
        ctx->mday = mday;
        ctx->wday = wday;
    }

    // 检查wifi是否已经连接,连接则显示白色,否则红色

    if (sys_status.wifi_connected != ctx->last_connected) {
        ctx->last_connected = sys_status.wifi_connected;
        if (ctx->last_connected) {
            lv_obj_set_style_text_color(ctx->wifi_icon, lv_color_hex(HOME_ICON_COLOR), 0);
        } else {
            lv_obj_set_style_text_color(ctx->wifi_icon, lv_color_hex(WIFI_ICON_COLOR), 0);
        }
    }
    if (ctx->env_update) {
        lv_label_set_text(ctx->weather_label, ctx->env_str);
        ctx->env_update = 0;
    }

    uint8_t power_level = 0;
    if (ctx->battery_update) {
        ctx->battery_update = 0;
        // 百分比来决定挡位 LV_SYMBOL_BATTERY_FULL,LV_SYMBOL_BATTERY_3,LV_SYMBOL_BATTERY_2,LV_SYMBOL_BATTERY_1,LV_SYMBOL_BATTERY_EMPTY
        if (ctx->power_percent >= 80) {
            power_level = 0;
        } else if (ctx->power_percent >= 60) {
            power_level = 1;
        } else if (ctx->power_percent >= 40) {
            power_level = 2;
        } else if (ctx->power_percent >= 20) {
            power_level = 3;
        } else {
            power_level = 4;
        }
        if (power_level != ctx->power_level) {
            ctx->power_level = power_level;
            switch (ctx->power_level) {
                case 0:
                    lv_label_set_text(ctx->battery_icon, LV_SYMBOL_BATTERY_FULL);
                    lv_obj_set_style_text_color(ctx->battery_icon, lv_color_hex(HOME_ICON_COLOR), 0);
                    break;
                case 1:
                    lv_label_set_text(ctx->battery_icon, LV_SYMBOL_BATTERY_3);
                    lv_obj_set_style_text_color(ctx->battery_icon, lv_color_hex(HOME_ICON_COLOR), 0);
                    break;
                case 2:
                    lv_label_set_text(ctx->battery_icon, LV_SYMBOL_BATTERY_2);
                    lv_obj_set_style_text_color(ctx->battery_icon, lv_color_hex(BATTERY_ICON_COLOR_MID), 0);
                    break;
                case 3:
                    lv_label_set_text(ctx->battery_icon, LV_SYMBOL_BATTERY_1);
                    lv_obj_set_style_text_color(ctx->battery_icon, lv_color_hex(BATTERY_ICON_COLOR_LOW), 0);
                    break;
                case 4:
                    lv_label_set_text(ctx->battery_icon, LV_SYMBOL_BATTERY_EMPTY);
                    lv_obj_set_style_text_color(ctx->battery_icon, lv_color_hex(BATTERY_ICON_COLOR_LOW), 0);
                    break;
                default:
                    break;
            }
        }
    }
}

/****************************************************************************
 * 创建 home_screen 页面
 ****************************************************************************/
lv_obj_t *ui_home_screen_create(lv_obj_t *parent, const char *path)
{
    lv_obj_t   *root;
    home_ctx_t *ctx;

    if (parent == NULL) {
        return NULL;
    }

    /* 申请上下文 */
    ctx = SCREEN_MALLOC(sizeof(home_ctx_t));
    if (ctx == NULL) {
        return NULL;
    }
    lv_memset(ctx, 0, sizeof(home_ctx_t));
    ctx->parent = parent;

    /* 根容器：深色背景 */
    root = lv_obj_create(parent);
    if (root == NULL) {
        SCREEN_FREE(ctx);
        return NULL;
    }
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_center(root);
    lv_obj_set_style_bg_color(root, lv_color_hex(HOME_BG_COLOR), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    /* 将上下文挂载到 root 的 user_data */
    ctx->root = root;
    lv_obj_set_user_data(root, ctx);

    /* 日期显示 - 左上角 */
    ctx->date_label = lv_label_create(root);
    if (ctx->date_label != NULL) {
        lv_label_set_text(ctx->date_label, "--月--日");
        lv_obj_set_style_text_color(ctx->date_label, lv_color_hex(HOME_DATE_COLOR), 0);
        lv_obj_set_style_text_font(ctx->date_label, UI_FONT_BODY, 0);
        lv_obj_align(ctx->date_label, LV_ALIGN_TOP_LEFT, 10, 10);
    }

    /* WiFi图标 - 右上角 */
    ctx->wifi_icon = lv_label_create(root);
    if (ctx->wifi_icon != NULL) {
        lv_label_set_text(ctx->wifi_icon, LV_SYMBOL_WIFI);
        lv_obj_set_style_text_color(ctx->wifi_icon, lv_color_hex(WIFI_ICON_COLOR), 0);
        lv_obj_align(ctx->wifi_icon, LV_ALIGN_TOP_RIGHT, -50, 10);
    }

    /* 电池图标 - 右上角（WiFi图标右侧） */
    ctx->battery_icon = lv_label_create(root);
    if (ctx->battery_icon != NULL) {
        lv_label_set_text(ctx->battery_icon, LV_SYMBOL_BATTERY_FULL);
        lv_obj_set_style_text_color(ctx->battery_icon, lv_color_hex(HOME_ICON_COLOR), 0);
        lv_obj_align(ctx->battery_icon, LV_ALIGN_TOP_RIGHT, -10, 10);
    }

    /* 时间显示 - 居中大号字体 */
    ctx->time_label = lv_label_create(root);
    if (ctx->time_label != NULL) {
        lv_label_set_text(ctx->time_label, "18:23");
        lv_obj_set_style_text_color(ctx->time_label, lv_color_hex(HOME_TIME_COLOR), 0);
        lv_obj_set_style_text_font(ctx->time_label, get_theme_default_font48(), 0);
        lv_obj_align(ctx->time_label, LV_ALIGN_CENTER, 0, 0);
    }

    /* 天气信息 - 底部 */
    ctx->weather_label = lv_label_create(root);
    if (ctx->weather_label != NULL) {
        lv_label_set_text(ctx->weather_label, "正在查询天气信息");
        lv_obj_set_style_text_color(ctx->weather_label, lv_color_hex(HOME_WEATHER_COLOR), 0);
        lv_obj_align(ctx->weather_label, LV_ALIGN_BOTTOM_MID, 0, -30);
        lv_obj_set_style_text_font(ctx->weather_label, UI_FONT_BODY, 0);
    }

    /* 天气装饰图标 - 右下角 */
    ctx->weather_icon = lv_label_create(root);
    if (ctx->weather_icon != NULL) {
        lv_label_set_text(ctx->weather_icon, LV_SYMBOL_HOME);
        lv_obj_set_style_text_color(ctx->weather_icon, lv_color_hex(HOME_TIME_COLOR), 0);
        lv_obj_align(ctx->weather_icon, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    }

    /* 错误信息弹窗 - 白底红字 */
    ctx->err_popup = lv_obj_create(root);
    if (ctx->err_popup != NULL) {
        lv_obj_remove_style_all(ctx->err_popup);
        lv_obj_set_size(ctx->err_popup, LV_PCT(88), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(ctx->err_popup, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_bg_opa(ctx->err_popup, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(ctx->err_popup, 10, 0);
        lv_obj_set_style_pad_all(ctx->err_popup, 12, 0);
        lv_obj_set_style_border_width(ctx->err_popup, 0, 0);
        lv_obj_align(ctx->err_popup, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_flag(ctx->err_popup, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ctx->err_popup, LV_OBJ_FLAG_SCROLLABLE);

        ctx->err_label = lv_label_create(ctx->err_popup);
        if (ctx->err_label != NULL) {
            lv_label_set_text(ctx->err_label, "");
            lv_obj_set_width(ctx->err_label, LV_PCT(100));
            lv_label_set_long_mode(ctx->err_label, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_align(ctx->err_label, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_style_text_color(ctx->err_label, lv_color_hex(0xFF0000), 0);
            lv_obj_center(ctx->err_label);
        }
    }

    /* 启动定时器：每秒更新 */
    ctx->update_timer = lv_timer_create(home_update_timer_cb, HOME_UPDATE_INTERVAL, ctx);
    sys_event_take(SYS_EVENT(SYS_EVENT_ENV, 0), ENV_event, (uint32_t) ctx);
    sys_event_take(SYS_EVENT(SYS_EVENT_SYSTEM, 0), SYSTEM_event, (uint32_t) ctx);
    return root;
}

/****************************************************************************
 * 销毁 home_screen 页面
 ****************************************************************************/
void ui_home_screen_destroy(lv_obj_t *root)
{
    home_ctx_t *ctx;

    if (root == NULL) {
        return;
    }

    ctx = (home_ctx_t *) lv_obj_get_user_data(root);
    if (ctx == NULL) {
        return;
    }

    /* 停止定时器 */
    if (ctx->update_timer != NULL) {
        lv_timer_del(ctx->update_timer);
        ctx->update_timer = NULL;
    }

    /* 释放上下文 */
    SCREEN_FREE(ctx);
}