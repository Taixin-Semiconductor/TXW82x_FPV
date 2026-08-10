#include "llm.h"

const char *llm_sts_state_str(llm_sts_state state)
{
    switch (state) {
        case LLM_STS_STATE_UNKNOWN:
            return "UNKNOWN";
        case LLM_STS_STATE_IDLE:
            return "IDLE";
        case LLM_STS_STATE_CONNECTING:
            return "CONNECTING";
        case LLM_STS_STATE_RECONNECTING:
            return "RECONNECTING";
        case LLM_STS_STATE_WORKING:
            return "WORKING";
        case LLM_STS_STATE_WAITING:
            return "WAITING";
        case LLM_STS_STATE_DIALOGUE:
            return "DIALOGUE";
        case LLM_STS_STATE_INTERRUPTING:
            return "INTERRUPTING";
        default:
            return "INVALID";
    }
    return "INVALID";
}

static const int llm_sts_state_transition[LLM_STS_STATE_MAX][LLM_STS_STATE_MAX] = {
    // 未知状态 → 仅能转到空闲
    [LLM_STS_STATE_UNKNOWN]       = {[LLM_STS_STATE_IDLE] = 1},
    // 空闲 → 打断 / 重连 / 正在连接 / 工作 / 等待进入监听 / 对话
    [LLM_STS_STATE_IDLE]          = {[LLM_STS_STATE_INTERRUPTING] = 1, [LLM_STS_STATE_RECONNECTING] = 1, [LLM_STS_STATE_CONNECTING] = 1, [LLM_STS_STATE_WORKING] = 1, [LLM_STS_STATE_WAITING] = 1, [LLM_STS_STATE_DIALOGUE] = 1},
    // 正在连接 → 空闲 / 重连 / 工作
    [LLM_STS_STATE_CONNECTING]    = {[LLM_STS_STATE_IDLE] = 1, [LLM_STS_STATE_RECONNECTING] = 1, [LLM_STS_STATE_WORKING] = 1},
    // 重连 → 空闲 / 正在连接
    [LLM_STS_STATE_RECONNECTING]  = {[LLM_STS_STATE_IDLE] = 1, [LLM_STS_STATE_CONNECTING] = 1},
    // 工作 → 空闲 / 打断 / 重连 / 等待进入监听 / 对话
    [LLM_STS_STATE_WORKING]       = {[LLM_STS_STATE_IDLE] = 1, [LLM_STS_STATE_INTERRUPTING] = 1, [LLM_STS_STATE_RECONNECTING] = 1, [LLM_STS_STATE_WAITING] = 1, [LLM_STS_STATE_DIALOGUE] = 1},
    // 等待 → 空闲 / 打断 / 重连 / 工作
    [LLM_STS_STATE_WAITING]       = {[LLM_STS_STATE_IDLE] = 1, [LLM_STS_STATE_INTERRUPTING] = 1, [LLM_STS_STATE_RECONNECTING] = 1, [LLM_STS_STATE_WORKING] = 1},
    // 对话 → 空闲 / 打断 / 重连 / 工作
    [LLM_STS_STATE_DIALOGUE]      = {[LLM_STS_STATE_IDLE] = 1, [LLM_STS_STATE_INTERRUPTING] = 1, [LLM_STS_STATE_RECONNECTING] = 1, [LLM_STS_STATE_WORKING] = 1},
    // 打断 → 空闲 / 重连 / 工作
    [LLM_STS_STATE_INTERRUPTING]  = {[LLM_STS_STATE_IDLE] = 1, [LLM_STS_STATE_RECONNECTING] = 1, [LLM_STS_STATE_WORKING] = 1},
    // 其他状态默认0（非法）
};

