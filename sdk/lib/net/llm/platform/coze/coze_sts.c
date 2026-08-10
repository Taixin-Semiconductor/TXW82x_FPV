#include "llm.h"
#include "cJSON.h"
#include "list.h"

struct upload_chunk {
    struct list_head list;
    unsigned char *data;          // 分片数据
    unsigned int size;            // 分片大小
};

struct upload_ctx {
    struct list_head chunks;       // 分片链表头
    struct upload_chunk *current;  // 当前正在上传的分片
    unsigned int offset_in_chunk;  // 当前分片内已上传了多少
    uint32 tot_len;                // 总长度
};

struct coze_sts_private_params {
    WsContext ws_ctx;
    void *upload_sk;
    struct upload_ctx ctx;
    struct os_semaphore upload_done;
    struct os_mutex upload_lock;
    void *upload_task;
    void *session_sts;
    uint32 upload_task_running: 1, tts_req: 1, rev: 30;
};

#define AUDIO_CHUNK_SIZE            1024        // 音频固定分片写入大小

/* -------------------------- recv -------------------------- */
static int32 coze_sts_audio_recv_proc(struct llm_session_sts *sts_session, char *data, int data_len)
{
    const char *base64_start_pattern = "\"content\":\"";
    const uint32 pattern_len = os_strlen(base64_start_pattern);
    char *base64_start = NULL;
    char *base64_end = NULL;
    uint32 base64_str_len = 0;
    unsigned char *decoded_audio_buf = NULL;
    uint32 decoded_audio_len = 0;
    int32 ret = RET_OK;

    if (data == NULL || data_len <= 0) {
        llm_err("Invalid param: data is NULL or data_len=%d\r\n", data_len);
        return RET_ERR;
    }

    // 定位Base64音频字符串起始位置
    base64_start = os_strstr((unsigned char *)data, base64_start_pattern);
    if (base64_start == NULL) {
        llm_err("Failed to find base64 audio start pattern [\"content:\"]\r\n");
        llm_err("Raw data (len=%d): ", data_len);
        hgprintf_out(data, data_len, 0);
        _os_printf("\r\n");
        return RET_ERR;
    }
    base64_start += pattern_len; // 跳过起始匹配串

    // 定位Base64音频字符串结束位置
    base64_end = os_strchr(base64_start, '\"');
    if (base64_end == NULL) {
        llm_err("Failed to find base64 audio end char [\"]\r\n");
        llm_err("Raw data (len=%d): ", data_len);
        hgprintf_out(data, data_len, 0);
        _os_printf("\r\n");
        return RET_ERR;
    }
    base64_str_len = base64_end - base64_start;
    if (base64_str_len == 0) {
        llm_err("Base64 audio string is empty\r\n");
        return RET_ERR;
    }
    *(base64_end) = 0;

    // Base64解码（错误路径统一释放资源）
    ret = llm_base64_decode(base64_start, &decoded_audio_buf, &decoded_audio_len);
    if (ret != RET_OK || decoded_audio_buf == NULL || decoded_audio_len == 0) {
        llm_err("Base64 decode failed (ret=%d, len=%d)\r\n", ret, decoded_audio_len);
        goto cleanup;
    }

    int32 temp = 0;
    uint32 written = 0;
    uint32 wait_cnt = 0;

    while (written < decoded_audio_len) {
        if (llm_sts_get_state(sts_session) == LLM_STS_STATE_INTERRUPTING) {
            ret = LLME_INTR;
            goto cleanup;
        }
        temp = rbuffer_set(&sts_session->sts_rx, (void *)decoded_audio_buf + written, decoded_audio_len - written);
        if (temp > 0) {
            written += temp;
        } else {
            os_sleep_ms(10);
            wait_cnt++;
            if (wait_cnt == 50) {
                wait_cnt = 0;
                llm_err("Waiting for the application to read audio!(%d:%d:%d:%d)\r\n", RB_IDLE(&sts_session->sts_rx), temp, decoded_audio_len, written);
            }
        }
    }

    ret = RET_OK;
cleanup:
    if (decoded_audio_buf) { llm_free(decoded_audio_buf); }
    return ret;
}

static int32 coze_sts_chat_created_recv_proc(struct llm_session_sts *sts_session, char *data, int data_len)
{
    // 解析 JSON 字符串
    cJSON *json = cJSON_Parse(data);
    if (json == NULL) {
        llm_err("Error parsing JSON\n");
        return RET_ERR; // 解析失败
    }

    // 查找 "data" 对象
    cJSON *pdata = cJSON_GetObjectItem(json, "data");
    if (pdata == NULL) {
        llm_err("Error finding 'data' object\n");
        cJSON_Delete(json);
        return RET_ERR; // 找不到 "data" 对象
    }

    // 查找 "conversation_id"
    cJSON *conversation_id_item = cJSON_GetObjectItem(pdata, "conversation_id");
    if (conversation_id_item == NULL || !cJSON_IsString(conversation_id_item)) {
        llm_err("Error finding 'conversation_id'\n");
        cJSON_Delete(json);
        return RET_ERR; // 找不到 "conversation_id" 或不是字符串
    }

    llm_sts_event(sts_session, LLM_EVENT_CONVERSATION_ID,
                  (uint32)conversation_id_item->valuestring, os_strlen(conversation_id_item->valuestring));
    cJSON_Delete(json);
    return RET_OK;
}

static int32 coze_sts_audio_transcript_completed_recv_proc(struct llm_session_sts *sts_session, char *data, int data_len)
{
    cJSON *root = cJSON_Parse(data);
    if (!root) {
        llm_err("JSON parse failed\r\n");
        return RET_ERR;
    }

    cJSON *data_obj = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (cJSON_IsObject(data_obj)) {
        // 提取 content
        cJSON *content = cJSON_GetObjectItemCaseSensitive(data_obj, "content");
        if (cJSON_IsString(content) && content->valuestring) {
            llm_sts_event(sts_session, LLM_EVENT_STT_RESULT, (uint32)content->valuestring, 0);
        }
    }

    cJSON_Delete(root);
    return RET_OK;
}

static int32 coze_sts_audio_sentence_start_recv_proc(struct llm_session_sts *sts_session, char *data, int data_len)
{
    cJSON *root = cJSON_Parse(data);
    if (!root) {
        llm_err("JSON parse failed\r\n");
        return RET_ERR;
    }

    cJSON *data_obj = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (cJSON_IsObject(data_obj)) {
        // 提取 text
        cJSON *text = cJSON_GetObjectItemCaseSensitive(data_obj, "text");
        if (cJSON_IsString(text) && text->valuestring) {
            llm_sts_event(sts_session, LLM_EVENT_TTS_RESULT, (uint32)text->valuestring, 0);
        }
    }

    cJSON_Delete(root);
    return RET_OK;
}

