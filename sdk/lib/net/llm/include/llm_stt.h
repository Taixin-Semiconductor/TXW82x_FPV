#ifndef _LLM_STT_H_
#define _LLM_STT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LLM_STT_STATE_UNKNOWN       = 0,    // 未知状态
    LLM_STT_STATE_IDLE          = 1,    // 空闲状态
    LLM_STT_STATE_CONNECTING    = 2,    // 正在连接
    LLM_STT_STATE_INTERRUPTING  = 3,    // 正在打断
    LLM_STT_STATE_MAX,                  // MAX
} llm_stt_state;

struct llm_session_stt {
    struct llm_session  base;
    struct os_mutex     lock;
    llm_stt_state       state;
    uint32              need_recycle: 1,    // 是否需要回收资源
                        rev: 31;
    uint32              again_cnt;          // 连续失败次数
    struct llm_stt_sparam *sparam;

    RBUFFER_DEF_R(mq_tx, struct llm_data);

    void *callback_rx_buff;
    void *private;
};

llm_stt_state llm_stt_get_state(struct llm_session_stt *stt_session);
int32 llm_stt_set_state(struct llm_session_stt *stt_session, llm_stt_state new_state);
int32 llm_stt_event(struct llm_session_stt *stt_session, uint16 event, uint32 param1, uint32 param2);

#ifdef __cplusplus
}
#endif

#endif

