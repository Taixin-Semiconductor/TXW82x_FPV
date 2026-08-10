#include "neteast_demo.h"

#if NETEAST_DEMO
/* -------------------------- 音频代码 -------------------------- */
/**
 * @brief 等待音频缓冲区为空
 * @param msi 音频播放器句柄
 */
void neteast_audio_wait_empty(void)
{
    if (neteast_mgr.txmplayer_hdl >= 0) {
        txmplayer_pause(neteast_mgr.txmplayer_hdl, 1);
    }
    msi_output_cmd(neteast_mgr.txmplayer_msi, MSI_CMD_STOP, 0, 0);
    while (ausys_da_fifo_is_empty() != 1) {
        os_sleep_ms(5);
    };
    msi_output_cmd(neteast_mgr.txmplayer_msi, MSI_CMD_START, 0, 0);
}

/**
 * @brief 设置音频音量
 * @param msi 音频播放器句柄
 */
void neteast_audio_set_volume(uint8 volume)
{
    if (neteast_mgr.txmplayer_hdl >= 0) {
        txmplayer_set_volume(neteast_mgr.txmplayer_hdl, volume);
    }
    msi_output_cmd(neteast_mgr.txmplayer_msi, MSI_CMD_SET_VOLUME, volume, 0);
}

/**
 * @brief 音频静音
 * @param msi 音频播放器句柄
 */
void neteast_audio_mute(void)
{
    neteast_audio_set_volume(0);
}

/**
 * @brief 音频恢复
 * @param msi 音频播放器句柄
 */
void neteast_audio_restore(void)
{
    neteast_audio_set_volume(neteast_mgr.volume);
}

/*
 * @brief 播放提示音
 * @param audio 音频数据指针
 * @param audio_len 音频数据长度
 * @param frame_len 音频帧长度
*/
void neteast_audio_play_prompt_tone(const char *audio, uint32 audio_len)
{
    //llm_sts_play_audio(neteast_mgr.sts_session, (char *)audio, audio_len);
    os_printf("%s,%d:Not support yet!\n", __FUNCTION__, __LINE__);
}

/*
 * @brief 发送音频数据到服务器
 * @return 发送结果，0表示成功，其他值表示失败
*/
int32 neteast_audio_send_data(void)
{
    int32 ret = RET_OK;
    static uint32 timeout = 0;
    struct framebuff *fb = NULL;

    fb = msi_get_fb(neteast_mgr.neteast_msi, 0);
    if (fb) {
        if (fb->mtype == MEDIA_DATA_AUDIO) {
            ret = llm_sts_send(neteast_mgr.sts_session, LLM_DATA_TYPE_AUDIO, LLM_DATA_STATE_MIDDLE, (char *)fb->data, fb->len);
            if (ret == LLME_AGAIN) {
                timeout++;
                if (timeout == 100) {
                    neteast_main_timeout_warn();
                }
                if (timeout > 500) {
                    timeout = 0;
                    neteast_main_timeout_reset();
                }
            } else if (ret == RET_OK) {
                timeout = 0;
                neteast_mgr.send_audio_cnt++;
            } else {
                timeout = 0;
                os_printf("send audio err!\n");
                neteast_main_set_state(NETEAST_DEMO_STATE_IDLE);
                llm_sts_reconnect(neteast_mgr.sts_session);
            }
        }
        msi_delete_fb(NULL, fb);
        fb = NULL;
    }
    return ret;
}

/**
 * @brief 音频信息结构体
 */
txAudioInfo_t neteast_audio_info = {
    .codec_id = AUDIO_CODEC_PCM_S16LE,
    .channels = 1,
    .sample_rate = NETEAST_AUDIO_DEC_PCM_SAMPLE_RATE,
};

/*
 * @brief 音频播放线程函数
 * @param arg 线程参数指针
 * @return 无
*/
void neteast_audio_play_thread(void)
{
    int32 ret = RET_OK;
    int32 recv_len = 0;
    struct framebuff *fb = NULL;
    char *audio_buf = llm_malloc(NETEAST_AUDIO_DEC_PCM_FRAME_LEN);

    if (!audio_buf) {
        os_printf("audio_buf alloc fail!\r\n");
        goto cleanup;
    }

    while (1) {
        // 留一个给图片
        if (neteast_mgr.neteast_msi->fb_limits.counter < 2) {
            os_sleep_ms(10);
            continue;
        }
        recv_len = llm_sts_recv(neteast_mgr.sts_session, audio_buf, NETEAST_AUDIO_DEC_PCM_FRAME_LEN, 1);
        if (recv_len > 0) {
            fb = msi_alloc_fb(neteast_mgr.neteast_msi, NULL, NULL, recv_len, 0, 0);
            if (fb) {
                os_memcpy(fb->data, audio_buf, recv_len);
                fb->codec_info  = &neteast_audio_info;
                fb->mtype = MEDIA_DATA_AUDIO;
                fb->stype = AUDIO_CODEC_PCM_S16LE;

                while (1) {
                    ret = msi_output_fb(neteast_mgr.neteast_msi, fb, 1);
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
    if (audio_buf) { llm_free(audio_buf); }
}

#endif