#ifndef __VIDEO_APP_CSC_MSI_H_
#define __VIDEO_APP_CSC_MSI_H_
#include "hal/csc.h"
#include "dev/csc/hgcsc.h"

#define LV_HW_CSC_2_VIDEO_DISABLE  0
#define LV_REFRESH_LINE_BUFFER     1             // 行缓存刷新模式
#define LV_REFRESH_FULL_SCREEN     2             // 整帧直接渲染模式


#define LVGL_HW_CSC_2_LCD_VIDEO         LV_HW_CSC_2_VIDEO_DISABLE

void video_app_csc_msi_init(const char* csc_msi_name, uint32_t input_format, uint32_t output_format, uint32_t width, uint32_t height);

#endif