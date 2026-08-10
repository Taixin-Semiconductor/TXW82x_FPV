#include "neteast_demo.h"

#if NETEAST_DEMO
/* -------------------------- 消息代码 -------------------------- */
typedef enum {
    NETEAST_IOT_VOLUME_RELATIVE = 1,//相对值模式
    NETEAST_IOT_VOLUME_ABSOLUTE = 2,//绝对值模式
} neteast_demo_iot_volume_mode;

/*
 * @brief: 处理Coze AI错误消息
 * @param: char *error 错误消息
 * @return: int32
*/
static int32 neteast_msg_error_process(char *error)
{
    int32 err_code = 0;
    char *err_msg = NULL;

    os_printf("**设备异常**\n");
    if (error) {
        cJSON *root = cJSON_Parse(error);
        if (root == NULL) {
            os_printf("Error before: %s\n", error);
            return RET_ERR;
        }

        cJSON *data = cJSON_GetObjectItemCaseSensitive(root, "data");
        if (cJSON_IsObject(data)) {
            cJSON *code_item = cJSON_GetObjectItemCaseSensitive(data, "code");
            cJSON *msg_item = cJSON_GetObjectItemCaseSensitive(data, "msg");

            if (cJSON_IsNumber(code_item) && cJSON_IsString(msg_item)) {
                err_code = code_item->valueint;
                err_msg = msg_item->valuestring;
            }
        }

        if (err_code == 0 && err_msg == NULL) {
            os_printf("Error after: %s\n", error);
            cJSON_Delete(root);
            return RET_ERR;
        }
        os_printf("\"%d\": \"%s\"\r\n", err_code, err_msg);
        switch (err_code) {
            case 4302: {
                os_printf("**没听清楚，您再说一遍!**\n");
                neteast_mgr.finish = 1;
                neteast_main_set_state(NETEAST_DEMO_STATE_WAITING);
                break;
            }
            default: {
                llm_sts_stop(neteast_mgr.sts_session);
                // 注意：所有调用 llm_sts_reconnect 前必须将状态设置为 NETEAST_DEMO_STATE_IDLE
                //neteast_main_set_state(NETEAST_DEMO_STATE_IDLE);
                //llm_sts_reconnect(neteast_mgr.sts_session);
                break;
            }
        }
        cJSON_Delete(root);
    }
    return RET_OK;
}

// 提取 submit_tool_outputs 中的第一个 tool_call 的 arguments 字段中的 url 字段
static char *neteast_msg_get_audio_url(char *data, uint32 data_len)
{
    char *audio_url = NULL;
    cJSON *root = NULL;
    cJSON *data_obj = NULL;
    cJSON *content_str = NULL;
    cJSON *content_obj = NULL;
    cJSON *url_item = NULL;

    if (!data || data_len == 0) {
        return NULL;
    }

    root = cJSON_Parse(data);
    if (!root) {
        const char *errorPtr = cJSON_GetErrorPtr();
        if (errorPtr != NULL) {
            os_printf("JSON parse error before: %s\n", errorPtr);
        }
        return NULL;
    }

    data_obj = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (!data_obj || !cJSON_IsObject(data_obj)) {
        os_printf("No data object found\n");
        goto cleanup;
    }

    content_str = cJSON_GetObjectItemCaseSensitive(data_obj, "content");
    if (!content_str || !cJSON_IsString(content_str)) {
        os_printf("No content string found\n");
        goto cleanup;
    }

    content_obj = cJSON_Parse(content_str->valuestring);
    if (!content_obj) {
        const char *errorPtr = cJSON_GetErrorPtr();
        if (errorPtr != NULL) {
            os_printf("Content JSON parse error before: %s\n", errorPtr);
        }
        goto cleanup;
    }

    url_item = cJSON_GetObjectItemCaseSensitive(content_obj, "url");
    if (url_item && cJSON_IsString(url_item)) {
        audio_url = llm_strdup(url_item->valuestring);
        os_printf("get audio url:%s\n", audio_url);
    } else {
        os_printf("No url found in content\n");
    }

    if (content_obj) {
        cJSON_Delete(content_obj);
    }

cleanup:
    if (root) {
        cJSON_Delete(root);
    }
    return audio_url;
}
/*
 * @brief: 处理Coze AI点歌指令
 * @param: char *data 点歌指令
 * @param: uint32 data_len 点歌指令长度
 * @return: int32
*/
static int32 neteast_msg_mplayer_process(char *data, uint32 data_len)
{
    struct txmplayer_param param = {
        .volume = neteast_mgr.volume,
    };

    char *audio_url = neteast_msg_get_audio_url(data, data_len);
    if (audio_url) {
        if (neteast_mgr.txmplayer_hdl >= 0) {
            txmplayer_close(neteast_mgr.txmplayer_hdl);
            neteast_mgr.txmplayer_hdl = -1;
        }
        neteast_mgr.txmplayer_hdl = txmplayer_open(audio_url, 0, &param);
        neteast_mgr.time_stamp = os_jiffies();
        neteast_mgr.pause = 0;
        llm_free(audio_url);
    }
    return RET_OK;
}

