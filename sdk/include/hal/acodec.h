#ifndef _HAL_ACODEC_H_
#define _HAL_ACODEC_H_

#include "lib/multimedia/framebuff.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Audio Codec 抽象层定义
 *   audio codec 需要支持双核异步调用，由于提前无法预知 [编码后] 或 [解码后] 的数据大小，
 *   同时避免 跨CPU 的内存分配操作，acodec_decode_req 和 acodec_encode_req 的输出buffer不能使用framebuff，
 *   当编码/解码完成后需要copy一次数据。
 *   异步编解码：
 *     仅在优化CPU负载，跨CPU执行编解码时启用，     例如 mp3解码库运行在CPU1，CPU0需要解码数据时就可以使用异步调用。
 *     跨CPU执行编解码时，也可以不启用异步调用，不过当需要多个audio coder同时运行时就不能充分发挥多核CPU的优势，
 *     原因是为了节省RAM资源，所有的audio coder共享了一个workqueue运行，同步执行时CPU0会处于waitting状态，不能执行其他的coder。
 * ============================================================================ */

/**
 * @brief acodec 中断函数定义
 * @param irq 中断号
 * @param irq_data 中断携带数据
 * @param param1 用户参数1
 * @param param2 用户参数2
 * @return 0=处理完成，<0=错误
 */
typedef int32 (*acodec_irq_hdl)(uint32 irq, uint32 irq_data, uint32 param1, uint32 param2);

/**
 * @brief 帧处理标志位 (Bitmask)
 */
#define ADEC_FLAG_NONE      0x00  ///< 无特殊标志
#define ADEC_FLAG_KEYFRAME  0x01  ///< [编码] 强制当前帧为关键帧 (I 帧)。用于 seek 起点或流开始。
#define ADEC_FLAG_DISCARD   0x02  ///< [解码] 提示此帧非关键且可丢弃。用于网络拥塞时跳过非重要帧。
#define ADEC_FLAG_PLC       0x04  ///< [解码] 触发丢包隐藏 (Packet Loss Concealment)。当输入数据丢失时，生成插值音频。
#define ADEC_FLAG_EOS       0x08  ///< [通用] 流结束 (End Of Stream)。通常配合 in_fb->data_size = 0 使用，通知驱动刷新内部缓冲。
#define ADEC_FLAG_FLUSH     0x10  ///< [解码] 强制清空内部缓冲并重置状态机 (用于 Seek 操作)。


/**
 * @brief 解码动态控制与结果
 * 
 * 每帧调用 acodec_decode() 时传入/传出。
 * [核心机制]: 
 *   - actual_* : 驱动解析出的**真实流格式** (Output).
 *   - target_* : 用户请求的**重采样/重映射** (Input, 0=无请求).
 * 
 */
struct acodec_decode_info {
    uint32   actual_sample_rate;  ///< [输出] 当前流的真实采样率 (Hz).
    uint32   target_sample_rate;  ///< [输入] 请求重采样率 (0=原始).
    uint16_t actual_channels;     ///< [输出] 当前流的真实声道数.
    uint8_t  target_channels;     ///< [输入] 请求重映射声道 (0=原始).
    uint8_t  flags;               ///< [输入] 控制标志 (PLC, EOS, FLUSH).
};

/**
 * @brief 解码任务请求
 */
struct acodec_decode_req {
    void  *chan;                    ///< 编码器通道句柄.
    void  *priv;                    ///< [用户] 私有上下文指针。回调时原样带回。
    struct acodec_decode_info info; ///< [核心] 动态控制与结果.
    struct framebuff *fb;           ///< [输入] 压缩音频码流.
    uint8 *outbuf;                  ///< 提交请求时
									///< - outbuf!=NULL: 表示使用调用者提供的 buffer 存放解码后的数据。
									///<                 此时outbuf_len也需要赋值，表示buffer的大小。 
									///<                 解码完成后，解码器需要修改outbuf_len为解码后的数据长度
									///< - outbuff=NULL: 表示使用解码器内部的buffer存放解码后的数据，解码完成后对outbuf进行赋值，
									///<                  由调用者从outbuf复制走数据。 
	                                ///< 根据方案内存使用情况来决定调用者是否给 outbuf 赋值。