static int32 coze_sts_message_completed_recv_proc(struct llm_session_sts *sts_session, char *data, int data_len)
{
    cJSON *root = NULL;
    cJSON *data_obj = NULL;
    cJSON *role_item = NULL;
    cJSON *type_item = NULL;
    cJSON *content_item = NULL;
    const char *image_marker = "data:image/";

    if (data == NULL || data_len == 0) {
        llm_err("msg_completed: invalid param\r\n");
        return RET_ERR;
    }

    root = cJSON_Parse(data);
    if (root == NULL) {
        llm_err("msg_completed: JSON parse failed\r\n");
        return RET_ERR;
    }

    data_obj = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (data_obj == NULL || !cJSON_IsObject(data_obj)) {
        llm_err("msg_completed: data field not found\r\n");
        cJSON_Delete(root);
        return RET_ERR;
    }

    role_item = cJSON_GetObjectItemCaseSensitive(data_obj, "role");
    type_item = cJSON_GetObjectItemCaseSensitive(data_obj, "type");
    content_item = cJSON_GetObjectItemCaseSensitive(data_obj, "content");

    if (role_item == NULL || !cJSON_IsString(role_item) ||
        type_item == NULL || !cJSON_IsString(type_item) ||
        content_item == NULL || !cJSON_IsString(content_item)) {
        llm_err("msg_completed: role/type/content missing or invalid\r\n");
        cJSON_Delete(root);
        return RET_ERR;
    }

    const char *role = role_item->valuestring;
    const char *type = type_item->valuestring;
    const char *content = content_item->valuestring;

    if (os_strcmp(role, "user") == 0 && os_strcmp(type, "question") == 0) {
        /* 用户语音/文字输入 */
        llm_sts_event(sts_session, LLM_EVENT_STT_RESULT, (uint32)content, 1);
    } else if (os_strcmp(role, "assistant") == 0 && os_strcmp(type, "answer") == 0) {
        /* AI回复，需进一步区分内容类型 */

        cJSON *content_json = cJSON_Parse(content);
        if (content_json != NULL) {
            cJSON *media_type = cJSON_GetObjectItemCaseSensitive(content_json, "type");
            cJSON *media_url = cJSON_GetObjectItemCaseSensitive(content_json, "url");

            if (media_type != NULL && cJSON_IsString(media_type) &&
                media_url != NULL && cJSON_IsString(media_url)) {
                /* 音乐/音频回复: content是JSON, 包含type和url字段 */
                llm_sts_event(sts_session, LLM_EVENT_MPLAYER_RESULT, (uint32)data, data_len);
            } else {
                /* content是其他格式的JSON，作为普通文本处理 */
                llm_sts_event(sts_session, LLM_EVENT_TTS_RESULT, (uint32)content, 1);
            }
            cJSON_Delete(content_json);
        } else if (os_strstr(content, image_marker) == content) {
            /* 图片回复: content以"data:image/"开头 */
            llm_sts_event(sts_session, LLM_EVENT_TTI_RESULT, (uint32)data, data_len);
        } else {
            /* 普通文本回复 */
            llm_sts_event(sts_session, LLM_EVENT_TTS_RESULT, (uint32)content, 1);
        }
    } else {
        llm_err("msg_completed: [UNKNOWN] role=%s type=%s\r\n", role, type);
    }

    cJSON_Delete(root);
    return RET_OK;
}

typedef void (*coze_sts_downward_events_hdl)(struct llm_session_sts *sts_session, char *json_buf, uint32 json_len);
typedef struct {
    const char *type_str;
    coze_sts_downward_events_hdl hdl;
} coze_sts_downward_events;

static inline void handle_chat_updated(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->recv chat updated!\r\n");
    if (sts_session->base.platform_config_change == 1) {
        llm_sts_event(sts_session, LLM_EVENT_WAITING_END, 0, 0);
    } else {
        llm_err("---->Intercept chat updated!\r\n");
    }
}

static inline void handle_conversation_chat_created(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->recv conversation.chat.created!\r\n");
    coze_sts_chat_created_recv_proc(sts_session, buf, len);
    llm_sts_event(sts_session, LLM_EVENT_DIALOGUE_START, 0, 0);
}

static inline void handle_conversation_chat_in_progress(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->recv conversation.chat.in_progress!\r\n");
    _os_printf("json len:%d\n", len);
    hgprintf_out(buf, len, 0);
    _os_printf("\r\n");
}

static inline void handle_conversation_audio_delta(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    struct coze_sts_private_params *private_params = (struct coze_sts_private_params *)sts_session->private;

    llm_dbg("---->recv conversation.audio.delta!\r\n");
    if (llm_sts_get_state(sts_session) == LLM_STS_STATE_DIALOGUE) {
        sts_session->base.timeout = os_jiffies_to_msecs(os_jiffies());
        coze_sts_audio_recv_proc(sts_session, buf, len);
    } else {
        if (llm_sts_get_state(sts_session) != LLM_STS_STATE_INTERRUPTING && private_params->tts_req == 1) {
            coze_sts_audio_recv_proc(sts_session, buf, len);
        } else {
            llm_dbg("---->Intercept conversation.audio.delta!\r\n");
        }
    }
}

static inline void handle_conversation_chat_completed(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->recv conversation.chat.completed!\r\n");
    if (llm_sts_get_state(sts_session) == LLM_STS_STATE_DIALOGUE) {
        llm_sts_event(sts_session, LLM_EVENT_DIALOGUE_END, 0, 0);
    } else {
        llm_err("---->Intercept conversation.chat.completed!\r\n");
    }
}

static inline void handle_conversation_audio_transcript_completed(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->recv conversation.audio_transcript.completed!\r\n");
    coze_sts_audio_transcript_completed_recv_proc(sts_session, buf, len);
}

static inline void handle_conversation_message_completed(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_dbg("---->recv conversation.message.completed!\r\n");
    coze_sts_message_completed_recv_proc(sts_session, buf, len);
}

static inline void handle_input_audio_buffer_speech_started(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->recv input_audio_buffer.speech_started!\r\n");
    llm_sts_event(sts_session, LLM_EVENT_VAD_RESULT, COZE_SERVER_VAD_START, 0);
}

static inline void handle_input_audio_buffer_speech_stopped(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->recv input_audio_buffer.speech_stopped!\r\n");
    llm_sts_event(sts_session, LLM_EVENT_VAD_RESULT, COZE_SERVER_VAD_STOP, 0);
}

static inline void handle_chat_created(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->recv chat.created!\r\n");
    hgprintf_out(buf, len, 0);
    _os_printf("\r\n");
}

static inline void handle_conversation_audio_sentence_start(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    struct coze_sts_private_params *private_params = (struct coze_sts_private_params *)sts_session->private;

    llm_err("---->recv conversation.audio.sentence_start!\r\n");
    if (llm_sts_get_state(sts_session) == LLM_STS_STATE_DIALOGUE) {
        coze_sts_audio_sentence_start_recv_proc(sts_session, buf, len);
    } else {
        if (llm_sts_get_state(sts_session) != LLM_STS_STATE_INTERRUPTING && private_params->tts_req == 1) {
            coze_sts_audio_sentence_start_recv_proc(sts_session, buf, len);
        } else {
            llm_err("---->Intercept conversation.audio.sentence_start!\r\n");
        }
    }
}

static inline void handle_conversation_audio_completed(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    struct coze_sts_private_params *private_params = (struct coze_sts_private_params *)sts_session->private;

    llm_dbg("---->recv conversation.audio.completed!\r\n");
    if (private_params->tts_req == 1) {
        private_params->tts_req = 0;
    }
}

static inline void handle_conversation_chat_canceled(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->recv conversation.chat.canceled!\r\n");
    llm_sts_event(sts_session, LLM_EVENT_INTERRUPT_END, 0, 0);
}

static inline void handle_conversation_chat_failed(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->Recv conversation.chat.failed!\r\n");
    llm_sts_event(sts_session, LLM_EVENT_ERROR_MSG, (uint32)buf, len);
}

static inline void handle_conversation_chat_requires_action(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->recv conversation.chat.requires_action!\r\n");
    llm_sts_event(sts_session, LLM_EVENT_IOT_RESULT, (uint32)buf, len);
}

static inline void handle_error_event(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->Recv error response from host\n");
    hgprintf_out(buf, len, 0);
    _os_printf("\r\n");
    llm_sts_event(sts_session, LLM_EVENT_ERROR_MSG, (uint32)buf, len);
}

static const coze_sts_downward_events g_coze_downward_events_table[] = {
    {"chat.updated",                            handle_chat_updated},
    {"conversation.chat.created",               handle_conversation_chat_created},
    {"conversation.chat.in_progress",           handle_conversation_chat_in_progress},
    {"conversation.audio.delta",                handle_conversation_audio_delta},
    {"conversation.chat.completed",             handle_conversation_chat_completed},
    {"input_audio_buffer.cleared",              NULL},
    {"conversation.audio_transcript.update",    NULL},
    {"conversation.audio_transcript.completed", handle_conversation_audio_transcript_completed},
    {"conversation.message.delta",              NULL},
    {"conversation.message.completed",          handle_conversation_message_completed},
    {"input_audio_buffer.speech_started",       handle_input_audio_buffer_speech_started},
    {"input_audio_buffer.speech_stopped",       handle_input_audio_buffer_speech_stopped},
    {"chat.created",                            handle_chat_created},
    {"conversation.audio.sentence_start",       handle_conversation_audio_sentence_start},
    {"conversation.audio.completed",            handle_conversation_audio_completed},
    {"conversation.chat.canceled",              handle_conversation_chat_canceled},
    {"conversation.chat.failed",                handle_conversation_chat_failed},
    {"conversation.chat.requires_action",       handle_conversation_chat_requires_action},
    {"input_audio_buffer.completed",            NULL},
    {"error",                                   handle_error_event},
};

