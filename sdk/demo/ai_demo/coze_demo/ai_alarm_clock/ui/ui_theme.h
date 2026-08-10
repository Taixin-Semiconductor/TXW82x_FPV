#ifndef UI_UI_THEME_H
#define UI_UI_THEME_H

#include "lvgl/lvgl.h"

lv_font_t *get_theme_default_font();
lv_font_t *get_theme_default_font48();
void ui_theme_init();
/* ── Primary Colors ── */
#define UI_COLOR_PRIMARY        0xffd54a
#define UI_COLOR_PRIMARY_DARK   0x2a2113

/* ── Background Colors ── */
#define UI_COLOR_BG_DARK        0x111217
#define UI_COLOR_BG_SCREEN      0x1a1c24
#define UI_COLOR_BG_DETAIL      0x171b24
#define UI_COLOR_BG_CARD        0xffffff
#define UI_COLOR_BG_LIGHT       0xf7f8fc

/* ── Text Colors ── */
#define UI_COLOR_TEXT_LIGHT     0xf1f3ff
#define UI_COLOR_TEXT_DARK      0x1f2736
#define UI_COLOR_TEXT_MUTED     0x70809b
#define UI_COLOR_TEXT_SECONDARY 0x536076

/* ── Accent Colors ── */
#define UI_COLOR_BLUE           0x206FE5
#define UI_COLOR_BLUE_DEEP      0x2f64f2
#define UI_COLOR_GREEN          0x8bd450
#define UI_COLOR_RED            0xff7b72
#define UI_COLOR_ACCENT_ICON    0x6a78ff

/* ── Border / Divider ── */
#define UI_COLOR_BORDER         0xd7dfea

/* ── Common Dimensions ── */
#define UI_HEADER_HEIGHT        35
#define UI_HEADER_RADIUS        16
#define UI_CORNER_RADIUS        14
#define UI_CARD_RADIUS          18
#define UI_BTN_RADIUS           23

/* ── Common Fonts ── */
#define UI_FONT_BODY  (get_theme_default_font())

#endif