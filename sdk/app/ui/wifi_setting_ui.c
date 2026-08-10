#include "basic_include.h"
#include "ui_manager.h"

#include "screen_manager.h"
#include "screen_memory.h"
#include "screen_common.h"

#include "ui_theme.h"
#include "lvgl/lvgl.h"
// wifi相关
#include "syscfg.h"
#include "lib/umac/ieee80211.h"

enum
{
    WIFI_LIST_SHOW,
    CONNECT_SUCCESS,
    CONNECTING,
};

extern lv_style_t g_style;

/****************************************************************************
 * 文件名：wifi_setting_ui.c
 *
 * UI 整体结构（自上而下）：
 *   ┌────────────────────────────────────────┐
 *   │ [ < 返回 ]          WIFI设置            │  ← 顶部黄色 header
 *   ├────────────────────────────────────────┤
 *   │  WI-FI 仅支持2.4G频段                   │  ← 提示信息
 *   ├────────────────────────────────────────┤
 *   │  📶 taixin3              连接成功 🔒    │  ← WiFi列表项（连接成功高亮）
 *   │  📶 mxd33023                   🔒      │  ← WiFi列表项
 *   │  📶 111LLM1                    🔒      │  ← WiFi列表项
 *   └────────────────────────────────────────┘
 *
 * 适配 Screen Manager 框架：
 *   - ctx 放在 screen->user_data（owns_self 控制释放）
 *   - 返回按钮：screen_finish(ui_s->screen)
 *   - 本期不支持物理按键（无 group / indev_keypad）
 *
 * 入口：wifi_setting_ui_create()
 *   直接创建并压栈 WiFi 设置页面 Screen
 ****************************************************************************/

/* ===== 文案常量：集中管理用户可见字符串，方便后续调整 ===== */
static const char *const wifi_str_title       = "WIFI设置";
static const char *const wifi_str_back        = "返回";
static const char *const wifi_str_hint        = "WI-FI 仅支持2.4G频段";
static const char *const wifi_str_encrypted   = "加密";
static const char *const wifi_str_connected   = "连接成功";
static const char *const wifi_str_connecting  = "连接中";
static const char *const wifi_str_connect_err = "密码错误";

/* ===== 颜色宏：统一管理本页面用到的所有颜色 ===== */
#define WIFI_BG_DARK        0x000000 /* 屏幕根背景（黑色） */
#define WIFI_HEADER_BG      0xFFCC33 /* 顶部黄色 header 背景 */
#define WIFI_HEADER_TEXT    0x000000 /* header 上文字色（黑色） */
#define WIFI_HINT_TEXT      0xAAAAAA /* 提示文字灰色 */
#define WIFI_LIST_BG        0x1A1A1A /* WiFi列表项背景 */
#define WIFI_LIST_ACTIVE    0xFFF2B3 /* 选中的WiFi项背景（浅色） */
#define WIFI_LIST_CONNECTED 0xFFCC33 /* 已连接WiFi项背景（黄色） */
#define WIFI_TEXT_WHITE     0xFFFFFF /* 白色文字 */
#define WIFI_ICON_COLOR     0x4CAF50 /* WiFi信号图标颜色（绿色） */

/* ===== 自适应缩放宏 ===== */
#define WIFI_BASE_H            480
#define WIFI_SCALE(v)          ((lv_coord_t) LV_MAX((int32_t) (v) * (LV_VER_RES) / WIFI_BASE_H, 2))
#define MAX_SCAN_TIME_INTERVAL (5 * 1000)

/* ===== 页面私有上下文，挂在 screen->user_data 上 ===== */
typedef struct
{
    screen_t *screen; /* 关联的 Screen 对象 */

    /* UI 控件指针 */
    lv_obj_t *header_panel; /* 顶部黄色 header 容器 */
    lv_obj_t *back_btn;     /* 返回按钮 */
    lv_obj_t *hint_label;   /* "WI-FI 仅支持2.4G频段"提示 */
    lv_obj_t *wifi_list;    /* WiFi列表容器 */
    lv_obj_t *refresh_icon; /* 刷新按钮图标 */

    /* WiFi扫描完成标志 */
    /* 正在扫描标志 */
    /* 密钥错误标志 */
    /* 连接中标志 */
    uint8_t  wifi_update : 1, is_scanning : 1, key_err : 1, is_connecting : 1, rev : 4;
    uint32_t last_scan_time;
    char     connect_ssid[64];
    char     ready_connect_ssid[64];

    lv_timer_t *update_timer; /* WiFi 更新检测定时器 */

} wifi_setting_ctx_t;