int32 llm_sts_event(struct llm_session_sts *sts_session, uint16 event, uint32 param1, uint32 param2)
{
    int32 ret = RET_OK;
    int32 ignore = 0;
    switch (event) {
        case LLM_EVENT_CONNECTED:
            if (llm_sts_get_state(sts_session) == LLM_STS_STATE_CONNECTING) {
                sts_session->base.connected = 1;
                sts_session->base.new_dialogue = 1;
                llm_sts_set_state((struct llm_session_sts *)sts_session, LLM_STS_STATE_WORKING);
            }
            break;
        case LLM_EVENT_DISCONNECT:
            if (llm_sts_get_state(sts_session) == LLM_STS_STATE_RECONNECTING) {
                ignore = sts_session->base.connected ? 0 : 1;
                sts_session->base.connected = 0;
                llm_sts_set_state((struct llm_session_sts *)sts_session, LLM_STS_STATE_CONNECTING);
            }
            break;
        case LLM_EVENT_INTERRUPT_END:
            ignore = 1;
            if (llm_sts_get_state(sts_session) == LLM_STS_STATE_INTERRUPTING) {
			    sts_session->base.new_dialogue = 0;
                llm_sts_set_state((struct llm_session_sts *)sts_session, LLM_STS_STATE_WORKING);
            }
            break;
        case LLM_EVENT_WAITING_END:
            ignore = 1;
            if (llm_sts_get_state(sts_session) == LLM_STS_STATE_WAITING) {
                sts_session->base.platform_config_change = 0;
                llm_sts_set_state((struct llm_session_sts *)sts_session, LLM_STS_STATE_WORKING);
            }
            break;
        case LLM_EVENT_DIALOGUE_START:
            if (llm_sts_get_state(sts_session) == LLM_STS_STATE_WORKING) {
                llm_sts_set_state((struct llm_session_sts *)sts_session, LLM_STS_STATE_DIALOGUE);
            }
            break;
        case LLM_EVENT_DIALOGUE_END:
            if (llm_sts_get_state(sts_session) == LLM_STS_STATE_DIALOGUE) {
                llm_sts_set_state((struct llm_session_sts *)sts_session, LLM_STS_STATE_WORKING);
            }
            break;
        case LLM_EVENT_TX_ERR:
        case LLM_EVENT_RX_ERR:
            llm_sts_set_state((struct llm_session_sts *)sts_session, LLM_STS_STATE_IDLE);
            break;
        default:
            break;
    }
    if (ret == RET_OK && !ignore) {
        ret = llm_event_notify((void *)sts_session, event, param1, param2);
    }
    return ret;
}

llm_sts_state llm_sts_get_state(struct llm_session_sts *sts_session)
{
    os_mutex_lock(&sts_session->lock, osWaitForever);
    llm_sts_state state = sts_session->state;
    os_mutex_unlock(&sts_session->lock);
    return state;
}

int32 llm_sts_set_state(struct llm_session_sts *sts_session, llm_sts_state new_state)
{
    int32 ret = RET_OK;
    int32 changed = 0;
    llm_sts_state curr_state = LLM_STS_STATE_UNKNOWN;

    os_mutex_lock(&sts_session->lock, osWaitForever);
    curr_state = sts_session->state;
    if (curr_state == new_state) {
        llm_err("The current status is \"%s\", the status has not changed!\r\n", llm_sts_state_str(curr_state));
        os_mutex_unlock(&sts_session->lock);
        return RET_OK;
    }
    if (!llm_sts_state_transition[curr_state][new_state]) {
        llm_err("invalid state: %s -> %s\r\n", llm_sts_state_str(curr_state), llm_sts_state_str(new_state));
        os_mutex_unlock(&sts_session->lock);
        return RET_ERR;
    }

    switch (new_state) {
        case LLM_STS_STATE_IDLE: {
            changed = 1;
            break;
        }
        case LLM_STS_STATE_CONNECTING: {
            sts_session->base.timeout = 0;
            changed = 1;
            break;
        }
        case LLM_STS_STATE_RECONNECTING: {
            changed = 1;
            break;
        }
        case LLM_STS_STATE_WORKING: {
            if (sts_session->base.connected) {
                changed = 1;
            }
            break;
        }
        case LLM_STS_STATE_WAITING: {
            if (sts_session->base.connected) {
                sts_session->base.timeout = 0;
				sts_session->base.new_dialogue = 1;
                changed = 1;
            }
            break;
        }
        case LLM_STS_STATE_DIALOGUE: {
            if (sts_session->base.connected) {
                sts_session->base.timeout = 0;
                changed = 1;
            }
            break;
        }
        case LLM_STS_STATE_INTERRUPTING: {
            if (sts_session->base.connected) {
                sts_session->need_recycle = 1;
                sts_session->need_cancel = (curr_state == LLM_STS_STATE_DIALOGUE ? 1 : 0);
                changed = 1;
            }
            break;
        }
        default:
            break;
    }
    if (changed) {
        llm_err("New State:%s -> %s\r\n", llm_sts_state_str(curr_state), llm_sts_state_str(new_state));
        sts_session->last_state = curr_state;
        sts_session->state = new_state;
        ret = RET_OK;
    } else {
        llm_err("invalid state: %s -> %s\r\n", llm_sts_state_str(curr_state), llm_sts_state_str(new_state));
        ret = RET_ERR;
    }
    os_mutex_unlock(&sts_session->lock);
    return ret;
}

