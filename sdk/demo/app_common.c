#include "basic_include.h"
#include "sys_config.h"
#include "demo/app_device.h"
#include "vfs_fatfs.h"
#include "app_lcd/app_lcd.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "cjson/cJSON.h"
#include "hg_lv_mem.h"
#include "update/ota.h"
#include "stream_define.h"
#include "app/interface_management/interface_mgnt_msi.h"
#include "lib/video/vpp/vpp_dev.h"
#include "video_app/video_msi.h"
#include "app_common.h"
#include "audio_msi/audio_adc.h"
#include "heap/aurpc_heap.h"
#include "multimedia/audio/audio_coder.h"
extern void fatfs_sd0_init(void);

/**********************************************************************************
 * @brief 初始化sd卡,注意如果即将ota,则返回失败(应用要处理是否ota,需要决定应用其他
          初始化是否要继续),其他情况都是返回RET_OK
 * @param start_ota 是否启动ota
 * @param ota_path ota路径
 * @return int32_t 0 成功 -1 失败
 ********************************************************************************/
int32_t app_sd_init(uint8_t start_ota, const char *ota_path)
{
    int32_t ret = RET_OK;
#if FS_EN
    // 硬件初始化
    // sd卡初始化
    vfs_fatfs_register();
    fatfs_sd0_init();
    if (start_ota)
    {
        if (!file_ota(ota_path))
        {
            ret = RET_ERR;
        }
    }
#endif
    return ret;
}

int32_t app_lcd_init(void *lcd_cfg)
{
    lcd_hardware_init(lcd_cfg);
    lcd_driver_init(R_OSD_ENCODE, R_LCD_OSD, R_VIDEO_P0, R_VIDEO_P1);
    return RET_OK;
}

int32_t app_lvgl_init(void *main_ui_fn,uint32_t indev_mask)
{
    uint16_t osd_w, osd_h;
    uint8_t osd_rotate;
#if LVGL_HW_ROTATE_RPC_EN
    struct hg_lv_mem_hooks hook = {
            .malloc  = _os_malloc,
            .realloc = _os_realloc,
            .zalloc  = _os_zalloc,
            .free    = _os_free,
    };
#else
    struct hg_lv_mem_hooks hook = {
            .malloc  = av_psram_malloc,
            .realloc = av_psram_realloc,
            .zalloc  = av_psram_zalloc,
            .free    = av_psram_free,
    };
#endif
    hg_lv_mem_register(&hook);
    get_osd_w_h(&osd_w, &osd_h, &osd_rotate);
    lvgl_init(osd_w, osd_h, osd_rotate);
    os_printf("osd_w:%d osd_h:%d osd_rotate:%d\tindev_mask:%X\n", osd_w, osd_h, osd_rotate,indev_mask);
    if(indev_mask & LVGL_INDEY_KEY)
    {
        lvgl_key_init(NULL);
    }
    if(indev_mask & LVGL_INDEY_TOUCHPAD)
    {
        struct dev_obj *dev = dev_get(HG_TOUCHPAD_DEVID);
        if (dev != NULL)
        {
            lvgl_touchpad_init((void *)dev);
        }
    }
    lvgl_run(main_ui_fn);
	return RET_OK;
}

/***********************************************
 * @brief 初始化h264
 * @param sub_en 是否启动副码流
 * @return int32_t 0 成功 -1 失败
 ********************************************* */
int32_t app_h264_init(int8_t sub_en)
{
    // 启动h264
    uint16_t h264_w = 0, h264_h = 0;
    uint8_t  h264_ret       = 1;
    // 判断副码流宏是否被打开
    h264_ret                = sub_en ? get_vpp1_w_h(&h264_w, &h264_h) : 1;
    // 是否需要启动副码流
    uint8_t sub_stream_from = h264_ret ? ~0 : GEN420_DATA;
    auto_h264_msi_init(AUTO_H264, VPP_DATA0, 0, 0, sub_stream_from, h264_w, h264_h);
    return RET_OK;
}

/********************************************
 * @brief 初始化cjson内存分配
 ********************************************/
int32_t app_cjson_init()
{
#ifdef PSRAM_HEAP
    cJSON_Hooks hook;
    hook.malloc_fn = _os_malloc_psram;
    hook.free_fn   = _os_free_psram;
    cJSON_InitHooks(&hook);
#endif
    return RET_OK;
}

/************************************************************************
 * @brief 初始化音频模块
 * @param ad_sample_rate 采样率,0不使能
 * @param dac_sample_rate 采样率,0不使能
 * @return int32_t 0 成功 -1 失败
 ************************************************************************/
