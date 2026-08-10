#include "llm.h"
#include "cJSON.h"
#include "list.h"
#include "mbedtls/sha1.h"
#include "osal_file.h"

struct neteast_sts_private_params {
    WsContext ws_ctx;
    void *session_sts;
};

/* 将输入数据计算 SHA1，并输出为十六进制字符串（小写）。*/
#define SHA_DIGEST_LENGTH 20
static char *neteast_sha1_hex_string(const unsigned char *data, size_t len)
{
    unsigned char hash[SHA_DIGEST_LENGTH];
    mbedtls_sha1(data, len, hash);

    char *out = (char *)llm_malloc(SHA_DIGEST_LENGTH * 2 + 1);
    if (!out) { return NULL; }

    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        os_sprintf(out + (i * 2), "%02x", hash[i]);
    }
    out[SHA_DIGEST_LENGTH * 2] = '\0';
    return out;
}

// 生成随机字符串
static void neteast_generate_nonce(char *nonce, int length)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < length; i++) {
        nonce[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    nonce[length] = '\0';
}

/* 计算 checksum: sha1(appsecret + nonce + curtime) 的十六进制字符串表示*/
static char *neteast_get_checksum(const char *appsecret, const char *nonce, char *cur_time_str)
{
    char *input = NULL;
    uint32 pos = 0;
    char *checksum = NULL;

    // 计算拼接后的总长度
    size_t app_len = os_strlen(appsecret);
    size_t nonce_len = os_strlen(nonce);
    size_t time_len = os_strlen(cur_time_str);
    size_t total_len = app_len + nonce_len + time_len;

    input = (char *)llm_malloc(total_len + 1);
    if (!input) {
        llm_err("Error!no memory!\n");
        return NULL;
    }

    memcpy(input, appsecret, app_len);
    pos = app_len;
    memcpy(input + pos, nonce, nonce_len);
    pos += nonce_len;
    memcpy(input + pos, cur_time_str, time_len);
    input[total_len] = '\0';

    // 计算SHA1并转换为十六进制字符串
    checksum = neteast_sha1_hex_string((const unsigned char *)input, total_len);
    llm_free(input);
    return checksum;
}

/**
 * @brief 从 License 激活响应 JSON 中提取 license 字段
 *
 * @param json_str JSON 响应字符串
 * @param out_license 输出参数，用于存储提取的 license（需要调用者自行 free）
 * @return int RET_OK(0) 表示成功，非 0 表示失败
 */
static int neteast_extract_license(const char *json_str, char **out_license)
{
    // 1. 参数有效性检查
    if (json_str == NULL || out_license == NULL) {
        llm_err("Invalid input params\r\n");
        return RET_ERR;
    }

    *out_license = NULL;

    // 2. 解析 JSON 字符串
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            llm_err("JSON parse error: %s\r\n", error_ptr);
        }
        return RET_ERR;
    }

    // 3. 检查响应码是否为 200
    cJSON *code_item = cJSON_GetObjectItemCaseSensitive(root, "code");
    if (code_item == NULL || !cJSON_IsNumber(code_item) || code_item->valueint != 200) {
        llm_err("Response code error: %d\r\n",
                (code_item != NULL && cJSON_IsNumber(code_item)) ? code_item->valueint : -1);
        cJSON_Delete(root);
        return RET_ERR;
    }

    // 4. 获取 data 对象
    cJSON *data_obj = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (data_obj == NULL || !cJSON_IsObject(data_obj)) {
        llm_err("data field not found or not an object\r\n");
        cJSON_Delete(root);
        return RET_ERR;
    }

    // 5. 提取 license 字段（核心）
    cJSON *license_item = cJSON_GetObjectItemCaseSensitive(data_obj, "license");
    if (license_item == NULL || !cJSON_IsString(license_item) || license_item->valuestring == NULL) {
        llm_err("license field not found or invalid\r\n");
        cJSON_Delete(root);
        return RET_ERR;
    }

    // 6. 复制 license 值到输出参数
    *out_license = llm_strdup(license_item->valuestring);
    if (*out_license == NULL) {
        llm_err("Memory allocation failed for license\r\n");
        cJSON_Delete(root);
        return RET_ERR;
    }

    // 7. 释放 JSON 对象
    cJSON_Delete(root);
    return RET_OK;
}


