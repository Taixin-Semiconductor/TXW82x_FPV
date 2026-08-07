#include "lvgl/lvgl.h"
#include "lvgl_ui.h"

extern lv_style_t g_style;

lv_obj_t *main_LLM_vision_ui(lv_obj_t *base_ui, lv_group_t *group)
{
    lv_obj_t *ui = lv_obj_create(lv_scr_act());  
    lv_obj_add_style(ui, &g_style, 0);
    lv_obj_set_size(ui, LV_PCT(100), LV_PCT(100));

    //static const uint16_t filter[] = { FSTYPE_JPG_FILE, FSTYPE_JPG_CAMERA0, FSTYPE_NONE };

    struct msi *decode_s = jpg_decode_msi(S_JPG_DECODE);
    //将解码的数据推送到Video P0和Video P1显示
    if (decode_s)
    {
        msi_add_output(decode_s, NULL, R_VIDEO_P0);
        msi_cmd(R_VIDEO_P0, MSI_CMD_LCD_VIDEO, MSI_VIDEO_ENABLE, 1);
    }
    else
    {
        os_printf("jpg decode new msi failed\n");
    }

    struct msi *jpg_decode_msg_s = jpg_decode_msg_msi(S_NET_JPEG, 320, 320, 320, 320, FSTYPE_JPG_FILE);
    //将other_jpg的数据给到S_JPG_DECODE进行编码
    if (jpg_decode_msg_s)
    {
        // 配置解码的坐标值
        msi_do_cmd(jpg_decode_msg_s, MSI_CMD_DECODE_JPEG_MSG, MSI_JPEG_DECODE_FORCE_TYPE, FSTYPE_YUV_P0);
        msi_do_cmd(jpg_decode_msg_s, MSI_CMD_DECODE_JPEG_MSG, MSI_JPEG_DECODE_X_Y, 0 << 16 | 0);
        msi_add_output(jpg_decode_msg_s, NULL, S_JPG_DECODE);
    }
    else
    {
        os_printf("jpg decode msg 0 new msi failed\n");
    }

    return base_ui;
}
