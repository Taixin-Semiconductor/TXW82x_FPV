#ifndef _ISP_IRCUT_H_
#define _ISP_IRCUT_H_

#include "hal/isp.h"
#include "osal/work.h"
#include "osal/string.h"

#define IR_ABS(x) ((x) < 0 ? -(x) : (x))
#define IRCUT_MAX_CH    3       // 最大支持3目，按需修改

// 单通道全套IO引脚集合，每个Sensor独立
typedef struct {
    uint8 pin_ircut_det;
    uint8 pin_irled;
    uint8 pin_whiteled;
    uint8 pin_ircut_in1;
    uint8 pin_ircut_in2;
} IRCUT_CH_PIN;

// 单通道独立日夜切换阈值参数
typedef struct {
    uint8 frame_to_switch;      // IRCUT电机通电保持帧数
    uint8 switch_max;           // 防抖连续计数阈值
    float to_day_bv;            // 夜转日基础BV亮度
    float to_night_bv;          // 日转夜BV亮度阈值
    uint16 to_day_sat;          // 白天最低饱和度
    float to_day_bv_max;        // 强光BV上限
    uint16 to_day_diff_rb_gain; // 红蓝增益总差值阈值
    uint16 to_day_diff_b_gain;  // 蓝增益单独差值阈值
} IRCUT_THRESHOLD;

// ISP画面统计结构体，软件测光数据源
typedef struct {
    float curr_bv;
    uint16_t r_mean, g_mean, b_mean;
    uint16_t r_gain, b_gain;
    uint16_t saturation;
} ISP_IRCUT_STAT;

// 单路Sensor IRCUT运行状态
typedef struct ircut_info {
    IRCUT_CH_PIN pin;
    IRCUT_THRESHOLD thresh;

    uint8 ch_id;
    uint8 ircut_opt_status;
    uint8 whiteled_status;
    uint8 irled_status;
    uint8 ircut_status;
    uint8 irdet_status;
    uint8 irled_detect_mode;
    uint8 sw_state;
    uint8 isp_mode;
    uint8 frame_cnt;
    uint8 switch_cnt;

    uint8 ircut_en;
    uint8 action_status;
    uint8 action_type;
    uint8 ircut_gpio_en, irled_gpio_en, irdet_gpio_en, whiteled_gpio_en;
    uint16 last_r_gain, last_b_gain;
    
    uint8 valid;
} IRCUT_INFO;

// 多路数组、全局任务、初始化标记
typedef struct {
    struct isp_device *isp_dev;
    IRCUT_INFO ch_info[IRCUT_MAX_CH];
    struct os_work ircut_work;
    uint8 ircut_init;
} IRCUT_MGR;

// 状态枚举定义
enum {
    IRLED_OFF = 0,
    IRLED_ON,
};

enum {
    IRDET_OFF = 0,
    IRDET_ON,
};

enum {
    WHITELED_OFF = 0,
    WHITELED_ON,
};

enum {
    IRCUT_OPT_STATE_IDLE,
    IRCUT_OPT_STATE_SW_ON,
    IRCUT_OPT_STATE_SW_OFF,
    IRCUT_OPT_STATE_ON,
    IRCUT_OPT_STATE_OFF,
};

enum {
    IRCUT_OFF,
    IRCUT_ON,
};

enum {
    IRCUT_ACTION_STOP,
    IRCUT_ACTION_START,
};


enum {
    IRCUT_DET_MODE_SW,      // 软件模式：通过ISP画面BV/饱和度/RGB增益自动判断昼夜
    IRCUT_DET_MODE_HW,      // 硬件模式：外接光敏GPIO电平检测昼夜
    IRCUT_DET_MODE_MANUAL,  // 手动模式：关闭自动切换，上层控制IRCUT与补光灯
};


enum {
    ISP_MODE_DAY_COLOR   = 0, // 白天彩色模式，开启红外滤光片，抑制红外光
    ISP_MODE_NIGHT_COLOR = 1, // 夜间彩色模式，关闭滤光片，保留红外，保留色彩
    ISP_MODE_NIGHT_MONO  = 2, // 夜间黑白模式，关闭滤光片，增强红外响应，去除色彩
};

enum {
    IRCUT_STAT_DAY_TO_NIGHT,
    IRCUT_STAT_WB_REC,
    IRCUT_STAT_NIGHT_TO_DAY,
};


void ircut_init(void);
void ircut_deinit(void);
uint8 ircut_ch_init(uint8 ch_id);


extern IRCUT_MGR g_ircut_mgr;

#endif