    uint32 outbuf_len;              ///< 解码后的数据长度

    /**
     * @brief 异步完成回调
     * @param req 指向本请求结构体的指针
     * @param status 最终状态 (0=成功, <0=失败, >0=提示信息如 FORMAT_CHANGED).
     *               若 status 指示格式变化，请检查 req->info.actual_* 字段。
     */
    void (*done)(struct acodec_decode_req *req, int32 status);
};

/**
 * @brief 编码动态控制与结果
 * 
 * 每帧调用 acodec_encode() 时传入/传出。
 */
struct acodec_encode_info {
    int16    target_bitrate;      ///< [输入] 目标码率 (kbps).
                                  ///   - >0 : 立即调整.
                                  ///   - -1 : 保持上一帧 (推荐).
                                  ///   - 0  : 重置为初始值.
    uint16   actual_sample_rate;  ///< [输入] 当前流的真实采样率 (Hz).  
    uint8    actual_channels;     ///< [输入] 当前流的真实声道数.
    uint8    need_more;       ///< [输出] 是否需要传入新的数据.
    uint16   frame_size;          ///< [输入] 编码每帧的样本数
};

/**
 * @brief 编码任务请求
 */
struct acodec_encode_req {
    void  *chan;                    ///< 编码器通道句柄.
    void *priv;                     ///< [用户] 私有上下文指针。
    struct acodec_encode_info info; ///< [核心] 动态控制参数.
    struct framebuff *fb;           ///< [输入] PCM 数据.
    uint8 *outbuf;                  ///< 提交请求时
									///< - outbuf!=NULL: 表示使用调用者提供的 buffer 存放解码后的数据。
									///<                 此时outbuf_len也需要赋值，表示buffer的大小。 
									///<                 解码完成后，解码器需要修改outbuf_len为解码后的数据长度
									///< - outbuff=NULL: 表示使用解码器内部的buffer存放解码后的数据，解码完成后对outbuf进行赋值，
									///<                  由调用者从outbuf复制走数据。 
	                                ///< 根据方案内存使用情况来决定调用者是否给 outbuf 赋值。

    uint32 outbuf_len;              ///< 解码后的数据长度
    /**
     * @brief 异步完成回调
     * @param req 指向本请求结构体的指针
     * @param status 最终状态 (0=成功, <0=失败).
     */
    void (*done)(struct acodec_encode_req *req, int32 status);
};

/**
 * @brief IOCTL 控制命令
 */
enum acodec_ioctl_cmd {
    ADEC_IOCTL_FLUSH,             ///< 清空内部所有缓冲队列 (软复位).
    ADEC_IOCTL_GET_DELAY,         ///< 获取当前处理延迟。param1: ms, param2: 帧数.
    ADEC_IOCTL_DO_PLC,            ///< 手动触发丢包隐藏.
    ADEC_IOCTL_SET_VOLUME,        ///< 设置硬件音量增益。param1: 0-100 (%).
    ADEC_IOCTL_GET_EXTRADATA,     ///< 获取当前流解析出的头信息.
    ADEC_IOCTL_GET_CURRENT_FMT,   ///< [查询] 获取当前正在处理的格式 (rate, ch).

    AENC_IOCTL_SET_BITRATE,
};

/**
 * @brief 音频编解码设备对象
 */
struct acodec_device {
    struct dev_obj dev;                 ///< 继承自通用设备模型.
};

/**
 * @brief HAL 层操作接口表
 * 
 * 驱动开发者需实现此结构体中的所有函数指针。
 */
struct acodec_hal_ops {
    struct devobj_ops ops;              ///< 设备基础操作集 (suspend/resume 等).
    
