#include "llm/llm_api.h"

#include "syscfg.h"
#include "devid.h"
#include "lib/multimedia/msi.h"
#include "utlist.h"
#include "jpgdef.h"
#include "lwip/netdb.h"

#if LLM_TEST == 3
/* Please add your server information */
#define TTI_URL         "https://ark.cn-beijing.volces.com/api/v3/images/generations"
#define ARK_API_KEY     "xxx"
#define ENDPOINT_ID     "doubao-seedream-3-0-t2i-250415"

#define LLM_TTI_TEST_EVENT_QUEUE_CNT    16

static uint8_t jpg_data_tag = 0;

struct llm_tti_test_event_msg {
    uint16  event;
    char    *msg;
    uint32  msg_len;
};

struct llm_tti_test_manage {
    void *tti_session;
    struct msi *tti_msi;
    RBUFFER_DEF_R(event_queue, struct llm_tti_test_event_msg);
} tti_mgr;

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

int32 llm_tti_test_event_cb(void *session, uint16 evt, uint32 param1, uint32 param2)
{
    uint8 process = 0;
    struct llm_tti_test_event_msg event_msg = {
        .event      = evt,
        .msg        = NULL,
        .msg_len    = 0,
    };

    switch (evt) {
        case LLM_EVENT_ERROR_MSG:
            event_msg.msg = llm_strdup((const char *)param1);
            if (event_msg.msg == NULL) {
                os_printf("tti err msg no memory!\r\n");
                break;
            }
            event_msg.msg_len = param2;
            process = 1;
            break;
        case LLM_EVENT_TTI_RESULT:
            event_msg.msg = llm_malloc(param2);
            if (event_msg.msg == NULL) {
                os_printf("tti result no memory!\r\n");
                break;
            }
            os_memcpy(event_msg.msg, param1, param2);
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
        RB_INT_SET(&tti_mgr.event_queue, event_msg);
    }
    return RET_OK;
}

////////////////////////////////////////////
const struct llm_model models[] = {
    {"doubao_tti",  &doubao_tti}
};

struct llm_global_param global = {
    .task_reuse     = 1,
    .model_count    = 1,
    .models         = models,
};

//TTI
struct llm_tti_sparam tti_session_cfg = {
    .qmsg_tx_cnt            = 16,
    .cb_max_size            = 2048,
    .evt_cb                 = llm_tti_test_event_cb,
};
struct Doubao_TTI_Cfg llm_tti_cfg = {
    .url = TTI_URL,
    .ark_api_key = ARK_API_KEY,
    .model = ENDPOINT_ID,
    .size = "512x512",
    .seed = "1",
    .watermark = "false",
    .guidance_scale = "2.5"
};
struct llm_trans_param tti_trans_cfg = {
    .buffersize         = 16384,
    .upload_buffersize  = 2048,
    .low_speed_limit    = 10,
    .low_speed_time     = 5,
    .connect_timeout    = 5,
    .blob_len           = 0,
    .blob_data          = NULL
};

sysevt_hdl_res llm_tti_test_network_event(uint32 event_id, uint32 data, uint32 priv)
{
    struct netif *nif;

    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_NETWORK, SYSEVT_LWIP_DHCPC_DONE):
            nif = netif_find("w0");
            gethostbyname_async("ark.cn-beijing.volces.com");
            break;
    }
    return SYSEVT_CONTINUE;
}

static int32 llm_tti_test_event_msg_process(void)
{
    struct framebuff *fb = NULL;
    struct llm_tti_test_event_msg event_msg = {0};

    RB_INT_GET(&tti_mgr.event_queue, event_msg);
    if (event_msg.event == LLM_EVENT_UNKNOWN) { return RET_OK; }

    switch (event_msg.event) {
        case LLM_EVENT_TTI_RESULT: {
            os_printf("TTI RESULT: %d\r\n", event_msg.msg_len);
            /*void *fp = osal_fopen("0:llm.jpeg","wb+");
            if (fp) {
                osal_fwrite(event_msg.msg,event_msg.msg_len,1,fp);
                os_printf("write sd: %d\r\n", event_msg.msg_len);
                osal_fclose(fp);
            }*/
            fb = fb_alloc(NULL, event_msg.msg_len, (F_JPG << 8) | FSTYPE_JPG_FILE, tti_mgr.tti_msi);
            if (fb) {
                fb->datatag = ++jpg_data_tag;
                hw_memcpy(fb->data, event_msg.msg, event_msg.msg_len);
                sys_dcache_clean_invalid_range_unaligned((uint32_t *)fb->data, event_msg.msg_len);
                msi_output_fb(tti_mgr.tti_msi, fb, 0);
            } else {
                os_printf("fb alloc fail!\r\n");
            }
            llm_free(event_msg.msg);
            break;
        }
        case LLM_EVENT_ERROR_MSG: {
            os_printf("recv TTI err msg(%d):", event_msg.msg_len);
            hgprintf_out(event_msg.msg, event_msg.msg_len, 0);
            _os_printf("\r\n");
            llm_free(event_msg.msg);
            // TODO
            break;
        }
        case LLM_EVENT_CONN_ERR:
        case LLM_EVENT_TX_ERR:
        case LLM_EVENT_RX_ERR: {
            os_printf("TTI %s err!\r\n", event_msg.event == LLM_EVENT_CONN_ERR ? "CONN" : (event_msg.event == LLM_EVENT_TX_ERR ? "TX" : "RX"));
            //llm_tti_interrupt(ig_mgr.tti_session, 0);
            break;
        }
        default: {
            os_printf("Not supported event msg: %d\r\n", event_msg.event);
            break;
        }
    }

    return RET_OK;
}

