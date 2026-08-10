#ifndef _LLM_TTI_H_
#define _LLM_TTI_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LLM_TTI_STATE_UNKNOWN       = 0,    // 未知状态
    LLM_TTI_STATE_IDLE          = 1,    // 空闲状态
    LLM_TTI_STATE_CONNECTING    = 2,    // 正在连接
    LLM_TTI_STATE_INTERRUPTING  = 3,    // 正在打断
    LLM_TTI_STATE_MAX,                  // MAX
} llm_tti_state;

struct llm_session_tti {
    struct llm_session  base;
    struct os_mutex     lock;
    llm_tti_state       state;
    uint32              need_recycle: 1,    // 是否需要回收资源
                        rev: 31;
    uint32              again_cnt;          // 连续失败次数
    struct llm_tti_sparam *sparam;

    RBUFFER_DEF_R(mq_tx, struct llm_data);

    void *callback_rx_buff;
    void *private;
};

llm_tti_state llm_tti_get_state(struct llm_session_tti *tti_session);
int32 llm_tti_set_state(struct llm_session_tti *tti_session, llm_tti_state new_state);
int32 llm_tti_event(struct llm_session_tti *tti_session, uint16 event, uint32 param1, uint32 param2);

#ifdef __cplusplus
}
#endif

#endif  //_LLM_TTI_H_

