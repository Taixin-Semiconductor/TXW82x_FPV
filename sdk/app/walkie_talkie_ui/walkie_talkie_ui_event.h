#ifndef _WALKIE_TALKIE_UI_EVENT_H_
#define _WALKIE_TALKIE_UI_EVENT_H_

#include "basic_include.h"
#include "walkie-talkie/walkie_talkie.h"
#include "intercom/intercom.h"
#include "walkie_talkie_calling/walkie_talkie_calling.h"

#define USE_90_DEGREE_LOGO  0

// 回调事件类型
typedef enum {
    WT_EVT_MSI_INIT,            // 初始化 MSI 组件
    WT_EVT_DECODE_LOGO,         // 解码开机 Logo
    WT_EVT_BACKLIGHT_ON,        // 开启背光
    WT_EVT_BACKLIGHT_OFF,       // 关闭背光
    WT_EVT_VOLUME_SET,          // 音量设置
    WT_EVT_VIEW_SWITCH,         // 视图切换
    WT_EVT_PROMPT_TONE_PLAY,    // 提示音播放
    WT_EVT_PAIR_MODE_SET,       // 配对模式设置
    WT_EVT_PAIRSTATUS_GET,      // 获取配对状态
    WT_EVT_PAIRSUCCESS_GET,     // 获取配对成功状态
    WT_EVT_DISP_NUM_SET,        // 设置当前显示的画面数量
    WT_EVT_DISP_NUM_GET,        // 获取当前显示的画面数量
    WT_EVT_DAC_EMPTY_GET,       // 获取 DAC 空状态
    WT_EVT_BAT_DET_INIT,        // 电池检测初始化
    WT_EVT_BAT_GET_LEVEL,       // 获取电池电压等级
    WT_EVT_VPP_IPF_INIT,        // 相框初始化
    WT_EVT_VPP_IPF_CTRL,        // 相框切换
    WT_EVT_JPG_DECODE_RUN,      // 解码jpg背景图
    WT_EVT_WELCOME_READY_GET,   // 获取UI初始化结束标志
    WT_EVT_WELCOME_READY_SET,   // 设置UI初始化结束标志

    WT_EVT_WIFI_CONNECT_GET,        // 获取 WiFi 连接状态
    WT_EVT_WIFI_SIGNAL_RSSI_GET,    // 获取 WiFi 信号强度（RSSI）
    WT_EVT_WIFI_SIGNAL_EVM_GET,     // 获取 WiFi 信号质量（EVM）
    WT_EVT_WIFI_CHANNEL_GET,        // 获取 WiFi 信道
    WT_EVT_WIFI_FREQ_OFFSET_GET,    // 获取 WiFi 频偏
    
    WT_EVT_CALLING_SET,
    WT_EVT_CALLING_GET,
} wt_msi_event_t;


// 回调函数指针
typedef int (*wt_msi_callback_t)(wt_msi_event_t event, uint32_t param1, uint32_t param2, uint32_t param3);

// 函数声明
void walkie_talkie_register_msi_callback(wt_msi_callback_t cb);

// 事件发送接口
int walkie_talkie_send_event(wt_msi_event_t event, uint32_t param1, uint32_t param2, uint32_t param3);

// 外部模块初始化入口
void walkie_talkie_msi_ext_init(void);

#endif