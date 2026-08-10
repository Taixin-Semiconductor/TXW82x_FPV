#include "llm.h"
#include "cJSON.h"

uint64 chat_used_times = 0;

struct doubao_llm_image_info {
    char    *image;
    uint32  image_len;
};

typedef struct {
    char *buffer;       // 累积缓冲区
    char last_char;
    uint32 total_size;  // 已累积数据大小
    
    char *parsedData_buff;
    uint32 parsedData_buff_len;
} doubao_llm_context;

struct doubao_llm_private_params {
    void *chat_session;
    uint32 is_stream: 1, rev: 31;
    RBUFFER_DEF_R(image_rb, struct doubao_llm_image_info);
    doubao_llm_context ctx;
};

#define DOUBAO_LLM_CACHE_BUF_SIZE 1024

static void doubao_llm_private_params_free(struct doubao_llm_private_params *private_params)
{
    if (private_params == NULL) { return; }
    if (private_params->ctx.parsedData_buff) {
        llm_free(private_params->ctx.parsedData_buff);
        private_params->ctx.parsedData_buff = NULL;
    }
    private_params->ctx.parsedData_buff_len = 0;
    if (private_params->ctx.buffer) {
        llm_free(private_params->ctx.buffer);
        private_params->ctx.buffer = NULL;
    }
    private_params->ctx.total_size = 0;
}

// 处理完整的文本
static int32 doubao_llm_process_complete(struct llm_session_chat *chat_session, const char *text)
{
    uint8 untreated = 1;
    char *text_tx_buffer = NULL;
    uint32 text_len = os_strlen(text);
    if (text_len <= 0) {
        return RET_OK;
    }
    do {
        if (llm_chat_get_state(chat_session) == LLM_CHAT_STATE_INTERRUPTING) {
            return LLME_INTR;
        }
        if (!RB_FULL(&chat_session->mq_rx)) {
            text_tx_buffer = llm_malloc(text_len + 1);
            if (text_tx_buffer == NULL) {
                llm_err("no memory!\r\n");
                return RET_ERR;
            }
            os_memcpy(text_tx_buffer, text, text_len);
            text_tx_buffer[text_len] = '\0';
            struct llm_data chat_recv = {
                .llm_name = chat_session->base.name,
                .buff1 = text_tx_buffer,
                .buff1_len = text_len,
            };
            os_printf("CHAT(%dms)\r\n", os_jiffies_to_msecs(os_jiffies() - chat_used_times));
            //dump_hex("", text_tx_buffer, os_strlen(text_tx_buffer)+1, 1);
            RB_INT_SET(&chat_session->mq_rx, chat_recv);
            untreated = 0;
        } else {
            os_sleep_ms(10);
        }
    } while (untreated);
    return RET_OK;
}

