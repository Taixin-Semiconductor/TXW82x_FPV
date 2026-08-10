#ifndef _LLM_API_H_
#define _LLM_API_H_

#include "basic_include.h"
#include "lib/net/llm/llm_errno.h"
#include "llm/llm_trans.h"

#ifdef __cplusplus
extern "C" {
#endif

//STUB
void *llm_malloc(uint32 size);
void  llm_free(void *ptr);
void *llm_zalloc(size_t size);
void *llm_calloc(size_t nmemb, size_t size);
void *llm_realloc(void *ptr, size_t size);
char *llm_strdup(const char *s);

#define LLM_SESSION_MAX (3)

struct llm_model_data;

typedef int32(*llm_evt_cb)(void *session, uint16 evt, uint32 param1, uint32 param2);

enum llm_event {
    LLM_EVENT_UNKNOWN               = 0,
        
    //general
    LLM_EVENT_CONNECTED             = 1,    // 已连接
    LLM_EVENT_DISCONNECT            = 2,    // 已断开
    LLM_EVENT_CONN_ERR              = 3,    // 连接错误
    LLM_EVENT_TX_ERR                = 4,    // TX 错误
    LLM_EVENT_RX_ERR                = 5,    // RX 错误
    LLM_EVENT_TIMEOUT               = 6,    // 超时
    LLM_EVENT_STT_RESULT            = 7,
    LLM_EVENT_TTS_RESULT            = 8,
    LLM_EVENT_CHAT_RESULT           = 9,
    LLM_EVENT_TTI_RESULT            = 10,
    LLM_EVENT_ERROR_MSG             = 11,   // 服务器返回错误信息
    LLM_EVENT_FUNC_CALL             = 12,   // Function_calling
    LLM_EVENT_CUSTOMIZE             = 13,   // 自定义（内部未知事件）

    //llm_tts
    LLM_EVENT_TTS_AUDIO_START       = 14,   // 语音开始
    
    //llm_sts
    LLM_EVENT_VAD_RESULT            = 15,   // VAD 结果
    LLM_EVENT_DIALOGUE_START        = 16,   // 对话开始
    LLM_EVENT_DIALOGUE_TIMEOUT      = 17,   // 对话超时
    LLM_EVENT_DIALOGUE_END          = 18,   // 对话结束
    LLM_EVENT_INTERRUPT_END         = 19,   // 打断结束
    LLM_EVENT_WAITING_END           = 20,   // 等待通知服务器结束
    LLM_EVENT_UPLOAD_FILE_RESULT    = 21,   // 上传文件结果
    LLM_EVENT_CONVERSATION_ID       = 22,   // 上传conversation_id
    LLM_EVENT_MPLAYER_RESULT        = 23,   // MediaPlayer,例如mp3
    LLM_EVENT_IOT_RESULT            = 24,   // IOT 控制
};

typedef enum {
    LLM_CONFIG_TYPE_MODEL       = 0,
    LLM_CONFIG_TYPE_TRANS       = 1,
} llm_cfg_type;

typedef enum {
    LLM_CTRL_TYPE_RUN           = 0,
    LLM_CTRL_TYPE_STOP          = 1,
    LLM_CTRL_TYPE_INTERRUPT     = 2,
    LLM_CTRL_TYPE_RECONNECT     = 3,
    LLM_CTRL_TYPE_WAITING       = 4,
} llm_ctrl_type;

typedef enum {
    LLM_DATA_STATE_START        = 1,
    LLM_DATA_STATE_MIDDLE       = 2,
    LLM_DATA_STATE_END          = 3,
} llm_data_state;

typedef enum {
    LLM_DATA_TYPE_MGMT          = 0,
    LLM_DATA_TYPE_AUDIO         = 1,
    LLM_DATA_TYPE_FILE          = 2,
    LLM_DATA_TYPE_TEXT          = 3,
    LLM_DATA_TYPE_RAW           = 4,
    LLM_DATA_TYPE_TTS           = 5,
    LLM_DATA_TYPE_IMAGE         = 6,
    LLM_DATA_TYPE_PING          = 7,
} llm_data_type;

typedef enum {
    LLM_IMAGE_TYPE_JPEG         = 0,
    LLM_IMAGE_TYPE_PNG          = 1,
    LLM_IMAGE_TYPE_GIF          = 2,     
    LLM_IMAGE_TYPE_WEBP         = 3,
    LLM_IMAGE_TYPE_BMP          = 4,
    LLM_IMAGE_TYPE_TIFF         = 5,
    LLM_IMAGE_TYPE_ICO          = 6,
    LLM_IMAGE_TYPE_DIB          = 7,
    LLM_IMAGE_TYPE_ICNS         = 8,
    LLM_IMAGE_TYPE_SGI          = 9,
    LLM_IMAGE_TYPE_JPEG2000     = 10,
    LLM_IMAGE_TYPE_HEIC         = 11,
    LLM_IMAGE_TYPE_HEIF         = 12,
} llm_image_type;

struct llm_model {
    const char *name;
    const struct llm_model_data *m;
};

struct llm_global_param {
    uint16 task_reuse: 1, rev: 15;
    uint16 model_count;
    const struct llm_model *models;
};

struct llm_chat_sparam {
    uint16 qmsg_tx_cnt;         // TX 队列大小
    uint16 qmsg_rx_cnt;         // RX 队列大小
    uint16 image_max_cnt;       // 图片队列大小(图像识别)
    uint16 reply_fragment_size; // 文本分片大小(仅流式有效)
    uint16 cb_max_size;         // 流式接收缓冲区大小
    void   *evt_cb;
};

struct llm_stt_sparam {
    uint16 qmsg_tx_cnt;         // TX 队列大小
    uint16 cb_max_size;         // 流式接收缓冲区大小
    void   *evt_cb;
};

struct llm_tts_sparam {
    uint16 qmsg_tx_cnt;         // TX 队列大小
    uint16 rx_buff_size;        // 音频缓冲区大小
    uint16 cb_max_size;         // 流式接收缓冲区大小
    void   *evt_cb;
};

struct llm_tti_sparam {
    uint16 qmsg_tx_cnt;         // TX 队列大小
    uint16 cb_max_size;         // 流式接收缓冲区大小
    void   *evt_cb;
};

struct llm_sts_sparam {
    uint16 qmsg_tx_cnt;         // TX 队列大小
    uint16 rx_buff_size;        // 音频缓冲区大小
    uint16 cb_max_size;         // 流式接收缓冲区大小
    void   *evt_cb;
};

int32 llm_global_init(struct llm_global_param *global);
int32 llm_global_deinit(void);

//CHAT
void *llm_chat_init(char *llm_name, struct llm_chat_sparam *sparam);
int32 llm_chat_deinit(void *chat_hdl);
int32 llm_chat_send(void *chat_hdl, llm_data_type type, llm_data_state state, char *text, uint32 text_len);
int32 llm_chat_recv(void *chat_hdl, char *buff, uint32 buff_size, uint8 flags);
int32 llm_chat_set_role(void *chat_hdl, char *role);
int32 llm_chat_config(void *chat_hdl, llm_cfg_type type, void *cfg, uint32 cfg_size);
int32 llm_chat_ctrl(void *chat_hdl, llm_ctrl_type ctrl_type, uint32 timeout);
int32 llm_chat_upload_file(void *chat_hdl, llm_data_type data_type, uint32 param, 
                                char *data, uint32 data_len);

#define llm_chat_interrupt(chat_hdl, timeout) llm_chat_ctrl(chat_hdl, LLM_CTRL_TYPE_INTERRUPT, timeout)

//STT
void *llm_stt_init(char *llm_name, struct llm_stt_sparam *sparam);
int32 llm_stt_deinit(void *stt_hdl);
int32 llm_stt_send(void *stt_hdl, llm_data_type type, llm_data_state state, char *audio, uint32 audio_len);
int32 llm_stt_recv(void *stt_hdl, char *buff, uint32 buff_size, uint8 flags);
int32 llm_stt_config(void *stt_hdl, llm_cfg_type type, void *cfg, uint32 cfg_size);
int32 llm_stt_ctrl(void *stt_hdl, llm_ctrl_type ctrl_type, uint32 timeout);

#define llm_stt_interrupt(stt_hdl, timeout) llm_stt_ctrl(stt_hdl, LLM_CTRL_TYPE_INTERRUPT, timeout)

//TTS
void *llm_tts_init(char *llm_name, struct llm_tts_sparam *sparam);
int32 llm_tts_deinit(void *tts_hdl);
int32 llm_tts_send(void *tts_hdl, llm_data_type type, llm_data_state state, char *text, uint32 text_len);
int32 llm_tts_recv(void *tts_hdl, char *buff, uint32 buff_size, uint8 flags);
int32 llm_tts_config(void *tts_hdl, llm_cfg_type type, void *cfg, uint32 cfg_size);
int32 llm_tts_play_audio(void *tts_hdl, const       char *audio, uint32 length);
int32 llm_tts_ctrl(void *tts_hdl, llm_ctrl_type ctrl_type, uint32 timeout);

#define llm_tts_interrupt(tts_hdl, timeout) llm_tts_ctrl(tts_hdl, LLM_CTRL_TYPE_INTERRUPT, timeout)

//TTI
void *llm_tti_init(char *llm_name, struct llm_tti_sparam *sparam);
int32 llm_tti_deinit(void *tti_hdl);
int32 llm_tti_send(void *tti_hdl, llm_data_type type, llm_data_state state, char *text, uint32 text_len);
int32 llm_tti_recv(void *tti_hdl, char *buff, uint32 buff_size, uint8 flags);
int32 llm_tti_config(void *tti_hdl, llm_cfg_type type, void *cfg, uint32 cfg_size);
int32 llm_tti_ctrl(void *tti_hdl, llm_ctrl_type ctrl_type, uint32 timeout);

#define llm_tti_interrupt(tti_hdl, timeout) llm_tti_ctrl(tti_hdl, LLM_CTRL_TYPE_INTERRUPT, timeout)

//STS
void *llm_sts_init(char *llm_name, struct llm_sts_sparam *sparam);
int32 llm_sts_deinit(void *sts_hdl);
int32 llm_sts_play_audio(void *sts_hdl, const char *audio, uint32 length);
int32 llm_sts_check_audio(void *sts_hdl);
int32 llm_sts_send(void *sts_hdl, llm_data_type type, llm_data_state state, char *data, uint32 data_len);
int32 llm_sts_recv(void *sts_hdl, char *buff, uint32 buff_size, uint8 flags);
int32 llm_sts_config(void *sts_hdl, llm_cfg_type type, void *cfg, uint32 cfg_size);
int32 llm_sts_ctrl(void *sts_hdl, llm_ctrl_type ctrl_type, uint32 timeout);
int32 llm_sts_upload_file(void *sts_hdl, llm_data_type type, llm_data_state state, 
                                char *data, uint32 data_len);

#define llm_sts_run(sts_hdl)                llm_sts_ctrl(sts_hdl, LLM_CTRL_TYPE_RUN, 0)
#define llm_sts_stop(sts_hdl)               llm_sts_ctrl(sts_hdl, LLM_CTRL_TYPE_STOP, 0)
#define llm_sts_reconnect(sts_hdl)          llm_sts_ctrl(sts_hdl, LLM_CTRL_TYPE_RECONNECT, 0)
#define llm_sts_interrupt(sts_hdl, timeout) llm_sts_ctrl(sts_hdl, LLM_CTRL_TYPE_INTERRUPT, timeout)
#define llm_sts_wait(sts_hdl, timeout)      llm_sts_ctrl(sts_hdl, LLM_CTRL_TYPE_WAITING, timeout)

/////////////////////////////////////////////////////////////////////
//doubao
#include "lib/net/llm/doubao/doubao_llm.h"
#include "lib/net/llm/doubao/doubao_stt.h"
#include "lib/net/llm/doubao/doubao_tts.h"
#include "lib/net/llm/doubao/doubao_tti.h"

//listenai
//#include "lib/net/llm/listenai/listenai.h"

//coze
#include "lib/net/llm/coze/coze.h"

//neteast
#include "lib/net/llm/neteast/neteast.h"


/////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif

