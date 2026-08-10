#ifndef AI_QA_SERVICE_H
#define AI_QA_SERVICE_H
 
#define AI_QA_TEXT_MAX_LEN    256

/* AI Q&A Service 状态机 */
typedef enum {
    AI_QA_SERVICE_STATE_IDLE,              /* 空闲状态 */
    AI_QA_SERVICE_STATE_WAIT_USER_TXT,     /* 等待用户语音文字转换完成 */
    AI_QA_SERVICE_STATE_USER_TXT_DONE,     /* 用户语音文字转换完成 */
    AI_QA_SERVICE_STATE_WAIT_AI_RSP,       /* 等待AI回复完成 */
    AI_QA_SERVICE_STATE_AI_RSP_DONE        /* AI回复完成 */
} ai_qa_service_state_t;

/* 初始化服务 */
void ai_qa_service_init(void);

/* 状态机操作 */
void ai_qa_service_set_state(ai_qa_service_state_t state);
ai_qa_service_state_t ai_qa_service_get_state(void);

/* 设置/获取 用户语音文本 */
void ai_qa_service_set_user_text(const char *text);
const char *ai_qa_service_get_user_text(void);

/* 设置/获取 AI回复文本 */
void ai_qa_service_set_ai_text(const char *text);
const char *ai_qa_service_get_ai_text(void);

/* 重置服务 */
void ai_qa_service_reset_all(void);

#endif /* AI_QA_SERVICE_H */