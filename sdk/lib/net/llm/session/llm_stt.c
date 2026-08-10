#include "llm.h"

const char *llm_stt_state_str(llm_stt_state state)
{
    switch (state) {
        case LLM_STT_STATE_UNKNOWN:
            return "UNKNOWN";
        case LLM_STT_STATE_IDLE:
            return "IDLE";
        case LLM_STT_STATE_CONNECTING:
            return "CONNECTING";
        case LLM_STT_STATE_INTERRUPTING:
            return "INTERRUPTING";
        default:
            return "INVALID";
    }
    return "INVALID";
}

static void llm_stt_recycle(struct llm_session_stt *stt_session)
{
    struct llm_data clean_buff;

    llm_err("stt_session clean start!\r\n");
    while (RB_INT_GET(&stt_session->mq_tx, clean_buff)) {
        if (clean_buff.buff1) { llm_free(clean_buff.buff1); }
    }
    RB_RESET(&stt_session->mq_tx);

    if (stt_session->base.unsend.buff1) {
        llm_free(stt_session->base.unsend.buff1);
        stt_session->base.unsend.buff1 = NULL;
    }
    stt_session->base.unsend.buff1_len = 0;
    stt_session->base.unsend.transfer_state = 0;

    if (stt_session->callback_rx_buff) {
        os_memset(stt_session->callback_rx_buff, 0, stt_session->sparam->cb_max_size);
    }

    stt_session->base.model->m->recycle((void *)stt_session, 1);
    stt_session->base.model->m->disconnect((void *)stt_session);
    llm_err("stt_session clean done\r\n");
}

int32 llm_stt_event(struct llm_session_stt *stt_session, uint16 event, uint32 param1, uint32 param2)
{
    int32 ret = RET_OK;
    int32 ignore = 0;
    switch (event) {
        case LLM_EVENT_CONNECTED:
            llm_dbg("CONNECTED\r\n");
            ignore = 1;
            stt_session->base.connected = 1;
            llm_stt_set_state((struct llm_session_stt *)stt_session, LLM_STT_STATE_IDLE);
            break;
        case LLM_EVENT_DISCONNECT:
            llm_dbg("DISCONNECT\r\n");
            ignore = 1;
            stt_session->base.connected = 0;
            if (llm_stt_get_state(stt_session) != LLM_STT_STATE_IDLE) {
                llm_stt_set_state((struct llm_session_stt *)stt_session, LLM_STT_STATE_IDLE);
            }
            break;
        case LLM_EVENT_STT_RESULT:            
            llm_dbg("STT_RESULT\r\n");
            llm_stt_recycle(stt_session);
            break;
        case LLM_EVENT_ERROR_MSG:            
            llm_dbg("ERROR_MSG\r\n");
            llm_stt_recycle(stt_session);
            break;    
        case LLM_EVENT_CONN_ERR:
        case LLM_EVENT_TX_ERR:
        case LLM_EVENT_RX_ERR:
            llm_stt_set_state((struct llm_session_stt *)stt_session, LLM_STT_STATE_INTERRUPTING);
            break;
        default:
            break;
    }
    if (ret == RET_OK && !ignore) {
        ret = llm_event_notify((void *)stt_session, event, param1, param2);
    }
    return ret;
}

llm_stt_state llm_stt_get_state(struct llm_session_stt *stt_session)
{
    os_mutex_lock(&stt_session->lock, osWaitForever);
    llm_stt_state state = stt_session->state;
    os_mutex_unlock(&stt_session->lock);
    return state;
}

