/****************************************************************************
 * 文件名：ai_education_screen.c
 *
 * UI 整体结构（自上而下）：
 *   ┌────────────────────────────────────────┐
 *   │ [ < 返回菜单 ]        AI教育大模型       │  ← 黄色顶部 header（永久保留）
 *   ├────────────────────────────────────────┤
 *   │  ╭────────────────────────────────╮    │
 *   │  │  ◉  │  请叫"小微、小微"可以... │    │  ← 中部白卡（AI 头像 + 提示文字）
 *   │  ╰────────────────────────────────╯    │
 *   │                                        │
 *   │          (  🎤  按住说话  )            │  ← 底部青色按钮（按下变红）
 *   └────────────────────────────────────────┘
 *
 * 页面模式（用 ctx->chat_container 是否为 NULL 判断）：
 *   INTRO      - 初始介绍态：显示唤醒提示 + "按住说话"按钮
 *   CHAT       - 对话模式：消息列表（用户蓝色气泡 / AI 深色气泡）+ "按住说话"按钮
 *   录音态     - voice 按钮的瞬态：按下时显示"我在听"弹窗 + 按钮变红，松开立即恢复
 *               仅作为 UI 录音标志，不影响 ASR 流程，voice 按钮始终可点
 *
 * 数据流（队列驱动）：
 *   on_create 时启动 100ms 队列轮询 timer（永久运行）。
 *   页面所有真实消息都来自"待显示消息队列"（ctx->msg_queue，screen_msg_queue_t），
 *   timer 持续扫描队列，有节点就弹出并创建气泡显示。
 *
 * 调用方应使用以下 API 推入消息：
 *     - screen_msg_queue_push_dispatch(screen_id, text, 0) 推入 AI 回复消息
 *   页面本身不管理 ASR/AI 时序——完全由后端服务控制推入时机。
 *
 *   voice 按钮事件（PRESSED/RELEASED）只做 UI 反馈（弹窗 + 颜色/文字），
 *   不再禁用按钮、不再显示"识别中..."占位、不再触发 ASR。
 *
 * 屏幕对象关系：
 *   screen->root
 *     ├── header_panel     （on_create 时创建，不会被 clean）
 *     ├── recording_popup  （RECORDING 态时动态创建/销毁）
 *     └── detail_page      （on_create 时创建，状态切换时 clean 重建）
 *            ├─ INTRO 态：card（avatar + desc_label） + voice_btn
 *            └─ CHAT  态：chat_container（消息列表）  + voice_btn
 *
 * 入口：ui_ai_education_screen_create(parent, path)
 *   在菜单列表项 parent 上显示图标，触摸后 ui_ai_edu_push_new_screen() 压栈本页面
 *
 * 文案集中管理：本页所有用户可见的固定字符串统一在下面的
 *   ai_edu_str_*  常量中声明，业务方只需改动本段即可完成文案调整，
 *   无需在各处 UI 创建代码中逐个修改。
 ****************************************************************************/

/* LVGL 默认 lv_coord_t 为 16-bit（±32767），每条气泡约 30~60px，
 * 因此消息数量超过约 500 条时坐标会溢出，导致滚动失效和显示异常。
 * 此处限制最大保留消息数，防止坐标溢出并控制内存使用。 */
#define AI_EDU_MAX_MSGS 300

/* 等待弹窗：圆圈动画参数 */
#define AI_EDU_WAITING_CIRCLE_CNT  5
#define AI_EDU_WAITING_CIRCLE_SIZE 22
#define AI_EDU_WAITING_CIRCLE_GAP  10
#define AI_EDU_WAITING_ANIM_MS     200

/* ===== 文案常量：本页所有用户可见字符串集中在此，方便后续调整 ===== */
static const char *const ai_edu_str_title = "AI教育大模型"; /* 顶部标题 */
static const char *const ai_edu_str_back  = "返回菜单";     /* 左上角返回按钮 */
static const char *const ai_edu_str_intro =                 /* INTRO 态卡片右侧唤醒提示（4 行） */
        "请叫\"小微、小微\"可以唤醒我。\n"
        "有什么问题，都可以问我。\n"
        "我会写作，唱歌，讲故事等，\n"
        "快来和我聊聊吧！";
static const char *const ai_edu_str_voice_idle   = "按住说话";       /* 底部按钮常态文字 */
static const char *const ai_edu_str_voice_active = "已聆听";         /* 底部按钮按下态文字 */
static const char *const ai_edu_str_popup        = "我在听，你请说"; /* 录音弹窗内文字 */
static const char *const ai_edu_str_user_avatar  = "?";              /* 用户消息头像字符 */
static const char *const ai_edu_str_ai_avatar    = "AI";             /* AI 消息头像字符 */

/* 正式版：所有消息由后端服务推入队列，页面只负责从队列读取显示
 * 调用方应使用：
 *   - screen_msg_queue_push_dispatch() 通用消息推入（AI 回复 / 其他）
 */

#include "ui_manager.h"

#include "screen_manager.h"
// #include "ai_education_screen.h"

#include "../ui_theme.h"
#include "osal/string.h"
#include "screen_msg_queue.h"

/* ===== 颜色宏：统一管理本页面用到的所有颜色 ===== */
#define AI_EDU_HEADER_BG         0xFFCC33 /* 顶部黄色 header 背景 */
#define AI_EDU_HEADER_TEXT       0x1F2937 /* header 上文字色（深灰） */
#define AI_EDU_AVATAR_BG         0x1E3A8A /* INTRO 态 AI 头像主体深蓝 */
#define AI_EDU_AVATAR_RING       0x3B82F6 /* INTRO 态头像外圈光晕（半透明蓝） */
#define AI_EDU_AVATAR_EYE        0xFFFFFF /* INTRO 态头像眼睛白色 */
#define AI_EDU_CARD_BG           0xF1F5F9 /* INTRO 态中部白卡背景（浅灰白） */
#define AI_EDU_CARD_TEXT         0x1F2937 /* INTRO 态卡片内文字色 */
#define AI_EDU_BTN_BG            0x22D3EE /* 按住说话按钮 - 常态青色 */
#define AI_EDU_BTN_BG_PRESS      0xFF4444 /* 按住说话按钮 - 按下红色 */
#define AI_EDU_BTN_TEXT          0xFFFFFF /* 按钮文字白色 */
#define AI_EDU_BG_DARK           0x0F172A /* 屏幕根背景深色（仪表盘外壳） */
#define AI_EDU_POPUP_BG          0x3B82F6 /* 录音弹窗蓝色背景 */
#define AI_EDU_POPUP_TEXT        0xFFFFFF /* 弹窗声波 / 文字白色 */
#define AI_EDU_USER_BUBBLE_BG    0x3B82F6 /* 用户消息气泡蓝色背景 */
#define AI_EDU_USER_AVATAR_BG    0x1E3A8A /* 用户消息头像深蓝 */
#define AI_EDU_USER_TEXT         0xFFFFFF /* 用户消息气泡文字白色 */
#define AI_EDU_AI_BUBBLE_BG      0x374151 /* AI 消息气泡深灰背景 */
#define AI_EDU_AI_AVATAR_BG      0xF59E0B /* AI 消息头像橙色背景 */
#define AI_EDU_AI_TEXT           0xFFFFFF /* AI 消息气泡文字白色 */
#define AI_EDU_AI_AVATAR_TEXT    0xFFFFFF /* AI 头像"AI"文字白色 */
#define AI_EDU_WAITING_CIRCLE_BG 0xFFFFFF /* 等待弹窗圆圈白色 */

