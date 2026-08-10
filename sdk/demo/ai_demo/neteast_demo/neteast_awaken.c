#include "neteast_demo.h"

#if NETEAST_DEMO
/**
 * @brief 普林芯驰唤醒识别线程
 */
void neteast_awaken_detect_thread(void)
{
#if LLM_SPV12XX
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
                        os_printf(KERN_ERR"Wakeup triggered!\r\n");

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
#else
    return;
#endif
}

/*
 * @brief 处理按键回调函数
 * @param callback_list 按键回调列表指针
 * @param keyvalue 按键值
 * @param extern_value 外部值
 * @return 操作结果，0表示成功，其他值表示失败
*/
uint32_t neteast_awaken_intercom_push_key(struct key_callback_list_s *callback_list, uint32_t keyvalue, uint32_t extern_value)
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
 * @brief 电源检测线程
 */
#if LLM_PWR_DETECT
#define LONG_PRESS_MS       2000        // 长按2秒
#define SHORT_PRESS_MS      500        	// 短按0.5秒
#define DEBOUNE_MS          50          // 去抖时间 50ms2
#define TASK_INTERVAL_MS    10          // 线程扫描周期

void neteast_pwr_detect_thread(void)
{
    // 去抖状态机变量
    uint8 key_current_raw = 1;      // 原始按键值
    uint8 key_stable_last = 1;      // 去抖后稳定值
    uint64 debounce_tick = 0;       // 去抖时间戳
    uint64 press_start_tick = 0;    // 按下开始时间戳

    // GPIO 初始化
    gpio_set_dir(LLM_SYS_PWR_EN, GPIO_DIR_OUTPUT);
    gpio_set_val(LLM_SYS_PWR_EN, neteast_mgr.pwr_en);

    gpio_set_dir(LLM_PWR_KEY_DET, GPIO_DIR_INPUT);
    gpio_set_mode(LLM_PWR_KEY_DET, GPIO_PULL_NONE, 0);

    while (1) {
        // 1. 读取原始按键电平
        key_current_raw = gpio_get_val(LLM_PWR_KEY_DET);

        // 2. 软件去抖处理
        if (key_current_raw != key_stable_last) {
            debounce_tick = os_jiffies_to_msecs(os_jiffies());
            key_stable_last = key_current_raw;
        }

        // 3. 检查去抖后是否有效
        if (os_jiffies_to_msecs(os_jiffies()) - debounce_tick >= DEBOUNE_MS) {
            if (key_current_raw == 0) {
                // 按键稳定按下
                if (press_start_tick == 0) {
                    press_start_tick = os_jiffies_to_msecs(os_jiffies());
                }
                if (press_start_tick != 0) {
                    if (os_jiffies_to_msecs(os_jiffies()) - press_start_tick >= LONG_PRESS_MS) {
                        // 长按3s → 关机
                        neteast_mgr.pwr_en = 0;
                        press_start_tick = 0;
                    }
                }
            } else {
                // 按键稳定松开
                if (press_start_tick != 0) {
                    if (os_jiffies_to_msecs(os_jiffies()) - press_start_tick >= SHORT_PRESS_MS) {
                        // 短按 → 语音触发
                        neteast_mgr.voice_triggered = 1;
                    }
                    press_start_tick = 0;
                }
            }
        }
        os_sleep_ms(TASK_INTERVAL_MS);
    }
    return;
}
#endif
#endif