int32 llm_stt_set_state(struct llm_session_stt *stt_session, llm_stt_state new_state)
{
    int32 ret = RET_OK;
    int32 changed = 0;
    llm_stt_state curr_state = LLM_STT_STATE_UNKNOWN;

    os_mutex_lock(&stt_session->lock, osWaitForever);
    curr_state = stt_session->state;
    if (curr_state == new_state) {
        llm_err("The current status is \"%s\", the status has not changed!\r\n", llm_stt_state_str(curr_state));
        os_mutex_unlock(&stt_session->lock);
        return RET_OK;
    }

    switch (new_state) {
        case LLM_STT_STATE_IDLE: {
            changed = 1;
            break;
        }
        case LLM_STT_STATE_CONNECTING: {
//            stt_session->base.timeout = 0;
            changed = 1;
            break;
        }
        case LLM_STT_STATE_INTERRUPTING: {
            stt_session->need_recycle = 1;
            changed = 1;
            break;
        }
        default:
            break;
    }
    if (changed) {
        llm_err("New State:%s -> %s\r\n", llm_stt_state_str(curr_state), llm_stt_state_str(new_state));
        stt_session->state = new_state;
        ret = RET_OK;
    } else {
        llm_err("invalid state: %s -> %s\r\n", llm_stt_state_str(curr_state), llm_stt_state_str(new_state));
        ret = RET_ERR;
    }
    os_mutex_unlock(&stt_session->lock);
    return ret;
}

static int32 llm_stt_tx(struct llm_session_stt *stt_session)
{
    int32 ret = RET_ERR;

    if (stt_session->base.unsend.buff1_len == 0) {
        RB_INT_GET(&stt_session->mq_tx, stt_session->base.unsend);
    }
    if (stt_session->base.unsend.buff1_len != 0) {
        if (os_strcasecmp(stt_session->base.unsend.llm_name, stt_session->base.name) == 0) {
            switch (stt_session->base.unsend.transfer_state) {
                case LLM_DATA_STATE_START:
                    if (stt_session->base.connected == 0) {
                        if (llm_stt_get_state(stt_session) != LLM_STT_STATE_CONNECTING) {
                            llm_stt_set_state(stt_session, LLM_STT_STATE_CONNECTING);
                        }
                        ret = RET_OK;
                    } else {
                        ret = stt_session->base.model->m->send((void *)stt_session, &stt_session->base.unsend);
                        if (ret != RET_OK && ret != LLME_AGAIN && ret != LLME_NOMEM) {
                            llm_err("data send fail!(%d)\r\n", ret);
                        }
                    }
                    break;
                case LLM_DATA_STATE_MIDDLE:
                case LLM_DATA_STATE_END:
                    if (stt_session->base.connected == 1) {
                        ret = stt_session->base.model->m->send((void *)stt_session, &stt_session->base.unsend);
                        if (ret != RET_OK && ret != LLME_AGAIN && ret != LLME_NOMEM) {
                            llm_err("data send fail!(%d)\r\n", ret);
                        }
                    } else {
                        llm_err("Not connected!\r\n");
                    }
                    break;
                default:
                    llm_err("Wrong state!\r\n");
            }
        } else {
            if (stt_session->base.unsend.buff1) {
                llm_free(stt_session->base.unsend.buff1);
                stt_session->base.unsend.buff1 = NULL;
            }
            stt_session->base.unsend.buff1_len = 0;
            llm_err("send not supported!\r\n");
        }
    } else {
        ret = LLME_AGAIN;
    }
    if (ret == LLME_NOMEM) { ret = LLME_AGAIN; }
    return ret;
}

static int32 llm_stt_rx(struct llm_session_stt *stt_session)
{
    int32 ret = LLME_AGAIN;

    ret = stt_session->base.model->m->recv((void *)stt_session, stt_session->callback_rx_buff, stt_session->sparam->cb_max_size);
    if (ret != RET_OK && ret != LLME_AGAIN && ret != LLME_NOMEM) {
        llm_err("data recv fail!(%d)\r\n", ret);
    }
    if (ret == LLME_NOMEM) { ret = LLME_AGAIN; }
//    if (ret == RET_OK) {stt_session->base.timeout = os_jiffies(); }
    return ret;
}