int32 llm_tti_test_app_init(char *llm_name)
{
    int32 ret = RET_OK;
    struct llm_tti_test_event_msg *event_queue_buf = NULL;
    llm_global_init(&global);

    tti_mgr.tti_session = llm_tti_init(llm_name, &tti_session_cfg);
    if (!tti_mgr.tti_session) {
        os_printf("tti session init fail!\r\n");
        return RET_ERR;
    }
    ret = llm_tti_config(tti_mgr.tti_session, LLM_CONFIG_TYPE_TRANS,
                            (void *)&tti_trans_cfg, sizeof(tti_trans_cfg));
    if (ret) {
        os_printf("tti_trans_cfg fail!\r\n");
        return RET_ERR;
    }
    ret = llm_tti_config(tti_mgr.tti_session, LLM_CONFIG_TYPE_MODEL,
                            (void *)&llm_tti_cfg, sizeof(llm_tti_cfg));
    if (ret) {
        os_printf("llm_tti_cfg fail!\r\n");
        return RET_ERR;
    }
    
    sys_event_take(SYS_EVENT(SYS_EVENT_NETWORK, 0), llm_tti_test_network_event, 0);

    tti_mgr.tti_msi = msi_new("tti_msi", 10, NULL);
    if (!tti_mgr.tti_msi) {
        os_printf("tti_msi err!\r\n");
        return RET_ERR;
    }
    tti_mgr.tti_msi->enable = 1;
    msi_add_output(tti_mgr.tti_msi, NULL, NULL, S_NET_JPEG);

    event_queue_buf = llm_malloc((LLM_TTI_TEST_EVENT_QUEUE_CNT + 1) * sizeof(struct llm_tti_test_event_msg));
    if (!event_queue_buf) {
        os_printf("event_queue_buf malloc fail, no memory!\r\n");
        return RET_ERR;
    }
    RB_INIT_R(&tti_mgr.event_queue, LLM_TTI_TEST_EVENT_QUEUE_CNT, event_queue_buf);

    os_printf("Waiting for network connection...");
    while (!sys_status.wifi_connected || !sys_status.dhcpc_done) {
        _os_printf(".");
        os_sleep(1);
    }
    os_printf("Network connected!\r\n");

    return ret;
}

void llm_tti_test_main(void)
{
    int32 ret = RET_OK;

    ret = llm_tti_test_app_init("doubao_tti");
    if (ret) {
        os_printf("tti main init failed: %d\r\n", ret);
        goto cleanup;
    }

    os_printf("tti main init start\r\n");
    ret = llm_tti_send(tti_mgr.tti_session, LLM_DATA_TYPE_TEXT, LLM_DATA_STATE_START, NULL, 0);
    ret = llm_tti_send(tti_mgr.tti_session, LLM_DATA_TYPE_TEXT, LLM_DATA_STATE_END, "鱼眼镜头，一只猫咪的头部，画面呈现出猫咪的五官因为拍摄方式扭曲的效果", os_strlen("鱼眼镜头，一只猫咪的头部，画面呈现出猫咪的五官因为拍摄方式扭曲的效果"));

    while (1) {
        llm_tti_test_event_msg_process();
        os_sleep_ms(1000);
    }

cleanup:
    if (tti_mgr.event_queue.rbq) llm_free(tti_mgr.event_queue.rbq);
    msi_destroy(tti_mgr.tti_msi);
    llm_tti_deinit(tti_mgr.tti_session);
    llm_global_deinit();
}

struct os_task llm_tti_test_task;
void llm_tti_test(void)
{
    OS_TASK_INIT("LLM_TTI_TEST", &llm_tti_test_task, llm_tti_test_main, NULL, OS_TASK_PRIORITY_NORMAL, NULL, 2048);
}
#endif


