#ifndef __NETEAST_DEMO_H
#define __NETEAST_DEMO_H

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

#define NETEAST_DEMO_EVENT_QUEUE_CNT               16  //事件队列大小

#define NETEAST_DEMO_DIALOGUE_IDLE_TIMEOUT         10  //多轮对话自动退出超时时间（s）
#define NETEAST_DEMO_DIALOGUE_PULLING_TIMEOUT      15  //对话拉流超时时间（s）
#define NETEAST_DEMO_DIALOGUE_WAITING_TIMEOUT      5   //对话等待超时时间（s）
#define NETEAST_DEMO_DIALOGUE_INTERRUPT_TIMEOUT    5   //对话中断超时时间（s）

#define NETEAST_AUDIO_DEC_PCM_SAMPLE_RATE         16000    //音频采样率（Hz）
#define NETEAST_AUDIO_DEC_PCM_FRAME_LEN          (640)

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
    DIALOGUE_MODE_KEYBOARD       = 0,     // 按键触发模式
    DIALOGUE_MODE_VOICE          = 1,     // 语音触发模式
    DIALOGUE_MODE_IMAGE_UPDATED  = 2,     // 图像上传完毕自动触发模式  
} neteast_demo_dialogue_mode;

typedef enum {
    NETEAST_DEMO_STATE_IDLE            = 0,    // 空闲状态
    NETEAST_DEMO_STATE_CONNECTED       = 1,    // 已连接状态
    NETEAST_DEMO_STATE_DISCONNECTED    = 2,    // 已断开状态
    NETEAST_DEMO_STATE_READY           = 3,    // 就绪状态
    NETEAST_DEMO_STATE_WAITING         = 4,    // 等待服务器进入监听状态
    NETEAST_DEMO_STATE_PUSHING         = 5,    // 推流状态
} neteast_demo_state;

struct neteast_demo_event_msg {
    uint16  event;
    char    *msg;
    uint32  msg_len;
};

struct neteast_demo_manage {
    void                *sts_session;
    struct os_mutex     lock;
    RBUFFER_DEF_R(event_queue, struct neteast_demo_event_msg);
    neteast_demo_state     state;
    uint32  key_triggered:1,        // 按键触发标志位
            voice_triggered:1,      // 语音触发标志位
            img_triggered:1,        // 图像识别完成触发标志位
            finish: 1,              // 回复完成标志位
            pause: 1,               // 主动暂停播放标志位
            connected: 1,           // 已连接标志位
            pwr_en: 1,              // 电源标志位
            rev: 26;
    uint32  send_audio_cnt;         // 已发送音频包数量
    uint32  ip_addr;                // IP 地址
    uint32  dialogue_timeout_cnt;   // 对话超时次数
    uint64  time_stamp;             // 用于触发多轮对话自动退出对话时间戳（单位：毫秒，默认值：UINT64_MAX）

    int8   volume;
    struct  msi *neteast_msi;
    struct  msi *txmplayer_msi;
    int32   txmplayer_hdl;
	// image	
    uint32  jpg_data_tag;
    char   chat_config_parameter[512];
    uint64  keep_alive;
};

extern struct sys_config sys_cfgs;
extern struct neteast_demo_manage neteast_mgr;
extern struct neteast_chat_platform_cfg neteast_sts_platform_cfg;

void neteast_demo(void);

const char *neteast_main_state_str(neteast_demo_state state);
void neteast_main_set_state(neteast_demo_state new_state);
neteast_demo_state neteast_main_get_state(void);
int32 neteast_main_waiting(void);
int32 neteast_main_interrupt(void);
void neteast_main_timeout_warn(void);
void neteast_main_timeout_reset(void);
char *neteast_msg_chat_config_parameters_append_ip(const char *input_json, const char *ip);

/* AUDIO */
void neteast_audio_play_thread(void);
void neteast_audio_wait_empty(void);
void neteast_audio_set_volume(uint8 volume);
void neteast_audio_mute(void);
void neteast_audio_restore(void);
void neteast_audio_play_prompt_tone(const char *audio, uint32 audio_len);
int32 neteast_audio_send_data(void);

/* MSG */
int32 neteast_msg_event_process(void);

/* AWAKE */
void neteast_awaken_detect_thread(void);
void neteast_pwr_detect_thread(void);
uint32_t neteast_awaken_intercom_push_key(struct key_callback_list_s *callback_list, uint32_t keyvalue, uint32_t extern_value);

#endif