/* ===== 函数前置声明 ===== */
static void wifi_setting_get_lifecycle(screen_lifecycle_t *lifecycle);
static void wifi_setting_create(screen_t *screen, void *params);
static void wifi_setting_destroy(screen_t *screen);
static void wifi_setting_start(screen_t *screen);
static void wifi_setting_resume(screen_t *screen);
static void wifi_setting_pause(screen_t *screen);
static void wifi_setting_stop(screen_t *screen);
static void wifi_setting_restart(screen_t *screen);
static void wifi_setting_finish(screen_t *screen);

static void wifi_setting_on_back_click(lv_event_t *e);
static void wifi_setting_on_wifi_item_pressed(lv_event_t *e);
static void wifi_setting_on_wifi_item_released(lv_event_t *e);

static void wifi_setting_create_header(lv_obj_t *page, wifi_setting_ctx_t *ui_s);
static void wifi_setting_create_hint(lv_obj_t *page, wifi_setting_ctx_t *ui_s);
static void wifi_setting_create_list(lv_obj_t *page, wifi_setting_ctx_t *ui_s, struct hgic_bss_info *bsslist, int32_t count);
static void wifi_setting_update_timer_cb(lv_timer_t *timer);
static void wifi_setting_on_refresh_click(lv_event_t *e);

sysevt_hdl_res wifi_scan_event(uint32 event_id, uint32 data, uint32 priv)
{
    screen_t *screen;
    switch (event_id)
    {
        case SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_SCAN_DONE):
        {
            screen                   = (screen_t *) priv;
            wifi_setting_ctx_t *ui_s = (wifi_setting_ctx_t *) screen_get_user_data(screen);
            ui_s->last_scan_time     = os_jiffies();
            if (ui_s)
            {
                ui_s->wifi_update = 1;
                ui_s->is_scanning = 0;
            }
            break;
        }
        case SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_CONNECTTED):
        {
            screen                   = (screen_t *) priv;
            wifi_setting_ctx_t *ui_s = (wifi_setting_ctx_t *) screen_get_user_data(screen);
            ui_s->is_connecting      = 0;
            ui_s->wifi_update        = 1;
            ui_s->key_err            = 0;
            break;
        }
        case SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_WRONG_KEY):
        {
            screen                   = (screen_t *) priv;
            wifi_setting_ctx_t *ui_s = (wifi_setting_ctx_t *) screen_get_user_data(screen);
            ui_s->is_connecting      = 1;
            ui_s->wifi_update        = 1;
            ui_s->key_err            = 1;
            break;
        }
    }
    return SYSEVT_CONTINUE;
}

static void wifi_setting_scan()
{
    struct ieee80211_scandata scan;
    os_memset(&scan, 0, sizeof(scan));
    scan.chan_bitmap = 0xffff;
    scan.scan_cnt    = 1;
    scan.scan_time   = 100;
    ieee80211_scan(sys_cfgs.wifi_mode, 1, &scan);
}

static void wifi_connect(const char *ssid, const char *password)
{
    uint8 ifidx = (sys_cfgs.wifi_mode == WIFI_MODE_APSTA ? WIFI_MODE_STA : sys_cfgs.wifi_mode);

    os_memset(sys_cfgs.bssid, 0, 6);
    ieee80211_conf_set_bssid(ifidx, NULL);

    os_strncpy(sys_cfgs.ssid, ssid, SSID_MAX_LEN);
    ieee80211_conf_set_ssid(ifidx, sys_cfgs.ssid);

    if (password == NULL)
    {
        sys_cfgs.key_mgmt = WPA_KEY_MGMT_NONE;
        ieee80211_conf_set_keymgmt(sys_cfgs.wifi_mode, sys_cfgs.key_mgmt);
    }
    else
    {
        if (os_strlen(sys_cfgs.passwd) >= 8)
        {
            sys_cfgs.key_mgmt = WPA_KEY_MGMT_PSK;
            ieee80211_conf_set_keymgmt(sys_cfgs.wifi_mode, sys_cfgs.key_mgmt);
            os_strncpy(sys_cfgs.passwd, password, PASSWD_MAX_LEN);
            wpa_passphrase(sys_cfgs.ssid, (char *) sys_cfgs.passwd, sys_cfgs.psk);
            ieee80211_conf_set_psk(ifidx, sys_cfgs.psk);
        }
    }
}

