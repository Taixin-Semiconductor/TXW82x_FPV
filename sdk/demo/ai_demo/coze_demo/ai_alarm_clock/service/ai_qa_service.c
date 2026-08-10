#include "ai_qa_service.h"
#include <string.h>
 
/* 最新文本缓存 */
static char g_user_text[AI_QA_TEXT_MAX_LEN];
static char g_ai_text[AI_QA_TEXT_MAX_LEN];

/* Service state */
static ai_qa_service_state_t g_service_state = AI_QA_SERVICE_STATE_IDLE;

void ai_qa_service_init(void)
{
    g_service_state = AI_QA_SERVICE_STATE_IDLE;
    g_user_text[0] = '\0';
    g_ai_text[0] = '\0';
}

void ai_qa_service_set_state(ai_qa_service_state_t state)
{
    g_service_state = state;
}

ai_qa_service_state_t ai_qa_service_get_state(void)
{
    return g_service_state;
}

/* 设置用户语音文本 */
void ai_qa_service_set_user_text(const char *text)
{
    if(text == NULL) return;
    strncpy(g_user_text, text, AI_QA_TEXT_MAX_LEN - 1);
    g_user_text[AI_QA_TEXT_MAX_LEN - 1] = '\0';
    g_service_state = AI_QA_SERVICE_STATE_USER_TXT_DONE;
}

/* 获取用户语音文本 */
const char *ai_qa_service_get_user_text(void)
{
    return g_user_text;
}

/* 设置AI回复文本 */
void ai_qa_service_set_ai_text(const char *text)
{
    if(text == NULL) return;
    strncpy(g_ai_text, text, AI_QA_TEXT_MAX_LEN - 1);
    g_ai_text[AI_QA_TEXT_MAX_LEN - 1] = '\0';
    g_service_state = AI_QA_SERVICE_STATE_AI_RSP_DONE;
}

/* 获取AI回复文本 */
const char *ai_qa_service_get_ai_text(void)
{
    return g_ai_text;
}

/* 重置服务 */
void ai_qa_service_reset_all(void)
{
    g_service_state = AI_QA_SERVICE_STATE_IDLE;
    g_user_text[0] = '\0';
    g_ai_text[0] = '\0';
}