#include "llm.h"
#include "cJSON.h"

#define SAFE_GET_STRING(obj, key) ({ \
        cJSON *_item = cJSON_GetObjectItem(obj, key); \
        (_item && cJSON_IsString(_item) ? _item->valuestring : NULL); \
    })

uint64 stt_used_times = 0;

struct doubao_stt_private_params {
    void *stt_session;
    char   *temp_data;
    int32  temp_data_len;
    WsContext ws_ctx;
    struct Doubao_STT_Header asr_stt_header;
};

static void doubao_stt_ws_ctx_free(WsContext *ws_ctx)
{
    if (ws_ctx == NULL) { return; }
    if (ws_ctx->buffer) {
        llm_free(ws_ctx->buffer);
        ws_ctx->buffer = NULL;
    }
    ws_ctx->total_size = 0;
    ws_ctx->is_final = 0;
}

static int32 doubao_stt_errmsg_check(struct llm_session_stt *stt_session, char *buff, uint32 buff_len)
{
    if (buff && buff_len > 0) {
        if (os_strncasestr(buff, "\"error\":\"", -1)) {
            llm_stt_event(stt_session, LLM_EVENT_ERROR_MSG, (uint32)buff, buff_len);
            return RET_OK;
        }
    }
    return RET_ERR;
}

// 处理完整的文本
static int32 doubao_stt_process_complete(struct llm_session_stt *stt_session, const char *text)
{
    int32 ret = RET_OK;
    uint32 text_len = 0;

    text_len = os_strlen(text);
    os_printf("STT(%dms)\r\n", os_jiffies_to_msecs(os_jiffies() - stt_used_times));
    //dump_hex("", text_tx_buffer, os_strlen(text_tx_buffer)+1, 1);
    llm_stt_event(stt_session, LLM_EVENT_STT_RESULT, (uint32)text, text_len);
    return ret;
}

static int32 doubao_stt_parse_json(struct llm_session_stt *stt_session, cJSON *root)
{
    int32 ret = RET_OK;

    if (!root) { return RET_ERR; }

    cJSON *sequence_item = cJSON_GetObjectItem(root, "sequence");
    if (!cJSON_IsNumber(sequence_item)) {
        llm_err("sequence is err！\n");
        return RET_ERR;
    }

    cJSON *code_item = cJSON_GetObjectItem(root, "code");
    if (!cJSON_IsNumber(code_item)) {
        llm_err("code is err！\n");
        return RET_ERR;
    }

    if (code_item->valueint == 1000) {
        if (sequence_item->valueint < 0) {
            cJSON *result_array = cJSON_GetObjectItem(root, "result");
            if (!cJSON_IsArray(result_array)) {
                llm_err("result is err!\n");
                return RET_ERR;
            }

            cJSON *first_item = cJSON_GetArrayItem(result_array, 0);
            if (!first_item) {
                llm_err("first_item is err!\n");
                return RET_ERR;
            }

            const char *text_result = SAFE_GET_STRING(first_item, "text");
            if (text_result) {
                ret = doubao_stt_process_complete(stt_session, text_result);
            }
        }
    } else {
        return RET_ERR;
    }
    return ret;
}