#define EVENT_TABLE_SIZE (sizeof(g_coze_downward_events_table) / sizeof(g_coze_downward_events_table[0]))

static int32 coze_sts_match_type(const char *candidate, int cand_len, const char *expected)
{
    if (!candidate || !expected) { return 0; }
    int32 exp_len = os_strlen(expected);
    return (cand_len == (int)exp_len) && (os_strncmp(candidate, expected, exp_len) == 0);
}

static int32 coze_sts_parse_json(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    const char *type_key = "\"event_type\":\"";
    char *type_start = os_strstr(buf, type_key);
    const char *event_type = NULL;
    int type_len = 0;

    if (type_start) {
        type_start += os_strlen(type_key); // 跳过 "\"type\":\""
        char *type_end = os_strchr(type_start, '"');
        if (type_end && type_end > type_start) {
            event_type = type_start;
            type_len = (int)(type_end - type_start);
        }
    }

    uint8 handled = 0;
    if (event_type) {
        for (int i = 0; i < EVENT_TABLE_SIZE; i++) {
            if (coze_sts_match_type(event_type, type_len, g_coze_downward_events_table[i].type_str)) {
                if (g_coze_downward_events_table[i].hdl) {
                    g_coze_downward_events_table[i].hdl(sts_session, buf, len);
                }
                handled = 1;
                break;
            }
        }
    }

    if (!handled) {
        _os_printf("unknown json len:%d\n", len);
        hgprintf_out(buf, len, 0);
        _os_printf("\r\n");
        llm_sts_event(sts_session, LLM_EVENT_CUSTOMIZE, (uint32)buf, len);
    }

    return RET_OK;
}

static void coze_sts_ws_ctx_free(WsContext *ws_ctx)
{
    if (ws_ctx == NULL) { return; }
    if (ws_ctx->buffer) {
        llm_free(ws_ctx->buffer);
        ws_ctx->buffer = NULL;
    }
    ws_ctx->total_size = 0;
    ws_ctx->is_final = 0;
}

static int32 coze_sts_handle_text_frame(struct llm_session_sts *sts_session, const char *chunk, llm_recv_meta *rmeta)
{
    struct coze_sts_private_params *private_params = (struct coze_sts_private_params *)sts_session->private;
    WsContext *ws_ctx = &private_params->ws_ctx;
    uint32 curr_read = rmeta->nread;

    // 合法性检查：避免无效的内存拷贝
    if (curr_read == 0 || chunk == NULL) {
        llm_err("Invalid text frame data (chunk null or nread=0)\r\n");
        return RET_ERR;
    }

    // 分片消息：追加到现有缓冲区
    if (ws_ctx->buffer != NULL && ws_ctx->is_final == 0) {
        char *new_buf = llm_realloc(ws_ctx->buffer, ws_ctx->total_size + curr_read + 1);
        if (new_buf == NULL) {
            llm_err("Realloc buffer failed (need: %zu bytes)\r\n",
                    ws_ctx->total_size + curr_read + 1);
            coze_sts_ws_ctx_free(ws_ctx); // 释放旧缓冲区，避免内存泄漏
            return RET_ERR;
        }
        ws_ctx->buffer = new_buf;
        os_memcpy(ws_ctx->buffer + ws_ctx->total_size, chunk, curr_read);
        ws_ctx->total_size += curr_read;
    }
    // 新消息：初始化缓冲区
    else {
        // 异常检查：已有缓冲区但标记为最终帧，说明状态异常
        if (ws_ctx->buffer != NULL) {
            llm_err("WS context exception: buffer exists but is_final=1\r\n");
            coze_sts_ws_ctx_free(ws_ctx);
            return RET_ERR;
        }

        // 分配缓冲区（预留'\0'终止符空间）
        uint32 buf_size = rmeta->len + rmeta->bytesleft + 1;
        ws_ctx->buffer = llm_malloc(buf_size);
        if (ws_ctx->buffer == NULL) {
            llm_err("Malloc buffer failed (need: %u bytes)\r\n", buf_size);
            coze_sts_ws_ctx_free(ws_ctx);
            return RET_ERR;
        }

        // 拷贝初始数据
        os_memcpy(ws_ctx->buffer, chunk, curr_read);
        ws_ctx->total_size = curr_read;
    }

    // 更新是否为最终帧标记
    ws_ctx->is_final = (rmeta->bytesleft == 0) ? 1 : 0;
    return RET_OK;
}

static int32 coze_sts_handle_websocket(struct llm_session_sts *sts_session, char *chunk, uint32 length)
{
    int32 ret = RET_ERR;
    llm_recv_meta rmeta = {0};
    struct coze_sts_private_params *private_params = (struct coze_sts_private_params *)sts_session->private;

    if (chunk == NULL || length == 0) {
        llm_err("Invalid input params (chunk null / length=0)\r\n");
        return RET_ERR;
    }

    // 预处理WebSocket数据，解析帧信息
    ret = llm_websocket_precv(sts_session->base.handle, chunk, length, &rmeta);
    if (ret != RET_OK) {
        return ret;
    }

    // 处理文本帧
    if (rmeta.flags & LLMWS_TEXT) {
        ret = coze_sts_handle_text_frame(sts_session, chunk, &rmeta);
        if (ret != RET_OK) {
            return ret;
        }
    }
    // 处理关闭帧
    else if (rmeta.flags & LLMWS_CLOSE) {
        llm_err("WS server requested close connection\r\n");
        coze_sts_ws_ctx_free(&private_params->ws_ctx); // 释放上下文资源
        return RET_ERR;
    }
    // 不支持的帧类型
    else {
        llm_err("Unsupported WS frame type (flags: 0x%02x)\r\n", rmeta.flags);
        coze_sts_ws_ctx_free(&private_params->ws_ctx); // 释放上下文资源
        return RET_ERR;
    }

    // 处理完整消息（所有分片接收完成）
    if (private_params->ws_ctx.is_final && private_params->ws_ctx.total_size > 0) {
        private_params->ws_ctx.buffer[private_params->ws_ctx.total_size] = '\0';
        // 处理JSON消息
        if (sts_session->base.new_dialogue == 1) {
            coze_sts_parse_json(sts_session, private_params->ws_ctx.buffer, private_params->ws_ctx.total_size);
        } else {
            llm_dbg("Intercept data(%d): %s\r\n", private_params->ws_ctx.total_size, private_params->ws_ctx.buffer);
        }
        coze_sts_ws_ctx_free(&private_params->ws_ctx);
    }

    return RET_OK;
}

static int32 coze_sts_recv(void *session, char *buff, uint32 buffer_size)
{
    struct llm_session_sts *sts_session = NULL;
    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    sts_session = (struct llm_session_sts *)session;

    return coze_sts_handle_websocket(sts_session, buff, buffer_size);
}

static cJSON *coze_json_check_legitimacy(char *json_string)
{
    // 尝试解析 JSON 字符串
    cJSON *parsed_json = cJSON_Parse(json_string);

    // 检查解析结果
    if (parsed_json == NULL) {
        // 解析失败，获取错误指针
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            llm_err("Invalid JSON format. Error near: %s,len:%d\n", error_ptr, os_strlen(json_string));
            hgprintf_out(json_string, os_strlen(json_string), 0);
            _os_printf("\r\n");
        }
        return NULL; // 返回 NULL 表示解析失败
    }

    return parsed_json; // 返回解析成功的 cJSON 对象
}