// 响应回调函数
static size_t neteast_license_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    char *reply_buff = userdata;
    int reply_len = size * nmemb;
    if (reply_buff) {
        memcpy(reply_buff, ptr, reply_len);
    }
    return reply_len;
}

static int neteast_get_license(void *session, char **license)
{
    struct neteast_chat_platform_cfg *platform_cfg = NULL;
    struct llm_session_sts *sts_session = NULL;
    char *checksum = NULL;
    char nonce[17] = {0};
    char cur_time_str[32];
    void *headers = NULL;
    void *license_hdl = NULL;
    char *reply_buff = NULL;
    int ret = RET_OK;
    char *license_str = NULL;

    if (license) {
        *license = NULL;
    }
    if (!session || !license) {
        llm_err("Input param error!\n");
        ret = RET_ERR;
        goto __cleanup;
    }

    sts_session = (struct llm_session_sts *)session;

    if (!sts_session->base.platform_config || !sts_session->base.transfer_config) {
        llm_err("Platform or transfer config missing!\n");
        ret = RET_ERR;
        goto __cleanup;
    }

    platform_cfg = sts_session->base.platform_config;

    if (!platform_cfg->license_key || !platform_cfg->app_key ||
        !platform_cfg->app_secret || !platform_cfg->license_url) {
        llm_err("Missing required config fields!\n");
        ret = RET_ERR;
        goto __cleanup;
    }

    reply_buff = llm_zalloc(2048);
    if (!reply_buff) {
        llm_err("Error, no memory for reply buffer!\n");
        ret = LLME_NOMEM;
        goto __cleanup;
    }

    // 生成签名
    os_snprintf(cur_time_str, sizeof(cur_time_str), "%lld", (int64)time(NULL));
    neteast_generate_nonce(nonce, 16);
    checksum = neteast_get_checksum(platform_cfg->app_secret, nonce, cur_time_str);
    if (!checksum) {
        llm_err("Failed to generate checksum!\n");
        ret = RET_ERR;
        goto __cleanup;
    }

    // 构建请求
    char post_data[256] = {0};
    os_snprintf(post_data, sizeof(post_data), "{\"licenseKey\": \"%s\"}", platform_cfg->license_key);

    char app_key_header[128], nonce_header[128], cur_time_header[128], checksum_header[128];
    os_sprintf(app_key_header, "AppKey: %s", platform_cfg->app_key);
    os_sprintf(nonce_header, "Nonce: %s", nonce);
    os_sprintf(cur_time_header, "CurTime: %s", cur_time_str);
    os_sprintf(checksum_header, "CheckSum: %s", checksum);
    llm_free(checksum);
    checksum = NULL;  // 防止重复释放

    // 设置请求头
    headers = llm_build_header(headers, app_key_header);
    headers = llm_build_header(headers, nonce_header);
    headers = llm_build_header(headers, cur_time_header);
    headers = llm_build_header(headers, checksum_header);
    headers = llm_build_header(headers, "Content-Type: application/json");

    // 建立连接
    license_hdl = llm_https_connect(headers, sts_session->base.transfer_config,
                                    neteast_license_write_callback, NULL, NULL, reply_buff);
    llm_free_header(headers);
    headers = NULL;

    if (!license_hdl) {
        llm_err("Connect to license url error!\n");
        ret = RET_ERR;
        goto __cleanup;
    }

    // 发送请求
    ret = llm_https_send(license_hdl, LLM_HTTP_POST, platform_cfg->license_url,
                         post_data, os_strlen(post_data));
    if (ret != RET_OK) {
        llm_err("post license_key error:%d\n", ret);
        goto __cleanup;
    }

    // 9. 解析响应
    ret = neteast_extract_license(reply_buff, &license_str);
    llm_err("Check Response: %s\n", reply_buff);
    if (ret == RET_OK && license_str != NULL) {
        llm_dbg("Get license %s\n", license_str);
        *license = license_str;
        license_str = NULL;
    }

__cleanup:
    // 统一清理资源
    if (checksum != NULL) {
        llm_free(checksum);
    }
    if (headers != NULL) {
        llm_free_header(headers);
    }
    if (license_hdl != NULL) {
        llm_https_disconnect(license_hdl);
    }
    if (reply_buff != NULL) {
        llm_free(reply_buff);
    }
    if (license_str != NULL) {
        llm_free(license_str);  // 如果所有权未转移，释放内存
    }

    return ret;
}

