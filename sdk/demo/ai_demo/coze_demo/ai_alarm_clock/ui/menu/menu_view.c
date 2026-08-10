#include "menu_view.h"

#include <stdbool.h>

#include "../ui_theme.h"

/* 判断菜单图片是否存在 */
static bool ui_menu_view_image_exists(const char *image_path)
{
    lv_fs_file_t file;

    if (image_path == NULL || image_path[0] == '\0')
    {
        return false;
    }

    if (lv_fs_open(&file, image_path, LV_FS_MODE_RD) != LV_FS_RES_OK)
    {
        return false;
    }

    lv_fs_close(&file);
    return true;
}

// 在页面显示一张图片,居中,没有图片使用默认图片
void ui_image_show(lv_obj_t *parent, const char *image_path)
{
    lv_obj_t *image_obj;
    lv_obj_set_style_bg_color(parent, lv_color_hex(UI_COLOR_BG_SCREEN), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    if (ui_menu_view_image_exists(image_path))
    {
        image_obj = lv_img_create(parent);
        lv_img_set_src(image_obj, image_path);
    }
    else
    {
        image_obj = lv_label_create(parent);
        lv_label_set_text(image_obj, LV_SYMBOL_IMAGE);
        lv_obj_set_style_text_font(image_obj, UI_FONT_BODY, 0);
        lv_obj_set_style_text_color(image_obj, lv_color_hex(UI_COLOR_TEXT_MUTED), 0);
    }

    lv_obj_center(image_obj);
}
