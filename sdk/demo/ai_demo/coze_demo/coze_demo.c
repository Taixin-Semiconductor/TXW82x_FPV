#include "coze_demo.h"
#include "coze_material.h"

#if COZE_DEMO
/* -------------------------- 宏 -------------------------- */
/* Please add your server information */
#define COZE_URL        "wss://ws.coze.cn/v1/chat?bot_id="
#define COZE_BOT_ID     "xxx"
#define COZE_PAT_KEY    "xxx"
#define COZE_UPDATE_URL "https://api.coze.cn/v1/files/upload"
#define COZE_GET_EXTERNAL_IP_URL    "http://ipinfo.io/ip"

/* -------------------------- 内存重定向 -------------------------- */
extern void *llm_mem_alloc(uint32 size);
extern void llm_mem_free(void *ptr);

void *llm_malloc(uint32 size) {return llm_mem_alloc(size);}
void  llm_free(void *ptr) {llm_mem_free(ptr);}
void *llm_zalloc(size_t size)
{
    void *ptr = llm_malloc(size);
    if (ptr) {
        os_memset(ptr, 0, size);
    }
    return ptr;
}
void *llm_calloc(size_t nmemb, size_t size) {return llm_zalloc(nmemb * size);}
void *llm_realloc(void *ptr, size_t size)
{
    void *nptr = llm_malloc(size);
    if (nptr) {
        if (ptr) {
            os_memcpy(nptr, ptr, size);
            llm_free(ptr);
        }
    }
    return nptr;
}
char *llm_strdup(const char *s)
{
    size_t len;
    char *d;

    if (s == NULL) {
        return NULL;
    }

    len = os_strlen(s);
    d = llm_malloc(len + 1);
    if (d == NULL) {
        return NULL;
    }
    os_memcpy(d, s, len);
    d[len] = '\0';
    return d;
}
/* -------------------------- END -------------------------- */

static int32 coze_main_event_cb(void *session, uint16 evt, uint32 param1, uint32 param2);

/* -------------------------- 全局变量 -------------------------- */
/*
 * @brief 模型列表
*/
const struct llm_model models[] = {
    {"coze_sts", (struct llm_model_data *) &coze_sts_model},
};
/*
 * @brief llm 库全局参数
*/
struct llm_global_param global = {
    .task_reuse     = 0,
    .model_count    = 1,
    .models         = models,
};

/*
 * @brief 会话层参数
*/
struct llm_sts_sparam coze_sts_session_cfg = {
    .qmsg_tx_cnt            = 16,       //audio sample msg num
    .rx_buff_size           = 4096,     //audeo recv buff
    .cb_max_size            = 2048,
    .evt_cb                 = coze_main_event_cb,
};

/*
 * @brief 平台层参数
*/
struct coze_chat_platform_cfg coze_sts_platform_cfg = {
    .host_url               = COZE_URL,
    .pat_key                = COZE_PAT_KEY,
    .bot_id                 = COZE_BOT_ID,
    .update_url             = COZE_UPDATE_URL,

    .input_audio_format         = "pcm",
    //.input_audio_codec          = "g711a",
    //.input_audio_sample_rate    = "8000",
    .input_audio_sample_rate    = "16000",

    .output_audio_voice_id      = "7426725529589596187",
    .output_audio_limit_config_max_frame_num = "18",
    .output_audio_loudness_rate = "-10",

    //.output_audio_codec         = "mp3",
    .output_audio_codec                     = "opus",
    .output_audio_opus_config_bitrate       = MACRO_TO_STR(COZE_AUDIO_DEC_OPUS_BITRATE),
    .output_audio_opus_config_use_cbr       = "true",
    .output_audio_opus_config_sample_rate   = MACRO_TO_STR(COZE_AUDIO_DEC_OPUS_SAMPLE_RATE),
    .output_audio_opus_config_frame_size_ms = MACRO_TO_STR(COZE_AUDIO_DEC_OPUS_FRAME_SIZE_MS),
};

/*
 * @brief 传输层参数
*/
struct llm_trans_param coze_sts_trans_cfg = {
    .buffersize         = 2048,
    .upload_buffersize  = 2048,
    .low_speed_limit    = 10,
    .low_speed_time     = 5,
    .connect_timeout    = 5,
    .blob_len           = 0,
    .blob_data          = NULL,
};

/*
 * @brief 应用全局管理结构体
*/
struct coze_demo_manage coze_mgr = {
    .pwr_en             = 1,
    .key_triggered      = 0,
    .voice_triggered    = 0,
    .finish             = 0,
    .pause              = 0,
    .connected          = 0,
    .send_audio_cnt     = 0,
    .time_stamp         = UINT64_MAX,
    .audio_url_hdl      = -1,
    .volume             = 50,
};

/**
 * @brief 视频信息结构体
 */
txVideoInfo_t coze_video_info = {
    .width = 320,
    .height = 240,
};

/**
 * @brief 音频信息结构体
 */
txAudioInfo_t coze_audio_info = {
    .codec_id = AUDIO_CODEC_OPUS,
    .channels = 1,
    .sample_rate = COZE_AUDIO_DEC_OPUS_SAMPLE_RATE,
    .bit_rate = COZE_AUDIO_DEC_OPUS_BITRATE,
};
/* -------------------------- END -------------------------- */