static cJSON *coze_json_copy(char *json_string)
{
    // 检查 JSON 字符串的合法性
    cJSON *cjson_str = coze_json_check_legitimacy(json_string);
    if (cjson_str == NULL) {
        llm_err("Invalid JSON format\n");
        return NULL; // 如果不合法，返回 NULL
    }

    // 进行 JSON 拷贝
    cJSON *copied_json = cJSON_Duplicate(cjson_str, cJSON_True);
    cJSON_Delete(cjson_str);
    return copied_json; // 返回拷贝的 JSON 对象
}


/* -------------------------- send -------------------------- */
static int32 coze_sts_create_chat_update_json(struct llm_session_sts *sts_session, char **json_str)
{
    int32 json_str_len = RET_ERR;
    char *json_str_temp = NULL;
    struct coze_chat_platform_cfg *cfg = sts_session->base.platform_config;

    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "id", "event_id");
    cJSON_AddStringToObject(root, "event_type", "chat.update");

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "data", data);

    //chat_config
    if (cfg->chat_config_auto_save_history != NULL || cfg->chat_config_conversation_id != NULL
        || cfg->chat_config_user_id != NULL || cfg->chat_config_meta_data != NULL
        || cfg->chat_config_custom_variables != NULL || cfg->chat_config_extra_params != NULL
        || cfg->chat_config_parameters != NULL) {
        cJSON *chat_config = cJSON_CreateObject();
        cJSON_AddItemToObject(data, "chat_config", chat_config);

        if (0 == os_strcasecmp(cfg->chat_config_auto_save_history, "FALSE")) {
            cJSON_AddFalseToObject(chat_config, "auto_save_history");
        }
        if (cfg->chat_config_conversation_id) {
            cJSON_AddStringToObject(chat_config, "conversation_id", cfg->chat_config_conversation_id);
        }
        if (cfg->chat_config_user_id) {
            cJSON_AddStringToObject(chat_config, "user_id", cfg->chat_config_user_id);
        }
        if (cfg->chat_config_meta_data) {
            cJSON *meta_data = coze_json_copy(cfg->chat_config_meta_data);
            if (meta_data) {
                cJSON_AddItemToObject(chat_config, "meta_data", meta_data);
            } else {
                llm_err("Add meta_data to chat_update failed!\n");
            }
        }
        if (cfg->chat_config_custom_variables) {
            cJSON *custom_variables = coze_json_copy(cfg->chat_config_custom_variables);
            if (custom_variables) {
                cJSON_AddItemToObject(chat_config, "custom_variables", custom_variables);
            } else {
                llm_err("Add custom_variables to chat_update failed!\n");
            }
        }
        if (cfg->chat_config_extra_params) {
            cJSON *extra_params = coze_json_copy(cfg->chat_config_extra_params);
            if (extra_params) {
                cJSON_AddItemToObject(chat_config, "extra_params", extra_params);
            } else {
                llm_err("Add extra_params to chat_update failed!\n");
            }
        }
        if (cfg->chat_config_parameters) {
            cJSON *parameters = coze_json_copy(cfg->chat_config_parameters);
            if (parameters) {
                cJSON_AddItemToObject(chat_config, "parameters", parameters);
            } else {
                llm_err("Add parameters to chat_update failed!\n");
            }
        }
    }

    //input audio
    if (cfg->input_audio_format || cfg->input_audio_codec ||
        cfg->input_audio_sample_rate || cfg->input_audio_channel ||
        cfg->input_audio_bit_depth) {
        cJSON *input_audio = cJSON_CreateObject();
        cJSON_AddItemToObject(data, "input_audio", input_audio);
        if (cfg->input_audio_format) {
            cJSON_AddStringToObject(input_audio, "format", cfg->input_audio_format);
        }
        if (cfg->input_audio_codec) {
            cJSON_AddStringToObject(input_audio, "codec", cfg->input_audio_codec);
        }
        if (cfg->input_audio_sample_rate) {
            cJSON_AddNumberToObject(input_audio, "sample_rate", os_atoi(cfg->input_audio_sample_rate));
        }
        if (cfg->input_audio_channel) {
            cJSON_AddNumberToObject(input_audio, "channel", os_atoi(cfg->input_audio_channel));
        }
        if (cfg->input_audio_bit_depth) {
            cJSON_AddNumberToObject(input_audio, "bit_depth", os_atoi(cfg->input_audio_bit_depth));
        }
    }

    //output audio
    uint8 pcm_config_en = (cfg->output_audio_pcm_config_sample_rate != NULL) ||
                          (cfg->output_audio_pcm_config_frame_size_ms != NULL);
    uint8 opus_config_en = (cfg->output_audio_opus_config_bitrate != NULL) ||
                           (cfg->output_audio_opus_config_use_cbr != NULL) ||
                           (cfg->output_audio_opus_config_sample_rate != NULL) ||
                           (cfg->output_audio_opus_config_frame_size_ms != NULL);
    uint8 mp3_config_en = (cfg->output_audio_mp3_config_sample_rate != NULL) ||
                          (cfg->output_audio_mp3_config_bit_rate != NULL);
    if (cfg->output_audio_codec || cfg->output_audio_speech_rate ||
        cfg->output_audio_loudness_rate || cfg->output_audio_voice_id || cfg->output_audio_context_texts ||
        pcm_config_en || opus_config_en || mp3_config_en) {
        cJSON *output_audio = cJSON_CreateObject();
        cJSON_AddItemToObject(data, "output_audio", output_audio);
        if (cfg->output_audio_codec) {
            cJSON_AddStringToObject(output_audio, "codec", cfg->output_audio_codec);
        }
        if (pcm_config_en) {
            cJSON *pcm_config = cJSON_CreateObject();
            cJSON_AddItemToObject(output_audio, "pcm_config", pcm_config);
            if (cfg->output_audio_pcm_config_sample_rate) {
                cJSON_AddNumberToObject(pcm_config, "sample_rate", os_atoi(cfg->output_audio_pcm_config_sample_rate));
            }
            if (cfg->output_audio_pcm_config_frame_size_ms) {
                cJSON_AddNumberToObject(pcm_config, "frame_size_ms", os_atoi(cfg->output_audio_pcm_config_frame_size_ms));
            }
            cJSON *limit_config = cJSON_CreateObject();
            cJSON_AddItemToObject(pcm_config, "limit_config", limit_config);
            cJSON_AddNumberToObject(limit_config, "period", 1);
            if (cfg->output_audio_limit_config_max_frame_num) {
                cJSON_AddNumberToObject(limit_config, "max_frame_num", os_atoi(cfg->output_audio_limit_config_max_frame_num));
            } else {
                cJSON_AddNumberToObject(limit_config, "max_frame_num", 32);
            }
        }
        if (opus_config_en) {
            cJSON *opus_config = cJSON_CreateObject();
            cJSON_AddItemToObject(output_audio, "opus_config", opus_config);
            if (cfg->output_audio_opus_config_bitrate) {
                cJSON_AddNumberToObject(opus_config, "bitrate", os_atoi(cfg->output_audio_opus_config_bitrate));
            }
            if (cfg->output_audio_opus_config_use_cbr) {
                if (os_strcasecmp(cfg->output_audio_opus_config_use_cbr, "true") == 0) {
                    cJSON_AddTrueToObject(opus_config, "use_cbr");
                } else {
                    cJSON_AddFalseToObject(opus_config, "use_cbr");
                }
            }
            if (cfg->output_audio_opus_config_sample_rate) {
                cJSON_AddNumberToObject(opus_config, "sample_rate", os_atoi(cfg->output_audio_opus_config_sample_rate));
            }
            if (cfg->output_audio_opus_config_frame_size_ms) {
                cJSON_AddNumberToObject(opus_config, "frame_size_ms", os_atoi(cfg->output_audio_opus_config_frame_size_ms));//可选frame_size20-60
            }
            cJSON *limit_config = cJSON_CreateObject();
            cJSON_AddItemToObject(opus_config, "limit_config", limit_config);
            cJSON_AddNumberToObject(limit_config, "period", 1);
            if (cfg->output_audio_limit_config_max_frame_num) {
                cJSON_AddNumberToObject(limit_config, "max_frame_num", os_atoi(cfg->output_audio_limit_config_max_frame_num));
            } else {
                cJSON_AddNumberToObject(limit_config, "max_frame_num", 32);
            }
        }
        if (mp3_config_en) {
            cJSON *mp3_config = cJSON_CreateObject();
            cJSON_AddItemToObject(output_audio, "mp3_config", mp3_config);
            if (cfg->output_audio_mp3_config_sample_rate) {
                cJSON_AddNumberToObject(mp3_config, "sample_rate", os_atoi(cfg->output_audio_mp3_config_sample_rate));
            }
            if (cfg->output_audio_mp3_config_bit_rate) {
                cJSON_AddNumberToObject(mp3_config, "bit_rate", os_atoi(cfg->output_audio_mp3_config_bit_rate));
            }
        }
        if (cfg->output_audio_speech_rate) {
            cJSON_AddNumberToObject(output_audio, "speech_rate", os_atoi(cfg->output_audio_speech_rate));
        }
        if (cfg->output_audio_loudness_rate) {
            cJSON_AddNumberToObject(output_audio, "loudness_rate", os_atoi(cfg->output_audio_loudness_rate));
        }
        if (cfg->output_audio_voice_id != NULL) {
            cJSON_AddStringToObject(output_audio, "voice_id", cfg->output_audio_voice_id);
        }
        if (cfg->output_audio_context_texts != NULL) {
            cJSON_AddStringToObject(data, "context_texts", cfg->output_audio_context_texts);
        }
    }


    //turn detection
    uint8 interrupt_config_en = (cfg->turn_detection_interrupt_config_mode != NULL) ||
                                (cfg->turn_detection_interrupt_config_keywords != NULL &&
                                 *cfg->turn_detection_interrupt_config_keywords != NULL);
    if (cfg->turn_detection_type || cfg->turn_detection_silence_duration_ms || interrupt_config_en) {
        cJSON *turn_detection = cJSON_CreateObject();
        cJSON_AddItemToObject(data, "turn_detection", turn_detection);
        if (cfg->turn_detection_type) {
            cJSON_AddStringToObject(turn_detection, "type", cfg->turn_detection_type);
        }
        if (cfg->turn_detection_prefix_padding_ms) {
            cJSON_AddNumberToObject(turn_detection, "prefix_padding_ms", os_atoi(cfg->turn_detection_prefix_padding_ms));
        }
        if (cfg->turn_detection_silence_duration_ms) {
            cJSON_AddNumberToObject(turn_detection, "silence_duration_ms", os_atoi(cfg->turn_detection_silence_duration_ms));
        }
        if (interrupt_config_en) {
            if (cfg->turn_detection_interrupt_config_mode) {
                cJSON_AddStringToObject(turn_detection, "mode", cfg->turn_detection_interrupt_config_mode);
            }
            if (cfg->turn_detection_interrupt_config_keywords && *cfg->turn_detection_interrupt_config_keywords) {
                cJSON *keywords_array = cJSON_CreateArray();
                char **keyword_ptr = cfg->turn_detection_interrupt_config_keywords;
                while (*keyword_ptr != NULL) {
                    cJSON_AddItemToArray(keywords_array, cJSON_CreateString(*keyword_ptr));
                    keyword_ptr++;
                }
                cJSON_AddItemToObject(turn_detection, "keywords", keywords_array);
            }
        }
    }

    //asr config
    if (cfg->asr_config_user_language || cfg->asr_config_enable_ddc || cfg->asr_config_enable_itn ||
        cfg->asr_config_enable_punc || cfg->asr_config_stream_mode || cfg->asr_config_enable_nostream ||
        (cfg->asr_config_hot_words != NULL && *cfg->asr_config_hot_words != NULL)) {
        cJSON *asr_config = cJSON_CreateObject();
        cJSON_AddItemToObject(data, "asr_config", asr_config);
        if (cfg->asr_config_user_language) {
            cJSON_AddStringToObject(asr_config, "user_language", cfg->asr_config_user_language);
        }
        if (cfg->asr_config_enable_ddc) {
            if (os_strcasecmp(cfg->asr_config_enable_ddc, "true") == 0) {
                cJSON_AddTrueToObject(asr_config, "enable_ddc");
            } else {
                cJSON_AddFalseToObject(asr_config, "enable_ddc");
            }
        }
        if (cfg->asr_config_enable_itn) {
            if (os_strcasecmp(cfg->asr_config_enable_itn, "true") == 0) {
                cJSON_AddTrueToObject(asr_config, "enable_itn");
            } else {
                cJSON_AddFalseToObject(asr_config, "enable_itn");
            }
        }
        if (cfg->asr_config_enable_punc) {
            if (os_strcasecmp(cfg->asr_config_enable_punc, "true") == 0) {
                cJSON_AddTrueToObject(asr_config, "enable_punc");
            } else {
                cJSON_AddFalseToObject(asr_config, "enable_punc");
            }
        }
        if (cfg->asr_config_stream_mode) {
            cJSON_AddStringToObject(asr_config, "stream_mode", cfg->asr_config_stream_mode);
        }
        if (cfg->asr_config_enable_nostream) {
            if (os_strcasecmp(cfg->asr_config_enable_nostream, "true") == 0) {
                cJSON_AddTrueToObject(asr_config, "enable_nostream");
            } else {
                cJSON_AddFalseToObject(asr_config, "enable_nostream");
            }
        }
        if (cfg->asr_config_hot_words && *cfg->asr_config_hot_words) {
            cJSON *hotwords_array = cJSON_CreateArray();
            char **hotword_ptr = cfg->asr_config_hot_words;
            while (*hotword_ptr != NULL) {
                cJSON_AddItemToArray(hotwords_array, cJSON_CreateString(*hotword_ptr));
                hotword_ptr++;
            }
            cJSON_AddItemToObject(asr_config, "hot_words", hotwords_array);
        }
    }

    json_str_temp = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    *json_str = llm_strdup(json_str_temp);
    if (*json_str) {json_str_len = os_strlen(*json_str);}
    if (json_str_temp) { cJSON_free(json_str_temp); }
    return json_str_len;
}