//测试代码，实际商用使用neteast_get_license获取并解析license
static char *neteast_read_license_file(const char *filename, unsigned int *filesize)
{
    char *cache_buf = NULL;
    void *fp = NULL;
    uint32_t bytes_read = 0;
    int32 fsize = 0;

    *filesize = 0;
    fp = osal_fopen(filename, "rb");
    if (!fp) {
        os_printf("%s file not exist\n", filename);
        return NULL;
    }

    fsize = osal_fsize(fp);
    cache_buf = llm_zalloc(fsize + 32);
    if (!cache_buf) {
        osal_fclose(fp);
        return NULL;
    }

    bytes_read = osal_fread(cache_buf, 1, fsize, fp);
    if (bytes_read == 0) {
        os_printf("Read file error\n");
        osal_fclose(fp);
        llm_free(cache_buf);
        return NULL;
    }
    *filesize = fsize;
    osal_fclose(fp);
    return cache_buf;
}

/**
 * @brief 生成网易云信 WebSocket 连接所需的 Token
 *
 * @param app_secret 网易云信控制台获取的 AppSecret
 * @param ttl_sec Token 有效期（秒），建议设置为 300（5分钟）
 * @param out_token 输出参数，用于存储生成的 Token（需要调用者自行 free）
 * @return int 0 表示成功，非 0 表示失败
 */
static int neteast_generate_token(const char *app_secret, int ttl_sec, char **out_token)
{
    if (app_secret == NULL || out_token == NULL || ttl_sec <= 0) {
        return -1;
    }
    *out_token = NULL;

    // 使用系统时间获取毫秒级时间戳
    struct timespec ts;
    clock_gettime(1, &ts);
    int64 cur_time_ms = (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

    // 2. 生成签名：SHA1(curTime + ttl + appSecret)
    char input_str[256];
    os_snprintf(input_str, sizeof(input_str), "%lld%d%s", cur_time_ms, ttl_sec, app_secret);

    unsigned char sha1_result[SHA_DIGEST_LENGTH];
    mbedtls_sha1((const unsigned char *)input_str, os_strlen(input_str), sha1_result);

    // 将 SHA1 结果转换为十六进制字符串
    char signature[41];
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        os_sprintf(signature + (i * 2), "%02x", sha1_result[i]);
    }
    signature[40] = '\0';

    // 3. 创建 JSON 对象
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return RET_ERR;
    }
    cJSON_AddStringToObject(root, "signature", signature);
    cJSON_AddNumberToObject(root, "curTime", cur_time_ms);
    cJSON_AddNumberToObject(root, "ttl", ttl_sec);
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json_str == NULL) {
        return RET_ERR;
    }

    char *output = NULL;
    unsigned int output_len = 0;
    int ret = llm_base64_encode((const char *)json_str, os_strlen(json_str),
                                &output, &output_len, 0);
    llm_free(json_str);

    if (ret != 0 || output == NULL) {
        if (output) {
            llm_free(output);
        }
        llm_err("base64 token error:%d\n", ret);
        return ret;
    }

    *out_token = (char *)output;
    return RET_OK;
}


/* -------------------------- recv -------------------------- */

typedef void (*neteast_sts_downward_events_hdl)(struct llm_session_sts *sts_session, char *json_buf, uint32 json_len);
typedef struct {
    const char *type_str;
    neteast_sts_downward_events_hdl hdl;
} neteast_sts_downward_events;

static inline void neteast_handle_server_ready(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->recv chat updated!\r\n");
    if (sts_session->base.platform_config_change == 1) {
        llm_sts_event(sts_session, LLM_EVENT_WAITING_END, 0, 0);
    } else {
        llm_err("---->Intercept chat updated!\r\n");
    }
}

static inline void neteast_handle_func_call(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->recv function call!\r\n");
    hgprintf_out(buf, len, 0);
    _os_printf("\r\n");
}

static inline void neteast_handle_tts_start(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->recv tts start!\r\n");
    if (llm_sts_get_state(sts_session) == LLM_STS_STATE_DIALOGUE) {
        llm_sts_event(sts_session, LLM_EVENT_DIALOGUE_START, 0, 0);
    } else {
        llm_err("---->Intercept conversation.chat.completed!\r\n");
    }
}

static inline void neteast_handle_tts_stop(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->recv tts stop!\r\n");
    if (llm_sts_get_state(sts_session) == LLM_STS_STATE_DIALOGUE) {
        llm_sts_event(sts_session, LLM_EVENT_DIALOGUE_END, 0, 0);
    } else {
        llm_err("---->Intercept conversation.chat.completed!\r\n");
    }
}

