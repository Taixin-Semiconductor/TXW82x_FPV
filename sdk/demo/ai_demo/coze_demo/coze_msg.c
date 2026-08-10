#include "coze_demo.h"

#if COZE_DEMO
/* -------------------------- 消息代码 -------------------------- */
typedef enum {
    COZE_IOT_VOLUME_RELATIVE = 1,//相对值模式
    COZE_IOT_VOLUME_ABSOLUTE = 2,//绝对值模式
} coze_demo_iot_volume_mode;

/*
 * @brief: 处理Coze AI错误消息
 * @param: char *error 错误消息
 * @return: int32
*/
static int32 coze_msg_error_process(char *error)
{
    int32 err_code = 0;
    char *err_msg = NULL;

    coze_err("**设备异常**\n");
    if (error) {
        cJSON *root = cJSON_Parse(error);
        if (root == NULL) {
            coze_err("Error before: %s\n", error);
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
            coze_err("Error after: %s\n", error);
            cJSON_Delete(root);
            return RET_ERR;
        }
        coze_err("\"%d\": \"%s\"\r\n", err_code, err_msg);
        switch (err_code) {
            case 4302: {
                coze_err("**没听清楚，您再说一遍!**\n");
                coze_mgr.finish = 1;
                coze_main_set_state(COZE_DEMO_STATE_WAITING);
                break;
            }
            default: {
                llm_sts_stop(coze_mgr.sts_session);
                // 注意：所有调用 llm_sts_reconnect 前必须将状态设置为 COZE_DEMO_STATE_IDLE
                //coze_main_set_state(COZE_DEMO_STATE_IDLE);
                //llm_sts_reconnect(coze_mgr.sts_session);
                break;
            }
        }
        cJSON_Delete(root);
    }
    return RET_OK;
}