// 将内容添加到缓冲区，并在合适的时候处理完整的句子
static int32 doubao_llm_append_to_buffer(struct llm_session_chat *chat_session, const char *content)
{
    int32 ret = RET_OK;
    struct doubao_llm_private_params *private_params = (struct doubao_llm_private_params *)chat_session->private;

    if (private_params->ctx.parsedData_buff == NULL) {
        private_params->ctx.parsedData_buff = llm_malloc(chat_session->sparam->reply_fragment_size + 1);
        if (private_params->ctx.parsedData_buff == NULL) {
            llm_err("no memory!\r\n");
            return RET_ERR;
        }
        private_params->ctx.parsedData_buff_len = 0;
    }

    if (content == NULL) {
        private_params->ctx.parsedData_buff_len = 0;
        return doubao_llm_process_complete(chat_session, private_params->ctx.parsedData_buff);
    }

    int content_len = os_strlen(content);
    if (content_len <= 0) {
        return RET_OK;
    }
    // 如果加上新内容后超过最大长度，则处理当前缓冲区的内容
    if (private_params->ctx.parsedData_buff_len + content_len > chat_session->sparam->reply_fragment_size) {
        // 找到最后一个完整的中文句子
        int last_sentence_end = private_params->ctx.parsedData_buff_len - 1;
        // 从 end 向前寻找句子结束符
        while (last_sentence_end >= 0) {
            /*// 先回退到当前UTF-8字符的起始位置
            uint8 char_len = llm_get_utf8_char_len((unsigned char)private_params->ctx.parsedData_buff[last_sentence_end]);
            int char_start = last_sentence_end - (char_len - 1);
            if (char_start < 0) char_start = 0;

            // 提取完整的UTF-8字符判断是否为句子结束符
            char current_char[5] = {0}; // UTF-8最多4字节 + '\0'
            os_strncpy(current_char, private_params->ctx.parsedData_buff + char_start, char_len);
            
            if (llm_is_sentence_end(current_char)) {
                last_sentence_end = char_start + char_len - 1; // 定位到字符末尾
                break;
            }

            // 回退到上一个字符的末尾
            last_sentence_end = char_start - 1;*/
            // 检查当前字符是否为句子结束符
            char current_char[4] = {0}; // 中文字符最多占用 3 个字节 + 1 个 '\0'
            if ((unsigned char)private_params->ctx.parsedData_buff[last_sentence_end] < 128) {
                // ASCII 字符，只取 1 个字节
                current_char[0] = private_params->ctx.parsedData_buff[last_sentence_end];
            } else {
                // 多字节字符，尝试取 3 个字节
                int len = (last_sentence_end >= 2) ? 3 : last_sentence_end + 1;
                os_strncpy(current_char, private_params->ctx.parsedData_buff + (last_sentence_end - len + 1), len);
            }
            if (llm_is_sentence_end(current_char)) {
                break;
            }
            last_sentence_end--;
        }

        if (last_sentence_end >= 0) {
            llm_dbg("last_sentence_end = %d\r\n", last_sentence_end);
            llm_dbg("parsedData_buff(%d): %s\r\n", private_params->ctx.parsedData_buff_len, private_params->ctx.parsedData_buff);
            uint32 complete_len = last_sentence_end + 1;
            if (complete_len <= chat_session->sparam->reply_fragment_size) {
                // 提取完整的句子
                char *complete_sentence = llm_malloc(complete_len+1);
                if (complete_sentence) {
                    os_memcpy(complete_sentence, private_params->ctx.parsedData_buff, complete_len);
                    complete_sentence[complete_len] = '\0';
                    // 处理完整句子
                    ret = doubao_llm_process_complete(chat_session, complete_sentence);
                    llm_free(complete_sentence);
                } else {
                    llm_err("no memory!\r\n");
                    return RET_ERR;
                }
            }
            // 将剩余的内容移动到缓冲区开头
            uint32 remaining_len = private_params->ctx.parsedData_buff_len - complete_len;
            if (remaining_len > 0 && remaining_len <= chat_session->sparam->reply_fragment_size) {
                os_memmove(private_params->ctx.parsedData_buff, 
                          private_params->ctx.parsedData_buff + complete_len, 
                          remaining_len);
                private_params->ctx.parsedData_buff_len = remaining_len;
                private_params->ctx.parsedData_buff[remaining_len] = '\0'; // 确保终止符
                llm_dbg("remaining(%d): %s\r\n", private_params->ctx.parsedData_buff_len, private_params->ctx.parsedData_buff);
            } else {
                llm_err("remaining_len = %d\r\n", remaining_len);
                private_params->ctx.parsedData_buff_len = 0;
            }
        } else {
            // 如果没有找到完整的句子，直接处理整个缓冲区
            private_params->ctx.parsedData_buff[private_params->ctx.parsedData_buff_len] = '\0';
            ret = doubao_llm_process_complete(chat_session, private_params->ctx.parsedData_buff);
            private_params->ctx.parsedData_buff_len = 0;
            llm_err("%s\r\n", private_params->ctx.parsedData_buff);
        }
    }

    // 将新内容添加到缓冲区
    os_strncpy(private_params->ctx.parsedData_buff + private_params->ctx.parsedData_buff_len, content, content_len);
    private_params->ctx.parsedData_buff_len += content_len;
    private_params->ctx.parsedData_buff[private_params->ctx.parsedData_buff_len] = '\0';
    llm_dbg("parsedData_buff(%d): %s\r\n", private_params->ctx.parsedData_buff_len, private_params->ctx.parsedData_buff);
    return ret;
}

