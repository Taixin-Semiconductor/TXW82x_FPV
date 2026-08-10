#include "llm.h"

const char *llm_chat_state_str(llm_chat_state state)
{
    switch (state) {
        case LLM_CHAT_STATE_UNKNOWN:
            return "UNKNOWN";
        case LLM_CHAT_STATE_IDLE:
            return "IDLE";
        case LLM_CHAT_STATE_CONNECTING:
            return "CONNECTING";
        case LLM_CHAT_STATE_INTERRUPTING:
            return "INTERRUPTING";
        default:
            return "INVALID";
    }
    return "INVALID";
}

static void llm_chat_recycle(struct llm_session_chat *chat_session)
{
    struct llm_data clean_buff;

    llm_err("chat_session clean start!\r\n");
    while (RB_INT_GET(&chat_session->mq_tx, clean_buff)) {
        if (clean_buff.buff1) { llm_free(clean_buff.buff1); }
    }
    RB_RESET(&chat_session->mq_tx);
    while (RB_INT_GET(&chat_session->mq_rx, clean_buff)) {
        if (clean_buff.buff1) { llm_free(clean_buff.buff1); }
    }
    RB_RESET(&chat_session->mq_rx);
    if (chat_session->pending_rx.buff1) {
        llm_free(chat_session->pending_rx.buff1);
        os_memset(&chat_session->pending_rx, 0, sizeof(chat_session->pending_rx));
    }

    if (chat_session->base.unsend.buff1) {
        llm_free(chat_session->base.unsend.buff1);
        chat_session->base.unsend.buff1 = NULL;
    }
    chat_session->base.unsend.buff1_len = 0;
    chat_session->base.unsend.transfer_state = 0;

    if (chat_session->callback_rx_buff) {
        os_memset(chat_session->callback_rx_buff, 0, chat_session->sparam->cb_max_size);
    }

    chat_session->base.model->m->recycle((void *)chat_session, 1);
    chat_session->base.model->m->disconnect((void *)chat_session);
    llm_err("chat_session clean done\r\n");
}

int32 llm_chat_event(struct llm_session_chat *chat_session, uint16 event, uint32 param1, uint32 param2)
{
    int32 ret = RET_OK;
    int32 ignore = 0;
    switch (event) {
        case LLM_EVENT_CONNECTED:
            llm_dbg("CONNECTED\r\n");
            ignore = 1;
            chat_session->base.connected = 1;
            llm_chat_set_state((struct llm_session_chat *)chat_session, LLM_CHAT_STATE_IDLE);
            break;
        case LLM_EVENT_DISCONNECT:
            llm_dbg("DISCONNECT\r\n");
            ignore = 1;
            chat_session->base.connected = 0;
            if (llm_chat_get_state(chat_session) != LLM_CHAT_STATE_IDLE) {
                llm_chat_set_state((struct llm_session_chat *)chat_session, LLM_CHAT_STATE_IDLE);
            }
            break;
        case LLM_EVENT_CHAT_RESULT:            
            llm_dbg("CHAT_RESULT\r\n");
            llm_chat_recycle(chat_session);
            break;
        case LLM_EVENT_ERROR_MSG:            
            llm_dbg("ERROR_MSG\r\n");
            llm_chat_recycle(chat_session);
            break;    
        case LLM_EVENT_CONN_ERR:
        case LLM_EVENT_TX_ERR:
        case LLM_EVENT_RX_ERR:
            llm_chat_set_state((struct llm_session_chat *)chat_session, LLM_CHAT_STATE_INTERRUPTING);
            break;
        default:
            break;
    }
    if (ret == RET_OK && !ignore) {
        ret = llm_event_notify((void *)chat_session, event, param1, param2);
    }
    return ret;
}

llm_chat_state llm_chat_get_state(struct llm_session_chat *chat_session)
{
    os_mutex_lock(&chat_session->lock, osWaitForever);
    llm_chat_state state = chat_session->state;
    os_mutex_unlock(&chat_session->lock);
    return state;
}

