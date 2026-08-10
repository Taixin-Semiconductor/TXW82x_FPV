#ifndef MUSIC_SERVICE_H
#define MUSIC_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

/* ===== 音乐服务数据结构 ===== */

/** 播放状态 */
typedef enum
{
    MUSIC_STATE_STOPPED = 0,
    MUSIC_STATE_PLAYING,
    MUSIC_STATE_PAUSED
} music_state_t;

/** 字符串字段最大长度（含 '\0'） */
#define MUSIC_TITLE_MAX_LEN   32
#define MUSIC_NAME_MAX_LEN    64
#define MUSIC_ARTIST_MAX_LEN  64

/** 歌曲信息（整合所有数据） */
typedef struct
{
    char title[MUSIC_TITLE_MAX_LEN];      /* 歌曲标题 */
    char name[MUSIC_NAME_MAX_LEN];        /* 歌曲名称 */
    char artist[MUSIC_ARTIST_MAX_LEN];    /* 艺术家 */
    music_state_t state;                  /* 播放状态 */
    bool is_playing;                      /* 是否正在播放 */
    uint32_t duration_sec;                /* 歌曲总时长(秒) */
    int32_t progress;                     /* 进度千分比 0-1000 */
    uint8_t volume;                       /* 音量 0-100 */
} music_song_info_t;

/* ===== 公共 API ===== */

/**
 * @brief 初始化音乐服务
 */
void music_service_init(void);

/**
 * @brief 获取歌曲信息（供 UI 直接读取）
 * @return 歌曲信息指针
 */
const music_song_info_t *music_service_get_info(void);

/**
 * @brief 设置歌曲信息
 * @param info 歌曲信息指针
 */
void music_service_set_info(const music_song_info_t *info);

/* ---------- 播放控制 ---------- */

/* 播放音乐 */
void music_service_play(void);

/* 暂停音乐 */
void music_service_pause(void);

/* 停止音乐 */
void music_service_stop(void);

/* 切换到下一曲 */
void music_service_next(void);

/* 切换到上一曲 */
void music_service_prev(void);

#endif /* MUSIC_SERVICE_H */