// 提取 submit_tool_outputs 中的第一个 tool_call 的 id
static char *neteast_msg_iot_extract_data_id(const char *json_string)
{
    // 解析 JSON 字符串
    cJSON *json = cJSON_Parse(json_string);
    if (json == NULL) {
        os_printf("Error parsing JSON\n");
        return NULL;
    }

    // 获取 "data" 对象
    cJSON *data = cJSON_GetObjectItem(json, "data");
    if (data == NULL) {
        printf("Error: 'data' not found in JSON\n");
        cJSON_Delete(json);
        return NULL;
    }

    // 获取 "id" 字段
    cJSON *id = cJSON_GetObjectItem(data, "id");
    if (id == NULL) {
        printf("Error: 'id' not found in 'data'\n");
        cJSON_Delete(json);
        return NULL;
    }

    // 复制 "id" 的字符串值
    char *id_value = llm_strdup(id->valuestring);

    // 释放 JSON 对象
    cJSON_Delete(json);

    return id_value;  // 返回 id 的副本
}
// 提取 submit_tool_outputs 中的第一个 tool_call 的 id
static char *neteast_msg_iot_extract_submit_tool_outputs_id(const char *json_string)
{
    // 解析 JSON 字符串
    cJSON *json = cJSON_Parse(json_string);
    if (json == NULL) {
        printf("Error parsing JSON\n");
        return NULL;
    }

    // 获取 "data" 对象
    cJSON *data = cJSON_GetObjectItem(json, "data");
    if (data == NULL) {
        printf("Error: 'data' not found in JSON\n");
        cJSON_Delete(json);
        return NULL;
    }

    // 获取 "required_action" 对象
    cJSON *required_action = cJSON_GetObjectItem(data, "required_action");
    if (required_action == NULL) {
        printf("Error: 'required_action' not found in 'data'\n");
        cJSON_Delete(json);
        return NULL;
    }

    // 获取 "submit_tool_outputs" 对象
    cJSON *submit_tool_outputs = cJSON_GetObjectItem(required_action, "submit_tool_outputs");
    if (submit_tool_outputs == NULL) {
        printf("Error: 'submit_tool_outputs' not found in 'required_action'\n");
        cJSON_Delete(json);
        return NULL;
    }

    // 获取 "tool_calls" 数组
    cJSON *tool_calls = cJSON_GetObjectItem(submit_tool_outputs, "tool_calls");
    if (tool_calls == NULL || !cJSON_IsArray(tool_calls)) {
        printf("Error: 'tool_calls' not found or is not an array\n");
        cJSON_Delete(json);
        return NULL;
    }

    // 获取第一个 "tool_calls" 的对象
    cJSON *first_tool_call = cJSON_GetArrayItem(tool_calls, 0);
    if (first_tool_call == NULL) {
        printf("Error: 'tool_calls' array is empty\n");
        cJSON_Delete(json);
        return NULL;
    }

    // 获取 "id" 字段
    cJSON *id = cJSON_GetObjectItem(first_tool_call, "id");
    if (id == NULL) {
        printf("Error: 'id' not found in first tool call\n");
        cJSON_Delete(json);
        return NULL;
    }

    // 复制 "id" 的字符串值
    char *id_value = llm_strdup(id->valuestring);

    // 释放 JSON 对象
    cJSON_Delete(json);

    return id_value;  // 返回 id 的副本
}
//提取 submit_tool_outputs 中的第一个 tool_call 的 arguments 字段
static char *neteast_msg_iot_extract_args(const char *json_string)
{
    // 解析 JSON 字符串
    cJSON *json = cJSON_Parse(json_string);
    if (json == NULL) {
        printf("Error parsing JSON\n");
        return NULL;
    }

    // 获取 "data" 对象
    cJSON *data = cJSON_GetObjectItem(json, "data");
    if (data == NULL) {
        printf("Error: 'data' not found in JSON\n");
        cJSON_Delete(json);
        return NULL;
    }

    // 获取 "required_action" 对象
    cJSON *required_action = cJSON_GetObjectItem(data, "required_action");
    if (required_action == NULL) {
        printf("Error: 'required_action' not found in 'data'\n");
        cJSON_Delete(json);
        return NULL;
    }

    // 获取 "submit_tool_outputs" 对象
    cJSON *submit_tool_outputs = cJSON_GetObjectItem(required_action, "submit_tool_outputs");
    if (submit_tool_outputs == NULL) {
        printf("Error: 'submit_tool_outputs' not found in 'required_action'\n");
        cJSON_Delete(json);
        return NULL;
    }

    // 获取 "tool_calls" 数组
    cJSON *tool_calls = cJSON_GetObjectItem(submit_tool_outputs, "tool_calls");
    if (tool_calls == NULL || !cJSON_IsArray(tool_calls)) {
        printf("Error: 'tool_calls' not found or is not an array\n");
        cJSON_Delete(json);
        return NULL;
    }

    // 获取第一个 "tool_calls" 的对象
    cJSON *first_tool_call = cJSON_GetArrayItem(tool_calls, 0);
    if (first_tool_call == NULL) {
        printf("Error: 'tool_calls' array is empty\n");
        cJSON_Delete(json);
        return NULL;
    }

    // 获取 "function" 对象
    cJSON *function = cJSON_GetObjectItem(first_tool_call, "function");
    if (function == NULL) {
        printf("Error: 'function' not found in first tool call\n");
        cJSON_Delete(json);
        return NULL;
    }

    // 获取 "arguments" 字段
    cJSON *arguments = cJSON_GetObjectItem(function, "arguments");
    if (arguments == NULL) {
        printf("Error: 'arguments' not found in 'function'\n");
        cJSON_Delete(json);
        return NULL;
    }

    // 复制 "arguments" 的字符串值
    char *arguments_value = llm_strdup(arguments->valuestring);

    // 释放 JSON 对象
    cJSON_Delete(json);

    return arguments_value;  // 返回 arguments 的副本
}
//提取 submit_tool_outputs 中的第一个 tool_call 的 arguments 字段中的 key 值对
static char *neteast_msg_iot_extract_arg_val(const char *json_string, char *key)
{
    // 检查输入参数
    char *val = NULL;
    if (json_string == NULL) {
        return NULL;  // 参数不合法
    }

    // 解析 JSON 字符串
    cJSON *json = cJSON_Parse(json_string);
    if (json == NULL) {
        return NULL;  // JSON 解析失败
    }

    // 提取 key 后字段
    cJSON *key_val = cJSON_GetObjectItem(json, key);
    if (key_val == NULL || !cJSON_IsString(key_val)) {
        cJSON_Delete(json);
        return NULL;  // 未找到 "volume" 字段或类型不匹配
    }

    // 分配内存并复制键和值
    val = llm_strdup(key_val->valuestring);

    // 释放 JSON 对象
    cJSON_Delete(json);

    return val;  // 成功
}
//生成conversation.chat.submit_tool_outputs JSON
static char *neteast_msg_iot_gen_resp_json(const char *debug_id, const char *chat_id,
                                        const char *tool_call_id, const char *output)
{
    // 检查输入参数
    if (chat_id == NULL || tool_call_id == NULL || output == NULL) {
        return NULL;  // 参数不合法，返回 NULL
    }

    // 创建 JSON 对象
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        return NULL;  // JSON 对象创建失败
    }

    // 添加 "id" 和 "event_type" 字段
    cJSON_AddStringToObject(json, "id", debug_id);
    cJSON_AddStringToObject(json, "event_type", "conversation.chat.submit_tool_outputs");

    // 创建 "data" 对象并添加到 JSON
    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(json, "data", data);

    // 添加 "chat_id" 字段到 "data"
    cJSON_AddStringToObject(data, "chat_id", chat_id);

    // 创建 "tool_outputs" 数组并添加到 "data"
    cJSON *tool_outputs = cJSON_CreateArray();
    cJSON_AddItemToObject(data, "tool_outputs", tool_outputs);

    // 创建工具输出对象并添加到 "tool_outputs" 数组
    cJSON *tool_output = cJSON_CreateObject();
    cJSON_AddItemToArray(tool_outputs, tool_output);

    // 添加 "tool_call_id" 和 "output" 字段
    cJSON_AddStringToObject(tool_output, "tool_call_id", tool_call_id);
    cJSON_AddStringToObject(tool_output, "output", output);

    // 将 JSON 对象转换为字符串
    char *json_string = cJSON_PrintUnformatted(json);
    if (json_string == NULL) {
        cJSON_Delete(json);
        return NULL;  // JSON 字符串生成失败
    }

    // 释放 JSON 对象
    cJSON_Delete(json);
    return json_string;  // 返回生成的 JSON 字符串
}
//提交conversation.chat.submit_tool_outputs
static int32 neteast_msg_iot_send_tool_outputs(const char *debug_id, const char *chat_id,
        const char *tool_call_id, const char *output)
{
    char *str = NULL;
    int32 ret = 0;

    if (debug_id == NULL || chat_id == NULL || tool_call_id == NULL || output == NULL) {
        os_printf("Input param error\n");
        return RET_ERR;
    }

    str = neteast_msg_iot_gen_resp_json(debug_id, chat_id, tool_call_id, output);
    if (!str) {
        os_printf("Error,no memory!\n");
        return LLME_NOMEM;
    }
    ret = llm_sts_send(neteast_mgr.sts_session, LLM_DATA_TYPE_RAW, LLM_DATA_STATE_START,
                       str, (os_strlen(str) + 1));
    if (ret != RET_OK) {
        os_printf("send conversation.chat.submit_tool_outputs failed!\n");
        llm_free(str);
        return RET_ERR;
    }
    llm_free(str);
    return RET_OK;
}
/*
 * @brief 处理Coze AI IoT指令
 * @param: void
 * @return: int32
*/
static int32 neteast_msg_iot_process(char *data, uint32 data_len)
{
    int32 play   = 1;
    char debug_id_str[32];
    char *val  = NULL;
    char *mode = NULL;
    int32 val_int = 0;
    int32 mode_int = 0;

    hgprintf_out(data, data_len, 0);
    _os_printf("\r\n");

    char *data_id = neteast_msg_iot_extract_data_id(data);
    char *tool_id = neteast_msg_iot_extract_submit_tool_outputs_id(data);
    char *args    = neteast_msg_iot_extract_args(data);

    os_printf("Get data_id:%s,tool_id:%s,args:%s,mplayer_hdl:%d\n", data_id, tool_id,
              args, neteast_mgr.txmplayer_hdl);

    os_snprintf(debug_id_str, sizeof(debug_id_str), "%llu", os_jiffies());
    neteast_msg_iot_send_tool_outputs(debug_id_str, data_id, tool_id, args);

    neteast_mgr.time_stamp = os_jiffies();

    if (NULL != os_strstr(data, "txmplayer_ioctrl_volume")) {   //调节音量
        mode = neteast_msg_iot_extract_arg_val(args, "mode");
        val  = neteast_msg_iot_extract_arg_val(args, "volume");
        if (mode != NULL && val != NULL) {
            mode_int = os_atoi(mode);
            val_int  = os_atoi(val);
            if (mode_int == NETEAST_IOT_VOLUME_RELATIVE) {
                os_printf("Volume relative mode:%d + [%d]\n", neteast_mgr.volume, val_int);
                neteast_mgr.volume += val_int;
            } else if (mode_int == NETEAST_IOT_VOLUME_ABSOLUTE) {
                os_printf("Volume absolute mode:%d -> [%d]\n", neteast_mgr.volume, val_int);
                neteast_mgr.volume = val_int;
            } else {
                os_printf("unknow volute mode:[%s]\n", mode);
            }
            if (neteast_mgr.volume > 100) {
                neteast_mgr.volume = 100;
            }
            if (neteast_mgr.volume < 10) {
                neteast_mgr.volume = 10;
            }
            neteast_audio_set_volume(neteast_mgr.volume);
            if (neteast_mgr.pause == 0 && neteast_mgr.txmplayer_hdl >= 0) {
                txmplayer_pause(neteast_mgr.txmplayer_hdl, 0); //start
            }
        } else {
            os_printf("Input param error:%s\n", args);
        }
    } else if (NULL != os_strstr(data, "txmplayer_ioctrl_play")) { //暂停 or 继续
        val = neteast_msg_iot_extract_arg_val(args, "play");
        if (val) {
            play = os_atoi(val);
            os_printf("set player to %d,[%s]\n", play, play == 0 ? "STOP" : "PLAY");
            if (neteast_mgr.txmplayer_hdl >= 0) {
                if (play == 0) {
                    txmplayer_pause(neteast_mgr.txmplayer_hdl, 1); //stop
                    neteast_mgr.pause = 1;
                } else {
                    txmplayer_pause(neteast_mgr.txmplayer_hdl, 0); //start
                    neteast_mgr.pause = 0;
                }
            }
        }
    } else if (NULL != os_strstr(data, "power_ctrl")) {
        os_printf("Power off...\n");
        neteast_mgr.pwr_en = 0;
    } else if (NULL != os_strstr(data, "conversation_exit")) {
        os_printf("Step back...\n");
        neteast_mgr.pause = 1;
        neteast_mgr.time_stamp = 0;
    }
    llm_free(data_id);
    llm_free(tool_id);
    llm_free(args);

    return RET_OK;
}