    /**
     * @brief 打开设备
     * 
     */
    void *(*open)(struct acodec_device *dec, txAudioInfo_t *info);
    
    /**
     * @brief 关闭设备
     */
    int32 (*close)(struct acodec_device *dec, void *chan);
    
    /**
     * @brief 提交解码任务
     * 
     * [返回]:
     *   - 0: 解码成功 (驱动实现为同步执行，done回调不会被调用).
	 *   - <0: 提交失败.
	 *   特殊值：
	 *   - -EAGAIN：acodec目前不接收新的数据，需要再次retry（驱动内部未设计接收队列，或者接收队列已满）
     *   - -EINPROGRESS: 显式表示正在异步处理，通过 req->done 回调通知最终结果.
     */
    int32 (*decode)(struct acodec_device *dec, struct acodec_decode_req *req);
    
    /**
     * @brief 提交编码任务
     * 
     * [返回]:
     *   - 0: 编码成功 (驱动实现为同步执行，done回调不会被调用).
	 *   - <0: 提交失败.
	 *   特殊值：
	 *   - -EAGAIN：acodec目前不接收新的数据，需要再次retry（驱动内部未设计接收队列，或者接收队列已满）
     *   - -EINPROGRESS: 显式表示正在异步处理，通过 req->done 回调通知最终结果.
     */
    int32 (*encode)(struct acodec_device *dec, struct acodec_encode_req *req);
    
    /**
     * @brief 设备控制
     */
    int32 (*ioctl)(struct acodec_device *dec, void *chan, enum acodec_ioctl_cmd cmd, uint32 param);
    
    /**
     * @brief 注册中断回调
     */
    int32 (*request_irq)(struct acodec_device *dec, uint32 irq_id, acodec_irq_hdl hdl, void *data);
    int32 (*release_irq)(struct acodec_device *dec, uint32 irq_id);
};

/**
 * @brief 打开编解码器
 * 
 * @param dec  [输入] 设备结构体指针.
 * @param info [输入] 编码参数.
 * @return 0=成功, <0=失败.
 */
void *acodec_open(struct acodec_device *dec, txAudioInfo_t *info);

/**
 * @brief 关闭编解码器
 * 
 * [注意]: 会等待当前正在处理的帧完成，然后释放资源.
 */
int32 acodec_close(struct acodec_device *dec, void *chan);

/**
 * @brief 提交解码请求
 * 
 * [线程安全]: 支持多线程并发调用.
 * @param req 请求结构体 (必须在 done 回调返回前保持有效，或者驱动内部复制保存).
 * @return 
 *   - 0: 编码成功 (驱动实现为同步执行，done回调不会被调用).
 *   - <0: 提交失败.
 *   特殊值：
 *   - -EAGAIN：acodec目前不接收新的数据，需要再次retry（驱动内部未设计接收队列，或者接收队列已满）
 *   - -EINPROGRESS: 显式表示正在异步处理，通过 req->done 回调通知最终结果.
*/
int32 acodec_decode(struct acodec_device *dec, struct acodec_decode_req *req);

/**
 * @brief 提交编码请求
 * @param req 请求结构体 (必须在 done 回调返回前保持有效，或者驱动内部复制保存).
 * @return 
 *   - 0: 编码成功 (驱动实现为同步执行，done回调不会被调用).
 *   - <0: 提交失败.
 *   特殊值：
 *   - -EAGAIN：acodec目前不接收新的数据，需要再次retry（驱动内部未设计接收队列，或者接收队列已满）
 *   - -EINPROGRESS: 显式表示正在异步处理，通过 req->done 回调通知最终结果.
 */
int32 acodec_encode(struct acodec_device *dec, struct acodec_encode_req *req);

/**
 * @brief 执行设备控制命令
 */
int32 acodec_ioctl(struct acodec_device *dec, void *chan, enum acodec_ioctl_cmd cmd, uint32 param);

#ifdef __cplusplus
}
#endif

#endif /* _HAL_ADEC_H_ */
