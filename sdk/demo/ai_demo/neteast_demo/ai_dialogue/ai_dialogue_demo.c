#include "basic_include.h"
#include "coze_demo.h"
#include "demo/app_mem.h"
#include "demo/app_device.h"
#include "demo/app_common.h"
#include "stream_define.h"
#include "syscfg.h"
#include "sys_config.h"
#include "cjson/cJSON.h"
#include "hal/pwm.h"
#include "lib/common/atcmd.h"
#include "lib/net/eloop/eloop.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "lib/video/para_in/para_in_dev.h"
#include "lib/multimedia/msi.h"
#include "lib/touch/touch_pad.h"
#include "lib/lvgl_rotate_rpc/lvgl_rotate_msi.h"
#include "lib/scale/scale_common.h"
#include "lib/net/dhcpd/dhcpd.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/heap/av_heap.h"
#include "audio_msi/audio_adc.h"
#include "heap/aurpc_heap.h"
#include "multimedia/image/coder/jpg_msi.h"
#include "multimedia/audio/audio_coder.h"
#include "multimedia/video/coder/vdec_wkq.h"
#include "hg_lv_mem.h"
#include "keyWork.h"
#include "keyScan.h"
#include "lib/sdhost/sdhost.h"
#include "decode/decode.h"
#include "video_app/video_msi.h"
#include "interface_management/interface_mgnt_msi.h"
#include "app_lcd/lcd_virtual.h"
#include "app/app_iic/app_iic.h"
#include "app_lcd/app_lcd.h"

#if RTT_USB_EN
#include "rtthread.h"
#endif

extern void user_workqueue_init(uint16 pri,void *stack,uint16 stack_size);
extern void huwen_wakeup_init(void);

/*
 * @brief 处理按键回调函数
 * @param callback_list 按键回调列表指针
 * @param keyvalue 按键值
 * @param extern_value 外部值
 * @return 操作结果，0表示成功，其他值表示失败
*/
static uint32_t app_llm_awaken_intercom_push_key(struct key_callback_list_s *callback_list, uint32_t keyvalue, uint32_t extern_value)
{
    static uint8 key_state = 0;
    if ((keyvalue >> 8) != AD_PRESS)
    { return 0; }
    uint32 key_val = (keyvalue & 0xff);
    if ((key_val == KEY_EVENT_LDOWN) || (key_val == KEY_EVENT_REPEAT)) {
        if (key_state == 0) {
            os_printf("key start\r\n");
            key_state = 1;
            neteast_mgr.key_triggered = 1;
        }
    } else if ((key_val == KEY_EVENT_LUP)) {
        if (key_state == 1) {
            os_printf("key stop\r\n");
            key_state = 0;
            neteast_mgr.key_triggered = 0;
        }
    }
    return 0;
}

/**
 * @brief 普林芯驰唤醒识别线程
 */
#if LLM_SPV12XX
struct os_task app_llm_awaken_task;
static void app_llm_awaken_detect_thread(void)
{
/*
 * @brief ASR 芯片唤醒识别线程(普林芯驰 SPV12x 系列)
*/
#define DEBOUNCE_COUNT  3       // 去抖动计数器阈值，连续检测到相同状态3次认为有效
#define COOLDOWN_TIME   3000    // 冷却时间，单位为毫秒（3秒）
    // 设置 LLM_WK_IO 为输入模式，并配置上拉电阻为 100K 欧姆
    gpio_set_dir(LLM_WK_IO, GPIO_DIR_INPUT);
    gpio_set_mode(LLM_WK_IO, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);

    uint64 low_start_time = 0;      // 记录引脚拉低开始的时间（单位：毫秒）
    uint64 cooldown_end_time = 0;   // 记录冷却结束的时间（单位：毫秒）
    bool is_low = false;            // 标记是否处于低电平状态
    int debounce_counter = 0;       // 去抖动计数器
    bool last_state = true;         // 记录上一次读取的状态，默认高电平

    while (1) {
        os_sleep_ms(1);
        uint64 current_time = os_jiffies_to_msecs(os_jiffies());
        // 如果处于冷却时间内，跳过检测逻辑
        if (current_time < cooldown_end_time) {
            continue;
        }

        bool current_state = gpio_get_val(LLM_WK_IO) == 0; // 当前状态，true 表示低电平
        if (current_state == last_state) {
            // 如果当前状态与上次相同，增加计数器
            debounce_counter++;
            if (debounce_counter >= DEBOUNCE_COUNT) {
                debounce_counter = DEBOUNCE_COUNT; // 防止计数器溢出

                // 如果低电平刚刚被确认有效，则记录时间为当前时间减去 DEBOUNCE_COUNT 消耗的时间
                if (!is_low && current_state) { // 从高变低
                    is_low = true;
                    low_start_time = current_time - DEBOUNCE_COUNT; // 减去去抖动消耗的时间
                }

                // 如果已经是低电平状态，检查持续时间
                if (is_low) {
                    uint64 duration = current_time - low_start_time;
                    //os_printf(KERN_ERR"LLM_WK_IO pulled low for %ums.\r\n", duration);

                    // 判断是否在 16ms 到 20ms 范围内
                    if (duration >= 16 && duration <= 20) {
                        // 持续低电平在 16ms 到 20ms 范围内，触发唤醒逻辑
                        os_printf("Wakeup triggered!\r\n");

                        is_low = false;         // 重置低电平状态
                        debounce_counter = 0;   // 重置去抖动计数器
                        last_state = true;      // 重置上次状态

                        //audac_disable_play();
                        // 设置冷却结束时间
                        cooldown_end_time = current_time + COOLDOWN_TIME;
                        neteast_mgr.voice_triggered = 1;
                        continue;
                    }
                }
            }
        } else {
            debounce_counter = 0;
        }
        last_state = current_state;
    }
}
#endif

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

static void app_hardware_init(void)
{
    void eff_stop();
    eff_stop();
    keyWork_init(10);

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

#ifdef AI_DIALOGUE_VISION
    scale_mutex_init();
extern int32 jpg_mutex_init();
extern void jpg_mem_init(int num);
    jpg_mutex_init();
    jpg_mem_init(32);

    app_lcd_init(NULL);
//    touch_pad_hardware_init(HG_TOUCHPAD_DEVID, cst226se_touch_chip);
#endif
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

extern void neteast_demo(void);
    neteast_demo();
    add_keycallback(app_llm_awaken_intercom_push_key, NULL);
#if LLM_SPV12XX
    OS_TASK_INIT("APP_LLM_AWAKEN", &app_llm_awaken_task, app_llm_awaken_detect_thread, NULL, OS_TASK_PRIORITY_NORMAL + 1, NULL, 1024);
#endif

#ifdef AI_DIALOGUE_VISION
extern void ai_dialogue_ui(void);
    app_lvgl_init(ai_dialogue_ui,0);
#endif
}

void ai_dialogue_demo_init(void)
{
    app_print_init();
    app_heap_init();
    app_power_init();
    app_workqueue_init();
    app_hardware_init(); 
    app_function_init();
    app_user_init();
}
