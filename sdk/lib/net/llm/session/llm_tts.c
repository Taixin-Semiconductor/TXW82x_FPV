#include "llm.h"

const char *llm_tts_state_str(llm_tts_state state)
{
    switch (state) {
        case LLM_TTS_STATE_UNKNOWN:
            return "UNKNOWN";
        case LLM_TTS_STATE_IDLE:
            return "IDLE";
        case LLM_TTS_STATE_CONNECTING:
            return "CONNECTING";
        case LLM_TTS_STATE_INTERRUPTING:
            return "INTERRUPTING";
        default:
            return "INVALID";
    }
    return "INVALID";
}

static void llm_tts_recycle(struct llm_session_tts *tts_session, uint8 clean_rx)
{
    struct llm_data clean_buff;

    llm_err("tts_session clean start!\r\n");
    while (RB_INT_GET(&tts_session->mq_tx, clean_buff)) {
        if (clean_buff.buff1) { llm_free(clean_buff.buff1); }
    }
    RB_RESET(&tts_session->mq_tx);

    if (clean_rx) {
        rbuffer_reset(&tts_session->tts_rx);
        tts_session->wpos_old = -1;
    }

    if (tts_session->base.unsend.buff1) {
        llm_free(tts_session->base.unsend.buff1);
        tts_session->base.unsend.buff1 = NULL;
    }
    tts_session->base.unsend.buff1_len = 0;
    tts_session->base.unsend.transfer_state = 0;

    if (tts_session->callback_rx_buff) {
        os_memset(tts_session->callback_rx_buff, 0, tts_session->sparam->cb_max_size);
    }

    tts_session->base.model->m->recycle((void *)tts_session, 1);
    tts_session->base.model->m->disconnect((void *)tts_session);
    llm_err("tts_session clean done\r\n");
}

int32 llm_tts_event(struct llm_session_tts *tts_session, uint16 event, uint32 param1, uint32 param2)
{
    int32 ret = RET_OK;
    int32 ignore = 0;
    switch (event) {
        case LLM_EVENT_CONNECTED:
            llm_dbg("CONNECTED\r\n");
            ignore = 1;
            tts_session->base.connected = 1;
            llm_tts_set_state((struct llm_session_tts *)tts_session, LLM_TTS_STATE_IDLE);
            break;
        case LLM_EVENT_DISCONNECT:
            llm_dbg("DISCONNECT\r\n");
            ignore = 1;
            tts_session->base.connected = 0;
            if (llm_tts_get_state(tts_session) != LLM_TTS_STATE_IDLE) {
                llm_tts_set_state((struct llm_session_tts *)tts_session, LLM_TTS_STATE_IDLE);
            }
            break;
        case LLM_EVENT_TTS_RESULT:
            llm_dbg("TTS_RESULT\r\n");
            ignore = 1;
            tts_session->base.model->m->disconnect((void *)tts_session);
            break;
        case LLM_EVENT_ERROR_MSG:
            llm_dbg("ERROR_MSG\r\n");
            tts_session->base.model->m->disconnect((void *)tts_session);
            break;    
        case LLM_EVENT_CONN_ERR:
        case LLM_EVENT_TX_ERR:
        case LLM_EVENT_RX_ERR:
            llm_tts_set_state((struct llm_session_tts *)tts_session, LLM_TTS_STATE_INTERRUPTING);
            break;
        default:
            break;
    }
    if (ret == RET_OK && !ignore) {
        ret = llm_event_notify((void *)tts_session, event, param1, param2);
    }
    return ret;
}

llm_tts_state llm_tts_get_state(struct llm_session_tts *tts_session)
{
    os_mutex_lock(&tts_session->lock, osWaitForever);
    llm_tts_state state = tts_session->state;
    os_mutex_unlock(&tts_session->lock);
    return state;
}

int32 llm_tts_set_state(struct llm_session_tts *tts_session, llm_tts_state new_state)
{
    int32 ret = RET_OK;
    int32 changed = 0;
    llm_tts_state curr_state = LLM_TTS_STATE_UNKNOWN;

    os_mutex_lock(&tts_session->lock, osWaitForever);
    curr_state = tts_session->state;
    if (curr_state == new_state) {
        llm_err("The current status is \"%s\", the status has not changed!\r\n", llm_tts_state_str(curr_state));
        os_mutex_unlock(&tts_session->lock);
        return RET_OK;
    }

    switch (new_state) {
        case LLM_TTS_STATE_IDLE: {
            changed = 1;
            break;
        }
        case LLM_TTS_STATE_CONNECTING: {
//            tts_session->base.timeout = 0;
            changed = 1;
            break;
        }
        case LLM_TTS_STATE_INTERRUPTING: {
            tts_session->need_recycle = 1;
            changed = 1;
            break;
        }
        default:
            break;
    }
    if (changed) {
        llm_err("New State:%s -> %s\r\n", llm_tts_state_str(curr_state), llm_tts_state_str(new_state));
        tts_session->state = new_state;
        ret = RET_OK;
    } else {
        llm_err("invalid state: %s -> %s\r\n", llm_tts_state_str(curr_state), llm_tts_state_str(new_state));
        ret = RET_ERR;
    }
    os_mutex_unlock(&tts_session->lock);
    return ret;
}