//构建update包中的chat_config.parameters的json obj
static char *neteast_msg_chat_config_parameters_append_fileid(const char *input_json, const char *file_id)
{
    // 创建一个 JSON 对象
    char *json_str = NULL;
    cJSON *json_obj = NULL;

    if (input_json) {
        // 解析输入的 JSON 字符串
        json_obj = cJSON_Parse(input_json);
        if (!json_obj) {
            const char *errorPtr = cJSON_GetErrorPtr();
            if (errorPtr != NULL) {
                os_printf("Input JSON parse error before: %s\n", errorPtr);
            }
            // 解析失败，创建新对象
            json_obj = cJSON_CreateObject();
        }
    } else {
        // 没有输入 JSON，创建新对象
        json_obj = cJSON_CreateObject();
    }

    // 创建嵌套的 JSON 字符串
    char inner_image[256]; // 确保足够的大小
    os_snprintf(inner_image, sizeof(inner_image), "{\"file_id\":\"%s\"}", file_id);

    // 添加或替换 image 字段
    cJSON *image_item = cJSON_GetObjectItem(json_obj, "image");
    if (image_item) {
        // 如果已存在，替换值
        cJSON_ReplaceItemInObject(json_obj, "image", cJSON_CreateString(inner_image));
    } else {
        // 如果不存在，添加新字段
        cJSON_AddStringToObject(json_obj, "image", inner_image);
    }

    char *json_str_temp = cJSON_PrintUnformatted(json_obj);
    json_str = llm_strdup(json_str_temp);
    if (json_str_temp) { cJSON_free(json_str_temp); }

    cJSON_Delete(json_obj);

    return json_str;
}