/* 页面模式：当前不需要枚举，通过 ctx->chat_container 是否为 NULL 判断
 *   - INTRO 态：chat_container == NULL（detail_page 内是 card 白卡）
 *   - CHAT  态：chat_container != NULL（detail_page 内是对话列表）
 *   - 录音是 voice 按钮的瞬态，由 PRESSED/RELEASED 事件自身处理，
 *     不持久化到 ctx 中
 */

/* 页面私有上下文，挂在 screen->user_data 上 */
typedef struct
{
    // 如果要支持消息队列,第一个参数一定是screen_msg_queue_t的结构体
    screen_msg_queue_t msg_queue;          /* 待显示消息队列 */
    screen_t          *screen;             /* 关联的 Screen 对象 */
    lv_obj_t          *detail_page;        /* 动态内容容器（位于 header 下方） */
    lv_obj_t          *card;               /* INTRO 态白卡（容纳头像 + 文字） */
    lv_obj_t          *voice_btn;          /* 底部"按住说话"按钮（INTRO/CHAT 共用） */
    lv_obj_t          *voice_label;        /* 按钮上的文字 */
    lv_obj_t          *desc_label;         /* INTRO 态卡片右侧提示文字 */
    lv_obj_t          *recording_popup;    /* 录音态弹窗（声波 + 文字），叠在中部） */
    lv_obj_t          *chat_container;     /* CHAT 态消息列表容器（NULL 表示 INTRO 态） */
    lv_timer_t        *queue_poll_timer;   /* 100ms 持续轮询待显示消息队列（on_create 启动，永久） */
    lv_obj_t          *waiting_popup;      /* 等待弹窗全屏遮罩（is_user=3 时显示） */
    lv_obj_t          *waiting_circle_box; /* 圆圈容器（5 个白色圆圈的父容器） */
    lv_timer_t        *circle_anim_timer;  /* 圆圈波浪动画 timer */
    uint8_t            circle_anim_idx;    /* 当前高亮圆圈索引 */
    uint8_t            show_voice_btn;     /* 是否显示"按住说话"按钮（由 params 控制） */

} ai_edu_ctx_t;

/* ===== 函数前置声明 ===== */
static void      ui_ai_edu_create_detail(screen_t *screen, void *params);
static void      ui_ai_edu_destroy_detail(screen_t *screen);
static void      ui_ai_edu_get_lifecycle(screen_lifecycle_t *lifecycle);
static void      ui_ai_edu_push_new_screen(void);
static void      ui_ai_edu_on_entry_click(lv_event_t *event);
static void      ui_ai_edu_on_back_click(lv_event_t *event);
static void      ai_edu_voice_btn_event(lv_event_t *event);
static void      ai_edu_create_header(lv_obj_t *page, ai_edu_ctx_t *ctx);
static lv_obj_t *ai_edu_create_avatar(lv_obj_t *parent);
static lv_obj_t *ai_edu_create_recording_popup(lv_obj_t *parent);
static void      ai_edu_destroy_recording_popup(ai_edu_ctx_t *ctx);
static void      ai_edu_show_intro_screen(ai_edu_ctx_t *ctx);
static void      ai_edu_show_chat_screen(ai_edu_ctx_t *ctx);
static lv_obj_t *ai_edu_create_chat_bubble(lv_obj_t *parent, const char *text, uint8_t is_user);
static void      ai_edu_queue_poll_timer_cb(lv_timer_t *timer);
static void      ai_edu_reset_detail_pointers(ai_edu_ctx_t *ctx);
static void      ai_edu_voice_btn_set_active(ai_edu_ctx_t *ctx);
static void      ai_edu_voice_btn_set_idle(ai_edu_ctx_t *ctx);
static void      ai_edu_circle_anim_cb(lv_timer_t *timer);
static void      ai_edu_show_waiting_popup(ai_edu_ctx_t *ctx);
static void      ai_edu_destroy_waiting_popup(ai_edu_ctx_t *ctx);

/****************************************************************************
 * 构建顶部黄色 header：包含左侧"返回菜单"按钮和居中标题
 * 注意：header 挂在 screen->root 上，不会被 detail_page 的 clean 误删
 ****************************************************************************/
