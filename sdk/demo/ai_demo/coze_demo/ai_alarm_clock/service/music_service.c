#include "music_service.h"
#include <string.h>
#include <stdio.h>
 
/* ===== 音乐服务统一上下文 ===== */
static music_song_info_t g_song_info;

/* ===== 内部辅助函数 ===== */

/**
 * @brief 根据进度千分比和总时长计算当前播放时间(秒)
 */
static uint32_t calc_current_time(int32_t permille, uint32_t duration)
{
    if (duration == 0 || permille <= 0)
        return 0;
    if (permille >= 1000)
        return duration;
    return (uint32_t)((uint64_t)permille * duration / 1000);
}

/* ===== 公共 API 实现 ===== */

/**
 * @brief 初始化音乐服务
 */
void music_service_init(void)
{
    memset(&g_song_info, 0, sizeof(music_song_info_t));
    strncpy(g_song_info.title,  "Wash Song",   MUSIC_TITLE_MAX_LEN - 1);
    g_song_info.title[MUSIC_TITLE_MAX_LEN - 1] = '\0';
    strncpy(g_song_info.name,   "Song: Wash Song........................................", MUSIC_NAME_MAX_LEN - 1);
    g_song_info.name[MUSIC_NAME_MAX_LEN - 1] = '\0';
    strncpy(g_song_info.artist, "Artist:作者........................................", MUSIC_ARTIST_MAX_LEN - 1);
    g_song_info.artist[MUSIC_ARTIST_MAX_LEN - 1] = '\0';
    g_song_info.state        = MUSIC_STATE_PAUSED;
    g_song_info.is_playing   = false;
    g_song_info.duration_sec = 65;
    g_song_info.progress     = 0;
    g_song_info.volume       = 50;
}

/* 获取歌曲信息 */
const music_song_info_t *music_service_get_info(void)
{
    return &g_song_info;
}

/* 设置歌曲信息 */
void music_service_set_info(const music_song_info_t *info)
{
    if (info != NULL)
    {
        memcpy(&g_song_info, info, sizeof(music_song_info_t));
    }
}

/* ---------- 播放控制(空实现) ---------- */

void music_service_play()
{
    /* TODO: 实现播放逻辑 */
    g_song_info.is_playing = true;
    g_song_info.state = MUSIC_STATE_PLAYING;
}

void music_service_pause()
{
    /* TODO: 实现暂停逻辑 */
    g_song_info.is_playing = false;
    g_song_info.state = MUSIC_STATE_PAUSED;
}

void music_service_stop(void)
{
    /* TODO: 实现停止逻辑 */
    g_song_info.is_playing = false;
    g_song_info.state = MUSIC_STATE_STOPPED;
    g_song_info.progress = 0;
}

void music_service_next(void)
{
    /* TODO: 实现下一曲逻辑 */
}

void music_service_prev(void)
{
    /* TODO: 实现上一曲逻辑 */
}