static int32 llm_sts_recycle(struct llm_session_sts *sts_session, uint8 clean_rx)
{
    struct llm_data clean_buff;

    llm_err("sts_session clean start!\r\n");
    while (RB_INT_GET(&sts_session->sts_tx, clean_buff)) {
        if (clean_buff.buff1) { llm_free(clean_buff.buff1); }
    }
    RB_RESET(&sts_session->sts_tx);

    if (clean_rx) {
        rbuffer_reset(&sts_session->sts_rx);
    }

    if (sts_session->base.unsend.buff1) {
        llm_free(sts_session->base.unsend.buff1);
        sts_session->base.unsend.buff1 = NULL;
    }
    sts_session->base.unsend.buff1_len = 0;

    if (sts_session->callback_rx_buff) {
        os_memset(sts_session->callback_rx_buff, 0, sts_session->sparam->cb_max_size);
    }
    llm_err("sts_session clean done!\r\n");
    return RET_OK;
}

static int32 llm_sts_tx(struct llm_session_sts *sts_session)
{
    int32 ret = LLME_AGAIN;

    if (sts_session->base.unsend.buff1_len == 0) {
        RB_INT_GET(&sts_session->sts_tx, sts_session->base.unsend);
    }
    if (sts_session->base.unsend.buff1_len != 0) {
        if (os_strcasecmp(sts_session->base.unsend.llm_name, sts_session->base.name) == 0) {
            ret = sts_session->base.model->m->send((void *)sts_session, &sts_session->base.unsend);
            if (ret != RET_OK && ret != LLME_AGAIN && ret != LLME_NOMEM) {
                llm_err("data send fail!(%d)\r\n", ret);
            }
        } else {
            if (sts_session->base.unsend.buff1) {
                llm_free(sts_session->base.unsend.buff1);
                sts_session->base.unsend.buff1 = NULL;
            }
            sts_session->base.unsend.buff1_len = 0;
            llm_err("send not supported!\r\n");
        }
    }
    if (ret == LLME_NOMEM) { ret = LLME_AGAIN; }
    return ret;
}

static int32 llm_sts_rx(struct llm_session_sts *sts_session)
{
    int32 ret = LLME_AGAIN;

    ret = sts_session->base.model->m->recv((void *)sts_session, sts_session->callback_rx_buff, sts_session->sparam->cb_max_size);
    if (ret != RET_OK && ret != LLME_AGAIN && ret != LLME_NOMEM) {
        llm_err("data recv fail!(%d)\r\n", ret);
    }
    if (ret == LLME_NOMEM) { ret = LLME_AGAIN; }
    return ret;
}

static inline int32 llm_sts_safe_tx(struct llm_session_sts *sts_session, int *idle)
{
    int32 ret = llm_sts_tx(sts_session);
    if (ret != RET_OK && ret != LLME_AGAIN) {
        llm_err("llm_sts_tx fail!(%d)\r\n", ret);
        llm_sts_event(sts_session, LLM_EVENT_TX_ERR, ret, 0);
        return -1;
    }
    *idle = (ret != LLME_AGAIN) ? 0 : 1;
    return 0;
}

static inline int32 llm_sts_safe_rx(struct llm_session_sts *sts_session, int *idle)
{
    int32 ret = llm_sts_rx(sts_session);
    if (ret != RET_OK && ret != LLME_AGAIN) {
        llm_err("llm_sts_rx fail!(%d)\r\n", ret);
        llm_sts_event(sts_session, LLM_EVENT_RX_ERR, ret, 0);
        return -1;
    }
    *idle = (ret != LLME_AGAIN) ? 0 : 1;
    return 0;
}