/****************************************************************************
 * 返回按钮点击回调：触发 screen_finish()
 ****************************************************************************/
static void wifi_setting_on_back_click(lv_event_t *e)
{
    wifi_setting_ctx_t *ui_s = (wifi_setting_ctx_t *) lv_event_get_user_data(e);
    if (ui_s == NULL || ui_s->screen == NULL)
    {
        return;
    }
    os_printf("wifi_setting: click back btn\n");
    screen_finish(ui_s->screen);
}

/****************************************************************************
 * 刷新按钮点击回调：触发 WiFi 扫描，切换图标为旋转状态
 ****************************************************************************/
static void wifi_setting_on_refresh_click(lv_event_t *e)
{
    wifi_setting_ctx_t *ui_s = (wifi_setting_ctx_t *) lv_event_get_user_data(e);
    if (ui_s == NULL)
    {
        return;
    }

    /* 避免重复点击 */
    if (ui_s->is_scanning)
    {
        return;
    }

    os_printf("wifi_setting: click refresh btn, start wifi scan\n");
    ui_s->is_scanning = 1;

    /* 切换图标为加载状态 */
    if (ui_s->refresh_icon != NULL)
    {
        lv_label_set_text(ui_s->refresh_icon, LV_SYMBOL_LOOP);
    }

    /* TODO: 触发 WiFi 扫描 */
    wifi_setting_scan();
    ui_s->last_scan_time = os_jiffies();
}

/****************************************************************************
 * WiFi列表项按下回调：显示选中颜色
 ****************************************************************************/
void keyboard_input_ui_create(const char *ssid, uint8_t min_length, void (*on_confirm)(void *user_data, const char *text), void (*on_cancel)(void *user_data), void *userdata);
void wifi_confirm(void *user_data, const char *password)
{
    wifi_setting_ctx_t *ui_s = (wifi_setting_ctx_t *) user_data;
    if (ui_s == NULL)
    {
        return;
    }
    os_strcpy(ui_s->connect_ssid, ui_s->ready_connect_ssid);
    // 显示正在连接,尝试更新wifi列表
    ui_s->key_err       = 0;
    ui_s->is_connecting = 1;
    ui_s->wifi_update   = 1;
    wifi_connect(ui_s->connect_ssid, password);
}

void wifi_cancel(void *user_data)
{
    os_printf("%s:%d\n", __FUNCTION__, __LINE__);
}
static void wifi_setting_on_wifi_item_pressed(lv_event_t *e)
{
    lv_obj_t *item = lv_event_get_target(e);
    if (item != NULL)
    {
        lv_obj_set_style_bg_color(item, lv_color_hex(WIFI_LIST_ACTIVE), 0);
    }
}

/****************************************************************************
 * WiFi列表项松开回调：恢复原色并打印ssid
 ****************************************************************************/
static void wifi_setting_on_wifi_item_released(lv_event_t *e)
{
    wifi_setting_ctx_t *ui_s = (wifi_setting_ctx_t *) lv_event_get_user_data(e);
    lv_obj_t           *item = lv_event_get_target(e);
    lv_obj_t           *name_label;
    const char         *ssid;
    if (ui_s == NULL || item == NULL)
    {
        return;
    }

    /* list_item 结构：[wifi_icon, name_label]，第二个子对象是 name_label */
    name_label = lv_obj_get_child(item, 1);
    if (name_label == NULL)
    {
        return;
    }

    /* 从 label 获取 ssid 并打印 */
    ssid = lv_label_get_text(name_label);
    if (ssid != NULL)
    {
        os_printf("wifi_setting: selected wifi ssid: %s\n", ssid);
    }

    // 进入一个wifi输入密码的UI
    if (lv_obj_has_flag(item, LV_OBJ_FLAG_USER_1))
    {
        strcpy(ui_s->ready_connect_ssid, ssid);
        keyboard_input_ui_create(ssid, 8, wifi_confirm, wifi_cancel, (void *) ui_s);
    }
    else
    {
        strcpy(ui_s->connect_ssid, ssid);
        // 显示正在连接,尝试更新wifi列表
        ui_s->key_err       = 0;
        ui_s->is_connecting = 1;
        ui_s->wifi_update   = 1;
        wifi_connect(ssid, NULL);
    }
}

