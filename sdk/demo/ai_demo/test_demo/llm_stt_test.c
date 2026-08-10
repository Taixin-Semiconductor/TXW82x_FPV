#include "llm/llm_api.h"

#include "syscfg.h"
#include "keyWork.h"
#include "keyScan.h"
#include "lib/multimedia/msi.h"
#include "audio_adc.h"
#include "lwip/netdb.h"

#if LLM_TEST == 1
/* Please add your server information */
#define STT_URL         "wss://openspeech.bytedance.com/api/v2/asr"
#define STT_APPID       "xxx"
#define STT_TOKEN       "xxx"
#define STT_CLUSTER     "xxx"

#define LLM_STT_TEST_EVENT_QUEUE_CNT    16

struct llm_stt_test_event_msg {
    uint16  event;
    char    *msg;
    uint32  msg_len;
};

struct llm_stt_test_manage {
    void    *stt_session;
    struct  msi *stt_msi;
    uint32  key_triggered:1,        // 按键触发
            auadc_model_start: 1,   // 音频采集
            rev: 29;
    uint32  send_audio_cnt;         // 已发送音频包数量
    RBUFFER_DEF_R(event_queue, struct llm_stt_test_event_msg);
} stt_mgr = {
    .key_triggered      = 0,
    .auadc_model_start  = 0,
    .send_audio_cnt     = 0
};

extern void *llm_mem_alloc(uint32 size);
extern void llm_mem_free(void *ptr);