/*******************************************************
从SD卡上传图片sample code,若要使用LLM视觉处理功能:
1.需要在project_config.h打开如下宏：
#define DVP_EN                          1
#define SDH_EN                          1   //SD卡，可以不要
#define FS_EN                           1   //文件系统，可以不要
#define OPENDML_EN                      1   //摄像头

2.将config-taixin.h中将#define CURL_DISABLE_MIME 屏蔽掉

3.确保coze智能体搭建支持视觉处理

4.请确定打开了PSRAM_HEAP以开启psram,并把libcurl的malloc/free/realloc/calloc/strdup使用的都是psram,不然内存可能不够

5.上传完成后，llm库会返回event:LLM_EVENT_UPLOAD_FILE_RESULT
从event的参数解析file_id(可参考coze_main_file_upload_process)，
用户可以保存fileid,并将需要跟智能体交互的fileid通过llm_sts_config更新到llm库

*******************************************************/
#define SD_CACHE_SIZE                           (4*1024)
int32 sys_atcmd_coze_upload_photo(const char *cmd, char *argv[], uint32 argc)
{
    char *cache_buf = NULL;
    const char *update_filename = NULL;
    void *fp = NULL;
    uint32_t filesize = 0;
    uint32_t file_tot_size = 0;
    uint32_t readsize = SD_CACHE_SIZE;
    uint32_t bytes_read = 0;
    int32 ret = 0;

    if (argc < 1) {
        coze_err("argv is too less\n");
        return RET_ERR;
    }
    update_filename = argv[0];
    fp = osal_fopen(update_filename, "rb");
    if (!fp) {
        coze_err("%s file not exist\n", update_filename);
        ret = RET_ERR;
        goto __atcmd_update_file_end;
    }

    filesize = osal_fsize(fp);
    file_tot_size = filesize;
    cache_buf = llm_malloc(SD_CACHE_SIZE);
    if (!cache_buf) {
        ret = RET_ERR;
        goto __atcmd_update_file_end;
    }
    coze_err("%s:filesize:%d,cache_buff:0x%x\n", __FUNCTION__, filesize, cache_buf);

    while (filesize) {
        if (filesize < SD_CACHE_SIZE) {
            readsize = filesize;
        } else {
            readsize = SD_CACHE_SIZE;
        }
        bytes_read = osal_fread(cache_buf, 1, readsize, fp);
        if (bytes_read == 0) {
            coze_err("Read file error\n");
            ret = RET_ERR;
            goto __atcmd_update_file_end;
        }

        if (filesize == file_tot_size) { // First chunk
            llm_sts_upload_file(coze_mgr.sts_session, LLM_DATA_TYPE_FILE, LLM_DATA_STATE_START, NULL, file_tot_size);
        }
        llm_sts_upload_file(coze_mgr.sts_session, LLM_DATA_TYPE_FILE, LLM_DATA_STATE_MIDDLE, cache_buf, bytes_read);
        filesize -= bytes_read;
    }
    llm_sts_upload_file(coze_mgr.sts_session, LLM_DATA_TYPE_FILE, LLM_DATA_STATE_END, NULL, 0);
    ret = RET_OK;

__atcmd_update_file_end:
    if (fp) {
        osal_fclose(fp);
    }
    if (cache_buf) {
        llm_free(cache_buf);
    }
    return ret;
}

/*
 * @brief 提交手动对话内容：conversation.message.create
 * @param id 对话ID
 * @param content 对话内容
 * @return 提交结果，0表示成功，其他值表示失败
*/
static int32 coze_main_send_manual_conversation_msg(uint64 id, char *content)
{
    int32 ret = 0;

    // 1. 参数合法性校验
    if (content == NULL) {
        coze_err("Input param error: content is NULL\n");
        return RET_ERR;
    }

    // 2. 定义JSON固定格式串

    const char *json_fmt = "{\"id\":\"%lld\",\"event_type\":\"conversation.message.create\",\"data\":{\"role\":\"user\",\"content_type\":\"text\",\"content\":\"%s\"}}";

    // 3. 第一步：仅计算所需总长度（不写入内存，安全）
    int32 need_len = os_snprintf(NULL, 0, json_fmt, id, content);
    if (need_len <= 0) {
        coze_err("Calculate json length failed\n");
        return RET_ERR;
    }
    // +1 存储字符串结束符 '\0'
    uint32 json_length = (uint32)need_len + 1;

    // 4. 精确分配内存（不多不少，无浪费）
    char *json_string = (char *)llm_zalloc(json_length);
    if (json_string == NULL) {
        coze_err("No memory for json string\n");
        return RET_ERR;
    }

    // 5. 第二步：真正格式化拼接（限定长度，绝对安全）
    os_snprintf(json_string, json_length, json_fmt, id, content);

    // 6. 发送数据
    ret = llm_sts_send(coze_mgr.sts_session, LLM_DATA_TYPE_RAW, LLM_DATA_STATE_START,
                       json_string, (os_strlen(json_string) + 1));
    if (ret != RET_OK) {
        coze_err("send manual conversation.message.create failed!\n");
        llm_free(json_string);
        return RET_ERR;
    }

    // 7. 释放内存并返回
    llm_free(json_string);
    return RET_OK;
}

/* -------------------------- 主要代码 -------------------------- */
const char *coze_main_state_str(coze_demo_state state)
{
    switch (state) {
        case COZE_DEMO_STATE_IDLE:
            return "IDLE";
        case COZE_DEMO_STATE_CONNECTED:
            return "CONNECTED";
        case COZE_DEMO_STATE_DISCONNECTED:
            return "DISCONNECTED";
        case COZE_DEMO_STATE_READY:
            return "READY";
        case COZE_DEMO_STATE_WAITING:
            return "WAITING";
        case COZE_DEMO_STATE_PUSHING:
            return "PUSHING";
        case COZE_DEMO_STATE_PULLING:
            return "PULLING";
        default:
            return "INVALID";
    }
    return "INVALID";
}

/*
 * @brief 获取当前状态
 * @return 当前状态
*/
coze_demo_state coze_main_get_state(void)
{
    coze_demo_state state = COZE_DEMO_STATE_IDLE;
    os_mutex_lock(&coze_mgr.lock, osWaitForever);
    state = coze_mgr.state;
    os_mutex_unlock(&coze_mgr.lock);
    return state;
}