static int32 doubao_stt_handle_received_message(struct llm_session_stt *stt_session, char *buffer, size_t length)
{
    int32 ret = RET_ERR;
    struct doubao_stt_private_params *private_params = (struct doubao_stt_private_params *)stt_session->private;

    if (!buffer || length < 4) {
        llm_err("Received message is null or too short.\n");
        return RET_ERR;
    }

    private_params->asr_stt_header.protocol_version = buffer[0] & 0xF0;
    private_params->asr_stt_header.header_size = buffer[0] & 0x0F;
    private_params->asr_stt_header.message_type = (buffer[1] >> 4) & 0x0F;
    private_params->asr_stt_header.specific_flags = buffer[1] & 0x0F;
    private_params->asr_stt_header.serialization_method = buffer[2] & 0xF0;
    private_params->asr_stt_header.compression = buffer[2] & 0x0F;
    private_params->asr_stt_header.reserved = buffer[3];
    int32 playload_size;

    // 根据消息类型处理不同的响应
    switch (private_params->asr_stt_header.message_type) {
        case 0x9: // Full Server Response
            // 大端序
            os_memcpy(&playload_size, buffer + 4, sizeof(playload_size));
            private_params->asr_stt_header.playload_size = os_ntohl(playload_size);

            llm_dbg("/*******************************/\r\n");
            llm_dbg("Protocol version:%d\r\n", private_params->asr_stt_header.protocol_version);
            llm_dbg("Header size:%d\r\n", private_params->asr_stt_header.header_size * 4);
            llm_dbg("Message type:0x%x\r\n", private_params->asr_stt_header.message_type);
            llm_dbg("Message type specific flags:%d\r\n", private_params->asr_stt_header.specific_flags);
            llm_dbg("Message serialization method:%d\r\n", private_params->asr_stt_header.serialization_method);
            llm_dbg("Message compression:%d\r\n", private_params->asr_stt_header.compression);
            llm_dbg("Payload size:%d\r\n", private_params->asr_stt_header.playload_size);
            llm_dbg("/*******************************/\r\n");
            llm_dbg("Full Server Response\n");
            //hgprintf_out(buffer + 8, length - 8, 0);
            //_os_printf("\r\n");
            cJSON *root = cJSON_Parse(buffer + 8);
            if (root) {
                // 处理JSON数据
                ret = doubao_stt_parse_json(stt_session, root);
                cJSON_Delete(root);
                if (ret == RET_ERR) {
                    ret = doubao_stt_errmsg_check(stt_session, buffer + 8, length - 8);
                }
            } else {
                llm_err("root err!\n");
            }
            break;
        case 0xf: // Error Message from Server
            llm_err("Error Message from Server\n");
            ret = doubao_stt_errmsg_check(stt_session, buffer + 12, length - 12);
            break;
        default:
            llm_err("Unknown message type received.\n");
    }
    return ret;
}