static inline void neteast_handle_asr_text(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->recv asr text!\r\n");
    llm_sts_event(sts_session, LLM_EVENT_STT_RESULT, (uint32)buf, len);
}

static inline void neteast_handle_llm_text(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->recv llm_text!\r\n");
    llm_sts_event(sts_session, LLM_EVENT_DIALOGUE_START, 0, 0);
    hgprintf_out(buf, len, 0);
    _os_printf("\r\n");
}

static inline void neteast_handle_function_call(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->recv func call!\r\n");
    llm_sts_event(sts_session, LLM_EVENT_IOT_RESULT, (uint32)buf, len);
    hgprintf_out(buf, len, 0);
    _os_printf("\r\n");
}

static inline void neteast_handle_error(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    llm_err("---->Recv error response from host\n");
    hgprintf_out(buf, len, 0);
    _os_printf("\r\n");
    llm_sts_event(sts_session, LLM_EVENT_ERROR_MSG, (uint32)buf, len);
}

static const neteast_sts_downward_events g_neteast_downward_events_table[] = {
    {"server_ready",                neteast_handle_server_ready},
    {"tool_calls",                  neteast_handle_function_call},
    {"asr_text",                    neteast_handle_asr_text},
    {"llm_text",                    neteast_handle_llm_text},
    {"tts_start",                   neteast_handle_tts_start},
    {"tts_stop",                    neteast_handle_tts_stop},
    {"error",                       neteast_handle_error},
};

#define EVENT_TABLE_SIZE (sizeof(g_neteast_downward_events_table) / sizeof(g_neteast_downward_events_table[0]))

static int32 neteast_sts_match_type(const char *candidate, int cand_len, const char *expected)
{
    if (!candidate || !expected) { return 0; }
    int32 exp_len = os_strlen(expected);
    return (cand_len == (int)exp_len) && (os_strncmp(candidate, expected, exp_len) == 0);
}

