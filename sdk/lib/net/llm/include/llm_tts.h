#ifndef _LLM_TTS_H_
#define _LLM_TTS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LLM_TTS_STATE_UNKNOWN       = 0,    // 未知状态
    LLM_TTS_STATE_IDLE          = 1,    // 空闲状态
    LLM_TTS_STATE_CONNECTING    = 2,    // 正在连接
    LLM_TTS_STATE_INTERRUPTING  = 3,    // 正在打断
    LLM_TTS_STATE_MAX,                  // MAX
} llm_tts_state;

struct llm_session_tts {
    struct llm_session  base;    
    struct os_mutex     lock;
    llm_tts_state       state;
    uint32              need_recycle: 1,    // 是否需要回收资源
                        rev: 31;
    uint32              again_cnt;          // 连续失败次数
    uint32              wpos_old;
    struct llm_tts_sparam *sparam;

    RBUFFER_DEF_R(mq_tx, struct llm_data);
    struct rbuffer  tts_rx;

    void *callback_rx_buff;
    void *private;
};

llm_tts_state llm_tts_get_state(struct llm_session_tts *tts_session);
int32 llm_tts_set_state(struct llm_session_tts *tts_session, llm_tts_state new_state);
int32 llm_tts_event(struct llm_session_tts *tts_session, uint16 event, uint32 param1, uint32 param2);

#ifdef __cplusplus
}
#endif

#endif