static void llm_sts_run_task(struct llm_session *sts_hdl)
{
    int32 ret = RET_ERR;
	uint64 diff = 0;
	uint64 run_jiff = os_jiffies();
    int32 tx_idle = 1;
    int32 rx_idle = 1;
    llm_sts_state curr_state = LLM_STS_STATE_UNKNOWN;
    struct llm_session_sts *sts_session = (struct llm_session_sts *)sts_hdl;

    curr_state = llm_sts_get_state(sts_session);
    switch (curr_state) {
        case LLM_STS_STATE_IDLE:
            break;
        case LLM_STS_STATE_CONNECTING: {
            if (sts_session->base.timeout == 0 ||
                os_jiffies_to_msecs(os_jiffies()) - sts_session->base.timeout > 1000) {
                sts_session->base.model->m->connect((void *)sts_session);
                sts_session->base.timeout = os_jiffies_to_msecs(os_jiffies());
            }
            break;
        }
        case LLM_STS_STATE_RECONNECTING: {
            llm_sts_recycle(sts_session, 0);
            sts_session->base.model->m->recycle((void *)sts_session, 0);
            sts_session->base.model->m->disconnect((void *)sts_session);
            break;
        }
        case LLM_STS_STATE_WORKING: {
            if (llm_sts_safe_tx(sts_session, &tx_idle)) { break; }
            if (llm_sts_safe_rx(sts_session, &rx_idle)) { break; }
            break;
        }
        case LLM_STS_STATE_DIALOGUE: {
            if (sts_session->base.timeout != 0 && 
                os_jiffies_to_msecs(os_jiffies()) - sts_session->base.timeout > 1000) {
                llm_sts_event(sts_session, LLM_EVENT_DIALOGUE_TIMEOUT, 0, 0);
                sts_session->base.timeout = os_jiffies_to_msecs(os_jiffies());
            }
            if (llm_sts_safe_tx(sts_session, &tx_idle)) { break; }
            if (llm_sts_safe_rx(sts_session, &rx_idle)) { break; }
            break;
        }
        case LLM_STS_STATE_INTERRUPTING: {
            if (sts_session->need_recycle) {
                sts_session->need_recycle = 0;
                llm_sts_recycle(sts_session, 1);
                ret = sts_session->base.model->m->recycle((void *)sts_session, sts_session->need_cancel);
                if (ret != RET_OK) {
                    llm_err("platform recycle fail!(%d)\r\n", ret);
                    llm_sts_event(sts_session, LLM_EVENT_RX_ERR, ret, 0);
                    break;
                }
            }
            if (llm_sts_safe_tx(sts_session, &tx_idle)) { break; }
            if (llm_sts_safe_rx(sts_session, &rx_idle)) { break; }
            break;
        }
        case LLM_STS_STATE_WAITING: {
            if (sts_session->base.timeout == 0 ||
                os_jiffies_to_msecs(os_jiffies()) - sts_session->base.timeout > 1000) {
                sts_session->need_update = 1;
                sts_session->base.timeout = os_jiffies_to_msecs(os_jiffies());
            }
            if (sts_session->need_update) {
                if (!RB_FULL(&sts_session->sts_tx)) {
                    struct llm_data start_send = {
                        .llm_name = sts_session->base.name,
                        .transfer_state = LLM_DATA_STATE_START,
                        .buff1 = NULL,
                        .buff1_len = -1,
                        .offset = -1,
                        .type   = LLM_DATA_TYPE_MGMT,
                    };
                    RB_INT_SET(&sts_session->sts_tx, start_send);
                    sts_session->need_update = 0;
                }
            }
            if (llm_sts_safe_tx(sts_session, &tx_idle)) { break; }
            if (llm_sts_safe_rx(sts_session, &rx_idle)) { break; }
            break;
        }
        default:
            llm_err("ERROR STATE: %s!\r\n", llm_sts_state_str(curr_state));
            break;
    }
    // 会话优雅退出
    if (sts_session->base.exit == 1) {
        sts_session->base.destroying = 1;
        llm_sts_deinit((void *)sts_session);
        return;
    }
    // 当tx_idle和rx_idle都为1时，说明没有数据发送和接收，等待10ms
    if (tx_idle && rx_idle) {
        os_sleep_ms(10);
        sts_session->again_cnt = 0;
    } else {
        sts_session->again_cnt++;
        // 当连续繁忙次数超过50次时，等待10ms，避免CPU占用过高
        if (sts_session->again_cnt % 50 == 0) {
            os_sleep_ms(10);
        }
    }
	diff = os_jiffies_to_msecs(os_jiffies() - run_jiff);
	if (diff > 100) {
		llm_err("llm_sts_run_task use %lld ms\r\n", diff);
	}
}

