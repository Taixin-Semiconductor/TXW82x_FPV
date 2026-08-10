#include "llm.h"
#include "cJSON.h"

uint64  tts_used_times = 0;

struct doubao_tts_private_params {
    void *tts_session;
    WsContext ws_ctx;
    struct Doubao_TTS_Header asr_tts_header;
};

#define AUDIO_CHUNK_SIZE            1024        // 音频固定分片写入大小

static void doubao_tts_ws_ctx_free(WsContext *ws_ctx)
{
    if (ws_ctx == NULL) { return; }
    if (ws_ctx->buffer) {
        llm_free(ws_ctx->buffer);
        ws_ctx->buffer = NULL;
    }
    ws_ctx->total_size = 0;
    ws_ctx->is_final = 0;
}

static int32 doubao_tts_errmsg_check(struct llm_session_tts *tts_session, char *buff, uint32 buff_len)
{
    if (buff && buff_len > 0) {
        if (os_strncasestr(buff, "\"error\":\"", -1)) {
            llm_tts_event(tts_session, LLM_EVENT_ERROR_MSG, (uint32)buff, buff_len);
            return RET_OK;
        }
    }
    return RET_ERR;
}

static int32 doubao_tts_process_audio(struct llm_session_tts *tts_session, char *buffer, size_t length)
{
    int32 ret = RET_OK;
    uint32 written = 0;
    uint32 to_write = 0;
    uint32 wait_cnt = 0;

    while (written < length) {
        to_write = (length - written > AUDIO_CHUNK_SIZE) ? AUDIO_CHUNK_SIZE : (length - written);
        if (RB_IDLE(&tts_session->tts_rx) < to_write) {
            if (llm_tts_get_state(tts_session) == LLM_TTS_STATE_INTERRUPTING) {
                ret = LLME_INTR;
                break;
            }
            os_sleep_ms(10);
            wait_cnt++;
            if (wait_cnt == 10) {
                wait_cnt = 0;
                llm_err("Waiting for the application to read audio!(%d:%d)\r\n", RB_IDLE(&tts_session->tts_rx), to_write);
            }
        } else {
            rbuffer_set(&tts_session->tts_rx, (void *)buffer + written, to_write);
            written += to_write;
        }
    }
    return ret;
}

