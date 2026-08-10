/***********************************************
 * @brief 1080p睡眠回调函数
 * @param[in] event 事件类型
 * @param[in] data 事件数据
 * @return 无
 *
 ***********************************************/
#include "basic_include.h"
#include "battery_camera_sleep_1080p_cb.h"
#include "audio_msi/audio_adc.h"
#include "audio/audio_coder.h"
#include "lib/video/isp/isp_dev.h"
#include "lib/video/vpp/vpp_dev.h"
#include "lib/video/mipi_csi/mipi_csi.h"
#include "lib/sdhost/sdhost.h"
#include "lowPower_app.h"
#ifdef PIN_FROM_PARAM
#include "pin_param.h"
#endif

#define APP_1080P_GPIO_PORT_COUNT 6
#define APP_1080P_GPIO_PORT_WIDTH 16

static void app_1080p_add_dsleep_pin(uint32 gpio_mask[APP_1080P_GPIO_PORT_COUNT], uint32 pin)
{
    uint32 port;
    uint32 bit;

    if (pin >= MAX_PIN_NUM || pin == 255) {
        return;
    }

    port = pin / APP_1080P_GPIO_PORT_WIDTH;
    bit = pin % APP_1080P_GPIO_PORT_WIDTH;
    gpio_mask[port] |= BIT(bit);
}

static void app_1080p_set_dsleep_pins(void)
{
    uint32 gpio_mask[APP_1080P_GPIO_PORT_COUNT] = {0};
    const uint32 pins[] = {
        MACRO_PIN(PIN_SYS_PWR_EN),
        MACRO_PIN(PIN_TF_PWR_EN),
        MACRO_PIN(PIN_VDD_CTRL),
        MACRO_PIN(PIN_AUDIO_PA_EN),
    };
    uint32 i;

    for (i = 0; i < ARRAY_SIZE(pins); i++) {
        app_1080p_add_dsleep_pin(gpio_mask, pins[i]);
    }

    dsleep_set_user_gioa(gpio_mask[0]);
    dsleep_set_user_giob(gpio_mask[1]);
    dsleep_set_user_gioc(gpio_mask[2]);
    dsleep_set_user_giod(gpio_mask[3]);
    dsleep_set_user_gioe(gpio_mask[4]);
    dsleep_set_user_giof(gpio_mask[5]);
}

static int app_1080p_prev_resume(void *param1, void *param2, void *param3, void *param4)
{
    // VDD PWR CTRL
    gpio_set_dir(MACRO_PIN(PIN_VDD_CTRL), GPIO_DIR_OUTPUT);
    gpio_set_val(MACRO_PIN(PIN_VDD_CTRL), 1);

    // TF PWR EN
    gpio_set_dir(MACRO_PIN(PIN_TF_PWR_EN), GPIO_DIR_OUTPUT);
    gpio_set_val(MACRO_PIN(PIN_TF_PWR_EN), 0);

    // AUDIO PA EN
    gpio_set_dir(MACRO_PIN(PIN_AUDIO_PA_EN), GPIO_DIR_OUTPUT);
    gpio_set_val(MACRO_PIN(PIN_AUDIO_PA_EN), 1);

    pmu_vcam_ldo_en(VCAM_EN, VCAM_VOL);
    pmu_vcam2_ldo_en(VCAM2_EN, VCAM2_VOL);
    sysctrl_mipi_lcd_h264_mjpeg_clk_init();
    return RET_OK;
}

static int app_1080p_mipi_suspend(void *param1, void *param2, void *param3, void *param4)
{
    sensor_info_destory();
    mipi_csi_hardware_deconfig();
    return RET_OK;
}

static int app_1080p_mipi_resume(void *param1, void *param2, void *param3, void *param4)
{
    sensor_info_init();
    mipi_csi_hardware_config(HG_MIPI_CSI_DEVID, 2, CAM_SINGLE_MASTER_MODE, 0, SENSOR_TYPE_MASTER, 15, NULL);
    return RET_OK;
}

