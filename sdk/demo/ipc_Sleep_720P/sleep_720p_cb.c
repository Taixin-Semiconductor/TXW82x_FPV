/***********************************************
 * @brief 720P睡眠回调函数
 * @param[in] event 事件类型
 * @param[in] data 事件数据
 * @return 无
 *
 ***********************************************/
#include "basic_include.h"
#include "sleep_720p_cb.h"
#include "audio_msi/audio_adc.h"
#include "audio/audio_coder.h"
#include "lib/video/isp/isp_dev.h"
#include "lib/video/vpp/vpp_dev.h"
#include "lib/video/mipi_csi/mipi_csi.h"
#include "lib/sdhost/sdhost.h"
#include "lowPower_app.h"

static int app_720p_prev_resume(void *param1, void *param2, void *param3, void *param4)
{
    pmu_vcam_ldo_en(VCAM_EN, VCAM_VOL);
    pmu_vcam2_ldo_en(VCAM2_EN, VCAM2_VOL);
    sysctrl_mipi_lcd_h264_mjpeg_clk_init();
    return RET_OK;
}

static int app_720p_mipi_suspend(void *param1, void *param2, void *param3, void *param4)
{
    sensor_info_destory();
    mipi_csi_hardware_deconfig();
    return RET_OK;
}

static int app_720p_mipi_resume(void *param1, void *param2, void *param3, void *param4)
{
    sensor_info_init();
    mipi_csi_hardware_config(HG_MIPI_CSI_DEVID, 1, CAM_SINGLE_MASTER_MODE, 0, SENSOR_TYPE_MASTER, 24, NULL);
    return RET_OK;
}

static int app_720p_isp_suspend(void *param1, void *param2, void *param3, void *param4)
{
    isp_dev_close();
    return RET_OK;
}

static int app_720p_isp_resume(void *param1, void *param2, void *param3, void *param4)
{
    isp_cfg_dev();
    return RET_OK;
}

static int app_720p_vpp_suspend(void *param1, void *param2, void *param3, void *param4)
{
    vpp_cfg_release();
    return RET_OK;
}

static int app_720p_vpp_resume(void *param1, void *param2, void *param3, void *param4)
{
    vpp_cfg(VPP_INPUT_FROM);
    return RET_OK;
}

static int app_720p_audio_suspend(void *param1, void *param2, void *param3, void *param4)
{
    audio_adc_deinit(AUSYS_AUAD);
    dac_msg_task_suspend();
    return RET_OK;
}

static int app_720p_audio_resume(void *param1, void *param2, void *param3, void *param4)
{
    audio_adc_init(AUSYS_AUAD, 8000, 1, 4);
    dac_msg_task_resume();
    return RET_OK;
}

static int app_720p_sd_suspend(void *param1, void *param2, void *param3, void *param4)
{
    extern void sys_mount_device(uint16 dev_id, uint16 dev_type, uint8 umount);
    sys_mount_device(HG_SD0_DEVID, DEV_TYPE_SD, 1);
    sdhost_deinit_for_sleep();
    return RET_OK;
}

static int app_720p_sd_resume(void *param1, void *param2, void *param3, void *param4)
{
    extern void fatfs_sd0_init(void);
    fatfs_sd0_init();
    return RET_OK;
}

static int app_720p_post_resume(void *param1, void *param2, void *param3, void *param4)
{
    jtag_map_set(1);
    return RET_OK;
}

static const struct lowPower_module_ops app_720p_lowpower_modules[] = {
    {
        "prev",
        NULL,
        app_720p_prev_resume,
        {NULL, NULL, NULL, NULL},
    },
    {
        "sd",
        app_720p_sd_suspend,
        app_720p_sd_resume,
        {NULL, NULL, NULL, NULL},
    },
    {
        "mipi",
        app_720p_mipi_suspend,
        app_720p_mipi_resume,
        {NULL, NULL, NULL, NULL},
    },
    {
        "isp",
        app_720p_isp_suspend,
        app_720p_isp_resume,
        {NULL, NULL, NULL, NULL},
    },
    {
        "vpp",
        app_720p_vpp_suspend,
        app_720p_vpp_resume,
        {NULL, NULL, NULL, NULL},
    },
    {
        "audio",
        app_720p_audio_suspend,
        app_720p_audio_resume,
        {NULL, NULL, NULL, NULL},
    },
    {
        "post",
        NULL,
        app_720p_post_resume,
        {NULL, NULL, NULL, NULL},
    },
};

int app_720p_lowpower_register(void)
{
    if (lowpower_app_init(app_720p_lowpower_modules, ARRAY_SIZE(app_720p_lowpower_modules)) != RET_OK) {
        os_printf("%s:%d init lowpower app failed\n",
                  __FUNCTION__, __LINE__);
        return RET_ERR;
    }

    return RET_OK;
}