static inline int32 llm_stt_safe_tx(struct llm_session_stt *stt_session, int *idle)
{
    int32 ret = llm_stt_tx(stt_session);
    if (ret != RET_OK && ret != LLME_AGAIN) {
        llm_err("llm_stt_tx fail!(%d)\r\n", ret);
        llm_stt_event(stt_session, LLM_EVENT_TX_ERR, ret, 0);
        return -1;
    }
    *idle = (ret != LLME_AGAIN) ? 0 : 1;
    return 0;
}

static inline int32 llm_stt_safe_rx(struct llm_session_stt *stt_session, int *idle)
{
    int32 ret = llm_stt_rx(stt_session);
    if (ret != RET_OK && ret != LLME_AGAIN) {
        llm_err("llm_stt_rx fail!(%d)\r\n", ret);
        llm_stt_event(stt_session, LLM_EVENT_RX_ERR, ret, 0);
        return -1;
    }
    *idle = (ret != LLME_AGAIN) ? 0 : 1;
    return 0;
}

static void llm_stt_run_task(struct llm_session *stt_hdl)
{
    int32 ret = RET_ERR;
    uint64 diff = 0;
    uint64 run_jiff = os_jiffies();
    int32 tx_idle = 1;
    int32 rx_idle = 1;
    llm_stt_state curr_state = LLM_STT_STATE_UNKNOWN;
    struct llm_session_stt *stt_session = (struct llm_session_stt *)stt_hdl;

    curr_state = llm_stt_get_state(stt_session);
    switch (curr_state) {
        case LLM_STT_STATE_IDLE:
            if (llm_stt_safe_tx(stt_session, &tx_idle)) { break; }
            if (stt_session->base.connected == 1) {
                if (llm_stt_safe_rx(stt_session, &rx_idle)) { break; }
            }
            break;
        case LLM_STT_STATE_CONNECTING: {
            ret = stt_session->base.model->m->connect((void *)stt_session);
            if (ret == LLME_AGAIN || ret == LLME_NOMEM) { tx_idle = rx_idle = 1; }
            break;
        }
        case LLM_STT_STATE_INTERRUPTING: {
            if (stt_session->need_recycle) {
                stt_session->need_recycle = 0;
                llm_stt_recycle(stt_session);
            }
            break;
        }
        default:
            llm_err("ERROR STATE: %s!\r\n", llm_stt_state_str(curr_state));
            break;
    }
    // 会话优雅退出
    if (stt_session->base.exit == 1) {
        stt_session->base.destroying = 1;
        llm_stt_deinit((void *)stt_session);
        return;
    }
    // 当tx_idle和rx_idle都为1时，说明没有数据发送和接收，等待10ms
    if (tx_idle && rx_idle) {
        os_sleep_ms(10);
        stt_session->again_cnt = 0;
    } else {
        stt_session->again_cnt++;
        // 当连续繁忙次数超过50次时，等待10ms，避免CPU占用过高
        if (stt_session->again_cnt % 50 == 0) {
            os_sleep_ms(10);
        }
    }
    diff = os_jiffies_to_msecs(os_jiffies() - run_jiff);
    if (diff > 100) {
        llm_err("llm_stt_run_task use %lld ms\r\n", diff);
    }
}

int32 llm_stt_send(void *stt_hdl, llm_data_type type, llm_data_state state, char *audio, uint32 audio_len)
{
    char *audio_buff = NULL;
    struct llm_session_stt *stt_session = NULL;
    llm_stt_state curr_state = LLM_STT_STATE_UNKNOWN;

    if (!stt_hdl) {
        llm_err("stt session is not exist!\r\n");
        return RET_ERR;
    }

    stt_session = (struct llm_session_stt *)stt_hdl;
    curr_state = llm_stt_get_state(stt_session);

    if (curr_state == LLM_STT_STATE_IDLE &&
        !RB_FULL(&stt_session->mq_tx)) {
        if (audio) {
            int32 reserved_header = stt_session->base.model->m->headsize;
            audio_buff = llm_malloc(audio_len + reserved_header);
            if (audio_buff == NULL) {
                llm_err("no memory!\r\n");
                return RET_ERR;
            }
            os_memcpy(audio_buff + reserved_header, audio, audio_len);
        }
        struct llm_data stt_send = {
            .llm_name = stt_session->base.name,
            .transfer_state = state,
            .buff1 = audio_buff,
            .buff1_len = audio_buff ? audio_len : -1,
            .offset = -1,
            .type   = type,
        };
        RB_INT_SET(&stt_session->mq_tx, stt_send);
        return RET_OK;
    }
    return LLME_AGAIN;
}