// 处理JSON字符串中的转义字符
static int32 doubao_llm_handle_escapes(const char *in, char *out, uint32 out_max_len)
{
    uint32 in_pos = 0;
    uint32 out_pos = 0;

    while (in[in_pos] != '\0' && out_pos < out_max_len - 1) {
        if (in[in_pos] == '\\') {
            in_pos++;
            switch (in[in_pos]) {
                case 'n':  // 转义换行符
                    out[out_pos++] = '\n';
                    break;
                case 't':  // 转义制表符
                    out[out_pos++] = '\t';
                    break;
                case '\\': // 转义反斜杠
                    out[out_pos++] = '\\';
                    break;
                case '"':  // 转义双引号
                    out[out_pos++] = '"';
                    break;
                default:   // 其他转义符，原样保留
                    out[out_pos++] = in[in_pos];
                    break;
            }
            in_pos++;
        } else {
            out[out_pos++] = in[in_pos++];
        }
    }

    out[out_pos] = '\0'; // 字符串结束符
    return out_pos;
}

static int32 doubao_llm_extract_content(struct llm_session_chat *chat_session, const char *json_str)
{
    char content_raw[20] = {0};
    char content_unescaped[20] = {0};

    if (!json_str) { return RET_ERR; }

    // 1. 定位"content":"起始位置
    const char *content_start = os_strstr(json_str, "\"content\":\"");
    if (!content_start) { 
        llm_err("not found: content!\n");
        return RET_ERR; 
    }
    content_start += os_strlen("\"content\":\""); // 跳过"content":"

    // 2. 定位content值的结束位置（下一个未转义的"）
    const char *content_end = content_start;
    int in_escape = 0;
    while (*content_end != '\0') {
        if (in_escape) {
            in_escape = 0;
            content_end++;
            continue;
        }
        if (*content_end == '\\') { // 遇到转义符，标记并跳过
            in_escape = 1;
            content_end++;
            continue;
        }
        if (*content_end == '"') { // 找到未转义的结束双引号
            break;
        }
        content_end++;
    }
    if (content_end == content_start) { return RET_OK; } // 空content

    // 3. 提取content原始内容（不包含结束引号）
    uint32 content_raw_len = content_end - content_start;
    os_strncpy(content_raw, content_start, content_raw_len);
    content_raw[content_raw_len] = '\0';

    // 4. 处理转义字符（如\\n转成\n）
	doubao_llm_handle_escapes(content_raw, content_unescaped, 20);
    llm_dbg("%s\r\n", content_unescaped);
    return doubao_llm_append_to_buffer(chat_session, content_unescaped);
}

static int32 doubao_llm_process_data_blocks(struct llm_session_chat *chat_session, char *line)
{
    const char *data_prefix = "data: ";
    uint32 prefix_len = os_strlen(data_prefix);

    // 仅处理data: 开头的行
    if (os_strncmp(line, data_prefix, prefix_len) != 0) { 
        llm_err("Does not comply with the SSE protocol!\n");
        return RET_ERR;
    }
    const char *data = line + prefix_len;

    // 处理流结束标记
    if (os_strstr(data, "[DONE]")) {
        return doubao_llm_append_to_buffer(chat_session, NULL);
    }

    // 提取并处理content（含转义字符）
    return doubao_llm_extract_content(chat_session, data);
}

static size_t doubao_llm_write_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
    int32 ret = RET_OK;
    size_t length = size * nmemb;
    struct llm_session_chat *chat_session = NULL;
    struct doubao_llm_private_params *private_params = NULL;
    doubao_llm_context *ctx = NULL;
    char *contents = NULL;

    if (ptr == NULL || length <= 0 || userp == NULL) {
        return 0;
    }
    contents = (char *)ptr;
    chat_session = (struct llm_session_chat *)userp;
    private_params = (struct doubao_llm_private_params *)chat_session->private;
    if (private_params == NULL) {
        llm_err("Input param error!\n");
        return 0;
    }
    ctx = &private_params->ctx;

    // 服务器 err msg
    // 即是是流式接收，也可能返回 err，只能靠 SSE 前 5 个固定字符判断并退出流式接收的流程
    if (os_strncasestr(contents, "\"error\":\"", -1) ||
        (chat_session->base.new_dialogue == 1 &&
         private_params->is_stream == 1 &&
         os_strncasecmp(contents, "data:", 5))) {
        private_params->is_stream = 0;
    }
    chat_session->base.new_dialogue = (chat_session->base.new_dialogue == 1) ? 0 : 1;
