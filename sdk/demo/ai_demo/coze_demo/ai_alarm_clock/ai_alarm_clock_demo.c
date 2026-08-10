#include "basic_include.h"
#include "coze_demo.h"
#include "demo/app_mem.h"
#include "demo/app_device.h"
#include "demo/app_common.h"
#include "syscfg.h"
#include "sys_config.h"
#include "stream_define.h"
#include "hal/pwm.h"
#include "cjson/cJSON.h"
#include "lib/common/atcmd.h"
#include "lib/net/eloop/eloop.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "lib/video/para_in/para_in_dev.h"
#include "lib/multimedia/msi.h"
#include "lib/net/dhcpd/dhcpd.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/heap/av_heap.h"
#include "lib/sdhost/sdhost.h"
#include "lib/touch/touch_pad.h"
#include "lib/lvgl_rotate_rpc/lvgl_rotate_msi.h"
#include "lib/scale/scale_common.h"
#include "lib/flashdisk/flashdisk.h"
#include "lib/video/dvp/jpeg/jpg_common.h"
#include "ota.h"
#include "vfs_fatfs.h"
#include "debug_log_msi.h"
#include "log_save_msi.h"
#include "hg_lv_mem.h"
#include "keyWork.h"
#include "keyScan.h"
#include "audio_msi/audio_adc.h"
#include "heap/aurpc_heap.h"
#include "audio/audio_coder.h"
#include "decode/decode.h"
#include "video_app/video_msi.h"
#include "interface_management/interface_mgnt_msi.h"
#include "app_lcd/app_lcd.h"
#include "app/app_iic/app_iic.h"
#include "app_lcd/lcd_virtual.h"
#include "user_work/user_work.h"

#if RTT_USB_EN
#include "rtthread.h"
#endif

extern void  huwen_wakeup_init(void);

void coze_demo_exit(void)
{
    gpio_set_val(MACRO_PIN(LLM_SYS_PWR_EN), 0);
}

/**
 * @brief 电源检测线程
 */
#define LONG_PRESS_MS    2000 // 长按2秒
#define SHORT_PRESS_MS   500  // 短按0.5秒
#define DEBOUNE_MS       50   // 去抖时间 50ms
#define TASK_INTERVAL_MS 10   // 线程扫描周期
struct os_task coze_pwr_detect_task;
void coze_pwr_detect_thread(void)
{
    // 去抖状态机变量
    uint8  initial_state_protection = 1;
    uint8  key_current_raw          = 1; // 原始按键值
    uint8  key_stable_last          = 1; // 去抖后稳定值
    uint64 debounce_tick            = 0; // 去抖时间戳
    uint64 press_start_tick         = 0; // 按下开始时间戳

    // GPIO 初始化
    gpio_set_dir(MACRO_PIN(LLM_SYS_PWR_EN), GPIO_DIR_OUTPUT);
    gpio_set_val(MACRO_PIN(LLM_SYS_PWR_EN), coze_mgr.pwr_en);

    gpio_set_dir(MACRO_PIN(LLM_PWR_KEY_DET), GPIO_DIR_INPUT);
    gpio_set_mode(MACRO_PIN(LLM_PWR_KEY_DET), GPIO_PULL_NONE, 0);

    while (1)
    {
        // 1. 读取原始按键电平
        key_current_raw = gpio_get_val(MACRO_PIN(LLM_PWR_KEY_DET));

        // 2. 软件去抖处理
        if (key_current_raw != key_stable_last)
        {
            debounce_tick   = os_jiffies_to_msecs(os_jiffies());
            key_stable_last = key_current_raw;
        }

        // 3. 检查去抖后是否有效
        if (os_jiffies_to_msecs(os_jiffies()) - debounce_tick >= DEBOUNE_MS)
        {
            // 刚开机保护
            if (initial_state_protection)
            {
                if (key_current_raw != 0)
                {
                    // 按键松开, 关闭保护
                    initial_state_protection = 0;
                }
            }
            else
            {
                if (key_current_raw == 0)
                {
                    // 按键稳定按下
                    if (press_start_tick == 0)
                    {
                        press_start_tick = os_jiffies_to_msecs(os_jiffies());
                    }
                    if (press_start_tick != 0)
                    {
                        if (os_jiffies_to_msecs(os_jiffies()) - press_start_tick >= LONG_PRESS_MS)
                        {
                            // 长按2s → 关机
                            coze_mgr.pwr_en  = 0;
                            press_start_tick = 0;
                        }
                    }
                }
                else
                {
                    // 按键稳定松开
                    if (press_start_tick != 0)
                    {
                        if (os_jiffies_to_msecs(os_jiffies()) - press_start_tick >= SHORT_PRESS_MS)
                        {
                            // 短按 → 语音触发
                            coze_mgr.voice_triggered = 1;
                        }
                        press_start_tick = 0;
                    }
                }
            }
        }
        os_sleep_ms(TASK_INTERVAL_MS);
    }
    return;
}