/*
 * @brief 设置当前状态
 * @param new_state 新状态
*/
void coze_main_set_state(coze_demo_state new_state)
{
    int32 changed = 0;
    if (coze_mgr.state == new_state) { return; }
    os_mutex_lock(&coze_mgr.lock, osWaitForever);
    switch (new_state) {
        case COZE_DEMO_STATE_IDLE:
            changed = 1;
            break;
        case COZE_DEMO_STATE_CONNECTED:
            changed = 1;
            break;
        case COZE_DEMO_STATE_DISCONNECTED:
            coze_mgr.time_stamp = UINT64_MAX;
            changed = 1;
            break;
        case COZE_DEMO_STATE_READY:
            if (coze_mgr.connected) {
                changed = 1;
            }
            break;
        case COZE_DEMO_STATE_PULLING:
            if (coze_mgr.connected) {
                changed = 1;
            }
            break;
        case COZE_DEMO_STATE_WAITING:
            coze_mgr.time_stamp = UINT64_MAX;
            if (coze_mgr.connected) {
                changed = 1;
            }
            break;
        case COZE_DEMO_STATE_PUSHING:
            if (coze_mgr.connected) {
                changed = 1;
                coze_mgr.coze_msi->enable = 1;
            }
            break;
        default:
            break;
    }
    if (changed) {
        coze_err("New State:%s -> %s\r\n", coze_main_state_str(coze_mgr.state), coze_main_state_str(new_state));
        if (coze_mgr.state == COZE_DEMO_STATE_PUSHING) {
            coze_mgr.coze_msi->enable = 0;
            msi_clear(coze_mgr.coze_msi);
        }
        if (coze_mgr.state != COZE_DEMO_STATE_IDLE && coze_mgr.state != COZE_DEMO_STATE_WAITING) {
            coze_mgr.last_state = coze_mgr.state;
        }
        coze_mgr.state = new_state;
    } else {
        coze_err("invalid state: %s -> %s\r\n", coze_main_state_str(coze_mgr.state), coze_main_state_str(new_state));
    }
    os_mutex_unlock(&coze_mgr.lock);
}

/*
 * @brief 超时警告函数
 * @param 无
*/
void coze_main_timeout_warn(void)
{
    coze_err("**当前网络较差，请稍等!**\n");
    coze_audio_play_prompt_tone(wangluocha, wangluocha_size);
    os_sleep_ms(3000);
}

/*
 * @brief 超时重连函数
 * @param 无
*/
void coze_main_timeout_reset(void)
{
    coze_err("**网络超时，即将重连!**\n");
    coze_audio_play_prompt_tone(chaoshi, chaoshi_size);
    os_sleep_ms(3000);
    coze_main_set_state(COZE_DEMO_STATE_IDLE);
    llm_sts_reconnect(coze_mgr.sts_session);
}

/*
 * @brief 检查是否退出对话
 * @param 无
 * @return 操作结果，0表示成功，其他值表示失败
*/
int32 coze_main_check_exit_dialog(void)
{
    int32 ret = RET_OK;
    uint64 jiff = 0;
    if (coze_mgr.time_stamp != UINT64_MAX) {
        jiff = os_jiffies() - coze_mgr.time_stamp;
        if (os_jiffies_to_msecs(jiff) > (COZE_DEMO_DIALOGUE_IDLE_TIMEOUT * 1000) && llm_sts_check_audio(coze_mgr.sts_session)) {
            ret = coze_main_interrupt();
            if (ret != RET_OK) {
                return ret;
            }
            coze_err("**自动退出**\r\n");
            coze_audio_play_prompt_tone(tuixia, tuixia_size);
            os_sleep_ms(2000);
            coze_mgr.time_stamp = UINT64_MAX;
            coze_mgr.finish = 0;
            coze_mgr.busy = 0;
            coze_ui_destroy(COZE_UI_ID_AI_DIALOGUE);
            if (coze_mgr.last_state == COZE_DEMO_STATE_PULLING) {
                coze_main_set_state(COZE_DEMO_STATE_PULLING);
            } else {
                coze_main_set_state(COZE_DEMO_STATE_READY);
            }
        }
    }
    return ret;
}

/*
 * @brief 等待进入监听函数
 * @param 无
 * @return 操作结果，0表示成功，其他值表示失败
*/
int32 coze_main_waiting(void)
{
    int32 ret = RET_OK;
    uint32 timeout = 0;
    do {
        ret = llm_sts_wait(coze_mgr.sts_session, 1000);
        if (ret == RET_ERR) {
            coze_err("This can be confirmed to be a disconnection!\r\n");
            break;
        } else if (ret == LLME_WAIT_TIMEOUT) {
            timeout++;
            if (timeout == (COZE_DEMO_DIALOGUE_WAITING_TIMEOUT / 2)) {
                coze_main_timeout_warn();
            }
            if (timeout > COZE_DEMO_DIALOGUE_WAITING_TIMEOUT) {
                coze_main_timeout_reset();
                break;
            }
        }
    } while (ret == LLME_WAIT_TIMEOUT);
    return ret;
}

/*
 * @brief 打断
 * @param 无
 * @return 操作结果，0表示成功，其他值表示失败
*/
int32 coze_main_interrupt(void)
{
    int32 ret = RET_OK;
    uint32 timeout = 0;
    coze_audio_mute();
    do {
        ret = llm_sts_interrupt(coze_mgr.sts_session, 1000);
        if (ret == RET_ERR) {
            coze_err("This can be confirmed to be a disconnection!\r\n");
            break;
        } else if (ret == LLME_WAIT_TIMEOUT) {
            timeout++;
            if (timeout == (COZE_DEMO_DIALOGUE_INTERRUPT_TIMEOUT / 2)) {
                coze_audio_wait_empty();
                coze_audio_restore();
                coze_main_timeout_warn();
            }
            if (timeout > COZE_DEMO_DIALOGUE_INTERRUPT_TIMEOUT) {
                coze_main_timeout_reset();
                break;
            }
        }
    } while (ret == LLME_WAIT_TIMEOUT);

    coze_audio_wait_empty();
    coze_audio_restore();
    return ret;
}