void *llm_malloc(uint32 size) {return llm_mem_alloc(size);}
void  llm_free(void *ptr) {llm_mem_free(ptr);}
void *llm_zalloc(size_t size) {
    void *ptr = llm_malloc(size);
    if (ptr) {
        os_memset(ptr, 0, size);
    }
    return ptr;
}
void *llm_calloc(size_t nmemb, size_t size) {return llm_zalloc(nmemb * size);}
void *llm_realloc(void *ptr, size_t size) {
    void *nptr = llm_malloc(size);
    if (nptr) {
        if(ptr){
            os_memcpy(nptr, ptr, size);
            llm_free(ptr);
        }
    }
    return nptr;
}
char *llm_strdup(const char *s) {
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

int32 llm_stt_test_event_cb(void *session, uint16 evt, uint32 param1, uint32 param2)
{
    uint8 process = 0;
    struct llm_stt_test_event_msg event_msg = {
        .event      = evt,
        .msg        = NULL,
        .msg_len    = 0,
    };

    switch (evt) {
        case LLM_EVENT_ERROR_MSG:
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                os_printf("stt err msg no memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        case LLM_EVENT_STT_RESULT:
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                os_printf("stt result no memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        case LLM_EVENT_CONN_ERR:
        case LLM_EVENT_TX_ERR:
        case LLM_EVENT_RX_ERR:
            process = 1;
        default:
            break;
    }
    if (process) {
        RB_INT_SET(&stt_mgr.event_queue, event_msg);
    }
    return RET_OK;
}

////////////////////////////////////////////
const struct llm_model models[] = {
    {"doubao_stt",  &doubao_stt}
};

struct llm_global_param global = {
    .task_reuse     = 0,
    .model_count    = 1,
    .models         = models,
};

//STT
struct llm_stt_sparam stt_session_cfg = {
    .qmsg_tx_cnt            = 32,
    .cb_max_size            = 2048,
    .evt_cb                 = llm_stt_test_event_cb,
};
struct Doubao_STT_Cfg asr_stt_cfg = {
    .url = STT_URL,
    .app_appid = STT_APPID,
    .app_token = STT_TOKEN,
    .app_cluster = STT_CLUSTER,
    .user_uid = "41494922",
    .audio_format = "raw",
    .audio_codec = "raw",
    .audio_rate = "16000",
    .audio_bits = "16",
    .audio_channel = "1",
    .boosting_table_name = NULL,
    .correct_table_name = NULL
};
struct llm_trans_param stt_trans_cfg = {
    .buffersize         = 2048,
    .upload_buffersize  = 2048,
    .low_speed_limit    = 10,
    .low_speed_time     = 5,
    .connect_timeout    = 5,
    .blob_len           = 0,
    .blob_data          = NULL
};
    
uint32_t llm_intercom_push_key(struct key_callback_list_s *callback_list, uint32_t keyvalue, uint32_t extern_value)
{
    static uint8 key_state = 0;
    if ((keyvalue >> 8) != AD_PRESS)
    { return 0; }
    uint32 key_val = (keyvalue & 0xff);
    if ((key_val == KEY_EVENT_LDOWN) || (key_val == KEY_EVENT_REPEAT)) {
        if (key_state == 0) {
            os_printf("key start\r\n");
            key_state = 1;
            stt_mgr.key_triggered = 1;
        }
    } else if ((key_val == KEY_EVENT_LUP)) {
        if (key_state == 1) {
            os_printf("key stop\r\n");
            key_state = 0;
            stt_mgr.key_triggered = 0;
        }
    }
    return 0;
}

sysevt_hdl_res llm_stt_test_network_event(uint32 event_id, uint32 data, uint32 priv)
{
    struct netif *nif;

    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_NETWORK, SYSEVT_LWIP_DHCPC_DONE):
            nif = netif_find("w0");
            gethostbyname_async("openspeech.bytedance.com");
            break;
    }
    return SYSEVT_CONTINUE;
}

static int32 llm_stt_test_event_msg_process(void)
{
    struct llm_stt_test_event_msg event_msg = {0};

    RB_INT_GET(&stt_mgr.event_queue, event_msg);
    if (event_msg.event == LLM_EVENT_UNKNOWN) { return RET_OK; }

    switch (event_msg.event) {
        case LLM_EVENT_STT_RESULT: {
            os_printf("STT(%d):", event_msg.msg_len);
            hgprintf_out(event_msg.msg, event_msg.msg_len, 0);
            _os_printf("\r\n");
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_ERROR_MSG: {
            os_printf("recv STT err msg(%d):", event_msg.msg_len);
            hgprintf_out(event_msg.msg, event_msg.msg_len, 0);
            _os_printf("\r\n");
            llm_free(event_msg.msg);
            // TODO
            break;
        }
        case LLM_EVENT_CONN_ERR:
        case LLM_EVENT_TX_ERR:
        case LLM_EVENT_RX_ERR: {
            os_printf("STT %s err!\r\n", event_msg.event == LLM_EVENT_CONN_ERR ? "CONN" : (event_msg.event == LLM_EVENT_TX_ERR ? "TX" : "RX"));
            //llm_stt_interrupt(stt_mgr.stt_session, 0);
            stt_mgr.auadc_model_start = 0;
            break;
        }
        default: {
            os_printf("Not supported event msg: %d\r\n", event_msg.event);
            break;
        }
    }
    return RET_OK;
}

int32 llm_stt_test_app_init(char *llm_name)
{
    int32 ret = RET_OK;
    struct llm_stt_test_event_msg *event_queue_buf = NULL;
    llm_global_init(&global);

    stt_mgr.stt_session = llm_stt_init(llm_name, &stt_session_cfg);
    if (!stt_mgr.stt_session) {
        os_printf("stt session init fail!\r\n");
        return RET_ERR;
    }
    ret = llm_stt_config(stt_mgr.stt_session, LLM_CONFIG_TYPE_TRANS,
                            (void *)&stt_trans_cfg, sizeof(stt_trans_cfg));
    if (ret) {
        os_printf("stt_trans_cfg fail!\r\n");
        return RET_ERR;
    }
    ret = llm_stt_config(stt_mgr.stt_session, LLM_CONFIG_TYPE_MODEL,
                            (void *)&asr_stt_cfg, sizeof(asr_stt_cfg));
    if (ret) {
        os_printf("asr_stt_cfg fail!\r\n");
        return RET_ERR;
    }
    
    sys_event_take(SYS_EVENT(SYS_EVENT_NETWORK, 0), llm_stt_test_network_event, 0);

    add_keycallback(llm_intercom_push_key, NULL);

    stt_mgr.stt_msi = msi_new("stt_msi", 10, NULL);
    if (!stt_mgr.stt_msi) {
        os_printf("stt_msi err!\r\n");
        return RET_ERR;
    }

    auadc_msi_add_output(AUSYS_AUAD, stt_mgr.stt_msi->name);
    stt_mgr.stt_msi->enable = 1;

    event_queue_buf = llm_malloc((LLM_STT_TEST_EVENT_QUEUE_CNT + 1) * sizeof(struct llm_stt_test_event_msg));
    if (!event_queue_buf) {
        os_printf("event_queue_buf malloc fail, no memory!\r\n");
        return RET_ERR;
    }
    RB_INIT_R(&stt_mgr.event_queue, LLM_STT_TEST_EVENT_QUEUE_CNT, event_queue_buf);

    os_printf("Waiting for network connection...");
    while (!sys_status.wifi_connected || !sys_status.dhcpc_done) {
        _os_printf(".");
        os_sleep(1);
    }
    os_printf("Network connected!\r\n");

    return ret;
}

void llm_stt_test_main(void)
{
    int32 ret = RET_OK;
    uint8 last_key_triggered = 0;   // 记录上一次按键状态；
    struct framebuff *fb = NULL;
    uint8 last_sta = 0;

    ret = llm_stt_test_app_init("doubao_stt");
    if (ret) {
        os_printf("stt main init failed: %d\r\n", ret);
        goto cleanup;
    }

    while (1) {
        llm_stt_test_event_msg_process();
        if (last_key_triggered != stt_mgr.key_triggered) {
            // 按键按下
            if (stt_mgr.key_triggered == 1) {
                llm_stt_send(stt_mgr.stt_session, LLM_DATA_TYPE_AUDIO, LLM_DATA_STATE_START, NULL, 0);
                os_printf("send start audio data!\r\n");
                stt_mgr.auadc_model_start = 1;
                os_printf("mic start\r\n");
            }
            // 按键松开
            else {
                stt_mgr.auadc_model_start = 0;
                os_printf("mic end\r\n");
            }
            last_key_triggered = stt_mgr.key_triggered;
        }
        fb = msi_get_fb(stt_mgr.stt_msi,0);
        if (fb) {
            if (fb->mtype == MEDIA_DATA_AUDIO) {
                if (stt_mgr.auadc_model_start) {
                    llm_stt_send(stt_mgr.stt_session, LLM_DATA_TYPE_AUDIO, LLM_DATA_STATE_MIDDLE, (char *)fb->data, fb->len);
                    os_printf("send audio data(%d)!\r\n", fb->len);
                } else if ((last_sta) && (!stt_mgr.auadc_model_start)) {
                    llm_stt_send(stt_mgr.stt_session, LLM_DATA_TYPE_AUDIO, LLM_DATA_STATE_END, (char *)fb->data, fb->len);
                    os_printf("send end audio data(%d)!\r\n", fb->len);
                }
                last_sta = stt_mgr.auadc_model_start;
            }
            msi_delete_fb(NULL,fb);
            fb = NULL;
        }
        os_sleep_ms(10);
    }

cleanup:
    msi_destroy(stt_mgr.stt_msi);
    llm_stt_deinit(stt_mgr.stt_session);
    llm_global_deinit();
}

struct os_task llm_stt_test_task;
void llm_stt_test(void)
{
    OS_TASK_INIT("LLM_STT_TEST", &llm_stt_test_task, llm_stt_test_main, NULL, OS_TASK_PRIORITY_NORMAL, NULL, 2048);
}
#endif