int32_t app_audio_init(uint32_t ad_sample_rate, uint32_t dac_sample_rate)
{
    int32 ret = RET_ERR;
    void *aurpc_psram_buf = os_malloc_psram(AURPC_PSRAM_HEAP_SIZE);
    if (aurpc_psram_buf)
    {
        uint32 flags = SYSHEAP_FLAGS_MEM_ALIGN_32;
        aurpc_psram_heap_init(aurpc_psram_buf, AURPC_PSRAM_HEAP_SIZE, flags);
    }
    if(ad_sample_rate != 0)
    {
        ret = audio_adc_init(AUSYS_AUAD, ad_sample_rate, 1, 4);
        if(ret == RET_OK) {
            struct dev_hotplug_info *auadc_dev_info = (struct dev_hotplug_info*)os_malloc_psram(sizeof(struct dev_hotplug_info));
            auadc_dev_info->release = audio_adc_dev_out;
            auadc_dev_info->priv = (txAudioInfo_t*)os_malloc_psram(sizeof(txAudioInfo_t));
            txAudioInfo_t *auadc_info = (txAudioInfo_t*)auadc_dev_info->priv;
            auadc_info->channels = 1;
            auadc_info->sample_rate = ad_sample_rate;
            dev_hotplug_in(HG_MIC0_DEVID, DEV_TYPE_MIC, auadc_dev_info);
        }
    }
    
    if(dac_sample_rate != 0)
    {
        ret = audio_mixer_init(dac_sample_rate, 20, 16, 1);
        if(ret == RET_OK) {
            dev_hotplug_in(HG_SPK0_DEVID, DEV_TYPE_DAC, 0);
        }
    }
    return 0;
}

/**
 * @brief 初始化USB模块(USB11、USB20)
 * 
 * @param usb_mode 初始化USB的模式(USB11:Host\Device、USB20:Host\Device\OTG)
 * @return int32_t 0 成功 -1 失败
 */
int32_t app_usb_init(uint8_t usb_mode)
{
    static uint8_t app_usb_register_flag = 0;
    static uint8_t app_usb_device_init   = 0;
    int32_t ret = RET_OK;

    const uint8_t valid_mask = USB11_DEVICE_MODE | USB11_HOST_MODE |
                               USB20_DEVICE_MODE | USB20_HOST_MODE | USB20_OTG_MODE;

    uint8_t usb11_mode = usb_mode & (USB11_DEVICE_MODE | USB11_HOST_MODE);
    uint8_t usb20_mode = usb_mode & (USB20_DEVICE_MODE | USB20_HOST_MODE | USB20_OTG_MODE);

    if(usb_mode == 0 || (usb_mode & ~valid_mask)) {
        return RET_ERR;
    }

    /* 一个控制器的掩码中只能设置一个 bit */
    if((usb11_mode & (usb11_mode - 1)) != 0) {
        return RET_ERR;
    }

    if((usb20_mode & (usb20_mode - 1)) != 0) {
        return RET_ERR;
    }

    usb_mode &= ~app_usb_register_flag;

    if(usb_mode & USB11_DEVICE_MODE) {
        extern void hg_usb11d_class_driver_register(void);
        extern int hg_usb11d_register(rt_uint32_t devid);

        if (!app_usb_device_init) {
            extern rt_err_t rt_usbd_core_device_list_init();
            rt_usbd_core_device_list_init();
            app_usb_device_init = 1;
        }

        hg_usb11d_class_driver_register();
        ret = hg_usb11d_register(HG_USB11_DEV_CONTROLLER_DEVID);
        if(ret != RET_OK) {
            return ret;
        }
        app_usb_register_flag |= USB11_DEVICE_MODE;
    }

    if(usb_mode & USB11_HOST_MODE) {
        extern rt_err_t hg_usb11h_register(rt_uint32_t devid);

        ret = hg_usb11h_register(HG_USB11_HOST_CONTROLLER_DEVID);
        if(ret != RET_OK) {
            return ret;
        }
        app_usb_register_flag |= USB11_HOST_MODE;
    }

    if(usb_mode & USB20_DEVICE_MODE) {
        extern void hg_usbd_class_driver_register(void);
        extern int hg_usbd_register(rt_uint32_t devid);

        if (!app_usb_device_init) {
            extern rt_err_t rt_usbd_core_device_list_init();
            rt_usbd_core_device_list_init();
            app_usb_device_init = 1;
        }

        hg_usbd_class_driver_register();
        ret = hg_usbd_register(HG_USB_DEV_CONTROLLER_DEVID);
        if(ret != RET_OK) {
            return ret;
        }
        app_usb_register_flag |= USB20_DEVICE_MODE;
    }

    if(usb_mode & USB20_HOST_MODE) {
        extern rt_err_t hg_usbh_register(rt_uint32_t devid);

        ret = hg_usbh_register(HG_USB_HOST_CONTROLLER_DEVID);
        if(ret != RET_OK) {
            return ret;
        }
        app_usb_register_flag |= USB20_HOST_MODE;
    }

    if(usb_mode & USB20_OTG_MODE) {
        extern void hg_usb_connect_detect_init(void);

        if (!app_usb_device_init) {
            extern rt_err_t rt_usbd_core_device_list_init();
            rt_usbd_core_device_list_init();
            app_usb_device_init = 1;
        }

        hg_usb_connect_detect_init();
        app_usb_register_flag |= USB20_OTG_MODE;
    }

    return RET_OK;
}

/**
 * @brief 初始化某些电源域
 * 
 */
void app_power_init(void)
{
    // 电源启动
    pmu_vccsd_power_set(1, VCCSD_33);
    pmu_vcam_ldo_en(VCAM_EN, VCAM_VOL);
    pmu_vcam2_ldo_en(VCAM2_EN, VCAM2_VOL);
}