static int32 coze_sts_create_audio_append_json(struct llm_session_sts *sts_session, struct llm_data *pdata, char **json_str)
{
    int ret = 0;
    int32 json_str_len = RET_ERR;
    char *json_str_temp = NULL;
    unsigned char *base64_data = NULL;
    size_t len = 0;

    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "id", "event_id");
    cJSON_AddStringToObject(root, "event_type", "input_audio_buffer.append");

    cJSON *data = cJSON_CreateObject();

    llm_dbg("Check encode data:%p,len:%d\n", pdata->buff1, pdata->buff1_len);

    ret = llm_base64_encode((const char *)pdata->buff1, pdata->buff1_len, (char **)&base64_data, &len, 0);
    if (ret != 0 || base64_data == NULL) {
        llm_err("base64 audio data failed,ret:%d,base64_data:%p,len:%d\n", ret, base64_data, len);
        if (base64_data) {
            llm_free(base64_data);
        }
        cJSON_Delete(root);
        return LLME_NOMEM;
    }

    llm_dbg("base64 audio data success,len:%d\n", len);
    cJSON_AddStringToObject(data, "delta", (char *)base64_data);
    cJSON_AddItemToObject(root, "data", data);

    json_str_temp = cJSON_PrintUnformatted(root);
    llm_free(base64_data);
    cJSON_Delete(root);
    *json_str = llm_strdup(json_str_temp);
    if (*json_str) {json_str_len = os_strlen(*json_str);}
    if (json_str_temp) { cJSON_free(json_str_temp); }
    return json_str_len;
}