static int32 doubao_stt_handle_binary_frame(struct llm_session_stt *stt_session, const char *chunk, llm_recv_meta *rmeta)
{
    struct doubao_stt_private_params *private_params = (struct doubao_stt_private_params *)stt_session->private;
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
            doubao_stt_ws_ctx_free(ws_ctx); // 释放旧缓冲区，避免内存泄漏
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
            doubao_stt_ws_ctx_free(ws_ctx);
            return RET_ERR;
        }

        // 分配缓冲区（预留'\0'终止符空间）
        uint32 buf_size = rmeta->len + rmeta->bytesleft + 1;
        ws_ctx->buffer = llm_malloc(buf_size);
        if (ws_ctx->buffer == NULL) {
            llm_err("Malloc buffer failed (need: %u bytes)\r\n", buf_size);
            doubao_stt_ws_ctx_free(ws_ctx);
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

static int32 doubao_stt_handle_websocket(struct llm_session_stt *stt_session, char *chunk, size_t length)
{
    int32 ret = RET_OK;
    llm_recv_meta rmeta = {0};
    struct doubao_stt_private_params *private_params = (struct doubao_stt_private_params *)stt_session->private;

    if (chunk == NULL || length == 0) {
        llm_err("Invalid input params (chunk null / length=0)\r\n");
        return RET_ERR;
    }


    // 预处理WebSocket数据，解析帧信息
    ret = llm_websocket_precv(stt_session->base.handle, chunk, length, &rmeta);
    if (ret != RET_OK) {
        return ret;
    }

    if (rmeta.flags & LLMWS_BINARY) {    // 处理 bin 帧
        ret = doubao_stt_handle_binary_frame(stt_session, chunk, &rmeta);
        if (ret != RET_OK) {
            return ret;
        }
    }
    // 处理关闭帧
    else if (rmeta.flags & LLMWS_CLOSE) {
        llm_err("WS server requested close connection\r\n");
        doubao_stt_ws_ctx_free(&private_params->ws_ctx); // 释放上下文资源
        return RET_ERR;
    }
    // 不支持的帧类型
    else {
        llm_err("Unsupported WS frame type (flags: 0x%02x)\r\n", rmeta.flags);
        doubao_stt_ws_ctx_free(&private_params->ws_ctx); // 释放上下文资源
        return RET_ERR;
    }

    // 处理完整消息（所有分片接收完成）
    if (private_params->ws_ctx.is_final && private_params->ws_ctx.total_size > 0) {
        private_params->ws_ctx.buffer[private_params->ws_ctx.total_size] = '\0';
        // 处理JSON消息
        ret = doubao_stt_handle_received_message(stt_session, private_params->ws_ctx.buffer, private_params->ws_ctx.total_size);
        doubao_stt_ws_ctx_free(&private_params->ws_ctx);
    }

    return ret;
}

static int32 doubao_stt_recv(void *session, char *buff, uint32 buffer_size)
{
    struct llm_session_stt *stt_session = NULL;
    if (!session || !buff || buffer_size <= 0) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    stt_session = (struct llm_session_stt *)session;

    return doubao_stt_handle_websocket(stt_session, buff, buffer_size);
}

static uint32 doubao_stt_send_full_client_request(struct llm_session_stt *stt_session, char **frame_str)
{
    uint32 json_str_len = RET_ERR;
    char *json_str = NULL;
    char *json_str_temp = NULL;
    struct Doubao_STT_Cfg *asr_stt_cfg = (struct Doubao_STT_Cfg *)stt_session->base.platform_config;

    uint8 temp[16] = {0};
    char reqid[32 + 4] = {0};
    os_random_bytes(temp, 16);
    key_str(temp, 16, reqid);

    // 创建 cJSON 对象
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        llm_err("Failed to create cJSON object\r\n");
        *frame_str = NULL;
        return LLME_NOMEM;
    }

    // 创建 "app" 对象
    cJSON *app = cJSON_CreateObject();
    cJSON_AddStringToObject(app, "appid", asr_stt_cfg->app_appid);
    cJSON_AddStringToObject(app, "token", asr_stt_cfg->app_token);
    cJSON_AddStringToObject(app, "cluster", asr_stt_cfg->app_cluster);
    cJSON_AddItemToObject(root, "app", app);

    // 创建 "user" 对象
    cJSON *user = cJSON_CreateObject();
    cJSON_AddStringToObject(user, "uid", asr_stt_cfg->user_uid);
    cJSON_AddItemToObject(root, "user", user);

    // 创建 "audio" 对象
    cJSON *audio = cJSON_CreateObject();
    cJSON_AddStringToObject(audio, "format", asr_stt_cfg->audio_format);
    if (asr_stt_cfg->audio_codec) {
        cJSON_AddStringToObject(audio, "codec", asr_stt_cfg->audio_codec);
    }
    if (asr_stt_cfg->audio_rate) {
        cJSON_AddNumberToObject(audio, "rate", os_atoi(asr_stt_cfg->audio_rate));
    }
    if (asr_stt_cfg->audio_bits) {
        cJSON_AddNumberToObject(audio, "bits", os_atoi(asr_stt_cfg->audio_bits));
    }
    if (asr_stt_cfg->audio_channel) {
        cJSON_AddNumberToObject(audio, "channel", os_atoi(asr_stt_cfg->audio_channel));
    }
    cJSON_AddItemToObject(root, "audio", audio);

    // 创建 "request" 对象
    cJSON *request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "reqid", reqid);
    cJSON_AddNumberToObject(request, "sequence", 1);
    cJSON_AddBoolToObject(request, "show_utterances", false);
    if (asr_stt_cfg->boosting_table_name) {
        cJSON_AddStringToObject(audio, "boosting_table_name", asr_stt_cfg->boosting_table_name);
    }
    if (asr_stt_cfg->correct_table_name) {
        cJSON_AddStringToObject(audio, "correct_table_name", asr_stt_cfg->correct_table_name);
    }
    cJSON_AddItemToObject(root, "request", request);

    json_str_temp = cJSON_PrintUnformatted(root);
    if (!json_str_temp) {
        llm_err("Failed to print JSON!\n");
        cJSON_Delete(root);
        *frame_str = NULL;
        return LLME_NOMEM;
    }
    cJSON_Delete(root);

