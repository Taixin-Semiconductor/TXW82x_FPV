#ifndef _LLM_STS_H_
#define _LLM_STS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LLM_STS_STATE_UNKNOWN       = 0,    // 未知状态
    LLM_STS_STATE_IDLE          = 1,    // 空闲状态
    LLM_STS_STATE_CONNECTING    = 2,    // 正在连接
    LLM_STS_STATE_RECONNECTING  = 3,    // 等待重连
    LLM_STS_STATE_WORKING       = 4,    // 工作状态
    LLM_STS_STATE_WAITING       = 5,    // 等待进入监听
    LLM_STS_STATE_DIALOGUE      = 6,    // 对话中
    LLM_STS_STATE_INTERRUPTING  = 7,    // 正在打断
    LLM_STS_STATE_MAX,                  // MAX
} llm_sts_state;
                    
struct llm_session_sts {
    struct llm_session  base;
    struct os_mutex     lock;
    llm_sts_state       state;
    llm_sts_state       last_state;
    uint32              need_recycle: 1,    // 是否需要回收资源
                        need_cancel: 1,     // 是否需要打断服务器
                        need_update: 1,     // 是否需要更新配置
                        rev: 29;
    uint32              again_cnt;          // 连续失败次数
    struct llm_sts_sparam *sparam;
    
    RBUFFER_DEF_R(sts_tx, struct llm_data);
    struct rbuffer sts_rx;

    void *callback_rx_buff;

    void *private;
};

llm_sts_state llm_sts_get_state(struct llm_session_sts *sts_session);
int32 llm_sts_set_state(struct llm_session_sts *sts_session, llm_sts_state new_state);
int32 llm_sts_event(struct llm_session_sts *sts_session, uint16 event, uint32 param1, uint32 param2);

#ifdef __cplusplus
}
#endif

#endif

