#include "lvgl/lvgl.h"
#include "lvgl_ui.h"

extern lv_style_t g_style;

void ai_dialogue_ui(void)
{
    lv_style_reset(&g_style);
    lv_style_init(&g_style);
    lv_style_set_bg_color(&g_style, lv_color_make(0x00, 0x00, 0x00));
    lv_style_set_shadow_color(&g_style, lv_color_make(0x00, 0x00, 0x00));
    lv_style_set_border_color(&g_style, lv_color_make(0x00, 0x00, 0x00));
    lv_style_set_outline_color(&g_style, lv_color_make(0x00, 0x00, 0x00));
    lv_style_set_radius(&g_style, 0);
    
    lv_obj_t *ui = lv_obj_create(lv_scr_act());  
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100)); 
}