int32 llm_sts_play_audio(void *sts_hdl, const char *audio, uint32 length)
{
    struct llm_session_sts *sts_session = NULL;
    int32 temp = 0;
    uint32 written = 0;

    if (!sts_hdl) {
        llm_err("sts session not exist!\r\n");
        return RET_ERR;
    }

    sts_session = (struct llm_session_sts *)sts_hdl;

    while (written < length) {
        if (llm_sts_get_state(sts_session) == LLM_STS_STATE_INTERRUPTING) {
            return RET_ERR;
        }
        temp = rbuffer_set(&sts_session->sts_rx, (void *)audio + written, length - written);
        if (temp > 0) {
            written += temp;
        } else {
            os_sleep_ms(10);
        }
    }
    return RET_OK;
}

int32 llm_sts_check_audio(void *sts_hdl)
{
    struct llm_session_sts *sts_session = NULL;
    llm_sts_state curr_state = LLM_STS_STATE_UNKNOWN;

    if (!sts_hdl) {
        llm_err("sts session not exist!\r\n");
        return RET_ERR;
    }

    sts_session = (struct llm_session_sts *)sts_hdl;
    curr_state = llm_sts_get_state(sts_session);
    return (curr_state != LLM_STS_STATE_DIALOGUE && RB_EMPTY(&sts_session->sts_rx)) ? 1 : 0;
}

int32 llm_sts_send(void *sts_hdl, llm_data_type type, llm_data_state state,
                   char *data, uint32 data_len)
{
    char *data_buff = NULL;
    struct llm_session_sts *sts_session = NULL;
    llm_sts_state curr_state = LLM_STS_STATE_UNKNOWN;

    if (!sts_hdl) {
        llm_err("sts session is not exist!\r\n");
        return RET_ERR;
    }

    sts_session = (struct llm_session_sts *)sts_hdl;
    curr_state = llm_sts_get_state(sts_session);

    if (sts_session->base.connected == 0) {
        llm_err("Not connected!\r\n");
        return RET_ERR;
    }

    if ((curr_state == LLM_STS_STATE_DIALOGUE ||
         curr_state == LLM_STS_STATE_WORKING)
        && !RB_FULL(&sts_session->sts_tx)) {
        if (data) {
            data_buff = llm_malloc(data_len);
            if (data_buff == NULL) {
                llm_err("no memory!\r\n");
                return RET_ERR;
            }
            os_memcpy(data_buff, data, data_len);
        }
        struct llm_data sts_send = {
            .llm_name = sts_session->base.name,
            .transfer_state = state,
            .buff1 = data_buff,
            .buff1_len = data_buff ? data_len : -1,
            .offset = -1,
            .type   = type,
        };
        RB_INT_SET(&sts_session->sts_tx, sts_send);
        return RET_OK;
    }
    return LLME_AGAIN;
}

int32 llm_sts_recv(void *sts_hdl, char *buff, uint32 buff_size, uint8 flags)
{
    struct llm_session_sts *sts_session = NULL;
    llm_sts_state curr_state = LLM_STS_STATE_UNKNOWN;

    if (!sts_hdl) {
        llm_err("sts session not exist!\r\n");
        return RET_ERR;
    }

    sts_session = (struct llm_session_sts *)sts_hdl;
    curr_state = llm_sts_get_state(sts_session);
    if (curr_state == LLM_STS_STATE_INTERRUPTING) {
        return LLME_AGAIN;
    }

    if (flags == 1 && RB_COUNT(&sts_session->sts_rx) < buff_size) {
        return LLME_AGAIN;
    }

    return rbuffer_get(&sts_session->sts_rx, buff, buff_size);
}