/*
 * @brief 事件回调函数(不可阻塞)
 * @param session 会话指针
 * @param evt 事件类型
 * @param param1 事件参数1
 * @param param2 事件参数2
 * @return 操作结果，0表示成功，其他值表示失败
*/
int32 coze_main_event_cb(void *session, uint16 evt, uint32 param1, uint32 param2)
{
    int ret = RET_OK;
    uint8 process = 0;
    struct coze_demo_event_msg event_msg = {
        .event      = evt,
        .msg        = NULL,
        .msg_len    = 0,
    };
    switch (evt) {
        case LLM_EVENT_CONNECTED:
        case LLM_EVENT_DISCONNECT:
        case LLM_EVENT_TX_ERR:
        case LLM_EVENT_RX_ERR:
        case LLM_EVENT_DIALOGUE_END:
        case LLM_EVENT_DIALOGUE_TIMEOUT: {
            process = 1;
            break;
        }
        case LLM_EVENT_STT_RESULT: {
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                coze_err("stt result no memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        }
        case LLM_EVENT_TTS_RESULT: {
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                coze_err("tts result no memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        }
        case LLM_EVENT_VAD_RESULT: {
            event_msg.msg = llm_strdup((const char *)(param1 == COZE_SERVER_VAD_START ? "START" : "STOP"));
            if (event_msg.msg == NULL) {
                coze_err("vad result no memory!\r\n");
                break;
            }
            event_msg.msg_len = os_strlen(event_msg.msg);
            process = 1;
            break;
        }
        case LLM_EVENT_ERROR_MSG: {
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                coze_err("err msg no memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        }
        case LLM_EVENT_CUSTOMIZE: {
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                coze_err("customize no memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        }
        case LLM_EVENT_UPLOAD_FILE_RESULT: {
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                coze_err("customize no memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        }
        case LLM_EVENT_TTI_RESULT: {
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                coze_err("tti result memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        }
        case LLM_EVENT_CONVERSATION_ID: {
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                coze_err("conversation memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        }
        case LLM_EVENT_MPLAYER_RESULT: {
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                coze_err("media player no memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        }
        case LLM_EVENT_IOT_RESULT: {
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                coze_err("iot no memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        }
        default:
            break;
    }
    if (process) {
        if (RB_INT_SET(&coze_mgr.event_queue, event_msg) == 0) {
            coze_err("event_queue is full!\r\n");
        }
    }
    return ret;
}

/**
 * @brief 系统事件回调函数
 * @param event_id 事件ID
 * @param data 事件数据
 * @param priv 私有数据
 * @return 操作结果，0表示成功，其他值表示失败
*/
sysevt_hdl_res coze_main_system_event(uint32 event_id, uint32 data, uint32 priv)
{
    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_SYSTEM, SYSEVT_SYSTEM_TOTAL_VOLUME):
            coze_msg_cmd_add(COZE_DEMO_CMD_VOLUME, 0, data);
            break;
    }
    return SYSEVT_CONTINUE;
}

/*
 * @brief 系统无线事件回调函数
 * @param event_id 事件ID
 * @param data 事件数据
 * @param priv 私有数据
 * @return 操作结果，0表示成功，其他值表示失败
*/
sysevt_hdl_res coze_main_wifi_event(uint32 event_id, uint32 data, uint32 priv)
{
    switch (event_id & 0xffff) {
        case SYSEVT_WIFI_DISCONNECT:
            coze_err("**网络异常!**\r\n", event_id);
            coze_main_set_state(COZE_DEMO_STATE_IDLE);
            llm_sts_reconnect(coze_mgr.sts_session);
            break;
        case SYSEVT_WIFI_CONNECTTED:
            break;
    }
    return SYSEVT_CONTINUE;
}

/*
 * @brief 系统网络事件回调函数
 * @param event_id 事件ID
 * @param data 事件数据
 * @param priv 私有数据
 * @return 操作结果，0表示成功，其他值表示失败
*/
sysevt_hdl_res coze_main_network_event(uint32 event_id, uint32 data, uint32 priv)
{
    struct netif *nif;

    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_NETWORK, SYSEVT_LWIP_DHCPC_DONE):
            nif = netif_find("w0");
            gethostbyname_async("ws.coze.cn");
            gethostbyname_async("lf-bot-studio-plugin-resource.coze.cn");
            if (coze_mgr.ip_addr != nif->ip_addr.addr) {
                coze_mgr.ip_addr = nif->ip_addr.addr;
                coze_main_set_state(COZE_DEMO_STATE_IDLE);
                llm_sts_reconnect(coze_mgr.sts_session);
            }
            break;
    }
    return SYSEVT_CONTINUE;
}

/*
 * @brief 系统ASR事件回调函数
 * @param event_id 事件ID
 * @param data 事件数据
 * @param priv 私有数据
 * @return 操作结果，0表示成功，其他值表示失败
*/
sysevt_hdl_res coze_main_asr_event(uint32 event_id, uint32 data, uint32 priv)
{
    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_ASR, SYSEVT_ASR_WAKEUP):
            coze_mgr.voice_triggered = 1;
            break;
    }
    return SYSEVT_CONTINUE;
}

/*
 * @brief 系统媒体事件回调函数
 * @param event_id 事件ID
 * @param data 事件数据
 * @param priv 私有数据
 * @return 操作结果，0表示成功，其他值表示失败
*/
sysevt_hdl_res coze_main_media_event(uint32 event_id, uint32 data, uint32 priv)
{
    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_MEDIA, SYSEVT_MEDIA_PLAY_PAUSE):
            coze_msg_cmd_add(COZE_DEMO_CMD_PAUSE, data, 0);
            break;
        case SYS_EVENT(SYS_EVENT_MEDIA, SYSEVT_MEDIA_PLAY_START):
            coze_msg_cmd_add(COZE_DEMO_CMD_PLAY, data, 0);
            break;
        case SYS_EVENT(SYS_EVENT_MEDIA, SYSEVT_MEDIA_VOLUME):
            coze_msg_cmd_add(COZE_DEMO_CMD_VOLUME, data & 0xff, data >> 8);
            break;
        case SYS_EVENT(SYS_EVENT_MEDIA, SYSEVT_MEDIA_PLAY_CLOSE):
            coze_msg_cmd_add(COZE_DEMO_CMD_CLOSE_MEDIA, data, 2);   //2: 正常退出
            break;
        case SYS_EVENT(SYS_EVENT_MEDIA, SYSEVT_MEDIA_PLAY_END):
            coze_msg_cmd_add(COZE_DEMO_CMD_CLOSE_MEDIA, data, 0);   //0: 需关闭txmplayer
            break;
        case SYS_EVENT(SYS_EVENT_MEDIA, SYSEVT_MEDIA_OPEN_FAIL):
            coze_msg_cmd_add(COZE_DEMO_CMD_CLOSE_MEDIA, data, 1);   //1：需关闭txmplayer并重新进入对话
            break;
        case SYS_EVENT(SYS_EVENT_MEDIA, SYSEVT_MEDIA_BUFFERING_DATA):
            coze_msg_cmd_add(COZE_DEMO_CMD_PLAY_TIMEOUT, data & 0xff, 0);
            break;
    }
    return SYSEVT_CONTINUE;
}

static void coze_main_conversation_id_init(void)
{
    char header[64];
    memset(&header, 0xFF, sizeof(header));
    if (os_memcmp(sys_cfgs.coze_conversation_id,
                  header, sizeof(sys_cfgs.coze_conversation_id)) != 0) {
        coze_sts_platform_cfg.chat_config_conversation_id = (char *)&sys_cfgs.coze_conversation_id;
        coze_err("use syscfg conversation_id:%s\n", coze_sts_platform_cfg.chat_config_conversation_id);
    }
}

#if COZE_DEMO_GET_EXTERNAL_IP
size_t coze_main_get_external_ip_callback(void *contents, size_t size, size_t nmemb, char *userp)
{
    char *buff = userp;

    size_t realsize = size * nmemb;
    os_strncpy(buff, contents, realsize);
    buff[realsize] = '\0'; // 添加字符串结束符
    coze_err("Get external ip info:[%s]\n", buff);
    return realsize;
}

static int32 coze_main_get_external_ip(char *ip, int32 buff_len)
{
    uint32 tick = 0;
    int32 ret = 0;
    void *handle = NULL;
    char *buff = NULL;

    buff = llm_malloc(2048);
    if (!buff) {
        coze_err("Error,no memory!\n");
        return ERR_MEM;
    }

    tick = os_jiffies();
    handle = llm_https_connect(NULL, (void *)&coze_sts_trans_cfg,
                               coze_main_get_external_ip_callback,
                               NULL, NULL, buff);
    if (handle == NULL) {
        coze_err("https connect failed!\n");
        ret = RET_ERR;
        goto __cleanup;
    }

    ret = llm_https_send(handle, LLM_HTTP_GET, COZE_GET_EXTERNAL_IP_URL, NULL, 0);
    if (ret) {
        coze_err("https send failed!\n");
        ret = RET_ERR;
        goto __cleanup;
    }

    coze_err("Get external ip from %s (%dms) success\r\n", COZE_GET_EXTERNAL_IP_URL,
              os_jiffies_to_msecs(os_jiffies() - tick));
    if (ip) {
        memset(ip, 0, buff_len);
        os_strncpy(ip, buff, os_strlen(buff) > buff_len ? buff_len : os_strlen(buff))
        ;
    }
    coze_err("Get external ip info:[%s]\n", ip);
    llm_free(buff);
    return ret;

__cleanup:
    if (buff) {
        llm_free(buff);
    }
    if (handle) {
        llm_https_disconnect(handle);
    }
    return ret;
}

void coze_get_external_ip_main(void *arg)
{
    char ip[40] = {0};
    int ret = 0;
    char *json = NULL;

    while (1) {
        coze_err("Connect to external ip server...\n");
        ret = coze_main_get_external_ip(ip, sizeof(ip));
        if (ret) {
            coze_err("get external ip error:%d\n", ret);
            os_sleep_ms(2000);
            continue;
        }
        break;
    }
    if (ret == 0) {
        json = coze_msg_chat_config_parameters_append_ip(coze_mgr.chat_config_parameter, ip);
        os_mutex_lock(&coze_mgr.lock, osWaitForever);
        os_memset(coze_mgr.chat_config_parameter, 0, sizeof(coze_mgr.chat_config_parameter));
        os_strncpy(coze_mgr.chat_config_parameter, json,
                   os_strlen(json) > sizeof(coze_mgr.chat_config_parameter) ? sizeof(coze_mgr.chat_config_parameter) : os_strlen(json));
        os_mutex_unlock(&coze_mgr.lock);
        ret = llm_sts_config(coze_mgr.sts_session, LLM_CONFIG_TYPE_MODEL,
                             (void *)&coze_sts_platform_cfg, sizeof(coze_sts_platform_cfg));//填入config并update到llm库中
        if (ret) {
            coze_err("sts_model_cfg fail!\r\n");
        }
        coze_mgr.get_public_ip_done = 1;
    }
    llm_free(json);
    coze_err("external ip task exit...\n");
}
#endif

/*
 * @brief 应用初始化函数
 * @param llm_name LLM名称
 * @return 操作结果，0表示成功，其他值表示失败
*/
struct os_task coze_audio_play_task;
struct os_task coze_get_extern_ip_task;
int32 coze_main_app_init(char *llm_name)
{
    int ret = 0;
    struct txmplayer_param param = {
        .volume = coze_mgr.volume,
    };
    struct coze_demo_event_msg *event_queue_buf = NULL;
    struct coze_demo_cmd_msg *cmd_queue_buf = NULL;
    llm_global_init(&global);

    coze_mgr.sts_session = llm_sts_init(llm_name, &coze_sts_session_cfg);
    if (!coze_mgr.sts_session) {
        coze_err("sts session init fail!\r\n");
        return RET_ERR;
    }

    ret = llm_sts_config(coze_mgr.sts_session, LLM_CONFIG_TYPE_TRANS,
                         (void *)&coze_sts_trans_cfg, sizeof(coze_sts_trans_cfg));
    if (ret) {
        coze_err("sts_trans_cfg fail!\r\n");
        return RET_ERR;
    }

    coze_sts_platform_cfg.turn_detection_type = "client_interrupt";
    ret = llm_sts_config(coze_mgr.sts_session, LLM_CONFIG_TYPE_MODEL,
                         (void *)&coze_sts_platform_cfg, sizeof(coze_sts_platform_cfg));
    if (ret) {
        coze_err("sts_trans_cfg fail!\r\n");
        return RET_ERR;
    }
    coze_sts_platform_cfg.chat_config_parameters = coze_mgr.chat_config_parameter;

    os_mutex_init(&coze_mgr.lock);

    event_queue_buf = llm_malloc((COZE_DEMO_EVENT_QUEUE_CNT + 1) * sizeof(struct coze_demo_event_msg));
    if (!event_queue_buf) {
        coze_err("event_queue_buf malloc fail, no memory!\r\n");
        return RET_ERR;
    }
    RB_INIT_R(&coze_mgr.event_queue, COZE_DEMO_EVENT_QUEUE_CNT, event_queue_buf);

    cmd_queue_buf = llm_malloc((COZE_DEMO_CMD_QUEUE_CNT + 1) * sizeof(struct coze_demo_cmd_msg));
    if (!cmd_queue_buf) {
        coze_err("cmd_queue_buf malloc fail, no memory!\r\n");
        return RET_ERR;
    }
    RB_INIT_R(&coze_mgr.cmd_queue, COZE_DEMO_CMD_QUEUE_CNT, cmd_queue_buf);

    coze_mgr.coze_msi = msi_new("coze_msi", 8, NULL);//确保比adc的fb队列小，自己丢数据，不要影响其他的模块
    if (!coze_mgr.coze_msi) {
        coze_err("coze_msi err!\r\n");
        return RET_ERR;
    }
    coze_mgr.coze_msi->fb_limits.counter = 32;
    coze_mgr.coze_msi->enable = 0;

    // 麦克风
    auadc_msi_add_output(AUSYS_AUAD, coze_mgr.coze_msi->name);

    txmplayer_init(0, 0, &param);
    // 音频
    coze_mgr.audio_msi = msi_find2("txmplayer", 0, 0, NULL);
    coze_mgr.audio_msi->type = MEDIA_DATA_AUDIO << 8 | AUDIO_CODEC_OPUS;
    msi_add_output(coze_mgr.coze_msi, NULL, coze_mgr.audio_msi, NULL);
    // 图片
    coze_mgr.image_msi = msi_find2("txmplayer", 0, 0, NULL);
    //coze_mgr.image_msi->type = MEDIA_DATA_PICTURE << 8 | PICTURE_FORMAT_JPEG;
    //msi_add_output(coze_mgr.coze_msi, NULL, coze_mgr.image_msi, NULL);

    OS_TASK_INIT("COZE_AUPLAY", &coze_audio_play_task, coze_audio_play_thread, NULL, OS_TASK_PRIORITY_NORMAL, NULL, 1024);
    coze_main_conversation_id_init();

    coze_audio_play_prompt_tone(kaiji, kaiji_size);
    os_sleep_ms(2000);

    coze_err("Waiting for network connection...");
    while (!sys_status.wifi_connected || !sys_status.dhcpc_done) {
        if (coze_mgr.pwr_en == 0) {
            return RET_ERR;
        }
        _os_printf(".");
        os_sleep(1);
    }
    coze_err("Network connected!\r\n");

    struct netif *nif = netif_find("w0");
    coze_mgr.ip_addr = nif->ip_addr.addr;

#if COZE_DEMO_GET_EXTERNAL_IP
    OS_TASK_INIT("COZE_GETIP", &coze_get_extern_ip_task, coze_get_external_ip_main, NULL,
                 OS_TASK_PRIORITY_NORMAL, NULL, 8192);
#endif

    sys_event_take(SYS_EVENT(SYS_EVENT_SYSTEM, 0), coze_main_system_event, 0);
    sys_event_take(SYS_EVENT(SYS_EVENT_WIFI, 0), coze_main_wifi_event, 0);
    sys_event_take(SYS_EVENT(SYS_EVENT_NETWORK, 0), coze_main_network_event, 0);
    sys_event_take(SYS_EVENT(SYS_EVENT_ASR, 0), coze_main_asr_event, 0);
    sys_event_take(SYS_EVENT(SYS_EVENT_MEDIA, 0), coze_main_media_event, 0);

    coze_main_set_state(COZE_DEMO_STATE_IDLE);
    llm_sts_reconnect(coze_mgr.sts_session);
    return ret;
}

/*
 * @brief 主线程
*/
void coze_main(void)
{
    int32 ret = RET_OK;
    coze_demo_state state = COZE_DEMO_STATE_IDLE;
    uint8 last_key_triggered = 0;   // 记录上一次按键状态；
    uint8 dialogue_mode = 0;        // 对话模式：coze_demo_dialogue_mode；
    uint8 send_complete = 0;        // 按键模式下，需发送完成帧告知服务器音频数据上传结束；
    uint64 update_weather_time = 0;
    uint8  need_update_weather;

    ret = coze_main_app_init("coze_sts");
    if (ret) {
        coze_err("coze main init failed: %d\r\n", ret);
        goto cleanup;
    }

    while (coze_mgr.pwr_en) {
        coze_msg_event_process();
        coze_msg_cmd_process();
    
        state = coze_main_get_state();
        if (coze_mgr.connected == 1 && state != COZE_DEMO_STATE_IDLE) {
            // 0. 处理获取天气事件
            if (coze_mgr.get_public_ip_done == 1 && coze_mgr.busy == 0) {
                need_update_weather = 0;
                if (coze_mgr.get_weather_done == 0) {
                    if (update_weather_time == 0 ||
                        os_jiffies_to_msecs(os_jiffies()-update_weather_time) > 10 * 1000) {
                        need_update_weather = 1;
                    }
                } else {
                    if (os_jiffies_to_msecs(os_jiffies()-coze_mgr.update_weather_time) > (COZE_DEMO_UPDATE_WEATHER_INTERVAL * 1000)) {
                       need_update_weather = 1;
                    }
                }
                if (need_update_weather == 1) {
                    if (dialogue_mode != COZE_DEMO_DIALOGUE_MODE_UPDATE_WEATHER) {
                        dialogue_mode = COZE_DEMO_DIALOGUE_MODE_UPDATE_WEATHER;
                    }
                    coze_main_set_state(COZE_DEMO_STATE_WAITING);
                }
            }
            // 1. 处理上传图片事件
            if (coze_mgr.img_triggered) {
                coze_mgr.img_triggered = 0;
                if (dialogue_mode != COZE_DEMO_DIALOGUE_MODE_IMAGE_UPDATED) {
                    dialogue_mode = COZE_DEMO_DIALOGUE_MODE_IMAGE_UPDATED;
                }
                coze_main_set_state(COZE_DEMO_STATE_WAITING);
            }
            // 2. 处理语音触发事件
            if (coze_mgr.voice_triggered) {
                coze_mgr.voice_triggered = 0;
                if (dialogue_mode != COZE_DEMO_DIALOGUE_MODE_VOICE) {
                    coze_sts_platform_cfg.turn_detection_type = "server_vad";
                    llm_sts_config(coze_mgr.sts_session, LLM_CONFIG_TYPE_MODEL,
                                   (void *)&coze_sts_platform_cfg, sizeof(coze_sts_platform_cfg));
                    dialogue_mode = COZE_DEMO_DIALOGUE_MODE_VOICE;
                }
                coze_main_set_state(COZE_DEMO_STATE_WAITING);
            }
            // 3. 处理按键触发事件
            if (last_key_triggered != coze_mgr.key_triggered) {
                // 按键按下
                if (coze_mgr.key_triggered == 1) {
                    if (dialogue_mode != COZE_DEMO_DIALOGUE_MODE_KEYBOARD) {
                        // 按键配置与语音配置不同（turn_detection_type），需更新
                        coze_sts_platform_cfg.turn_detection_type = "client_interrupt";
                        llm_sts_config(coze_mgr.sts_session, LLM_CONFIG_TYPE_MODEL,
                                       (void *)&coze_sts_platform_cfg, sizeof(coze_sts_platform_cfg));
                        dialogue_mode = COZE_DEMO_DIALOGUE_MODE_KEYBOARD;
                    }
                    coze_main_set_state(COZE_DEMO_STATE_WAITING);
                }
                // 按键松开
                else {
                    // 若发送过音频数据需发送完成帧
                    if (coze_mgr.send_audio_cnt > 0) {
                        send_complete = 1;
                    }
                    coze_main_set_state(COZE_DEMO_STATE_READY);
                }
                last_key_triggered = coze_mgr.key_triggered;
            }
        }
        switch (state) {
            case COZE_DEMO_STATE_IDLE:
                break;
            case COZE_DEMO_STATE_CONNECTED: {
                coze_err("**已连接!**\n");
                coze_audio_play_prompt_tone(yilianjie, yilianjie_size);
                os_sleep_ms(1000);
                
                coze_mgr.key_triggered   = 0;
                coze_mgr.voice_triggered = 0;
                coze_mgr.img_triggered   = 0;
                coze_mgr.finish          = 0;
                coze_mgr.connected       = 1;
                last_key_triggered       = 0;
                send_complete            = 0;

                // 连接成功后，需更新一次配置
                if (dialogue_mode == COZE_DEMO_DIALOGUE_MODE_VOICE) {
                    coze_sts_platform_cfg.turn_detection_type = "server_vad";
                } else if (dialogue_mode == COZE_DEMO_DIALOGUE_MODE_KEYBOARD) {
                    coze_sts_platform_cfg.turn_detection_type = "client_interrupt";
                }
                llm_sts_config(coze_mgr.sts_session, LLM_CONFIG_TYPE_MODEL,
                               (void *)&coze_sts_platform_cfg, sizeof(coze_sts_platform_cfg));
                if (coze_mgr.last_state == COZE_DEMO_STATE_PULLING) {
                    coze_main_set_state(COZE_DEMO_STATE_PULLING);
                } else {
                    coze_main_set_state(COZE_DEMO_STATE_READY);
                }
                break;
            }
            case COZE_DEMO_STATE_READY: {
                if (coze_mgr.finish && llm_sts_check_audio(coze_mgr.sts_session) && ausys_da_fifo_is_empty()) {
                    if ((coze_mgr.pause & 0x1) == 0 &&
                        coze_mgr.audio_url_hdl >= 0 &&
                        txmplayer_state(coze_mgr.audio_url_hdl) == TXMPLAYER_STATE_PAUSE) {
                        coze_err("**媒体播放**\r\n");
                        coze_ui_create(COZE_UI_ID_MUSIC_PLAYER);
                        // 停止TTS播放
                        msi_cmd2(coze_mgr.audio_msi, MSI_CMD_STOP, 0, 0);
                        // 媒体播放
                        txmplayer_pause(coze_mgr.audio_url_hdl, 0);
                        coze_mgr.finish = 0;
                        coze_mgr.pause = 0;
                        coze_main_set_state(COZE_DEMO_STATE_PULLING);
                    } else if (dialogue_mode == COZE_DEMO_DIALOGUE_MODE_VOICE) {
                        coze_err("**新对话：**\r\n");
                        coze_main_set_state(COZE_DEMO_STATE_WAITING);
                    } else if (dialogue_mode == COZE_DEMO_DIALOGUE_MODE_KEYBOARD) {
                        coze_mgr.busy = 0;
                    } else if (dialogue_mode == COZE_DEMO_DIALOGUE_MODE_IMAGE_UPDATED) {
                        coze_mgr.busy = 0;
                    }
                }
                // 按键触发模式
                if (dialogue_mode == COZE_DEMO_DIALOGUE_MODE_KEYBOARD) {
                    if (send_complete) {
                        ret = llm_sts_send(coze_mgr.sts_session, LLM_DATA_TYPE_AUDIO, LLM_DATA_STATE_END, NULL, 0);
                        if (ret == RET_OK) {
                            send_complete = 0;
                            coze_audio_play_prompt_tone(fasong, fasong_size);
                        } else if (ret != LLME_AGAIN) {
                            coze_err("send_complete fail!\r\n");
                            coze_main_set_state(COZE_DEMO_STATE_IDLE);
                            llm_sts_reconnect(coze_mgr.sts_session);
                            break;
                        }
                    }
                } else if (dialogue_mode == COZE_DEMO_DIALOGUE_MODE_VOICE) {
                    coze_main_check_exit_dialog();
                }
                break;
            }
            case COZE_DEMO_STATE_WAITING: {
                // 打断上一次对话
                ret = coze_main_interrupt();
                if (ret != RET_OK) {
                    break;
                }
                // 等待服务器进入监听状态
                ret = coze_main_waiting();
                if (ret != RET_OK) {
                    break;
                }
                if (dialogue_mode == COZE_DEMO_DIALOGUE_MODE_VOICE) {
                    coze_ui_create(COZE_UI_ID_AI_DIALOGUE);
                    coze_ui_create(COZE_UI_ID_AI_THINKING);
                    if (coze_mgr.finish == 0) {
                        coze_err("**我在，请说：**\r\n");
                        coze_audio_play_prompt_tone(wozai, wozai_size);
                        os_sleep_ms(1000);
                    }
                    coze_mgr.finish = 0;
                    coze_mgr.time_stamp = os_jiffies();
                } else if (dialogue_mode == COZE_DEMO_DIALOGUE_MODE_IMAGE_UPDATED) {
                    coze_main_send_manual_conversation_msg(os_jiffies(), "这张图片里有什么");
                    coze_main_set_state(COZE_DEMO_STATE_READY);
                } else if (dialogue_mode == COZE_DEMO_DIALOGUE_MODE_UPDATE_WEATHER) {
                    coze_main_send_manual_conversation_msg(os_jiffies(), "仅查询天气不返回语音");
                    coze_err("** 正在查询天气... **\r\n");
                    coze_mgr.get_weather_done = 0;
                    update_weather_time = os_jiffies();
                    coze_main_set_state(COZE_DEMO_STATE_READY);
                }
                if (dialogue_mode != COZE_DEMO_DIALOGUE_MODE_IMAGE_UPDATED && 
                    dialogue_mode != COZE_DEMO_DIALOGUE_MODE_UPDATE_WEATHER) {
                    coze_audio_play_prompt_tone(beep, beep_size);
                    coze_main_set_state(COZE_DEMO_STATE_PUSHING);
                    coze_mgr.send_audio_cnt = 0;
                }
                if (dialogue_mode != COZE_DEMO_DIALOGUE_MODE_UPDATE_WEATHER) {
                    coze_mgr.busy = 1;
                }
                break;
            }
            case COZE_DEMO_STATE_PULLING: {
                break;
            }
            case COZE_DEMO_STATE_PUSHING: {
                if (dialogue_mode == COZE_DEMO_DIALOGUE_MODE_VOICE) {
                    coze_main_check_exit_dialog();
                }
                // 发送音频数据
                coze_audio_send_data();
                break;
            }
            case COZE_DEMO_STATE_DISCONNECTED: {
                coze_err("**已断开!**\n");                
                txmplayer_pause(coze_mgr.audio_url_hdl, 1);
                coze_audio_play_prompt_tone(yiduankai, yiduankai_size);
                os_sleep_ms(1000);
                coze_mgr.connected = 0;
                coze_ui_destroy(COZE_UI_ID_AI_DIALOGUE);
                coze_main_set_state(COZE_DEMO_STATE_IDLE);
                break;
            }
            default:
                break;
        }
        os_sleep_ms(10);
    }

cleanup:
    coze_main_interrupt();
    // 关闭屏幕
//extern void lcd_bl_pwm(uint32 duty_percent);
//  lcd_bl_pwm(0);
    coze_audio_play_prompt_tone(guanji, guanji_size);
    os_sleep_ms(2000);
    coze_audio_wait_empty();
    txmplayer_close(coze_mgr.audio_url_hdl);
    msi_output_cmd(coze_mgr.audio_msi, MSI_CMD_STOP, 0, 0);
    msi_put(coze_mgr.audio_msi);
    txmplayer_deinit();
    msi_destroy(coze_mgr.coze_msi);
    llm_free(coze_mgr.event_queue.rbq);
    if (coze_mgr.weather_str) { llm_free(coze_mgr.weather_str); }
    llm_sts_deinit(coze_mgr.sts_session);
    llm_global_deinit();
    coze_demo_exit();
}

struct os_task coze_main_task;
void coze_demo(void)
{
    OS_TASK_INIT("COZE_DEMO", &coze_main_task, coze_main, NULL, OS_TASK_PRIORITY_NORMAL, NULL, 2048);
}
#endif