char *neteast_msg_chat_config_parameters_append_ip(const char *input_json, const char *ip)
{
    // 创建一个 JSON 对象
    char *json_str = NULL;
    cJSON *json_obj = NULL;

    if (input_json) {
        // 解析输入的 JSON 字符串
        json_obj = cJSON_Parse(input_json);
        if (!json_obj) {
            const char *errorPtr = cJSON_GetErrorPtr();
            if (errorPtr != NULL) {
                os_printf("Input JSON parse error before: %s\n", errorPtr);
            }
            // 解析失败，创建新对象
            json_obj = cJSON_CreateObject();
        }
    } else {
        // 没有输入 JSON，创建新对象
        json_obj = cJSON_CreateObject();
    }

    // 添加或替换 user_ip 字段
    if (ip) {
        cJSON *user_ip_item = cJSON_GetObjectItem(json_obj, "user_ip");
        if (user_ip_item) {
            // 如果已存在，替换值
            cJSON_ReplaceItemInObject(json_obj, "user_ip", cJSON_CreateString(ip));
        } else {
            // 如果不存在，添加新字段
            cJSON_AddStringToObject(json_obj, "user_ip", ip);
        }
    }

    char *json_str_temp = cJSON_PrintUnformatted(json_obj);
    json_str = llm_strdup(json_str_temp);
    if (json_str_temp) { cJSON_free(json_str_temp); }

    cJSON_Delete(json_obj);

    return json_str;
}
/*
 * @brief 处理Coze AI文件上传
 * @param: void
 * @return: int32
*/
static int32 neteast_msg_file_upload_process(char *data, uint32 data_len)
{
    cJSON *json = cJSON_Parse(data);
    int32 ret = RET_OK;

    if (json == NULL) {
        const char *errorPtr = cJSON_GetErrorPtr();
        if (errorPtr != NULL) {
            os_printf("%s:Json error before: %s\n", errorPtr);
        }
        return RET_ERR;
    }

    cJSON *pdata = cJSON_GetObjectItem(json, "data");
    if (pdata == NULL) {
        os_printf("No 'data' found in JSON.\n");
        cJSON_Delete(json);
        return RET_ERR;
    }

    cJSON *id = cJSON_GetObjectItem(pdata, "id");
    if (id != NULL && cJSON_IsString(id)) {
        os_printf("get file id: [%s]\n", id->valuestring);//获取file_ID字符串
        char *param = neteast_msg_chat_config_parameters_append_fileid(neteast_mgr.chat_config_parameter, id->valuestring);
        os_mutex_lock(&neteast_mgr.lock, osWaitForever);
        os_memset(neteast_mgr.chat_config_parameter, 0, sizeof(neteast_mgr.chat_config_parameter));
        os_strncpy(neteast_mgr.chat_config_parameter, param,
                   os_strlen(param) > sizeof(neteast_mgr.chat_config_parameter) ? sizeof(neteast_mgr.chat_config_parameter) : os_strlen(param));
        os_mutex_unlock(&neteast_mgr.lock);
        llm_free(param);
        ret = llm_sts_config(neteast_mgr.sts_session, LLM_CONFIG_TYPE_MODEL,
                             (void *)&neteast_sts_platform_cfg, sizeof(neteast_sts_platform_cfg));//填入config并update到llm库中
        if (ret) {
            os_printf("sts_model_cfg fail!\r\n");
            cJSON_Delete(json);
            return RET_ERR;
        }
        neteast_mgr.img_triggered = 1;//如果想要传完图像后，语音问问题, 就把这行注释掉即可。
    } else {
        os_printf("'id' field not found or is not a string\n");
    }

    cJSON_Delete(json);
    return RET_OK;
}