//    llm_err("json_body(%d):", json_body);
//    hgprintf_out(json_body, os_strlen(json_body), 0);
//    _os_printf("\r\n");

    unsigned char req_header[8];
    req_header[0] = 0x11; // Protocol version: 0b0001, Header size: 0b0001 (4 bytes)
    req_header[1] = 0x10; // Message type: full client request (0b0001), Message type specific flags: none (0b0000)
    req_header[2] = 0x10; // Message serialization method: JSON (0b0001), Message compression: None (0b0000)
    req_header[3] = 0x00; // Reserved

    json_str_len = os_strlen(json_str_temp) + 8;
    json_str = llm_malloc(json_str_len);
    if (!json_str) {
        llm_err("No buffer!\r\n");
        cJSON_free(json_str_temp);
        *frame_str = NULL;
        return LLME_NOMEM;
    }
    uint32 payload_length = os_htonl(os_strlen(json_str_temp));
    os_memcpy(req_header + 4, &payload_length, sizeof(payload_length));
    os_memcpy(json_str, req_header, sizeof(req_header));
    os_memcpy(json_str + 8, json_str_temp, os_strlen(json_str_temp));
    cJSON_free(json_str_temp);
    *frame_str = json_str;
    return json_str_len;
}

static int32 doubao_stt_send_audio_only_request(struct llm_session_stt *stt_session, struct llm_data *data, char **frame_str)
{
    char *aduio = NULL;
    int32 audio_len = RET_ERR;
    unsigned char audio_header[8];

    aduio = data->buff1;
    audio_len = data->buff1_len;
    uint8 is_end = data->transfer_state == LLM_DATA_STATE_END;

    if (aduio == NULL || audio_len <= 0) {
        llm_err("audio data err!");
        return RET_ERR;
    }

    audio_header[0] = 0x11; // Protocol version: 0b0001, Header size: 0b0001 (4 bytes)
    audio_header[1] = 0x20 | (is_end << 1); // Message type: audio only request (0b0010), Message type specific flags: last (0b0010)
    audio_header[2] = 0x10; // Message serialization method: JSON (0b0001), Message compression: None (0b0000)
    audio_header[3] = 0x00; // Reserved
    uint32_t audio_payload_length = os_htonl(audio_len);
    os_memcpy(audio_header + 4, &audio_payload_length, sizeof(audio_payload_length));
    os_memcpy(aduio, audio_header, sizeof(audio_header));
    audio_len += 8;
    *frame_str = aduio;
    return audio_len;
}

static int32 doubao_stt_create_frame(struct llm_session_stt *stt_session, struct llm_data *data, char **frame_str)
{
    int32 data_len = RET_ERR;
    struct doubao_stt_private_params *private_params = NULL;
    private_params = (struct doubao_stt_private_params *)stt_session->private;

    switch (data->type) {
        case LLM_DATA_TYPE_AUDIO: {
            if (data->transfer_state == LLM_DATA_STATE_START) {
                data_len = doubao_stt_send_full_client_request(stt_session, frame_str);
                if (private_params && data->buff1 && data->buff1_len > 0) {
                    if (private_params->temp_data) {
                        llm_free(private_params->temp_data);
                        private_params->temp_data = NULL;
                        private_params->temp_data_len = 0;
                    }
                    private_params->temp_data = llm_malloc(data->buff1_len);
                    if (private_params->temp_data == NULL) {
                        if (*frame_str) {
                            llm_free(*frame_str);
                            *frame_str = NULL;
                        }
                        return LLME_NOMEM;
                    }
                    os_memcpy(private_params->temp_data, data->buff1, data->buff1_len);
                    private_params->temp_data_len = data->buff1_len;
                }
            } else if (data->transfer_state == LLM_DATA_STATE_MIDDLE) {
                data_len = doubao_stt_send_audio_only_request(stt_session, data, frame_str);
            } else if (data->transfer_state == LLM_DATA_STATE_END) {
                data_len = doubao_stt_send_audio_only_request(stt_session, data, frame_str);
            } else {
                llm_err("Warning: Unknow transfer state:%d\n", data->transfer_state);
                if (data->buff1) { llm_free(data->buff1); }
                data->buff1 = NULL;
                data->buff1_len = 0;
            }
            break;
        }
        default: {
            llm_err("Warning: Unknow file type:%d\n", data->type);
            if (data->buff1) { llm_free(data->buff1); }
            data->buff1 = NULL;
            data->buff1_len = 0;
            break;
        }
    }
    if (data->transfer_state == LLM_DATA_STATE_START &&
        data_len > 0) {
        _os_printf("doubao_stt_create_frame: len = %d\n", data_len);
        hgprintf_out(*frame_str+8, data_len-8, 0);
        _os_printf("\r\n");
    }
    return data_len;
}