static int32 coze_sts_create_tts_json(struct llm_session_sts *sts_session, struct llm_data *pdata, char **json_str)
{
    int32 json_str_len = RET_ERR;
    char *json_str_temp = NULL;
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "id", "event_id_stt");
    cJSON_AddStringToObject(root, "event_type", "input_text.generate_audio");

    cJSON *data = cJSON_CreateObject();

    cJSON_AddStringToObject(data, "mode", "text");
    cJSON_AddStringToObject(data, "text", pdata->buff1);

    cJSON_AddItemToObject(root, "data", data);

    json_str_temp = cJSON_PrintUnformatted(root);//cJSON_Print(root);

    cJSON_Delete(root);
    *json_str = llm_strdup(json_str_temp);
    if (*json_str) {json_str_len = os_strlen(*json_str);}
    if (json_str_temp) { cJSON_free(json_str_temp); }
    return json_str_len;
}

#define COZE_INPUT_AUDIO_BUFFER_COMPLETE "{\"id\":\"event_id\",\"event_type\":\"input_audio_buffer.complete\"}"
static int32 coze_sts_create_json(struct llm_session_sts *sts_session, struct llm_data *data, char **frame_str)
{
    int32 data_len = RET_ERR;
    struct coze_sts_private_params *private_params = (struct coze_sts_private_params *)sts_session->private;
    
    switch (data->type) {
        case LLM_DATA_TYPE_MGMT: {
            data_len = coze_sts_create_chat_update_json(sts_session, frame_str);
            break;
        }
        case LLM_DATA_TYPE_AUDIO: {
            if (data->buff1 && data->buff1_len > 0) {
                data_len = coze_sts_create_audio_append_json(sts_session, data, frame_str);
                if (data->transfer_state == LLM_DATA_STATE_END) {
                    if (!RB_FULL(&sts_session->sts_tx)) {
                        struct llm_data json_send = {
                            .llm_name = sts_session->base.name,
                            .transfer_state = LLM_DATA_STATE_END,
                            .buff1 = NULL,
                            .buff1_len = 1,
                            .offset = -1,
                            .type   = LLM_DATA_TYPE_AUDIO,
                        };
                        RB_INT_SET(&sts_session->sts_tx, json_send);
                    } else {
                        llm_err("Shouldn't be here!\n");
                        data_len = LLME_NOMEM;
                        break;
                    }
                }
            } else {
                if (data->transfer_state == LLM_DATA_STATE_END) {
                    *frame_str = llm_strdup(COZE_INPUT_AUDIO_BUFFER_COMPLETE);
					data_len = os_strlen(*frame_str);
                }
            }
            break;
        }
        case LLM_DATA_TYPE_TEXT: {
            llm_err("Not currently supported: LLM_DATA_TYPE_TEXT\r\n");
            if (data->buff1)
            { llm_free(data->buff1); }
            data->buff1 = NULL;
            data->buff1_len = 0;
            break;
        }
        case LLM_DATA_TYPE_TTS: {
            if (data->buff1 && data->buff1_len > 0) {
                data_len = coze_sts_create_tts_json(sts_session, data, frame_str);
                if (data_len > 0) {
                    private_params->tts_req = 1;
                }
            }
            break;
        }
        case LLM_DATA_TYPE_RAW: {
            if (data->buff1 && data->buff1_len > 0) {
                *frame_str = llm_strdup(data->buff1);
				data_len = os_strlen(*frame_str);
            }
            break;
        }
        default: {
            llm_err("Warning:Unknow file type:%d\n", data->type);
            if (data->buff1)
            { llm_free(data->buff1); }
            data->buff1 = NULL;
            data->buff1_len = 0;
            break;
        }
    }
    return data_len;
}

/**********************upload file*********************/
static void coze_upload_protect(struct coze_sts_private_params *llm)
{
    os_mutex_lock(&llm->upload_lock, osWaitForever);
}

static void coze_upload_unprotect(struct coze_sts_private_params *llm)
{
    os_mutex_unlock(&llm->upload_lock);
}

static size_t coze_upload_file_read_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
    struct llm_session_sts *sts_session = NULL;
    struct coze_sts_private_params *llm = NULL;
    struct upload_ctx *ctx = NULL;
    size_t max_bytes = size * nmemb;
    size_t xferred = 0;
    size_t this_chunk_remain = 0;
    size_t to_copy = 0;

    if (!ptr || !userp) {
        llm_err("Invalid pointer arguments\n");
        return 0;
    }

    sts_session = (struct llm_session_sts *)userp;
    if (!sts_session) {
        llm_err("Invalid sts_session\n");
        return 0;
    }

    llm = (struct coze_sts_private_params *)sts_session->private;
    if (!llm) {
        llm_err("Invalid private params\n");
        return 0;
    }

    ctx = &llm->ctx;
    coze_upload_protect(llm);
    if (list_empty(&ctx->chunks)) {
        llm_err("No chunks to upload\n");
        coze_upload_unprotect(llm);
        return 0;
    }

    if (!ctx->current) {
        ctx->current = list_first_entry_or_null(&ctx->chunks, struct upload_chunk, list);
        if (!ctx->current) {
            llm_err("Error,no current chunk!\n");
            coze_upload_unprotect(llm);
            return 0;
        }
    }

    while (ctx->current && xferred < max_bytes) {
        if (ctx->current->size == 0) {
            llm_err("Invalid chunk data or size\n");
            ctx->current = NULL;
            break;
        }
        this_chunk_remain = ctx->current->size - ctx->offset_in_chunk;
        to_copy = this_chunk_remain;
        if (to_copy > max_bytes - xferred)
        { to_copy = max_bytes - xferred; }

        memcpy((char *)ptr + xferred,
               ctx->current->data + ctx->offset_in_chunk,
               to_copy);

        ctx->offset_in_chunk += to_copy;
        xferred += to_copy;

        if (ctx->offset_in_chunk >= ctx->current->size) {
            struct list_head *next = ctx->current->list.next;
            if (next == &ctx->chunks) {
                ctx->current = NULL;
            } else {
                ctx->current = list_entry(next, struct upload_chunk, list);
            }
            ctx->offset_in_chunk = 0;
        }
    }
    coze_upload_unprotect(llm);
    return xferred;
}

static size_t coze_upload_file_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    struct llm_session_sts *sts_session = (struct llm_session_sts *)userp;
    size_t length = size * nmemb;
    llm_sts_event(sts_session, LLM_EVENT_UPLOAD_FILE_RESULT, (uint32)contents, length);
    return length;
}

static int coze_upload_server_connect(struct llm_session_sts *sts)
{
    struct coze_sts_private_params *llm  = NULL;
    struct coze_chat_platform_cfg *platform_cfg = NULL;
    uint16 auth_header_length = 0;
    int32 ret = 0;
    void *header = NULL;
    char *auth_header = NULL;
    void *upload_hdl  = NULL;
    int32 len = 0;

    if (!sts || !sts->base.platform_config || !sts->base.transfer_config) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    llm = (struct coze_sts_private_params *)sts->private;
    if (!llm) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    platform_cfg = sts->base.platform_config;
    auth_header_length = os_strlen("Authorization:Bearer ") + os_strlen(platform_cfg->pat_key) + 1;
    auth_header = (char *)llm_zalloc(auth_header_length);
    if (auth_header == NULL) {
        llm_err("Memory allocation failed\r\n");
        return LLME_NOMEM;
    }
    os_snprintf(auth_header, auth_header_length, "Authorization:Bearer %s", platform_cfg->pat_key);

    header = llm_build_header(header, auth_header);
    llm_free(auth_header);
    if (header == NULL) {
        ret = RET_ERR;
        goto __cleanup;
    }

    upload_hdl = llm_https_connect(header, sts->base.transfer_config,
                                   coze_upload_file_write_callback, NULL, NULL, sts);
    if (upload_hdl) {
        coze_upload_protect(llm);
        llm->upload_sk = upload_hdl;
        len = llm->ctx.tot_len;
        coze_upload_unprotect(llm);
        ret = llm_https_upload(upload_hdl, platform_cfg->update_url, NULL, len,
                               "file", "test.jpg",
                               coze_upload_file_read_callback, NULL, NULL, sts);
        if (ret) {
            llm_err("Update file failed:%d，free size:%d\n", ret, sysheap_freesize(&sram_heap));
        }
    }
__cleanup:
    if (llm->upload_sk) {
        llm_https_disconnect(llm->upload_sk);
        coze_upload_protect(llm);
        llm->upload_sk = NULL;
        coze_upload_unprotect(llm);
    }
    if (header) {
        llm_free_header(header);
    }
    return ret;
}

