#ifndef __COZE_DEMO_H
#define __COZE_DEMO_H

#include "llm/llm_api.h"
#include "llm/llm_trans.h"
#include "lib/multimedia/txmplayer.h"

#include "syscfg.h"
#include "keyWork.h"
#include "keyScan.h"
#include "audio_adc.h"
#include "jpgdef.h"
#include "cJSON.h"
#include "osal_file.h"
#include "lwip/netdb.h"

#include "screen_command.h"
#include "screen_common.h"
#include "screen_msg_queue.h"

#ifdef PIN_FROM_PARAM
#include "pin_param.h"
#endif

#define coze_dbg(fmt, ...) //os_printf(TAG"%s:%d:"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define coze_err(fmt, ...) os_printf(KERN_ERR"%s:%d:"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define COZE_DEMO_EVENT_QUEUE_CNT               16      //事件队列大小
#define COZE_DEMO_CMD_QUEUE_CNT                 16      //命令队列大小

#define COZE_DEMO_DIALOGUE_IDLE_TIMEOUT         10      //多轮对话自动退出超时时间（s）
#define COZE_DEMO_DIALOGUE_PULLING_TIMEOUT      15      //对话拉流超时时间（s）
#define COZE_DEMO_DIALOGUE_WAITING_TIMEOUT      5       //对话等待超时时间（s）
#define COZE_DEMO_DIALOGUE_INTERRUPT_TIMEOUT    5       //对话中断超时时间（s）
#define COZE_DEMO_UPDATE_WEATHER_INTERVAL       3600    //天气更新间隔（s）

#define COZE_AUDIO_DEC_OPUS_SAMPLE_RATE         8000    //音频采样率（Hz）
#define COZE_AUDIO_DEC_OPUS_BITRATE             8000    //音频码率（kbps）
#define COZE_AUDIO_DEC_OPUS_FRAME_SIZE_MS       60      //音频帧长（ms）
#define COZE_AUDIO_DEC_OPUS_FRAME_LEN  ((COZE_AUDIO_DEC_OPUS_BITRATE * COZE_AUDIO_DEC_OPUS_FRAME_SIZE_MS / 1000) / 8)

#ifndef STRINGIFY
#define STRINGIFY(x) #x
#endif

#ifndef MACRO_TO_STR
#define MACRO_TO_STR(x) STRINGIFY(x)
#endif

#ifndef UINT64_MAX
#define UINT64_MAX 0xffffffffffffffffULL
#endif

typedef enum {
    COZE_DEMO_DIALOGUE_MODE_KEYBOARD       = 0,     // 按键触发模式
    COZE_DEMO_DIALOGUE_MODE_VOICE          = 1,     // 语音触发模式
    COZE_DEMO_DIALOGUE_MODE_IMAGE_UPDATED  = 2,     // 图像识别模式
    COZE_DEMO_DIALOGUE_MODE_UPDATE_WEATHER = 3,     // 更新天气信息模式
} coze_demo_dialogue_mode;

typedef enum {
    COZE_DEMO_STATE_IDLE            = 0,    // 空闲状态
    COZE_DEMO_STATE_CONNECTED       = 1,    // 已连接状态
    COZE_DEMO_STATE_DISCONNECTED    = 2,    // 已断开状态
    COZE_DEMO_STATE_READY           = 3,    // 就绪状态
    COZE_DEMO_STATE_WAITING         = 4,    // 等待服务器进入监听状态
    COZE_DEMO_STATE_PUSHING         = 5,    // 推流状态
    COZE_DEMO_STATE_PULLING         = 6,    // 拉流状态
} coze_demo_state;

typedef enum {
    COZE_DEMO_CMD_UNKNOWN           = 0,    // 未知命令
    COZE_DEMO_CMD_PAUSE             = 1,    // 暂停
    COZE_DEMO_CMD_PLAY              = 2,    // 播放
    COZE_DEMO_CMD_VOLUME            = 3,    // 设置音量
    COZE_DEMO_CMD_INTERRUPT         = 4,    // 打断
    COZE_DEMO_CMD_CLOSE_MEDIA       = 5,    // 关闭音乐
    COZE_DEMO_CMD_PLAY_TIMEOUT      = 6,    // 播放超时提示音
} coze_demo_cmd;

