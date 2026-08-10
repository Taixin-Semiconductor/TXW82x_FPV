#include "llm/llm_api.h"

#include "syscfg.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/txmplayer.h"
#include "lwip/netdb.h"

#if LLM_TEST == 2
/* Please add your server information */
#define TTS_URL         "wss://openspeech.bytedance.com/api/v1/tts/ws_binary"
#define TTS_APPID       "xxx"
#define TTS_TOKEN       "xxx"
#define TTS_CLUSTER     "xxx"

#define LLM_TTS_TEST_EVENT_QUEUE_CNT    16

struct llm_tts_test_event_msg {
    uint16  event;
    char    *msg;
    uint32  msg_len;
};

struct llm_tts_test_manage {
    void *tts_session;
    struct msi *txmplayer_msi;
    struct msi *tts_msi;
    RBUFFER_DEF_R(event_queue, struct llm_tts_test_event_msg);
} tts_mgr;

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

int32 llm_tts_test_event_cb(void *session, uint16 evt, uint32 param1, uint32 param2)
{
    uint8 process = 0;
    struct llm_tts_test_event_msg event_msg = {
        .event      = evt,
        .msg        = NULL,
        .msg_len    = 0,
    };

    switch (evt) {
        case LLM_EVENT_ERROR_MSG:
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                os_printf("tts err msg no memory!\r\n");
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
        RB_INT_SET(&tts_mgr.event_queue, event_msg);
    }
    return RET_OK;
}

////////////////////////////////////////////
const struct llm_model models[] = {
    {"doubao_tts",  &doubao_tts}
};

struct llm_global_param global = {
    .task_reuse     = 0,
    .model_count    = 1,
    .models         = models,
};

//TTS
struct llm_tts_sparam tts_session_cfg = {
    .qmsg_tx_cnt            = 16,
    .cb_max_size            = 2048,
    .rx_buff_size           = 4096,
    .evt_cb                 = llm_tts_test_event_cb,
};
struct Doubao_TTS_Cfg asr_tts_cfg = {
    .url                    = TTS_URL,
    .app_appid              = TTS_APPID,
    .app_token              = TTS_TOKEN,
    .app_cluster            = TTS_CLUSTER,
    .user_uid               = "41494922",
    .audio_voice_type       = "BV700_streaming",
    .audio_rate             = "8000",
    .audio_compression_rate = NULL,
    .audio_encoding         = "mp3",
    .audio_speed_ratio      = "1.0",
    .audio_volume_ratio     = "0.5",
    .audio_pitch_ratio      = "1.0",
    .audio_emotion          = "comfort",
    .audio_language         = "cn",
    .silence_duration       = NULL,
};
struct llm_trans_param tts_trans_cfg = {
    .buffersize         = 2048,
    .upload_buffersize  = 2048,
    .low_speed_limit    = 10,
    .low_speed_time     = 5,
    .connect_timeout    = 5,
    .blob_len           = 0,
    .blob_data          = NULL
};
txAudioInfo_t tts_audio_info = {
    .codec_id = AUDIO_CODEC_MP3,
    .channels = 1,
};

sysevt_hdl_res llm_tts_test_network_event(uint32 event_id, uint32 data, uint32 priv)
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

static int32 llm_tts_test_event_msg_process(void)
{
    struct llm_tts_test_event_msg event_msg = {0};

    RB_INT_GET(&tts_mgr.event_queue, event_msg);
    if (event_msg.event == LLM_EVENT_UNKNOWN) { return RET_OK; }

    switch (event_msg.event) {
        case LLM_EVENT_ERROR_MSG: {
            os_printf("recv TTS err msg(%d):", event_msg.msg_len);
            hgprintf_out(event_msg.msg, event_msg.msg_len, 0);
            _os_printf("\r\n");
            llm_free(event_msg.msg);
            // TODO
            break;
        }
        case LLM_EVENT_CONN_ERR:
        case LLM_EVENT_TX_ERR:
        case LLM_EVENT_RX_ERR: {
            os_printf("TTS %s err!\r\n", event_msg.event == LLM_EVENT_CONN_ERR ? "CONN" : (event_msg.event == LLM_EVENT_TX_ERR ? "TX" : "RX"));
            //llm_tts_interrupt(tts_mgr.tts_session, 0);
            break;
        }
        default: {
            os_printf("Not supported event msg: %d\r\n", event_msg.event);
            break;
        }
    }
    return RET_OK;
}