//     os_printf("contents: %d\r\n", length);
//    hgprintf_out(contents, length, 0);
//    _os_printf("\r\n");

    if (private_params->is_stream) {
        if (ctx->buffer == NULL) {
            ctx->buffer = llm_malloc(DOUBAO_LLM_CACHE_BUF_SIZE);
            if (ctx->buffer == NULL) {
                llm_err("no memory!\r\n");
                return 0;
            }
            ctx->total_size = 0;
            ctx->last_char = '\0';
        }
        for (int i = 0; i < length; i++) {
//            _os_printf("%c", contents[i]);
            if (contents[i] == '\n') {
                if (ctx->last_char == '\n') {
                    ctx->buffer[ctx->total_size] = '\0';
//                    os_printf("ctx->buffer: %d\r\n", os_strlen(ctx->buffer));
//                    hgprintf_out(ctx->buffer, os_strlen(ctx->buffer), 0);
//                    _os_printf("\r\n");
                    ret = doubao_llm_process_data_blocks(chat_session, ctx->buffer);
                    if (ret) {
                        if (ret == LLME_INTR) {
                            break;
                        }
                        return 0;
                    } else {
                        os_memset(ctx->buffer, 0, DOUBAO_LLM_CACHE_BUF_SIZE);
                        ctx->total_size = 0;
                    }
                }
            } else if (ctx->total_size < DOUBAO_LLM_CACHE_BUF_SIZE - 1) {
                ctx->buffer[ctx->total_size++] = contents[i];
            } else {
                llm_err("cache full(%d)!\n", ctx->total_size);
                return 0;
            }
            ctx->last_char = contents[i];
        }
        _os_printf("\r\n");
    } else {
        char *new_buffer = llm_realloc(private_params->ctx.buffer, private_params->ctx.total_size + length + 1);
        if (new_buffer == NULL) {
            llm_err("no memory!\r\n");
            return 0;
        }

        os_memcpy(new_buffer + private_params->ctx.total_size, contents, length);
        private_params->ctx.buffer = new_buffer;
        private_params->ctx.total_size += length;
        private_params->ctx.buffer[private_params->ctx.total_size] = '\0';
        llm_dbg("recv: %d!\r\n", length);
    }
    return length;
}

static int doubao_llm_xferinfo_callback(void *clientp,
        long dltotal,
        long dlnow,
        long ultotal,
        long ulnow)
{

    struct llm_session_chat *chat_session = (struct llm_session_chat *)clientp;

    if (llm_chat_get_state(chat_session) == LLM_CHAT_STATE_INTERRUPTING) {
        return 1;
    }
    return 0x10000001; // 继续传输
}

static int32 doubao_llm_errmsg_check(struct llm_session_chat *chat_session, char *buff, uint32 buff_len)
{
    if (buff && buff_len > 0) {
        if (os_strncasestr(buff, "\"error\":\"", -1)) {
            llm_chat_event(chat_session, LLM_EVENT_ERROR_MSG, (uint32)buff, buff_len);
            return RET_OK;
        }
    }
    return RET_ERR;
}

static int32 doubao_llm_recv(void *session, char *buff, uint32 buffer_size)
{
    int32 ret = LLME_AGAIN;
    struct llm_session_chat *chat_session = NULL;
    struct doubao_llm_private_params *private_params = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    chat_session = (struct llm_session_chat *)session;
    private_params = (struct doubao_llm_private_params *)chat_session->private;

    ret = llm_https_recv(chat_session->base.handle, NULL, 0, NULL);

    if (private_params->is_stream) {
        return RET_OK;
    }

    if (private_params->ctx.buffer && private_params->ctx.total_size > 0) {
        llm_err("recv done(%d)!\r\n", private_params->ctx.total_size);
//        hgprintf_out(private_params->ctx.buffer, private_params->ctx.total_size, 0);
//        _os_printf("\r\n");
        ret = doubao_llm_errmsg_check(chat_session, private_params->ctx.buffer, private_params->ctx.total_size);
        if (ret != RET_OK) {
            llm_chat_event(chat_session, LLM_EVENT_CHAT_RESULT, (uint32)private_params->ctx.buffer, private_params->ctx.total_size);
            ret = RET_OK;
        }
    }
    return ret;
}