void coze_ui_set_ai_text(char *text)
{
    if (coze_mgr.ai_dialogue_screen_id < 0)
    {
        coze_err("screen_id is invalid(%d)\n", coze_mgr.ai_dialogue_screen_id);
        return;
    }
    screen_msg_queue_push_dispatch(coze_mgr.ai_dialogue_screen_id, text, 0);
}

void coze_ui_set_user_text(char *text)
{
    if (coze_mgr.ai_dialogue_screen_id < 0)
    {
        coze_err("screen_id is invalid(%d)\n", coze_mgr.ai_dialogue_screen_id);
        return;
    }
    screen_msg_queue_push_dispatch(coze_mgr.ai_dialogue_screen_id, text, 1);
}

static void coze_ui_cmd_recording(uint32 screen_id, uint8_t isrecoding)
{
    if (isrecoding)
    {
        coze_mgr.ai_dialogue_screen_id = screen_id;
        coze_mgr.key_triggered         = 1;
        coze_err("key start\r\n");
    }
    else
    {
        coze_mgr.key_triggered = 0;
        coze_err("key stop\r\n");
    }
}

static void coze_ui_cmd_destroy(uint32_t screen_id, uint8_t param)
{
    if (param == 0)
    {
        coze_msg_cmd_add(COZE_DEMO_CMD_INTERRUPT, param, 0);
    }
}

int32 coze_ui_cb(uint32_t screen_id, enum callback_cmd_t cmd, void *priv, uint32_t param)
{
    switch (cmd)
    {
        case CALLBACK_CMD_RECORDING:
            coze_ui_cmd_recording(screen_id, param);
            break;
        case CALLBACK_CMD_DESTROY:
            coze_ui_cmd_destroy(screen_id, param);
            break;
        default:
            break;
    }
    return 0;
}

void coze_ui_create(coze_ui_id ui_id)
{
    switch (ui_id)
    {
        case COZE_UI_ID_AI_DIALOGUE:
        {
            struct screen_common_s cmd = {0};
            cmd.magic                  = SCREEN_MAGIC;
            cmd.ui_id                  = AI_SCREEN;
            cmd.cb_priv                = NULL;
            cmd.cb                     = coze_ui_cb;
            screen_ioctl(0, SCREEN_IOCTL_CMD_CREATE_UI, (uint32_t) &cmd, 0);
            coze_mgr.ai_dialogue_screen_id = cmd.screen_id;
            break;
        }
        case COZE_UI_ID_AI_THINKING:
            if (coze_mgr.ai_dialogue_screen_id > 0)
            {
                screen_msg_queue_push_dispatch(coze_mgr.ai_dialogue_screen_id, "thinking", 3);
            }
            break;
        case COZE_UI_ID_MUSIC_PLAYER:
        {
            if (coze_mgr.audio_url_hdl >= 0)
            {
                struct screen_music_s cmd = {0};
                cmd.type                  = SCREEN_MUSIC_TYPE;
                cmd.magic                 = SCREEN_MAGIC;
                cmd.ui_id                 = MUSIC_SCREEN;
                cmd.cb_priv               = (void *) coze_mgr.audio_url_hdl;
                cmd.cb                    = coze_ui_cb;
                cmd.stream_id             = (uint32) coze_mgr.audio_url_hdl;
                screen_ioctl(0, SCREEN_IOCTL_CMD_CREATE_UI, (uint32_t) &cmd, 0);
                coze_mgr.music_player_screen_id = cmd.screen_id;
            }
            break;
        }
        default:
            break;
    }
}