int32 llm_stt_recv(void *stt_hdl, char *buff, uint32 buff_size, uint8 flags)
{
    if (!stt_hdl) {
        llm_err("stt session not exist!\r\n");
        return RET_ERR;
    }
    return RET_OK;
}

int32 llm_stt_config(void *stt_hdl, llm_cfg_type type, void *cfg, uint32 cfg_size)
{
    struct llm_session_stt *stt_session = NULL;
    if (!stt_hdl) {
        llm_err("stt session is not exist!\r\n");
        return RET_ERR;
    }

    stt_session = (struct llm_session_stt *)stt_hdl;

    void **target_config = type ? &stt_session->base.transfer_config : &stt_session->base.platform_config;
    if (*target_config != NULL && os_memcmp(*target_config, cfg, cfg_size) == 0) {
        return RET_OK;
    }

    if (llm_copy_config(target_config, cfg, cfg_size) != RET_OK) {
        return RET_ERR;
    }
    if (type == LLM_CONFIG_TYPE_MODEL) {
        stt_session->base.platform_config_change = 1;
    }
    return RET_OK;
}

static int32 llm_stt_ctrl_set_state_with_timeout(struct llm_session_stt *stt_session, llm_stt_state state, uint32 timeout)
{
    int32 ret = RET_OK;
    uint64 start_jiffies = os_jiffies();
    llm_stt_state curr_state = LLM_STT_STATE_UNKNOWN;
    int32 elapsed_ms = 0;

    ret = llm_stt_set_state(stt_session, state);
    if (ret != RET_OK) {
        llm_err("llm_stt_set_state fail!(%d)\r\n", ret);
        return ret;
    }
    while (1) {
        curr_state = llm_stt_get_state(stt_session);
        if (curr_state != state) {
            ret = (curr_state == LLM_STT_STATE_IDLE) ? RET_OK : RET_ERR;
            if (ret != RET_OK) {
                llm_err("target state \"%s\" modified to \"%s\"\r\n",
                        llm_stt_state_str(state), llm_stt_state_str(curr_state));
            }
            return ret;
        }

        if (timeout > 0) {
            elapsed_ms = os_jiffies_to_msecs(os_jiffies() - start_jiffies);
            if (elapsed_ms >= timeout) {
                llm_err("target state \"%s\" timeout\"\r\n",
                        llm_stt_state_str(state));
                ret = LLME_WAIT_TIMEOUT;
                break;
            }
        }
        os_sleep_ms(10);
    }
    return ret;
}

int32 llm_stt_ctrl(void *stt_hdl, llm_ctrl_type ctrl_type, uint32 timeout)
{
    int32 ret = RET_OK;
    struct llm_session_stt *stt_session = NULL;
    if (!stt_hdl) {
        llm_err("stt session is not exist!\r\n");
        return RET_ERR;
    }
    stt_session = (struct llm_session_stt *)stt_hdl;

    switch (ctrl_type) {
        case LLM_CTRL_TYPE_INTERRUPT:
            return llm_stt_ctrl_set_state_with_timeout(stt_session, LLM_STT_STATE_INTERRUPTING, timeout);
        default:
            llm_err("ERROR STATE!(%d)\r\n", ctrl_type);
            return RET_ERR;
    }
    return ret;
}

