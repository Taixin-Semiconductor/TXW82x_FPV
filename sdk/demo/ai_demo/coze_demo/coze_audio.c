#include "coze_demo.h"

#if COZE_DEMO
/* -------------------------- 音频代码 -------------------------- */
/**
 * @brief 等待音频缓冲区为空
 * @param msi 音频播放器句柄
 */
void coze_audio_wait_empty(void)
{
    txmplayer_pause(coze_mgr.audio_url_hdl, 1);
    msi_cmd2(coze_mgr.audio_msi, MSI_CMD_STOP, 0, 0);
    while (ausys_da_fifo_is_empty() != 1) {
        os_sleep_ms(5);
    };
    msi_cmd2(coze_mgr.audio_msi, MSI_CMD_START, 0, 0);
}

/**
 * @brief 设置音频音量
 * @param msi 音频播放器句柄
 */
void coze_audio_set_volume(uint8 volume)
{
    txmplayer_set_volume(coze_mgr.audio_url_hdl, volume);
    msi_output_cmd(coze_mgr.audio_msi, MSI_CMD_SET_VOLUME, volume, 0);
}

/**
 * @brief 音频静音
 * @param msi 音频播放器句柄
 */
void coze_audio_mute(void)
{
    coze_audio_set_volume(0);
}

/**
 * @brief 音频恢复
 * @param msi 音频播放器句柄
 */
void coze_audio_restore(void)
{
    coze_audio_set_volume(coze_mgr.volume);
}

/*
 * @brief 播放提示音
 * @param audio 音频数据指针
 * @param audio_len 音频数据长度
 * @param frame_len 音频帧长度
*/
void coze_audio_play_prompt_tone(const char *audio, uint32 audio_len)
{
    txmplayer_pause(coze_mgr.audio_url_hdl, 1);
    msi_cmd2(coze_mgr.audio_msi, MSI_CMD_START, 0, 0);
    llm_sts_play_audio(coze_mgr.sts_session, (char *)audio, audio_len);
}

/*
 * @brief 发送音频数据到服务器
 * @return 发送结果，0表示成功，其他值表示失败
*/
int32 coze_audio_send_data(void)
{
    int32 ret = RET_OK;
    static uint32 timeout = 0;
    struct framebuff *fb = NULL;

    fb = msi_get_fb(coze_mgr.coze_msi, 0);
    if (fb) {
        if (fb->mtype == MEDIA_DATA_AUDIO) {
            ret = llm_sts_send(coze_mgr.sts_session, LLM_DATA_TYPE_AUDIO, LLM_DATA_STATE_MIDDLE, (char *)fb->data, fb->len);
            coze_dbg("send audio len:%d\r\n", fb->len);
            if (ret == LLME_AGAIN) {
                timeout++;
                if (timeout == 100) {
                    coze_main_timeout_warn();
                }
                if (timeout > 500) {
                    timeout = 0;
                    coze_main_timeout_reset();
                }
            } else if (ret == RET_OK) {
                timeout = 0;
                coze_mgr.send_audio_cnt++;
            } else {
                timeout = 0;
                coze_err("send audio err!\n");
                coze_main_set_state(COZE_DEMO_STATE_IDLE);
                llm_sts_reconnect(coze_mgr.sts_session);
            }
        }
        msi_delete_fb(NULL, fb);
        fb = NULL;
    }
    return ret;
}

/*
 * @brief 音频播放线程函数
 * @param arg 线程参数指针
 * @return 无
*/
void coze_audio_play_thread(void)
{
    int32 ret = RET_OK;
    int32 recv_len = 0;
    struct framebuff *fb = NULL;
    char *audio_buf = llm_malloc(COZE_AUDIO_DEC_OPUS_FRAME_LEN);

    if (!audio_buf) {
        coze_err("audio_buf alloc fail!\r\n");
        goto cleanup;
    }

    while (1) {
        // 留一个给图片
        if (coze_mgr.coze_msi->fb_limits.counter < 2) {
            os_sleep_ms(10);
            continue;
        }

        recv_len = llm_sts_recv(coze_mgr.sts_session, audio_buf, COZE_AUDIO_DEC_OPUS_FRAME_LEN, 1);
        if (recv_len == COZE_AUDIO_DEC_OPUS_FRAME_LEN) {
            fb = msi_alloc_fb(coze_mgr.coze_msi, NULL, NULL, COZE_AUDIO_DEC_OPUS_FRAME_LEN, 0, 0);
            if (fb) {
                os_memcpy(fb->data, audio_buf, COZE_AUDIO_DEC_OPUS_FRAME_LEN);
                //sys_dcache_clean_range_unaligned((uint32_t *)fb->data, COZE_AUDIO_DEC_OPUS_FRAME_LEN);
                fb->codec_info = &coze_audio_info;
                fb->mtype = MEDIA_DATA_AUDIO;
                fb->stype = AUDIO_CODEC_OPUS;

                while (1) {
                    ret = msi_output_fb(coze_mgr.coze_msi, fb, 1);
                    if (ret > 0) {
                        break;
                    } else {
                        os_sleep_ms(10);
                    }
                }
            }
        } else {
            if (recv_len != LLME_AGAIN) {
                coze_err("audio recv fail, ret=%d\r\n", recv_len);
            }
            os_sleep_ms(10);
        }
    }

cleanup:
    if (audio_buf) { llm_free(audio_buf); }
}
#endif