static int32 llm_sts_ctrl_set_state_with_timeout(struct llm_session_sts *sts_session, llm_sts_state state, uint32 timeout)
{
    int32 ret = RET_OK;
    uint64 start_jiffies = os_jiffies();
    llm_sts_state curr_state = LLM_STS_STATE_UNKNOWN;
    int32 elapsed_ms = 0;

    ret = llm_sts_set_state(sts_session, state);
    if (ret != RET_OK) {
        llm_err("llm_sts_set_state fail!(%d)\r\n", ret);
        return ret;
    }
    while (1) {
        curr_state = llm_sts_get_state(sts_session);
        if (curr_state != state) {
            ret = (curr_state == LLM_STS_STATE_WORKING) ? RET_OK : RET_ERR;
            if (ret != RET_OK) {
                llm_err("target state \"%s\" modified to \"%s\"\r\n",
                        llm_sts_state_str(state), llm_sts_state_str(curr_state));
            }
            return ret;
        }

        if (timeout > 0) {
            elapsed_ms = os_jiffies_to_msecs(os_jiffies() - start_jiffies);
            if (elapsed_ms >= timeout) {
                llm_err("target state \"%s\" timeout\"\r\n",
                        llm_sts_state_str(state));
                ret = LLME_WAIT_TIMEOUT;
                break;
            }
        }
        os_sleep_ms(10);
    }
    return ret;
}

int32 llm_sts_ctrl(void *sts_hdl, llm_ctrl_type ctrl_type, uint32 timeout)
{
    int32 ret = RET_OK;
    struct llm_session_sts *sts_session = NULL;
    if (!sts_hdl) {
        llm_err("sts session is not exist!\r\n");
        return RET_ERR;
    }
    sts_session = (struct llm_session_sts *)sts_hdl;

    switch (ctrl_type) {
        case LLM_CTRL_TYPE_RUN: {
            ret = llm_sts_set_state(sts_session, sts_session->last_state);
            if (ret != RET_OK) {
                llm_err("llm_sts_set_state fail!(%d)\r\n", ret);
                llm_sts_set_state(sts_session, LLM_STS_STATE_RECONNECTING);
            }
            break;
        }
        case LLM_CTRL_TYPE_STOP:
            return llm_sts_set_state(sts_session, LLM_STS_STATE_IDLE);
        case LLM_CTRL_TYPE_RECONNECT:
            return llm_sts_set_state(sts_session, LLM_STS_STATE_RECONNECTING);
        case LLM_CTRL_TYPE_INTERRUPT:
            return llm_sts_ctrl_set_state_with_timeout(sts_session, LLM_STS_STATE_INTERRUPTING, timeout);
        case LLM_CTRL_TYPE_WAITING:
            return llm_sts_ctrl_set_state_with_timeout(sts_session, LLM_STS_STATE_WAITING, timeout);
        default:
            llm_err("ERROR STATE!(%d)\r\n", ctrl_type);
            return RET_ERR;
    }
    return RET_OK;
}

int32 llm_sts_config(void *sts_hdl, llm_cfg_type type, void *cfg, uint32 cfg_size)
{
    struct llm_session_sts *sts_session = NULL;
    if (!sts_hdl) {
        llm_err("sts session is not exist!\r\n");
        return RET_ERR;
    }

    sts_session = (struct llm_session_sts *)sts_hdl;

    void **target_config = type ? &sts_session->base.transfer_config : &sts_session->base.platform_config;
    if (*target_config != NULL && os_memcmp(*target_config, cfg, cfg_size) == 0) {
        return RET_OK;
    }

    if (llm_copy_config(target_config, cfg, cfg_size) != RET_OK) {
        return RET_ERR;
    }
    if (type == LLM_CONFIG_TYPE_MODEL) {
        sts_session->base.platform_config_change = 1;
    }
    return RET_OK;
}

static int32 llm_sts_sparam_check(struct llm_sts_sparam *sparam)
{
    ASSERT(sparam);

    if (sparam->qmsg_tx_cnt <= 0) {
        sparam->qmsg_tx_cnt = 16;
    }
    if (sparam->rx_buff_size <= 0) {
        sparam->rx_buff_size = 4096;
    }
    if (sparam->cb_max_size <= 0) {
        sparam->cb_max_size = 2048;
    }
    return RET_OK;
}

