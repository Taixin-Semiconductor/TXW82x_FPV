#ifndef __APP_COMMON_H
#define __APP_COMMON_H

#define LVGL_INDEY_KEY      (1 << 0)
#define LVGL_INDEY_TOUCHPAD (1 << 1)

// clang-format off
#define APP_RETUNR_RET(RET) {if(RET) return RET;}
#define RETUNR(RET)         {if(RET) return ;}
// clang-format on

enum {
    USB11_DEVICE_MODE = BIT(0),
    USB11_HOST_MODE   = BIT(1),
    USB20_DEVICE_MODE = BIT(2),
    USB20_HOST_MODE   = BIT(3),
    USB20_OTG_MODE    = BIT(4),
};

int32_t app_sd_init(uint8_t startup_ota, const char *ota_path);
int32_t app_lcd_init(void *lcd_cfg);
int32_t app_lvgl_init(void *main_ui_fn, uint32_t indev_mask);
int32_t app_h264_init(int8_t sub_en);
int32_t app_cjson_init();
int32_t app_audio_init(uint32_t ad_sample_rate,uint32_t dac_sample_rate);
int32_t app_usb_init(uint8_t usb_mode);
void app_power_init(void);
#endif