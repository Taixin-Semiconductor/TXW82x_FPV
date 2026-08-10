#include "llm.h"

const char *llm_tti_state_str(llm_tti_state state)
{
    switch (state) {
        case LLM_TTI_STATE_UNKNOWN:
            return "UNKNOWN";
        case LLM_TTI_STATE_IDLE:
            return "IDLE";
        case LLM_TTI_STATE_CONNECTING:
            return "CONNECTING";
        case LLM_TTI_STATE_INTERRUPTING:
            return "INTERRUPTING";
        default:
            return "INVALID";
    }
    return "INVALID";
}

static void llm_tti_recycle(struct llm_session_tti *tti_session)
{
    struct llm_data clean_buff;

    llm_err("tti_session clean start!\r\n");
    while (RB_INT_GET(&tti_session->mq_tx, clean_buff)) {
        if (clean_buff.buff1) { llm_free(clean_buff.buff1); }
    }
    RB_RESET(&tti_session->mq_tx);

    if (tti_session->base.unsend.buff1) {
        llm_free(tti_session->base.unsend.buff1);
        tti_session->base.unsend.buff1 = NULL;
    }
    tti_session->base.unsend.buff1_len = 0;
    tti_session->base.unsend.transfer_state = 0;

    if (tti_session->callback_rx_buff) {
        os_memset(tti_session->callback_rx_buff, 0, tti_session->sparam->cb_max_size);
    }

    tti_session->base.model->m->recycle((void *)tti_session, 1);
    tti_session->base.model->m->disconnect((void *)tti_session);
    llm_err("tti_session clean done\r\n");
}

int32 llm_tti_event(struct llm_session_tti *tti_session, uint16 event, uint32 param1, uint32 param2)
{
    int32 ret = RET_OK;
    int32 ignore = 0;
    switch (event) {
        case LLM_EVENT_CONNECTED:
            llm_dbg("CONNECTED\r\n");
            ignore = 1;
            tti_session->base.connected = 1;
            llm_tti_set_state((struct llm_session_tti *)tti_session, LLM_TTI_STATE_IDLE);
            break;
        case LLM_EVENT_DISCONNECT:
            llm_dbg("DISCONNECT\r\n");
            ignore = 1;
            tti_session->base.connected = 0;
            if (llm_tti_get_state(tti_session) != LLM_TTI_STATE_IDLE) {
                llm_tti_set_state((struct llm_session_tti *)tti_session, LLM_TTI_STATE_IDLE);
            }
            break;
        case LLM_EVENT_TTI_RESULT:            
            llm_dbg("TTI_RESULT\r\n");
            llm_tti_recycle(tti_session);
            break;
        case LLM_EVENT_ERROR_MSG:            
            llm_dbg("ERROR_MSG\r\n");
            llm_tti_recycle(tti_session);
            break;    
        case LLM_EVENT_CONN_ERR:
        case LLM_EVENT_TX_ERR:
        case LLM_EVENT_RX_ERR:
            llm_tti_set_state((struct llm_session_tti *)tti_session, LLM_TTI_STATE_INTERRUPTING);
            break;
        default:
            break;
    }
    if (ret == RET_OK && !ignore) {
        ret = llm_event_notify((void *)tti_session, event, param1, param2);
    }
    return ret;
}

llm_tti_state llm_tti_get_state(struct llm_session_tti *tti_session)
{
    os_mutex_lock(&tti_session->lock, osWaitForever);
    llm_tti_state state = tti_session->state;
    os_mutex_unlock(&tti_session->lock);
    return state;
}