static uint32 doubao_llm_build_body(struct llm_session_chat *chat_session, struct llm_data *ll_file, char **body)
{
    struct Doubao_LLM_Cfg *chat_cfg = (struct Doubao_LLM_Cfg *)chat_session->base.platform_config;
    struct doubao_llm_private_params *private_params = (struct doubao_llm_private_params *)chat_session->private;
    struct doubao_llm_image_info image_info = {0};
    char *question = ll_file->buff1;
    uint32 json_str_len = 0;
    char *json_str_temp = NULL;

    // 创建 cJSON 对象
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        llm_err("Failed to create cJSON object\r\n");
        return 0;
    }

    // 设置 model 字段
    cJSON_AddStringToObject(root, "model", chat_cfg->endpoint_id);

    // 设置 messages 字段
    cJSON *messages = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "messages", messages);

    if (chat_session->role) {
        cJSON *system_message = cJSON_CreateObject();
        cJSON_AddStringToObject(system_message, "role", "system");
        cJSON_AddStringToObject(system_message, "content", chat_session->role);
        cJSON_AddItemToArray(messages, system_message);
    }

    cJSON *user_message = cJSON_CreateObject();
    cJSON_AddStringToObject(user_message, "role", "user");
    if (private_params && !RB_EMPTY(&private_params->image_rb)) {
        // 设置 content 字段
        cJSON *content = cJSON_CreateArray();
        // 设置 image_url 字段
        while (RB_INT_GET(&private_params->image_rb, image_info)) {
            if (image_info.image) {
                cJSON *image_obj = cJSON_CreateObject();
                cJSON_AddStringToObject(image_obj, "type", "image_url");
                cJSON *image_url = cJSON_CreateObject();
                cJSON_AddStringToObject(image_url, "url", image_info.image);
                cJSON_AddItemToObject(image_obj, "image_url", image_url);
                cJSON_AddItemToArray(content, image_obj);
                llm_free(image_info.image);
            } else {
                if (image_info.image) { llm_free(image_info.image); }
                llm_err("Invalid parameter\r\n");
            }
        }
        // 设置 text 字段
        cJSON *text_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(text_obj, "type", "text");
        cJSON_AddStringToObject(text_obj, "text", question);
        cJSON_AddItemToArray(content, text_obj);
        cJSON_AddItemToObject(user_message, "content", content);
    } else {
        cJSON_AddStringToObject(user_message, "content", question);
    }
    cJSON_AddItemToArray(messages, user_message);

    // 设置 thinking 字段
    if (chat_cfg->thinking) {
        cJSON *thinking = cJSON_CreateObject();
        cJSON_AddItemToObject(root, "thinking", thinking);
        cJSON_AddStringToObject(thinking, "type", chat_cfg->thinking);
    }

    // 设置 stream 字段
    if (chat_cfg->stream) {
        private_params->is_stream = 1;
        cJSON_AddBoolToObject(root, "stream", os_strcmp(chat_cfg->stream, "true") == 0 ? true : false);
    }

    // 设置 max_tokens 字段
    if (chat_cfg->max_tokens) {
        cJSON_AddNumberToObject(root, "max_tokens", os_atoi(chat_cfg->max_tokens));
    }

    // 设置 max_completion_tokens 字段
    if (chat_cfg->max_completion_tokens) {
        cJSON_AddNumberToObject(root, "max_completion_tokens", os_atoi(chat_cfg->max_completion_tokens));
    }

    // 设置 service_tier 字段
    if (chat_cfg->service_tier) {
        cJSON_AddStringToObject(root, "service_tier", chat_cfg->service_tier);
    }

    // 设置 stop 字段
    if (chat_cfg->stop) {
        cJSON_AddStringToObject(root, "stop", chat_cfg->stop);
    }

    // 设置 reasoning_effort   字段
    if (chat_cfg->reasoning_effort) {
        cJSON_AddStringToObject(root, "reasoning_effort", chat_cfg->reasoning_effort);
    }

    // 设置 response_format 字段
    if (chat_cfg->response_format) {
        cJSON *response_format = cJSON_CreateObject();
        cJSON_AddItemToObject(root, "response_format", response_format);
        cJSON_AddStringToObject(response_format, "type", chat_cfg->response_format);
    }

    // 设置 frequency_penalty 字段
    if (chat_cfg->frequency_penalty) {
        cJSON_AddNumberToObject(root, "frequency_penalty", os_atoi(chat_cfg->frequency_penalty));
    }

    // 设置 presence_penalty 字段
    if (chat_cfg->presence_penalty) {
        cJSON_AddNumberToObject(root, "presence_penalty", os_atoi(chat_cfg->presence_penalty));
    }

    // 设置 temperature 字段
    if (chat_cfg->temperature) {
        cJSON_AddNumberToObject(root, "temperature", os_atoi(chat_cfg->temperature));
    }

    // 设置 top_p 字段
    if (chat_cfg->top_p) {
        cJSON_AddNumberToObject(root, "top_p", os_atoi(chat_cfg->top_p));
    }

    json_str_temp = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json_str_temp) {
        json_str_len = os_strlen(json_str_temp);
        *body = llm_strdup(json_str_temp);
        cJSON_free(json_str_temp);
    } else {
        json_str_len = 0;
    }

    return json_str_len;
}