static int app_1080p_isp_suspend(void *param1, void *param2, void *param3, void *param4)
{
    isp_dev_close();
    return RET_OK;
}

static int app_1080p_isp_resume(void *param1, void *param2, void *param3, void *param4)
{
    isp_cfg_dev();
    return RET_OK;
}

static int app_1080p_vpp_suspend(void *param1, void *param2, void *param3, void *param4)
{
    vpp_cfg_release();
    return RET_OK;
}

static int app_1080p_vpp_resume(void *param1, void *param2, void *param3, void *param4)
{
    vpp_cfg(VPP_INPUT_FROM);
    return RET_OK;
}

static int app_1080p_audio_suspend(void *param1, void *param2, void *param3, void *param4)
{
    audio_adc_deinit(AUSYS_AUAD);
    dac_msg_task_suspend();
    return RET_OK;
}

static int app_1080p_audio_resume(void *param1, void *param2, void *param3, void *param4)
{
    audio_adc_init(AUSYS_AUAD, 8000, 1, 4);
    dac_msg_task_resume();
    return RET_OK;
}

static int app_1080p_sd_suspend(void *param1, void *param2, void *param3, void *param4)
{
    extern void sys_mount_device(uint16 dev_id, uint16 dev_type, uint8 umount);
    sys_mount_device(HG_SD0_DEVID, DEV_TYPE_SD, 1);
    sdhost_deinit_for_sleep();
    return RET_OK;
}

static int app_1080p_sd_resume(void *param1, void *param2, void *param3, void *param4)
{
    extern void fatfs_sd0_init(void);
    fatfs_sd0_init();
    return RET_OK;
}

static int app_1080p_post_suspend(void *param1, void *param2, void *param3, void *param4)
{
    // TF PWR EN
    gpio_set_val(MACRO_PIN(PIN_TF_PWR_EN), 1);

    // VDD PWR CTRL
    gpio_set_val(MACRO_PIN(PIN_VDD_CTRL), 0);

    // AUDIO PA EN
    gpio_set_val(MACRO_PIN(PIN_AUDIO_PA_EN), 0);

    app_1080p_set_dsleep_pins();

    return RET_OK;
}

static int app_1080p_post_resume(void *param1, void *param2, void *param3, void *param4)
{
    jtag_map_set(1);
    return RET_OK;
}

static const struct lowPower_module_ops app_1080p_lowpower_modules[] = {
    {
        "prev",
        NULL,
        app_1080p_prev_resume,
        {NULL, NULL, NULL, NULL},
    },
    {
        "sd",
        app_1080p_sd_suspend,
        app_1080p_sd_resume,
        {NULL, NULL, NULL, NULL},
    },
    {
        "mipi",
        app_1080p_mipi_suspend,
        app_1080p_mipi_resume,
        {NULL, NULL, NULL, NULL},
    },
    {
        "isp",
        app_1080p_isp_suspend,
        app_1080p_isp_resume,
        {NULL, NULL, NULL, NULL},
    },
    {
        "vpp",
        app_1080p_vpp_suspend,
        app_1080p_vpp_resume,
        {NULL, NULL, NULL, NULL},
    },
    {
        "audio",
        app_1080p_audio_suspend,
        app_1080p_audio_resume,
        {NULL, NULL, NULL, NULL},
    },
    {
        "post",
        app_1080p_post_suspend,
        app_1080p_post_resume,
        {NULL, NULL, NULL, NULL},
    },
};

int app_1080p_lowpower_register(void)
{
    if (lowpower_app_init(app_1080p_lowpower_modules, ARRAY_SIZE(app_1080p_lowpower_modules)) != RET_OK) {
        os_printf("%s:%d init lowpower app failed\n",
                  __FUNCTION__, __LINE__);
        return RET_ERR;
    }

    return RET_OK;
}