//文生图官方文档做法，返回的是URL
static int32 neteast_msg_tti_url_process(char *data, uint32 data_len)
{
    //目前只是将分析的image_url打印出来，还需要用户通过https去该url拉数据。
    cJSON *json = cJSON_Parse(data);
    if (json == NULL) {
        os_printf("parse json error!\n");
        return RET_ERR;
    }

    // 查找 "data" 对象
    cJSON *json_data = cJSON_GetObjectItem(json, "data");
    if (json_data == NULL || !cJSON_IsObject(json_data)) {
        os_printf("can not find data obj!\n");
        cJSON_Delete(json);
        return RET_ERR;
    }

    // 查找 "content" 字段
    cJSON *content_item = cJSON_GetObjectItem(json_data, "content");
    if (content_item == NULL || !cJSON_IsString(content_item) || (content_item->valuestring == NULL)) {
        os_printf("can not find content obj!\n");
        cJSON_Delete(json);
        return RET_ERR;
    }

    // 解析 content 的 JSON 字符串
    cJSON *content_json = cJSON_Parse(content_item->valuestring);
    if (content_json == NULL) {
        os_printf("can not find content val str!\n");
        cJSON_Delete(json);
        return RET_ERR;
    }

    // 查找 "data" 对象中的 "image_urls" 数组
    cJSON *image_urls = cJSON_GetObjectItem(content_json, "data");
    if (image_urls == NULL || !cJSON_IsObject(image_urls)) {
        os_printf("can not find content data!\n");
        cJSON_Delete(content_json);
        cJSON_Delete(json);
        return RET_ERR;
    }

    // 从 "data" 对象中获取 "image_urls" 数组
    cJSON *urls_array = cJSON_GetObjectItem(image_urls, "image_urls");
    if (urls_array == NULL || !cJSON_IsArray(urls_array)) {
        os_printf("can not find image url array!\n");
        cJSON_Delete(content_json);
        cJSON_Delete(json);
        return RET_ERR;
    }

    // 遍历数组，提取 HTTPS 链接
    int array_size = cJSON_GetArraySize(urls_array);
    for (int i = 0; i < array_size; i++) {
        cJSON *url_item = cJSON_GetArrayItem(urls_array, i);
        if (cJSON_IsString(url_item) && (url_item->valuestring != NULL)) {
            os_printf("get neteast gen image url:%s\n", url_item->valuestring);
        }
    }
    // 释放 JSON 对象
    cJSON_Delete(content_json);
    cJSON_Delete(json);
    return RET_OK;
}
//文生图：生成的图片数据通过base64的格式发送过来，此为从json分离base64数据
static char *neteast_msg_tti_extract_base64_from_json(const char *json_str, uint32 json_tot_len, uint32 *b64_len)
{
    cJSON *root = NULL;
    cJSON *data_obj = NULL;
    cJSON *content = NULL;
    cJSON *content_json = NULL;
    cJSON *data_field = NULL;
    char *base64_content = NULL;
    const char *base64_marker = "base64,";
    char *marker_pos = NULL;
    char *content_end = NULL;
    uint32 str_len = 0;
    int ret = RET_ERR;

    if (!json_str || !b64_len || json_tot_len == 0) {
        return NULL;
    }

    *b64_len = 0;

    // 检查JSON字符串长度是否超过限制
    str_len = os_strlen(json_str);
    if (str_len > json_tot_len) {
        os_printf("JSON string length %u exceeds total length %u\n", str_len, json_tot_len);
        str_len = json_tot_len;
    }

    // 解析外层JSON
    root = cJSON_Parse(json_str);
    if (!root) {
        os_printf("JSON parse error: %s\n", cJSON_GetErrorPtr());
        goto exit;
    }

    // 获取data对象
    data_obj = cJSON_GetObjectItem(root, "data");
    if (!data_obj || !cJSON_IsObject(data_obj)) {
        os_printf("JSON structure error: data object not found or not an object\n");
        goto exit;
    }

    // 获取content字段
    content = cJSON_GetObjectItem(data_obj, "content");
    if (!content || !cJSON_IsString(content) || !content->valuestring) {
        os_printf("JSON structure error: content not found, not a string, or empty\n");
        goto exit;
    }

    // 尝试解析content字段为JSON
    content_json = cJSON_Parse(content->valuestring);
    if (content_json) {
        // content 是 JSON 格式，从 data 字段获取
        data_field = cJSON_GetObjectItem(content_json, "data");
        if (!data_field || !cJSON_IsString(data_field) || !data_field->valuestring) {
            os_printf("JSON structure error: data field not found in content JSON, not a string, or empty\n");
            goto exit;
        }
        // 查找base64标记
        marker_pos = os_strstr((char *)data_field->valuestring, base64_marker);
    } else {
        // content 不是 JSON 格式，直接从 content 字符串中提取
        os_printf("content is not JSON format, extracting directly from content string\n");
        marker_pos = os_strstr((char *)content->valuestring, base64_marker);
    }

    if (!marker_pos) {
        os_printf("base64 marker not found\n");
        goto exit;
    }

    // 移动到base64内容开始位置
    marker_pos += os_strlen(base64_marker);

    // 查找base64内容的结束位置
    content_end = marker_pos;
    while (*content_end && *content_end != '"') {
        content_end++;
    }

    // 计算base64内容长度
    *b64_len = content_end - marker_pos;
    if (*b64_len == 0) {
        os_printf("base64 content is empty\n");
        goto exit;
    }

    // 分配内存并清空
    base64_content = llm_malloc(*b64_len + 1);
    if (!base64_content) {
        os_printf("Memory allocation failed\n");
        goto exit;
    }

    // 复制base64内容
    os_memcpy(base64_content, marker_pos, *b64_len);
    base64_content[*b64_len] = '\0';

    ret = RET_OK;

exit:
    // 统一释放JSON对象
    if (content_json) {
        cJSON_Delete(content_json);
    }
    if (root) {
        cJSON_Delete(root);
    }

    return (ret == RET_OK) ? base64_content : NULL;
}
//文生图：生成的图片数据通过base64的格式发送过来
static int32 neteast_msg_tti_base64_process(char *data, uint32 data_len)
{
    struct framebuff *fb = NULL;
    unsigned char *decoded_image_buf = NULL;
    uint32 decoded_image_len = 0;
    char *base64_content = NULL;
    uint32 base64_content_len = 0;
    int ret = 0;

    base64_content = neteast_msg_tti_extract_base64_from_json(data, data_len, &base64_content_len);//reduce
    if (!base64_content) {
        os_printf("Extract base64 content failed!\n");
        return RET_ERR;
    }

    os_printf("Get TTI base64 data %p,len=%d\r\n", base64_content, base64_content_len);

    ret = llm_base64_decode(base64_content, &decoded_image_buf, &decoded_image_len);
    llm_free(base64_content);

    if (ret != RET_OK || decoded_image_buf == NULL || decoded_image_len == 0) {
        os_printf("Base64 decode failed (ret=%d, len=%d)\r\n", ret, decoded_image_len);
        return RET_ERR;
    }

    fb = msi_alloc_fb(neteast_mgr.neteast_msi, NULL, NULL, decoded_image_len, 0, 0);
    if (fb) {
        fb->mtype = F_JPG;
        fb->stype = FSTYPE_JPG_FILE;
        fb->datatag = ++neteast_mgr.jpg_data_tag;
        hw_memcpy(fb->data, decoded_image_buf, decoded_image_len);
        sys_dcache_clean_invalid_range_unaligned((uint32_t *)fb->data, decoded_image_len);
        msi_output_fb(neteast_mgr.neteast_msi, fb, 0);
    } else {
        os_printf("fb alloc fail!\r\n");
    }
    llm_free(decoded_image_buf);
    return RET_OK;
}
/*
 * @brief: 处理Coze AI图片数据
 * @param: char *data 图片数据
 * @param: uint32 data_len 图片数据长度
 * @return: int32
*/
static int32 neteast_msg_tti_process(char *data, uint32 data_len)
{
    const char *base64_key = ";base64,";
    if (NULL != os_strstr(data, base64_key)) {
        return neteast_msg_tti_base64_process(data, data_len);//智能体通过base64压缩过的图片数据
    } else {
        return neteast_msg_tti_url_process(data, data_len);//官方文档做法,需要解析URL并通过https获取图片数据
    }
}