static void coze_upload_chunks_free(struct upload_ctx *ctx)
{
    struct upload_chunk *chunk, *tmp;

    if (ctx) {
        list_for_each_entry_safe(chunk, tmp, &ctx->chunks, list) {
            list_del(&chunk->list);
            llm_free(chunk);
        }
        ctx->current = NULL;
        ctx->offset_in_chunk = 0;
        ctx->tot_len = 0;
        INIT_LIST_HEAD(&ctx->chunks);
    }
}

static void coze_upload_task(void *arg)
{
    struct llm_session_sts *sts = (struct llm_session_sts *)arg;
    struct coze_sts_private_params *llm = (struct coze_sts_private_params *)sts->private;
    int running = 1;

    while (running != 0) {
        if (os_sema_down(&llm->upload_done, 1000) != 1) {
            coze_upload_protect(llm);
            running = llm->upload_task_running;  // 定期检查最新状态
            coze_upload_unprotect(llm);
            continue;
        }
        llm_dbg("Connect to upload server...\n");
        coze_upload_server_connect(sts);
        llm_dbg("Upload to server done\n");
        break;
    }
    llm_dbg("upload task exit...\n");
    coze_upload_protect(llm);
    coze_upload_chunks_free(&llm->ctx);
    llm->upload_task = NULL;
    llm->upload_task_running = 0;
    coze_upload_unprotect(llm);
}

static int coze_upload_init(struct llm_session_sts *sts, struct llm_data *data)
{
    struct coze_sts_private_params *llm = NULL;
    if (!sts || !sts->private || !data) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    llm = (struct coze_sts_private_params *)sts->private;

    coze_upload_protect(llm);
    if (llm->upload_task_running != 0) {
        llm_err("upload task is busy!please wait...\n");
        coze_upload_unprotect(llm);
        return RET_OK;
    }
    llm_dbg("Upload start,init upload context...\n");
    coze_upload_chunks_free(&llm->ctx);
    INIT_LIST_HEAD(&llm->ctx.chunks);
    llm->ctx.tot_len = data->buff1_len;
    llm->upload_task_running = 1;
    coze_upload_unprotect(llm);
    llm->upload_task = os_task_create("coze_upload", coze_upload_task,
                                      (void *)sts, OS_TASK_PRIORITY_NORMAL,
                                      0, NULL, 8192);
    if (!llm->upload_task) {
        llm_err("Error,create task failed!\n");
        llm->upload_task_running = 0;
        return RET_ERR;
    }
    return RET_OK;
}

static int coze_upload_data(struct llm_session_sts *sts, struct llm_data *data)
{
    struct coze_sts_private_params *llm = NULL;
    if (!sts || !sts->private || !data) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    if (!data->buff1 || data->buff1_len <= 0) {
        llm_err("Invalid chunk data or size: buff1=%p, len=%d\n", data->buff1, data->buff1_len);
        return RET_ERR;
    }
    llm = (struct coze_sts_private_params *)sts->private;

    struct upload_chunk *chunk = (struct upload_chunk *)llm_malloc(sizeof(struct upload_chunk) + data->buff1_len);
    if (!chunk) {
        llm_err("Failed to allocate upload chunk\n");
        return RET_ERR;
    }
    memset(chunk, 0, sizeof(struct upload_chunk));
    INIT_LIST_HEAD(&chunk->list);
    chunk->data = (unsigned char *)(chunk + 1);
    chunk->size = data->buff1_len;
    memcpy(chunk->data, data->buff1, data->buff1_len);
    coze_upload_protect(llm);
    list_add_tail(&chunk->list, &llm->ctx.chunks);
    coze_upload_unprotect(llm);
    return RET_OK;
}

static int coze_upload_data_done(struct llm_session_sts *sts, struct llm_data *data)
{
    struct coze_sts_private_params *llm = NULL;

    if (!sts || !sts->private || !data) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    llm = (struct coze_sts_private_params *)sts->private;
    os_sema_up(&llm->upload_done);
    llm_dbg("Upload file construction completed\n");
    return RET_OK;
}

static int32 coze_sts_send(void *session, struct llm_data *data)
{
    int32 ret = LLME_AGAIN;
    struct llm_session_sts *sts_session = NULL;
    char *json_str = NULL;
    int32 json_len = 0;

    llm_send_meta smeta = {
        .flags = LLMWS_TEXT,
    };

    if (!session || !data) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    sts_session = (struct llm_session_sts *)session;
    if (!sts_session->base.handle) {
        llm_err("Error!Session not connect!\n");
        return RET_ERR;
    }
    if (!sts_session->base.platform_config || !sts_session->base.transfer_config) {
        llm_err("Error!Platform or transfer not config!\n");
        return RET_ERR;
    }

    if (data->offset == -1) {
        if (data->type == LLM_DATA_TYPE_MGMT &&
            data->transfer_state == LLM_DATA_STATE_START &&
            sts_session->base.platform_config_change == 0) {
            llm_sts_event(sts_session, LLM_EVENT_WAITING_END, 0, 0);
            data->buff1_len = 0;
            return RET_OK;
        }
        json_len = coze_sts_create_json(sts_session, data, &json_str);
        if (json_str && json_len > 0) {
            if (data->buff1)
            { llm_free(data->buff1); }
            data->buff1 = json_str;
            data->buff1_len = json_len;
        } else {
            if (json_len < 0) {
                ret = json_len;
				llm_err("create_json fail!\n");
                return ret;
            }
        }
    } else {
        json_str = data->buff1 + data->offset;
        json_len = data->buff1_len - data->offset;
    }

    if (json_str == NULL || json_len <= 0) {
        llm_err("json_str is err!\n");
        return RET_ERR;
    }

    ret = llm_websocket_psend(sts_session->base.handle, json_str, json_len, &smeta);
    if (data->transfer_state == LLM_DATA_STATE_START ||
        data->transfer_state == LLM_DATA_STATE_END ||
        data->type == LLM_DATA_TYPE_RAW) {
        _os_printf("coze_sts_send len: (%d:%d)\n", json_len, smeta.sent);
        hgprintf_out(json_str, json_len, 0);
        _os_printf("\r\n");
    }

    llm_dbg("send data %d : %d!\n", json_len, smeta.sent);
    if (json_len != smeta.sent) {
        if (data->offset == -1) { data->offset = 0; }
        if (smeta.sent > 0) {
            data->offset += smeta.sent;
        }
    } else {
        if (data->buff1) {
            llm_free(data->buff1);
            data->buff1 = NULL;
        }
        data->buff1_len = 0;
        data->offset = -1;
    }
    llm_dbg("data status %d : %d!\n", data->offset, data->buff1_len);
    return ret;
}