static int32 llm_tts_tx(struct llm_session_tts *tts_session)
{
    int32 ret = RET_ERR;

    if (tts_session->base.unsend.buff1_len == 0) {
        RB_INT_GET(&tts_session->mq_tx, tts_session->base.unsend);
    }
    if (tts_session->base.unsend.buff1_len != 0) {
        if (os_strcasecmp(tts_session->base.unsend.llm_name, tts_session->base.name) == 0) {
            switch (tts_session->base.unsend.transfer_state) {
                case LLM_DATA_STATE_START:
                    if (tts_session->base.connected == 0) {
                        if (llm_tts_get_state(tts_session) != LLM_TTS_STATE_CONNECTING) {
                            llm_tts_set_state(tts_session, LLM_TTS_STATE_CONNECTING);
                        }
                        ret = RET_OK;
                    } else {
                        ret = tts_session->base.model->m->send((void *)tts_session, &tts_session->base.unsend);
                        if (ret != RET_OK && ret != LLME_AGAIN && ret != LLME_NOMEM) {
                            llm_err("data send fail!(%d)\r\n", ret);
                        }
                    }
                    break;
                case LLM_DATA_STATE_MIDDLE:
                case LLM_DATA_STATE_END:
                    if (tts_session->base.connected == 1) {
                        ret = tts_session->base.model->m->send((void *)tts_session, &tts_session->base.unsend);
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
            if (tts_session->base.unsend.buff1) {
                llm_free(tts_session->base.unsend.buff1);
                tts_session->base.unsend.buff1 = NULL;
            }
            tts_session->base.unsend.buff1_len = 0;
            llm_err("send not supported!\r\n");
        }
    } else {
        ret = LLME_AGAIN;
    }
    if (ret == LLME_NOMEM) { ret = LLME_AGAIN; }
    return ret;
}

static int32 llm_tts_rx(struct llm_session_tts *tts_session)
{
    int32 ret = LLME_AGAIN;

    ret = tts_session->base.model->m->recv((void *)tts_session, tts_session->callback_rx_buff, tts_session->sparam->cb_max_size);
    if (ret != RET_OK && ret != LLME_AGAIN && ret != LLME_NOMEM) {
        llm_err("data recv fail!(%d)\r\n", ret);
    }
    if (ret == LLME_NOMEM) { ret = LLME_AGAIN; }
//    if (ret == RET_OK) {tts_session->base.timeout = os_jiffies(); }
    return ret;
}

static inline int32 llm_tts_safe_tx(struct llm_session_tts *tts_session, int *idle)
{
    int32 ret = llm_tts_tx(tts_session);
    if (ret != RET_OK && ret != LLME_AGAIN) {
        llm_err("llm_tts_tx fail!(%d)\r\n", ret);
        llm_tts_event(tts_session, LLM_EVENT_TX_ERR, ret, 0);
        return -1;
    }
    *idle = (ret != LLME_AGAIN) ? 0 : 1;
    return 0;
}

static inline int32 llm_tts_safe_rx(struct llm_session_tts *tts_session, int *idle)
{
    int32 ret = llm_tts_rx(tts_session);
    if (ret != RET_OK && ret != LLME_AGAIN) {
        llm_err("llm_tts_rx fail!(%d)\r\n", ret);
        llm_tts_event(tts_session, LLM_EVENT_RX_ERR, ret, 0);
        return -1;
    }
    *idle = (ret != LLME_AGAIN) ? 0 : 1;
    return 0;
}

static void llm_tts_run_task(struct llm_session *tts_hdl)
{
    int32 ret = RET_ERR;
    uint64 diff = 0;
    uint64 run_jiff = os_jiffies();
    int32 tx_idle = 1;
    int32 rx_idle = 1;
    llm_tts_state curr_state = LLM_TTS_STATE_UNKNOWN;
    struct llm_session_tts *tts_session = (struct llm_session_tts *)tts_hdl;

    curr_state = llm_tts_get_state(tts_session);
    switch (curr_state) {
        case LLM_TTS_STATE_IDLE:
            if (llm_tts_safe_tx(tts_session, &tx_idle)) { break; }
            if (tts_session->base.connected == 1) {
                if (llm_tts_safe_rx(tts_session, &rx_idle)) { break; }
            }
            break;
        case LLM_TTS_STATE_CONNECTING: {
            ret = tts_session->base.model->m->connect((void *)tts_session);
            if (ret == LLME_AGAIN || ret == LLME_NOMEM) { tx_idle = rx_idle = 1; }
            break;
        }
        case LLM_TTS_STATE_INTERRUPTING: {
            if (tts_session->need_recycle) {
                tts_session->need_recycle = 0;
                llm_tts_recycle(tts_session, 1);
            }
            break;
        }
        default:
            llm_err("ERROR STATE: %s!\r\n", llm_tts_state_str(curr_state));
            break;
    }
    // 会话优雅退出
    if (tts_session->base.exit == 1) {
        tts_session->base.destroying = 1;
        llm_tts_deinit((void *)tts_session);
        return;
    }
    // 当tx_idle和rx_idle都为1时，说明没有数据发送和接收，等待10ms
    if (tx_idle && rx_idle) {
        os_sleep_ms(10);
        tts_session->again_cnt = 0;
    } else {
        tts_session->again_cnt++;
        // 当连续繁忙次数超过50次时，等待10ms，避免CPU占用过高
        if (tts_session->again_cnt % 50 == 0) {
            os_sleep_ms(10);
        }
    }
    diff = os_jiffies_to_msecs(os_jiffies() - run_jiff);
    if (diff > 100) {
        llm_err("llm_tts_run_task use %lld ms\r\n", diff);
    }
}

int32 llm_tts_send(void *tts_hdl, llm_data_type type, llm_data_state state, char *text, uint32 text_len)
{
    char *text_buff = NULL;
    struct llm_session_tts *tts_session = NULL;
    llm_tts_state curr_state = LLM_TTS_STATE_UNKNOWN;

    if (!tts_hdl) {
        llm_err("tts session is not exist!\r\n");
        return RET_ERR;
    }

    tts_session = (struct llm_session_tts *)tts_hdl;
    curr_state = llm_tts_get_state(tts_session);

    if (curr_state == LLM_TTS_STATE_IDLE &&
        !RB_FULL(&tts_session->mq_tx)) {
        if (text) {
            int32 reserved_header = tts_session->base.model->m->headsize;
            text_buff = llm_malloc(text_len + reserved_header + 1);
            if (text_buff == NULL) {
                llm_err("no memory!\r\n");
                return RET_ERR;
            }
            os_memcpy(text_buff + reserved_header, text, text_len);
            text_buff[text_len + reserved_header] = '\0';
        }
        struct llm_data tts_send = {
            .llm_name = tts_session->base.name,
            .transfer_state = state,
            .buff1 = text_buff,
            .buff1_len = text_buff ? text_len : -1,
            .offset = -1,
            .type   = type,
        };
        RB_INT_SET(&tts_session->mq_tx, tts_send);
        return RET_OK;
    }
    return LLME_AGAIN;
}

int32 llm_tts_recv(void *tts_hdl, char *buff, uint32 buff_size, uint8 flags)
{
    struct llm_session_tts *tts_session = NULL;
    llm_tts_state curr_state = LLM_TTS_STATE_UNKNOWN;
    int32 audio_len = 0;
    int32 count = 0;
    uint32 rpos, wpos;
    uint32 flag;

    if (!tts_hdl) {
        llm_err("tts session not exist!\r\n");
        return RET_ERR;
    }

    tts_session = (struct llm_session_tts *)tts_hdl;
    curr_state = llm_tts_get_state(tts_session);
    if (curr_state == LLM_TTS_STATE_INTERRUPTING) {
        return LLME_AGAIN;
    }
    
    if (flags == 1 && RB_COUNT(&tts_session->tts_rx) < buff_size) {
        return LLME_AGAIN;
    }
    
    flag = disable_irq();
    rpos = tts_session->tts_rx.rpos;
    wpos = (tts_session->wpos_old != -1 && tts_session->wpos_old != rpos) ? tts_session->wpos_old : tts_session->tts_rx.wpos;
    enable_irq(flag);

    count = ((rpos <= wpos) ? (wpos - rpos) : (tts_session->tts_rx.qsize - rpos + wpos));
    if (tts_session->wpos_old != -1 && (buff_size >= count || rpos == tts_session->wpos_old)) {
        llm_tts_event(tts_session, LLM_EVENT_TTS_AUDIO_START, 0, 0);
        tts_session->wpos_old = -1;
    }
    if (count > buff_size) {
        count = buff_size;
    }
    audio_len = rbuffer_get(&tts_session->tts_rx, buff, count);
    return audio_len;
}

int32 llm_tts_config(void *tts_hdl, llm_cfg_type type, void *cfg, uint32 cfg_size)
{
    struct llm_session_tts *tts_session = NULL;
    if (!tts_hdl) {
        llm_err("tts session is not exist!\r\n");
        return RET_ERR;
    }

    tts_session = (struct llm_session_tts *)tts_hdl;

    void **target_config = type ? &tts_session->base.transfer_config : &tts_session->base.platform_config;
    if (*target_config != NULL && os_memcmp(*target_config, cfg, cfg_size) == 0) {
        return RET_OK;
    }

    if (llm_copy_config(target_config, cfg, cfg_size) != RET_OK) {
        return RET_ERR;
    }
    if (type == LLM_CONFIG_TYPE_MODEL) {
        tts_session->base.platform_config_change = 1;
    }
    return RET_OK;
}

int32 llm_tts_play_audio(void *tts_hdl, const       char *audio, uint32 length)
{
    struct llm_session_tts *tts_session = NULL;
    uint32 chunk_size = 1024;
    uint32 written = 0;
    uint32 to_write = 0;

    if (!tts_hdl) {
        llm_err("tts session not exist!\r\n");
        return RET_ERR;
    }

    tts_session = (struct llm_session_tts *)tts_hdl;

    while (written < length) {
        if (llm_tts_get_state(tts_session) == LLM_TTS_STATE_INTERRUPTING) {
            return RET_ERR;
        }
        to_write = (length - written > chunk_size) ? chunk_size : (length - written);

        if (RB_IDLE(&tts_session->tts_rx) < to_write) {
            os_sleep_ms(10);
        } else {
            rbuffer_set(&tts_session->tts_rx, (void *)audio + written, to_write);
            written += to_write;
        }
    }
    return RET_OK;
}

static int32 llm_tts_ctrl_set_state_with_timeout(struct llm_session_tts *tts_session, llm_tts_state state, uint32 timeout)
{
    int32 ret = RET_OK;
    uint64 start_jiffies = os_jiffies();
    llm_tts_state curr_state = LLM_TTS_STATE_UNKNOWN;
    int32 elapsed_ms = 0;

    ret = llm_tts_set_state(tts_session, state);
    if (ret != RET_OK) {
        llm_err("llm_tts_set_state fail!(%d)\r\n", ret);
        return ret;
    }
    while (1) {
        curr_state = llm_tts_get_state(tts_session);
        if (curr_state != state) {
            ret = (curr_state == LLM_TTS_STATE_IDLE) ? RET_OK : RET_ERR;
            if (ret != RET_OK) {
                llm_err("target state \"%s\" modified to \"%s\"\r\n",
                        llm_tts_state_str(state), llm_tts_state_str(curr_state));
            }
            return ret;
        }

        if (timeout > 0) {
            elapsed_ms = os_jiffies_to_msecs(os_jiffies() - start_jiffies);
            if (elapsed_ms >= timeout) {
                llm_err("target state \"%s\" timeout\"\r\n",
                        llm_tts_state_str(state));
                ret = LLME_WAIT_TIMEOUT;
                break;
            }
        }
        os_sleep_ms(10);
    }
    return ret;
}

int32 llm_tts_ctrl(void *tts_hdl, llm_ctrl_type ctrl_type, uint32 timeout)
{
    int32 ret = RET_OK;
    struct llm_session_tts *tts_session = NULL;
    if (!tts_hdl) {
        llm_err("tts session is not exist!\r\n");
        return RET_ERR;
    }
    tts_session = (struct llm_session_tts *)tts_hdl;

    switch (ctrl_type) {
        case LLM_CTRL_TYPE_INTERRUPT:
            return llm_tts_ctrl_set_state_with_timeout(tts_session, LLM_TTS_STATE_INTERRUPTING, timeout);
        default:
            llm_err("ERROR STATE!(%d)\r\n", ctrl_type);
            return RET_ERR;
    }
    return ret;
}

static int32 llm_tts_sparam_check(struct llm_tts_sparam *sparam)
{
    ASSERT(sparam);

    if (sparam->qmsg_tx_cnt <= 0) {
        sparam->qmsg_tx_cnt = 16;
    }
    if (sparam->rx_buff_size <= 0) {
        sparam->rx_buff_size = 4096;
    }
    return RET_OK;
}

void *llm_tts_init(char *llm_name, struct llm_tts_sparam *sparam)
{
    if (llm_equipment_legality_check()) {
        return NULL;
    }

    int32 ret = RET_OK;
    struct llm_session_tts *tts_session = NULL;
    struct llm_data *tx_q = NULL;
    void *tts_rx_buff = NULL;
    uint8 platform_inited = 0;

    if (!llm_name || !sparam) {
        llm_err("Please check the parameters!\r\n");
        return NULL;
    }

    if (llm_tts_sparam_check(sparam)) {
        llm_err("Please check the parameters!\r\n");
        return NULL;
    }

    struct llm_model *model = llm_find_model(llm_name);

    if (!model) {
        llm_err("llm [%s] not supported!\r\n", llm_name);
        return NULL;
    }

    tts_session = llm_zalloc(sizeof(struct llm_session_tts));
    if (!tts_session) {
        llm_err("tts_session malloc fail!\r\n");
        return NULL;
    }

    tts_session->base.model    = model;
    tts_session->base.name     = llm_name;
    tts_session->base.type     = LLM_TTS;
    tts_session->base.run      = llm_tts_run_task;
    tts_session->base.evt_cb   = sparam->evt_cb;
    tts_session->wpos_old      = -1;

    os_mutex_init(&tts_session->lock);

    if (llm_copy_config((void **)&tts_session->sparam, sparam, sizeof(struct llm_tts_sparam)) != RET_OK) {
        llm_err("no memory!\r\n");
        goto cleanup;
    }
    tx_q = llm_malloc((tts_session->sparam->qmsg_tx_cnt + 1) * sizeof(struct llm_data));
    if (!tx_q) {
        llm_err("no memory!\r\n");
        goto cleanup;
    }
    RB_INIT_R(&tts_session->mq_tx, tts_session->sparam->qmsg_tx_cnt, tx_q);

    tts_rx_buff = llm_zalloc(sparam->rx_buff_size);
    if (!tts_rx_buff) {
        llm_err("no memory!\r\n");
        goto cleanup;
    }
    rbuffer_init(&tts_session->tts_rx, sparam->rx_buff_size, tts_rx_buff);

    if (sparam->cb_max_size > 0) {
        tts_session->callback_rx_buff = llm_malloc(sparam->cb_max_size);
        if (!tts_session->callback_rx_buff) {
            llm_err("no buffer!\r\n");
            goto cleanup;
        }
    }

    ret = tts_session->base.model->m->init((void *)tts_session);
    if (ret) {
        llm_err("platform init fail!\r\n");
        goto cleanup;
    }
    platform_inited = 1;
    llm_tts_set_state(tts_session, LLM_TTS_STATE_IDLE);

    ret = llm_add_session((struct llm_session *)tts_session);
    if (ret) {
        llm_err("llm mgr add session fail!\r\n");
        goto cleanup;
    }

    return (void *)tts_session;
cleanup:
    if (platform_inited) { tts_session->base.model->m->deinit((void *)tts_session); }
    if (tts_session) {
    if (tts_session->callback_rx_buff) { llm_free(tts_session->callback_rx_buff); }
        llm_free(tts_session);
    }
    if (tts_rx_buff) { llm_free(tts_rx_buff); }
    if (tx_q) { llm_free(tx_q); }
    return NULL;
}

int32 llm_tts_deinit(void *tts_hdl)
{
    int32 ret = RET_OK;
    struct llm_session_tts *tts_session = NULL;
    if (!tts_hdl) {
        llm_err("llm session not exist!\r\n");
        return RET_ERR;
    }

    tts_session = (struct llm_session_tts *)tts_hdl;

    if (!tts_session->base.destroying) {
        tts_session->base.exit = 1;
        return RET_OK;
    }

    tts_session->base.model->m->deinit((void *)tts_session);

    ret = llm_del_session((struct llm_session *)tts_hdl);
    if (ret) {
        llm_err("llm mgr remove session fail!\r\n");
        ASSERT(!ret);
    }

    llm_free(tts_session->mq_tx.rbq);
    llm_free(tts_session->tts_rx.rbq);
    if (tts_session->callback_rx_buff) { llm_free(tts_session->callback_rx_buff); }
    llm_free(tts_session);
    return RET_OK;
}


