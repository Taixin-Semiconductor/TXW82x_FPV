#include "demo/app_device.h"
#include "demo/app_common.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "lib/video/mipi_csi/mipi_csi.h"
#include "lib/video/vpp/vpp_dev.h"
#include "audio_msi/audio_adc.h"
#include "app/app_iic/app_iic.h"
#include "lib/video/isp/isp_dev.h"
#include "heap/aurpc_heap.h"
#include "audio/audio_coder.h"
#include "cjson/cJSON.h"
#include "demo/app_mem.h"
#include "lib/net/eloop/eloop.h"
#include "gen420/gen420_jpg_en.h"
#include "video/coder/vdec_wkq.h"
#include "video_app/video_msi.h"
#include "lib/video/dvp/jpeg/jpg_common.h"
#include "user_work/sd_work.h"
#include "lib/scale/scale_common.h"
#include "gen420_hardware_msi.h"
#include "spook.h"
#include "vfs_fatfs.h"
#include "fatfs/ff.h"
#include "stream_define.h"
#include "sys_config.h"
#include "app/viidure/recorder_viidure.h"
#include "ota.h"

extern void fatfs_sd0_init(void);
extern void user_workqueue_init(uint16 pri, void *stack, uint16 stack_size);

/**********************************************************************
 * print_level设置打印的等级,7是将所有打印都打开(调试的时候可以打开)
 * 例子:对应不同等级参考string.h
 *      os_printf(KERN_DEBUG"ABC"); //等级7
 *      os_printf(KERN_EMERG"ABC"); //等级1
 * disable_print_color 是否关闭打印颜色(特定串口工具)
 *******************************************************************/
static void app_print_init(void)
{
    print_level(7);
    disable_print_color(1);
}

static void app_heap_init(void)
{
    // 视频内存分配,分别配置av_psram和av_heap
    video_psram_init(0, CONFIG_PSRAM_AVHEAP_SIZE);
    video_sram_init(0, CONFIG_AVHEAP_SIZE);
}

static void app_workqueue_init(void)
{
    user_workqueue_init(OS_TASK_PRIORITY_HIGH, NULL, 2048);
    sd_workqueue_init(OS_TASK_PRIORITY_HIGH, NULL, 2048);
}

/***********************************************************
 * @brief 初始化vpp模块,注意不同方案需要的vpp配置不一样
 **********************************************************/
static void app_vpp_init(uint16_t w, uint16_t h)
{
    os_printf(KERN_INFO "vpp_cfg w:%d h:%d\n", w, h);
    // 设置buf0,
    vpp_set_buf_msg(VPP_BUF0, 1, VPP_BUF_IN_SRAM, w, h);
    // 设置buf1(如果需要副码流)
    vpp_set_buf_msg(VPP_BUF1, SUB_STREAM_EN, VPP_BUF_IN_PSRAM, SUB_STREAM_WIDTH, 0);
    vpp_set_double_psram_for_buf1(0);
    vpp_cfg(VPP_INPUT_FROM);
}

/*************************************************
 * @brief 初始化拍照模块,不同方案配置不一样
 ************************************************/
static void app_takephoto_init()
{
    // 初始化拍照模块,从vpp_data0获取数据
    auto_jpg_msi_init(AUTO_JPG, JPGID0, VPP_DATA0);
}

// 硬件模块初始化
static int32_t app_hardware_init(void)
{
    int32_t  ret = RET_OK;
    uint16_t w = 0, h = 0;
// 硬件初始化
// sd卡初始化
#if FS_EN
    ret = app_sd_init(STARTUP_OTA, "/sd0/ota.bin");
    APP_RETUNR_RET(ret);
#endif
    // iic线程初始化,i2c模块复用
    iic_thread_init();
    // sensor信息初始化
    sensor_info_init();
    mipi_csi_hardware_config(HG_MIPI_CSI_DEVID, 1, CAM_SINGLE_MASTER_MODE, 0, SENSOR_TYPE_MASTER, 15, NULL);
    get_single_mipi(HG_MIPI_CSI_DEVID, &w, &h);
    // isp初始化
    isp_cfg_dev();
    ircut_init();
    app_vpp_init(w, h);
    // 音频模块初始化
    app_audio_init(8000,16000);
    return ret;
}

// 用户应用初始化
static void app_user_init(void)
{
    // rtsp 初始化
    spook_init();
    // 录风者初始化
    config_Viidure(80);
}

static void app_function_init(void)
{
    // scale相关信号量初始化
    scale_mutex_init(); // scale相关锁初始化
    // jpg编码相关信号量以及空间初始化
    jpg_mutex_init();
    jpg_mem_init(32);

    // gen420模块启动,h264副码流或者mjpg手动编码需要
    gen420_hardware_msi_init();
    app_cjson_init();
    // 网络eloop模块初始化
    eloop_init();
    ota_Tcp_Server();
    app_h264_init(SUB_STREAM_EN);
    app_takephoto_init();
}

void ipc_1080p_demo_init(void)
{
    int32_t ret = RET_OK;
    app_print_init();
    app_heap_init();
    app_power_init();
    app_workqueue_init();
    // 硬件模块初始化
    ret = app_hardware_init();
    RETUNR(ret);
    // 应用代码初始化
    app_function_init();
    // 用户应用功能初始化
    app_user_init();
}