int32 llm_tti_set_state(struct llm_session_tti *tti_session, llm_tti_state new_state)
{
    int32 ret = RET_OK;
    int32 changed = 0;
    llm_tti_state curr_state = LLM_TTI_STATE_UNKNOWN;

    os_mutex_lock(&tti_session->lock, osWaitForever);
    curr_state = tti_session->state;
    if (curr_state == new_state) {
        llm_err("The current status is \"%s\", the status has not changed!\r\n", llm_tti_state_str(curr_state));
        os_mutex_unlock(&tti_session->lock);
        return RET_OK;
    }

    switch (new_state) {
        case LLM_TTI_STATE_IDLE: {
            changed = 1;
            break;
        }
        case LLM_TTI_STATE_CONNECTING: {
//            tti_session->base.timeout = 0;
            changed = 1;
            break;
        }
        case LLM_TTI_STATE_INTERRUPTING: {
            tti_session->need_recycle = 1;
            changed = 1;
            break;
        }
        default:
            break;
    }
    if (changed) {
        llm_err("New State:%s -> %s\r\n", llm_tti_state_str(curr_state), llm_tti_state_str(new_state));
        tti_session->state = new_state;
        ret = RET_OK;
    } else {
        llm_err("invalid state: %s -> %s\r\n", llm_tti_state_str(curr_state), llm_tti_state_str(new_state));
        ret = RET_ERR;
    }
    os_mutex_unlock(&tti_session->lock);
    return ret;
}

