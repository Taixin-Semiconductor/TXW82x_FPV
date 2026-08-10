#include "llm.h"
#include "cJSON.h"

#define SAFE_GET_STRING(obj, key) ({ \
        cJSON *_item = cJSON_GetObjectItem(obj, key); \
        (_item && cJSON_IsString(_item) ? _item->valuestring : NULL); \
    })

uint64 tti_used_times = 0;

struct doubao_tti_private_params {
    void *tti_session;
    WsContext ws_ctx;
};

static void doubao_tti_ws_ctx_free(WsContext *ws_ctx)
{
    if (ws_ctx == NULL) { return; }
    if (ws_ctx->buffer) {
        llm_free(ws_ctx->buffer);
        ws_ctx->buffer = NULL;
    }
    ws_ctx->total_size = 0;
    ws_ctx->is_final = 0;
}

static size_t doubao_tti_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t length = size * nmemb;
    struct llm_session_tti *tti_session = NULL;
    struct doubao_tti_private_params *private_params = NULL;

    if (length == 0 || userp == NULL) {
        return 0;
    }
    tti_session = (struct llm_session_tti *)userp;
    private_params = (struct doubao_tti_private_params *)tti_session->private;
    if (private_params == NULL) {
        llm_err("Input param error!\n");
        return 0;
    }

    char *new_buffer = llm_realloc(private_params->ws_ctx.buffer, private_params->ws_ctx.total_size + length + 1);
    if (new_buffer == NULL) {
        llm_err("no memory!\r\n");
        return 0;
    }

    os_memcpy(new_buffer + private_params->ws_ctx.total_size, contents, length);
    private_params->ws_ctx.buffer = new_buffer;
    private_params->ws_ctx.total_size += length;
    private_params->ws_ctx.buffer[private_params->ws_ctx.total_size] = '\0';
    llm_dbg("recv: %d!\r\n", length);
    return length;
}

static int doubao_tti_xferinfo_callback(void *clientp,
        long dltotal,
        long dlnow,
        long ultotal,
        long ulnow)
{

    struct llm_session_tti *tti_session = (struct llm_session_tti *)clientp;

    if (llm_tti_get_state(tti_session) == LLM_TTI_STATE_INTERRUPTING) {
        return 1;
    }
    return 0x10000001; // 继续传输
}

static int32 doubao_tti_errmsg_check(struct llm_session_tti *tti_session, char *buff, uint32 buff_len)
{
    if (buff && buff_len > 0) {
        if (os_strncasestr(buff, "\"error\":\"", -1)) {
            llm_tti_event(tti_session, LLM_EVENT_ERROR_MSG, (uint32)buff, buff_len);
            return RET_OK;
        }
    }
    return RET_ERR;
}

static int32 doubao_tti_parse_json(struct llm_session_tti *tti_session, char *data, uint32 data_len)
{
    int32 ret = RET_ERR;
    unsigned char *decoded_image_buf = NULL;
    uint32 decoded_image_len = 0;
    uint32 b64_json_len = 0;
    char *b64_json_start = os_strstr(data, "\"b64_json\":\"");
    if (b64_json_start) {
        b64_json_start += os_strlen("\"b64_json\":\"");
        char *b64_json_end = os_strchr(b64_json_start, '"');
        if (b64_json_end) {
            *b64_json_end = '\0';
            b64_json_len = os_strlen(b64_json_start);
            b64_json_start[b64_json_len] = 0;
            if (b64_json_len > 0) {
                ret = llm_base64_decode(b64_json_start, &decoded_image_buf, &decoded_image_len);
                if (ret != RET_OK || decoded_image_buf == NULL || decoded_image_len == 0) {
                    llm_err("Base64 decode failed (ret=%d, len=%d)\r\n", ret, decoded_image_len);
                    return RET_ERR;
                }
                os_printf("TTI(%dms)\r\n", os_jiffies_to_msecs(os_jiffies() - tti_used_times));
                llm_tti_event(tti_session, LLM_EVENT_TTI_RESULT, (uint32)decoded_image_buf, decoded_image_len);
                return RET_OK;
            }
        }
    }
    return ret;
}

static int32 doubao_tti_recv(void *session, char *buff, uint32 buffer_size)
{
    int32 ret = LLME_AGAIN;
    struct llm_session_tti *tti_session = NULL;
    struct doubao_tti_private_params *private_params = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    tti_session = (struct llm_session_tti *)session;
    private_params = (struct doubao_tti_private_params *)tti_session->private;

    ret = llm_https_recv(tti_session->base.handle, NULL, 0, NULL);

    if (private_params->ws_ctx.buffer && private_params->ws_ctx.total_size > 0) {
        llm_err("recv done(%d)!\r\n", private_params->ws_ctx.total_size);
//        hgprintf_out(private_params->ws_ctx.buffer, private_params->ws_ctx.total_size, 0);
//        _os_printf("\r\n");

        ret = doubao_tti_parse_json(tti_session, private_params->ws_ctx.buffer, private_params->ws_ctx.total_size);
        if (ret == RET_ERR) {
            ret = doubao_tti_errmsg_check(tti_session, private_params->ws_ctx.buffer, private_params->ws_ctx.total_size);
        }
    }
    return ret;
}