int32 llm_chat_set_state(struct llm_session_chat *chat_session, llm_chat_state new_state)
{
    int32 ret = RET_OK;
    int32 changed = 0;
    llm_chat_state curr_state = LLM_CHAT_STATE_UNKNOWN;

    os_mutex_lock(&chat_session->lock, osWaitForever);
    curr_state = chat_session->state;
    if (curr_state == new_state) {
        llm_err("The current status is \"%s\", the status has not changed!\r\n", llm_chat_state_str(curr_state));
        os_mutex_unlock(&chat_session->lock);
        return RET_OK;
    }

    switch (new_state) {
        case LLM_CHAT_STATE_IDLE: {
            changed = 1;
            break;
        }
        case LLM_CHAT_STATE_CONNECTING: {
//            chat_session->base.timeout = 0;
            changed = 1;
            break;
        }
        case LLM_CHAT_STATE_INTERRUPTING: {
            chat_session->need_recycle = 1;
            changed = 1;
            break;
        }
        default:
            break;
    }
    if (changed) {
        llm_err("New State:%s -> %s\r\n", llm_chat_state_str(curr_state), llm_chat_state_str(new_state));
        chat_session->state = new_state;
        ret = RET_OK;
    } else {
        llm_err("invalid state: %s -> %s\r\n", llm_chat_state_str(curr_state), llm_chat_state_str(new_state));
        ret = RET_ERR;
    }
    os_mutex_unlock(&chat_session->lock);
    return ret;
}

