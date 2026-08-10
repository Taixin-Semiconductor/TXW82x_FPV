/**
 * @file txmplayer.h
 * @brief 媒体播放器核心接口定义
 *
 * @section intro 模块简介
 * txmplayer 是一个综合性的媒体播放模块，支持音频、视频、图片及字幕的解析与播放。
 * 它具备自动识别媒体容器格式的能力，并能调用相应的解码器模块进行处理。
 * 支持同时播放多路数据流，默认最多支持8个通道。
 *
 * @section usage 使用方式
 * txmplayer 支持两种主要的数据输入与播放模式：
 *   -# **主动读取模式**：通过 txmplayer_open API 指定文件路径或网络 URL，由播放器内部负责数据的读取与解复用。
 *   -# **MSI 被动输入模式**：由调用者提供数据帧缓冲（framebuff），并通过 `msi_output_fb` 接口将数据推送到 txmplayer 进行播放。
 *      使用MSI接口推送数据时，使用 msi_find2("txmplayer", 0, NULL) 获取 txmplayer 通道的 msi组件，按msi使用方法进行开发。
 *   -# txmplayer在运行过程会产生多种event通知应用程序，event定义在sdk/include/lib/common/sysevnt.h中，@ref SYSEVT_MEDIA_SUBEVT.
 */

#ifndef __TX_MPLAYER_H__
#define __TX_MPLAYER_H__

#include "basic_include.h"
#include <hal/vdd.h>
#include "lib/multimedia/framebuff.h"
#include "lib/multimedia/AVContainer.h"

/** @brief 支持的编解码器通道数量 */
#define TXMPLAYER_CODEC_COUNT  (4)

/**
 * @brief 播放器状态枚举
 */
enum TXMPLAYER_STATE {
    TXMPLAYER_STATE_CLOSE,           ///< 关闭状态：播放器未初始化或已停止
    TXMPLAYER_STATE_PAUSE,           ///< 暂停状态：播放已暂停
    TXMPLAYER_STATE_BUFFERING,       ///< 缓冲状态：正在缓冲数据（通常用于网络流）
    TXMPLAYER_STATE_PLAYING,         ///< 播放状态：正常播放中
    TXMPLAYER_STATE_PLAY_END,        ///< 播放结束：文件播放完毕
    TXMPLAYER_STATE_DECODE_ERR,      ///< 解码失败：不再继续解析数据
    TXMPLAYER_STATE_OPEN_FAIL,       ///< 打开失败：网络连接打开超时
    TXMPLAYER_STATE_SEEKING,         ///< 播放状态：正在执行seek
};

/**
 * @brief 音视频同步模式枚举
 */
enum TXMPLAYER_AVSYNC {
    TXMPLAYER_AVSYNC_AUDIO,   ///< 音频主同步：以音频时钟为基准同步视频
    TXMPLAYER_AVSYNC_VIDEO,   ///< 视频主同步：以视频帧率为基准
    TXMPLAYER_AVSYNC_CLOCK,   ///< 外部时钟同步：以系统时钟为基准
};

/**
 * @brief 播放器全局参数配置结构体
 * @note 播放器在初始化时会复制此结构体的数据，调用者无需保持其生命周期。
 */
struct txmplayer_param {
    uint8    fbQ_size;               ///< 帧缓冲（Framebuff）队列深度，网络播放时应加大此队列
    uint8    volume;                 ///< 初始音量 (0-100)
    uint8    mix_mode;               ///< 启动时混音模式, @ref audio_mix_mode_t
    uint8    mix_volume;             ///< 启动时压低音量 - mix_mode=AUDIO_MIX_MODE_DUCKING 时有效
    uint8    smoothness_first;       ///< 平顺优先 - 开始播放时会先缓存再播放
    uint32   netbuf_size;            ///< 网络流缓冲大小 (单位：字节)

    struct msi   *vdd;               ///< 自定义播放器的输出显示设备，未指定时播放器自动查找VDD设备。

    /**
     * @brief 自定义播放器输出组件名称配置。例如demux后的数据不需要送给解码器，输出到自定义的组件。
     * @note 若设为 NULL，则自动查找默认解码器作为输出目标。
     *
     * 数组索引对应的媒体类型：
     * - [0]: 视频 (Video)
     * - [1]: 音频 (Audio)
     * - [2]: 图片 (Picture)
     * - [3]: 字幕 (Subtitle)
     */
    const char *decoder[TXMPLAYER_CODEC_COUNT]; 
};

/**
 * @brief 根据数据头检测媒体容器类型
 *
 * @param[in] data  指向数据缓冲区的指针
 * @param[in] len   数据长度
 * @return 检测到的媒体容器类型 (media_container_type_t)
 */
media_container_type_t detect_container_type(const uint8 *data, uint32 len);

/**
 * @brief 初始化播放器模块
 *
 * @param[in] stack_size  播放器任务栈大小 (单位：字节)。默认值为 1024。
 * @param[in] task_pri    播放器任务优先级。参考 @ref OS_TASK_PRIORITY，默认为 OS_TASK_PRIORITY_NORMAL。
 * @param[in] param       全局参数配置指针。
 *                        - 若为 NULL，则使用系统默认参数 `txmplayer_param_def`：
 *                          static const struct txmplayer_param txmplayer_param_def = {
 *                              .fbQ_size       = 16,
 *                              .volume         = 100,
 *                              .netbuf_size    = 256 * 1024,
 *                              .vdd            = {-1, -1, 0, 0},
 *                          };
 *
 * @return int32
 *        - 0: 成功
 *        - <0: 失败，返回负错误码
 */