static int32 doubao_llm_send(void *session, struct llm_data *data)
{
    int32 ret = LLME_AGAIN;
    struct llm_session_chat *chat_session = NULL;
    struct Doubao_LLM_Cfg *llm_chat_cfg = NULL;
    char *json_str = NULL;
    uint32 json_len = 0;

    if (!session || !data) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    chat_session = (struct llm_session_chat *)session;
    if (!chat_session->base.handle) {
        llm_err("Error!Session not connect!\n");
        return RET_ERR;
    }
    if (!chat_session->base.platform_config || !chat_session->base.transfer_config) {
        llm_err("Error!Platform or transfer not config!\n");
        return RET_ERR;
    }
    llm_chat_cfg = (struct Doubao_LLM_Cfg *)chat_session->base.platform_config;

    if (data->type == LLM_DATA_TYPE_TEXT && data->buff1 && data->buff1_len > 0) {
        json_len = doubao_llm_build_body(chat_session, data, &json_str);
        if (json_str && json_len) {
            _os_printf("doubao_llm_send len: (%d)\n", json_len);
            hgprintf_out(json_str, json_len, 0);
            _os_printf("\r\n");
            chat_used_times = os_jiffies();
            chat_session->base.new_dialogue = 1;
            ret = llm_https_send(chat_session->base.handle, LLM_HTTP_POST, llm_chat_cfg->url, json_str, json_len);
            llm_free(json_str);
        }
    } else {
        llm_err("There is a problem with the data, please check!\n");
        ret = RET_ERR;
    }
    if (data->buff1) { llm_free(data->buff1); }
    data->buff1 = NULL;
    data->buff1_len = 0;
    return ret;
}

static int32 doubao_llm_recycle(void *session, uint8 param)
{
    struct llm_session_chat *chat_session = NULL;
    struct doubao_llm_private_params *private_params = NULL;
    struct doubao_llm_image_info image_info = {0};

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    chat_session = (struct llm_session_chat *)session;
    private_params = (struct doubao_llm_private_params *)chat_session->private;
    if (private_params) {
        private_params->is_stream = 0;
        while (RB_INT_GET(&private_params->image_rb, image_info)) {
            if (image_info.image) { llm_free(image_info.image); }
        }
        RB_RESET(&private_params->image_rb);
        doubao_llm_private_params_free(private_params);
    }
    return RET_OK;
}

static const char *doubao_llm_get_image_type_str(llm_image_type type)
{
    switch (type) {
        case LLM_IMAGE_TYPE_JPEG:
            return "image/jpeg";
        case LLM_IMAGE_TYPE_PNG:
            return "image/png";
        case LLM_IMAGE_TYPE_GIF:
            return "image/gif";
        case LLM_IMAGE_TYPE_WEBP:
            return "image/webp";
        case LLM_IMAGE_TYPE_BMP:
            return "image/bmp";
        case LLM_IMAGE_TYPE_TIFF:
            return "image/tiff";
        case LLM_IMAGE_TYPE_ICO:
            return "image/ico";
        case LLM_IMAGE_TYPE_DIB:
            return "image/bmp";
        case LLM_IMAGE_TYPE_ICNS:
            return "image/icns";
        case LLM_IMAGE_TYPE_SGI:
            return "image/sgi";
        case LLM_IMAGE_TYPE_JPEG2000:
            return "image/jp2";
        case LLM_IMAGE_TYPE_HEIC:
            return "image/heic";
        case LLM_IMAGE_TYPE_HEIF:
            return "image/heif";
        default:
            return NULL;
    }
    return NULL;
}