// 提取 submit_tool_outputs 中的第一个 tool_call 的 arguments 字段中的 url 字段
static char *coze_msg_get_audio_url(char *data, uint32 data_len)
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
            coze_err("JSON parse error before: %s\n", errorPtr);
        }
        return NULL;
    }

    data_obj = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (!data_obj || !cJSON_IsObject(data_obj)) {
        coze_err("No data object found\n");
        goto cleanup;
    }

    content_str = cJSON_GetObjectItemCaseSensitive(data_obj, "content");
    if (!content_str || !cJSON_IsString(content_str)) {
        coze_err("No content string found\n");
        goto cleanup;
    }

    content_obj = cJSON_Parse(content_str->valuestring);
    if (!content_obj) {
        const char *errorPtr = cJSON_GetErrorPtr();
        if (errorPtr != NULL) {
            coze_err("Content JSON parse error before: %s\n", errorPtr);
        }
        goto cleanup;
    }

    url_item = cJSON_GetObjectItemCaseSensitive(content_obj, "url");
    if (url_item && cJSON_IsString(url_item)) {
        audio_url = llm_strdup(url_item->valuestring);
        coze_err("get audio url:%s\n", audio_url);
    } else {
        coze_err("No url found in content\n");
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
static int32 coze_msg_mplayer_process(char *data, uint32 data_len)
{
    struct txmplayer_param param = {
        .volume = coze_mgr.volume,
    };

    char *audio_url = coze_msg_get_audio_url(data, data_len);
    if (audio_url) {
        if (coze_mgr.audio_url_hdl >= 0) {
            txmplayer_close(coze_mgr.audio_url_hdl);
            coze_mgr.audio_url_hdl = -1;
        }
        coze_mgr.audio_url_hdl = txmplayer_open(audio_url, 0, &param);
        //先暂停，等播放完TTS后再播放
        txmplayer_pause(coze_mgr.audio_url_hdl, 1);
        coze_mgr.pause = 2;
        coze_mgr.time_stamp = os_jiffies();
        llm_free(audio_url);
    }
    return RET_OK;
}

//生成conversation.chat.submit_tool_outputs JSON
static char *coze_msg_iot_gen_resp_json(const char *debug_id, const char *chat_id,
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
static int32 coze_msg_iot_send_tool_outputs(const char *debug_id, const char *chat_id,
        const char *tool_call_id, const char *output)
{
    char *str = NULL;
    int32 ret = 0;

    if (debug_id == NULL || chat_id == NULL || tool_call_id == NULL || output == NULL) {
        coze_err("Input param error\n");
        return RET_ERR;
    }

    str = coze_msg_iot_gen_resp_json(debug_id, chat_id, tool_call_id, output);
    if (!str) {
        coze_err("Error,no memory!\n");
        return LLME_NOMEM;
    }
    ret = llm_sts_send(coze_mgr.sts_session, LLM_DATA_TYPE_RAW, LLM_DATA_STATE_START,
                       str, (os_strlen(str) + 1));
    if (ret != RET_OK) {
        coze_err("send conversation.chat.submit_tool_outputs failed!\n");
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
static int32 coze_msg_iot_process(char *data, uint32 data_len)
{
    int32 ret = RET_OK;
    int32 play = 1;
    char debug_id_str[32];
    uint8 volume = coze_mgr.volume;
    cJSON *root = NULL;
    cJSON *data_obj = NULL;
    cJSON *data_id = NULL;
    cJSON *required_action = NULL;
    cJSON *submit_tool_outputs = NULL;
    cJSON *tool_calls = NULL;
    cJSON *first_tool_call = NULL;
    cJSON *tool_id = NULL;
    cJSON *function_obj = NULL;
    cJSON *func_name = NULL;
    cJSON *args = NULL;
    cJSON *args_obj = NULL;
    cJSON *mode = NULL;
    cJSON *val = NULL;
    cJSON *play_item = NULL;
    cJSON *weather_info = NULL;
    cJSON *weather_json = NULL;
    cJSON *weather_item = NULL;
    cJSON *temp_low = NULL;
    cJSON *temp_high = NULL;
    cJSON *weather_day = NULL;
    char *weather_json_str = NULL;
    const char *func_name_str = NULL;
    const char *args_str = NULL;

    if (data == NULL || data_len == 0) {
        coze_err("Input iot msg invalid\n");
        return RET_ERR;
    }

    hgprintf_out(data, data_len, 0);
    _os_printf("\r\n");

    root = cJSON_Parse(data);
    if (root == NULL) {
        coze_err("Parse iot msg json failed\n");
        return RET_ERR;
    }

    data_obj = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (data_obj == NULL || !cJSON_IsObject(data_obj)) {
        coze_err("Input iot msg error: data invalid\n");
        ret = RET_ERR;
        goto exit;
    }

    data_id = cJSON_GetObjectItemCaseSensitive(data_obj, "id");
    required_action = cJSON_GetObjectItemCaseSensitive(data_obj, "required_action");
    if (data_id == NULL || !cJSON_IsString(data_id) ||
        required_action == NULL || !cJSON_IsObject(required_action)) {
        coze_err("Input iot msg error: id or required_action invalid\n");
        ret = RET_ERR;
        goto exit;
    }

    submit_tool_outputs = cJSON_GetObjectItemCaseSensitive(required_action, "submit_tool_outputs");
    if (submit_tool_outputs == NULL || !cJSON_IsObject(submit_tool_outputs)) {
        coze_err("Input iot msg error: submit_tool_outputs invalid\n");
        ret = RET_ERR;
        goto exit;
    }

    tool_calls = cJSON_GetObjectItemCaseSensitive(submit_tool_outputs, "tool_calls");
    if (tool_calls == NULL || !cJSON_IsArray(tool_calls)) {
        coze_err("Input iot msg error: tool_calls invalid\n");
        ret = RET_ERR;
        goto exit;
    }

    first_tool_call = cJSON_GetArrayItem(tool_calls, 0);
    if (first_tool_call == NULL || !cJSON_IsObject(first_tool_call)) {
        coze_err("Input iot msg error: first tool_call invalid\n");
        ret = RET_ERR;
        goto exit;
    }

    tool_id = cJSON_GetObjectItemCaseSensitive(first_tool_call, "id");
    function_obj = cJSON_GetObjectItemCaseSensitive(first_tool_call, "function");
    if (tool_id == NULL || !cJSON_IsString(tool_id) ||
        function_obj == NULL || !cJSON_IsObject(function_obj)) {
        coze_err("Input iot msg error: tool id or function invalid\n");
        ret = RET_ERR;
        goto exit;
    }

    func_name = cJSON_GetObjectItemCaseSensitive(function_obj, "name");
    args = cJSON_GetObjectItemCaseSensitive(function_obj, "arguments");
    if (func_name == NULL || !cJSON_IsString(func_name) ||
        args == NULL || !cJSON_IsString(args)) {
        coze_err("Input iot msg error: function name or arguments invalid\n");
        ret = RET_ERR;
        goto exit;
    }

    func_name_str = func_name->valuestring;
    args_str = args->valuestring;
    args_obj = cJSON_Parse(args_str);
    if (args_obj == NULL || !cJSON_IsObject(args_obj)) {
        coze_err("Input iot msg error: arguments json invalid\n");
        ret = RET_ERR;
        goto exit;
    }

//    coze_err("Get data_id:%s,tool_id:%s,func:%s,args:%s,mplayer_hdl:%d\n",
//             data_id->valuestring, tool_id->valuestring, func_name_str, args_str,
//             coze_mgr.audio_url_hdl);

    os_snprintf(debug_id_str, sizeof(debug_id_str), "%llu", os_jiffies());
    coze_msg_iot_send_tool_outputs(debug_id_str, data_id->valuestring, tool_id->valuestring, args_str);

    coze_mgr.time_stamp = os_jiffies();
    //先暂停，等播放完TTS后再播放
    txmplayer_pause(coze_mgr.audio_url_hdl, 1);

    if (os_strcmp(func_name_str, "txmplayer_ioctrl_volume") == 0) {   //调节音量
        mode = cJSON_GetObjectItemCaseSensitive(args_obj, "mode");
        val = cJSON_GetObjectItemCaseSensitive(args_obj, "volume");
        if (mode != NULL && cJSON_IsString(mode) && val != NULL && cJSON_IsString(val)) {
            int32 mode_int = os_atoi(mode->valuestring);
            int32 val_int = os_atoi(val->valuestring);
            if (mode_int == COZE_IOT_VOLUME_RELATIVE) {
                coze_err("Volume relative mode:%d + [%d]\n", coze_mgr.volume, val_int);
                volume += val_int;
            } else if (mode_int == COZE_IOT_VOLUME_ABSOLUTE) {
                coze_err("Volume absolute mode:%d -> [%d]\n", coze_mgr.volume, val_int);
                volume = val_int;
            } else {
                coze_err("unknow volute mode:[%s]\n", mode->valuestring);
            }
            coze_mgr.volume = volume > 100 ? 100 : volume < 10 ? 10 : volume;
            coze_mgr.pause = 2;
            coze_audio_set_volume(coze_mgr.volume);
        } else {
            coze_err("Input volume arguments error:%s\n", args_str);
        }
    } else if (os_strcmp(func_name_str, "txmplayer_ioctrl_play") == 0) { //暂停 or 继续
        play_item = cJSON_GetObjectItemCaseSensitive(args_obj, "play");
        if (play_item != NULL && cJSON_IsString(play_item)) {
            play = os_atoi(play_item->valuestring);
            coze_err("set player to %d,[%s]\n", play, play == 0 ? "STOP" : "PLAY");
            if (play) {
                coze_mgr.pause = 0;
            } else {
                coze_mgr.pause = 1;
            }
        } else {
            coze_err("Input play arguments error:%s\n", args_str);
        }
    } else if (os_strcmp(func_name_str, "power_ctrl") == 0) {
        coze_err("Power off...\n");
        coze_mgr.pwr_en = 0;
    } else if (os_strcmp(func_name_str, "conversation_exit") == 0) {
        coze_err("Step back...\n");
        coze_mgr.time_stamp = 0;
    } else if (os_strcmp(func_name_str, "manual_get_weather") == 0) {
        weather_info = cJSON_GetObjectItemCaseSensitive(args_obj, "weather_info");
        if (weather_info != NULL && cJSON_IsString(weather_info)) {
            weather_json_str = os_strstr(weather_info->valuestring, "weather_info:");
            if (weather_json_str != NULL) {
                weather_json_str += os_strlen("weather_info:");
                weather_json = cJSON_Parse(weather_json_str);
            }

            if (weather_json != NULL && cJSON_IsArray(weather_json)) {
                weather_item = cJSON_GetArrayItem(weather_json, 0);
                if (weather_item != NULL && cJSON_IsObject(weather_item)) {
                    temp_low = cJSON_GetObjectItemCaseSensitive(weather_item, "temp_low");
                    temp_high = cJSON_GetObjectItemCaseSensitive(weather_item, "temp_high");
                    weather_day = cJSON_GetObjectItemCaseSensitive(weather_item, "weather_day");
                }
            }

            if (temp_low != NULL && cJSON_IsNumber(temp_low) &&
                temp_high != NULL && cJSON_IsNumber(temp_high) &&
                weather_day != NULL && cJSON_IsString(weather_day)) {
                uint32 weather_str_len = os_snprintf(NULL, 0, "%d~%d°C %s", temp_low->valueint, temp_high->valueint, weather_day->valuestring);
                char *weather_str = llm_realloc(coze_mgr.weather_str, weather_str_len+1);
                if (weather_str != NULL) {
                    os_snprintf(weather_str, weather_str_len+1, "%d~%d°C %s", temp_low->valueint, temp_high->valueint, weather_day->valuestring);  
                    coze_mgr.weather_str = weather_str;
                    coze_err("weather_str: %s\r\n", coze_mgr.weather_str);
                    SYSEVT_NEW_ENV_EVT(SYSEVT_ENV_WEATHER, coze_mgr.weather_str);                    
                    coze_mgr.time_stamp = UINT64_MAX;
                    coze_mgr.update_weather_time = os_jiffies();
                    coze_mgr.get_weather_done = 1;
                }
            } else {
                coze_err("Input weather arguments error:%s\n", args_str);
            }
        } else {
            coze_err("Input weather arguments error:%s\n", args_str);
        }
    } else {
        coze_err("Not supported iot function:%s\n", func_name_str);
    }

exit:
    if (weather_json != NULL) {
        cJSON_Delete(weather_json);
    }
    if (args_obj != NULL) {
        cJSON_Delete(args_obj);
    }
    cJSON_Delete(root);

    return ret;
}

//构建update包中的chat_config.parameters的json obj
static char *coze_msg_chat_config_parameters_append_fileid(const char *input_json, const char *file_id)
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
                coze_err("Input JSON parse error before: %s\n", errorPtr);
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

char *coze_msg_chat_config_parameters_append_ip(const char *input_json, const char *ip)
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
                coze_err("Input JSON parse error before: %s\n", errorPtr);
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
static int32 coze_msg_file_upload_process(char *data, uint32 data_len)
{
    cJSON *json = cJSON_Parse(data);
    int32 ret = RET_OK;

    if (json == NULL) {
        const char *errorPtr = cJSON_GetErrorPtr();
        if (errorPtr != NULL) {
            coze_err("%s:Json error before: %s\n", errorPtr);
        }
        return RET_ERR;
    }

    cJSON *pdata = cJSON_GetObjectItem(json, "data");
    if (pdata == NULL) {
        coze_err("No 'data' found in JSON.\n");
        cJSON_Delete(json);
        return RET_ERR;
    }

    cJSON *id = cJSON_GetObjectItem(pdata, "id");
    if (id != NULL && cJSON_IsString(id)) {
        coze_err("get file id: [%s]\n", id->valuestring);//获取file_ID字符串
        char *param = coze_msg_chat_config_parameters_append_fileid(coze_mgr.chat_config_parameter, id->valuestring);
        os_mutex_lock(&coze_mgr.lock, osWaitForever);
        os_memset(coze_mgr.chat_config_parameter, 0, sizeof(coze_mgr.chat_config_parameter));
        os_strncpy(coze_mgr.chat_config_parameter, param,
                   os_strlen(param) > sizeof(coze_mgr.chat_config_parameter) ? sizeof(coze_mgr.chat_config_parameter) : os_strlen(param));
        os_mutex_unlock(&coze_mgr.lock);
        llm_free(param);
        ret = llm_sts_config(coze_mgr.sts_session, LLM_CONFIG_TYPE_MODEL,
                             (void *)&coze_sts_platform_cfg, sizeof(coze_sts_platform_cfg));//填入config并update到llm库中
        if (ret) {
            coze_err("sts_model_cfg fail!\r\n");
            cJSON_Delete(json);
            return RET_ERR;
        }
        coze_mgr.img_triggered = 1;//如果想要传完图像后，语音问问题, 就把这行注释掉即可。
    } else {
        coze_err("'id' field not found or is not a string\n");
    }

    cJSON_Delete(json);
    return RET_OK;
}

//文生图官方文档做法，返回的是URL
static int32 coze_msg_tti_url_process(char *data, uint32 data_len)
{
    //目前只是将分析的image_url打印出来，还需要用户通过https去该url拉数据。
    cJSON *json = cJSON_Parse(data);
    if (json == NULL) {
        coze_err("parse json error!\n");
        return RET_ERR;
    }

    // 查找 "data" 对象
    cJSON *json_data = cJSON_GetObjectItem(json, "data");
    if (json_data == NULL || !cJSON_IsObject(json_data)) {
        coze_err("can not find data obj!\n");
        cJSON_Delete(json);
        return RET_ERR;
    }

    // 查找 "content" 字段
    cJSON *content_item = cJSON_GetObjectItem(json_data, "content");
    if (content_item == NULL || !cJSON_IsString(content_item) || (content_item->valuestring == NULL)) {
        coze_err("can not find content obj!\n");
        cJSON_Delete(json);
        return RET_ERR;
    }

    // 解析 content 的 JSON 字符串
    cJSON *content_json = cJSON_Parse(content_item->valuestring);
    if (content_json == NULL) {
        coze_err("can not find content val str!\n");
        cJSON_Delete(json);
        return RET_ERR;
    }

    // 查找 "data" 对象中的 "image_urls" 数组
    cJSON *image_urls = cJSON_GetObjectItem(content_json, "data");
    if (image_urls == NULL || !cJSON_IsObject(image_urls)) {
        coze_err("can not find content data!\n");
        cJSON_Delete(content_json);
        cJSON_Delete(json);
        return RET_ERR;
    }

    // 从 "data" 对象中获取 "image_urls" 数组
    cJSON *urls_array = cJSON_GetObjectItem(image_urls, "image_urls");
    if (urls_array == NULL || !cJSON_IsArray(urls_array)) {
        coze_err("can not find image url array!\n");
        cJSON_Delete(content_json);
        cJSON_Delete(json);
        return RET_ERR;
    }

    // 遍历数组，提取 HTTPS 链接
    int array_size = cJSON_GetArraySize(urls_array);
    for (int i = 0; i < array_size; i++) {
        cJSON *url_item = cJSON_GetArrayItem(urls_array, i);
        if (cJSON_IsString(url_item) && (url_item->valuestring != NULL)) {
            coze_err("get coze gen image url:%s\n", url_item->valuestring);
        }
    }
    // 释放 JSON 对象
    cJSON_Delete(content_json);
    cJSON_Delete(json);
    return RET_OK;
}
//文生图：生成的图片数据通过base64的格式发送过来，此为从json分离base64数据
static char *coze_msg_tti_extract_base64_from_json(const char *json_str, uint32 json_tot_len, uint32 *b64_len)
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
        coze_err("JSON string length %u exceeds total length %u\n", str_len, json_tot_len);
        str_len = json_tot_len;
    }

    // 解析外层JSON
    root = cJSON_Parse(json_str);
    if (!root) {
        coze_err("JSON parse error: %s\n", cJSON_GetErrorPtr());
        goto exit;
    }

    // 获取data对象
    data_obj = cJSON_GetObjectItem(root, "data");
    if (!data_obj || !cJSON_IsObject(data_obj)) {
        coze_err("JSON structure error: data object not found or not an object\n");
        goto exit;
    }

    // 获取content字段
    content = cJSON_GetObjectItem(data_obj, "content");
    if (!content || !cJSON_IsString(content) || !content->valuestring) {
        coze_err("JSON structure error: content not found, not a string, or empty\n");
        goto exit;
    }

    // 尝试解析content字段为JSON
    content_json = cJSON_Parse(content->valuestring);
    if (content_json) {
        // content 是 JSON 格式，从 data 字段获取
        data_field = cJSON_GetObjectItem(content_json, "data");
        if (!data_field || !cJSON_IsString(data_field) || !data_field->valuestring) {
            coze_err("JSON structure error: data field not found in content JSON, not a string, or empty\n");
            goto exit;
        }
        // 查找base64标记
        marker_pos = os_strstr((char *)data_field->valuestring, base64_marker);
    } else {
        // content 不是 JSON 格式，直接从 content 字符串中提取
        coze_err("content is not JSON format, extracting directly from content string\n");
        marker_pos = os_strstr((char *)content->valuestring, base64_marker);
    }

    if (!marker_pos) {
        coze_err("base64 marker not found\n");
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
        coze_err("base64 content is empty\n");
        goto exit;
    }

    // 分配内存并清空
    base64_content = llm_malloc(*b64_len + 1);
    if (!base64_content) {
        coze_err("Memory allocation failed\n");
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
static int32 coze_msg_tti_base64_process(char *data, uint32 data_len)
{
    struct framebuff *fb = NULL;
    unsigned char *decoded_image_buf = NULL;
    uint32 decoded_image_len = 0;
    char *base64_content = NULL;
    uint32 base64_content_len = 0;
    int ret = 0;

    base64_content = coze_msg_tti_extract_base64_from_json(data, data_len, &base64_content_len);//reduce
    if (!base64_content) {
        coze_err("Extract base64 content failed!\n");
        return RET_ERR;
    }

    coze_err("Get TTI base64 data %p,len=%d\r\n", base64_content, base64_content_len);

    ret = llm_base64_decode(base64_content, &decoded_image_buf, &decoded_image_len);
    llm_free(base64_content);

    if (ret != RET_OK || decoded_image_buf == NULL || decoded_image_len == 0) {
        coze_err("Base64 decode failed (ret=%d, len=%d)\r\n", ret, decoded_image_len);
        return RET_ERR;
    }

    fb = msi_alloc_fb(coze_mgr.coze_msi, NULL, NULL, decoded_image_len, 0, 0);
    if (fb) {
        hw_memcpy(fb->data, decoded_image_buf, decoded_image_len);
        fb->codec_info = &coze_video_info;
        //sys_dcache_clean_invalid_range_unaligned((uint32_t *)fb->data, decoded_image_len);
        ret = msi_recv_fb(coze_mgr.image_msi, fb);
        if (ret) {
            coze_err("TTI image send fail!\r\n");
            fb_put(fb);
        }
    } else {
        coze_err("fb alloc fail!\r\n");
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
static int32 coze_msg_tti_process(char *data, uint32 data_len)
{
    const char *base64_key = ";base64,";
    if (NULL != os_strstr(data, base64_key)) {
        return coze_msg_tti_base64_process(data, data_len);//智能体通过base64压缩过的图片数据
    } else {
        return coze_msg_tti_url_process(data, data_len);//官方文档做法,需要解析URL并通过https获取图片数据
    }
}

/*
 * @brief: 处理Coze AI对话ID
 * @param: char *data 对话ID
 * @param: uint32 data_len 对话ID长度
 * @return: int32
*/
static int32 coze_msg_conversation_id_process(char *data, uint32 data_len)
{
    int32 ret = 0;
    if (os_strlen(data) > sizeof(sys_cfgs.coze_conversation_id)) {
        coze_err("Error:conversation_id too long!\n");
        return RET_ERR;
    }
    if (os_strcmp(sys_cfgs.coze_conversation_id, data) == 0) {
        return RET_OK;
    }
    //保存到conversation_id到flash
    coze_err("Modify conversation_id to %s\n", data);
    memset(sys_cfgs.coze_conversation_id, 0, sizeof(sys_cfgs.coze_conversation_id));
    os_strncpy(sys_cfgs.coze_conversation_id, data, os_strlen(data) + 1);
    syscfg_save();

    coze_sts_platform_cfg.chat_config_conversation_id = (char *)&sys_cfgs.coze_conversation_id;
    ret = llm_sts_config(coze_mgr.sts_session, LLM_CONFIG_TYPE_MODEL,
                         (void *)&coze_sts_platform_cfg, sizeof(coze_sts_platform_cfg));//填入config并update到llm库中
    if (ret) {
        coze_err("update conversation_id fail!\r\n");
        return RET_ERR;
    }
    return RET_OK;
}

/*
 * @brief: 处理Coze AI事件消息
 * @param: void
 * @return: int32
*/
int32 coze_msg_event_process(void)
{
    int32 ret = RET_OK;
    struct coze_demo_event_msg event_msg = {0};

    RB_INT_GET(&coze_mgr.event_queue, event_msg);
    if (event_msg.event == LLM_EVENT_UNKNOWN) { return RET_OK; }

    switch (event_msg.event) {
        case LLM_EVENT_CONNECTED: {
            coze_err("Coze AI connected!\r\n");
            coze_main_set_state(COZE_DEMO_STATE_CONNECTED);
            break;
        }
        case LLM_EVENT_DISCONNECT: {
            coze_err("Coze AI disconnect!\r\n");
            coze_main_set_state(COZE_DEMO_STATE_DISCONNECTED);
            break;
        }
        case LLM_EVENT_STT_RESULT: {
            if (event_msg.msg_len == 1) {
                coze_err("COZE AI STT(%d):", event_msg.msg_len);
                // event_msg.msg_len: 0/段结果；1/最终结果；
                hgprintf_out(event_msg.msg, os_strlen(event_msg.msg), 0);
                _os_printf("\r\n");
                coze_ui_set_user_text(event_msg.msg);
            }
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_TTS_RESULT: {
            if (event_msg.msg_len == 1) {
                coze_err("COZE AI TTS(%d):", event_msg.msg_len);
                // event_msg.msg_len: 0/段结果；1/最终结果；
                hgprintf_out(event_msg.msg, os_strlen(event_msg.msg), 0);
                _os_printf("\r\n");
                coze_ui_set_ai_text(event_msg.msg);
            }
            llm_free(event_msg.msg);
            coze_mgr.time_stamp = os_jiffies();
            coze_mgr.dialogue_timeout_cnt = 0;
            break;
        }
        case LLM_EVENT_VAD_RESULT: {
            coze_err("Coze AI recv server vad result:%s\r\n", event_msg.msg);
            if (os_strncmp(event_msg.msg, "START", event_msg.msg_len) == 0) {
                coze_mgr.time_stamp = UINT64_MAX;
            } else if (os_strncmp(event_msg.msg, "STOP", event_msg.msg_len) == 0) {
                coze_main_set_state(COZE_DEMO_STATE_READY);
            }
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_DIALOGUE_END: {
            coze_err("COZE AI check finish!\r\n");
            if (coze_mgr.time_stamp != 0 && coze_mgr.time_stamp != UINT64_MAX) {
                coze_mgr.finish = 1;
                coze_mgr.time_stamp = os_jiffies();
            }
            break;
        }
        case LLM_EVENT_DIALOGUE_TIMEOUT: {
            coze_err("COZE AI dialogue timeout!\r\n");
            coze_mgr.dialogue_timeout_cnt++;    //1s
            if (coze_mgr.dialogue_timeout_cnt > COZE_DEMO_DIALOGUE_PULLING_TIMEOUT) {
                coze_mgr.dialogue_timeout_cnt = 0;
                ret = coze_main_interrupt();
                if (ret != RET_OK) {
                    break;
                }
                coze_mgr.finish = 1;
                coze_mgr.time_stamp = os_jiffies();
                coze_err("End the current conversation and start a new one!\r\n");
            }
            break;
        }
        case LLM_EVENT_ERROR_MSG: {
            coze_err("COZE AI recv err msg(%d):", event_msg.msg_len);
            hgprintf_out(event_msg.msg, event_msg.msg_len, 0);
            _os_printf("\r\n");
            coze_msg_error_process(event_msg.msg);
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_CUSTOMIZE: {
            coze_err("COZE AI recv customize(%d):", event_msg.msg_len);
            hgprintf_out(event_msg.msg, event_msg.msg_len, 0);
            _os_printf("\r\n");
            //coze_msg_customize_process(event_msg.msg, event_msg.msg_len);
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_UPLOAD_FILE_RESULT: {
            coze_err("COZE AI recv upload file result(%d):", event_msg.msg_len);
            hgprintf_out(event_msg.msg, event_msg.msg_len, 0);
            _os_printf("\r\n");
            coze_msg_file_upload_process(event_msg.msg, event_msg.msg_len);
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_TTI_RESULT: {
            coze_err("COZE AI recv tti result(%d):", event_msg.msg_len);
            //hgprintf_out(event_msg.msg, event_msg.msg_len, 0);
            //_os_printf("\r\n");
            coze_msg_tti_process(event_msg.msg, event_msg.msg_len);
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_CONVERSATION_ID: {
            coze_err("Get conversation id:[%s]\n", event_msg.msg);
            coze_msg_conversation_id_process(event_msg.msg, event_msg.msg_len);
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_MPLAYER_RESULT: {
            coze_err("COZE AI recv media player result(%d):", event_msg.msg_len);
            coze_msg_mplayer_process(event_msg.msg, event_msg.msg_len);
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_IOT_RESULT: {
            coze_err("COZE AI recv iot result(%d):", event_msg.msg_len);
            coze_msg_iot_process(event_msg.msg, event_msg.msg_len);
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_TX_ERR:
        case LLM_EVENT_RX_ERR: {
            coze_err("COZE AI recv %s err!\r\n", (event_msg.event == LLM_EVENT_TX_ERR ? "TX" : "RX"));
            coze_main_set_state(COZE_DEMO_STATE_IDLE);
            llm_sts_reconnect(coze_mgr.sts_session);
            break;
        }
        default: {
            coze_err("Not supported event msg: %d\r\n", event_msg.event);
            break;
        }
    }
    return ret;
}

/*
 * @brief: 处理命令消息
 * @param: void
 * @return: int32
*/
int32 coze_msg_cmd_process(void)
{
    int32 ret = RET_OK;
    struct coze_demo_cmd_msg cmd_msg = {0};

    while (!RB_EMPTY(&coze_mgr.cmd_queue)) {
        RB_INT_GET(&coze_mgr.cmd_queue, cmd_msg);
        if (cmd_msg.cmd == COZE_DEMO_CMD_UNKNOWN) { continue; }

        switch (cmd_msg.cmd) {
            case COZE_DEMO_CMD_PAUSE: {
                if (coze_mgr.audio_url_hdl == cmd_msg.param1 && coze_mgr.pause == 0) {
                    coze_err("Coze CMD set pause: %d:%d!\r\n", cmd_msg.param1, cmd_msg.param2);
                    coze_mgr.pause = 1;
                }
                break;
            }
            case COZE_DEMO_CMD_PLAY: {
                if (coze_mgr.audio_url_hdl == cmd_msg.param1 && coze_mgr.pause == 1) {
                    coze_err("Coze CMD set play: %d:%d!\r\n", cmd_msg.param1, cmd_msg.param2);
                    coze_mgr.pause = 0;
                }
                break;
            }
            case COZE_DEMO_CMD_VOLUME: {
                if (coze_mgr.volume != cmd_msg.param2) {
                    coze_err("Coze CMD set volume: %d:%d!\r\n", cmd_msg.param1, cmd_msg.param2);
                    coze_mgr.volume = cmd_msg.param2 > 100 ? 100 : cmd_msg.param2 < 10 ? 10 : cmd_msg.param2;
                }
                break;
            }
            case COZE_DEMO_CMD_INTERRUPT: {
                coze_err("Coze CMD set interrupt: %d:%d!\r\n", cmd_msg.param1, cmd_msg.param2);
                coze_main_interrupt();
                coze_mgr.time_stamp = UINT64_MAX;
                if (cmd_msg.param1 == coze_mgr.ai_dialogue_screen_id) {
                    coze_mgr.ai_dialogue_screen_id = 0;
                } else if (cmd_msg.param1 == coze_mgr.music_player_screen_id) {
                    coze_mgr.music_player_screen_id = 0;
                }
                break;
            }
            case COZE_DEMO_CMD_CLOSE_MEDIA: {
                if (coze_mgr.audio_url_hdl == cmd_msg.param1) {
                    coze_err("Coze CMD close media: %d:%d!\r\n", cmd_msg.param1, cmd_msg.param2);
                    coze_mgr.audio_url_hdl = -1;
                    coze_mgr.time_stamp = UINT64_MAX;
                    coze_mgr.busy = 0;
                    if (cmd_msg.param2 == 0) {
                        coze_err("**媒体播放结束!**\n");
                        txmplayer_close(coze_mgr.audio_url_hdl);
                    } else if (cmd_msg.param2 == 1) {
                        coze_err("**媒体播放失败!**\n");
                        txmplayer_close(coze_mgr.audio_url_hdl);
                        // 重新进入对话
                        coze_mgr.finish = 1;
                        coze_main_set_state(COZE_DEMO_STATE_READY);
                    }
                }
                break;
            }
            case COZE_DEMO_CMD_PLAY_TIMEOUT: {
                if (coze_mgr.audio_url_hdl == cmd_msg.param1) {
                    coze_err("Coze CMD play timeout: %d:%d!\r\n", cmd_msg.param1, cmd_msg.param2);
                    coze_err("**媒体播放超时!**\n");
                    coze_main_timeout_warn();
                    // 停止TTS播放
                    msi_cmd2(coze_mgr.audio_msi, MSI_CMD_STOP, 0, 0);
                    // 媒体播放
                    txmplayer_pause(coze_mgr.audio_url_hdl, 0);
                }
                break;
            }
            default: {
                coze_err("Not supported cmd msg: %d\r\n", cmd_msg.cmd);
                break;
            }
        }
    }
    return ret;
}

int32 coze_msg_cmd_add(coze_demo_cmd cmd, uint32 param1, uint32 param2)
{
    struct coze_demo_cmd_msg cmd_msg = {
        .cmd      = cmd,
        .param1   = param1,
        .param2   = param2,
    };
    if (RB_INT_SET(&coze_mgr.cmd_queue, cmd_msg) == 0) {
        coze_err("cmd_queue is full!\r\n");
        return RET_ERR;
    }
    return RET_OK;
}
#endif

