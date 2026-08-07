#include "sys_config.h"
#include "basic_include.h"
#include "lib/common/atcmd.h"
#include "lib/net/eloop/eloop.h"
#include "lib/video/isp/isp_dev.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "lib/video/mipi_csi/mipi_csi.h"
#include "lib/video/h264/h264_drv.h"
#include "lib/video/vpp/vpp_dev.h"
#include "lib/video/para_in/para_in_dev.h"
#include "lib/multimedia/msi.h"
#include "stream_define.h"
#include "syscfg.h"

#include "app_lcd/app_lcd.h"
#include "lib/net/dhcpd/dhcpd.h"

#include "audio_msi/audio_adc.h"
#include "audio_msi/audio_dac.h"
#include "intercom/intercom.h"
#include "lib/audio/audio_code/audio_code.h"
#include "lib/audio/audio_proc/audio_proc.h"
#include "lib/audio/wsola/wsola_process.h"

#if RTT_USB_EN
#include "rtthread.h"
#endif
#include "lib/heap/av_psram_heap.h"
#include "lib/heap/av_heap.h"
#include "app/spook/spook.h"
#include "app/recorder/recorder_viidure.h"
#include "app/app_iic/app_iic.h"
#include "ota.h"

#include "cjson/cJSON.h"
#include "scale_msi/scale_msi.h"
#include "lib/touch/touch_pad.h"
#include "gen420_hardware_msi.h"
#include "lib/sdhost/sdhost.h"
#include "hg_lv_mem.h"
#include "keyWork.h"
#include "video_app/video_msi.h"

#include "lib/lvgl_rotate_rpc/lvgl_rotate_msi.h"
#include "debug_log_msi.h"
#include "log_save_msi.h"
#include "fpv_mem.h"
#include "hal/i2s.h"
#include "scale/scale_common.h"
#include "takephoto_module/takephoto.h"
#include "scale_msi/scale3_normal_msi.h"
#include "mp4_encode_msi2.h"

void user_workqueue_init(uint16 pri, void *stack, uint16 stack_size);

static void app_user_protocol()
{
    spook_init();
    config_Viidure(80);
}

// 应用程序初始化
__init static void app_init(void)
{
#if TAKEPHOTO_EN || JPG_EN
    int8_t takephoto_from = 0;
#endif
#ifdef PSRAM_HEAP
    cJSON_Hooks hook;
    hook.malloc_fn = _os_malloc_psram;
    hook.free_fn   = _os_free_psram;
    cJSON_InitHooks(&hook);
#endif

    eloop_init();
    os_task_create("eloop_run", user_eloop_run, NULL, OS_TASK_PRIORITY_NORMAL + 2, 0, NULL, 2048);
    // 独立的文件保存msi(独立线程,后续可以所有的fb需要保存都发到这个msi去执行)
    extern struct msi *file_msi_init(const char *msi_name);
    file_msi_init(R_FILE_MSI);
#if H264_EN == 1
    extern struct msi *auto_h264_msi_init(const char *auto_h264_name, uint8_t src_from0, uint16_t w0, uint16_t h0, uint8_t src_from1, uint16_t w1, uint16_t h1);
#if SUB_STREAM_EN == 1
    {
        uint16_t h264_w, h264_h;
        uint8_t  h264_ret = get_vpp1_w_h(&h264_w, &h264_h);
        if (!h264_ret)
        {
            auto_h264_msi_init(AUTO_H264, VPP_DATA0, 0, 0, GEN420_DATA, h264_w, h264_h);
        }
        else
        {
            auto_h264_msi_init(AUTO_H264, VPP_DATA0, 0, 0, ~0, 0, 0);
        }
    }
#else
    auto_h264_msi_init(AUTO_H264, VPP_DATA0, 0, 0, ~0, 0, 0);
#endif
#endif

#ifndef LCD_EN
    //启动无屏的scale3
    scale3_msi_no_lcd(S_PREVIEW_SCALE3,1, FSTYPE_YUV_P0, 320, 180);
#endif

// 支持拍照和缩略图,暂时默认启动,(紧紧支持录风者模式)
#if TAKEPHOTO_EN
    // 正常拍照和缩略图
    {
        extern uint8_t get_vpp_w_h(uint16_t *w, uint16_t *h);
        extern void    takephoto_with_thumb_init(const char *thumb_msi_name);
        uint16_t       camera_w, camera_h;
        get_vpp_w_h(&camera_w, &camera_h);
        // 没有识别到摄像头
        if (!camera_w || !camera_h)
        {
        }
        // 如果是720P摄像头,支持原分辨率以及大分辨拍照
        else if (camera_w == 1280)
        {
            common_takephoto_normal_init(R_THUMB);
            takephoto_from = VPP_DATA0;
        }
    }
#endif

    //MP4的缩略图初始化
    mp4_thumb_init();

#if JPG_EN == 1
    if (takephoto_from >= 0)
    {
        auto_jpg_msi_init(AUTO_JPG, JPGID0, takephoto_from);
    }
#endif

    app_user_protocol();
}

