#ifndef _TXSEMI_MEDIA_AUDIO_H_
#define _TXSEMI_MEDIA_AUDIO_H_

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief 音频编码格式（Codec ID）
 *
 * 表示音频流所使用的压缩或未压缩编码格式。
 * 这里定义了常见的编码格式，但是不意味着 芯片/SDK 支持所有列出来的格式
 */
typedef enum {
    AUDIO_CODEC_MP3      = 0,  ///< MPEG-1 Audio Layer III
    AUDIO_CODEC_AAC      = 1,  ///< Advanced Audio Coding (LC/HE-AAC)
    AUDIO_CODEC_OPUS     = 2,  ///< Opus (低延迟、高效率，WebRTC 标准)
    AUDIO_CODEC_FLAC     = 3,  ///< Free Lossless Audio Codec
    AUDIO_CODEC_PCM_S16LE = 4, ///< 16-bit signed PCM, little-endian (WAV 常见)
    AUDIO_CODEC_PCM_S24LE = 5, ///< 24-bit signed PCM, little-endian
    AUDIO_CODEC_VORBIS   = 6,  ///< Vorbis (常用于 Ogg 容器)
    AUDIO_CODEC_AMR_NB   = 7,  ///< Adaptive Multi-Rate Narrowband (移动语音)
    AUDIO_CODEC_WMAV2    = 8,  ///< Windows Media Audio v2
    AUDIO_CODEC_AMR_WB   = 9,
    AUDIO_CODEC_ALAW     = 10,
    AUDIO_CODEC_ULAW     = 11,
    AUDIO_CODEC_AC3      = 12,

    ///////////////////////////////////////
    //添加SDK自定义类型，从0x30开始
    AUDIO_CODEC_DAC_LOOPBACK = 0x30,   //DAC回采的数据
    AUDIO_CODEC_ADC_READ     = 0x31,
    ///////////////////////////////////////
    AUDIO_CODEC_INVALID = 0xff, ///< Unknown
} audio_codec_t;


/**
 * @brief 声道布局类型
 *
 * 使用 32 位无符号整数表示扬声器位置的位掩码。
 * 每个 bit 代表一个标准声道位置（如 bit0 = Front Left）。
 * 足够覆盖消费级及大部分专业音频场景（≤32 声道）。
 */
typedef uint32 audio_channel_layout_t;

// 标准声道布局位掩码定义（兼容 Apple Core Audio / Microsoft WAVEFORMATEXTENSIBLE）
#define AUDIO_CH_LAYOUT_MONO           (1U << 0)                                      ///< 单声道（Front Center）
#define AUDIO_CH_LAYOUT_STEREO         ((1U << 0) | (1U << 1))                       ///< 立体声（Front Left + Front Right）
#define AUDIO_CH_LAYOUT_2POINT1        ((1U << 0) | (1U << 1) | (1U << 2))            ///< 2.1（L + R + LFE）
#define AUDIO_CH_LAYOUT_2_1            ((1U << 0) | (1U << 1) | (1U << 3))            ///< 2/1（L + R + Rear Center）
#define AUDIO_CH_LAYOUT_SURROUND       ((1U << 0) | (1U << 1) | (1U << 2) | (1U << 4)) ///< 3/1（L + R + C + Rear Center）
#define AUDIO_CH_LAYOUT_4POINT0        ((1U << 0) | (1U << 1) | (1U << 2) | (1U << 4)) ///< 4.0（同 SURROUND）
#define AUDIO_CH_LAYOUT_4POINT1        ((1U << 0) | (1U << 1) | (1U << 2) | (1U << 4) | (1U << 5)) ///< 4.1
#define AUDIO_CH_LAYOUT_5POINT0        ((1U << 0) | (1U << 1) | (1U << 2) | (1U << 4) | (1U << 5)) ///< 5.0
#define AUDIO_CH_LAYOUT_5POINT1        ((1U << 0) | (1U << 1) | (1U << 2) | (1U << 3) | (1U << 4) | (1U << 5)) ///< 5.1（L + R + C + LFE + RL + RR）
#define AUDIO_CH_LAYOUT_7POINT1        ((1U << 0) | (1U << 1) | (1U << 2) | (1U << 3) | (1U << 4) | (1U << 5) | (1U << 6) | (1U << 7)) ///< 7.1（含 Side Left/Right）

/**
 * @brief 音频编解码参数信息
 *
 * 描述一个音频流的静态元数据，用于初始化解码器。
 */
typedef struct {
    uint8  codec_id;              ///< 音频编码格式 ID，取值见 audio_codec_t
    uint8  profile;               ///< 编码 Profile（AAC: 1=LC, 2=Main, 5=HE-AAC）
    uint8  channels;              ///< 声道数量（1=mono, 2=stereo, 6=5.1）
    uint8  bits_per_coded_sample; ///< 编码位深度（8/16/24/32；PCM 必填，压缩格式可为 0）

    uint16 frame_size;            ///< 解码后每帧样本数（AAC=1024, Opus=960, MP3=1152）
    uint16 block_align;           ///< PCM 数据块对齐字节数（channels × bits/8）

    uint32 sample_rate;           ///< 采样率（Hz），如 8000, 44100, 48000, 192000
    uint32 bit_rate;              ///< 标称码率（bps），0 表示 VBR

    audio_channel_layout_t ch_layout; ///< 声道布局位掩码，见 AUDIO_CH_LAYOUT_* 宏

    uint8 *extradata;             ///< 带外配置数据指针（如 AAC 的 ASC，Opus 的 OpusHead）
    uint16 extradata_size;        ///< extradata 数据长度（字节），通常 < 1KB
    uint16 _padding;              ///< 填充字段，确保结构体自然对齐（避免跨 cache line）
    char  *reuse_msi_name;         ///< 重复利用msi名称，当不为空时则内部寻找是否已创建过该msi，若有则返回此前已创建过的通道，无则新建
} txAudioInfo_t;

/**
 * @brief 音频混音模式 / 优先级枚举
 *
 * 定义各音频通道的混音策略，用于多流冲突管理与优先级控制。
 */
typedef enum {
    /**
     * @brief 普通混音模式（默认）
     * 行为：正常参与混音，不影响其他通道
     * 适用：背景音乐、常规音效
     */
    AUDIO_MIX_MODE_NORMAL = 0,

    /**
     * @brief 闪避（压低）模式
     * 行为：激活时自动降低其他普通通道音量
     * 适用：语音提示、导航、通话
     */
    AUDIO_MIX_MODE_DUCKING,

    /**
     * @brief 独占模式
     * 行为：暂停/静音所有其他音频通道
     * 适用：通话、重要提示、录音
     */
    AUDIO_MIX_MODE_EXCLUSIVE,

    /**
     * @brief 模式总数（用于数组大小/越界判断）
     */
    AUDIO_MIX_MODE_MAX
} audio_mix_mode_t;

/**
 * @brief 混音模式使用说明（使用流程）
 *
 * @note 模式切换流程：
 *       1. 启动：主动发送命令 MSI_CMD_SET_MIXER_MODE 切换目标混音模式
 *       2. 数据推送
 *       3. 结束：最后一帧数据设置 last=1
 *       4. 混音器处理完最后一帧后，自动恢复默认正常模式
 *
 * @note 作用：实现提示音/通话/导航等场景的自动混音策略控制
 */

struct msi *aenc_get_msi(audio_codec_t type, const char *self_str, txAudioInfo_t *info);

#ifdef __cplusplus
}
#endif

#endif
