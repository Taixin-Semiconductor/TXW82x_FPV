#ifndef _TXSEMI_LLM__H_
#define _TXSEMI_LLM__H_

#include "lib/net/llm/llm_api.h"
#include "lib/common/rbuffer.h"

#if !defined(LLM_CONFIG_FILE)
#include "llm_config_taixin.h"
#else
#include LLM_CONFIG_FILE
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define TAG "LLM:   "
#define llm_dbg(fmt, ...) //os_printf(TAG"%s:%d:"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define llm_err(fmt, ...) os_printf(KERN_ERR TAG"%s:%d:"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

enum LLM_TYPE {
    LLM_CHAT    = 1,
    LLM_STT     = 2,
    LLM_TTS     = 3,
    LLM_TTI     = 4,
    LLM_STS     = 5
};

struct llm_data {
    char    *llm_name;
    char    *buff1;
    int32   buff1_len;
    int32   offset;
    uint32  transfer_state: 4, type: 4, sub_type: 8, rev: 16;
};

typedef struct {
    char *buffer;       // 累积缓冲区
    uint32 total_size;  // 已累积数据大小
    int is_final;       // 是否最后一帧
} WsContext;

struct llm_model_data {
    int32 headsize;
    int32(*init)(void *session);
    int32(*deinit)(void *session);
    int32(*connect)(void *session);
    int32(*disconnect)(void *session);
    int32(*recycle)(void *session, uint8 param);
    int32(*upload)(void *session, struct llm_data *data);
    int32(*send)(void *session, struct llm_data *data);
    int32(*recv)(void *session, char *buff, uint32 buffer_size);
};

struct llm_session {
    char    *name;
    struct llm_model *model;
    struct llm_data  unsend;
    atomic_t users;

    void    *handle;
    void    *headers;
    void    *platform_config;
    void    *transfer_config;

    uint32  type: 8, 
            connected: 1,               // 连接状态
            exit: 1,                    // 会话退出
            destroying: 1,              // 内部线程正在销毁
            platform_config_change: 1,  // 平台配置是否改变
			new_dialogue: 1,  			// 新对话
            rev: 19;
    uint64	timeout;

    void(*run)(struct llm_session *session);
    llm_evt_cb evt_cb;
};

struct llm_taskinfo {
    void       *hdl;
    uint8       exit;
    uint32      bitmap;
};

struct llm_manager {
    uint16 task_reuse: 1, rev: 15;
    uint16 model_count;
    const struct llm_model   *models;
    struct llm_session *session[LLM_SESSION_MAX];
    struct llm_taskinfo tasks[LLM_SESSION_MAX];
};

int32 llm_add_session(struct llm_session *session);
int32 llm_del_session(struct llm_session *session);
struct llm_model *llm_find_model(char *name);

int32 llm_event_notify(struct llm_session *session, uint16 event, uint32 param1, uint32 param2);
uint32 llm_copy_config(void **old, void *new, uint32 size);
void llm_remove_spaces_and_newlines(char *str);
bool llm_is_sentence_end(const char *str);
int32 llm_equipment_legality_check(void);

#include "lib/net/llm/llm_trans.h"
#include "llm_chat.h"
#include "llm_stt.h"
#include "llm_tts.h"
#include "llm_tti.h"
#include "llm_sts.h"

#ifdef __cplusplus
}
#endif

#endif