static int32 doubao_tts_handle_received_message(struct llm_session_tts *tts_session, char *buffer, size_t length, uint8 type)
{
    int32 ret = RET_ERR;
    struct doubao_tts_private_params *private_params = (struct doubao_tts_private_params *)tts_session->private;

    if (type) {
        if (!buffer || length < 4) {
            llm_err("Received message is null or too short.\n");
            return RET_ERR;
        }

        private_params->asr_tts_header.protocol_version = buffer[0] & 0xF0;
        private_params->asr_tts_header.header_size = buffer[0] & 0x0F;
        private_params->asr_tts_header.message_type = (buffer[1] >> 4) & 0x0F;
        private_params->asr_tts_header.specific_flags = buffer[1] & 0x0F;
        private_params->asr_tts_header.serialization_method = buffer[2] & 0xF0;
        private_params->asr_tts_header.compression = buffer[2] & 0x0F;
        private_params->asr_tts_header.reserved = buffer[3];
        int32 sequence_number;
        int32 playload_size;
        // 根据消息类型处理不同的响应
        switch (private_params->asr_tts_header.message_type) {
            case 0xb: // Audio-only server response (ACK)
                // 大端序
                os_memcpy(&sequence_number, buffer + 4, sizeof(sequence_number));
                private_params->asr_tts_header.sequence_number = os_ntohl(sequence_number);
                os_memcpy(&playload_size, buffer + 8, sizeof(playload_size));
                private_params->asr_tts_header.playload_size = os_ntohl(playload_size);

                llm_dbg("/*******************************/\r\n");
                llm_dbg("Protocol version:%d\r\n", private_params->asr_tts_header.protocol_version);
                llm_dbg("Header size:%d\r\n", private_params->asr_tts_header.header_size * 4);
                llm_dbg("Message type:0x%x\r\n", private_params->asr_tts_header.message_type);
                llm_dbg("Message type specific flags:%d\r\n", private_params->asr_tts_header.specific_flags);
                llm_dbg("Message serialization method:%d\r\n", private_params->asr_tts_header.serialization_method);
                llm_dbg("Message compression:%d\r\n", private_params->asr_tts_header.compression);
                llm_dbg("Sequence number:%d\r\n", private_params->asr_tts_header.sequence_number);
                llm_dbg("Payload size:%d\r\n", private_params->asr_tts_header.playload_size);
                llm_dbg("/*******************************/\r\n");
                if (private_params->asr_tts_header.sequence_number == 0) {
                    llm_dbg("Audio-only server ACK\n");
                    tts_session->wpos_old = tts_session->tts_rx.wpos;
                    ret = RET_OK;
                } else {
                    llm_dbg("Audio-only server response\n");
                    os_printf("TTS(%dms)\r\n", os_jiffies_to_msecs(os_jiffies() - tts_used_times));                    
                    ret = doubao_tts_process_audio(tts_session, buffer + 12, length - 12);
                    if (ret == RET_ERR) {
                        ret = doubao_tts_errmsg_check(tts_session, buffer + 12, length - 12);
                    }
                }
                break;
            case 0xf: // Error Message from Server
                llm_err("Error Message from Server\n");
                ret = doubao_tts_errmsg_check(tts_session, buffer + 12, length - 12);
                break;
            default:
                llm_err("Unknown message type received.\n");
        }
    } else {
        ret = doubao_tts_process_audio(tts_session, buffer, length);
    }
    return ret;
}

static int32 doubao_tts_handle_websocket(struct llm_session_tts *tts_session, char *chunk, size_t length)
{
    int32 ret = RET_OK;
    llm_recv_meta rmeta = {0};
    struct doubao_tts_private_params *private_params = (struct doubao_tts_private_params *)tts_session->private;

    if (chunk == NULL || length == 0) {
        llm_err("Invalid input params (chunk null / length=0)\r\n");
        return RET_ERR;
    }


    // 预处理WebSocket数据，解析帧信息
    ret = llm_websocket_precv(tts_session->base.handle, chunk, length, &rmeta);
    if (ret != RET_OK) {
        return ret;
    }

    if (rmeta.flags & LLMWS_BINARY) {    // 处理 bin 帧
        if (rmeta.offset == 0) {
            ret = doubao_tts_handle_received_message(tts_session, chunk, rmeta.nread, 1);
        } else {
            ret = doubao_tts_handle_received_message(tts_session, chunk, rmeta.nread, 0);
        }
        if (rmeta.bytesleft == 0) {
            if (private_params->asr_tts_header.sequence_number < 0) {
                os_printf("TTS(%dms)\r\n", os_jiffies_to_msecs(os_jiffies() - tts_used_times));
                llm_tts_event(tts_session, LLM_EVENT_TTS_RESULT, 0, 0);
            }
        }
    }
    // 处理关闭帧
    else if (rmeta.flags & LLMWS_CLOSE) {
        llm_err("WS server requested close connection\r\n");
        doubao_tts_ws_ctx_free(&private_params->ws_ctx); // 释放上下文资源
        return RET_ERR;
    }
    // 不支持的帧类型
    else {
        llm_err("Unsupported WS frame type (flags: 0x%02x)\r\n", rmeta.flags);
        doubao_tts_ws_ctx_free(&private_params->ws_ctx); // 释放上下文资源
        return RET_ERR;
    }

//    // 处理完整消息（所有分片接收完成）
//    if (private_params->ws_ctx.is_final && private_params->ws_ctx.total_size > 0) {
//        private_params->ws_ctx.buffer[private_params->ws_ctx.total_size] = '\0';
//        // 处理JSON消息
//        ret = doubao_tts_handle_received_message(tts_session, private_params->ws_ctx.buffer, private_params->ws_ctx.total_size);
//        doubao_tts_ws_ctx_free(&private_params->ws_ctx);
//    }

    return ret;
}

