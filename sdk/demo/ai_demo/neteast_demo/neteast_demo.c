#include "neteast_demo.h"
#include "neteast_material.h"

#if NETEAST_DEMO
/* -------------------------- 宏 -------------------------- */
/* Please add your server information */
#define NETEAST_URL                     "wss://mps.yunxinvcloud.com"
#define NETEAST_LICENSE_URL             "https://nrtc-plugins.yunxinapi.com/v2/license/activate"
#define NETEAST_LICENSE_KEY             "neteast_taixin_1234"   //需要网易商务提供已授权的license_key
#define NETEAST_APP_SECURT              "bf7bad2258ef"
#define NETEAST_APP_KEY                 "e99d9f6bcc877748d6201252301611eb"
#define NETEAST_DEVICE_ID               "taixin"
#define NETEAST_AGENT_ID                "5733cb5ae45a4fd989b5e0ced1f0347b"
#define NETEAST_GET_EXTERNAL_IP_URL     "http://ipinfo.io/ip"


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

static int32 neteast_main_event_cb(void *session, uint16 evt, uint32 param1, uint32 param2);

/* -------------------------- 全局变量 -------------------------- */
/*
 * @brief 模型列表
*/
const struct llm_model models[] = {
    {"neteast_sts", (struct llm_model_data *) &neteast_sts_model},
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
struct llm_sts_sparam neteast_sts_session_cfg = {
    .qmsg_tx_cnt            = 16,       //audio sample msg num
    .rx_buff_size           = 4096,     //audeo recv buff
    .cb_max_size            = 2048,
    .evt_cb                 = neteast_main_event_cb,
};

/*
 * @brief 平台层参数
*/
struct neteast_chat_platform_cfg neteast_sts_platform_cfg = {
    .host_url                 = NETEAST_URL,   
    .license_url              = NETEAST_LICENSE_URL,                           
    .license_key              = NETEAST_LICENSE_KEY,                           
    .app_key                  = NETEAST_APP_KEY,
    .app_secret               = NETEAST_APP_SECURT,  
    .device_id                = NETEAST_DEVICE_ID,  
    .agent_id                 = NETEAST_AGENT_ID,

    .input_audio_format       = "pcm",
    .input_audio_sample_rate  = "16000",
    .input_audio_channel      = "1",
    .input_audio_encoding     = "raw",

	.output_audio_format      = "pcm",
    .output_audio_sample_rate = MACRO_TO_STR(NETEAST_AUDIO_DEC_PCM_SAMPLE_RATE),
    .output_audio_channel 	  = "1",
    .output_audio_encoding    = "raw",
};

/*
 * @brief 传输层参数
*/
struct llm_trans_param neteast_sts_trans_cfg = {
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
struct neteast_demo_manage neteast_mgr = {
    .pwr_en             = 1,
    .key_triggered      = 0,
    .voice_triggered    = 0,
    .finish             = 0,
    .pause              = 0,
    .connected          = 0,
    .send_audio_cnt     = 0,
    .time_stamp         = UINT64_MAX,
    .txmplayer_hdl      = -1,
    .volume             = 50,
};
/* -------------------------- END -------------------------- */

/*
 * @brief 提交手动对话内容：conversation.message.create
 * @param id 对话ID
 * @param content 对话内容
 * @return 提交结果，0表示成功，其他值表示失败
*/
static int32 neteast_main_send_manual_conversation_msg(uint64 id, char *content)
{
    os_printf("%s,%d:Not support yet!\n", __FUNCTION__, __LINE__);
	return RET_OK;
}

/* -------------------------- 主要代码 -------------------------- */
const char *neteast_main_state_str(neteast_demo_state state)
{
    switch (state) {
        case NETEAST_DEMO_STATE_IDLE:
            return "IDLE";
        case NETEAST_DEMO_STATE_CONNECTED:
            return "CONNECTED";
        case NETEAST_DEMO_STATE_DISCONNECTED:
            return "DISCONNECTED";
        case NETEAST_DEMO_STATE_READY:
            return "READY";
        case NETEAST_DEMO_STATE_WAITING:
            return "WAITING";
        case NETEAST_DEMO_STATE_PUSHING:
            return "PUSHING";
        default:
            return "INVALID";
    }
    return "INVALID";
}

/*
 * @brief 获取当前状态
 * @return 当前状态
*/
neteast_demo_state neteast_main_get_state(void)
{
    neteast_demo_state state = NETEAST_DEMO_STATE_IDLE;
    os_mutex_lock(&neteast_mgr.lock, osWaitForever);
    state = neteast_mgr.state;
    os_mutex_unlock(&neteast_mgr.lock);
    return state;
}

/*
 * @brief 设置当前状态
 * @param new_state 新状态
*/
void neteast_main_set_state(neteast_demo_state new_state)
{
    int32 changed = 0;
    if (neteast_mgr.state == new_state) { return; }
    os_mutex_lock(&neteast_mgr.lock, osWaitForever);
    switch (new_state) {
        case NETEAST_DEMO_STATE_IDLE:
            changed = 1;
            break;
        case NETEAST_DEMO_STATE_CONNECTED:
            changed = 1;
            break;
        case NETEAST_DEMO_STATE_DISCONNECTED:
            neteast_mgr.time_stamp = UINT64_MAX;
            changed = 1;
            break;
        case NETEAST_DEMO_STATE_READY:
            if (neteast_mgr.connected) {
                changed = 1;
            }
            break;
        case NETEAST_DEMO_STATE_WAITING:
            neteast_mgr.time_stamp = UINT64_MAX;
            if (neteast_mgr.connected) {
                changed = 1;
            }
            break;
        case NETEAST_DEMO_STATE_PUSHING:
            if (neteast_mgr.connected) {
                changed = 1;
                neteast_mgr.neteast_msi->enable = 1;
            }
            break;
        default:
            break;
    }
    if (changed) {
        os_printf("New State:%s -> %s\r\n", neteast_main_state_str(neteast_mgr.state), neteast_main_state_str(new_state));
        if (neteast_mgr.state == NETEAST_DEMO_STATE_PUSHING) {
            neteast_mgr.neteast_msi->enable = 0;
            msi_clear(neteast_mgr.neteast_msi);
        }
        neteast_mgr.state = new_state;
    } else {
        os_printf("invalid state: %s -> %s\r\n", neteast_main_state_str(neteast_mgr.state), neteast_main_state_str(new_state));
    }
    os_mutex_unlock(&neteast_mgr.lock);
}

/*
 * @brief 超时警告函数
 * @param 无
*/
void neteast_main_timeout_warn(void)
{
    os_printf("**当前网络较差，请稍等!**\n");
    //neteast_audio_play_prompt_tone(wangluocha, wangluocha_size);//opus
    os_sleep_ms(3000);
}

/*
 * @brief 超时重连函数
 * @param 无
*/
void neteast_main_timeout_reset(void)
{
    os_printf("**网络超时，即将重连!**\n");
    //neteast_audio_play_prompt_tone(chaoshi, chaoshi_size);//opus
    os_sleep_ms(3000);
    neteast_main_set_state(NETEAST_DEMO_STATE_IDLE);
    llm_sts_reconnect(neteast_mgr.sts_session);
}

/*
 * @brief 等待进入监听函数
 * @param 无
 * @return 操作结果，0表示成功，其他值表示失败
*/
int32 neteast_main_waiting(void)
{
    int32 ret = RET_OK;
    uint32 timeout = 0;
    do {
        ret = llm_sts_wait(neteast_mgr.sts_session, 1000);
        if (ret == RET_ERR) {
            os_printf("This can be confirmed to be a disconnection!\r\n");
            break;
        } else if (ret == LLME_WAIT_TIMEOUT) {
            timeout++;
            if (timeout == (NETEAST_DEMO_DIALOGUE_WAITING_TIMEOUT / 2)) {
                neteast_main_timeout_warn();
            }
            if (timeout > NETEAST_DEMO_DIALOGUE_WAITING_TIMEOUT) {
                neteast_main_timeout_reset();
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
int32 neteast_main_interrupt(void)
{
    int32 ret = RET_OK;
    uint32 timeout = 0;
    neteast_audio_mute();
    do {
        ret = llm_sts_interrupt(neteast_mgr.sts_session, 1000);
        if (ret == RET_ERR) {
            os_printf("This can be confirmed to be a disconnection!\r\n");
            break;
        } else if (ret == LLME_WAIT_TIMEOUT) {
            timeout++;
            if (timeout == (NETEAST_DEMO_DIALOGUE_INTERRUPT_TIMEOUT / 2)) {
                neteast_audio_wait_empty();
                neteast_audio_restore();
                neteast_main_timeout_warn();
            }
            if (timeout > NETEAST_DEMO_DIALOGUE_INTERRUPT_TIMEOUT) {
                neteast_main_timeout_reset();
                break;
            }
        }
    } while (ret == LLME_WAIT_TIMEOUT);

    neteast_audio_wait_empty();
    neteast_audio_restore();
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
int32 neteast_main_event_cb(void *session, uint16 evt, uint32 param1, uint32 param2)
{
    int ret = RET_OK;
    uint8 process = 0;
    struct neteast_demo_event_msg event_msg = {
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
                os_printf("stt result no memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        }
        case LLM_EVENT_TTS_RESULT: {
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                os_printf("tts result no memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        }
        case LLM_EVENT_VAD_RESULT: {
            event_msg.msg = llm_strdup((const char *)(param1 == NETEAST_SERVER_VAD_START ? "START" : "STOP"));
            if (event_msg.msg == NULL) {
                os_printf("vad result no memory!\r\n");
                break;
            }
            event_msg.msg_len = os_strlen(event_msg.msg);
            process = 1;
            break;
        }
        case LLM_EVENT_ERROR_MSG: {
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                os_printf("err msg no memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        }
        case LLM_EVENT_CUSTOMIZE: {
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                os_printf("customize no memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        }
        case LLM_EVENT_UPLOAD_FILE_RESULT: {
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                os_printf("customize no memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        }
        case LLM_EVENT_TTI_RESULT: {
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                os_printf("tti result memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        }
        case LLM_EVENT_CONVERSATION_ID: {
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                os_printf("conversation memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        }
        case LLM_EVENT_MPLAYER_RESULT: {
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                os_printf("media player no memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        }
        case LLM_EVENT_IOT_RESULT: {
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                os_printf("iot no memory!\r\n");
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
        if (RB_INT_SET(&neteast_mgr.event_queue, event_msg) == 0) {
            os_printf("event_queue is full!\r\n");
        }
    }
    return ret;
}

/*
 * @brief 系统无线事件回调函数
 * @param event_id 事件ID
 * @param data 事件数据
 * @param priv 私有数据
 * @return 操作结果，0表示成功，其他值表示失败
*/
sysevt_hdl_res neteast_main_wifi_event(uint32 event_id, uint32 data, uint32 priv)
{
    switch (event_id & 0xffff) {
        case SYSEVT_WIFI_DISCONNECT:
            os_printf("**网络异常!**\r\n", event_id);
            break;
        case SYSEVT_WIFI_CONNECTTED:
            neteast_main_set_state(NETEAST_DEMO_STATE_IDLE);
            llm_sts_reconnect(neteast_mgr.sts_session);
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
sysevt_hdl_res neteast_main_network_event(uint32 event_id, uint32 data, uint32 priv)
{
    struct netif *nif;

    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_NETWORK, SYSEVT_LWIP_DHCPC_DONE):
            nif = netif_find("w0");
            gethostbyname_async("mps.yunxinvcloud.com");
            if (neteast_mgr.ip_addr != nif->ip_addr.addr) {
                neteast_mgr.ip_addr = nif->ip_addr.addr;
                neteast_main_set_state(NETEAST_DEMO_STATE_IDLE);
                llm_sts_reconnect(neteast_mgr.sts_session);
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
sysevt_hdl_res neteast_main_asr_event(uint32 event_id, uint32 data, uint32 priv)
{
    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_ASR, SYSEVT_ASR_WAKEUP):
            neteast_mgr.voice_triggered = 1;
            break;
    }
    return SYSEVT_CONTINUE;
}

static void neteast_main_conversation_id_init()
{
    os_printf("%s,%d:not supported yet!\n", __FUNCTION__, __LINE__);
}

/*
 * @brief 应用初始化函数
 * @param llm_name LLM名称
 * @return 操作结果，0表示成功，其他值表示失败
*/
struct os_task neteast_awaken_task;
struct os_task neteast_pwr_detect_task;
struct os_task neteast_audio_play_task;
int32 neteast_main_app_init(char *llm_name)
{
    int ret = 0;
    struct txmplayer_param param = {
        .volume = neteast_mgr.volume,
    };
    struct neteast_demo_event_msg *event_queue_buf = NULL;
    llm_global_init(&global);

    neteast_mgr.sts_session = llm_sts_init(llm_name, &neteast_sts_session_cfg);
    if (!neteast_mgr.sts_session) {
        os_printf("sts session init fail!\r\n");
        return RET_ERR;
    }

    ret = llm_sts_config(neteast_mgr.sts_session, LLM_CONFIG_TYPE_TRANS,
                         (void *)&neteast_sts_trans_cfg, sizeof(neteast_sts_trans_cfg));
    if (ret) {
        os_printf("sts_trans_cfg fail!\r\n");
        return RET_ERR;
    }

    //neteast_sts_platform_cfg.turn_detection_type = "client_interrupt";
    ret = llm_sts_config(neteast_mgr.sts_session, LLM_CONFIG_TYPE_MODEL,
                         (void *)&neteast_sts_platform_cfg, sizeof(neteast_sts_platform_cfg));
    if (ret) {
        os_printf("sts_trans_cfg fail!\r\n");
        return RET_ERR;
    }
    //neteast_sts_platform_cfg.chat_config_parameters = neteast_mgr.chat_config_parameter;

    os_mutex_init(&neteast_mgr.lock);

    event_queue_buf = llm_malloc((NETEAST_DEMO_EVENT_QUEUE_CNT + 1) * sizeof(struct neteast_demo_event_msg));
    if (!event_queue_buf) {
        os_printf("event_queue_buf malloc fail, no memory!\r\n");
        return RET_ERR;
    }
    RB_INIT_R(&neteast_mgr.event_queue, NETEAST_DEMO_EVENT_QUEUE_CNT, event_queue_buf);

    neteast_mgr.neteast_msi = msi_new("neteast_msi", 8, NULL);//确保比adc的fb队列小，自己丢数据，不要影响其他的模块
    if (!neteast_mgr.neteast_msi) {
        os_printf("neteast_msi err!\r\n");
        return RET_ERR;
    }
    neteast_mgr.neteast_msi->fb_limits.counter = 32;
    neteast_mgr.neteast_msi->enable = 0;

    txmplayer_init(0, 0, &param);
    neteast_mgr.txmplayer_msi = msi_find2("txmplayer", 0, 0, NULL);
    msi_add_output(neteast_mgr.neteast_msi, NULL, neteast_mgr.txmplayer_msi, NULL);

    auadc_msi_add_output(AUSYS_AUAD, neteast_mgr.neteast_msi->name);
    msi_add_output(neteast_mgr.neteast_msi, NULL, NULL, S_NET_JPEG);

#if LLM_SPV12XX
    OS_TASK_INIT("NETEAST_AWAKEN", &neteast_awaken_task, neteast_awaken_detect_thread, NULL, OS_TASK_PRIORITY_NORMAL + 1, NULL, 1024);
#endif

#if LLM_PWR_DETECT
    OS_TASK_INIT("NETEAST_PWR", &neteast_pwr_detect_task, neteast_pwr_detect_thread, NULL, OS_TASK_PRIORITY_ABOVE_NORMAL, NULL, 1024);
#endif

    OS_TASK_INIT("NETEAST_AUPLAY", &neteast_audio_play_task, neteast_audio_play_thread, NULL, OS_TASK_PRIORITY_NORMAL, NULL, 1024);
    neteast_main_conversation_id_init();

#if LLM_TAIXIN_LOGO
    struct framebuff *fb = msi_alloc_fb(neteast_mgr.neteast_msi, NULL, NULL, logo_size, 0, 0);
    if (fb) {
        fb->mtype = F_JPG;
        fb->stype = FSTYPE_JPG_FILE;
        fb->datatag = ++neteast_mgr.jpg_data_tag;
        hw_memcpy(fb->data, logo, logo_size);
        sys_dcache_clean_invalid_range_unaligned((uint32_t *)fb->data, logo_size);
        msi_output_fb(neteast_mgr.neteast_msi, fb, 0);
    } else {
        os_printf("fb alloc fail!\r\n");
    }
#endif

    //neteast_audio_play_prompt_tone(kaiji, kaiji_size);
    os_sleep_ms(2000);

    os_printf("Waiting for network connection...");
    while (!sys_status.wifi_connected || !sys_status.dhcpc_done) {
        if (neteast_mgr.pwr_en == 0) {
            return RET_ERR;
        }
        _os_printf(".");
        os_sleep(1);
    }
    os_printf("Network connected!\r\n");
    add_keycallback(neteast_awaken_intercom_push_key, NULL);

    struct netif *nif = netif_find("w0");
    neteast_mgr.ip_addr = nif->ip_addr.addr;

    sys_event_take(SYS_EVENT(SYS_EVENT_WIFI, 0), neteast_main_wifi_event, 0);
    sys_event_take(SYS_EVENT(SYS_EVENT_NETWORK, 0), neteast_main_network_event, 0);
    sys_event_take(SYS_EVENT(SYS_EVENT_ASR, 0), neteast_main_asr_event, 0);

    neteast_main_set_state(NETEAST_DEMO_STATE_IDLE);
    llm_sts_reconnect(neteast_mgr.sts_session);
    return ret;
}

/*
 * @brief 主线程
*/
void neteast_main(void)
{
    int32 ret = RET_OK;
    neteast_demo_state state = NETEAST_DEMO_STATE_IDLE;
    uint64 jiff = 0;
    uint8 last_key_triggered = 0;   // 记录上一次按键状态；
    uint8 dialogue_mode = 0;        // 对话模式，0为按键模式，1为语音模式；
    uint8 send_complete = 0;        // 按键模式下，需发送完成帧告知服务器音频数据上传结束；
    uint8 txmplayer_busy = 0;

    ret = neteast_main_app_init("neteast_sts");
    if (ret) {
        os_printf("neteast main init failed: %d\r\n", ret);
        goto cleanup;
    }

    while (neteast_mgr.pwr_en) {
        neteast_msg_event_process();

        if (neteast_mgr.connected == 1) {
            if (neteast_mgr.img_triggered) {
                neteast_mgr.img_triggered = 0;
                if (dialogue_mode != DIALOGUE_MODE_IMAGE_UPDATED) {
                    dialogue_mode = DIALOGUE_MODE_IMAGE_UPDATED;
                }
                neteast_main_set_state(NETEAST_DEMO_STATE_WAITING);
            }
            // 处理语音触发事件
            if (neteast_mgr.voice_triggered) {
                neteast_mgr.voice_triggered = 0;
                if (dialogue_mode != DIALOGUE_MODE_VOICE) {
                    //neteast_sts_platform_cfg.turn_detection_type = "server_vad";
                    llm_sts_config(neteast_mgr.sts_session, LLM_CONFIG_TYPE_MODEL,
                                   (void *)&neteast_sts_platform_cfg, sizeof(neteast_sts_platform_cfg));
                    dialogue_mode = DIALOGUE_MODE_VOICE;
                }
                neteast_main_set_state(NETEAST_DEMO_STATE_WAITING);
            }
            // 处理按键触发事件，优先级：按键 > 语音
            if (last_key_triggered != neteast_mgr.key_triggered) {
                // 按键按下
                if (neteast_mgr.key_triggered == 1) {
                    if (dialogue_mode != DIALOGUE_MODE_KEYBOARD) {
                        // 按键配置与语音配置不同（turn_detection_type），需更新
                        //neteast_sts_platform_cfg.turn_detection_type = "client_interrupt";
                        llm_sts_config(neteast_mgr.sts_session, LLM_CONFIG_TYPE_MODEL,
                                       (void *)&neteast_sts_platform_cfg, sizeof(neteast_sts_platform_cfg));
                        dialogue_mode = DIALOGUE_MODE_KEYBOARD;
                    }
                    neteast_main_set_state(NETEAST_DEMO_STATE_WAITING);
                }
                // 按键松开
                else {
                    // 若发送过音频数据需发送完成帧
                    if (neteast_mgr.send_audio_cnt > 0) {
                        send_complete = 1;
                    }
                    neteast_main_set_state(NETEAST_DEMO_STATE_READY);
                }
                last_key_triggered = neteast_mgr.key_triggered;
            }
            // 处理语音触发模式下自动退出多轮对话
            if (dialogue_mode == DIALOGUE_MODE_VOICE) {
                if (neteast_mgr.time_stamp != UINT64_MAX) {
                    jiff = os_jiffies() - neteast_mgr.time_stamp;
                    if (os_jiffies_to_msecs(jiff) > (NETEAST_DEMO_DIALOGUE_IDLE_TIMEOUT * 1000) && llm_sts_check_audio(neteast_mgr.sts_session)) {
                        if (neteast_mgr.pause == 0 &&
                            neteast_mgr.txmplayer_hdl >= 0 &&
                            txmplayer_state(neteast_mgr.txmplayer_hdl) == TXMPLAYER_STATE_PAUSE) {
                            os_printf("**继续播放**\r\n");
                            neteast_mgr.time_stamp = os_jiffies();
                            txmplayer_pause(neteast_mgr.txmplayer_hdl, 0);
                        } else {
                            ret = neteast_main_interrupt();
                            if (ret != RET_OK) {
                                break;
                            }
                            os_printf("**自动退出**\r\n");
                            //neteast_audio_play_prompt_tone(tuixia, tuixia_size);
                            neteast_mgr.time_stamp = UINT64_MAX;
                        }
                        neteast_mgr.finish = 0;
                        neteast_main_set_state(NETEAST_DEMO_STATE_READY);
                    }
                }
            }
        }
        state = neteast_main_get_state();
        switch (state) {
            case NETEAST_DEMO_STATE_IDLE:
                break;
            case NETEAST_DEMO_STATE_CONNECTED: {
                os_printf("**已连接!**\n");
                neteast_mgr.connected = 1;
                //neteast_audio_play_prompt_tone(yilianjie, yilianjie_size);
                os_sleep_ms(1000);
                neteast_mgr.key_triggered = 0;
                neteast_mgr.voice_triggered = 0;
                neteast_mgr.img_triggered = 0;
                // 连接成功后，需更新一次配置
                if (dialogue_mode == DIALOGUE_MODE_VOICE) {
                    //neteast_sts_platform_cfg.turn_detection_type = "server_vad";
                } else if (dialogue_mode == DIALOGUE_MODE_KEYBOARD) {
                    //neteast_sts_platform_cfg.turn_detection_type = "client_interrupt";
                }
                llm_sts_config(neteast_mgr.sts_session, LLM_CONFIG_TYPE_MODEL,
                               (void *)&neteast_sts_platform_cfg, sizeof(neteast_sts_platform_cfg));
                neteast_main_set_state(NETEAST_DEMO_STATE_READY);
                break;
            }
            case NETEAST_DEMO_STATE_READY: {
                if (neteast_mgr.txmplayer_hdl >= 0) {
                    if (txmplayer_state(neteast_mgr.txmplayer_hdl) == TXMPLAYER_STATE_PLAYING) {
                        neteast_mgr.finish = 0;
                        neteast_mgr.time_stamp = os_jiffies();
                        txmplayer_busy = 0;
                    } else if (txmplayer_state(neteast_mgr.txmplayer_hdl) == TXMPLAYER_STATE_PLAY_END) {
                        txmplayer_close(neteast_mgr.txmplayer_hdl);
                        neteast_mgr.txmplayer_hdl = -1;
                    } else if (txmplayer_state(neteast_mgr.txmplayer_hdl) == TXMPLAYER_STATE_BUFFERING && txmplayer_busy == 0) {
                        neteast_main_timeout_warn();
                        txmplayer_busy = 1;
                    } else if (txmplayer_state(neteast_mgr.txmplayer_hdl) == TXMPLAYER_STATE_OPEN_FAIL) {
                        os_printf("*媒体播放失败!**\n");
                        txmplayer_close(neteast_mgr.txmplayer_hdl);
                        neteast_mgr.txmplayer_hdl = -1;
                    }
                }
                // 按键触发模式
                if (dialogue_mode == DIALOGUE_MODE_KEYBOARD) {
                    if (send_complete) {
                        ret = llm_sts_send(neteast_mgr.sts_session, LLM_DATA_TYPE_AUDIO, LLM_DATA_STATE_END, NULL, 0);
                        if (ret == RET_OK) {
                            send_complete = 0;
                            //neteast_audio_play_prompt_tone(fasong, fasong_size);
                        } else if (ret != LLME_AGAIN) {
                            os_printf("send_complete fail!\r\n");
                            neteast_main_set_state(NETEAST_DEMO_STATE_IDLE);
                            llm_sts_reconnect(neteast_mgr.sts_session);
                            break;
                        }
                    }
                }
                // 语音触发模式
                else if (dialogue_mode == DIALOGUE_MODE_VOICE) {
                    // 自动触发多轮对话
                    if (neteast_mgr.finish && llm_sts_check_audio(neteast_mgr.sts_session) && ausys_da_fifo_is_empty()) {
                        neteast_main_set_state(NETEAST_DEMO_STATE_WAITING);
                        os_printf("**新对话：**\r\n");
                    }
                }
                if(os_jiffies() - neteast_mgr.keep_alive > 30000) {
                    ret = llm_sts_send(neteast_mgr.sts_session, LLM_DATA_TYPE_PING, LLM_DATA_STATE_START,
                       "keepalive", 10);                    
                    os_printf("%s,%d:Send ping to keepalive...\n",__FUNCTION__,__LINE__);
                    neteast_mgr.keep_alive = os_jiffies();
                }
                break;
                break;
            }
            case NETEAST_DEMO_STATE_WAITING: {
                // 打断上一次对话
                ret = neteast_main_interrupt();
                if (ret != RET_OK) {
                    break;
                }
                // 等待服务器进入监听状态
                ret = neteast_main_waiting();
                if (ret != RET_OK) {
                    break;
                }
                if (dialogue_mode == DIALOGUE_MODE_VOICE) {
                    if (neteast_mgr.finish == 0) {
                        os_printf("**我在，请说：**\r\n");
                        //neteast_audio_play_prompt_tone(wozai, wozai_size);
                        os_sleep_ms(1000);
                    }
                    neteast_mgr.finish = 0;
                    neteast_mgr.time_stamp = os_jiffies();
                } else if (dialogue_mode == DIALOGUE_MODE_IMAGE_UPDATED) {
                    neteast_main_send_manual_conversation_msg(os_jiffies(), "这张图片里有什么");
                    neteast_main_set_state(NETEAST_DEMO_STATE_READY);
                }
                if (dialogue_mode != DIALOGUE_MODE_IMAGE_UPDATED) {
                    //neteast_audio_play_prompt_tone(beep, beep_size);
                    neteast_main_set_state(NETEAST_DEMO_STATE_PUSHING);
                    neteast_mgr.send_audio_cnt = 0;
                }
                break;
            }
            case NETEAST_DEMO_STATE_PUSHING: {
                // 发送音频数据
                neteast_audio_send_data();
                break;
            }
            case NETEAST_DEMO_STATE_DISCONNECTED: {
                os_printf("**已断开!**\n");
                neteast_mgr.connected = 0;
                //neteast_audio_play_prompt_tone(yiduankai, yiduankai_size);
                os_sleep_ms(1000);
                neteast_mgr.finish = 0;
                neteast_mgr.key_triggered = 0;
                last_key_triggered = 0;
                send_complete = 0;
                neteast_mgr.voice_triggered = 0;
                neteast_mgr.img_triggered   = 0;
                neteast_main_set_state(NETEAST_DEMO_STATE_IDLE);
                break;
            }
            default:
                break;
        }
        os_sleep_ms(10);
    }

cleanup:
    neteast_audio_wait_empty();
    // 关闭屏幕
//extern void lcd_bl_pwm(uint32 duty_percent);//opus
//  lcd_bl_pwm(0);
    //neteast_audio_play_prompt_tone(guanji, guanji_size);
    os_sleep_ms(2000);
    txmplayer_close(neteast_mgr.txmplayer_hdl);
    msi_output_cmd(neteast_mgr.txmplayer_msi, MSI_CMD_STOP, 0, 0);
    msi_put(neteast_mgr.txmplayer_msi);
    txmplayer_deinit();
    msi_destroy(neteast_mgr.neteast_msi);
    llm_free(neteast_mgr.event_queue.rbq);
    llm_sts_deinit(neteast_mgr.sts_session);
    llm_global_deinit();
#if LLM_PWR_DETECT
    gpio_set_val(LLM_SYS_PWR_EN, 0);
#endif
}

struct os_task neteast_main_task;
void neteast_demo(void)
{
    OS_TASK_INIT("NETEAST_DEMO", &neteast_main_task, neteast_main, NULL, OS_TASK_PRIORITY_NORMAL, NULL, 2048);
}
#endif