static int32 doubao_llm_upload(void *session, struct llm_data *data)
{
    int32 ret = RET_OK;
    struct llm_session_chat *chat_session = NULL;
    struct doubao_llm_private_params *private_params = NULL;
    char *image_url = NULL;
    uint32 image_url_len = 0;
    char *base64_data = NULL;
    size_t base64_data_len = 0;

    if (!session || !data || !data->buff1 || data->buff1_len <= 0) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    chat_session = (struct llm_session_chat *)session;
    private_params = (struct doubao_llm_private_params *)chat_session->private;

    ret = llm_base64_encode((const char *)data->buff1, data->buff1_len, &base64_data, &base64_data_len, 0);
    if (ret != 0 || base64_data == NULL) {
        llm_err("base64 image data failed,ret:%d,base64_data:%p,len:%d\n", ret, base64_data, base64_data_len);
        goto cleanup;
    }

    image_url_len = os_snprintf(NULL, 0, "data:%s;base64,%s", doubao_llm_get_image_type_str(data->sub_type), base64_data);
    image_url = (char *)llm_malloc(image_url_len + 1);
    if (image_url == NULL) {
        llm_err("Memory allocation failed\r\n");
        ret = LLME_NOMEM;
        goto cleanup;
    }
    os_snprintf(image_url, image_url_len + 1, "data:%s;base64,%s", doubao_llm_get_image_type_str(data->sub_type), base64_data);

    if (!RB_FULL(&private_params->image_rb)) {
        struct doubao_llm_image_info image_info = {
            .image      = image_url,
            .image_len  = image_url_len,
        };
        RB_INT_SET(&private_params->image_rb, image_info);
        if (base64_data) { llm_free(base64_data); }
        return RET_OK;
    } else {
        ret = LLME_AGAIN;
    }

cleanup:
    if (base64_data) { llm_free(base64_data); }
    if (image_url) { llm_free(image_url); }
    return ret;
}

static int32 doubao_llm_connect(void *session)
{
    int32 ret = RET_OK;
    struct llm_session_chat *chat_session = NULL;
    struct Doubao_LLM_Cfg *llm_chat_cfg = NULL;
    uint16 auth_header_length = 0;
    char *auth_header = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    chat_session = (struct llm_session_chat *)session;

    if (!chat_session->base.platform_config || !chat_session->base.transfer_config) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }

    llm_chat_cfg = (struct Doubao_LLM_Cfg *)chat_session->base.platform_config;

    // 创建 HEADER
    auth_header_length = os_strlen("Content-Type: application/json\r\nAuthorization: Bearer ") + os_strlen(llm_chat_cfg->ark_api_key) + 1;
    auth_header = (char *)llm_zalloc(auth_header_length);
    if (auth_header == NULL) {
        llm_err("Memory allocation failed\r\n");
        ret = LLME_NOMEM;
        goto cleanup;
    }
    os_snprintf(auth_header, auth_header_length, "Content-Type: application/json\r\nAuthorization: Bearer %s", llm_chat_cfg->ark_api_key);
    chat_session->base.headers = llm_build_header(chat_session->base.headers, auth_header);
    if (chat_session->base.headers == NULL) {
        llm_err("header alloc failed\r\n");
        ret = LLME_NOMEM;
        goto cleanup;
    }
    /*chat_session->base.headers = llm_build_header(chat_session->base.headers, "Content-Type: application/json");
    if (chat_session->base.headers == NULL) {
        llm_err("header alloc failed\r\n");
        ret = RET_ERR;
        goto cleanup;
    }

    auth_header_length = os_strlen("Authorization: Bearer ") + os_strlen(llm_chat_cfg->ark_api_key) + 1;
    auth_header = (char *)llm_malloc(auth_header_length);
    if (auth_header == NULL) {
        llm_err("auth_header alloc failed\r\n");
        ret = LLME_NOMEM;
        goto cleanup;
    }

    os_snprintf(auth_header, auth_header_length, "Authorization: Bearer %s", llm_chat_cfg->ark_api_key);
    llm_err("auth_header: %s\r\n", auth_header);
    chat_session->base.headers = llm_build_header(chat_session->base.headers, auth_header);
    if (chat_session->base.headers == NULL) {
        llm_err("header alloc failed\r\n");
        ret = RET_ERR;
        goto cleanup;
    }*/

    chat_used_times = os_jiffies();
    chat_session->base.handle = llm_https_connect(chat_session->base.headers, chat_session->base.transfer_config, 
                                (void *)doubao_llm_write_callback, NULL, (void *)doubao_llm_xferinfo_callback, (void *)chat_session);
    if (chat_session->base.handle == NULL) {
        //os_printf("CHAT connect fail(%dms)\r\n", os_jiffies_to_msecs(os_jiffies() - chat_used_times));
        ret = RET_ERR;
        llm_chat_event(chat_session, LLM_EVENT_CONN_ERR, 0, 0);
        goto cleanup;
    }
    //os_printf("CHAT connect(%dms)\r\n", os_jiffies_to_msecs(os_jiffies() - chat_used_times));
    llm_free(auth_header);
    llm_chat_event(chat_session, LLM_EVENT_CONNECTED, 0, 0);
    return ret;