static int32 llm_tti_tx(struct llm_session_tti *tti_session)
{
    int32 ret = RET_ERR;

    if (tti_session->base.unsend.buff1_len == 0) {
        RB_INT_GET(&tti_session->mq_tx, tti_session->base.unsend);
    }
    if (tti_session->base.unsend.buff1_len != 0) {
        if (os_strcasecmp(tti_session->base.unsend.llm_name, tti_session->base.name) == 0) {
            switch (tti_session->base.unsend.transfer_state) {
                case LLM_DATA_STATE_START:
                    if (tti_session->base.connected == 0) {
                        if (llm_tti_get_state(tti_session) != LLM_TTI_STATE_CONNECTING) {
                            llm_tti_set_state(tti_session, LLM_TTI_STATE_CONNECTING);
                        }
                        ret = RET_OK;
                    } else {
                        ret = tti_session->base.model->m->send((void *)tti_session, &tti_session->base.unsend);
                        if (ret != RET_OK && ret != LLME_AGAIN && ret != LLME_NOMEM) {
                            llm_err("data send fail!(%d)\r\n", ret);
                        }
                    }
                    break;
                case LLM_DATA_STATE_MIDDLE:
                case LLM_DATA_STATE_END:
                    if (tti_session->base.connected == 1) {
                        ret = tti_session->base.model->m->send((void *)tti_session, &tti_session->base.unsend);
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
            if (tti_session->base.unsend.buff1) {
                llm_free(tti_session->base.unsend.buff1);
                tti_session->base.unsend.buff1 = NULL;
            }
            tti_session->base.unsend.buff1_len = 0;
            llm_err("send not supported!\r\n");
        }
    } else {
        ret = LLME_AGAIN;
    }
    if (ret == LLME_NOMEM) { ret = LLME_AGAIN; }
    return ret;
}

static int32 llm_tti_rx(struct llm_session_tti *tti_session)
{
    int32 ret = LLME_AGAIN;

    ret = tti_session->base.model->m->recv((void *)tti_session, tti_session->callback_rx_buff, tti_session->sparam->cb_max_size);
    if (ret != RET_OK && ret != LLME_AGAIN && ret != LLME_NOMEM) {
        llm_err("data recv fail!(%d)\r\n", ret);
    }
    if (ret == LLME_NOMEM) { ret = LLME_AGAIN; }
//    if (ret == RET_OK) {tti_session->base.timeout = os_jiffies(); }
    return ret;
}

static inline int32 llm_tti_safe_tx(struct llm_session_tti *tti_session, int *idle)
{
    int32 ret = llm_tti_tx(tti_session);
    if (ret != RET_OK && ret != LLME_AGAIN) {
        llm_err("llm_tti_tx fail!(%d)\r\n", ret);
        llm_tti_event(tti_session, LLM_EVENT_TX_ERR, ret, 0);
        return -1;
    }
    *idle = (ret != LLME_AGAIN) ? 0 : 1;
    return 0;
}

static inline int32 llm_tti_safe_rx(struct llm_session_tti *tti_session, int *idle)
{
    int32 ret = llm_tti_rx(tti_session);
    if (ret != RET_OK && ret != LLME_AGAIN) {
        llm_err("llm_tti_rx fail!(%d)\r\n", ret);
        llm_tti_event(tti_session, LLM_EVENT_RX_ERR, ret, 0);
        return -1;
    }
    *idle = (ret != LLME_AGAIN) ? 0 : 1;
    return 0;
}

static void llm_tti_run_task(struct llm_session *tti_hdl)
{
    int32 ret = RET_ERR;
    uint64 diff = 0;
    uint64 run_jiff = os_jiffies();
    int32 tx_idle = 1;
    int32 rx_idle = 1;
    llm_tti_state curr_state = LLM_TTI_STATE_UNKNOWN;
    struct llm_session_tti *tti_session = (struct llm_session_tti *)tti_hdl;

    curr_state = llm_tti_get_state(tti_session);
    switch (curr_state) {
        case LLM_TTI_STATE_IDLE:
            if (llm_tti_safe_tx(tti_session, &tx_idle)) { break; }
            if (tti_session->base.connected == 1) {
                if (llm_tti_safe_rx(tti_session, &rx_idle)) { break; }
            }
            break;
        case LLM_TTI_STATE_CONNECTING: {
            ret = tti_session->base.model->m->connect((void *)tti_session);
            if (ret == LLME_AGAIN || ret == LLME_NOMEM) { tx_idle = rx_idle = 1; }
            break;
        }
        case LLM_TTI_STATE_INTERRUPTING: {
            if (tti_session->need_recycle) {
                tti_session->need_recycle = 0;
                llm_tti_recycle(tti_session);
            }
            break;
        }
        default:
            llm_err("ERROR STATE: %s!\r\n", llm_tti_state_str(curr_state));
            break;
    }
    // 会话优雅退出
    if (tti_session->base.exit == 1) {
        tti_session->base.destroying = 1;
        llm_tti_deinit((void *)tti_session);
        return;
    }
    // 当tx_idle和rx_idle都为1时，说明没有数据发送和接收，等待10ms
    if (tx_idle && rx_idle) {
        os_sleep_ms(10);
        tti_session->again_cnt = 0;
    } else {
        tti_session->again_cnt++;
        // 当连续繁忙次数超过50次时，等待10ms，避免CPU占用过高
        if (tti_session->again_cnt % 50 == 0) {
            os_sleep_ms(10);
        }
    }
    diff = os_jiffies_to_msecs(os_jiffies() - run_jiff);
    if (diff > 100) {
        llm_err("llm_tti_run_task use %lld ms\r\n", diff);
    }
}

int32 llm_tti_send(void *tti_hdl, llm_data_type type, llm_data_state state, char *text, uint32 text_len)
{
    char *text_buff = NULL;
    struct llm_session_tti *tti_session = NULL;
    llm_tti_state curr_state = LLM_TTI_STATE_UNKNOWN;

    if (!tti_hdl) {
        llm_err("tti session is not exist!\r\n");
        return RET_ERR;
    }

    tti_session = (struct llm_session_tti *)tti_hdl;
    curr_state = llm_tti_get_state(tti_session);

    if (curr_state == LLM_TTI_STATE_IDLE &&
        !RB_FULL(&tti_session->mq_tx)) {
        if (text) {
            int32 reserved_header = tti_session->base.model->m->headsize;
            text_buff = llm_malloc(text_len + reserved_header + 1);
            if (text_buff == NULL) {
                llm_err("no memory!\r\n");
                return RET_ERR;
            }
            os_memcpy(text_buff + reserved_header, text, text_len);
            text_buff[text_len + reserved_header] = '\0';
        }
        struct llm_data tti_send = {
            .llm_name = tti_session->base.name,
            .transfer_state = state,
            .buff1 = text_buff,
            .buff1_len = text_buff ? text_len : -1,
            .offset = -1,
            .type   = type,
        };
        RB_INT_SET(&tti_session->mq_tx, tti_send);
        return RET_OK;
    }
    return LLME_AGAIN;
}

int32 llm_tti_recv(void *tti_hdl, char *buff, uint32 buff_size, uint8 flags)
{
    if (!tti_hdl) {
        llm_err("tti session not exist!\r\n");
        return RET_ERR;
    }
    return RET_OK;
}

int32 llm_tti_config(void *tti_hdl, llm_cfg_type type, void *cfg, uint32 cfg_size)
{
    struct llm_session_tti *tti_session = NULL;
    if (!tti_hdl) {
        llm_err("tti session is not exist!\r\n");
        return RET_ERR;
    }

    tti_session = (struct llm_session_tti *)tti_hdl;

    void **target_config = type ? &tti_session->base.transfer_config : &tti_session->base.platform_config;
    if (*target_config != NULL && os_memcmp(*target_config, cfg, cfg_size) == 0) {
        return RET_OK;
    }

    if (llm_copy_config(target_config, cfg, cfg_size) != RET_OK) {
        return RET_ERR;
    }
    if (type == LLM_CONFIG_TYPE_MODEL) {
        tti_session->base.platform_config_change = 1;
    }
    return RET_OK;
}

static int32 llm_tti_ctrl_set_state_with_timeout(struct llm_session_tti *tti_session, llm_tti_state state, uint32 timeout)
{
    int32 ret = RET_OK;
    uint64 start_jiffies = os_jiffies();
    llm_tti_state curr_state = LLM_TTI_STATE_UNKNOWN;
    int32 elapsed_ms = 0;

    ret = llm_tti_set_state(tti_session, state);
    if (ret != RET_OK) {
        llm_err("llm_tti_set_state fail!(%d)\r\n", ret);
        return ret;
    }
    while (1) {
        curr_state = llm_tti_get_state(tti_session);
        if (curr_state != state) {
            ret = (curr_state == LLM_TTI_STATE_IDLE) ? RET_OK : RET_ERR;
            if (ret != RET_OK) {
                llm_err("target state \"%s\" modified to \"%s\"\r\n",
                        llm_tti_state_str(state), llm_tti_state_str(curr_state));
            }
            return ret;
        }

        if (timeout > 0) {
            elapsed_ms = os_jiffies_to_msecs(os_jiffies() - start_jiffies);
            if (elapsed_ms >= timeout) {
                llm_err("target state \"%s\" timeout\"\r\n",
                        llm_tti_state_str(state));
                ret = LLME_WAIT_TIMEOUT;
                break;
            }
        }
        os_sleep_ms(10);
    }
    return ret;
}

int32 llm_tti_ctrl(void *tti_hdl, llm_ctrl_type ctrl_type, uint32 timeout)
{
    int32 ret = RET_OK;
    struct llm_session_tti *tti_session = NULL;
    if (!tti_hdl) {
        llm_err("tti session is not exist!\r\n");
        return RET_ERR;
    }
    tti_session = (struct llm_session_tti *)tti_hdl;

    switch (ctrl_type) {
        case LLM_CTRL_TYPE_INTERRUPT:
            return llm_tti_ctrl_set_state_with_timeout(tti_session, LLM_TTI_STATE_INTERRUPTING, timeout);
        default:
            llm_err("ERROR STATE!(%d)\r\n", ctrl_type);
            return RET_ERR;
    }
    return ret;
}

static int32 llm_tti_sparam_check(struct llm_tti_sparam *sparam)
{
    ASSERT(sparam);

    if (sparam->qmsg_tx_cnt <= 0) {
        sparam->qmsg_tx_cnt = 16;
    }
    return RET_OK;
}

void *llm_tti_init(char *llm_name, struct llm_tti_sparam *sparam)
{
    if (llm_equipment_legality_check()) {
        return NULL;
    }

    int32 ret = RET_OK;
    struct llm_session_tti *tti_session = NULL;
    struct llm_data *tx_q = NULL;
    uint8 platform_inited = 0;

    if (!llm_name || !sparam) {
        llm_err("Please check the parameters!\r\n");
        return NULL;
    }

    if (llm_tti_sparam_check(sparam)) {
        llm_err("Please check the parameters!\r\n");
        return NULL;
    }

    struct llm_model *model = llm_find_model(llm_name);

    if (!model) {
        llm_err("llm [%s] not supported!\r\n", llm_name);
        return NULL;
    }

    tti_session = llm_zalloc(sizeof(struct llm_session_tti));
    if (!tti_session) {
        llm_err("tti_session malloc fail!\r\n");
        return NULL;
    }

    tti_session->base.model    = model;
    tti_session->base.name     = llm_name;
    tti_session->base.type     = LLM_TTI;
    tti_session->base.run      = llm_tti_run_task;
    tti_session->base.evt_cb   = sparam->evt_cb;

    os_mutex_init(&tti_session->lock);

    if (llm_copy_config((void **)&tti_session->sparam, sparam, sizeof(struct llm_tti_sparam)) != RET_OK) {
        llm_err("no memory!\r\n");
        goto cleanup;
    }
    tx_q = llm_malloc((tti_session->sparam->qmsg_tx_cnt + 1) * sizeof(struct llm_data));
    if (!tx_q) {
        llm_err("no memory!\r\n");
        goto cleanup;
    }
    RB_INIT_R(&tti_session->mq_tx, tti_session->sparam->qmsg_tx_cnt, tx_q);

    if (sparam->cb_max_size > 0) {
        tti_session->callback_rx_buff = llm_malloc(sparam->cb_max_size);
        if (!tti_session->callback_rx_buff) {
            llm_err("no buffer!\r\n");
            goto cleanup;
        }
    }

    ret = tti_session->base.model->m->init((void *)tti_session);
    if (ret) {
        llm_err("platform init fail!\r\n");
        goto cleanup;
    }
    platform_inited = 1;
    llm_tti_set_state(tti_session, LLM_TTI_STATE_IDLE);

    ret = llm_add_session((struct llm_session *)tti_session);
    if (ret) {
        llm_err("llm mgr add session fail!\r\n");
        goto cleanup;
    }

    return (void *)tti_session;
cleanup:
    if (platform_inited) { tti_session->base.model->m->deinit((void *)tti_session); }
    if (tti_session) {
        if (tti_session->callback_rx_buff) { llm_free(tti_session->callback_rx_buff); }
        llm_free(tti_session);
    }
    if (tx_q) { llm_free(tx_q); }
    return NULL;
}

int32 llm_tti_deinit(void *tti_hdl)
{
    int32 ret = RET_OK;
    struct llm_session_tti *tti_session = NULL;
    if (!tti_hdl) {
        llm_err("llm session not exist!\r\n");
        return RET_ERR;
    }

    tti_session = (struct llm_session_tti *)tti_hdl;

    if (!tti_session->base.destroying) {
        tti_session->base.exit = 1;
        return RET_OK;
    }

    tti_session->base.model->m->deinit((void *)tti_session);

    ret = llm_del_session((struct llm_session *)tti_hdl);
    if (ret) {
        llm_err("llm mgr remove session fail!\r\n");
        ASSERT(!ret);
    }

    llm_free(tti_session->mq_tx.rbq);
    if (tti_session->callback_rx_buff) { llm_free(tti_session->callback_rx_buff); }
    llm_free(tti_session);
    return RET_OK;
}