static int32 doubao_stt_send(void *session, struct llm_data *data)
{
    int32 ret = LLME_AGAIN;
    struct llm_session_stt *stt_session = NULL;
    struct doubao_stt_private_params *private_params = NULL;
    char *frame_str = NULL;
    int32 frame_len = 0;

    llm_send_meta smeta = {
        .flags = LLMWS_BINARY,
    };

    if (!session || !data) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    stt_session = (struct llm_session_stt *)session;
    if (!stt_session->base.handle) {
        llm_err("Error!Session not connect!\n");
        return RET_ERR;
    }
    if (!stt_session->base.platform_config || !stt_session->base.transfer_config) {
        llm_err("Error!Platform or transfer not config!\n");
        return RET_ERR;
    }
    private_params = (struct doubao_stt_private_params *)stt_session->private;

    if (data->offset == -1) {
        frame_len = doubao_stt_create_frame(stt_session, data, &frame_str);
        if (frame_str && frame_len > 0) {
            if (data->buff1)
            { llm_free(data->buff1); }
            data->buff1 = frame_str;
            data->buff1_len = frame_len;
        } else {
            if (frame_len < 0) {
                ret = frame_len;
                return ret;
            }
        }
    } else {
        frame_str = data->buff1 + data->offset;
        frame_len = data->buff1_len - data->offset;
    }

    if (frame_str == NULL || frame_len <= 0) {
        llm_err("frame_str is err!\n");
        return RET_ERR;
    }

    ret = llm_websocket_psend(stt_session->base.handle, frame_str, frame_len, &smeta);
    llm_dbg("send data %d : %d!\n", frame_len, smeta.sent);
    if (frame_len != smeta.sent) {
        if (data->offset == -1) { data->offset = 0; }
        if (smeta.sent > 0) {
            data->offset += smeta.sent;
        }
    } else {
        if (private_params && private_params->temp_data && private_params->temp_data_len > 0) {
            if (data->buff1) {
                llm_free(data->buff1);
            }
            data->buff1 = private_params->temp_data;
            data->buff1_len = private_params->temp_data_len;
            data->offset = 0;
            private_params->temp_data = NULL;
            private_params->temp_data_len = 0;
        } else {
            if (data->buff1) { 
                llm_free(data->buff1); 
                data->buff1 = NULL;
            }
            data->buff1_len = 0;
            data->offset = -1;
        }
    }
    llm_dbg("data status %d : %d!\n", data->offset, data->buff1_len);
    return ret;
}

static int32 doubao_stt_recycle(void *session, uint8 param)
{
    struct llm_session_stt *stt_session = NULL;
    struct doubao_stt_private_params *private_params = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    stt_session = (struct llm_session_stt *)session;
    private_params = (struct doubao_stt_private_params *)stt_session->private;
    if (private_params) {
        if (private_params->temp_data) {
            llm_free(private_params->temp_data);
            private_params->temp_data = NULL;
        }
        private_params->temp_data_len = 0;
        doubao_stt_ws_ctx_free(&private_params->ws_ctx);
    }
    os_memset(&private_params->asr_stt_header, 0, sizeof(struct Doubao_STT_Header));
    return RET_OK;
}

