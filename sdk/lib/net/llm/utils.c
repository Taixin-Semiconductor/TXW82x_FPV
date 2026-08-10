#include "llm.h"

// 判断字符是否为句子结束符
bool llm_is_sentence_end(const char *str)
{
    // 中文标点符号：句号、问号、感叹号、逗号、分号
    const char *punctuations[] = {".", "?", "!", ":", "。", "？", "！", "，", "；", "：", "、", "\""};
    for (size_t i = 0; i < sizeof(punctuations) / sizeof(punctuations[0]); ++i) {
        if (os_strcmp(str, punctuations[i]) == 0) {
            return true;
        }
    }
    return false;
}

// 删除多余的字符
void llm_remove_spaces_and_newlines(char *str)
{
    int i = 0, j = 0;
    while (str[i]) {
        // 如果当前字符不是空格或换行符，则保留
        if (str[i] != ' ' && str[i] != '\n' && str[i] != '\r' && str[i] != '\t') {
            str[j++] = str[i];
        }
        i++;
    }
    // 添加字符串结束符
    str[j] = '\0';
}

uint32 llm_copy_config(void **old, void *new, uint32 size)
{
    if (new == NULL || size == 0) {
        return RET_ERR;
    }

    if (*old) {
        llm_free(*old);
        *old = NULL;
    }

    *old = llm_malloc(size);
    if (*old) {
        os_memcpy(*old, new, size);
        return RET_OK;
    }
    return RET_ERR;
}

int32 llm_equipment_legality_check(void)
{
	//uint8 value = get_chip_pack();
    //if (value == 0x7 || value == 0xF7) {        //PACK_KL908
    //    return RET_ERR;
    //}
	return RET_OK;
}

int32 llm_event_notify(struct llm_session *session, uint16 event, uint32 param1, uint32 param2)
{
    int32 ret = RET_OK;
    if (session->evt_cb) {
        uint64 jiff = os_jiffies();
        ret = session->evt_cb((void *)session, event, param1, param2);
        jiff = DIFF_JIFFIES(jiff, os_jiffies());
        if (jiff > 10) {
            llm_err("EVTCB hdl %p took %d ticks! event id = %d.\r\n", session->evt_cb, jiff, event);
        }
    }
    return ret;
}