cleanup:
    if (auth_header) { llm_free(auth_header); }
    if (chat_session->base.headers) {
        llm_free_header(chat_session->base.headers);
        chat_session->base.headers = NULL;
    }
    if (chat_session->base.handle) {
        llm_websocket_disconnect(chat_session->base.handle);
        chat_session->base.handle = NULL;
    }
    return ret;
}

static int32 doubao_llm_disconnect(void *session)
{
    struct llm_session_chat *chat_session = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    chat_session = (struct llm_session_chat *)session;

    if (chat_session->base.headers) {
        llm_free_header(chat_session->base.headers);
        chat_session->base.headers = NULL;
    }
    if (chat_session->base.handle) {
        llm_https_disconnect(chat_session->base.handle);
        chat_session->base.handle = NULL;
    }
    llm_chat_event(chat_session, LLM_EVENT_DISCONNECT, 0, 0);
    return RET_OK;
}

static int32 doubao_llm_init(void *session)
{
    int32 ret = RET_OK;
    struct llm_session_chat *chat_session = NULL;
    struct doubao_llm_private_params *private_params = NULL;
    struct doubao_llm_image_info *image_info = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    chat_session = (struct llm_session_chat *)session;

    private_params = llm_zalloc(sizeof(struct doubao_llm_private_params));
    if (!private_params) {
        llm_err("no memory!\n");
        ret = LLME_NOMEM;
        goto cleanup;
    }
    image_info = llm_malloc((chat_session->sparam->image_max_cnt + 1) * sizeof(struct doubao_llm_image_info));
    if (!image_info) {
        llm_err("no memory!\r\n");
        ret = LLME_NOMEM;
        goto cleanup;
    }
    RB_INIT_R(&private_params->image_rb, chat_session->sparam->image_max_cnt, image_info);

    chat_session->private  = private_params;
    private_params->chat_session = chat_session;
    return RET_OK;

cleanup:
    if (private_params) { llm_free(private_params); }
    if (image_info) { llm_free(image_info); }
    chat_session->private = NULL;
    return ret;
}

static int32 doubao_llm_deinit(void *session)
{
    struct llm_session_chat *chat_session = NULL;
    struct doubao_llm_private_params *private_params = NULL;
    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    chat_session = (struct llm_session_chat *)session;

    private_params = (struct doubao_llm_private_params *)chat_session->private;
    if (private_params) {
        doubao_llm_private_params_free(private_params);
        llm_free(private_params);
    }
    llm_free(private_params->image_rb.rbq);
    chat_session->private = NULL;
    return RET_OK;
}

const struct llm_model_data doubao_llm = {
    .headsize   = 0,
    .init       = doubao_llm_init,
    .deinit     = doubao_llm_deinit,
    .connect    = doubao_llm_connect,
    .disconnect = doubao_llm_disconnect,
    .recycle    = doubao_llm_recycle,
    .upload     = doubao_llm_upload,
    .send       = doubao_llm_send,
    .recv       = doubao_llm_recv,
};