void *llm_sts_init(char *llm_name, struct llm_sts_sparam *sparam)
{
    if (llm_equipment_legality_check()) {
        return NULL;
    }
    
    int32 ret = RET_OK;
    struct llm_model *model;
    struct llm_session_sts *sts_session = NULL;
    struct llm_data *tx_buff = NULL;
    char *rx_buff = NULL;
    uint8 platform_inited = 0;

    if (!llm_name || !sparam) {
        llm_err("Please check the parameters!\r\n");
        return NULL;
    }

    if (llm_sts_sparam_check(sparam)) {
        llm_err("Please check the parameters!\r\n");
        return NULL;
    }

    model = llm_find_model(llm_name);
    if (!model) {
        llm_err("llm [%s] not supported!\r\n", llm_name);
        return NULL;
    }

    sts_session = llm_zalloc(sizeof(struct llm_session_sts));
    if (!sts_session) {
        llm_err("sts_session malloc fail!\r\n");
        return NULL;
    }

    sts_session->base.model    = model;
    sts_session->base.name     = llm_name;
    sts_session->base.type     = LLM_STS;
    sts_session->base.run      = llm_sts_run_task;
    sts_session->base.evt_cb   = sparam->evt_cb;

    os_mutex_init(&sts_session->lock);

    if (llm_copy_config((void **)&sts_session->sparam, sparam, sizeof(struct llm_sts_sparam)) != RET_OK) {
        llm_err("no memory!\r\n");
        goto cleanup;
    }

    sts_session->callback_rx_buff = llm_malloc(sparam->cb_max_size);
    if (!sts_session->callback_rx_buff) {
        llm_err("no buffer!\r\n");
        goto cleanup;
    }

    tx_buff = llm_malloc((sts_session->sparam->qmsg_tx_cnt + 1) * sizeof(struct llm_data));
    if (!tx_buff) {
        llm_err("no memory!\r\n");
        goto cleanup;
    }
    RB_INIT_R(&sts_session->sts_tx, sts_session->sparam->qmsg_tx_cnt, tx_buff);

    rx_buff = llm_zalloc(sparam->rx_buff_size);
    if (!rx_buff) {
        llm_err("no memory!\r\n");
        goto cleanup;
    }
    rbuffer_init(&sts_session->sts_rx, sparam->rx_buff_size, rx_buff);

    ret = sts_session->base.model->m->init((void *)sts_session);
    if (ret) {
        llm_err("platform init fail!\r\n");
        goto cleanup;
    }
    platform_inited = 1;
    llm_sts_set_state(sts_session, LLM_STS_STATE_IDLE);
    ret = llm_add_session((struct llm_session *)sts_session);
    if (ret) {
        llm_err("llm mgr add session fail!\r\n");
        goto cleanup;
    }

    return (void *)sts_session;

cleanup:
    if (platform_inited) { sts_session->base.model->m->deinit((void *)sts_session); }
    if (sts_session) {
        if (sts_session->callback_rx_buff) { llm_free(sts_session->callback_rx_buff); }
        llm_free(sts_session);
    }
    if (tx_buff) { llm_free(tx_buff); }
    if (rx_buff) { llm_free(rx_buff); }
    return NULL;
}

int32 llm_sts_deinit(void *sts_hdl)
{
    int32 ret = RET_OK;
    struct llm_session_sts *sts_session = NULL;
    if (!sts_hdl) {
        llm_err("sts session is not exist!\r\n");
        return RET_ERR;
    }

    sts_session = (struct llm_session_sts *)sts_hdl;
    if (!sts_session->base.destroying) {
        sts_session->base.exit = 1;
        return RET_OK;
    }

    sts_session->base.model->m->deinit((void *)sts_session);

    ret = llm_del_session((struct llm_session *)sts_hdl);
    if (ret) {
        llm_err("llm mgr remove session fail!\r\n");
        ASSERT(!ret);
    }

    llm_free(sts_session->sts_tx.rbq);
    llm_free(sts_session->sts_rx.rbq);
    llm_free(sts_session->callback_rx_buff);
    llm_free(sts_session);
    return RET_OK;
}

int32 llm_sts_upload_file(void *sts_hdl, llm_data_type type, llm_data_state state,
                          char *data, uint32 data_len)
{
    int32 ret = RET_OK;
    struct llm_session_sts *sts_session = NULL;
    if (!sts_hdl) {
        llm_err("sts session is not exist!\r\n");
        return RET_ERR;
    }
    sts_session = (struct llm_session_sts *)sts_hdl;
    
    struct llm_data sts_send = {
        .llm_name = sts_session->base.name,
        .transfer_state = state,
        .buff1 = data,
        .buff1_len = data_len,
        .offset = -1,
        .type   = type,
    };

    ret = sts_session->base.model->m->upload((void *)sts_session, &sts_send);
    if (ret != RET_OK) {
        llm_err("upload fail!(%d)\r\n", ret);
    }
    return ret;
}