void coze_ui_destroy(coze_ui_id ui_id)
{
    struct screen_common_s cmd = {0};
    cmd.magic                  = SCREEN_MAGIC;
    cmd.ui_id                  = SCREEN_DEL;

    switch (ui_id)
    {
        case COZE_UI_ID_AI_DIALOGUE:
            if (coze_mgr.ai_dialogue_screen_id > 0)
            {
                cmd.screen_id = coze_mgr.ai_dialogue_screen_id;
            }
            break;
        case COZE_UI_ID_MUSIC_PLAYER:
            if (coze_mgr.music_player_screen_id > 0)
            {
                cmd.screen_id = coze_mgr.music_player_screen_id;
            }
            break;
        default:
            break;
    }

    if (cmd.screen_id > 0)
    {
        screen_ioctl(0, SCREEN_IOCTL_CMD_CREATE_UI, (uint32_t) &cmd, 0);
    }
}

static void device_pwr_en(void)
{
    uint8_t pwr_io;
    pwr_io = PD_13;
    gpio_iomap_output(pwr_io, GPIO_IOMAP_OUTPUT);
    gpio_set_mode(pwr_io, GPIO_PULL_NONE, GPIO_PULL_LEVEL_NONE);
    gpio_set_dir(pwr_io, GPIO_DIR_OUTPUT);
    gpio_set_val(pwr_io, 0);
}

static void lcd_bl_pwm_init(void)
{
    struct hgpwm_v0 *pwm_dev;
    uint32           period_cnt = (DEFAULT_SYS_CLK / 1) / 1000 - 1;
    uint32           duty_cnt;
    uint8_t          bl_pwm_io    = PB_6;
    uint32           duty_percent = 50;

    gpio_iomap_output(MACRO_PIN(LCD_BACKLIGHT_IO), GPIO_IOMAP_OUTPUT);
    gpio_set_mode(MACRO_PIN(LCD_BACKLIGHT_IO), GPIO_PULL_NONE, GPIO_PULL_LEVEL_NONE); // PA_3
    gpio_set_dir(MACRO_PIN(LCD_BACKLIGHT_IO), GPIO_DIR_OUTPUT);
    gpio_set_val(MACRO_PIN(LCD_BACKLIGHT_IO), 1);

    pwm_dev = (struct hgpwm_v0 *) dev_get(HG_PWM0_DEVID);
    if (!pwm_dev)
    {
        return;
    }

    if (duty_percent > 100)
    {
        duty_percent = 100;
    }

    duty_cnt = (period_cnt * duty_percent) / 100;

    gpio_iomap_output(bl_pwm_io, GPIO_IOMAP_OUT_TMR0_PWM_OUT);
    gpio_set_mode(bl_pwm_io, GPIO_PULL_NONE, GPIO_PULL_LEVEL_NONE);
    gpio_driver_strength(bl_pwm_io, GPIO_DS_G1);

    pwm_init((struct pwm_device *) pwm_dev, PWM_CHANNEL_0, period_cnt, duty_cnt);
    pwm_start((struct pwm_device *) pwm_dev, PWM_CHANNEL_0);
}