static uint32 doubao_tti_build_body(struct llm_session_tti *tti_session, struct llm_data *ll_file, char **body)
{
    struct Doubao_TTI_Cfg *llm_tti_cfg = (struct Doubao_TTI_Cfg *)tti_session->base.platform_config;
    char *prompt = ll_file->buff1;
    uint32 json_str_len = 0;
    char *json_str_temp = NULL;

    // 创建 cJSON 对象
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        llm_err("Failed to create cJSON object\r\n");
        return 0;
    }

    // 设置 model 字段
    cJSON_AddStringToObject(root, "model", llm_tti_cfg->model);

    // 设置 prompt 字段
    cJSON_AddStringToObject(root, "prompt", prompt);

    // 设置 response_format 字段
    cJSON_AddStringToObject(root, "response_format", "b64_json");

    // 设置 size 字段
    if (llm_tti_cfg->size) {
        cJSON_AddStringToObject(root, "size", llm_tti_cfg->size);
    }

    // 设置 seed 字段
    if (llm_tti_cfg->seed) {
        cJSON_AddNumberToObject(root, "seed", os_atoi(llm_tti_cfg->seed));
    }

    // 设置 guidance_scale 字段
    if (llm_tti_cfg->guidance_scale) {
        cJSON_AddNumberToObject(root, "guidance_scale", os_atof(llm_tti_cfg->guidance_scale));
    }

    // 设置 watermark 字段
    if (llm_tti_cfg->watermark) {
        cJSON_AddBoolToObject(root, "watermark", os_strcmp(llm_tti_cfg->watermark, "true") == 0 ? true : false);
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

static int32 doubao_tti_send(void *session, struct llm_data *data)
{
    int32 ret = LLME_AGAIN;
    struct llm_session_tti *tti_session = NULL;
    struct Doubao_TTI_Cfg *llm_tti_cfg = NULL;
    char *json_str = NULL;
    uint32 json_len = 0;

    if (!session || !data) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    tti_session = (struct llm_session_tti *)session;
    if (!tti_session->base.handle) {
        llm_err("Error!Session not connect!\n");
        return RET_ERR;
    }
    if (!tti_session->base.platform_config || !tti_session->base.transfer_config) {
        llm_err("Error!Platform or transfer not config!\n");
        return RET_ERR;
    }
    llm_tti_cfg = (struct Doubao_TTI_Cfg *)tti_session->base.platform_config;

    if (data->type == LLM_DATA_TYPE_TEXT && data->buff1 && data->buff1_len > 0) {
        json_len = doubao_tti_build_body(tti_session, data, &json_str);
        if (json_str && json_len) {
            _os_printf("doubao_tti_send len: (%d)\n", json_len);
            hgprintf_out(json_str, json_len, 0);
            _os_printf("\r\n");
            tti_used_times = os_jiffies();
            ret = llm_https_send(tti_session->base.handle, LLM_HTTP_POST, llm_tti_cfg->url, json_str, json_len);
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

static int32 doubao_tti_recycle(void *session, uint8 param)
{
    struct llm_session_tti *tti_session = NULL;
    struct doubao_tti_private_params *private_params = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    tti_session = (struct llm_session_tti *)session;
    private_params = (struct doubao_tti_private_params *)tti_session->private;
    if (private_params) {
        doubao_tti_ws_ctx_free(&private_params->ws_ctx);
    }
    return RET_OK;
}

static int32 doubao_tti_connect(void *session)
{
    int32 ret = RET_OK;
    struct llm_session_tti *tti_session = NULL;
    struct Doubao_TTI_Cfg *llm_tti_cfg = NULL;
    uint16 auth_header_length = 0;
    char *auth_header = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    tti_session = (struct llm_session_tti *)session;

    if (!tti_session->base.platform_config || !tti_session->base.transfer_config) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }

    llm_tti_cfg = (struct Doubao_TTI_Cfg *)tti_session->base.platform_config;

    // 创建 HEADER
    auth_header_length = os_strlen("Content-Type: application/json\r\nAuthorization: Bearer ") + os_strlen(llm_tti_cfg->ark_api_key) + 1;
    auth_header = (char *)llm_zalloc(auth_header_length);
    if (auth_header == NULL) {
        llm_err("Memory allocation failed\r\n");
        ret = LLME_NOMEM;
        goto __cleanup;
    }
    os_snprintf(auth_header, auth_header_length, "Content-Type: application/json\r\nAuthorization: Bearer %s", llm_tti_cfg->ark_api_key);
    tti_session->base.headers = llm_build_header(tti_session->base.headers, auth_header);
    if (tti_session->base.headers == NULL) {
        llm_err("header alloc failed\r\n");
        ret = LLME_NOMEM;
        goto __cleanup;
    }
    /*tti_session->base.headers = llm_build_header(tti_session->base.headers, "Content-Type: application/json");
    if (tti_session->base.headers == NULL) {
        llm_err("header alloc failed\r\n");
        ret = RET_ERR;
        goto __cleanup;
    }

    auth_header_length = os_strlen("Authorization: Bearer ") + os_strlen(llm_tti_cfg->ark_api_key) + 1;
    auth_header = (char *)llm_malloc(auth_header_length);
    if (auth_header == NULL) {
        llm_err("auth_header alloc failed\r\n");
        ret = LLME_NOMEM;
        goto __cleanup;
    }

    os_snprintf(auth_header, auth_header_length, "Authorization: Bearer %s", llm_tti_cfg->ark_api_key);
    llm_err("auth_header: %s\r\n", auth_header);
    tti_session->base.headers = llm_build_header(tti_session->base.headers, auth_header);
    if (tti_session->base.headers == NULL) {
        llm_err("header alloc failed\r\n");
        ret = RET_ERR;
        goto __cleanup;
    }*/

    tti_used_times = os_jiffies();
    tti_session->base.handle = llm_https_connect(tti_session->base.headers, tti_session->base.transfer_config, 
                               (void *)doubao_tti_write_callback, NULL, (void *)doubao_tti_xferinfo_callback, (void *)tti_session);
    if (tti_session->base.handle == NULL) {
        //os_printf("TTI connect fail(%dms)\r\n", os_jiffies_to_msecs(os_jiffies() - tti_used_times));
        ret = RET_ERR;
        llm_tti_event(tti_session, LLM_EVENT_CONN_ERR, 0, 0);
        goto __cleanup;
    }
    //os_printf("TTI connect(%dms)\r\n", os_jiffies_to_msecs(os_jiffies() - tti_used_times));
    llm_free(auth_header);
    llm_tti_event(tti_session, LLM_EVENT_CONNECTED, 0, 0);
    return ret;

__cleanup:
    if (auth_header) { llm_free(auth_header); }
    if (tti_session->base.headers) {
        llm_free_header(tti_session->base.headers);
        tti_session->base.headers = NULL;
    }
    if (tti_session->base.handle) {
        llm_websocket_disconnect(tti_session->base.handle);
        tti_session->base.handle = NULL;
    }
    return ret;
}

static int32 doubao_tti_disconnect(void *session)
{
    struct llm_session_tti *tti_session = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    tti_session = (struct llm_session_tti *)session;

    if (tti_session->base.headers) {
        llm_free_header(tti_session->base.headers);
        tti_session->base.headers = NULL;
    }
    if (tti_session->base.handle) {
        llm_https_disconnect(tti_session->base.handle);
        tti_session->base.handle = NULL;
    }
    llm_tti_event(tti_session, LLM_EVENT_DISCONNECT, 0, 0);
    return RET_OK;
}

static int32 doubao_tti_init(void *session)
{
    int32 ret = RET_OK;
    struct llm_session_tti *tti_session = NULL;
    struct doubao_tti_private_params *private_params = NULL;

    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    tti_session = (struct llm_session_tti *)session;

    private_params = llm_zalloc(sizeof(struct doubao_tti_private_params));
    if (!private_params) {
        llm_err("Error,no memory!\n");
        ret = LLME_NOMEM;
        goto __failed;
    }

    tti_session->private  = private_params;
    private_params->tti_session = tti_session;
    return RET_OK;

__failed:
    if (private_params) {
        llm_free(private_params);
    }
    tti_session->private = NULL;
    return ret;
}

static int32 doubao_tti_deinit(void *session)
{
    struct llm_session_tti *tti_session = NULL;
    struct doubao_tti_private_params *private_params = NULL;
    if (!session) {
        llm_err("Input param error!\n");
        return RET_ERR;
    }
    tti_session = (struct llm_session_tti *)session;

    private_params = (struct doubao_tti_private_params *)tti_session->private;
    if (private_params) {
        doubao_tti_ws_ctx_free(&private_params->ws_ctx);
        llm_free(private_params);
    }
    tti_session->private = NULL;
    return RET_OK;
}

const struct llm_model_data doubao_tti = {
    .headsize   = 0,
    .init       = doubao_tti_init,
    .deinit     = doubao_tti_deinit,
    .connect    = doubao_tti_connect,
    .disconnect = doubao_tti_disconnect,
    .recycle    = doubao_tti_recycle,
    .upload     = NULL,
    .send       = doubao_tti_send,
    .recv       = doubao_tti_recv,
};