int32 llm_tts_test_app_init(char *llm_name)
{
    int32 ret = RET_OK;
    struct txmplayer_param param = {
        .volume = 100,
    };
    struct llm_tts_test_event_msg *event_queue_buf = NULL;
    llm_global_init(&global);

    tts_mgr.tts_session = llm_tts_init(llm_name, &tts_session_cfg);
    if (!tts_mgr.tts_session) {
        os_printf("tts session init fail!\r\n");
        return RET_ERR;
    }
    ret = llm_tts_config(tts_mgr.tts_session, LLM_CONFIG_TYPE_TRANS,
                            (void *)&tts_trans_cfg, sizeof(tts_trans_cfg));
    if (ret) {
        os_printf("tts_trans_cfg fail!\r\n");
        return RET_ERR;
    }
    ret = llm_tts_config(tts_mgr.tts_session, LLM_CONFIG_TYPE_MODEL,
                            (void *)&asr_tts_cfg, sizeof(asr_tts_cfg));
    if (ret) {
        os_printf("llm_tts_cfg fail!\r\n");
        return RET_ERR;
    }
    
    sys_event_take(SYS_EVENT(SYS_EVENT_NETWORK, 0), llm_tts_test_network_event, 0);

    tts_mgr.tts_msi = msi_new("tts_msi", 0, NULL);
    if (!tts_mgr.tts_msi) {
        os_printf("tts_msi err!\r\n");
        return RET_ERR;
    }
    tts_mgr.tts_msi->fb_limits.counter = 32;
    tts_mgr.tts_msi->enable = 1;
    
    txmplayer_init(0, 0, &param);
    tts_mgr.txmplayer_msi = msi_find2("txmplayer", 0, 0, NULL);
    msi_add_output(tts_mgr.tts_msi, NULL, tts_mgr.txmplayer_msi, NULL);

    event_queue_buf = llm_malloc((LLM_TTS_TEST_EVENT_QUEUE_CNT + 1) * sizeof(struct llm_tts_test_event_msg));
    if (!event_queue_buf) {
        os_printf("event_queue_buf malloc fail, no memory!\r\n");
        return RET_ERR;
    }
    RB_INIT_R(&tts_mgr.event_queue, LLM_TTS_TEST_EVENT_QUEUE_CNT, event_queue_buf);

    os_printf("Waiting for network connection...");
    while (!sys_status.wifi_connected || !sys_status.dhcpc_done) {
        _os_printf(".");
        os_sleep(1);
    }
    os_printf("Network connected!\r\n");

    return ret;
}

void *tts_fp = NULL;
void llm_tts_test_main(void)
{
    int32 ret = RET_OK;
    struct framebuff *fb = NULL;
    char *audio_buf = NULL;
    uint32 audio_len = 0;

    audio_buf = llm_malloc(2048);
    if (!audio_buf) {
        os_printf("audio_buf alloc fail!\r\n");
        goto cleanup;
    }

    ret = llm_tts_test_app_init("doubao_tts");
    if (ret) {
        os_printf("tts main init failed: %d\r\n", ret);
        goto cleanup;
    }

    os_printf("tts main init start\r\n");
    do {
        ret = llm_tts_send(tts_mgr.tts_session, LLM_DATA_TYPE_TEXT, LLM_DATA_STATE_START, NULL, 0);
        ret = llm_tts_send(tts_mgr.tts_session, LLM_DATA_TYPE_TEXT, LLM_DATA_STATE_END, "珠海泰芯半导体", os_strlen("珠海泰芯半导体"));
    } while (ret == LLME_AGAIN);

    do {
        ret = llm_tts_send(tts_mgr.tts_session, LLM_DATA_TYPE_TEXT, LLM_DATA_STATE_START, "欢迎您", os_strlen("欢迎您"));
    } while (ret == LLME_AGAIN);

    while (1) {
        llm_tts_test_event_msg_process();

        if (!tts_mgr.tts_msi->fb_limits.counter) {
            os_sleep_ms(10);
            continue;
        }
        
        audio_len = llm_tts_recv(tts_mgr.tts_session, audio_buf, 2048, 0);
        if (audio_len > 0) {
            //os_printf("recv audio_len = %d!\r\n", audio_len);
            fb = msi_alloc_fb(tts_mgr.tts_msi, NULL, NULL, audio_len, 0, 0);
            if (fb) {
                os_memcpy(fb->data, audio_buf, audio_len);
                fb->codec_info = &tts_audio_info;
                while (1) {
                    ret = msi_output_fb(tts_mgr.tts_msi, fb, 1);
                    if (ret > 0) {
                        break;
                    } else {
                        os_sleep_ms(10);
                    }
                }

            }
        } else {
            os_sleep_ms(10);
        }
    }

cleanup:
    if (audio_buf) llm_free(audio_buf);
    if (tts_mgr.event_queue.rbq) llm_free(tts_mgr.event_queue.rbq);
    txmplayer_deinit();
    msi_destroy(tts_mgr.tts_msi);
    llm_tts_deinit(tts_mgr.tts_session);
    llm_global_deinit();
}

struct os_task llm_tts_test_task;
void llm_tts_test(void)
{
    OS_TASK_INIT("LLM_TTS_TEST", &llm_tts_test_task, llm_tts_test_main, NULL, OS_TASK_PRIORITY_NORMAL, NULL, 2048);
}
#endif

