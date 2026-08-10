
#include "basic_include.h"
#include "ui_manager.h"

#include "screen_manager.h"
#include "screen_memory.h"
#include "screen_common.h"

#include "ui_theme.h"
#include "lvgl/lvgl.h"
#include "lib/multimedia/txmplayer.h"
/****************************************************************************
 * 文件名：media_player_ui.c
 *
 * UI 整体结构（自上而下）：
 *   ┌────────────────────────────────────────┐
 *   │ [ < 返回 ]            音乐畅听          │  ← 顶部黄色 header
 *   ├────────────────────────────────────────┤
 *   │  ╭────╮   【放音邪会VOL.3】女...         │
 *   │  │ 💿 │   Disc B (伴奏)(伴奏)          │  ← 中部：蓝色唱片 + 歌曲信息
 *   │  │    │   歌手:放映邪会                │
 *   │  ╰────╯   作词人：音情晨曦              │
 *   │                                        │
 *   │  00:00 ━━━━━━━━━━━━━━━━━━━━━━ 00:00   │  ← 进度条
 *   │      ⏮    ▶    ⏭    🔊                 │  ← 4 个控制按钮
 *   │                                  ┌─┐    │  ← 点击 🔊 后右侧弹出音量滑块
 *   │                                  │▓│    │
 *   │                                  └─┘    │
 *   └────────────────────────────────────────┘
 *
 * 适配 Screen Manager 框架：
 *   - ctx 放在 screen->user_data（owns_self 控制释放）
 *   - 返回按钮：screen_finish(ui_s->screen)
 *   - 本期不支持物理按键（无 group / indev_keypad）
 *
 * 入口：media_player_ui_create(parent, path)
 *   在菜单列表项 parent 上显示图标，点击后 push 新页面
 ****************************************************************************/

/* ===== 文案常量：集中管理用户可见字符串，方便后续调整 ===== */
static const char *const mp_str_title  = "音乐畅听";
static const char *const mp_str_back   = "返回";
static const char *const mp_str_song   = "【放音邪会VOL.3】女...";
static const char *const mp_str_album  = "Disc B (伴奏)(伴奏)";
static const char *const mp_str_artist = "歌手:放映邪会";
static const char *const mp_str_lyric  = "作词人:音情晨曦";
static const char *const mp_str_time   = "00:00";

/* ===== 颜色宏：统一管理本页面用到的所有颜色 ===== */
#define MP_BG_DARK       0x000000  /* 屏幕根背景（黑色仪表盘） */
#define MP_HEADER_BG     0xFFCC33  /* 顶部黄色 header 背景 */
#define MP_HEADER_TEXT   0x000000  /* header 上文字色（黑色） */
#define MP_DISC_BG       0x1E40AF  /* 蓝色唱片主体 */
#define MP_DISC_RING     0x3B82F6  /* 唱片外圈光晕 */
#define MP_DISC_CENTER   0xFCD34D  /* 唱片中心黄色（针孔+标签） */
#define MP_DISC_NEEDLE   0xFFFFFF  /* 唱针白色 */
#define MP_SONG_NAME     0xFCD34D  /* 歌曲名黄色 */
#define MP_INFO_WHITE    0xFFFFFF  /* 歌曲信息白色 */
#define MP_INFO_LABEL    0xFCD34D  /* "作词人"标签黄色 */
#define MP_BAR_BG        0xFFFFFF  /* 进度条背景（白色轨道） */
#define MP_BAR_INDICATOR 0xFFFFFF  /* 进度条指示（保持白色） */
#define MP_TIME_TEXT     0xFFFFFF  /* 时间文字白色 */
#define MP_BTN_ICON      0xFFFFFF  /* 控制按钮图标白色 */
#define MP_VOL_BG        0xFFF700  /* 音量滑块轨道（黄色） */
#define MP_VOL_KNOB      0xFFF700  /* 音量滑块手柄（黄色） */
#define MP_VOL_BG_OPA    LV_OPA_40 /* 音量滑块轨道透明度 */

extern lv_style_t g_style;

/* 自适应缩放：以 480px 高度为设计基准，小屏等比缩小，
 * 不低于 2px 防止元素不可见。320x240 时缩放因子约 0.5 */
#define MP_BASE_H   480
#define MP_SCALE(v) ((lv_coord_t) LV_MAX((int32_t) (v) * (LV_VER_RES) / MP_BASE_H, 2))

/* 页面私有上下文，挂在 screen->user_data 上 */
typedef struct
{
    screen_t *screen; /* 关联的 Screen 对象 */

    /* UI 控件指针（不显隐 root，由 screen_manager 统一管理） */
    lv_obj_t *header_panel;     /* 顶部黄色胶囊 header */
    lv_obj_t *back_btn;         /* 左上角返回按钮 */
    lv_obj_t *disc;             /* 中部左侧蓝色唱片 */
    lv_obj_t *progress_bar;     /* 进度条 */
    lv_obj_t *play_time_label;  /* 当前播放时间 00:00 */
    lv_obj_t *total_time_label; /* 总时长 00:00 */
    lv_obj_t *btn_prev;         /* 上一首 */
    lv_obj_t *btn_play;         /* 播放/暂停 */
    lv_obj_t *btn_next;         /* 下一首 */
    lv_obj_t *btn_volume;       /* 音量按钮 */
    lv_obj_t *volume_slider;    /* 音量调节滑块（默认隐藏，点击音量按钮后显示） */

    uint32_t stream_id;

    /* 状态 */
    uint8_t  is_playing;       /* 播放/暂停状态：0=暂停，1=播放 */
    uint8_t  volume;           /* 当前音量（0-100） */
    uint32_t last_update_time; /* 上次更新时间播放时间ui的time（毫秒） */
    uint32_t play_time;        /* 当前播放时间（秒） */
    uint32_t total_time;       /* 总时长（秒） */

    /* timer diff 检测用的缓存值：与源变量比对，不同才刷新 UI 并同步缓存 */
    uint8_t  cached_playing;    /* 缓存的 playing 值 */
    uint8_t  cached_volume;     /* 缓存的 volume 值 */
    uint32_t cached_play_time;  /* 缓存的播放时间（秒） */
    uint32_t cached_total_time; /* 缓存的总时长（秒） */

    lv_timer_t *event_timer; /* UI 状态同步定时器 */
} media_player_ctx_t;