int32 txmplayer_init(uint32 stack_size, uint32 task_pri, struct txmplayer_param *param);

/**
 * @brief 反初始化播放器模块
 * @note txmplayer_init可以被多次调用，但是要和txmplayer_deinit成对匹配，才能真正释放所有资源。
 * @return int32 成功返回 0，失败返回负错误码。
 */
int32 txmplayer_deinit(void);

/**
 * @brief 打开文件或 URL 链接进行播放
 *
 * @param[in] file  文件路径字符串或网络 URL 地址。
 * @param[in] param 本次播放的特定参数配置。
 *                  - 若为 NULL，则复用 `txmplayer_init` 时设置的全局参数。
 * @return int32
 *        - >=0: 成功，返回播放器通道句柄 (stream_id)
 *        - <0: 失败，返回负错误码
 */
int32 txmplayer_open(char *file, uint8 preview, struct txmplayer_param *param);

/**
 * @brief 在不关闭播放器通道的情况下重新打开新的文件或链接进行播放。
 *
 * @param[in] stream_id 播放器通道句柄
 * @param[in] file  文件路径字符串或网络 URL 地址。
 * @return int32 成功返回 0，失败返回负错误码。
 */
int32 txmplayer_reopen(int32 stream_id, char *file);

/**
 * @brief 停止播放并关闭指定流
 *
 * @param[in] stream_id 播放器通道句柄
 * @return int32 成功返回 0，失败返回负错误码。
 */
int32 txmplayer_close(int32 stream_id);

/**
 * @brief 控制播放暂停/恢复
 *
 * @param[in] stream_id 播放器通道句柄
 * @param[in] pause     控制标志
 *                      - 1: 暂停播放
 *                      - 0: 恢复播放
 * @return int32 成功返回 0，失败返回负错误码。
 */
int32 txmplayer_pause(int32 stream_id, uint8 pause);

/**
 * @brief 跳转播放到指定时间点 (Seek)
 *
 * @param[in] stream_id 播放器通道句柄
 * @param[in] new_time  目标时间点 (单位：毫秒)
 * @return int32 成功返回 0，失败返回负错误码。
 */
int32 txmplayer_seek(int32 stream_id, uint32 new_time);

/**
 * @brief 设置播放倍速
 *
 * @param[in] stream_id 播放器通道句柄
 * @param[in] speed     播放倍速 (例如：1表示正常，2表示2倍速)
 * @return int32 成功返回 0，失败返回负错误码。
 */
int32 txmplayer_set_speed(int32 stream_id, int32 speed);

/**
 * @brief 设置音量大小
 *
 * @param[in] stream_id 播放器通道句柄
 * @param[in] volume    音量值 (范围: 0-100)
 * @return int32 成功返回 0，失败返回负错误码。
 */
int32 txmplayer_set_volume(int32 stream_id, uint8 volume);

/**
 * @brief 切换 音频/字幕 track
 
 *
 * @param[in] stream_id 播放器通道句柄
 * @param[in] track     目标 track id 
 * @return int32 成功返回 0，失败返回负错误码。
 */
int32 txmplayer_set_track(int32 stream_id, uint32 track_id);

/**
 * @brief 获取当前流的音频编码信息
 *
 * @param[in] stream_id 播放器通道句柄
 * @param[out] info     指向 `txAudioInfo_t` 结构体的指针，用于接收音频信息
 * @return int32 成功返回 0，失败返回负错误码。
 */
int32 txmplayer_get_audio_info(int32 stream_id, txAudioInfo_t *info);

/**
 * @brief 获取当前流的视频编码信息
 *
 * @param[in] stream_id 播放器通道句柄
 * @param[out] info     指向 `txVideoInfo_t` 结构体的指针，用于接收视频信息
 * @return int32 成功返回 0，失败返回负错误码。
 */
int32 txmplayer_get_video_info(int32 stream_id, txVideoInfo_t *info);

/**
 * @brief 通过 txmplayer 通道句柄执行 MSI 命令
 *
 * @param[in] stream_id 播放器通道句柄
 * @param[in] cmd       MSI命令字, @ref enum MSI_CMDs
 * @param[in] param1    命令参数 1
 * @param[in] param2    命令参数 2
 * @return int32 命令执行结果
 */
int32 txmplayer_msi_cmd(int32 stream_id, uint32 cmd, uint32 param1, uint32 param2);

/**
 * @brief 获取当前播放进度和媒体总时长
 *
 * @param[in] stream_id 播放器通道句柄
 * @param[out] tot_time 输出参数，用于返回媒体总时长 (单位：毫秒)。若不需要可填 NULL。
 * @return int32
 *         - >=0: 返回当前的播放时间 (单位：毫秒)
 *         - <0: 出错，返回负值
 */
int32 txmplayer_playtime(int32 stream_id, uint32 *tot_time);

/**
 * @brief 获取当前播放器的状态
 *
 * @param[in] stream_id 播放器通道句柄
 * @return int32 返回当前的 @ref TXMPLAYER_STATE 状态值。
 */
int32 txmplayer_state(int32 stream_id);

#endif /* __TX_MPLAYER_H__ */