static int32 neteast_sts_parse_json(struct llm_session_sts *sts_session, char *buf, uint32 len)
{
    const char *type_key = "\"action\":\"";
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
            if (neteast_sts_match_type(event_type, type_len, g_neteast_downward_events_table[i].type_str)) {
                if (g_neteast_downward_events_table[i].hdl) {
                    g_neteast_downward_events_table[i].hdl(sts_session, buf, len);
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

static void neteast_sts_ws_ctx_free(WsContext *ws_ctx)
{
    if (ws_ctx == NULL) { return; }
    if (ws_ctx->buffer) {
        llm_free(ws_ctx->buffer);
        ws_ctx->buffer = NULL;
    }
    ws_ctx->total_size = 0;
    ws_ctx->is_final = 0;
}

static int32 neteast_sts_handle_text_frame(struct llm_session_sts *sts_session, const char *chunk, llm_recv_meta *rmeta)
{
    struct neteast_sts_private_params *private_params = (struct neteast_sts_private_params *)sts_session->private;
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
            neteast_sts_ws_ctx_free(ws_ctx); // 释放旧缓冲区，避免内存泄漏
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
            neteast_sts_ws_ctx_free(ws_ctx);
            return RET_ERR;
        }

        // 分配缓冲区（预留'\0'终止符空间）
        uint32 buf_size = rmeta->len + rmeta->bytesleft + 1;
        ws_ctx->buffer = llm_malloc(buf_size);
        if (ws_ctx->buffer == NULL) {
            llm_err("Malloc buffer failed (need: %u bytes)\r\n", buf_size);
            neteast_sts_ws_ctx_free(ws_ctx);
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

static int neteast_sts_handle_binary_frame(struct llm_session_sts *sts_session,
        const char *chunk, llm_recv_meta *rmeta)
{
    int32 temp = 0;
    uint32 written = 0;
    uint32 wait_cnt = 0;
    uint32 decoded_audio_len   = rmeta->nread;
    const char *decoded_audio_buf    = chunk;

    if (chunk == NULL || sts_session == NULL || rmeta == NULL) {
        llm_err("Input param error!\r\n");
        return RET_ERR;
    }

    while (written < decoded_audio_len) {
        if (llm_sts_get_state(sts_session) != LLM_STS_STATE_DIALOGUE) {
            //llm_err("Error state:%d\n", llm_sts_get_state(sts_session));
            return RET_OK;
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

    return RET_OK;
}

static int32 neteast_sts_handle_websocket(struct llm_session_sts *sts_session, char *chunk, uint32 length)
{
    int32 ret = RET_ERR;
    llm_recv_meta rmeta = {0};
    struct neteast_sts_private_params *private_params = (struct neteast_sts_private_params *)sts_session->private;

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
        ret = neteast_sts_handle_text_frame(sts_session, chunk, &rmeta);
        if (ret != RET_OK) {
            return ret;
        }
    } else if (rmeta.flags & LLMWS_BINARY) {// 处理BINARY帧(音频)
        return neteast_sts_handle_binary_frame(sts_session, chunk, &rmeta);
    } else if (rmeta.flags & LLMWS_CLOSE) {// 处理关闭帧
        llm_err("WS server requested close connection\r\n");
        neteast_sts_ws_ctx_free(&private_params->ws_ctx); // 释放上下文资源
        return RET_ERR;
    } else if (rmeta.flags & LLMWS_PONG) {
        llm_err("Recv ping/pong frame:%s\n",chunk);
    } else {// 不支持的帧类型
        llm_err("Unsupported WS frame type (flags: 0x%02x)\r\n", rmeta.flags);
        neteast_sts_ws_ctx_free(&private_params->ws_ctx); // 释放上下文资源
        return RET_ERR;
    }

    // 处理完整消息（所有分片接收完成）
    if (private_params->ws_ctx.is_final && private_params->ws_ctx.total_size > 0) {
        private_params->ws_ctx.buffer[private_params->ws_ctx.total_size] = '\0';
        // 处理JSON消息
        if (sts_session->base.new_dialogue == 1) {
            neteast_sts_parse_json(sts_session, private_params->ws_ctx.buffer, private_params->ws_ctx.total_size);
        } else {
            llm_dbg("Intercept data(%d): %s\r\n", private_params->ws_ctx.total_size, private_params->ws_ctx.buffer);
        }
        neteast_sts_ws_ctx_free(&private_params->ws_ctx);
    }

    return RET_OK;
}

static int32 neteast_sts_recv(void *session, char *buff, uint32 buffer_size)
{
    struct llm_session_sts *sts_session = NULL;
    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    sts_session = (struct llm_session_sts *)session;

    return neteast_sts_handle_websocket(sts_session, buff, buffer_size);
}

/* -------------------------- send -------------------------- */
static char *neteast_sts_create_chat_update_json(struct llm_session_sts *sts_session)
{
    char *json_str = NULL;
    char *json_str_temp = NULL;
    struct neteast_chat_platform_cfg *cfg = sts_session->base.platform_config;

    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "action", "start");

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "data", data);

    //chat_config

    //input audio
    cJSON *input_audio = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "input_audio", input_audio);
    if (cfg->input_audio_format) {
        cJSON_AddStringToObject(input_audio, "format", cfg->input_audio_format);
    }
    if (cfg->input_audio_sample_rate) {
        cJSON_AddNumberToObject(input_audio, "sample_rate", os_atoi(cfg->input_audio_sample_rate));
    }
    if (cfg->input_audio_channel) {
        cJSON_AddNumberToObject(input_audio, "channels", os_atoi(cfg->input_audio_channel));
    }
    if (cfg->input_audio_encoding) {
        cJSON_AddStringToObject(input_audio, "encoding", cfg->input_audio_encoding);
    }

    //output audio
    cJSON *output_audio = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "output_audio", output_audio);
    if (cfg->output_audio_format) {
        cJSON_AddStringToObject(output_audio, "format", cfg->output_audio_format);
    }
    if (cfg->output_audio_sample_rate) {
        cJSON_AddNumberToObject(output_audio, "sample_rate", os_atoi(cfg->output_audio_sample_rate));
    }
    if (cfg->output_audio_channel) {
        cJSON_AddNumberToObject(output_audio, "channels", os_atoi(cfg->output_audio_channel));
    }
    if (cfg->output_audio_encoding) {
        cJSON_AddStringToObject(output_audio, "encoding", cfg->output_audio_encoding);
    }

    json_str_temp = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    json_str = llm_strdup(json_str_temp);
    if (json_str_temp) { cJSON_free(json_str_temp); }
    return json_str;
}

static char *neteast_sts_create_audio_append_json(struct llm_session_sts *sts_session, struct llm_data *pdata)
{
    return pdata->buff1;
}

#define NETEAST_INPUT_AUDIO_BUFFER_COMPLETE "{\"id\":\"event_id\",\"event_type\":\"input_audio_buffer.complete\"}"
static char *neteast_sts_create_json(struct llm_session_sts *sts_session, struct llm_data *data)
{
    char *json_str = NULL;

    switch (data->type) {
        case LLM_DATA_TYPE_MGMT: {
            json_str = neteast_sts_create_chat_update_json(sts_session);
            break;
        }
        case LLM_DATA_TYPE_AUDIO:
            break;
        case LLM_DATA_TYPE_TEXT: {
            llm_err("Not currently supported: LLM_DATA_TYPE_TEXT\r\n");
            if (data->buff1)
            { llm_free(data->buff1); }
            data->buff1 = NULL;
            data->buff1_len = 0;
            break;
        }
        case LLM_DATA_TYPE_RAW: {
            if (data->buff1 && data->buff1_len > 0) {
                json_str = llm_strdup(data->buff1);
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
    return json_str;
}

/**********************upload file*********************/
static int32 neteast_sts_send(void *session, struct llm_data *data)
{
    int32 ret = LLME_AGAIN;
    struct llm_session_sts *sts_session = NULL;
    char *json_str = NULL;
    uint32 json_len = 0;

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
        if (data->type == LLM_DATA_TYPE_AUDIO) {
            json_str = data->buff1;
            json_len = data->buff1_len;
        } else if(data->type == LLM_DATA_TYPE_PING){
            json_str = data->buff1;
            json_len = data->buff1_len;
            smeta.flags = LLMWS_PING;
            llm_err("Send ping to keepalive...\n");
        } else {
            json_str = neteast_sts_create_json(sts_session, data);
            if (json_str) {
                json_len = os_strlen(json_str);
                if (data->buff1)
                { llm_free(data->buff1); }
                data->buff1 = json_str;
                data->buff1_len = json_len;
            } else {
                return LLME_NOMEM;
            }
        }
    } else {
        json_str = data->buff1 + data->offset;
        json_len = data->buff1_len - data->offset;
    }
    if (data->type == LLM_DATA_TYPE_AUDIO) {
        smeta.flags = LLMWS_BINARY;
        if (json_str == NULL) {
            data->buff1_len = 0;
            return RET_OK;
        }
    }
    ret = llm_websocket_psend(sts_session->base.handle, json_str, json_len, &smeta);
    if (data->transfer_state == LLM_DATA_STATE_START ||
        data->transfer_state == LLM_DATA_STATE_END ||
        data->type == LLM_DATA_TYPE_RAW) {
        _os_printf("neteast_sts_send len: (%d:%d)\n", json_len, smeta.sent);
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
#define NETEAST_CONVERSATION_CHAT_CANCEL "{\"action\":\"manual_interrupt\",\"data\":{\"id\":\"123\"}}"
static int32 neteast_sts_recycle(void *session, uint8 param)
{
    char *cancel_str = NULL;
    struct llm_session_sts *sts_session = NULL;
    struct neteast_sts_private_params *private_params = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    sts_session = (struct llm_session_sts *)session;
    private_params = (struct neteast_sts_private_params *)sts_session->private;
    if (private_params) {
        //neteast_sts_ws_ctx_free(&private_params->ws_ctx);
    }

    llm_err("Conversation interrupt!\n");
    cancel_str = llm_strdup(NETEAST_CONVERSATION_CHAT_CANCEL);
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
        llm_sts_event(sts_session, LLM_EVENT_INTERRUPT_END, 0, 0);
        return RET_OK;
    } else {
        llm_err("Shouldn't be here!\n");
        llm_free(cancel_str);
        return RET_ERR;
    }
}

static int32 neteast_sts_upload(void *session, struct llm_data *data)
{
    return RET_OK;
}

static int32 neteast_sts_init(void *session)
{
    struct llm_session_sts *sts_session = NULL;
    struct neteast_sts_private_params  *neteast_sts = NULL;
    int32 ret = RET_OK;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    sts_session = (struct llm_session_sts *)session;

    neteast_sts = llm_zalloc(sizeof(struct neteast_sts_private_params));
    if (!neteast_sts) { //sts_session->private
        llm_err("Error,no memory!\n");
        ret = LLME_NOMEM;
        goto __failed;
    }
    sts_session->private  = neteast_sts;
    neteast_sts->session_sts = sts_session;
    return RET_OK;

__failed:
    if (neteast_sts) {
        llm_free(neteast_sts);
    }
    sts_session->private = NULL;
    return ret;
}

static int32 neteast_sts_deinit(void *session)
{
    struct llm_session_sts *sts_session = NULL;
    struct neteast_sts_private_params *private_params = NULL;
    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    sts_session = (struct llm_session_sts *)session;

    private_params = (struct neteast_sts_private_params *)sts_session->private;
    if (private_params) {
        llm_free(private_params);
    }
    sts_session->private = NULL;
    return RET_OK;
}

static int32 neteast_sts_connect(void *session)
{
    int32 ret = RET_OK;
    struct llm_session_sts *sts_session = NULL;
    struct neteast_chat_platform_cfg *platform_cfg = NULL;
    uint32 tick = 0;
    void *headers = NULL;
    char *chat_url = NULL;
    char *license = NULL;
    char *token   = NULL;
    unsigned int license_len = 0;
    int url_len = 0;

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

    ret = neteast_get_license(sts_session, &license);
    /*
    if (ret) {
        llm_err("Get license failed:%d\n", ret);
        ret = RET_ERR;
        goto __finish;
    }
    */
    //需要网易商务提供license_key,才能通过neteast_get_license成功获取license,目前暂时用网易测试用的license
    if (license) { llm_free(license); }
    license = neteast_read_license_file("license.txt", &license_len);
    if (NULL == license) {
        llm_err("Get license failed:%d\n", ret);
        ret = RET_ERR;
        goto __finish;
    }

    ret = neteast_generate_token(platform_cfg->app_secret, 300, &token);
    if (ret) {
        llm_err("Generate token failed:%d\n", ret);
        goto __finish;
    }

    url_len = os_snprintf(NULL, 0, "%s/?device_id=%s", platform_cfg->host_url, platform_cfg->device_id);
    chat_url = (char *)llm_zalloc(url_len + 1);
    if (chat_url == NULL) {
        llm_err("chat_url alloc failed\r\n");
        ret = LLME_NOMEM;
        goto __finish;
    }
    os_snprintf(chat_url, url_len + 1, "%s/?device_id=%s",
                platform_cfg->host_url, platform_cfg->device_id);

    // 添加必需的请求头
    char license_header[256];
    char appkey_header[128];
    char token_header[512];
    os_snprintf(license_header, sizeof(license_header), "yunxin-license: %s", license);
    os_snprintf(appkey_header, sizeof(appkey_header), "app-key: %s", platform_cfg->app_key);
    os_snprintf(token_header, sizeof(token_header), "token: %s", token);
    headers = llm_build_header(headers, license_header);
    headers = llm_build_header(headers, appkey_header);
    headers = llm_build_header(headers, token_header);

    tick = os_jiffies();
    sts_session->base.handle = llm_websocket_connect(chat_url, headers, sts_session->base.transfer_config);
    if (!sts_session->base.handle) {
        llm_err("Creat websocket failed,url:%s\n", chat_url);
        ret = RET_ERR;
        goto __finish;
    }
    os_printf("llm websocket connect to %s (%dms) success\r\n",
              chat_url, os_jiffies_to_msecs(os_jiffies() - tick));
    llm_sts_event(sts_session, LLM_EVENT_CONNECTED, 0, 0);

__finish:
    if (license) {
        llm_free(license);
    }
    if (token) {
        llm_free(token);
    }
    if (headers) {
        llm_free_header(headers);
    }
    if (chat_url) {
        llm_free(chat_url);
    }
    return ret;
}

static int32 neteast_sts_disconnect(void *session)
{
    struct llm_session_sts *sts_session = NULL;
    struct neteast_sts_private_params *private_params = NULL;

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
    private_params = (struct neteast_sts_private_params *)sts_session->private;
    if (private_params) {
        ;
    }
    llm_sts_event(sts_session, LLM_EVENT_DISCONNECT, 0, 0);
    return RET_OK;
}

const struct llm_model_data neteast_sts_model = {
    .headsize   = 0,
    .init       = neteast_sts_init,
    .deinit     = neteast_sts_deinit,
    .connect    = neteast_sts_connect,
    .disconnect = neteast_sts_disconnect,
    .recycle    = neteast_sts_recycle,
    .upload     = neteast_sts_upload,
    .send       = neteast_sts_send,
    .recv       = neteast_sts_recv,
};