void lcd_bl_pwm(uint32 duty_percent)
{
    struct hgpwm_v0 *pwm_dev    = (struct hgpwm_v0 *) dev_get(HG_PWM0_DEVID);
    uint32           period_cnt = (DEFAULT_SYS_CLK / 1) / 1000 - 1;
    uint32           duty_cnt;
    if (duty_percent > 100)
    {
        duty_percent = 100;
    }
    duty_cnt = (period_cnt * duty_percent) / 100;
    pwm_ioctl((struct pwm_device *) pwm_dev, PWM_CHANNEL_0, PWM_IOCTL_CMD_SET_PERIOD_DUTY, period_cnt, duty_cnt);
}

static uint32_t power_adc_check(struct key_callback_list_s *callback_list, uint32_t keyvalue, uint32_t extern_value)
{
    // 这里计算百分比,需要根据实际电路
    uint32_t key = keyvalue >> 8;
    if (key == POWER_CHECK)
    {
        // os_printf("battery adc extern_value = %d\r\n", extern_value);
        extern_value           = extern_value < 1600 ? 1600 : extern_value;
        extern_value           = extern_value > 1900 ? 1900 : extern_value;
        uint32_t power_percent = (extern_value - 1600) * 100 / (1900 - 1600);
        SYSEVT_NEW_SYSTEM_EVT(SYSEVT_SYSTEM_BATTERY_LEVEL, power_percent);
    }
    return 0;
}

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
}

static int32_t app_hardware_init(void)
{
    int32_t ret = RET_OK;
#if LCD_ST7789_SPI_EN
    device_pwr_en();
    // lcd_bl_pwm_init(50);
#endif
    iic_thread_init();

    keyWork_init(10);
    add_keycallback(power_adc_check, NULL);
    vfs_fatfs_register();
    flash_fatfs_init(0, 0);

    gpio_set_dir(PC_13, GPIO_DIR_OUTPUT);
    gpio_set_val(PC_13, 1);

#if HUWEN_WAKEUP_EN == 1
    void *aurpc_psram_buf = os_malloc_psram(AURPC_PSRAM_HEAP_SIZE);
    if(aurpc_psram_buf) {
        uint32 flags = SYSHEAP_FLAGS_MEM_ALIGN_32;   
        aurpc_psram_heap_init(aurpc_psram_buf, AURPC_PSRAM_HEAP_SIZE, flags);
    }
    audio_adc_init(AUSYS_AUAD, 16000, 1, 1);
    audio_mixer_init(16000, 32, 16, 1);
    dac_msg_init(0, 16000, 1);

    huwen_wakeup_init();
#else
    app_audio_init(16000,16000);
#endif

    app_lcd_init(NULL);
    lcd_ready_callback(lcd_bl_pwm_init);
    touch_pad_hardware_init(HG_TOUCHPAD_DEVID, chsc6x_touch_chip);

    return ret;
}

static void app_user_init(void)
{
}

static void app_function_init(void)
{
#ifdef PSRAM_HEAP
    cJSON_Hooks hook;
    hook.malloc_fn = _os_malloc_psram;
    hook.free_fn = _os_free_psram;
    cJSON_InitHooks(&hook);
#endif

    scale_mutex_init(); // scale相关锁初始化
    // jpg编码相关信号量以及空间初始化
    jpg_mutex_init();
    jpg_mem_init(32);
    app_cjson_init();
    coze_demo();
    OS_TASK_INIT("COZE_PWR", &coze_pwr_detect_task, coze_pwr_detect_thread, NULL, OS_TASK_PRIORITY_ABOVE_NORMAL, NULL, 1024);
    void ai_main_ui();
    app_lvgl_init(ai_main_ui,LVGL_INPUTDEV_SUPPORT);
}

void ai_alarm_clock_demo_init(void)
{
    app_print_init();
    app_heap_init();
    app_power_init();
    app_workqueue_init();
    app_hardware_init();
    app_function_init();
    app_user_init();
}