typedef enum {
    COZE_UI_ID_UNKNOWN      = 0,    // 未知界面
    COZE_UI_ID_AI_DIALOGUE  = 1,    // 对话界面
    COZE_UI_ID_AI_THINKING  = 2,    // 思考界面
    COZE_UI_ID_MUSIC_PLAYER = 3,    // 音乐播放界面
    COZE_UI_ID_MAX          = 4,    // 最大界面数量
} coze_ui_id;

struct coze_demo_event_msg {
    uint16  event;
    char    *msg;
    uint32  msg_len;
};

struct coze_demo_cmd_msg {
    uint16  cmd;
    uint32  param1;
    uint32  param2;
};

struct coze_demo_manage {
    void                *sts_session;
    struct os_mutex     lock;
    RBUFFER_DEF_R(event_queue, struct coze_demo_event_msg);
    RBUFFER_DEF_R(cmd_queue, struct coze_demo_cmd_msg);
    coze_demo_state     state;
    coze_demo_state     last_state;
    uint32  key_triggered:1,        // 按键触发标志位
            voice_triggered:1,      // 语音触发标志位
            img_triggered:1,        // 图像识别完成触发标志位
            finish: 1,              // 回复完成标志位
            pause: 2,               // 主动暂停播放标志位( pause & 0x1 == 1 即暂停)
            connected: 1,           // 已连接标志位
            pwr_en: 1,              // 电源标志位
            busy: 1,                // 正在对话
            get_public_ip_done: 1,  // 已获取公网IP
            get_weather_done: 1,    // 已获取天气信息
            rev: 21;
    uint32  send_audio_cnt;         // 已发送音频包数量
    uint32  ip_addr;                // IP 地址
    uint32  dialogue_timeout_cnt;   // 对话超时次数
    uint64  time_stamp;             // 用于触发多轮对话自动退出对话时间戳（单位：毫秒，默认值：UINT64_MAX）

    // weather
    char    *weather_str;
    uint64  update_weather_time;

    // audio
    int8    volume;
    struct  msi *coze_msi;
    struct  msi *audio_msi;
    int32   audio_url_hdl;
    
	// image	
    struct  msi *image_msi;
    uint32  jpg_data_tag;
    uint32  ai_dialogue_screen_id;
    uint32  music_player_screen_id;
    char    chat_config_parameter[512];
};

extern struct sys_config sys_cfgs;
extern struct coze_demo_manage coze_mgr;
extern struct coze_chat_platform_cfg coze_sts_platform_cfg;
extern txVideoInfo_t coze_video_info;
extern txAudioInfo_t coze_audio_info;

void coze_demo(void);

const char *coze_main_state_str(coze_demo_state state);
void coze_main_set_state(coze_demo_state new_state);
coze_demo_state coze_main_get_state(void);
int32 coze_main_waiting(void);
int32 coze_main_interrupt(void);
void coze_main_timeout_warn(void);
void coze_main_timeout_reset(void);
char *coze_msg_chat_config_parameters_append_ip(const char *input_json, const char *ip);

/* AUDIO */
void coze_audio_play_thread(void);
void coze_audio_wait_empty(void);
void coze_audio_set_volume(uint8 volume);
void coze_audio_mute(void);
void coze_audio_restore(void);
void coze_audio_play_prompt_tone(const char *audio, uint32 audio_len);
int32 coze_audio_send_data(void);

/* MSG */
int32 coze_msg_event_process(void);
int32 coze_msg_cmd_process(void);
int32 coze_msg_cmd_add(coze_demo_cmd cmd, uint32 param1, uint32 param2);

/* STUB */
void coze_demo_exit(void);
void coze_ui_set_ai_text(char *text);
void coze_ui_set_user_text(char *text);
void coze_ui_create(coze_ui_id ui_id);
void coze_ui_destroy(coze_ui_id ui_id);

#endif