static uint8_t app_vcam_en(void)
{
#if VCAM_EN

#ifdef VCAM_33
    pmu_vcam_ldo_en(1, VCAM_VOL_3V30);
#else
    pmu_vcam_ldo_en(1, VCAM_VOL_2V80);
#endif

#endif
    return TRUE;
}

extern void scale2_mutex_init();

static void app_hardware_init(uint8_t vcam)
{

    void eff_stop();
    eff_stop();
    iic_thread_init();
    sensor_info_init();
    scale2_mutex_init();
    scale_mutex_init(); // scale相关锁初始化

#if JPG_EN == 1
    extern int32 jpg_mutex_init();
    extern void  jpg_mem_init(int num);
    jpg_mutex_init();
    jpg_mem_init(32);
#endif
    // 默认打开gen420的模块
    gen420_hardware_msi_init();

#if SDH_EN && FS_EN
    extern bool fatfs_register();
    sd_open();
    fatfs_register();
    file_ota();
#endif

#if DEBUG_LOG_FILE_SAVE_EN
    // 默认写入到SD卡，不支持中断打印保存
    // 默认每10秒同步写卡，每60秒新建一个文件保存
    creat_log_save_stream(R_DEBUG_STREAM, 1000, 60 * 1000);
    struct msi *dbg_msi = dbg_log_msi(S_DEBUG_STREAM);
    if (dbg_msi)
    {
        print_redirect((osprint_hook) hgprntf_debug_log_msi, dbg_msi->priv, 1);
    }

#endif

#if MIPI_CSI_EN
    struct mipi_csi_debug mipi_debug;
    os_memset(&mipi_debug, 0, sizeof(struct mipi_csi_debug));
    mipi_debug.debug_enable = 0;
    mipi_debug.debug_io0    = PD_4;
    mipi_debug.debug_io1    = PD_5;
    mipi_debug.debug_io2    = PD_6;
    mipi_debug.debug_io3    = PD_7;
    mipi_debug.debug_io4    = 0xff;
    mipi_debug.debug_io5    = 0xff;
    mipi_debug.debug_type0  = 6;
    mipi_debug.debug_type1  = 7;
    mipi_debug.debug_type2  = 8;
    mipi_debug.debug_type3  = 9;

    mipi_csi_hardware_config(HG_MIPI_CSI_DEVID, 1, CAM_DUAL_SPLICE_SLAVE_MODE, 1, SENSOR_TYPE_MASTER, 24, &mipi_debug);
    mipi_csi_hardware_config(HG_MIPI1_CSI_DEVID, 1, CAM_DUAL_SPLICE_SLAVE_MODE, 1, SENSOR_TYPE_SLAVE0, 24, &mipi_debug);

#endif

#if ISP_EN
    isp_cfg_dev();
    ircut_init();
#endif
#if VPP_EN
    {
        extern uint8_t set_vpp_bu1_shrink(uint16_t w, uint16_t shrink_w);
        extern void    get_single_mipi(uint32_t csi_dev_id, uint16_t *w, uint16_t *h);
        uint16_t       w = 0, h = 0;
        get_single_mipi(HG_MIPI_CSI_DEVID, &w, &h);
        os_printf(KERN_INFO "vpp_cfg w:%d h:%d\n", w, h);
#ifdef SUB_STREAM_WIDTH
        set_vpp_bu1_shrink(w, SUB_STREAM_WIDTH);
#endif
        vpp_cfg(w, h, VPP_INPUT_FROM);
    }

#endif

#if AUDIO_EN
    reg_auproc_alloc(av_psram_malloc, av_psram_zalloc, av_psram_calloc, av_psram_realloc, av_psram_free);
    reg_wsola_alloc(av_psram_malloc, av_psram_zalloc, av_psram_calloc, av_psram_realloc, av_psram_free);
    reg_aucoder_alloc(av_psram_malloc, av_psram_zalloc, av_psram_calloc, av_psram_realloc, av_psram_free);
    aucode_mutex_init();
    audio_adc_init(AUSYS_AUAD, 8000, 1, 4, 0);
    audio_dac_init();
#endif
}

static struct os_work app_wk;
static int32          app_loop(struct os_work *work)
{
    os_run_work_delay(work, 1000);
    return 0;
}

/**********************************************************************
 * print_level设置打印的等级,7是将所有打印都打开(调试的时候可以打开)
 * 例子:对应不同等级参考string.h
 *      os_printf(KERN_DEBUG"ABC"); //等级7
 *      os_printf(KERN_EMERG"ABC"); //等级1
 * disable_print_color 是否关闭打印颜色(特定串口工具)
 *******************************************************************/
int sys_app_double_sensor_splice_init(void)
{
    print_level(7);
    disable_print_color(1);
    uint8_t vcam;
    user_heap_init();
    vcam = app_vcam_en();
    pmu_vcam2_ldo_en(1, VCC_LDO_VOL_1V80);
    msi_core_init();
    // 初始化fpv应用用的workqueue,注意这个workqueu是应用,尽量不要执行过长时间
    user_workqueue_init(OS_TASK_PRIORITY_HIGH, NULL, 2048);
    app_hardware_init(vcam);
    app_init();
    OS_WORK_INIT(&app_wk, app_loop, 0);
    os_run_work_delay(&app_wk, 1000);
    return 0;
}