static void ai_edu_create_header(lv_obj_t *page, ai_edu_ctx_t *ctx)
{
    lv_obj_t *header_panel;
    lv_obj_t *back_btn;
    lv_obj_t *back_label;
    lv_obj_t *title_label;

    /* 黄色胶囊状 header 容器 */
    header_panel = lv_obj_create(page);
    if (header_panel == NULL)
    {
        return;
    }

    lv_obj_remove_style_all(header_panel);
    lv_obj_set_size(header_panel, LV_PCT(96), 44);
    lv_obj_set_style_radius(header_panel, 22, 0);
    lv_obj_align(header_panel, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_bg_color(header_panel, lv_color_hex(AI_EDU_HEADER_BG), 0);
    lv_obj_set_style_bg_opa(header_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header_panel, 0, 0);
    lv_obj_set_style_shadow_width(header_panel, 0, 0);
    lv_obj_clear_flag(header_panel, LV_OBJ_FLAG_SCROLLABLE);

    /* 左上角"返回菜单"按钮：< 图标 + "返回菜单" 文字 */
    back_btn = lv_obj_create(header_panel);
    if (back_btn != NULL)
    {
        lv_obj_remove_style_all(back_btn);
        lv_obj_set_size(back_btn, 96, 36);
        lv_obj_set_style_radius(back_btn, 18, 0);
        lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(back_btn, 0, 0);
        lv_obj_set_style_shadow_width(back_btn, 0, 0);
        lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_flex_flow(back_btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(back_btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(back_btn, 4, 0);
        lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 4, 0);
        lv_obj_add_event_cb(back_btn, ui_ai_edu_on_back_click, LV_EVENT_CLICKED, ctx);

        /* < 左箭头图标 */
        lv_obj_t *back_icon = lv_label_create(back_btn);
        if (back_icon != NULL)
        {
            lv_label_set_text(back_icon, LV_SYMBOL_LEFT);
            lv_obj_set_style_text_font(back_icon, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(back_icon, lv_color_hex(AI_EDU_HEADER_TEXT), 0);
        }

        /* "返回菜单" 文字 */
        back_label = lv_label_create(back_btn);
        if (back_label != NULL)
        {
            lv_label_set_text(back_label, ai_edu_str_back);
            lv_obj_set_style_text_font(back_label, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(back_label, lv_color_hex(AI_EDU_HEADER_TEXT), 0);
        }
    }

    /* 居中标题：AI教育大模型（向右偏移 28 避开左侧返回按钮） */
    title_label = lv_label_create(header_panel);
    if (title_label != NULL)
    {
        lv_label_set_text(title_label, ai_edu_str_title);
        lv_obj_set_style_text_font(title_label, UI_FONT_BODY, 0);
        lv_obj_set_style_text_color(title_label, lv_color_hex(AI_EDU_HEADER_TEXT), 0);
        lv_obj_align(title_label, LV_ALIGN_CENTER, 28, 0);
    }
}

/****************************************************************************
 * 构建 AI 头像：外圈半透明光晕 + 内部深蓝圆形 + 两个白色眼睛
 * 挂在 card 内左侧
 ****************************************************************************/
static lv_obj_t *ai_edu_create_avatar(lv_obj_t *parent)
{
    lv_obj_t *ring;
    lv_obj_t *head;
    lv_obj_t *eye_l;
    lv_obj_t *eye_r;

    /* 外层光晕（半透明蓝） */
    ring = lv_obj_create(parent);
    if (ring == NULL)
    {
        return NULL;
    }
    lv_obj_remove_style_all(ring);
    lv_obj_set_size(ring, 76, 76);
    lv_obj_set_style_radius(ring, 38, 0);
    lv_obj_set_style_bg_color(ring, lv_color_hex(AI_EDU_AVATAR_RING), 0);
    lv_obj_set_style_bg_opa(ring, LV_OPA_40, 0);
    lv_obj_set_style_border_width(ring, 0, 0);
    lv_obj_set_style_shadow_width(ring, 0, 0);
    lv_obj_align(ring, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_SCROLLABLE);

    /* 内层深蓝机器人头部 */
    head = lv_obj_create(ring);
    if (head != NULL)
    {
        lv_obj_remove_style_all(head);
        lv_obj_set_size(head, 60, 60);
        lv_obj_set_style_radius(head, 30, 0);
        lv_obj_set_style_bg_color(head, lv_color_hex(AI_EDU_AVATAR_BG), 0);
        lv_obj_set_style_bg_opa(head, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(head, 0, 0);
        lv_obj_set_style_shadow_width(head, 0, 0);
        lv_obj_clear_flag(head, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_center(head);

        /* 两只白色眼睛（左/右） */
        eye_l = lv_obj_create(head);
        eye_r = lv_obj_create(head);
        if (eye_l != NULL)
        {
            lv_obj_remove_style_all(eye_l);
            lv_obj_set_size(eye_l, 10, 14);
            lv_obj_set_style_radius(eye_l, 5, 0);
            lv_obj_set_style_bg_color(eye_l, lv_color_hex(AI_EDU_AVATAR_EYE), 0);
            lv_obj_set_style_bg_opa(eye_l, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(eye_l, 0, 0);
            lv_obj_clear_flag(eye_l, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_align(eye_l, LV_ALIGN_CENTER, -10, 0);
        }
        if (eye_r != NULL)
        {
            lv_obj_remove_style_all(eye_r);
            lv_obj_set_size(eye_r, 10, 14);
            lv_obj_set_style_radius(eye_r, 5, 0);
            lv_obj_set_style_bg_color(eye_r, lv_color_hex(AI_EDU_AVATAR_EYE), 0);
            lv_obj_set_style_bg_opa(eye_r, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(eye_r, 0, 0);
            lv_obj_clear_flag(eye_r, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_align(eye_r, LV_ALIGN_CENTER, 10, 0);
        }
    }
    return ring;
}

/****************************************************************************
 * 底部"按住说话"按钮的触摸事件回调
 *   voice 按钮仅作录音 UI 标志，不参与 ASR 状态管理，不影响队列数据
 *   PRESSED  - 按下：按钮变红 + 文字"已聆听" + 叠加"我在听"弹窗
 *   RELEASED - 松开：销毁弹窗 + 按钮恢复青色 + 文字"按住说话"
 *   CLICKED  - 占位
 *
 * 按钮文字/颜色变化与 PRESSED/RELEASED 事件一一对应，方便后续维护和调试
 * ASR 启动/结束由外部模块自己控制，UI 不感知
 ****************************************************************************/
static void ai_edu_voice_btn_event(lv_event_t *event)
{
    ai_edu_ctx_t   *ctx  = (ai_edu_ctx_t *) lv_event_get_user_data(event);
    lv_event_code_t code = lv_event_get_code(event);

    if (ctx == NULL)
    {
        return;
    }

    if (code == LV_EVENT_PRESSED)
    {
        /* 按下：按钮文字"按住说话"→"已聆听"，颜色青色→红色 + 叠加"我在听"弹窗 */
        ai_edu_voice_btn_set_active(ctx);
        if (ctx->recording_popup == NULL && ctx->screen != NULL)
        {
            ctx->recording_popup = ai_edu_create_recording_popup(ctx->screen->root);
        }
        if (ctx->screen->cb)
        {
            ctx->screen->cb(ctx->screen->id, CALLBACK_CMD_RECORDING, ctx->screen->cb_priv, 1);
        }
    }
    else if (code == LV_EVENT_RELEASED)
    {
        /* 松开：销毁弹窗 + 按钮文字"已聆听"→"按住说话"，颜色红色→青色 */
        ai_edu_destroy_recording_popup(ctx);
        ai_edu_voice_btn_set_idle(ctx);
        if (ctx->screen->cb)
        {
            ctx->screen->cb(ctx->screen->id, CALLBACK_CMD_RECORDING, ctx->screen->cb_priv, 0);
        }
    }
    else if (code == LV_EVENT_CLICKED)
    {
    }
}

/* voice 按钮按下态：红色背景 + "已聆听" 文字（PRESSED 触发） */
static void ai_edu_voice_btn_set_active(ai_edu_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }
    if (ctx->voice_btn != NULL)
    {
        lv_obj_set_style_bg_color(ctx->voice_btn, lv_color_hex(AI_EDU_BTN_BG_PRESS), 0);
    }
    if (ctx->voice_label != NULL)
    {
        lv_label_set_text(ctx->voice_label, ai_edu_str_voice_active);
    }
}

/* voice 按钮常态：青色背景 + "按住说话" 文字（RELEASED / 创建时使用） */
static void ai_edu_voice_btn_set_idle(ai_edu_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }
    if (ctx->voice_btn != NULL)
    {
        lv_obj_set_style_bg_color(ctx->voice_btn, lv_color_hex(AI_EDU_BTN_BG), 0);
    }
    if (ctx->voice_label != NULL)
    {
        lv_label_set_text(ctx->voice_label, ai_edu_str_voice_idle);
    }
}

/****************************************************************************
 * 构建录音态弹窗：蓝色圆角矩形，中央偏下
 *   上半部：5 根白色声波竖条（中间高两边低，模拟音频波形）
 *   下半部：白色文字"我在听，你请说"
 * 挂在 screen->root 上，叠在白卡中央偏下位置
 ****************************************************************************/
static lv_obj_t *ai_edu_create_recording_popup(lv_obj_t *parent)
{
    lv_obj_t            *popup          = NULL;
    lv_obj_t            *wave_bar       = NULL;
    /* 5 根声波条的高度比例：左矮、中高、右矮，模拟音频波形 */
    static const uint8_t wave_heights[] = {8, 18, 28, 18, 8};

    if (parent == NULL)
    {
        return NULL;
    }

    /* 蓝色弹窗容器 */
    popup = lv_obj_create(parent);
    if (popup == NULL)
    {
        return NULL;
    }
    lv_obj_remove_style_all(popup);
    lv_obj_set_size(popup, 180, 80);
    lv_obj_align(popup, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_radius(popup, 14, 0);
    lv_obj_set_style_bg_color(popup, lv_color_hex(AI_EDU_POPUP_BG), 0);
    lv_obj_set_style_bg_opa(popup, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(popup, 0, 0);
    lv_obj_set_style_shadow_width(popup, 0, 0);
    lv_obj_set_flex_flow(popup, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(popup, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(popup, 10, 0);
    lv_obj_set_style_pad_bottom(popup, 10, 0);
    lv_obj_set_style_pad_row(popup, 6, 0);
    lv_obj_clear_flag(popup, LV_OBJ_FLAG_SCROLLABLE);

    /* 声波条容器：横向排列 5 根白色竖条 */
    {
        lv_obj_t *wave_box = lv_obj_create(popup);
        if (wave_box != NULL)
        {
            uint8_t i;
            lv_obj_remove_style_all(wave_box);
            lv_obj_set_size(wave_box, LV_SIZE_CONTENT, 32);
            lv_obj_set_flex_flow(wave_box, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(wave_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END);
            lv_obj_set_style_pad_column(wave_box, 5, 0);
            lv_obj_clear_flag(wave_box, LV_OBJ_FLAG_SCROLLABLE);

            for (i = 0; i < sizeof(wave_heights) / sizeof(wave_heights[0]); i++)
            {
                wave_bar = lv_obj_create(wave_box);
                if (wave_bar != NULL)
                {
                    lv_obj_remove_style_all(wave_bar);
                    lv_obj_set_size(wave_bar, 5, wave_heights[i]);
                    lv_obj_set_style_radius(wave_bar, 2, 0);
                    lv_obj_set_style_bg_color(wave_bar, lv_color_hex(AI_EDU_POPUP_TEXT), 0);
                    lv_obj_set_style_bg_opa(wave_bar, LV_OPA_COVER, 0);
                    lv_obj_clear_flag(wave_bar, LV_OBJ_FLAG_SCROLLABLE);
                }
            }
        }
    }

    /* 弹窗文字："我在听，你请说" */
    {
        lv_obj_t *popup_label = lv_label_create(popup);
        if (popup_label != NULL)
        {
            lv_label_set_text(popup_label, ai_edu_str_popup);
            lv_obj_set_style_text_color(popup_label, lv_color_hex(AI_EDU_POPUP_TEXT), 0);
            lv_obj_set_style_text_font(popup_label, UI_FONT_BODY, 0);
        }
    }

    return popup;
}

/* 销毁录音弹窗（由 ai_edu_show_intro_screen 调用，回到 INTRO 态时清理） */
static void ai_edu_destroy_recording_popup(ai_edu_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }
    if (ctx->recording_popup != NULL)
    {
        lv_obj_del(ctx->recording_popup);
        ctx->recording_popup = NULL;
    }
}

/****************************************************************************
 * 等待弹窗：全屏半透明深色遮罩 + 5 个白色圆圈（波浪动画）
 *   is_user=3 时由队列轮询 timer 触发显示
 *   收到真实消息（is_user=0/1）时自动关闭
 *   圆圈动画：每隔 AI_EDU_WAITING_ANIM_MS 高亮下一个圆圈，形成从左到右的波浪效果
 ****************************************************************************/
static void ai_edu_circle_anim_cb(lv_timer_t *timer)
{
    ai_edu_ctx_t *ctx;
    lv_obj_t     *prev_circle;
    lv_obj_t     *curr_circle;
    int           cnt;

    if (timer == NULL)
    {
        return;
    }
    ctx = (ai_edu_ctx_t *) timer->user_data;
    if (ctx == NULL || ctx->waiting_circle_box == NULL)
    {
        return;
    }

    cnt = (int) lv_obj_get_child_cnt(ctx->waiting_circle_box);
    if (cnt <= 0)
    {
        return;
    }

    /* 恢复上一个圆圈为暗淡状态 */
    prev_circle = lv_obj_get_child(ctx->waiting_circle_box, ctx->circle_anim_idx);
    if (prev_circle != NULL)
    {
        lv_obj_set_style_bg_opa(prev_circle, LV_OPA_30, 0);
    }

    /* 移动到下一个圆圈 */
    ctx->circle_anim_idx = (ctx->circle_anim_idx + 1) % cnt;

    /* 高亮当前圆圈 */
    curr_circle = lv_obj_get_child(ctx->waiting_circle_box, ctx->circle_anim_idx);
    if (curr_circle != NULL)
    {
        lv_obj_set_style_bg_opa(curr_circle, LV_OPA_COVER, 0);
    }
}

static void ai_edu_show_waiting_popup(ai_edu_ctx_t *ctx)
{
    lv_obj_t *overlay;
    lv_obj_t *circle_box;
    int       i;

    if (ctx == NULL || ctx->screen == NULL)
    {
        return;
    }

    /* 已经显示则不重复创建 */
    if (ctx->waiting_popup != NULL)
    {
        return;
    }

    /* 非全屏遮罩：仅覆盖 detail_page 区域（header 下方），保留 header 可交互 */
    overlay = lv_obj_create(ctx->screen->root);
    if (overlay == NULL)
    {
        return;
    }
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, LV_PCT(100), LV_VER_RES - 60);
    lv_obj_align(overlay, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(AI_EDU_BG_DARK), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_70, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    ctx->waiting_popup = overlay;

    /* 圆圈容器：水平居中排列 */
    circle_box = lv_obj_create(overlay);
    if (circle_box == NULL)
    {
        return;
    }
    lv_obj_remove_style_all(circle_box);
    lv_obj_set_size(circle_box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_center(circle_box);
    lv_obj_set_flex_flow(circle_box, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(circle_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(circle_box, AI_EDU_WAITING_CIRCLE_GAP, 0);
    lv_obj_clear_flag(circle_box, LV_OBJ_FLAG_SCROLLABLE);
    ctx->waiting_circle_box = circle_box;

    /* 创建 5 个白色圆圈，第一个默认高亮 */
    for (i = 0; i < AI_EDU_WAITING_CIRCLE_CNT; i++)
    {
        lv_obj_t *circle = lv_obj_create(circle_box);
        if (circle != NULL)
        {
            lv_obj_remove_style_all(circle);
            lv_obj_set_size(circle, AI_EDU_WAITING_CIRCLE_SIZE, AI_EDU_WAITING_CIRCLE_SIZE);
            lv_obj_set_style_radius(circle, AI_EDU_WAITING_CIRCLE_SIZE / 2, 0);
            lv_obj_set_style_bg_color(circle, lv_color_hex(AI_EDU_WAITING_CIRCLE_BG), 0);
            lv_obj_set_style_bg_opa(circle, (i == 0) ? LV_OPA_COVER : LV_OPA_30, 0);
            lv_obj_set_style_border_width(circle, 0, 0);
            lv_obj_clear_flag(circle, LV_OBJ_FLAG_SCROLLABLE);
        }
    }

    /* 启动圆圈波浪动画 timer */
    ctx->circle_anim_idx = 0;
    if (ctx->circle_anim_timer == NULL)
    {
        ctx->circle_anim_timer = lv_timer_create(ai_edu_circle_anim_cb, AI_EDU_WAITING_ANIM_MS, ctx);
    }
}

static void ai_edu_destroy_waiting_popup(ai_edu_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    /* 停止动画 timer */
    if (ctx->circle_anim_timer != NULL)
    {
        lv_timer_del(ctx->circle_anim_timer);
        ctx->circle_anim_timer = NULL;
    }

    /* 销毁弹窗对象（子对象自动清理） */
    if (ctx->waiting_popup != NULL)
    {
        lv_obj_del(ctx->waiting_popup);
        ctx->waiting_popup = NULL;
    }
    ctx->waiting_circle_box = NULL;
}

/****************************************************************************
 * 重置 detail_page 相关的所有 ctx 指针 + 销毁各类 timer/弹窗
 * 必须在 lv_obj_clean(detail_page) 之前调用，避免悬空指针
 *  - card / voice_btn / voice_label / desc_label / chat_container /
 *    recording_popup 都是 LVGL 对象指针，clean 后会失效
 *  - 待显示消息队列节点是 malloc 分配的，必须单独释放避免内存泄漏
 *  - 单独调 ai_edu_destroy_recording_popup 处理挂在 screen->root 上的弹窗
 ****************************************************************************/
static void ai_edu_reset_detail_pointers(ai_edu_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }
    /* 录音弹窗挂在 screen->root 上，不受 detail_page clean 影响，单独销毁 */
    ai_edu_destroy_recording_popup(ctx);
    /* 等待弹窗同样挂在 screen->root 上，单独销毁 */
    ai_edu_destroy_waiting_popup(ctx);

    /* 把所有可能指向被 detail_page clean 删掉的对象的指针全部置 NULL */
    ctx->card           = NULL;
    ctx->voice_btn      = NULL;
    ctx->voice_label    = NULL;
    ctx->desc_label     = NULL;
    ctx->chat_container = NULL;
}

/****************************************************************************
 * 显示初始介绍态：白卡（头像 + 唤醒提示）+ 底部"按住说话"按钮
 * 先 clean 掉 detail_page，再重建 card / voice_btn；
 * header 不在 detail_page 内，所以不会被误删
 ****************************************************************************/
static void ai_edu_show_intro_screen(ai_edu_ctx_t *ctx)
{
    lv_obj_t *card;
    lv_obj_t *avatar;
    lv_obj_t *desc_label;
    lv_obj_t *voice_btn;
    lv_obj_t *voice_icon;
    lv_obj_t *voice_label;

    if (ctx == NULL || ctx->detail_page == NULL)
    {
        return;
    }

    /* 回到 INTRO 态时清空待显示消息队列 */
    screen_msg_queue_clear(&ctx->msg_queue);
    /* 统一清理 detail_page 相关指针 + 销毁弹窗，避免悬空指针 */
    ai_edu_reset_detail_pointers(ctx);
    lv_obj_clean(ctx->detail_page);

    /* 中部白卡：浅灰白底圆角矩形，容纳头像和提示文字 */
    card = lv_obj_create(ctx->detail_page);
    if (card == NULL)
    {
        return;
    }
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, LV_PCT(90), LV_PCT(60));
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 8);
    lv_obj_set_style_radius(card, 18, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(AI_EDU_CARD_BG), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    ctx->card = card;

    /* 卡片左侧：AI 头像（光晕 + 圆头 + 双眼） */
    avatar = ai_edu_create_avatar(card);

    /* 卡片右侧：4 行唤醒提示文字（自动换行） */
    desc_label = lv_label_create(card);
    if (desc_label != NULL)
    {
        lv_label_set_text(desc_label, ai_edu_str_intro);
        lv_obj_set_width(desc_label, LV_PCT(58));
        lv_label_set_long_mode(desc_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_color(desc_label, lv_color_hex(AI_EDU_CARD_TEXT), 0);
        lv_obj_set_style_text_font(desc_label, UI_FONT_BODY, 0);
        lv_obj_set_style_text_line_space(desc_label, 4, 0);
        lv_obj_align(desc_label, LV_ALIGN_TOP_RIGHT, -2, 6);
        ctx->desc_label = desc_label;
        (void) avatar;
    }

    /* 底部"按住说话"按钮：根据 show_voice_btn 标志决定是否创建 */
    if (ctx->show_voice_btn)
    {
        voice_btn = lv_obj_create(ctx->detail_page);
        if (voice_btn == NULL)
        {
            return;
        }
        lv_obj_remove_style_all(voice_btn);
        lv_obj_set_size(voice_btn, 160, 38);
        lv_obj_align(voice_btn, LV_ALIGN_BOTTOM_MID, 0, -14);
        lv_obj_set_style_radius(voice_btn, 19, 0);
        lv_obj_set_style_bg_color(voice_btn, lv_color_hex(AI_EDU_BTN_BG), 0);
        lv_obj_set_style_bg_opa(voice_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(voice_btn, 0, 0);
        lv_obj_set_style_shadow_width(voice_btn, 0, 0);
        lv_obj_set_flex_flow(voice_btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(voice_btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(voice_btn, 6, 0);
        lv_obj_add_flag(voice_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(voice_btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(voice_btn, ai_edu_voice_btn_event, LV_EVENT_ALL, ctx);
        ctx->voice_btn = voice_btn;

        /* 按钮左侧：麦克风图标 */
        voice_icon  = lv_label_create(voice_btn);
        voice_label = lv_label_create(voice_btn);
        if (voice_icon != NULL)
        {
            lv_label_set_text(voice_icon, LV_SYMBOL_AUDIO);
            lv_obj_set_style_text_color(voice_icon, lv_color_hex(AI_EDU_BTN_TEXT), 0);
            lv_obj_set_style_text_font(voice_icon, UI_FONT_BODY, 0);
        }
        /* 按钮右侧："按住说话" 文字 */
        if (voice_label != NULL)
        {
            lv_label_set_text(voice_label, ai_edu_str_voice_idle);
            lv_obj_set_style_text_color(voice_label, lv_color_hex(AI_EDU_BTN_TEXT), 0);
            lv_obj_set_style_text_font(voice_label, UI_FONT_BODY, 0);
            ctx->voice_label = voice_label;
        }
    }
}

/****************************************************************************
 * 创建一条对话气泡行：头像 + 气泡
 *   is_user = 1  - 用户消息：右对齐，蓝色气泡 + 白色文字，蓝色头像 + "?"
 *   is_user = 0  - AI 消息：左对齐，深灰气泡 + 白色文字，橙色头像 + "AI"
 * 挂在 chat_container 内，单行高度自适应内容
 * 返回创建的 row 容器指针，调用方需要时可保存用于后续删除（如占位气泡）
 ****************************************************************************/
static lv_obj_t *ai_edu_create_chat_bubble(lv_obj_t *parent, const char *text, uint8_t is_user)
{
    lv_obj_t *row;
    lv_obj_t *avatar;
    lv_obj_t *avatar_label;
    lv_obj_t *bubble;
    lv_obj_t *bubble_label;

    if (parent == NULL || text == NULL)
    {
        return NULL;
    }

    /* 一行：横向排列，气泡 flex_grow 占据剩余宽度 */
    row = lv_obj_create(parent);
    if (row == NULL)
    {
        return NULL;
    }
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_top(row, 6, 0);
    lv_obj_set_style_pad_bottom(row, 6, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    if (is_user)
    {
        /* 用户：右对齐，左侧留空给头像 */
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_left(row, 40, 0);
        lv_obj_set_style_pad_right(row, 4, 0);
        /* 创建顺序：先气泡再头像，让头像在右、bubble 在左 */
        bubble = lv_obj_create(row);
        avatar = lv_obj_create(row);
    }
    else
    {
        /* AI：左对齐，右侧留空 */
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_left(row, 4, 0);
        lv_obj_set_style_pad_right(row, 40, 0);
        /* 创建顺序：先头像再气泡 */
        avatar = lv_obj_create(row);
        bubble = lv_obj_create(row);
    }

    /* 头像：28x28 圆形 */
    if (avatar != NULL)
    {
        lv_obj_remove_style_all(avatar);
        lv_obj_set_size(avatar, 28, 28);
        lv_obj_set_style_radius(avatar, 14, 0);
        lv_obj_set_style_border_width(avatar, 0, 0);
        lv_obj_set_style_shadow_width(avatar, 0, 0);
        lv_obj_set_style_pad_all(avatar, 0, 0);
        lv_obj_set_style_bg_opa(avatar, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(avatar, lv_color_hex(is_user ? AI_EDU_USER_AVATAR_BG : AI_EDU_AI_AVATAR_BG), 0);
        lv_obj_clear_flag(avatar, LV_OBJ_FLAG_SCROLLABLE);

        avatar_label = lv_label_create(avatar);
        if (avatar_label != NULL)
        {
            lv_label_set_text(avatar_label, is_user ? ai_edu_str_user_avatar : ai_edu_str_ai_avatar);
            lv_obj_set_style_text_color(avatar_label, lv_color_hex(is_user ? AI_EDU_USER_TEXT : AI_EDU_AI_AVATAR_TEXT), 0);
            lv_obj_set_style_text_font(avatar_label, UI_FONT_BODY, 0);
            lv_obj_center(avatar_label);
        }
    }

    /* 气泡：圆角矩形，内部文字自动换行 */
    if (bubble == NULL)
    {
        return NULL;
    }
    lv_obj_remove_style_all(bubble);
    lv_obj_set_height(bubble, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(bubble, 1);
    lv_obj_set_style_max_width(bubble, LV_PCT(70), 0);
    lv_obj_set_style_border_width(bubble, 0, 0);
    lv_obj_set_style_shadow_width(bubble, 0, 0);
    lv_obj_set_style_pad_left(bubble, 10, 0);
    lv_obj_set_style_pad_right(bubble, 10, 0);
    lv_obj_set_style_pad_top(bubble, 8, 0);
    lv_obj_set_style_pad_bottom(bubble, 8, 0);
    lv_obj_set_style_radius(bubble, 12, 0);
    lv_obj_set_style_bg_color(bubble, lv_color_hex(is_user ? AI_EDU_USER_BUBBLE_BG : AI_EDU_AI_BUBBLE_BG), 0);
    lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
    lv_obj_clear_flag(bubble, LV_OBJ_FLAG_SCROLLABLE);

    bubble_label = lv_label_create(bubble);
    if (bubble_label != NULL)
    {
        lv_label_set_text(bubble_label, text);
        lv_label_set_long_mode(bubble_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(bubble_label, LV_PCT(100));
        lv_obj_set_style_text_color(bubble_label, lv_color_hex(is_user ? AI_EDU_USER_TEXT : AI_EDU_AI_TEXT), 0);
        lv_obj_set_style_text_font(bubble_label, UI_FONT_BODY, 0);
    }

    return row;
}

/* 滚动对话列表到底部（最新消息可见） */
static void ai_edu_scroll_chat_to_bottom(ai_edu_ctx_t *ctx)
{
    lv_obj_t *last;

    if (ctx == NULL || ctx->chat_container == NULL)
    {
        return;
    }
    /* 取最后一个子项（最新消息）并滚动到可见位置。
     * 用 lv_obj_scroll_to_view 而非 lv_obj_scroll_to_y(LV_COORD_MAX)，
     * 因为后者在 flex 子项非常多时可能因坐标极限 clamp 失效 */
    last = lv_obj_get_child(ctx->chat_container, -1);
    if (last != NULL)
    {
        lv_obj_scroll_to_view(last, LV_ANIM_OFF);
    }
}

/****************************************************************************
 * 进入 CHAT 态：clean detail_page，建对话列表容器 + 底部 voice 按钮
 * 然后调用 add_chat_messages 添加第一组模拟消息
 ****************************************************************************/
static void ai_edu_show_chat_screen(ai_edu_ctx_t *ctx)
{
    lv_obj_t *chat_box;
    lv_obj_t *voice_btn;
    lv_obj_t *voice_icon;
    lv_obj_t *voice_label;

    if (ctx == NULL || ctx->detail_page == NULL)
    {
        return;
    }

    /* 统一清理 detail_page 相关指针 + 销毁弹窗 + 删 timer，避免悬空指针 */
    ai_edu_reset_detail_pointers(ctx);
    lv_obj_clean(ctx->detail_page);

    /* 对话列表容器：flex column，纵向排列消息，支持滚动 */
    chat_box = lv_obj_create(ctx->detail_page);
    if (chat_box == NULL)
    {
        return;
    }
    lv_obj_remove_style_all(chat_box);
    /* 根据是否显示语音按钮调整聊天列表高度 */
    if (ctx->show_voice_btn)
    {
        lv_obj_set_size(chat_box, LV_PCT(100), LV_VER_RES - 60 - 56);
    }
    else
    {
        lv_obj_set_size(chat_box, LV_PCT(100), LV_VER_RES - 60);
    }
    lv_obj_align(chat_box, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_flex_flow(chat_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_left(chat_box, 6, 0);
    lv_obj_set_style_pad_right(chat_box, 6, 0);
    lv_obj_set_style_pad_top(chat_box, 4, 0);
    lv_obj_set_style_pad_bottom(chat_box, 4, 0);
    lv_obj_set_scroll_dir(chat_box, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(chat_box, LV_SCROLLBAR_MODE_AUTO);
    ctx->chat_container = chat_box;
    /* 注意：队列轮询 timer 在 on_create 时启动（永久运行），这里不重复启动 */

    /* 底部"按住说话"按钮：根据 show_voice_btn 标志决定是否创建 */
    if (ctx->show_voice_btn)
    {
        voice_btn = lv_obj_create(ctx->detail_page);
        if (voice_btn == NULL)
        {
            return;
        }
        lv_obj_remove_style_all(voice_btn);
        lv_obj_set_size(voice_btn, 160, 38);
        lv_obj_align(voice_btn, LV_ALIGN_BOTTOM_MID, 0, -14);
        lv_obj_set_style_radius(voice_btn, 19, 0);
        lv_obj_set_style_bg_color(voice_btn, lv_color_hex(AI_EDU_BTN_BG), 0);
        lv_obj_set_style_bg_opa(voice_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(voice_btn, 0, 0);
        lv_obj_set_style_shadow_width(voice_btn, 0, 0);
        lv_obj_set_flex_flow(voice_btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(voice_btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(voice_btn, 6, 0);
        lv_obj_add_flag(voice_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(voice_btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(voice_btn, ai_edu_voice_btn_event, LV_EVENT_ALL, ctx);
        ctx->voice_btn = voice_btn;

        voice_icon  = lv_label_create(voice_btn);
        voice_label = lv_label_create(voice_btn);
        if (voice_icon != NULL)
        {
            lv_label_set_text(voice_icon, LV_SYMBOL_AUDIO);
            lv_obj_set_style_text_color(voice_icon, lv_color_hex(AI_EDU_BTN_TEXT), 0);
            lv_obj_set_style_text_font(voice_icon, UI_FONT_BODY, 0);
        }
        if (voice_label != NULL)
        {
            lv_label_set_text(voice_label, ai_edu_str_voice_idle);
            lv_obj_set_style_text_color(voice_label, lv_color_hex(AI_EDU_BTN_TEXT), 0);
            lv_obj_set_style_text_font(voice_label, UI_FONT_BODY, 0);
            ctx->voice_label = voice_label;
        }
    }
}

/****************************************************************************
 * 队列轮询 timer 回调（100ms 周期）：
 *   持续扫描待显示消息队列，弹出所有节点并创建气泡显示。
 *   如果 chat_container 尚未创建但队列有数据，自动创建对话界面。
 *   is_user=3 为特殊命令：显示等待弹窗（圆圈动画），收到真实消息时关闭。
 ****************************************************************************/
static void ai_edu_queue_poll_timer_cb(lv_timer_t *timer)
{
    ai_edu_ctx_t      *ctx;
    screen_msg_node_t *node;

    if (timer == NULL)
    {
        return;
    }
    ctx = (ai_edu_ctx_t *) timer->user_data;
    if (ctx == NULL)
    {
        return;
    }

    /* 没有待显示消息，无需处理 */
    if (screen_msg_queue_is_empty(&ctx->msg_queue))
    {
        return;
    }

    /* 弹出并处理所有待显示节点 */
    while (!screen_msg_queue_is_empty(&ctx->msg_queue))
    {
        node = screen_msg_queue_pop(&ctx->msg_queue);
        if (node == NULL)
        {
            break;
        }

        /* is_user=3：特殊命令，显示等待弹窗（圆圈动画），不需要创建气泡 */
        if (node->is_user == 3)
        {
            ai_edu_show_waiting_popup(ctx);
            screen_msg_node_free(node);
            continue;
        }

        /* 收到真实消息（is_user=0/1）：关闭等待弹窗 */
        ai_edu_destroy_waiting_popup(ctx);

        /* 队列有真实消息但 chat_container 还未创建 → 自动创建对话界面 */
        if (ctx->chat_container == NULL)
        {
            ai_edu_show_chat_screen(ctx);
            if (ctx->chat_container == NULL)
            {
                screen_msg_node_free(node);
                return; /* 创建失败，下次轮询重试 */
            }
        }

        /* 创建气泡显示 */
        if (node->text != NULL)
        {
            ai_edu_create_chat_bubble(ctx->chat_container, node->text, node->is_user);
        }
        screen_msg_node_free(node);
    }

    /* 限制消息总数，防止 lv_coord_t 16-bit 坐标溢出 */
    if (ctx->chat_container != NULL)
    {
        int cnt    = (int) lv_obj_get_child_cnt(ctx->chat_container);
        int excess = cnt - AI_EDU_MAX_MSGS;
        while (excess > 0)
        {
            lv_obj_t *old = lv_obj_get_child(ctx->chat_container, 0);
            if (old != NULL)
            {
                lv_obj_del(old);
            }
            excess--;
        }

        ai_edu_scroll_chat_to_bottom(ctx);
    }
}

/****************************************************************************
 * 顶部"返回菜单"按钮回调：
 *   当前策略：无论当前在 CHAT 态还是 INTRO 态，点击 back 都直接退出当前页面
 *
 *   注释代码保留了"CHAT 态回 INTRO 介绍态、INTRO 态才退出"的分级返回逻辑。
 *   如需恢复该行为：删除下方的 #if 0 ... #endif 包裹，并注释掉 screen_finish 调用。
 ****************************************************************************/
static void ui_ai_edu_on_back_click(lv_event_t *event)
{
    ai_edu_ctx_t *ctx = (ai_edu_ctx_t *) lv_event_get_user_data(event);

    if (ctx == NULL || ctx->screen == NULL)
    {
        return;
    }

#if 0
    /* 注释：CHAT 态点击 back 时先回 INTRO 介绍态（清空对话、销毁弹窗/计时器） */
    if (ctx->chat_container != NULL)
    {
        ai_edu_show_intro_screen(ctx);
        return;
    }
#endif
    screen_finish(ctx->screen);
}

/****************************************************************************
 * on_create：首次进入页面时调用
 *   1. 申请 ctx 上下文并挂到 screen->user_data
 *   2. 配置 screen->root 深色背景
 *   3. 在 root 上创建 header（永久保留）
 *   4. 在 root 上创建 detail_page（让出 header 区域，可 clean 重建）
 *   5. 启动 100ms 队列轮询 timer（永久运行，扫描待显示消息队列）
 *   6. 调用 show_intro_screen 渲染初始介绍态
 ****************************************************************************/
static void ui_ai_edu_create_detail(screen_t *screen, void *params)
{
    ai_edu_ctx_t *ctx;
    lv_obj_t     *page;

    if (screen == NULL)
    {
        return;
    }

    /* 申请并初始化页面私有上下文 */
    ctx = (ai_edu_ctx_t *) os_malloc(sizeof(ai_edu_ctx_t));
    if (ctx == NULL)
    {
        return;
    }
    lv_memset(ctx, 0, sizeof(ai_edu_ctx_t));
    ctx->screen = screen;
    screen_set_user_data(screen, ctx);
    screen_msg_queue_init(&ctx->msg_queue);

    /* params 为 NULL 时显示语音按钮，非 NULL 时不显示 */
    ctx->show_voice_btn = (params == NULL) ? 1 : 0;

    /* 配置根容器：全屏深色背景 */
    lv_obj_remove_style_all(screen->root);
    lv_obj_set_size(screen->root, LV_PCT(100), LV_PCT(100));
    lv_obj_center(screen->root);
    lv_obj_set_style_bg_color(screen->root, lv_color_hex(AI_EDU_BG_DARK), 0);
    lv_obj_set_style_bg_opa(screen->root, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen->root, LV_OBJ_FLAG_SCROLLABLE);

    /* 顶部 header：与 detail_page 平级，作为 screen->root 的子对象（不会被 clean 误删） */
    ai_edu_create_header(screen->root, ctx);

    /* 动态内容容器：从 y=60 开始，让出 header 占用的顶部 60px 高度 */
    page = lv_obj_create(screen->root);
    if (page == NULL)
    {
        return;
    }
    lv_obj_remove_style_all(page);
    lv_obj_set_size(page, LV_PCT(100), LV_VER_RES - 60);
    lv_obj_align(page, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    ctx->detail_page = page;

    /* 启动 100ms 队列轮询 timer（永久运行，扫描待显示消息队列显示） */
    if (ctx->queue_poll_timer == NULL)
    {
        ctx->queue_poll_timer = lv_timer_create(ai_edu_queue_poll_timer_cb, 100, ctx);
        lv_timer_set_repeat_count(ctx->queue_poll_timer, -1);
    }

    /* 渲染初始介绍态 UI */
    ai_edu_show_intro_screen(ctx);
}

/****************************************************************************
 * on_destroy：页面销毁时调用，释放 ctx 内存
 * detail_page / header 等 LVGL 对象由 screen_manager 统一销毁
 ****************************************************************************/
static void ui_ai_edu_destroy_detail(screen_t *screen)
{
    ai_edu_ctx_t *ctx;

    if (screen == NULL)
    {
        return;
    }

    ctx = (ai_edu_ctx_t *) screen_get_user_data(screen);
    if (ctx == NULL)
    {
        return;
    }
    /* 销毁等待弹窗（含动画 timer） */
    ai_edu_destroy_waiting_popup(ctx);

    /* 清空 ctx 内部指针（不释放 LVGL 对象，由 screen_manager 统一销毁） */
    ctx->screen             = NULL;
    ctx->detail_page        = NULL;
    ctx->card               = NULL;
    ctx->voice_btn          = NULL;
    ctx->voice_label        = NULL;
    ctx->desc_label         = NULL;
    ctx->recording_popup    = NULL;
    ctx->chat_container     = NULL;
    ctx->waiting_popup      = NULL;
    ctx->waiting_circle_box = NULL;
    ctx->circle_anim_timer  = NULL;
    ctx->circle_anim_idx    = 0;
    /* 队列轮询 timer：on_create 启动，页面销毁时统一停 */
    if (ctx->queue_poll_timer != NULL)
    {
        lv_timer_del(ctx->queue_poll_timer);
        ctx->queue_poll_timer = NULL;
    }
    screen_msg_queue_clear(&ctx->msg_queue);
    screen_set_user_data(screen, NULL);
    os_free(ctx);
}

/* ===== 生命周期钩子：本页面无硬件资源/按键钩子，全部空实现 ===== */
static void ui_ai_edu_screen_start(screen_t *screen)
{
    (void) screen;
}

static void ui_ai_edu_screen_resume(screen_t *screen)
{
    (void) screen;
}

static void ui_ai_edu_screen_pause(screen_t *screen)
{
    (void) screen;
}

static void ui_ai_edu_screen_stop(screen_t *screen)
{
    (void) screen;
}

static void ui_ai_edu_screen_restart(screen_t *screen)
{
    (void) screen;
}

static void ui_ai_edu_screen_finish(screen_t *screen)
{
    (void) screen;
}

/* 组装生命周期回调集合 */
static void ui_ai_edu_get_lifecycle(screen_lifecycle_t *lifecycle)
{
    if (lifecycle == NULL)
    {
        return;
    }

    *lifecycle = (screen_lifecycle_t) {
            .on_create  = ui_ai_edu_create_detail,
            .on_start   = ui_ai_edu_screen_start,
            .on_resume  = ui_ai_edu_screen_resume,
            .on_pause   = ui_ai_edu_screen_pause,
            .on_stop    = ui_ai_edu_screen_stop,
            .on_destroy = ui_ai_edu_destroy_detail,
            .on_restart = ui_ai_edu_screen_restart,
            .on_finish  = ui_ai_edu_screen_finish,
            .screen_id  = 0,
    };
}

/* 注册并压栈一个新 Screen（菜单入口点击时调用） */
static void ui_ai_edu_push_new_screen(void)
{
    screen_lifecycle_t lifecycle;
    screen_t          *screen;

    ui_ai_edu_get_lifecycle(&lifecycle);
    screen = screen_register(NULL, &lifecycle);
    if (screen != NULL)
    {
extern int32 coze_ui_cb(uint32_t screen_id, enum callback_cmd_t cmd, void *priv,uint32_t param);
        screen->cb = coze_ui_cb;
        screen_push(screen, NULL);
    }
}

/* 菜单入口图标点击回调：触发 push 新页面 */
static void ui_ai_edu_on_entry_click(lv_event_t *event)
{
    (void) event;
    ui_ai_edu_push_new_screen();
}

/****************************************************************************
 * 外部入口：在主菜单列表项上调用，绑定入口图标和触摸回调
 *   parent - 菜单列表的某个 item
 *   path   - 入口图标图片路径（由 ui_image_show 显示）
 * 用法示例（在 main_ui.c 等列表中）：
 *   ui_ai_education_screen_create(list_item, icon_path);
 ****************************************************************************/
void ui_ai_education_screen_create(lv_obj_t *parent, const char *path)
{
    if (parent == NULL)
    {
        return;
    }

    ui_image_show(parent, path);
    lv_obj_add_flag(parent, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(parent, ui_ai_edu_on_entry_click, LV_EVENT_CLICKED, NULL);
}

uint32_t extern_ai_education_screen_create(struct screen_common_s *m)
{
    screen_lifecycle_t lifecycle;
    screen_t          *screen;
    uint32_t           real_screen_id = 0;
    ui_ai_edu_get_lifecycle(&lifecycle);
    lifecycle.screen_id = m->screen_id;
    lifecycle.only      = m->ui_id;
    screen              = screen_register(NULL, &lifecycle);
    if (screen != NULL)
    {
        screen->cb      = m->cb;
        screen->cb_priv = m->cb_priv;
        screen_push(screen, (void *) 1);
        real_screen_id = screen->id;
    }
    return real_screen_id;
}