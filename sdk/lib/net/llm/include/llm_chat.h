#ifndef _LLM_CHAT_H_
#define _LLM_CHAT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LLM_CHAT_STATE_UNKNOWN       = 0,    // 未知状态
    LLM_CHAT_STATE_IDLE          = 1,    // 空闲状态
    LLM_CHAT_STATE_CONNECTING    = 2,    // 正在连接
    LLM_CHAT_STATE_INTERRUPTING  = 3,    // 正在打断
    LLM_CHAT_STATE_MAX,                  // MAX
} llm_chat_state;

struct llm_session_chat {
    struct llm_session  base;
    struct os_mutex     lock;
    llm_chat_state      state;
    uint32              need_recycle: 1,    // 是否需要回收资源
                        rev: 31;
    uint32              again_cnt;          // 连续失败次数
    struct llm_chat_sparam *sparam;

    RBUFFER_DEF_R(mq_tx, struct llm_data);
    RBUFFER_DEF_R(mq_rx, struct llm_data);
    struct llm_data pending_rx;

    char *role;
    void *callback_rx_buff;
    void *private;
};

llm_chat_state llm_chat_get_state(struct llm_session_chat *chat_session);
int32 llm_chat_set_state(struct llm_session_chat *chat_session, llm_chat_state new_state);
int32 llm_chat_event(struct llm_session_chat *chat_session, uint16 event, uint32 param1, uint32 param2);

#ifdef __cplusplus
}
#endif

#endif