/* ===== 函数前置声明 ===== */
static void media_player_get_lifecycle(screen_lifecycle_t *lifecycle);
static void media_player_create(screen_t *screen, void *params);
static void media_player_destroy(screen_t *screen);
static void media_player_start(screen_t *screen);
static void media_player_resume(screen_t *screen);
static void media_player_pause(screen_t *screen);
static void media_player_stop(screen_t *screen);
static void media_player_restart(screen_t *screen);
static void media_player_finish(screen_t *screen);

static void media_player_on_back_click(lv_event_t *e);
static void media_player_on_prev_click(lv_event_t *e);
static void media_player_on_play_click(lv_event_t *e);
static void media_player_on_next_click(lv_event_t *e);
static void media_player_on_volume_click(lv_event_t *e);
static void media_player_on_volume_changed(lv_event_t *e);
static void media_player_on_progress_click(lv_event_t *e);

static void media_player_format_time(uint32_t time_sec, char *buf);
static void media_player_seek_to_time(media_player_ctx_t *ui_s, uint32_t time_sec);
static void media_player_event_timer_cb(lv_timer_t *t);

static void      media_player_create_header(lv_obj_t *page, media_player_ctx_t *ui_s);
static lv_obj_t *media_player_create_disc(lv_obj_t *parent);
static lv_obj_t *media_player_create_song_info(lv_obj_t *parent);
static lv_obj_t *media_player_create_progress(lv_obj_t *parent);
static lv_obj_t *media_player_create_control_btn(lv_obj_t *parent, const char *symbol);
static lv_obj_t *media_player_create_volume_slider(lv_obj_t *parent);

static void media_player_format_time(uint32_t time_sec, char *buf)
{
    uint32_t minute;
    uint32_t second;

    if (buf == NULL)
    {
        return;
    }

    minute = time_sec / 60;
    second = time_sec % 60;
    os_sprintf(buf, "%02d:%02d", (int) minute, (int) second);
}

/****************************************************************************
 * 控件创建：4 个控制按钮共用的工厂函数（上一首/播放/下一首/音量）
 * 透明背景 + 大字体白色图标，挂在传入的 parent 上
 * 返回创建的 btn 指针
 ****************************************************************************/