static int32 doubao_stt_connect(void *session)
{
    int32 ret = RET_OK;
    struct llm_session_stt *stt_session = NULL;
    struct Doubao_STT_Cfg *asr_stt_cfg = NULL;
    uint16 auth_header_length = 0;
    void *header = NULL;
    char *auth_header = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    stt_session = (struct llm_session_stt *)session;

    if (!stt_session->base.platform_config || !stt_session->base.transfer_config) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }

    asr_stt_cfg = (struct Doubao_STT_Cfg *)stt_session->base.platform_config;

    // 创建 HEADER
    auth_header_length = os_strlen("Authorization: Bearer; ") + os_strlen(asr_stt_cfg->app_token) + 1;
    auth_header = (char *)llm_malloc(auth_header_length);
    if (auth_header == NULL) {
        llm_err("auth_header alloc failed\r\n");
        ret = LLME_NOMEM;
        goto __cleanup;
    }

    os_snprintf(auth_header, auth_header_length, "Authorization: Bearer; %s", asr_stt_cfg->app_token);
    header = llm_build_header(header, auth_header);
    if (header == NULL) {
        llm_err("header alloc failed\r\n");
        ret = RET_ERR;
        goto __cleanup;
    }

    stt_used_times = os_jiffies();
    stt_session->base.handle = llm_websocket_connect(asr_stt_cfg->url, header, stt_session->base.transfer_config);
    if (stt_session->base.handle == NULL) {
        os_printf("STT connect fail(%dms)\r\n", os_jiffies_to_msecs(os_jiffies() - stt_used_times));
        ret = RET_ERR;
        llm_stt_event(stt_session, LLM_EVENT_CONN_ERR, 0, 0);
        goto __cleanup;
    }
    os_printf("STT connect(%dms)\r\n", os_jiffies_to_msecs(os_jiffies() - stt_used_times));
    llm_free(auth_header);
    llm_free_header(header);
    llm_stt_event(stt_session, LLM_EVENT_CONNECTED, 0, 0);
    return ret;

__cleanup:
    if (auth_header) { llm_free(auth_header); }
    if (header) { llm_free_header(header); }
    if (stt_session->base.handle) {
        llm_websocket_disconnect(stt_session->base.handle);
        stt_session->base.handle = NULL;
    }
    return ret;
}

static int32 doubao_stt_disconnect(void *session)
{
    struct llm_session_stt *stt_session = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    stt_session = (struct llm_session_stt *)session;

    if (stt_session->base.handle) {
        llm_websocket_disconnect(stt_session->base.handle);
        stt_session->base.handle = NULL;
    }
    llm_stt_event(stt_session, LLM_EVENT_DISCONNECT, 0, 0);
    return RET_OK;
}

static int32 doubao_stt_init(void *session)
{
    int32 ret = RET_OK;
    struct llm_session_stt *stt_session = NULL;
    struct doubao_stt_private_params *private_params = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    stt_session = (struct llm_session_stt *)session;

    private_params = llm_zalloc(sizeof(struct doubao_stt_private_params));
    if (!private_params) {
        llm_err("Error,no memory!\n");
        ret = LLME_NOMEM;
        goto __failed;
    }

    stt_session->private  = private_params;
    private_params->stt_session = stt_session;
    return RET_OK;

__failed:
    if (private_params) {
        llm_free(private_params);
    }
    stt_session->private = NULL;
    return ret;
}

static int32 doubao_stt_deinit(void *session)
{
    struct llm_session_stt *stt_session = NULL;
    struct doubao_stt_private_params *private_params = NULL;
    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    stt_session = (struct llm_session_stt *)session;

    private_params = (struct doubao_stt_private_params *)stt_session->private;
    if (private_params) {
        doubao_stt_ws_ctx_free(&private_params->ws_ctx);
        llm_free(private_params);
    }
    stt_session->private = NULL;
    return RET_OK;
}

const struct llm_model_data doubao_stt = {
    .headsize   = 8,
    .init       = doubao_stt_init,
    .deinit     = doubao_stt_deinit,
    .connect    = doubao_stt_connect,
    .disconnect = doubao_stt_disconnect,
    .recycle    = doubao_stt_recycle,
    .upload     = NULL,
    .send       = doubao_stt_send,
    .recv       = doubao_stt_recv,
};