/* -------------------------- recycle -------------------------- */
#define COZE_CONVERSATION_CHAT_CANCEL "{\"id\":\"event_id\",\"event_type\":\"conversation.chat.cancel\"}"
static int32 coze_sts_recycle(void *session, uint8 param)
{
    int32 ret = RET_OK;
    char *cancel_str = NULL;
    struct llm_session_sts *sts_session = NULL;
    struct coze_sts_private_params *private_params = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    sts_session = (struct llm_session_sts *)session;
    private_params = (struct coze_sts_private_params *)sts_session->private;
    if (private_params) {
        //coze_sts_ws_ctx_free(&private_params->ws_ctx);
        private_params->tts_req = 0;
    }

    if (param) {
        llm_err("Conversation interrupt!\n");
        cancel_str = llm_strdup(COZE_CONVERSATION_CHAT_CANCEL);
        if (!cancel_str) {
            llm_err("Error,no memory!\n");
            return RET_ERR;
        }
        if (!RB_FULL(&sts_session->sts_tx)) {
            struct llm_data json_send = {
                .llm_name = sts_session->base.name,
                .transfer_state = LLM_DATA_STATE_MIDDLE,
                .buff1 = cancel_str,
                .buff1_len = os_strlen(cancel_str),
                .offset = -1,
                .type   = LLM_DATA_TYPE_RAW,
            };
            RB_INT_SET(&sts_session->sts_tx, json_send);
            return RET_OK;
        } else {
            llm_err("Shouldn't be here!\n");
            llm_free(cancel_str);
            return RET_ERR;
        }
    } else {
        llm_sts_event(sts_session, LLM_EVENT_INTERRUPT_END, 0, 0);
    }
    return ret;
}

static int32 coze_sts_upload(void *session, struct llm_data *data)
{
    struct llm_session_sts *sts = (struct llm_session_sts *)session;
    struct coze_sts_private_params *llm = NULL;

    if (!sts || !sts->private || !data) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    if (data->type != LLM_DATA_TYPE_FILE) {
        llm_err("Invaild data type:%d!\n", data->type);
        return -RET_ERR;
    }
    llm = (struct coze_sts_private_params *)sts->private;
    if (data->transfer_state == LLM_DATA_STATE_START) {
        return coze_upload_init(sts, data);
    } else if (data->transfer_state == LLM_DATA_STATE_MIDDLE) {
        return coze_upload_data(sts, data);
    } else if (data->transfer_state == LLM_DATA_STATE_END) {
        return coze_upload_data_done(sts, data);
    } else {
        llm_err("Unknown transfer state:%d\n", data->transfer_state);
        return RET_ERR;
    }
}

static int32 coze_sts_init(void *session)
{
    struct llm_session_sts *sts_session = NULL;
    struct coze_sts_private_params  *coze_sts = NULL;
    int32 ret = RET_OK;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    sts_session = (struct llm_session_sts *)session;

    coze_sts = llm_zalloc(sizeof(struct coze_sts_private_params));
    if (!coze_sts) { //sts_session->private
        llm_err("Error,no memory!\n");
        ret = LLME_NOMEM;
        goto __failed;
    }
    os_sema_init(&coze_sts->upload_done, 0);
    os_mutex_init(&coze_sts->upload_lock);
    INIT_LIST_HEAD(&coze_sts->ctx.chunks);
    sts_session->private  = coze_sts;
    coze_sts->session_sts = sts_session;
    return RET_OK;

__failed:
    if (coze_sts) {
        os_sema_del(&coze_sts->upload_done);
        os_mutex_del(&coze_sts->upload_lock);
        llm_free(coze_sts);
    }
    sts_session->private = NULL;
    return ret;
}

static int32 coze_sts_deinit(void *session)
{
    struct llm_session_sts *sts_session = NULL;
    struct coze_sts_private_params *private_params = NULL;
    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    sts_session = (struct llm_session_sts *)session;

    private_params = (struct coze_sts_private_params *)sts_session->private;
    if (private_params) {
        coze_sts_ws_ctx_free(&private_params->ws_ctx);
        if (private_params->upload_task) {
            coze_upload_protect(private_params);
            private_params->upload_task_running = 0;
            coze_upload_unprotect(private_params);
            os_sema_up(&private_params->upload_done);
            os_sleep_ms(200);
        }
        if (private_params->upload_sk) {
            llm_https_disconnect(private_params->upload_sk);
        }
        coze_upload_protect(private_params);
        coze_upload_chunks_free(&private_params->ctx);
        private_params->upload_task = NULL;
        private_params->upload_sk   = NULL;
        coze_upload_unprotect(private_params);
        os_sema_del(&private_params->upload_done);
        os_mutex_del(&private_params->upload_lock);
        llm_free(private_params);
    }
    sts_session->private = NULL;
    return RET_OK;
}

static int32 coze_sts_connect(void *session)
{
    int32 ret = RET_OK;
    struct llm_session_sts *sts_session = NULL;
    struct coze_chat_platform_cfg *platform_cfg = NULL;
    uint16 auth_header_length = 0;
    uint32 url_len = 0;
    uint32 tick = 0;
    void *header = NULL;
    char *chat_url = NULL;
    char *auth_header = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    sts_session = (struct llm_session_sts *)session;

    if (!sts_session->base.platform_config || !sts_session->base.transfer_config) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }

    platform_cfg = sts_session->base.platform_config;

    auth_header_length = os_snprintf(NULL, 0, "Authorization:Bearer %s", platform_cfg->pat_key);
    auth_header = (char *)llm_zalloc(auth_header_length + 1);
    if (auth_header == NULL) {
        llm_err("auth_header alloc failed\r\n");
        ret = LLME_NOMEM;
        goto __cleanup;
    }
    os_snprintf(auth_header, auth_header_length + 1, "Authorization:Bearer %s", platform_cfg->pat_key);

    header = llm_build_header(header, auth_header);
    if (header == NULL) {
        llm_err("header alloc failed\r\n");
        ret = RET_ERR;
        goto __cleanup;
    }

    url_len = os_snprintf(NULL, 0, "%s%s", platform_cfg->host_url, platform_cfg->bot_id);
    chat_url = (char *)llm_zalloc(url_len + 1);
    if (chat_url == NULL) {
        llm_err("chat_url alloc failed\r\n");
        ret = LLME_NOMEM;
        goto __cleanup;
    }
    os_snprintf(chat_url, url_len + 1, "%s%s", platform_cfg->host_url, platform_cfg->bot_id);

    tick = os_jiffies();
    sts_session->base.handle = llm_websocket_connect(chat_url, header, sts_session->base.transfer_config);
    if (!sts_session->base.handle) {
        llm_err("Creat websocket failed!\n");
        ret = RET_ERR;
        goto __cleanup;
    }
    os_printf("llm websocket connect to %s (%dms) success\r\n",
              chat_url, os_jiffies_to_msecs(os_jiffies() - tick));
    llm_free(auth_header);
    llm_free(chat_url);
    llm_free_header(header);
    llm_sts_event(sts_session, LLM_EVENT_CONNECTED, 0, 0);
    return ret;

__cleanup:
    if (auth_header) { llm_free(auth_header); }
    if (chat_url) { llm_free(chat_url); }
    if (header) { llm_free_header(header); }
    if (sts_session->base.handle) {
        llm_websocket_disconnect(sts_session->base.handle);
        sts_session->base.handle = NULL;
    }
    return ret;
}

static int32 coze_sts_disconnect(void *session)
{
    struct llm_session_sts *sts_session = NULL;
    struct coze_sts_private_params *private_params = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    sts_session = (struct llm_session_sts *)session;
    if (sts_session->base.unsend.buff1) {
        llm_free(sts_session->base.unsend.buff1);
        sts_session->base.unsend.buff1 = NULL;
    }
    sts_session->base.unsend.buff1_len = 0;
    if (sts_session->base.handle) {
        llm_websocket_disconnect(sts_session->base.handle);
        sts_session->base.handle = NULL;
    }
    private_params = (struct coze_sts_private_params *)sts_session->private;
    if (private_params) {
        private_params->tts_req = 0;
        coze_sts_ws_ctx_free(&private_params->ws_ctx);
        coze_upload_protect(private_params);
        coze_upload_chunks_free(&private_params->ctx);
        coze_upload_unprotect(private_params);
    }
    llm_sts_event(sts_session, LLM_EVENT_DISCONNECT, 0, 0);
    return RET_OK;
}

const struct llm_model_data coze_sts_model = {
    .headsize   = 0,
    .init       = coze_sts_init,
    .deinit     = coze_sts_deinit,
    .connect    = coze_sts_connect,
    .disconnect = coze_sts_disconnect,
    .recycle    = coze_sts_recycle,
    .upload     = coze_sts_upload,
    .send       = coze_sts_send,
    .recv       = coze_sts_recv,
};