/*
 * @brief: 处理Coze AI对话ID
 * @param: char *data 对话ID
 * @param: uint32 data_len 对话ID长度
 * @return: int32
*/
static int32 neteast_msg_conversation_id_process(char *data, uint32 data_len)
{
#if 0	
    int32 ret = 0;
    if (os_strlen(data) > sizeof(sys_cfgs.neteast_conversation_id)) {
        os_printf("Error:conversation_id too long!\n");
        return RET_ERR;
    }
    if (os_strcmp(sys_cfgs.neteast_conversation_id, data) == 0) {
        return RET_OK;
    }
    //保存到conversation_id到flash
    os_printf("Modify conversation_id to %s\n", data);
    memset(sys_cfgs.neteast_conversation_id, 0, sizeof(sys_cfgs.neteast_conversation_id));
    os_strncpy(sys_cfgs.neteast_conversation_id, data, os_strlen(data) + 1);
    syscfg_save();

    neteast_sts_platform_cfg.chat_config_conversation_id = (char *)&sys_cfgs.neteast_conversation_id;
    ret = llm_sts_config(neteast_mgr.sts_session, LLM_CONFIG_TYPE_MODEL,
                         (void *)&neteast_sts_platform_cfg, sizeof(neteast_sts_platform_cfg));//填入config并update到llm库中
    if (ret) {
        os_printf("update conversation_id fail!\r\n");
        return RET_ERR;
    }
#endif
    return RET_OK;
}