static int32 llm_stt_sparam_check(struct llm_stt_sparam *sparam)
{
    ASSERT(sparam);

    if (sparam->qmsg_tx_cnt <= 0) {
        sparam->qmsg_tx_cnt = 16;
    }
    return RET_OK;
}

void *llm_stt_init(char *llm_name, struct llm_stt_sparam *sparam)
{
    if (llm_equipment_legality_check()) {
        return NULL;
    }

    int32 ret = RET_OK;
    struct llm_session_stt *stt_session = NULL;
    struct llm_data *tx_q = NULL;
    uint8 platform_inited = 0;

    if (!llm_name || !sparam) {
        llm_err("Please check the parameters!\r\n");
        return NULL;
    }

    if (llm_stt_sparam_check(sparam)) {
        llm_err("Please check the parameters!\r\n");
        return NULL;
    }

    struct llm_model *model = llm_find_model(llm_name);

    if (!model) {
        llm_err("llm [%s] not supported!\r\n", llm_name);
        return NULL;
    }

    stt_session = llm_zalloc(sizeof(struct llm_session_stt));
    if (!stt_session) {
        llm_err("stt_session malloc fail!\r\n");
        return NULL;
    }

    stt_session->base.model    = model;
    stt_session->base.name     = llm_name;
    stt_session->base.type     = LLM_STT;
    stt_session->base.run      = llm_stt_run_task;
    stt_session->base.evt_cb   = sparam->evt_cb;

    os_mutex_init(&stt_session->lock);

    if (llm_copy_config((void **)&stt_session->sparam, sparam, sizeof(struct llm_stt_sparam)) != RET_OK) {
        llm_err("no memory!\r\n");
        goto cleanup;
    }
    tx_q = llm_malloc((stt_session->sparam->qmsg_tx_cnt + 1) * sizeof(struct llm_data));
    if (!tx_q) {
        llm_err("no memory!\r\n");
        goto cleanup;
    }
    RB_INIT_R(&stt_session->mq_tx, stt_session->sparam->qmsg_tx_cnt, tx_q);

    if (sparam->cb_max_size > 0) {
        stt_session->callback_rx_buff = llm_malloc(sparam->cb_max_size);
        if (!stt_session->callback_rx_buff) {
            llm_err("no buffer!\r\n");
            goto cleanup;
        }
    }

    ret = stt_session->base.model->m->init((void *)stt_session);
    if (ret) {
        llm_err("platform init fail!\r\n");
        goto cleanup;
    }
    platform_inited = 1;
    llm_stt_set_state(stt_session, LLM_STT_STATE_IDLE);

    ret = llm_add_session((struct llm_session *)stt_session);
    if (ret) {
        llm_err("llm mgr add session fail!\r\n");
        goto cleanup;
    }

    return (void *)stt_session;
cleanup:
    if (platform_inited) { stt_session->base.model->m->deinit((void *)stt_session); }
    if (stt_session) {
        if (stt_session->callback_rx_buff) { llm_free(stt_session->callback_rx_buff); }
        llm_free(stt_session);
    }
    if (tx_q) { llm_free(tx_q); }
    return NULL;
}

int32 llm_stt_deinit(void *stt_hdl)
{
    int32 ret = RET_OK;
    struct llm_session_stt *stt_session = NULL;
    if (!stt_hdl) {
        llm_err("llm session not exist!\r\n");
        return RET_ERR;
    }

    stt_session = (struct llm_session_stt *)stt_hdl;

    if (!stt_session->base.destroying) {
        stt_session->base.exit = 1;
        return RET_OK;
    }

    stt_session->base.model->m->deinit((void *)stt_session);

    ret = llm_del_session((struct llm_session *)stt_hdl);
    if (ret) {
        llm_err("llm mgr remove session fail!\r\n");
        ASSERT(!ret);
    }

    llm_free(stt_session->mq_tx.rbq);
    if (stt_session->callback_rx_buff) { llm_free(stt_session->callback_rx_buff); }
    llm_free(stt_session);
    return RET_OK;
}