/****************************************************************************
 * 构建顶部黄色 header：包含左侧"< 返回"按钮和居中标题
 ****************************************************************************/
static void wifi_setting_create_header(lv_obj_t *page, wifi_setting_ctx_t *ui_s)
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
    lv_obj_set_size(header_panel, LV_PCT(96), WIFI_SCALE(50));
    lv_obj_set_style_radius(header_panel, WIFI_SCALE(25), 0);
    lv_obj_align(header_panel, LV_ALIGN_TOP_MID, 0, WIFI_SCALE(10));
    lv_obj_set_style_bg_color(header_panel, lv_color_hex(WIFI_HEADER_BG), 0);
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
        lv_obj_set_size(back_btn, WIFI_SCALE(110), WIFI_SCALE(40));
        lv_obj_set_style_radius(back_btn, WIFI_SCALE(20), 0);
        lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(back_btn, 0, 0);
        lv_obj_set_style_shadow_width(back_btn, 0, 0);
        lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_flex_flow(back_btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(back_btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(back_btn, WIFI_SCALE(4), 0);
        lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, WIFI_SCALE(8), 0);
        lv_obj_add_event_cb(back_btn, wifi_setting_on_back_click, LV_EVENT_CLICKED, ui_s);
        ui_s->back_btn = back_btn;

        /* "<" 左箭头图标 */
        back_icon = lv_label_create(back_btn);
        if (back_icon != NULL)
        {
            lv_label_set_text(back_icon, LV_SYMBOL_LEFT);
            lv_obj_set_style_text_font(back_icon, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(back_icon, lv_color_hex(WIFI_HEADER_TEXT), 0);
        }

        /* "返回" 文字 */
        back_label = lv_label_create(back_btn);
        if (back_label != NULL)
        {
            lv_label_set_text(back_label, wifi_str_back);
            lv_obj_set_style_text_font(back_label, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(back_label, lv_color_hex(WIFI_HEADER_TEXT), 0);
        }
    }

    /* 居中标题：WIFI设置（向右偏移避开左侧返回按钮） */
    title_label = lv_label_create(header_panel);
    if (title_label != NULL)
    {
        lv_label_set_text(title_label, wifi_str_title);
        lv_obj_set_style_text_font(title_label, UI_FONT_BODY, 0);
        lv_obj_set_style_text_color(title_label, lv_color_hex(WIFI_HEADER_TEXT), 0);
        lv_obj_align(title_label, LV_ALIGN_CENTER, WIFI_SCALE(40), 0);
    }
}

/****************************************************************************
 * 构建提示信息行：左侧"WI-FI 仅支持2.4G频段" + 右侧刷新按钮
 ****************************************************************************/
static void wifi_setting_create_hint(lv_obj_t *page, wifi_setting_ctx_t *ui_s)
{
    lv_obj_t *hint_container;
    lv_obj_t *hint_label;
    lv_obj_t *refresh_btn;

    if (page == NULL || ui_s == NULL)
    {
        return;
    }

    /* 提示信息行容器（横向布局） */
    hint_container = lv_obj_create(page);
    if (hint_container == NULL)
    {
        return;
    }
    lv_obj_remove_style_all(hint_container);
    lv_obj_set_size(hint_container, LV_PCT(90), WIFI_SCALE(40));
    lv_obj_align(hint_container, LV_ALIGN_TOP_MID, 0, WIFI_SCALE(70));
    lv_obj_set_flex_flow(hint_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hint_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(hint_container, WIFI_SCALE(10), 0);
    lv_obj_set_style_pad_right(hint_container, WIFI_SCALE(10), 0);
    lv_obj_clear_flag(hint_container, LV_OBJ_FLAG_SCROLLABLE);

    /* 左侧：提示文字 */
    hint_label = lv_label_create(hint_container);
    if (hint_label != NULL)
    {
        lv_label_set_text(hint_label, wifi_str_hint);
        lv_obj_set_style_text_font(hint_label, UI_FONT_BODY, 0);
        lv_obj_set_style_text_color(hint_label, lv_color_hex(WIFI_HINT_TEXT), 0);
        ui_s->hint_label = hint_label;
    }

    /* 右侧：刷新按钮 */
    refresh_btn = lv_btn_create(hint_container);
    if (refresh_btn != NULL)
    {
        lv_obj_remove_style_all(refresh_btn);
        lv_obj_set_size(refresh_btn, WIFI_SCALE(36), WIFI_SCALE(36));
        lv_obj_set_style_radius(refresh_btn, WIFI_SCALE(18), 0);
        lv_obj_set_style_bg_color(refresh_btn, lv_color_hex(0x666666), 0);
        lv_obj_set_style_bg_opa(refresh_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(refresh_btn, 0, 0);
        lv_obj_set_style_shadow_width(refresh_btn, 0, 0);
        lv_obj_add_flag(refresh_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(refresh_btn, wifi_setting_on_refresh_click, LV_EVENT_CLICKED, ui_s);

        /* 刷新图标 */
        ui_s->refresh_icon = lv_label_create(refresh_btn);
        if (ui_s->refresh_icon != NULL)
        {
            lv_label_set_text(ui_s->refresh_icon, LV_SYMBOL_REFRESH);
            lv_obj_set_style_text_color(ui_s->refresh_icon, lv_color_hex(WIFI_TEXT_WHITE), 0);
            lv_obj_center(ui_s->refresh_icon);
        }
    }
}

/****************************************************************************
 * 构建单个 WiFi 列表项：
 *   左侧：WiFi信号图标
 *   中间：WiFi名称（ssid）
 *   右侧：连接状态 + 加密标识
 * 返回创建的列表项对象
 ****************************************************************************/
static lv_obj_t *wifi_setting_create_list_item(lv_obj_t *parent, const char *ssid, uint8_t is_encrypted, uint8_t is_connected, wifi_setting_ctx_t *ui_s)
{
    lv_obj_t *list_item;
    lv_obj_t *wifi_icon;
    lv_obj_t *name_label;
    lv_obj_t *connected_label;
    lv_obj_t *secure_label;

    if (parent == NULL || ssid == NULL || ui_s == NULL)
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
    lv_obj_set_size(list_item, LV_PCT(95), WIFI_SCALE(50));
    lv_obj_set_style_radius(list_item, WIFI_SCALE(10), 0);
    lv_obj_set_style_bg_opa(list_item, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(list_item, 0, 0);
    lv_obj_set_style_shadow_width(list_item, 0, 0);
    lv_obj_set_flex_flow(list_item, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(list_item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(list_item, WIFI_SCALE(12), 0);
    lv_obj_set_style_pad_left(list_item, WIFI_SCALE(15), 0);
    lv_obj_set_style_pad_right(list_item, WIFI_SCALE(15), 0);
    lv_obj_clear_flag(list_item, LV_OBJ_FLAG_SCROLLABLE);

    /* 背景颜色 */
    lv_obj_set_style_bg_color(list_item, lv_color_hex(is_connected ? WIFI_LIST_CONNECTED : WIFI_LIST_BG), 0);
    if (is_connected)
    {
        lv_obj_add_state(list_item, LV_STATE_CHECKED);
    }

    /* 左侧：WiFi信号图标（使用LV_SYMBOL_WIFI） */
    wifi_icon = lv_label_create(list_item);
    if (wifi_icon != NULL)
    {
        lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
        lv_obj_set_style_text_font(wifi_icon, UI_FONT_BODY, 0);
        lv_obj_set_style_text_color(wifi_icon, lv_color_hex(WIFI_ICON_COLOR), 0);
    }

    /* 中间：WiFi名称（ssid） */
    name_label = lv_label_create(list_item);
    if (name_label != NULL)
    {
        lv_label_set_text(name_label, ssid);
        lv_obj_set_style_text_font(name_label, UI_FONT_BODY, 0);
        lv_obj_set_style_text_color(name_label, lv_color_hex(is_connected ? WIFI_HEADER_TEXT : WIFI_TEXT_WHITE), 0);
        lv_obj_set_flex_grow(name_label, 1); /* 占满剩余空间 */
    }

    if (is_connected)
    {
        connected_label = lv_label_create(list_item);
        if (connected_label != NULL)
        {
            if (is_connected == CONNECT_SUCCESS)
            {
                lv_label_set_text(connected_label, wifi_str_connected);
            }
            else
            {
                if (ui_s->key_err)
                {
                    lv_label_set_text(connected_label, wifi_str_connect_err);
                }
                else
                {
                    lv_label_set_text(connected_label, wifi_str_connecting);
                }
            }
            lv_obj_set_style_text_font(connected_label, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(connected_label, lv_color_hex(WIFI_HEADER_TEXT), 0);
        }
    }

    if (is_encrypted)
    {
        secure_label = lv_label_create(list_item);
        if (secure_label != NULL)
        {
            lv_label_set_text(secure_label, wifi_str_encrypted);
            lv_obj_set_style_text_font(secure_label, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(secure_label, lv_color_hex(is_connected ? WIFI_HEADER_TEXT : WIFI_HINT_TEXT), 0);
        }
    }
    // 如果不是已经连接,则还是可以响应事件
    if (!(is_connected == CONNECT_SUCCESS))
    {
        /* 添加点击事件：按下变色，松开恢复 */
        lv_obj_add_flag(list_item, LV_OBJ_FLAG_CLICKABLE);
        // lv_obj_add_event_cb(list_item, wifi_setting_on_wifi_item_pressed, LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(list_item, wifi_setting_on_wifi_item_released, LV_EVENT_CLICKED, ui_s);
        lv_obj_set_style_bg_color(list_item, lv_color_hex(WIFI_LIST_ACTIVE), LV_STATE_PRESSED);
        // 设置加密标志
        if (is_encrypted)
        {
            lv_obj_add_flag(list_item, LV_OBJ_FLAG_USER_1);
        }
        else
        {
            lv_obj_clear_flag(list_item, LV_OBJ_FLAG_USER_1);
        }
        // 正在连接
        if (is_connected == CONNECTING)
        {
            lv_obj_set_style_bg_color(list_item, lv_color_hex(WIFI_LIST_CONNECTED), 0);
        }
        else
        {
            lv_obj_set_style_bg_color(list_item, lv_color_hex(WIFI_LIST_BG), LV_STATE_DEFAULT);
        }
    }

    return list_item;
}

/****************************************************************************
 * 构建 WiFi 列表：遍历 bsslist 数组创建列表项
 ****************************************************************************/
static void wifi_setting_create_list(lv_obj_t *page, wifi_setting_ctx_t *ui_s, struct hgic_bss_info *bsslist, int32_t count)
{
    lv_obj_t *wifi_list;
    int32_t   i;

    if (page == NULL || ui_s == NULL || bsslist == NULL || count <= 0)
    {
        return;
    }

    /* 列表容器（垂直方向） */
    wifi_list = lv_obj_create(page);
    if (wifi_list == NULL)
    {
        return;
    }
    lv_obj_remove_style_all(wifi_list);
    lv_obj_set_size(wifi_list, LV_PCT(100), LV_VER_RES - WIFI_SCALE(140));
    lv_obj_align(wifi_list, LV_ALIGN_TOP_MID, 0, WIFI_SCALE(120));
    lv_obj_set_flex_flow(wifi_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wifi_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(wifi_list, WIFI_SCALE(10), 0);
    lv_obj_set_style_pad_top(wifi_list, WIFI_SCALE(5), 0);
    lv_obj_set_scroll_dir(wifi_list, LV_DIR_VER); /* 允许垂直滚动 */
    ui_s->wifi_list = wifi_list;

    // 首先如果已经连接,则显示当前连接的ssid
    if (sys_status.wifi_connected)
    {
        // 成功连接
        os_strcpy(ui_s->connect_ssid, (const char *) sys_cfgs.ssid);
        wifi_setting_create_list_item(wifi_list, (const char *) sys_cfgs.ssid, 0, CONNECT_SUCCESS, ui_s);
    }
    else
    {
        // 没有连接,就检查列表是否有这个ssid,有就显示正在连接或者密码错误
        for (i = 0; i < count; i++)
        {
            if (os_strlen(bsslist[i].ssid) > 0 && os_strcmp(bsslist[i].ssid, (const char *) sys_cfgs.ssid) == 0)
            {
                bsslist[i].ssid[0] = '\0';
                os_strcpy(ui_s->connect_ssid, (const char *) sys_cfgs.ssid);
                wifi_setting_create_list_item(wifi_list, (const char *) sys_cfgs.ssid, bsslist[i].encrypt, CONNECTING, ui_s);
                break;
            }
        }
    }
    /* 遍历 bsslist 数据，创建每个列表项 */
    for (i = 0; i < count; i++)
    {
        if (os_strlen(bsslist[i].ssid) > 0 && os_strcmp(bsslist[i].ssid, ui_s->connect_ssid) != 0)
        {
            wifi_setting_create_list_item(wifi_list, (const char *) bsslist[i].ssid, bsslist[i].encrypt, WIFI_LIST_SHOW, ui_s);
        }
    }
}

/****************************************************************************
 * 定时器回调：检测 wifi_update 标志，为 1 时清 0 并刷新列表
 ****************************************************************************/
static void wifi_setting_update_timer_cb(lv_timer_t *timer)
{
    wifi_setting_ctx_t *ui_s;

    if (timer == NULL)
    {
        return;
    }

    ui_s = (wifi_setting_ctx_t *) timer->user_data;
    if (ui_s == NULL)
    {
        return;
    }

    /* 检测 wifi_update 标志 */
    if (ui_s->wifi_update)
    {
        ui_s->wifi_update = 0;
        ui_s->is_scanning = 0;

        /* 恢复刷新图标 */
        if (ui_s->refresh_icon != NULL)
        {
            lv_label_set_text(ui_s->refresh_icon, LV_SYMBOL_REFRESH);
        }
        os_printf("wifi_setting: wifi_update detected, refreshing list\n");

        /* 获取扫描结果并创建 WiFi 列表 */
        struct hgic_bss_info bsslistp[20];
        int32_t              cnt = ieee80211_get_bsslist(bsslistp, 20, 0);

        /* 如果之前有列表，先清空 */
        if (ui_s->wifi_list != NULL)
        {
            lv_obj_del(ui_s->wifi_list);
            ui_s->wifi_list = NULL;
        }

        /* 创建新的 WiFi 列表 */
        if (cnt > 0)
        {
            wifi_setting_create_list(ui_s->screen->root, ui_s, bsslistp, cnt);
        }

        os_printf("wifi_setting: found %d wifi networks\n", (int) cnt);
    }

    // 主动触发一次扫描
    if (ui_s->is_scanning == 0 && (uint32_t) os_jiffies() - ui_s->last_scan_time > MAX_SCAN_TIME_INTERVAL)
    {
        /* 切换图标为加载状态 */
        if (ui_s->refresh_icon != NULL)
        {
            lv_label_set_text(ui_s->refresh_icon, LV_SYMBOL_LOOP);
        }
        ui_s->is_scanning = 1;
        wifi_setting_scan();
        ui_s->last_scan_time = os_jiffies();
    }
}
/****************************************************************************
 * on_create：首次进入页面时调用
 *   1. 申请 ctx 上下文并挂到 screen->user_data
 *   2. 配置 screen->root 黑色背景
 *   3. 创建 header（返回按钮 + 标题）
 *   4. 创建提示信息
 *   5. 触发 WiFi 扫描（扫描完成后显示列表）
 ****************************************************************************/
static void wifi_setting_create(screen_t *screen, void *params)
{
    wifi_setting_ctx_t *ui_s;

    (void) params; /* 暂不使用入参 */

    if (screen == NULL)
    {
        return;
    }

    /* 申请上下文内存 */
    ui_s = (wifi_setting_ctx_t *) SCREEN_MALLOC(sizeof(wifi_setting_ctx_t));
    if (ui_s == NULL)
    {
        return;
    }
    memset(ui_s, 0, sizeof(wifi_setting_ctx_t));
    ui_s->screen = screen;
    screen_set_user_data(screen, ui_s);

    /* 配置根容器：全屏黑色背景 */
    lv_obj_add_style(screen->root, &g_style, 0);
    lv_obj_set_size(screen->root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(screen->root, lv_color_hex(WIFI_BG_DARK), 0);
    lv_obj_set_style_bg_opa(screen->root, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen->root, LV_OBJ_FLAG_SCROLLABLE);

    /* 顶部 header：黄色胶囊状，包含返回按钮 + 标题 */
    wifi_setting_create_header(screen->root, ui_s);

    /* 提示信息 */
    wifi_setting_create_hint(screen->root, ui_s);

    /* 监听 WiFi 扫描事件 */
    sys_event_take(SYS_EVENT(SYS_EVENT_WIFI, 0), wifi_scan_event, (uint32_t) screen);

    /* 创建 WiFi 更新检测定时器（500ms 间隔） */
    ui_s->update_timer = lv_timer_create(wifi_setting_update_timer_cb, 500, ui_s);

    /* 页面创建主动显示wifi列表 */
    ui_s->wifi_update = 1;
    if (ui_s->refresh_icon != NULL)
    {
        lv_label_set_text(ui_s->refresh_icon, LV_SYMBOL_LOOP);
    }
    os_printf("wifi_setting: start wifi scan on create\n");
}

/****************************************************************************
 * on_start：页面即将可见时调用
 *   - 本页无硬件资源，空实现
 ****************************************************************************/
static void wifi_setting_start(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_resume：页面成为栈顶可交互时调用
 *   - 本页不支持物理按键，空实现
 ****************************************************************************/
static void wifi_setting_resume(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_pause：页面失去焦点时调用
 *   - 本页无硬件资源/按键钩子，空实现
 ****************************************************************************/
static void wifi_setting_pause(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_stop：页面不可见 / 退出前台时调用
 *   - 本页无硬件资源，空实现
 ****************************************************************************/
static void wifi_setting_stop(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_destroy：页面对象销毁时调用
 *   - 释放 ui_s
 *   - 不释放 LVGL 对象，由 screen_manager 统一销毁
 ****************************************************************************/
static void wifi_setting_destroy(screen_t *screen)
{
    wifi_setting_ctx_t *ui_s;

    if (screen == NULL)
    {
        return;
    }
    ui_s = (wifi_setting_ctx_t *) screen_get_user_data(screen);
    if (ui_s == NULL)
    {
        return;
    }

    /* 删除定时器 */
    if (ui_s->update_timer != NULL)
    {
        lv_timer_del(ui_s->update_timer);
        ui_s->update_timer = NULL;
    }

    sys_event_untake(SYS_EVENT(SYS_EVENT_WIFI, 0), wifi_scan_event);
    /* 清空 ctx 内部指针（不释放 LVGL 对象，由 screen_manager 统一销毁） */
    ui_s->screen       = NULL;
    ui_s->header_panel = NULL;
    ui_s->back_btn     = NULL;
    ui_s->hint_label   = NULL;
    ui_s->wifi_list    = NULL;
    ui_s->refresh_icon = NULL;
    ui_s->update_timer = NULL;

    SCREEN_FREE(ui_s);
    screen_set_user_data(screen, NULL);
}

/****************************************************************************
 * on_restart：STOPPED → 重新前台前调用
 ****************************************************************************/
static void wifi_setting_restart(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_finish：screen_finish() 调用时触发
 *   - 本页无业务结果回传，空实现
 ****************************************************************************/
static void wifi_setting_finish(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * 组装生命周期回调集合
 ****************************************************************************/
static void wifi_setting_get_lifecycle(screen_lifecycle_t *lifecycle)
{
    if (lifecycle == NULL)
    {
        return;
    }

    *lifecycle = (screen_lifecycle_t) {
            .on_create  = wifi_setting_create,
            .on_start   = wifi_setting_start,
            .on_resume  = wifi_setting_resume,
            .on_pause   = wifi_setting_pause,
            .on_stop    = wifi_setting_stop,
            .on_destroy = wifi_setting_destroy,
            .on_restart = wifi_setting_restart,
            .on_finish  = wifi_setting_finish,
            .screen_id  = 0,
            .only       = 0,
    };
}

/****************************************************************************
 * 外部入口：直接创建并压栈 WiFi 设置页面
 * 用法示例：
 *   wifi_setting_ui_create();
 ****************************************************************************/
void wifi_setting_ui_create(void)
{
    screen_lifecycle_t lifecycle;
    screen_t          *screen;

    os_printf("wifi_setting: create and push wifi_setting screen\n");
    wifi_setting_get_lifecycle(&lifecycle);
    screen = screen_register("wifi_setting", &lifecycle);
    if (screen != NULL)
    {
        screen_push(screen, NULL);
    }
}