static int32 llm_chat_tx(struct llm_session_chat *chat_session)
{
    int32 ret = RET_ERR;

    if (chat_session->base.unsend.buff1_len == 0) {
        RB_INT_GET(&chat_session->mq_tx, chat_session->base.unsend);
    }
    if (chat_session->base.unsend.buff1_len != 0) {
        if (os_strcasecmp(chat_session->base.unsend.llm_name, chat_session->base.name) == 0) {
            switch (chat_session->base.unsend.transfer_state) {
                case LLM_DATA_STATE_START:
                    if (chat_session->base.connected == 0) {
                        if (llm_chat_get_state(chat_session) != LLM_CHAT_STATE_CONNECTING) {
                            llm_chat_set_state(chat_session, LLM_CHAT_STATE_CONNECTING);
                        }
                        ret = RET_OK;
                    } else {
                        ret = chat_session->base.model->m->send((void *)chat_session, &chat_session->base.unsend);
                        if (ret != RET_OK && ret != LLME_AGAIN && ret != LLME_NOMEM) {
                            llm_err("data send fail!(%d)\r\n", ret);
                        }
                    }
                    break;
                case LLM_DATA_STATE_MIDDLE:
                case LLM_DATA_STATE_END:
                    if (chat_session->base.connected == 1) {
                        ret = chat_session->base.model->m->send((void *)chat_session, &chat_session->base.unsend);
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
            if (chat_session->base.unsend.buff1) {
                llm_free(chat_session->base.unsend.buff1);
                chat_session->base.unsend.buff1 = NULL;
            }
            chat_session->base.unsend.buff1_len = 0;
            llm_err("send not supported!\r\n");
        }
    } else {
        ret = LLME_AGAIN;
    }
    if (ret == LLME_NOMEM) { ret = LLME_AGAIN; }
    return ret;
}

static int32 llm_chat_rx(struct llm_session_chat *chat_session)
{
    int32 ret = LLME_AGAIN;

    ret = chat_session->base.model->m->recv((void *)chat_session, chat_session->callback_rx_buff, chat_session->sparam->cb_max_size);
    if (ret != RET_OK && ret != LLME_AGAIN && ret != LLME_NOMEM) {
        llm_err("data recv fail!(%d)\r\n", ret);
    }
    if (ret == LLME_NOMEM) { ret = LLME_AGAIN; }
//    if (ret == RET_OK) {chat_session->base.timeout = os_jiffies(); }
    return ret;
}

static inline int32 llm_chat_safe_tx(struct llm_session_chat *chat_session, int *idle)
{
    int32 ret = llm_chat_tx(chat_session);
    if (ret != RET_OK && ret != LLME_AGAIN) {
        llm_err("llm_chat_tx fail!(%d)\r\n", ret);
        llm_chat_event(chat_session, LLM_EVENT_TX_ERR, ret, 0);
        return -1;
    }
    *idle = (ret != LLME_AGAIN) ? 0 : 1;
    return 0;
}

static inline int32 llm_chat_safe_rx(struct llm_session_chat *chat_session, int *idle)
{
    int32 ret = llm_chat_rx(chat_session);
    if (ret != RET_OK && ret != LLME_AGAIN) {
        llm_err("llm_chat_rx fail!(%d)\r\n", ret);
        llm_chat_event(chat_session, LLM_EVENT_RX_ERR, ret, 0);
        return -1;
    }
    *idle = (ret != LLME_AGAIN) ? 0 : 1;
    return 0;
}

static void llm_chat_run_task(struct llm_session *chat_hdl)
{
    int32 ret = RET_ERR;
    uint64 diff = 0;
    uint64 run_jiff = os_jiffies();
    int32 tx_idle = 1;
    int32 rx_idle = 1;
    llm_chat_state curr_state = LLM_CHAT_STATE_UNKNOWN;
    struct llm_session_chat *chat_session = (struct llm_session_chat *)chat_hdl;

    curr_state = llm_chat_get_state(chat_session);
    switch (curr_state) {
        case LLM_CHAT_STATE_IDLE:
            if (llm_chat_safe_tx(chat_session, &tx_idle)) { break; }
            if (chat_session->base.connected == 1) {
                if (llm_chat_safe_rx(chat_session, &rx_idle)) { break; }
            }
            break;
        case LLM_CHAT_STATE_CONNECTING: {
            ret = chat_session->base.model->m->connect((void *)chat_session);
            if (ret == LLME_AGAIN || ret == LLME_NOMEM) { tx_idle = rx_idle = 1; }
            break;
        }
        case LLM_CHAT_STATE_INTERRUPTING: {
            if (chat_session->need_recycle) {
                chat_session->need_recycle = 0;
                llm_chat_recycle(chat_session);
            }
            break;
        }
        default:
            llm_err("ERROR STATE: %s!\r\n", llm_chat_state_str(curr_state));
            break;
    }
    // 会话优雅退出
    if (chat_session->base.exit == 1) {
        chat_session->base.destroying = 1;
        llm_chat_deinit((void *)chat_session);
        return;
    }
    // 当tx_idle和rx_idle都为1时，说明没有数据发送和接收，等待10ms
    if (tx_idle && rx_idle) {
        os_sleep_ms(10);
        chat_session->again_cnt = 0;
    } else {
        chat_session->again_cnt++;
        // 当连续繁忙次数超过50次时，等待10ms，避免CPU占用过高
        if (chat_session->again_cnt % 50 == 0) {
            os_sleep_ms(10);
        }
    }
    diff = os_jiffies_to_msecs(os_jiffies() - run_jiff);
    if (diff > 100) {
        llm_err("llm_chat_run_task use %lld ms\r\n", diff);
    }
}

int32 llm_chat_set_role(void *chat_hdl, char *role)
{
    struct llm_session_chat *chat_session = NULL;

    if (!chat_hdl) {
        llm_err("chat session is not exist!\r\n");
        return RET_ERR;
    }

    if (!role) {
        llm_err("role is not exist!\r\n");
        return RET_ERR;
    }

    chat_session = (struct llm_session_chat *)chat_hdl;
    chat_session->role = role;
    return RET_OK;
}

int32 llm_chat_upload_file(void *chat_hdl, llm_data_type data_type, uint32 param, 
                                char *data, uint32 data_len)
{
    int32 ret = RET_OK;
    struct llm_session_chat *chat_session = NULL;
    if (!chat_hdl) {
        llm_err("chat session is not exist!\r\n");
        return RET_ERR;
    }
    chat_session = (struct llm_session_chat *)chat_hdl;
    
    struct llm_data chat_send = {
        .llm_name   = chat_session->base.name,
        .buff1      = data,
        .buff1_len  = data_len,
        .offset     = -1,
        .type       = data_type,
        .sub_type   = param,
    };

    ret = chat_session->base.model->m->upload((void *)chat_session, &chat_send);
    if (ret != RET_OK) {
        llm_err("upload fail!(%d)\r\n", ret);
    }
    return ret;
}

int32 llm_chat_send(void *chat_hdl, llm_data_type type, llm_data_state state, char *text, uint32 text_len)
{
    char *text_buff = NULL;
    struct llm_session_chat *chat_session = NULL;
    llm_chat_state curr_state = LLM_CHAT_STATE_UNKNOWN;

    if (!chat_hdl) {
        llm_err("chat session is not exist!\r\n");
        return RET_ERR;
    }

    chat_session = (struct llm_session_chat *)chat_hdl;
    curr_state = llm_chat_get_state(chat_session);

    if (curr_state == LLM_CHAT_STATE_IDLE &&
        !RB_FULL(&chat_session->mq_tx)) {
        if (text) {
            int32 reserved_header = chat_session->base.model->m->headsize;
            text_buff = llm_malloc(text_len + reserved_header + 1);
            if (text_buff == NULL) {
                llm_err("no memory!\r\n");
                return RET_ERR;
            }
            os_memcpy(text_buff + reserved_header, text, text_len);
            text_buff[text_len + reserved_header] = '\0';
        }
        struct llm_data chat_send = {
            .llm_name = chat_session->base.name,
            .transfer_state = state,
            .buff1 = text_buff,
            .buff1_len = text_buff ? text_len : -1,
            .offset = -1,
            .type   = type,
        };
        RB_INT_SET(&chat_session->mq_tx, chat_send);
        return RET_OK;
    }
    return LLME_AGAIN;
}

int32 llm_chat_recv(void *chat_hdl, char *buff, uint32 buff_size, uint8 flags)
{
    struct llm_session_chat *chat_session = NULL;
    struct llm_data ll_file = {0};

    if (!chat_hdl) {
        llm_err("session not exist!\r\n");
        return RET_ERR;
    }
    
    chat_session = (struct llm_session_chat *)chat_hdl;

    if (chat_session->pending_rx.buff1) {
        ll_file = chat_session->pending_rx;
        os_memset(&chat_session->pending_rx, 0, sizeof(chat_session->pending_rx));
    } else if (!RB_INT_GET(&chat_session->mq_rx, ll_file)) {
        return RET_ERR;
    }

    if (os_strcasecmp(ll_file.llm_name, chat_session->base.name) == 0) {
        if (buff != NULL && buff_size >= (ll_file.buff1_len + 1)) {
            os_memcpy(buff, ll_file.buff1, ll_file.buff1_len + 1);
            llm_free(ll_file.buff1);
            return ll_file.buff1_len;
        }
        chat_session->pending_rx = ll_file;
        return LLME_NOMEM;
    }

    llm_free(ll_file.buff1);
    llm_err("Not Supported!\r\n");
    return RET_ERR;
}

int32 llm_chat_config(void *chat_hdl, llm_cfg_type type, void *cfg, uint32 cfg_size)
{
    struct llm_session_chat *chat_session = NULL;
    if (!chat_hdl) {
        llm_err("chat session is not exist!\r\n");
        return RET_ERR;
    }

    chat_session = (struct llm_session_chat *)chat_hdl;

    void **target_config = type ? &chat_session->base.transfer_config : &chat_session->base.platform_config;
    if (*target_config != NULL && os_memcmp(*target_config, cfg, cfg_size) == 0) {
        return RET_OK;
    }

    if (llm_copy_config(target_config, cfg, cfg_size) != RET_OK) {
        return RET_ERR;
    }
    if (type == LLM_CONFIG_TYPE_MODEL) {
        chat_session->base.platform_config_change = 1;
    }
    return RET_OK;
}

static int32 llm_chat_ctrl_set_state_with_timeout(struct llm_session_chat *chat_session, llm_chat_state state, uint32 timeout)
{
    int32 ret = RET_OK;
    uint64 start_jiffies = os_jiffies();
    llm_chat_state curr_state = LLM_CHAT_STATE_UNKNOWN;
    int32 elapsed_ms = 0;

    ret = llm_chat_set_state(chat_session, state);
    if (ret != RET_OK) {
        llm_err("llm_chat_set_state fail!(%d)\r\n", ret);
        return ret;
    }
    while (1) {
        curr_state = llm_chat_get_state(chat_session);
        if (curr_state != state) {
            ret = (curr_state == LLM_CHAT_STATE_IDLE) ? RET_OK : RET_ERR;
            if (ret != RET_OK) {
                llm_err("target state \"%s\" modified to \"%s\"\r\n",
                        llm_chat_state_str(state), llm_chat_state_str(curr_state));
            }
            return ret;
        }

        if (timeout > 0) {
            elapsed_ms = os_jiffies_to_msecs(os_jiffies() - start_jiffies);
            if (elapsed_ms >= timeout) {
                llm_err("target state \"%s\" timeout\"\r\n",
                        llm_chat_state_str(state));
                ret = LLME_WAIT_TIMEOUT;
                break;
            }
        }
        os_sleep_ms(10);
    }
    return ret;
}

int32 llm_chat_ctrl(void *chat_hdl, llm_ctrl_type ctrl_type, uint32 timeout)
{
    int32 ret = RET_OK;
    struct llm_session_chat *chat_session = NULL;
    if (!chat_hdl) {
        llm_err("chat session is not exist!\r\n");
        return RET_ERR;
    }
    chat_session = (struct llm_session_chat *)chat_hdl;

    switch (ctrl_type) {
        case LLM_CTRL_TYPE_INTERRUPT:
            return llm_chat_ctrl_set_state_with_timeout(chat_session, LLM_CHAT_STATE_INTERRUPTING, timeout);
        default:
            llm_err("ERROR STATE!(%d)\r\n", ctrl_type);
            return RET_ERR;
    }
    return ret;
}

static int32 llm_chat_sparam_check(struct llm_chat_sparam *sparam)
{
    ASSERT(sparam);

    if (sparam->qmsg_tx_cnt <= 0) {
        sparam->qmsg_tx_cnt = 16;
    }
    if (sparam->qmsg_rx_cnt <= 0) {
        sparam->qmsg_rx_cnt = 16;
    }
    if (sparam->image_max_cnt <= 0) {
        sparam->image_max_cnt = 8;
    }
    if (sparam->reply_fragment_size <= 0) {
        sparam->reply_fragment_size = 3 * 50 + 10;
    }
    return RET_OK;
}

void *llm_chat_init(char *llm_name, struct llm_chat_sparam *sparam)
{
    if (llm_equipment_legality_check()) {
        return NULL;
    }

    int32 ret = RET_OK;
    struct llm_session_chat *chat_session = NULL;
    struct llm_data *tx_q = NULL;
    struct llm_data *rx_q = NULL;
    uint8 platform_inited = 0;

    if (!llm_name || !sparam) {
        llm_err("Please check the parameters!\r\n");
        return NULL;
    }

    if (llm_chat_sparam_check(sparam)) {
        llm_err("Please check the parameters!\r\n");
        return NULL;
    }

    struct llm_model *model = llm_find_model(llm_name);

    if (!model) {
        llm_err("llm [%s] not supported!\r\n", llm_name);
        return NULL;
    }

    chat_session = llm_zalloc(sizeof(struct llm_session_chat));
    if (!chat_session) {
        llm_err("chat_session malloc fail!\r\n");
        return NULL;
    }

    chat_session->base.model    = model;
    chat_session->base.name     = llm_name;
    chat_session->base.type     = LLM_CHAT;
    chat_session->base.run      = llm_chat_run_task;
    chat_session->base.evt_cb   = sparam->evt_cb;

    os_mutex_init(&chat_session->lock);

    if (llm_copy_config((void **)&chat_session->sparam, sparam, sizeof(struct llm_chat_sparam)) != RET_OK) {
        llm_err("no memory!\r\n");
        goto cleanup;
    }
    tx_q = llm_malloc((chat_session->sparam->qmsg_tx_cnt + 1) * sizeof(struct llm_data));
    if (!tx_q) {
        llm_err("no memory!\r\n");
        goto cleanup;
    }
    RB_INIT_R(&chat_session->mq_tx, chat_session->sparam->qmsg_tx_cnt, tx_q);
    rx_q = llm_malloc((chat_session->sparam->qmsg_rx_cnt + 1) * sizeof(struct llm_data));
    if (!rx_q) {
        llm_err("no memory!\r\n");
        goto cleanup;
    }
    RB_INIT_R(&chat_session->mq_rx, chat_session->sparam->qmsg_rx_cnt, rx_q);

    if (sparam->cb_max_size > 0) {
        chat_session->callback_rx_buff = llm_malloc(sparam->cb_max_size);
        if (!chat_session->callback_rx_buff) {
            llm_err("no buffer!\r\n");
            goto cleanup;
        }
    }

    ret = chat_session->base.model->m->init((void *)chat_session);
    if (ret) {
        llm_err("platform init fail!\r\n");
        goto cleanup;
    }
    platform_inited = 1;
    llm_chat_set_state(chat_session, LLM_CHAT_STATE_IDLE);

    ret = llm_add_session((struct llm_session *)chat_session);
    if (ret) {
        llm_err("llm mgr add session fail!\r\n");
        goto cleanup;
    }

    return (void *)chat_session;
cleanup:
    if (platform_inited) { chat_session->base.model->m->deinit((void *)chat_session); }
    if (chat_session) {
        if (chat_session->callback_rx_buff) { llm_free(chat_session->callback_rx_buff); }
        llm_free(chat_session);
    }
    if (tx_q) { llm_free(tx_q); }
    if (rx_q) { llm_free(rx_q); }
    return NULL;
}

int32 llm_chat_deinit(void *chat_hdl)
{
    int32 ret = RET_OK;
    struct llm_session_chat *chat_session = NULL;
    if (!chat_hdl) {
        llm_err("llm session not exist!\r\n");
        return RET_ERR;
    }

    chat_session = (struct llm_session_chat *)chat_hdl;

    if (!chat_session->base.destroying) {
        chat_session->base.exit = 1;
        return RET_OK;
    }

    chat_session->base.model->m->deinit((void *)chat_session);

    ret = llm_del_session((struct llm_session *)chat_hdl);
    if (ret) {
        llm_err("llm mgr remove session fail!\r\n");
        ASSERT(!ret);
    }

    llm_free(chat_session->mq_tx.rbq);
    llm_free(chat_session->mq_rx.rbq);
    if (chat_session->pending_rx.buff1) { llm_free(chat_session->pending_rx.buff1); }
    if (chat_session->callback_rx_buff) { llm_free(chat_session->callback_rx_buff); }
    llm_free(chat_session);
    return RET_OK;
}


