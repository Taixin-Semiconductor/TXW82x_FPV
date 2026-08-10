# TXW82x AI时钟 UI开发文档

## 目录
1. [UI架构概述](#1-ui架构概述)
2. [核心模块](#2-核心模块)
3. [页面模块](#3-页面模块)
4. [主题与样式](#4-主题与样式)
5. [开发指南](#5-开发指南)

---

## 1. UI架构概述

### 1.1 技术栈
- **GUI框架**: LVGL (Light and Versatile Graphics Library)
- **字体渲染**: FreeType
- **页面管理**: Tileview (平铺视图)

### 1.2 架构设计
```
┌─────────────────────────────────────────────────┐
│                 UI Manager                       │
│  ┌───────────────────────────────────────────┐  │
│  │            Tileview Container             │  │
│  │  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ...   │  │
│  │  │Home │ │Plan │ │AI_QA│ │Time │       │  │
│  │  │     │ │List │ │     │ │     │       │  │
│  │  └─────┘ └─────┘ └─────┘ └─────┘ ...   │  │
│  └───────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
```

UI采用**Tileview**布局，每个页面作为一个Tile，通过水平滑动切换。

---

## 2. 核心模块

### 2.1 UI管理器 (ui_manager)

**文件位置**: `project/ui/ui_manager.c`, `project/ui/ui_manager.h`

#### 功能说明
- 初始化LVGL和FreeType字体
- 创建根页面和Tileview容器
- 管理页面创建与切换
- 处理页面滑动事件

#### 主要API

| 函数名 | 参数 | 返回值 | 说明 |
|--------|------|--------|------|
| `ui_manager_init()` | 无 | void | UI系统初始化入口 |
| `ui_manager_get_menu_image_path()` | `ui_screen_id_t screen_id` | `const char*` | 获取菜单图标路径 |
| `ui_manager_get_freetype_font()` | 无 | `const lv_font_t*` | 获取FreeType字体对象 |
| `ui_manager_set_tile_swipe_enabled()` | `bool enabled` | void | 启用/禁用Tile滑动 |

#### 内部结构
```c
typedef struct {
    ui_screen_id_t current_screen_id;  // 当前页面ID
    lv_obj_t *root_screen;             // 根屏幕对象
    lv_obj_t *root_tileview;           // Tileview容器
    lv_obj_t *tiles[UI_SCREEN_ID_COUNT]; // 各页面Tile对象
} ui_manager_state;
```

### 2.2 页面ID定义 (ui_screen_ids)

**文件位置**: `project/ui/ui_screen_ids.h`

```c
typedef enum {
    UI_SCREEN_ID_NONE = -1,        // 无效页面
    UI_SCREEN_ID_HOME = 0,         // 主页
    UI_SCREEN_ID_PLAN_LIST,        // 计划清单
    UI_SCREEN_ID_AI_QA,            // AI对话
    UI_SCREEN_ID_TIMING,           // 定时/秒表
    UI_SCREEN_ID_CALENDAR,         // 日历
    UI_SCREEN_ID_SETTINGS,         // 设置
    UI_SCREEN_ID_COUNT             // 页面总数
} ui_screen_id_t;
```

### 2.3 主题配置 (ui_theme)

**文件位置**: `project/ui/ui_theme.h`

#### 颜色配置

| 常量名 | 颜色值 | 用途 |
|--------|--------|------|
| `UI_COLOR_PRIMARY` | 0xffd54a | 主题色（金色） |
| `UI_COLOR_PRIMARY_DARK` | 0x2a2113 | 主题深色 |
| `UI_COLOR_BG_DARK` | 0x111217 | 深色背景 |
| `UI_COLOR_BG_SCREEN` | 0x1a1c24 | 屏幕背景 |
| `UI_COLOR_BG_CARD` | 0xffffff | 卡片背景 |
| `UI_COLOR_TEXT_LIGHT` | 0xf1f3ff | 浅色文字 |
| `UI_COLOR_TEXT_DARK` | 0x1f2736 | 深色文字 |
| `UI_COLOR_TEXT_MUTED` | 0x70809b | 辅助文字 |
| `UI_COLOR_BLUE` | 0x206FE5 | 蓝色强调 |
| `UI_COLOR_GREEN` | 0x8bd450 | 绿色强调 |
| `UI_COLOR_RED` | 0xff7b72 | 红色强调 |

#### 尺寸配置

| 常量名 | 值 | 用途 |
|--------|-----|------|
| `UI_HEADER_HEIGHT` | 35 | 头部高度 |
| `UI_HEADER_RADIUS` | 16 | 头部圆角 |
| `UI_CORNER_RADIUS` | 14 | 通用圆角 |
| `UI_CARD_RADIUS` | 18 | 卡片圆角 |
| `UI_BTN_RADIUS` | 23 | 按钮圆角 |

#### 字体配置

| 常量名 | 字体 | 用途 |
|--------|------|------|
| `UI_FONT_BODY` | montserrat_10 | 小字 |
| `UI_FONT_BODY` | montserrat_12 | 极小字 |
| `UI_FONT_BODY` | montserrat_14 | 正文 |
| `UI_FONT_BODY` | montserrat_16 | 中等文字 |
| `UI_FONT_BODY` | montserrat_18 | 大标题 |
| `UI_FONT_BODY` | montserrat_20 | 标题 |
| `UI_FONT_BODY` | montserrat_22 | 大标题 |
| `UI_FONT_BODY` | montserrat_48 | 时间显示 |

---

## 3. 页面模块

### 3.1 主页 (home)

**文件位置**: `project/ui/home/home_view.c/h`, `project/ui/home/home_refresh.c/h`

#### 功能
- 显示时间、日期
- 显示天气信息
- 显示WiFi状态
- 显示电池状态

#### 数据模型
```c
typedef struct {
    int year;              // 年份
    int hour;              // 小时
    int minute;            // 分钟
    int weekday;           // 星期
    int month;             // 月份
    int day;               // 日期
    int temperature;       // 温度
    const char *weather_text; // 天气文字
    int wifi_connected;    // WiFi连接状态
    int wifi_level;        // WiFi信号强度
    int battery_percent;   // 电池百分比
    int battery_charging;  // 充电状态
} ui_home_view_model_t;
```

#### API列表
| 函数名 | 说明 |
|--------|------|
| `ui_home_view_create()` | 创建主页视图 |
| `ui_home_view_set_time()` | 设置时间 |
| `ui_home_view_set_date()` | 设置日期 |
| `ui_home_view_set_weather()` | 设置天气 |
| `ui_home_view_set_wifi()` | 设置WiFi状态 |
| `ui_home_view_set_battery()` | 设置电池状态 |
| `ui_home_view_refresh()` | 刷新主页 |

### 3.2 菜单 (menu)

**文件位置**: `project/ui/menu/menu_view.c/h`, `project/ui/menu/menu_detail_page.c/h`

#### 功能
- 菜单列表展示
- 菜单详情页

### 3.3 设置模块 (settings)

**文件位置**: `project/ui/settings/`

#### 3.3.1 模块架构

设置模块采用**二级导航**设计：
- **第一级**：设置详情列表页（`settings_detail`）- 显示所有设置项
- **第二级**：具体设置子页面（如WiFi、亮度等）

```
设置页面 (settings_screen)
    │
    └── 菜单详情页 (menu_detail_page)
            │
            └── 设置管理器 (settings_manager)
                    │
                    ├── 设置详情列表 (settings_detail)
                    │       ├── Wi-Fi
                    │       ├── Bluetooth
                    │       ├── Brightness
                    │       ├── Sleep Timer
                    │       └── ...
                    │
                    └── 子页面 (sub_page)
                            ├── WiFi设置页
                            └── 亮度调节页
```

#### 3.3.2 设置项配置

设置项通过枚举和配置数组定义：

```c
// 设置项ID枚举 (settings_manager.c)
typedef enum {
    UI_SETTINGS_PAGE_ID_WIFI = 0,        // Wi-Fi
    UI_SETTINGS_PAGE_ID_BLUETOOTH,       // 蓝牙
    UI_SETTINGS_PAGE_ID_BRIGHTNESS,      // 亮度
    UI_SETTINGS_PAGE_ID_SLEEP_TIMER,     // 睡眠定时
    UI_SETTINGS_PAGE_ID_BAIDU_NETDISK,   // 百度网盘
    UI_SETTINGS_PAGE_ID_VOLUME,          // 音量
    UI_SETTINGS_PAGE_ID_DO_NOT_DISTURB,  // 免打扰
    UI_SETTINGS_PAGE_ID_NIGHT_SCREEN_OFF,// 夜间关屏
    UI_SETTINGS_PAGE_ID_WECHAT_BINDING,  // 微信绑定
    UI_SETTINGS_PAGE_ID_SYSTEM_UPDATE,   // 系统更新
    UI_SETTINGS_PAGE_ID_FACTORY_RESET,   // 恢复出厂
    UI_SETTINGS_PAGE_ID_COUNT
} ui_settings_page_id_t;
```

#### 3.3.3 核心API

| 函数名 | 参数 | 返回值 | 说明 |
|--------|------|--------|------|
| `ui_settings_screen_create()` | `lv_obj_t *screen` | void | 创建设置主页面 |
| `ui_settings_manager_create_detail_page()` | `lv_obj_t *parent`, `ui_menu_detail_page_t *ctx` | `lv_obj_t*` | 创建设置详情页 |
| `ui_settings_manager_destroy_detail_page()` | `lv_obj_t **detail_page` | void | 销毁详情页及子页 |

#### 3.3.4 状态管理结构

```c
typedef struct {
    lv_obj_t *detail_page;                // 详情页对象
    lv_obj_t *sub_page;                   // 当前子页面（如WiFi、亮度）
    lv_obj_t *item_buttons[COUNT];        // 设置项按钮数组
    ui_settings_page_id_t selected_page;  // 当前选中项
    ui_menu_detail_page_t *menu_detail;   // 菜单上下文
} ui_settings_manager_t;
```

---

### 3.4 亮度设置页面 (setting_brightness)

**文件位置**: `project/ui/settings/setting_brightness/`

#### 3.4.1 页面结构
```
┌──────────────────────────┐
│  ← Brightness            │  ← Header (menu_detail_page_header)
├──────────────────────────┤
│                          │
│     Brightness           │  ← 标题标签
│                          │
│   ━━━━━━━━━━●━━━━━      │  ← Slider (0-100)
│                          │
│        50                │  ← 当前值显示
│                          │
└──────────────────────────┘
```

#### 3.4.2 核心API

```c
// 创建亮度调节页面
lv_obj_t *ui_setting_brightness_create(lv_obj_t *parent, lv_event_cb_t back_event_cb);
```

#### 3.4.3 实现细节

- **Slider范围**: 0-100（百分比）
- **默认值**: 50
- **事件回调**: `brightness_slider_event_cb` - 滑动时更新数值显示
- **待实现**: 需要接入LCD背光控制API设置实际亮度

```c
// 事件回调示例
static void brightness_slider_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    lv_obj_t *val_label = lv_event_get_user_data(e);
    int value = lv_slider_get_value(slider);
    lv_label_set_text_fmt(val_label, "%d", value);
    // TODO: 调用LCD背光设置API
}
```

---

### 3.5 WiFi设置页面 (setting_wifi)

**文件位置**: `project/ui/settings/setting_wifi/`

#### 3.5.1 页面结构
```
┌──────────────────────────┐
│  ← Wi-Fi           ↻    │  ← Header + 刷新按钮
├──────────────────────────┤
│ ┌──────────────────────┐ │
│ │  25 HomeWiFi    ✓    │ │  ← 已连接WiFi（高亮）
│ │  42 OfficeNet    *   │ │  ← 加密网络
│ │  18 GuestWiFi        │ │  ← 开放网络
│ │  ...                 │ │
│ └──────────────────────┘ │
└──────────────────────────┘

密码输入页：
┌──────────────────────────┐
│  ← HomeWiFi      ✓      │  ← 返回 + 确认
├──────────────────────────┤
│ ┌──────────────────────┐ │
│ │ ••••••••             │ │  ← 密码输入框
│ │                      │ │
│ │  [Q] [W] [E] [R]...  │ │  ← 键盘
│ │  ...                 │ │
│ └──────────────────────┘ │
└──────────────────────────┘
```

#### 3.5.2 核心API

```c
// 创建WiFi设置页面
lv_obj_t *ui_setting_wifi_create(lv_obj_t *parent, lv_event_cb_t back_event_cb);

// 外部调用：刷新WiFi列表
void ui_wifi_list_update(void);
```

#### 3.5.3 WiFi状态定义

```c
typedef enum {
    WIFI_STATUS_IDLE = 0,        // 未连接
    WIFI_STATUS_CONNECTED = 1,   // 已连接
    WIFI_STATUS_CONNECTING = 2,  // 连接中
    WIFI_STATUS_FAILED = 3       // 连接失败
} wifi_status_t;
```

#### 3.5.4 配色方案

| 常量 | 值 | 用途 |
|------|-----|------|
| `COLOR_THEME` | 0xffe454 | 主题黄：顶部栏、高亮项 |
| `COLOR_TEXT` | 0x2c2c2c | 深灰：文字、图标 |
| `COLOR_GREY` | 0xbbbbbb | 浅灰：提示、禁用态 |

#### 3.5.5 WiFi列表特性

- **自动扫描**: 进入页面自动触发扫描，300ms定时器刷新
- **扫描超时**: 9秒无结果显示 "Scan timeout"
- **排序规则**: 
  1. 当前连接的WiFi排在最前
  2. 信号强度降序
  3. 字母顺序
- **去重**: 同名SSID只显示一次
- **状态显示**: 信号值 + SSID + 加密锁 + 连接状态图标

#### 3.5.6 连接流程

```
1. 用户点击WiFi项
        │
2. 检查是否加密 (encrypt字段)
        │
   ┌────┴────┐
   │         │
  无加密    有加密
   │         │
   │    显示密码输入页
   │         │
   │    输入密码 → 确认
   │         │
3. 调用 wifi_service_connect()
        │
4. 更新连接状态
        │
5. 返回列表页刷新
```

#### 3.5.7 关键数据结构

```c
typedef struct {
    lv_obj_t *page;                 // 主页面
    lv_obj_t *header;               // 头部
    lv_obj_t *body;                 // 内容区
    lv_obj_t *wifi_list;            // WiFi列表容器
    lv_timer_t *wifi_scan_timer;    // 扫描定时器
    uint32_t scan_tick_count;       // 扫描计数

    lv_obj_t *password_page;        // 密码页
    lv_obj_t *password_textarea;    // 密码输入框
    lv_obj_t *connect_btn;          // 连接按钮

    char connecting_ssid[33];       // 当前连接中的SSID
} wifi_screen_t;
```

#### 3.5.8 WiFi服务集成

```c
// 初始化WiFi服务
wifi_service_init();

// 开始扫描
wifi_service_scan();

// 获取WiFi信息
wifi_info_t *wifi_service_get_wifi_info();

// 获取扫描状态
bool wifi_service_get_scan_status();

// 设置连接状态
wifi_service_set_connect_status(status);

// 连接WiFi
wifi_service_connect(ssid, password);
```

#### 3.5.9 密码输入页特性

- **密码模式**: 输入内容以 `•` 显示
- **最大长度**: 64字符
- **最小长度校验**: 密码少于8字符时确认按钮为灰色
- **键盘**: 自动弹出LVGL键盘输入
- **返回**: 可返回列表页

### 3.4 其他功能页面

| 模块 | 文件位置 | 说明 |
|------|----------|------|
| AI对话 | `ai_qa/ai_qa_screen.c/h` | AI问答界面 |
| 日历 | `calendar/calendar_screen.c/h` | 日历视图 |
| 音乐 | `music/music_screen.c/h` | 音乐播放界面 |
| 计划清单 | `plan_list/plan_list_screen.c/h` | 待办事项 |
| 定时 | `timing/timing_screen.c/h` | 定时器/秒表 |

---

## 4. 开发指南

### 4.1 添加新页面

**步骤1**: 在 `ui_screen_ids.h` 中添加页面ID
```c
typedef enum {
    // ... 现有ID ...
    UI_SCREEN_ID_NEW_PAGE,  // 新增
    UI_SCREEN_ID_COUNT
} ui_screen_id_t;
```

**步骤2**: 创建页面文件 `new_page/new_page_screen.c/h`
```c
// new_page_screen.h
#ifndef UI_NEW_PAGE_SCREEN_H
#define UI_NEW_PAGE_SCREEN_H
#include "lvgl/lvgl.h"
void ui_new_page_screen_create(lv_obj_t *screen);
#endif

// new_page_screen.c
#include "new_page_screen.h"
void ui_new_page_screen_create(lv_obj_t *screen) {
    // 创建UI元素
    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "New Page");
    lv_obj_center(label);
}
```

**步骤3**: 在 `ui_manager.c` 中注册页面
```c
// 添加头文件
#include "new_page/new_page_screen.h"

// 添加到配置数组
static const ui_screen_config_t g_ui_manager_screens[UI_SCREEN_ID_COUNT] = {
    // ... 现有配置 ...
    [UI_SCREEN_ID_NEW_PAGE] = {
        .name = "new_page",
        .image_path = "FFLASH:/icon.png",
        .create = ui_new_page_screen_create
    }
};
```

### 4.2 页面设计规范

#### 背景颜色
```c
// 使用主题色
lv_obj_set_style_bg_color(obj, lv_color_hex(UI_COLOR_BG_DARK), 0);
lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
```

#### 圆角样式
```c
lv_obj_set_style_radius(obj, UI_CARD_RADIUS, 0);
```

#### 字体使用
```c
lv_obj_set_style_text_font(obj, UI_FONT_BODY, 0);
```

### 4.3 注意事项

1. **内存管理**: 页面创建时避免内存泄漏
2. **性能优化**: 不在可见页面时暂停刷新定时器
3. **字体加载**: FreeType字体从 `FLASH:/myfont.ttf` 加载
4. **图片路径**: 使用 `FFLASH:/` 前缀访问Flash存储的图片
5. **滑动控制**: 使用 `ui_manager_set_tile_swipe_enabled()` 控制滑动

### 4.4 事件处理

```c
// 示例：添加点击事件
static void button_click_cb(lv_event_t *e) {
    // 处理点击事件
}

lv_obj_add_event_cb(button, button_click_cb, LV_EVENT_CLICKED, NULL);
```

---

## 5. 附录

### 5.1 文件结构
```
project/ui/
├── ui_manager.c/h          # UI管理器
├── ui_screen_ids.h         # 页面ID定义
├── ui_theme.h              # 主题配置
├── ai_qa/                  # AI对话页面
├── calendar/               # 日历页面
├── home/                   # 主页模块
│   ├── home_view.c/h       # 视图
│   └── home_refresh.c/h    # 刷新逻辑
├── menu/                   # 菜单模块
├── music/                  # 音乐页面
├── plan_list/              # 计划清单
├── settings/               # 设置模块
│   ├── settings_screen.c/h # 设置主页
│   ├── setting_brightness/ # 亮度设置
│   ├── setting_wifi/       # WiFi设置
│   └── settings_detail/    # 设置详情
└── timing/                 # 定时页面