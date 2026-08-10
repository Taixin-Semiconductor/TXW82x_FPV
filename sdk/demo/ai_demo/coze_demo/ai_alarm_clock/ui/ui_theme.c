
#include "osal/string.h"
#include "lvgl/lvgl.h"
#include "ai_common.h"
struct ui_theme_s
{
    lv_font_t *default_font;
    lv_font_t *default_font48;
};

static struct ui_theme_s *g_ui_theme;

void ui_theme_init()
{
    g_ui_theme = os_zalloc(sizeof(struct ui_theme_s));
#if LV_USE_FREETYPE
    g_ui_theme->default_font = lv_freetype_font_create(AI_RES_FONT"myfont.ttf", 16, LV_FREETYPE_FONT_STYLE_NORMAL);
    g_ui_theme->default_font48 = lv_freetype_font_create(AI_RES_FONT"myfont.ttf", 96, LV_FREETYPE_FONT_STYLE_NORMAL);
#else
    g_ui_theme->default_font = lv_font_default();
#endif
}

lv_font_t *get_theme_default_font()
{
    return g_ui_theme->default_font;
}

lv_font_t *get_theme_default_font48()
{
    return g_ui_theme->default_font48;
}