/*
 * @brief: 处理Coze AI事件消息
 * @param: void
 * @return: int32
*/
int32 neteast_msg_event_process(void)
{
    int32 ret = RET_OK;
    struct neteast_demo_event_msg event_msg = {0};

    RB_INT_GET(&neteast_mgr.event_queue, event_msg);
    if (event_msg.event == LLM_EVENT_UNKNOWN) { return RET_OK; }

    switch (event_msg.event) {
        case LLM_EVENT_CONNECTED: {
            os_printf("Coze AI connected!\r\n");
            neteast_main_set_state(NETEAST_DEMO_STATE_CONNECTED);
            break;
        }
        case LLM_EVENT_DISCONNECT: {
            os_printf("Coze AI disconnect!\r\n");
            neteast_main_set_state(NETEAST_DEMO_STATE_DISCONNECTED);
            break;
        }
        case LLM_EVENT_STT_RESULT: {
            os_printf("NETEAST AI STT(%d):", event_msg.msg_len);
            hgprintf_out(event_msg.msg, event_msg.msg_len, 0);
            _os_printf("\r\n");
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_TTS_RESULT: {
            os_printf("NETEAST AI TTS(%d):", event_msg.msg_len);
            hgprintf_out(event_msg.msg, event_msg.msg_len, 0);
            _os_printf("\r\n");
            llm_free(event_msg.msg);
            neteast_mgr.time_stamp = os_jiffies();
            neteast_mgr.dialogue_timeout_cnt = 0;
            break;
        }
        case LLM_EVENT_VAD_RESULT: {
            os_printf("Coze AI recv server vad result:%s\r\n", event_msg.msg);
            if (os_strncmp(event_msg.msg, "START", event_msg.msg_len) == 0) {
                neteast_mgr.time_stamp = UINT64_MAX;
            } else if (os_strncmp(event_msg.msg, "STOP", event_msg.msg_len) == 0) {
                neteast_main_set_state(NETEAST_DEMO_STATE_READY);
            }
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_DIALOGUE_END: {
            os_printf("NETEAST AI check finish!\r\n");
            neteast_mgr.finish = 1;
            if (neteast_mgr.time_stamp != 0) {
                neteast_mgr.time_stamp = os_jiffies();
            }
            break;
        }
        case LLM_EVENT_DIALOGUE_TIMEOUT: {
            os_printf("NETEAST AI dialogue timeout!\r\n");
            neteast_mgr.dialogue_timeout_cnt++;    //1s
            if (neteast_mgr.dialogue_timeout_cnt > NETEAST_DEMO_DIALOGUE_PULLING_TIMEOUT) {
                neteast_mgr.dialogue_timeout_cnt = 0;
                ret = neteast_main_interrupt();
                if (ret != RET_OK) {
                    break;
                }
                neteast_mgr.finish = 1;
                neteast_mgr.time_stamp = os_jiffies();
                os_printf("End the current conversation and start a new one!\r\n");
            }
            break;
        }
        case LLM_EVENT_ERROR_MSG: {
            os_printf("NETEAST AI recv err msg(%d):", event_msg.msg_len);
            hgprintf_out(event_msg.msg, event_msg.msg_len, 0);
            _os_printf("\r\n");
            neteast_msg_error_process(event_msg.msg);
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_CUSTOMIZE: {
            os_printf("NETEAST AI recv customize(%d):", event_msg.msg_len);
            hgprintf_out(event_msg.msg, event_msg.msg_len, 0);
            _os_printf("\r\n");
            //neteast_msg_customize_process(event_msg.msg, event_msg.msg_len);
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_UPLOAD_FILE_RESULT: {
            os_printf("NETEAST AI recv upload file result(%d):", event_msg.msg_len);
            hgprintf_out(event_msg.msg, event_msg.msg_len, 0);
            _os_printf("\r\n");
            neteast_msg_file_upload_process(event_msg.msg, event_msg.msg_len);
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_TTI_RESULT: {
            os_printf("NETEAST AI recv tti result(%d):", event_msg.msg_len);
            //hgprintf_out(event_msg.msg, event_msg.msg_len, 0);
            //_os_printf("\r\n");
            neteast_msg_tti_process(event_msg.msg, event_msg.msg_len);
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_CONVERSATION_ID: {
            os_printf("Get conversation id:[%s]\n", event_msg.msg);
            neteast_msg_conversation_id_process(event_msg.msg, event_msg.msg_len);
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_MPLAYER_RESULT: {
            os_printf("NETEAST AI recv media player result(%d):", event_msg.msg_len);
            neteast_msg_mplayer_process(event_msg.msg, event_msg.msg_len);
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_IOT_RESULT: {
            os_printf("NETEAST AI recv iot result(%d):", event_msg.msg_len);
            neteast_msg_iot_process(event_msg.msg, event_msg.msg_len);
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_TX_ERR:
        case LLM_EVENT_RX_ERR: {
            os_printf("NETEAST AI recv %s err!\r\n", (event_msg.event == LLM_EVENT_TX_ERR ? "TX" : "RX"));
            neteast_main_set_state(NETEAST_DEMO_STATE_IDLE);
            llm_sts_reconnect(neteast_mgr.sts_session);
            break;
        }
        default: {
            os_printf("Not supported event msg: %d\r\n", event_msg.event);
            break;
        }
    }
    return ret;
}
#endif