static int32 doubao_tts_recv(void *session, char *buff, uint32 buffer_size)
{
    struct llm_session_tts *tts_session = NULL;
    if (!session || !buff || buffer_size <= 0) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    tts_session = (struct llm_session_tts *)session;

    return doubao_tts_handle_websocket(tts_session, buff, buffer_size);
}

static int32 doubao_tts_send_full_client_request(struct llm_session_tts *tts_session, char *text, char **frame_str)
{
    int32 json_str_len = RET_ERR;
    char *json_str = NULL;
    char *json_str_temp = NULL;
    struct Doubao_TTS_Cfg *asr_tts_cfg = (struct Doubao_TTS_Cfg *)tts_session->base.platform_config;

    uint8 temp[16] = {0};
    char reqid[32 + 4] = {0};
    os_random_bytes(temp, 16);
    key_str(temp, 16, reqid);

    // 创建 cJSON 对象
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        llm_err("Failed to create cJSON object\r\n");
        return LLME_NOMEM;
    }

    // 创建 "app" 对象
    cJSON *app = cJSON_CreateObject();
    cJSON_AddStringToObject(app, "appid", asr_tts_cfg->app_appid);
    cJSON_AddStringToObject(app, "token", asr_tts_cfg->app_token);
    cJSON_AddStringToObject(app, "cluster", asr_tts_cfg->app_cluster);
    cJSON_AddItemToObject(root, "app", app);

    // 创建 "user" 对象
    cJSON *user = cJSON_CreateObject();
    cJSON_AddStringToObject(user, "uid", asr_tts_cfg->user_uid);
    cJSON_AddItemToObject(root, "user", user);

    // 创建 "audio" 对象
    cJSON *audio = cJSON_CreateObject();
    cJSON_AddStringToObject(audio, "voice_type", asr_tts_cfg->audio_voice_type);
    if (asr_tts_cfg->audio_rate) {
        cJSON_AddNumberToObject(audio, "rate", os_atoi(asr_tts_cfg->audio_rate));
    }
    if (asr_tts_cfg->audio_encoding) {
        cJSON_AddStringToObject(audio, "encoding", asr_tts_cfg->audio_encoding);
    }
    if (asr_tts_cfg->audio_compression_rate) {
        cJSON_AddNumberToObject(audio, "compression_rate", os_atoi(asr_tts_cfg->audio_compression_rate));
    }
    if (asr_tts_cfg->audio_speed_ratio) {
        cJSON_AddNumberToObject(audio, "speed_ratio", os_atof(asr_tts_cfg->audio_speed_ratio));
    }
    if (asr_tts_cfg->audio_volume_ratio) {
        cJSON_AddNumberToObject(audio, "volume_ratio", os_atof(asr_tts_cfg->audio_volume_ratio));
    }
    if (asr_tts_cfg->audio_pitch_ratio) {
        cJSON_AddNumberToObject(audio, "pitch_ratio", os_atof(asr_tts_cfg->audio_pitch_ratio));
    }
    if (asr_tts_cfg->audio_emotion) {
        cJSON_AddStringToObject(audio, "emotion", asr_tts_cfg->audio_emotion);
    }
    if (asr_tts_cfg->audio_language) {
        cJSON_AddStringToObject(audio, "language", asr_tts_cfg->audio_language);
    }
    cJSON_AddItemToObject(root, "audio", audio);

    // 创建 "request" 对象
    cJSON *request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "reqid", reqid);
    cJSON_AddStringToObject(request, "text", text);
    cJSON_AddStringToObject(request, "operation", "submit");
    if (asr_tts_cfg->silence_duration) {
        cJSON_AddNumberToObject(request, "silence_duration", os_atoi(asr_tts_cfg->silence_duration));
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

static int32 doubao_tts_create_frame(struct llm_session_tts *tts_session, struct llm_data *data, char **frame_str)
{
    int32 data_len = RET_ERR;
    if (tts_session->base.new_dialogue == 1) {
        return LLME_AGAIN;
    }

    switch (data->type) {
        case LLM_DATA_TYPE_TEXT: {
            if (data->transfer_state == LLM_DATA_STATE_START) {
                if (data->buff1 && data->buff1_len > 0) {
                    data_len = doubao_tts_send_full_client_request(tts_session, data->buff1, frame_str);
                } else {
                    if (data->buff1) { llm_free(data->buff1); }
                    data->buff1_len = 0;
                }
            } else if (data->transfer_state == LLM_DATA_STATE_MIDDLE ||
                       data->transfer_state == LLM_DATA_STATE_END) {
                if (data->buff1 && data->buff1_len > 0) {
                    data_len = doubao_tts_send_full_client_request(tts_session, data->buff1, frame_str);
                } else {
                    llm_err("Warning: Invalid data:%d\r\n", data->buff1_len);
                    if (data->buff1) { llm_free(data->buff1); }
                    data->buff1_len = 0;
                }
            } else {
                llm_err("Warning: Unknow transfer state:%d\n", data->transfer_state);
                if (data->buff1) { llm_free(data->buff1); }
                data->buff1 = NULL;
                data->buff1_len = 0;
            }
            break;
        }
        default: {
            llm_err("Warning: Unknow file type:%d\r\n", data->type);
            if (data->buff1) { llm_free(data->buff1); }
            data->buff1 = NULL;
            data->buff1_len = 0;
            break;
        }
    }
    if (data_len > 0) {
        tts_session->base.new_dialogue = 1;
        _os_printf("doubao_tts_create_frame: len = %d\n", data_len);
        hgprintf_out(*frame_str+8, data_len-8, 0);
        _os_printf("\r\n");
    }
    return data_len;
}

static int32 doubao_tts_send(void *session, struct llm_data *data)
{
    int32 ret = LLME_AGAIN;
    struct llm_session_tts *tts_session = NULL;
    char *frame_str = NULL;
    int32 frame_len = 0;

    llm_send_meta smeta = {
        .flags = LLMWS_BINARY,
    };

    if (!session || !data) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    tts_session = (struct llm_session_tts *)session;
    if (!tts_session->base.handle) {
        llm_err("Error!Session not connect!\n");
        return RET_ERR;
    }
    if (!tts_session->base.platform_config || !tts_session->base.transfer_config) {
        llm_err("Error!Platform or transfer not config!\n");
        return RET_ERR;
    }

    if (data->offset == -1) {
        frame_len = doubao_tts_create_frame(tts_session, data, &frame_str);
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

    ret = llm_websocket_psend(tts_session->base.handle, frame_str, frame_len, &smeta);
    llm_dbg("send data %d : %d!\n", frame_len, smeta.sent);
    if (frame_len != smeta.sent) {
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

static int32 doubao_tts_recycle(void *session, uint8 param)
{
    struct llm_session_tts *tts_session = NULL;
    struct doubao_tts_private_params *private_params = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    tts_session = (struct llm_session_tts *)session;
    private_params = (struct doubao_tts_private_params *)tts_session->private;
    if (private_params) {
        doubao_tts_ws_ctx_free(&private_params->ws_ctx);
    }
    os_memset(&private_params->asr_tts_header, 0, sizeof(struct Doubao_TTS_Header));
    return RET_OK;
}

static int32 doubao_tts_connect(void *session)
{
    int32 ret = RET_OK;
    struct llm_session_tts *tts_session = NULL;
    struct Doubao_TTS_Cfg *asr_tts_cfg = NULL;
    uint16 auth_header_length = 0;
    void *header = NULL;
    char *auth_header = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    tts_session = (struct llm_session_tts *)session;

    if (!tts_session->base.platform_config || !tts_session->base.transfer_config) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }

    asr_tts_cfg = (struct Doubao_TTS_Cfg *)tts_session->base.platform_config;

    // 创建 HEADER
    auth_header_length = os_strlen("Authorization: Bearer; ") + os_strlen(asr_tts_cfg->app_token) + 1;
    auth_header = (char *)llm_malloc(auth_header_length);
    if (auth_header == NULL) {
        llm_err("auth_header alloc failed\r\n");
        ret = LLME_NOMEM;
        goto __cleanup;
    }

    os_snprintf(auth_header, auth_header_length, "Authorization: Bearer; %s", asr_tts_cfg->app_token);
    header = llm_build_header(header, auth_header);
    if (header == NULL) {
        llm_err("header alloc failed\r\n");
        ret = RET_ERR;
        goto __cleanup;
    }

    tts_used_times = os_jiffies();
    tts_session->base.handle = llm_websocket_connect(asr_tts_cfg->url, header, tts_session->base.transfer_config);
    if (tts_session->base.handle == NULL) {
        os_printf("TTS connect fail(%dms)\r\n", os_jiffies_to_msecs(os_jiffies() - tts_used_times));
        ret = RET_ERR;
        llm_tts_event(tts_session, LLM_EVENT_CONN_ERR, 0, 0);
        goto __cleanup;
    }
    os_printf("TTS connect(%dms)\r\n", os_jiffies_to_msecs(os_jiffies() - tts_used_times));
    llm_free(auth_header);
    llm_free_header(header);
    llm_tts_event(tts_session, LLM_EVENT_CONNECTED, 0, 0);
    return ret;

__cleanup:
    if (auth_header) { llm_free(auth_header); }
    if (header) { llm_free_header(header); }
    if (tts_session->base.handle) {
        llm_websocket_disconnect(tts_session->base.handle);
        tts_session->base.handle = NULL;
    }
    return ret;
}

static int32 doubao_tts_disconnect(void *session)
{
    struct llm_session_tts *tts_session = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    tts_session = (struct llm_session_tts *)session;

    if (tts_session->base.handle) {
        llm_websocket_disconnect(tts_session->base.handle);
        tts_session->base.handle = NULL;
    }
    llm_tts_event(tts_session, LLM_EVENT_DISCONNECT, 0, 0);
    tts_session->base.new_dialogue = 0;
    return RET_OK;
}

static int32 doubao_tts_init(void *session)
{
    int32 ret = RET_OK;
    struct llm_session_tts *tts_session = NULL;
    struct doubao_tts_private_params *private_params = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    tts_session = (struct llm_session_tts *)session;

    private_params = llm_zalloc(sizeof(struct doubao_tts_private_params));
    if (!private_params) {
        llm_err("Error,no memory!\n");
        ret = LLME_NOMEM;
        goto __failed;
    }

    tts_session->private  = private_params;
    private_params->tts_session = tts_session;
    return RET_OK;

__failed:
    if (private_params) {
        llm_free(private_params);
    }
    tts_session->private = NULL;
    return ret;
}

static int32 doubao_tts_deinit(void *session)
{
    struct llm_session_tts *tts_session = NULL;
    struct doubao_tts_private_params *private_params = NULL;
    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    tts_session = (struct llm_session_tts *)session;

    private_params = (struct doubao_tts_private_params *)tts_session->private;
    if (private_params) {
        doubao_tts_ws_ctx_free(&private_params->ws_ctx);
        llm_free(private_params);
    }
    tts_session->private = NULL;
    return RET_OK;
}

const struct llm_model_data doubao_tts = {
    .headsize   = 0,
    .init       = doubao_tts_init,
    .deinit     = doubao_tts_deinit,
    .connect    = doubao_tts_connect,
    .disconnect = doubao_tts_disconnect,
    .recycle    = doubao_tts_recycle,    
    .upload     = NULL,
    .send       = doubao_tts_send,
    .recv       = doubao_tts_recv,
};

