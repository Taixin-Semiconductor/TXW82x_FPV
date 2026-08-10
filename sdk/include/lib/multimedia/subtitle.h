#ifndef _TXSEMI_MEDIA_SUBTITLE_H_
#define _TXSEMI_MEDIA_SUBTITLE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 字幕具体格式（隐含文本/图形类型）
 * 这里定义了常见的字幕格式，但是不意味着 芯片/SDK 支持所有列出来的格式
 */
typedef enum {
    // === 文本类字幕 ===
    SUBTITLE_FMT_SRT     = 0,  ///< SubRip (.srt) - 纯文本
    SUBTITLE_FMT_ASS     = 1,  ///< Advanced SubStation Alpha (.ass) - 富文本
    SUBTITLE_FMT_WEBVTT  = 2,  ///< WebVTT (.vtt)
    SUBTITLE_FMT_TTML    = 3,  ///< Timed Text Markup Language

    // === 图形类字幕 ===
    SUBTITLE_FMT_PGS     = 4,  ///< Blu-ray Presentation Graphics Stream
    SUBTITLE_FMT_VOBSUB  = 5,  ///< DVD VobSub (.sub/.idx)
    SUBTITLE_FMT_DVB_SUB = 6,  ///< DVB 图形字幕
    SUBTITLE_FMT_CEA608  = 7,  ///< 模拟电视行间字幕（通常转为文本）

    ///////////////////////////////////////
    //添加SDK自定义类型，从0x30开始

    ///////////////////////////////////////
    SUBTITLE_FMT_INVALID = 0xff, ///< Unknown
} subtitle_format_t;

/**
 * @brief 字幕编解码参数信息
 *
 * 描述字幕流的静态元数据，用于初始化字幕解码器或渲染器。
 */
typedef struct {
    uint32 format;                ///< 具体格式 ID（见 subtitle_format_t）

    uint32 width;                 ///< 图形字幕的参考宽度（如 PGS/VobSub 的视频分辨率）
    uint32 height;                ///< 图形字幕的参考高度
    uint32 extradata_size;        ///< extradata 长度（字节）

    char   language[4];           ///< ISO 639-2 语言代码（"eng", "chi", "jpn"），末尾补 '\0'

    uint8 *extradata;             ///< 带外配置数据（如 ASS 的 [Script Info] 和 [V4+ Styles] 头部）
} txSubtitleInfo_t;

/**
 * @brief 判断字幕格式是否为文本类型（含富文本）
 */
static inline int subtitle_is_text(subtitle_format_t fmt)
{
    return (fmt == SUBTITLE_FMT_SRT ||
            fmt == SUBTITLE_FMT_ASS ||
            fmt == SUBTITLE_FMT_WEBVTT ||
            fmt == SUBTITLE_FMT_TTML ||
            fmt == SUBTITLE_FMT_CEA608); // CEA608 通常转文本
}

/**
 * @brief 判断字幕格式是否为位图类型
 */
static inline int subtitle_is_bitmap(subtitle_format_t fmt)
{
    return (fmt == SUBTITLE_FMT_PGS ||
            fmt == SUBTITLE_FMT_VOBSUB ||
            fmt == SUBTITLE_FMT_DVB_SUB);
}

#ifdef __cplusplus
}
#endif

#endif