static lv_obj_t *media_player_create_control_btn(lv_obj_t *parent, const char *symbol)
{
    lv_obj_t *btn;
    lv_obj_t *label;

    if (parent == NULL || symbol == NULL)
    {
        return NULL;
    }

    btn = lv_btn_create(parent);
    if (btn == NULL)
    {
        return NULL;
    }
    lv_obj_remove_style_all(btn);
    lv_obj_set_size(btn, MP_SCALE(80), MP_SCALE(60));
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_radius(btn, MP_SCALE(8), 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    label = lv_label_create(btn);
    if (label != NULL)
    {
        lv_label_set_text(label, symbol);
        lv_obj_set_style_text_color(label, lv_color_hex(MP_BTN_ICON), 0);
        //lv_obj_set_style_text_font(label, UI_FONT_BODY, 0);
        lv_obj_center(label);
    }
    return btn;
}

/****************************************************************************
 * 构建右侧垂直音量调节滑块：
 *   - 初始为隐藏状态（LV_OBJ_FLAG_HIDDEN）
 *   - 点击音量按钮时切换显示/隐藏
 *   - 黄色轨道 + 黄色圆形手柄
 * 挂在传入的 parent 上
 ****************************************************************************/
static lv_obj_t *media_player_create_volume_slider(lv_obj_t *parent)
{
    lv_obj_t *slider;

    if (parent == NULL)
    {
        return NULL;
    }

    slider = lv_slider_create(parent);
    if (slider == NULL)
    {
        return NULL;
    }
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);
    /* 垂直方向（height > width） */
    lv_obj_set_size(slider, MP_SCALE(30), MP_SCALE(180));
    /* 轨道（背景）：黄色半透明 */
    lv_obj_set_style_bg_color(slider, lv_color_hex(MP_VOL_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider, MP_VOL_BG_OPA, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, MP_SCALE(8), LV_PART_MAIN);
    /* 指示部分（已填充）：黄色 */
    lv_obj_set_style_bg_color(slider, lv_color_hex(MP_VOL_BG), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, MP_SCALE(8), LV_PART_INDICATOR);
    /* 手柄：黄色圆形 */
    lv_obj_set_style_bg_color(slider, lv_color_hex(MP_VOL_KNOB), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_radius(slider, MP_SCALE(12), LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider, MP_SCALE(4), LV_PART_KNOB);
    /* 定位到右侧（不与控制按钮同行） */
    lv_obj_align(slider, LV_ALIGN_RIGHT_MID, MP_SCALE(-20), 0);
    /* 初始隐藏 */
    lv_obj_add_flag(slider, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(slider, LV_OBJ_FLAG_SCROLLABLE);

    return slider;
}

/****************************************************************************
 * 构建顶部黄色 header：包含左侧"< 返回"按钮和居中标题
 * 挂在 screen->root 上，永久保留（不在 detail_page 中）
 ****************************************************************************/
static void media_player_create_header(lv_obj_t *page, media_player_ctx_t *ui_s)
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
    lv_obj_set_size(header_panel, LV_PCT(96), MP_SCALE(50));
    lv_obj_set_style_radius(header_panel, MP_SCALE(25), 0);
    lv_obj_align(header_panel, LV_ALIGN_TOP_MID, 0, MP_SCALE(10));
    lv_obj_set_style_bg_color(header_panel, lv_color_hex(MP_HEADER_BG), 0);
    lv_obj_set_style_bg_opa(header_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header_panel, 0, 0);
    lv_obj_set_style_shadow_width(header_panel, 0, 0);
    lv_obj_clear_flag(header_panel, LV_OBJ_FLAG_SCROLLABLE);
    ui_s->header_panel = header_panel;

    /* 左上角"返回"按钮：< 图标 + "返回" 文字 */
    back_btn = lv_obj_create(header_panel);
    if (back_btn != NULL)
    {
        lv_obj_remove_style_all(back_btn);
        lv_obj_set_size(back_btn, MP_SCALE(110), MP_SCALE(40));
        lv_obj_set_style_radius(back_btn, MP_SCALE(20), 0);
        lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(back_btn, 0, 0);
        lv_obj_set_style_shadow_width(back_btn, 0, 0);
        lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_flex_flow(back_btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(back_btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(back_btn, MP_SCALE(4), 0);
        lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, MP_SCALE(8), 0);
        lv_obj_add_event_cb(back_btn, media_player_on_back_click, LV_EVENT_CLICKED, ui_s);
        ui_s->back_btn = back_btn;

        /* < 左箭头图标 */
        back_icon = lv_label_create(back_btn);
        if (back_icon != NULL)
        {
            lv_label_set_text(back_icon, LV_SYMBOL_LEFT);
            //lv_obj_set_style_text_font(back_icon, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(back_icon, lv_color_hex(MP_HEADER_TEXT), 0);
        }

        /* "返回" 文字 */
        back_label = lv_label_create(back_btn);
        if (back_label != NULL)
        {
            lv_label_set_text(back_label, mp_str_back);
            lv_obj_set_style_text_font(back_label, UI_FONT_BODY, 0);
            lv_obj_set_style_text_color(back_label, lv_color_hex(MP_HEADER_TEXT), 0);
        }
    }

    /* 居中标题：音乐畅听（向右偏移避开左侧返回按钮） */
    title_label = lv_label_create(header_panel);
    if (title_label != NULL)
    {
        lv_label_set_text(title_label, mp_str_title);
        lv_obj_set_style_text_font(title_label, UI_FONT_BODY, 0);
        lv_obj_set_style_text_color(title_label, lv_color_hex(MP_HEADER_TEXT), 0);
        lv_obj_align(title_label, LV_ALIGN_CENTER, MP_SCALE(40), 0);
    }
}

/****************************************************************************
 * 构建中部左侧蓝色唱片：
 *   外圈：深蓝圆形（唱片主体）
 *   中圈：浅蓝高光
 *   中心：黄色圆点（针孔+标签）
 *   右上角：白色唱针
 *   中央：可旋转的浅色圆盘（模拟唱片纹路）
 * 挂在传入的 parent 上
 ****************************************************************************/
static lv_obj_t *media_player_create_disc(lv_obj_t *parent)
{
    lv_obj_t *disc;
    lv_obj_t *ring;
    lv_obj_t *center;
    lv_obj_t *needle;
    lv_obj_t *spindle;

    if (parent == NULL)
    {
        return NULL;
    }

    /* 唱片主体：深蓝大圆 */
    disc = lv_obj_create(parent);
    if (disc == NULL)
    {
        return NULL;
    }
    lv_obj_remove_style_all(disc);
    lv_obj_set_size(disc, MP_SCALE(200), MP_SCALE(200));
    lv_obj_set_style_radius(disc, MP_SCALE(100), 0);
    lv_obj_set_style_bg_color(disc, lv_color_hex(MP_DISC_BG), 0);
    lv_obj_set_style_bg_opa(disc, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(disc, 0, 0);
    lv_obj_set_style_shadow_width(disc, 0, 0);
    lv_obj_clear_flag(disc, LV_OBJ_FLAG_SCROLLABLE);

    /* 唱片外圈光晕 */
    ring = lv_obj_create(disc);
    if (ring != NULL)
    {
        lv_obj_remove_style_all(ring);
        lv_obj_set_size(ring, MP_SCALE(180), MP_SCALE(180));
        lv_obj_set_style_radius(ring, MP_SCALE(90), 0);
        lv_obj_set_style_bg_color(ring, lv_color_hex(MP_DISC_RING), 0);
        lv_obj_set_style_bg_opa(ring, LV_OPA_30, 0);
        lv_obj_set_style_border_width(ring, 0, 0);
        lv_obj_set_style_shadow_width(ring, 0, 0);
        lv_obj_center(ring);
        lv_obj_clear_flag(ring, LV_OBJ_FLAG_SCROLLABLE);
    }

    /* 唱片中心黄色标签 */
    center = lv_obj_create(disc);
    if (center != NULL)
    {
        lv_obj_remove_style_all(center);
        lv_obj_set_size(center, MP_SCALE(40), MP_SCALE(40));
        lv_obj_set_style_radius(center, MP_SCALE(20), 0);
        lv_obj_set_style_bg_color(center, lv_color_hex(MP_DISC_CENTER), 0);
        lv_obj_set_style_bg_opa(center, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(center, 0, 0);
        lv_obj_set_style_shadow_width(center, 0, 0);
        lv_obj_center(center);
        lv_obj_clear_flag(center, LV_OBJ_FLAG_SCROLLABLE);

        /* 中心针孔（深色小圆） */
        spindle = lv_obj_create(center);
        if (spindle != NULL)
        {
            lv_obj_remove_style_all(spindle);
            lv_obj_set_size(spindle, MP_SCALE(6), MP_SCALE(6));
            lv_obj_set_style_radius(spindle, MP_SCALE(3), 0);
            lv_obj_set_style_bg_color(spindle, lv_color_hex(MP_BG_DARK), 0);
            lv_obj_set_style_bg_opa(spindle, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(spindle, 0, 0);
            lv_obj_center(spindle);
            lv_obj_clear_flag(spindle, LV_OBJ_FLAG_SCROLLABLE);
        }
    }

    /* 唱针：白色细长矩形，从右上角斜插到唱片中心
     * 用一个略倾斜的杆（视觉上像唱针） */
    needle = lv_obj_create(disc);
    if (needle != NULL)
    {
        lv_obj_remove_style_all(needle);
        lv_obj_set_size(needle, MP_SCALE(6), MP_SCALE(50));
        lv_obj_set_style_radius(needle, MP_SCALE(3), 0);
        lv_obj_set_style_bg_color(needle, lv_color_hex(MP_DISC_NEEDLE), 0);
        lv_obj_set_style_bg_opa(needle, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(needle, 0, 0);
        lv_obj_set_style_shadow_width(needle, 0, 0);
        /* 唱针从右上角向中心方向摆放，pivot 在杆的顶端 */
        lv_obj_align(needle, LV_ALIGN_TOP_RIGHT, MP_SCALE(-15), MP_SCALE(15));
        lv_obj_clear_flag(needle, LV_OBJ_FLAG_SCROLLABLE);
    }

    return disc;
}

/****************************************************************************
 * 构建中部右侧歌曲信息：4 行文字（歌名/专辑/歌手/作词）
 *   歌名   - 黄色
 *   专辑   - 白色
 *   歌手   - 白色
 *   作词   - 黄色"作词人:" + 白色姓名
 * 挂在传入的 parent 上
 ****************************************************************************/
static lv_obj_t *media_player_create_song_info(lv_obj_t *parent)
{
    lv_obj_t *info_box;
    lv_obj_t *lbl_song;
    lv_obj_t *lbl_album;
    lv_obj_t *lbl_artist;
    lv_obj_t *lbl_lyric;

    if (parent == NULL)
    {
        return NULL;
    }

    /* 4 行文字容器 */
    info_box = lv_obj_create(parent);
    if (info_box == NULL)
    {
        return NULL;
    }
    lv_obj_remove_style_all(info_box);
    lv_obj_set_size(info_box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(info_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info_box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(info_box, MP_SCALE(6), 0);
    lv_obj_clear_flag(info_box, LV_OBJ_FLAG_SCROLLABLE);

    /* 歌名（黄色） */
    lbl_song = lv_label_create(info_box);
    if (lbl_song != NULL)
    {
        lv_label_set_text(lbl_song, mp_str_song);
        lv_obj_set_style_text_color(lbl_song, lv_color_hex(MP_SONG_NAME), 0);
        lv_obj_set_style_text_font(lbl_song, UI_FONT_BODY, 0);
    }

    /* 专辑（白色） */
    lbl_album = lv_label_create(info_box);
    if (lbl_album != NULL)
    {
        lv_label_set_text(lbl_album, mp_str_album);
        lv_obj_set_style_text_color(lbl_album, lv_color_hex(MP_INFO_WHITE), 0);
        lv_obj_set_style_text_font(lbl_album, UI_FONT_BODY, 0);
    }

    /* 歌手（白色） */
    lbl_artist = lv_label_create(info_box);
    if (lbl_artist != NULL)
    {
        lv_label_set_text(lbl_artist, mp_str_artist);
        lv_obj_set_style_text_color(lbl_artist, lv_color_hex(MP_INFO_WHITE), 0);
        lv_obj_set_style_text_font(lbl_artist, UI_FONT_BODY, 0);
    }

    /* 作词（黄色"作词人:" + 白色姓名） */
    lbl_lyric = lv_label_create(info_box);
    if (lbl_lyric != NULL)
    {
        lv_label_set_text(lbl_lyric, mp_str_lyric);
        lv_obj_set_style_text_color(lbl_lyric, lv_color_hex(MP_INFO_LABEL), 0);
        lv_obj_set_style_text_font(lbl_lyric, UI_FONT_BODY, 0);
    }

    return info_box;
}

/****************************************************************************
 * 构建进度条区域：左 00:00 + 中间 bar + 右 00:00
 * 挂在传入的 parent 上，使用 flex 横向布局
 ****************************************************************************/
static lv_obj_t *media_player_create_progress(lv_obj_t *parent)
{
    lv_obj_t *prog_box;
    lv_obj_t *bar;
    lv_obj_t *lbl_cur;
    lv_obj_t *lbl_total;

    if (parent == NULL)
    {
        return NULL;
    }

    /* 进度条行容器 */
    prog_box = lv_obj_create(parent);
    if (prog_box == NULL)
    {
        return NULL;
    }
    lv_obj_remove_style_all(prog_box);
    lv_obj_set_size(prog_box, LV_PCT(90), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(prog_box, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(prog_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(prog_box, MP_SCALE(10), 0);
    lv_obj_clear_flag(prog_box, LV_OBJ_FLAG_SCROLLABLE);

    /* 左侧当前时间 00:00 */
    lbl_cur = lv_label_create(prog_box);
    if (lbl_cur != NULL)
    {
        lv_label_set_text(lbl_cur, mp_str_time);
        lv_obj_set_style_text_color(lbl_cur, lv_color_hex(MP_TIME_TEXT), 0);
        lv_obj_set_style_text_font(lbl_cur, UI_FONT_BODY, 0);
    }

    /* 中间进度条（flex grow 占满剩余空间） */
    bar = lv_bar_create(prog_box);
    if (bar != NULL)
    {
        lv_bar_set_range(bar, 0, 100);
        lv_bar_set_value(bar, 0, LV_ANIM_OFF);
        lv_obj_set_flex_grow(bar, 1);
        lv_obj_set_style_radius(bar, MP_SCALE(4), 0);
        lv_obj_set_style_bg_color(bar, lv_color_hex(MP_BAR_BG), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bar, LV_OPA_40, LV_PART_MAIN);
        lv_obj_set_style_radius(bar, MP_SCALE(4), LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(bar, lv_color_hex(MP_BAR_INDICATOR), LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
        lv_obj_clear_flag(bar, LV_OBJ_FLAG_CLICKABLE);
    }

    /* 右侧总时长 00:00 */
    lbl_total = lv_label_create(prog_box);
    if (lbl_total != NULL)
    {
        lv_label_set_text(lbl_total, mp_str_time);
        lv_obj_set_style_text_color(lbl_total, lv_color_hex(MP_TIME_TEXT), 0);
        lv_obj_set_style_text_font(lbl_total, UI_FONT_BODY, 0);
    }

    return prog_box;
}

/****************************************************************************
 * 返回按钮点击回调：触发 screen_finish()，由 on_destroy 释放资源
 ****************************************************************************/
static void media_player_on_back_click(lv_event_t *e)
{
    media_player_ctx_t *ui_s = (media_player_ctx_t *) lv_event_get_user_data(e);
    if (ui_s == NULL || ui_s->screen == NULL)
    {
        return;
    }
    os_printf("media_player: click back btn\n");
    screen_finish(ui_s->screen);
}

/****************************************************************************
 * 上一首按钮点击（占位，后续接音频服务）
 ****************************************************************************/
static void media_player_on_prev_click(lv_event_t *e)
{
    media_player_ctx_t *ui_s = (media_player_ctx_t *) lv_event_get_user_data(e);
    if (ui_s == NULL || ui_s->screen == NULL)
    {
        return;
    }
    os_printf("media_player: click prev btn\n");
}

/****************************************************************************
 * 播放/暂停按钮点击：切换图标（▶ ⏸ 占位）
 ****************************************************************************/
static void media_player_on_play_click(lv_event_t *e)
{
    media_player_ctx_t *ui_s = (media_player_ctx_t *) lv_event_get_user_data(e);
    if (ui_s == NULL || ui_s->btn_play == NULL)
    {
        return;
    }
    /* 基于缓存值（UI 当前显示状态）翻转，避免 is_playing 被异步事件回调修改
   导致翻转结果与按钮当前图标不一致的临界问题 */
    uint8_t new_playing  = (ui_s->cached_playing == 0) ? 1 : 0;
    ui_s->is_playing     = new_playing;
    ui_s->cached_playing = new_playing;
    lv_obj_t *label      = lv_obj_get_child(ui_s->btn_play, 0);
    if (label != NULL)
    {
        txmplayer_pause(ui_s->stream_id, new_playing ? 0 : 1);
        lv_label_set_text(label, new_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    }
    os_printf("media_player: click play btn, is_playing=%d\n", new_playing);
}

/****************************************************************************
 * 下一首按钮点击（占位，后续接音频服务）
 ****************************************************************************/
static void media_player_on_next_click(lv_event_t *e)
{
    media_player_ctx_t *ui_s = (media_player_ctx_t *) lv_event_get_user_data(e);
    if (ui_s == NULL || ui_s->screen == NULL)
    {
        return;
    }
    os_printf("media_player: click next btn\n");
}

/****************************************************************************
 * 进度条点击：只有获取到总时长后才计算并触发跳转时间。
 ****************************************************************************/
static void media_player_on_progress_click(lv_event_t *e)
{
    media_player_ctx_t *ui_s = (media_player_ctx_t *) lv_event_get_user_data(e);
    lv_obj_t           *bar  = lv_event_get_target(e);
    lv_indev_t         *indev;
    lv_point_t          point;
    lv_area_t           area;
    int32_t             width;
    int32_t             x;
    uint32_t            seek_time;

    if (ui_s == NULL || bar == NULL || ui_s->total_time == 0)
    {
        return;
    }

    indev = lv_indev_get_act();
    if (indev == NULL)
    {
        return;
    }

    lv_indev_get_point(indev, &point);
    lv_obj_get_coords(bar, &area);
    width = area.x2 - area.x1 + 1;
    if (width <= 0)
    {
        return;
    }

    x = point.x - area.x1;
    if (x < 0)
    {
        x = 0;
    }
    else if (x > width)
    {
        x = width;
    }

    seek_time = (uint32_t) (((uint64_t) ui_s->total_time * (uint32_t) x) / (uint32_t) width);
    if (seek_time > ui_s->total_time)
    {
        seek_time = ui_s->total_time;
    }

    ui_s->play_time = seek_time;
    txmplayer_seek(ui_s->stream_id, seek_time * 1000);
    os_printf("media_player: seek to %d/%d\n", (int) seek_time, (int) ui_s->total_time);
}

/****************************************************************************
 * 音量滑块值改变回调：拖动滑块时同步 volume / cached_volume 并通知播放器
 ****************************************************************************/
static void media_player_on_volume_changed(lv_event_t *e)
{
    media_player_ctx_t *ui_s   = (media_player_ctx_t *) lv_event_get_user_data(e);
    lv_obj_t           *slider = lv_event_get_target(e);
    if (ui_s == NULL || slider == NULL)
    {
        return;
    }
    ui_s->volume        = (uint8_t) lv_slider_get_value(slider);
    ui_s->cached_volume = ui_s->volume;
    txmplayer_set_volume(ui_s->stream_id, ui_s->cached_volume);
    os_printf("media_player: volume slider changed, volume=%d\n", (int) ui_s->volume);
}

/****************************************************************************
 * 音量按钮点击：切换音量调节滑块的显示/隐藏
 ****************************************************************************/
static void media_player_on_volume_click(lv_event_t *e)
{
    media_player_ctx_t *ui_s = (media_player_ctx_t *) lv_event_get_user_data(e);
    if (ui_s == NULL || ui_s->volume_slider == NULL)
    {
        return;
    }
    /* 切换显示/隐藏 */
    if (lv_obj_has_flag(ui_s->volume_slider, LV_OBJ_FLAG_HIDDEN))
    {
        lv_obj_clear_flag(ui_s->volume_slider, LV_OBJ_FLAG_HIDDEN);
        os_printf("media_player: volume slider show, value=%d\n", (int) lv_slider_get_value(ui_s->volume_slider));
    }
    else
    {
        lv_obj_add_flag(ui_s->volume_slider, LV_OBJ_FLAG_HIDDEN);
        os_printf("media_player: volume slider hide, value=%d\n", (int) lv_slider_get_value(ui_s->volume_slider));
    }

    /* 同步滑块当前值到源变量与缓存，保持三方一致，避免 event_timer 重复刷新 */
    ui_s->volume        = (uint8_t) lv_slider_get_value(ui_s->volume_slider);
    ui_s->cached_volume = ui_s->volume;
}

/****************************************************************************
 * 状态同步定时器回调：在 LVGL 线程上下文中轮询检测 is_playing / volume
 *   是否与缓存值不同，仅在变化时刷新对应 UI 并同步缓存，
 *   无变化则不做任何操作（不空转刷新 UI）。
 *   源变量由媒体事件回调在非 LVGL 上下文 / 按钮回调在 LVGL 上下文写入。
 ****************************************************************************/
static void media_player_event_timer_cb(lv_timer_t *t)
{
    media_player_ctx_t *ui_s = (media_player_ctx_t *) t->user_data;
    uint8_t             time_changed;
    char                time_label[16];

    if (ui_s == NULL)
    {
        return;
    }

    time_changed = 0;

    /* playing 状态变化：先同步缓存，再刷新播放按钮图标，避免重复刷新 */
    if (ui_s->is_playing != ui_s->cached_playing)
    {
        os_printf("is playing:%d\tcached playing:%d\n", ui_s->is_playing, ui_s->cached_playing);
        ui_s->cached_playing = ui_s->is_playing;
        if (ui_s->btn_play != NULL)
        {
            lv_obj_t *label = lv_obj_get_child(ui_s->btn_play, 0);
            if (label != NULL)
            {
                lv_label_set_text(label, ui_s->cached_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
            }
        }
        os_printf("media_player: playing changed, is_playing=%d\n", (int) ui_s->cached_playing);
    }

    /* 音量变化：先同步缓存，再刷新音量滑块，避免重复刷新 */
    if (ui_s->volume != ui_s->cached_volume)
    {
        ui_s->cached_volume = ui_s->volume;
        if (ui_s->volume_slider != NULL)
        {
            lv_slider_set_value(ui_s->volume_slider, ui_s->cached_volume, LV_ANIM_OFF);
        }
        os_printf("media_player: volume changed, volume=%d\n", (int) ui_s->cached_volume);
    }

    // 定时获取播放时间，更新进度条,尽量不要频繁获取
    if ((uint32_t) os_jiffies() - ui_s->last_update_time >= 800)
    {
        ui_s->play_time = txmplayer_playtime(ui_s->stream_id, &ui_s->total_time);
        ui_s->total_time /= 1000;
        ui_s->play_time /= 1000;
        ui_s->last_update_time = os_jiffies();
    }

    /* 播放时间变化：只在秒数变化时刷新当前时间 label */
    if (ui_s->play_time != ui_s->cached_play_time)
    {
        ui_s->cached_play_time = ui_s->play_time;
        time_changed           = 1;
        if (ui_s->play_time_label != NULL)
        {
            media_player_format_time(ui_s->cached_play_time, time_label);
            lv_label_set_text(ui_s->play_time_label, time_label);
        }
    }

    /* 总时长变化：只在秒数变化时刷新总时长 label */
    if (ui_s->total_time != ui_s->cached_total_time)
    {
        ui_s->cached_total_time = ui_s->total_time;
        time_changed            = 1;
        if (ui_s->progress_bar != NULL)
        {
            if (ui_s->cached_total_time > 0)
            {
                lv_obj_add_flag(ui_s->progress_bar, LV_OBJ_FLAG_CLICKABLE);
            }
            else
            {
                lv_obj_clear_flag(ui_s->progress_bar, LV_OBJ_FLAG_CLICKABLE);
            }
        }
        if (ui_s->total_time_label != NULL)
        {
            media_player_format_time(ui_s->cached_total_time, time_label);
            lv_label_set_text(ui_s->total_time_label, time_label);
        }
    }

    if (time_changed && ui_s->progress_bar != NULL && ui_s->cached_total_time > 0)
    {
        uint32_t progress = (ui_s->cached_play_time * 100) / ui_s->cached_total_time;
        if (progress > 100)
        {
            progress = 100;
        }
        lv_bar_set_value(ui_s->progress_bar, (int32_t) progress, LV_ANIM_OFF);
    }
}

/****************************************************************************
 * 媒体事件回调：在非 LVGL 上下文中触发，仅更新源变量，不直接操作 LVGL 对象。
 *   UI 刷新统一由 event_timer 在 LVGL 上下文中 diff 检测完成。
 ****************************************************************************/
sysevt_hdl_res ui_music_screen_media_event(uint32 event_id, uint32 data, uint32 priv)
{
    screen_t           *screen    = (screen_t *) priv;
    media_player_ctx_t *ui_s      = (screen != NULL) ? (media_player_ctx_t *) screen_get_user_data(screen) : NULL;
    uint32_t            stream_id = data & 0xff;
    if (ui_s == NULL)
    {
        os_printf("media_player: media event (event=0x%x, data=%d) without ctx\n", (unsigned) event_id, (int) data);
        return SYSEVT_CONTINUE;
    }

    switch (event_id)
    {
        /* 播放/暂停属同一事件：data 携带当前 playing 状态（0=暂停, 1=播放） */
        case SYS_EVENT(SYS_EVENT_MEDIA, SYSEVT_MEDIA_PLAY_PAUSE):
        {
            if (stream_id == ui_s->stream_id)
            {
                ui_s->is_playing = 0;
            }
        }
        break;
        case SYS_EVENT(SYS_EVENT_MEDIA, SYSEVT_MEDIA_PLAY_START):
        {
            if (stream_id == ui_s->stream_id)
            {
                ui_s->is_playing = 1;
            }
        }
        break;
        case SYS_EVENT(SYS_EVENT_MEDIA, SYSEVT_MEDIA_VOLUME):
        {
            /* 音量事件：data 携带音量值（0-100） */
            ui_s->volume = data >> 8;
            os_printf("media_player: event volume=%d\n", ui_s->volume);
            break;
        }
        case SYS_EVENT(SYS_EVENT_MEDIA, SYSEVT_MEDIA_PLAY_STOP):
        {
            if (stream_id == ui_s->stream_id)
            {
                ui_s->is_playing = 1;
            }
            break;
        }
    }
    return SYSEVT_CONTINUE;
}

/****************************************************************************
 * on_create：首次进入页面时调用
 *   1. 申请 ctx 上下文并挂到 screen->user_data
 *   2. 配置 screen->root 黑色背景
 *   3. 创建 header（永久保留）
 *   4. 创建中部：唱片 + 歌曲信息
 *   5. 创建进度条
 *   6. 创建 4 个控制按钮 + 绑定触摸事件
 ****************************************************************************/
static void media_player_create(screen_t *screen, void *params)
{
    media_player_ctx_t *ui_s;
    lv_obj_t           *page;

    if (screen == NULL)
    {
        return;
    }

    ui_s = (media_player_ctx_t *) SCREEN_MALLOC(sizeof(media_player_ctx_t));
    if (ui_s == NULL)
    {
        return;
    }
    memset(ui_s, 0, sizeof(media_player_ctx_t));
    uint32_t stream_id      = (uint32_t) params;
    ui_s->stream_id         = stream_id;
    ui_s->screen            = screen;
    ui_s->is_playing        = 0;
    ui_s->volume            = 50; /* 与音量滑块初始值保持一致 */
    ui_s->play_time         = 0;
    ui_s->total_time        = 0;
    ui_s->cached_playing    = 0;  /* 与 is_playing 初值一致，避免启动即触发刷新 */
    ui_s->cached_volume     = 50; /* 与 volume 初值一致 */
    ui_s->cached_play_time  = 0;
    ui_s->cached_total_time = 0;
    screen_set_user_data(screen, ui_s);

    /* 配置根容器：全屏黑色背景 */
    lv_obj_add_style(screen->root, &g_style, 0);
    lv_obj_set_size(screen->root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(screen->root, lv_color_hex(MP_BG_DARK), 0);
    lv_obj_set_style_bg_opa(screen->root, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen->root, LV_OBJ_FLAG_SCROLLABLE);

    /* 顶部 header：黄色胶囊状，包含返回按钮 + 标题 */
    media_player_create_header(screen->root, ui_s);

    /* 中部内容容器：让出顶部 70px（header 高度 + 间距） */
    page = lv_obj_create(screen->root);
    if (page == NULL)
    {
        return;
    }
    lv_obj_remove_style_all(page);
    lv_obj_set_size(page, LV_PCT(100), LV_VER_RES - MP_SCALE(70));
    lv_obj_align(page, LV_ALIGN_TOP_MID, 0, MP_SCALE(70));
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(page, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(page, MP_SCALE(20), 0);
    lv_obj_set_style_pad_bottom(page, MP_SCALE(10), 0);
    lv_obj_set_style_pad_row(page, MP_SCALE(24), 0);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    /* 中部行：左侧唱片 + 右侧歌曲信息 */
    {
        lv_obj_t *info_row = lv_obj_create(page);
        if (info_row != NULL)
        {
            lv_obj_remove_style_all(info_row);
            lv_obj_set_size(info_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_flex_flow(info_row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(info_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_column(info_row, MP_SCALE(30), 0);
            lv_obj_clear_flag(info_row, LV_OBJ_FLAG_SCROLLABLE);

            ui_s->disc = media_player_create_disc(info_row);
            (void) media_player_create_song_info(info_row);
        }
    }

    /* 进度条 */
    {
        lv_obj_t *prog_holder = lv_obj_create(page);
        if (prog_holder != NULL)
        {
            lv_obj_remove_style_all(prog_holder);
            lv_obj_set_size(prog_holder, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_set_flex_flow(prog_holder, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(prog_holder, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_clear_flag(prog_holder, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t *prog = media_player_create_progress(prog_holder);
            if (prog != NULL)
            {
                /* prog 内含 3 个子项，最后一个是 total label */
                lv_obj_t *cur          = lv_obj_get_child(prog, 0);
                lv_obj_t *bar          = lv_obj_get_child(prog, 1);
                lv_obj_t *tot          = lv_obj_get_child(prog, 2);
                ui_s->play_time_label  = cur;
                ui_s->progress_bar     = bar;
                ui_s->total_time_label = tot;
                if (ui_s->progress_bar != NULL)
                {
                    lv_obj_add_event_cb(ui_s->progress_bar, media_player_on_progress_click, LV_EVENT_CLICKED, ui_s);
                }
            }
        }
    }

    /* 4 个控制按钮行：上一首 / 播放 / 下一首 / 音量 */
    {
        lv_obj_t *btn_row = lv_obj_create(page);
        if (btn_row != NULL)
        {
            lv_obj_remove_style_all(btn_row);
            lv_obj_set_size(btn_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_column(btn_row, MP_SCALE(30), 0);
            lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);

            ui_s->btn_prev   = media_player_create_control_btn(btn_row, LV_SYMBOL_PREV);
            ui_s->btn_play   = media_player_create_control_btn(btn_row, LV_SYMBOL_PLAY);
            ui_s->btn_next   = media_player_create_control_btn(btn_row, LV_SYMBOL_NEXT);
            ui_s->btn_volume = media_player_create_control_btn(btn_row, LV_SYMBOL_VOLUME_MAX);

            if (ui_s->btn_prev != NULL)
            {
                lv_obj_add_event_cb(ui_s->btn_prev, media_player_on_prev_click, LV_EVENT_CLICKED, ui_s);
            }
            if (ui_s->btn_play != NULL)
            {
                lv_obj_add_event_cb(ui_s->btn_play, media_player_on_play_click, LV_EVENT_CLICKED, ui_s);
            }
            if (ui_s->btn_next != NULL)
            {
                lv_obj_add_event_cb(ui_s->btn_next, media_player_on_next_click, LV_EVENT_CLICKED, ui_s);
            }
            if (ui_s->btn_volume != NULL)
            {
                lv_obj_add_event_cb(ui_s->btn_volume, media_player_on_volume_click, LV_EVENT_CLICKED, ui_s);
            }
        }
    }

    /* 音量调节滑块：默认隐藏，点击音量按钮后显示/隐藏 */
    ui_s->volume_slider = media_player_create_volume_slider(screen->root);
    if (ui_s->volume_slider != NULL)
    {
        lv_obj_add_event_cb(ui_s->volume_slider, media_player_on_volume_changed, LV_EVENT_VALUE_CHANGED, ui_s);
    }

    /* 媒体事件刷新定时器：周期检测事件标志并在 LVGL 上下文中刷新 UI */
    ui_s->event_timer = lv_timer_create(media_player_event_timer_cb, 50, ui_s);

    sys_event_take(SYS_EVENT(SYS_EVENT_MEDIA, 0), ui_music_screen_media_event, (uint32_t) screen);
}

/****************************************************************************
 * on_start：页面即将可见时调用
 *   - 本页无硬件资源，空实现
 ****************************************************************************/
static void media_player_start(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_resume：页面成为栈顶可交互时调用
 *   - 本页不支持物理按键，空实现
 ****************************************************************************/
static void media_player_resume(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_pause：页面失去焦点时调用
 *   - 本页无硬件资源/按键钩子，空实现
 ****************************************************************************/
static void media_player_pause(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_stop：页面不可见 / 退出前台时调用
 *   - 本页无硬件资源，空实现
 ****************************************************************************/
static void media_player_stop(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_destroy：页面对象销毁时调用
 *   - 释放 ui_s
 *   - 不释放 LVGL 对象，由 screen_manager 统一销毁
 ****************************************************************************/
static void media_player_destroy(screen_t *screen)
{
    media_player_ctx_t *ui_s;

    if (screen == NULL)
    {
        return;
    }
    ui_s = (media_player_ctx_t *) screen_get_user_data(screen);
    if (ui_s == NULL)
    {
        return;
    }

    /* 销毁事件刷新定时器（LVGL 对象，需在 ctx 释放前删除） */
    if (ui_s->event_timer != NULL)
    {
        lv_timer_del(ui_s->event_timer);
        ui_s->event_timer = NULL;
    }
    txmplayer_close(ui_s->stream_id);
    sys_event_untake(SYS_EVENT(SYS_EVENT_MEDIA, 0), ui_music_screen_media_event);
    /* 清空 ctx 内部指针（不释放 LVGL 对象，由 screen_manager 统一销毁） */
    ui_s->screen            = NULL;
    ui_s->header_panel      = NULL;
    ui_s->back_btn          = NULL;
    ui_s->disc              = NULL;
    ui_s->progress_bar      = NULL;
    ui_s->play_time_label   = NULL;
    ui_s->total_time_label  = NULL;
    ui_s->btn_prev          = NULL;
    ui_s->btn_play          = NULL;
    ui_s->btn_next          = NULL;
    ui_s->btn_volume        = NULL;
    ui_s->volume_slider     = NULL;
    ui_s->cached_playing    = 0;
    ui_s->cached_volume     = 0;
    ui_s->cached_play_time  = 0;
    ui_s->cached_total_time = 0;

    SCREEN_FREE(ui_s);

    screen_set_user_data(screen, NULL);
}

/****************************************************************************
 * on_restart：STOPPED → 重新前台前调用
 ****************************************************************************/
static void media_player_restart(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * on_finish：screen_finish() 调用时触发
 *   - 本页无业务结果回传，空实现
 ****************************************************************************/
static void media_player_finish(screen_t *screen)
{
    (void) screen;
}

/****************************************************************************
 * 组装生命周期回调集合
 ****************************************************************************/
static void media_player_get_lifecycle(screen_lifecycle_t *lifecycle)
{
    if (lifecycle == NULL)
    {
        return;
    }

    *lifecycle = (screen_lifecycle_t) {
            .on_create  = media_player_create,
            .on_start   = media_player_start,
            .on_resume  = media_player_resume,
            .on_pause   = media_player_pause,
            .on_stop    = media_player_stop,
            .on_destroy = media_player_destroy,
            .on_restart = media_player_restart,
            .on_finish  = media_player_finish,
            .screen_id  = 0,
            .only       = 0,
    };
}

/****************************************************************************
 * 菜单入口点击回调：注册并压栈一个新 Screen
 ****************************************************************************/
static void enter_media_player(lv_event_t *e)
{
    screen_lifecycle_t lifecycle;
    screen_t          *screen;

    (void) e;
    os_printf("media_player: enter media_player screen\n");
    media_player_get_lifecycle(&lifecycle);
    screen = screen_register("media_player", &lifecycle);
    if (screen != NULL)
    {
        screen_push(screen, NULL);
    }
}

/****************************************************************************
 * 外部入口：在主菜单列表项上调用，绑定入口图标和触摸回调
 *   parent - 菜单列表的某个 item
 *   path   - 入口图标图片路径（由 ui_image_show 显示）
 * 用法示例（在 main_ui.c 等列表中）：
 *   media_player_ui_create(list_item, icon_path);
 ****************************************************************************/
void media_player_ui_create(lv_obj_t *parent, const char *path)
{
    if (parent == NULL)
    {
        return;
    }

    ui_image_show(parent, path);
    lv_obj_add_flag(parent, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(parent, enter_media_player, LV_EVENT_CLICKED, NULL);
}

uint32_t extern_media_player_ui_create(struct screen_common_s *m)
{
    screen_lifecycle_t     lifecycle;
    screen_t              *screen;
    uint32_t               stream_id      = 0;
    uint32_t               real_screen_id = 0;
    struct screen_music_s *music_msg      = (struct screen_music_s *) m;
    if (music_msg && music_msg->magic == SCREEN_MAGIC && music_msg->type == SCREEN_MUSIC_TYPE)
    {
        stream_id = music_msg->stream_id;
    }
    if (!stream_id)
    {
        os_printf("music player id err,stream_id is %d\n", stream_id);
        return 0;
    }
    media_player_get_lifecycle(&lifecycle);
    lifecycle.screen_id = music_msg->screen_id;
    lifecycle.only      = music_msg->ui_id;
    screen              = screen_register("voice_media_player", &lifecycle);

    if (screen != NULL)
    {
        screen->cb      = m->cb;
        screen->cb_priv = m->cb_priv;
        screen_push(screen, (void *) stream_id);
        real_screen_id = screen->id;
    }
    return real_screen_id;
}
