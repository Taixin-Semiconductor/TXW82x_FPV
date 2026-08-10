#ifndef _TXSEMI_AENC_MSI_H_
#define _TXSEMI_AENC_MSI_H_

#define aenc_dbg(fmt, ...)    //os_printf(KERN_DEBUG"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define aenc_err(fmt, ...)    os_printf(KERN_ERR"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define aenc_warn(fmt, ...)   os_printf(KERN_WARNING"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

enum {
    enc_name_1,
    enc_name_2,
    enc_name_3,
    enc_name_4,
};

struct audio_enc_msi {
    struct msi       *msi;           ///< [基类] MSI 组件基类句柄，用于注册到系统调度器、定时器及生命周期管理
    void             *enc;           ///< [内核] 具体编码器实例的私有句柄 (由 ops->open 创建)

    struct framebuff *out_fb;        ///< [状态] 当前正在处理中或挂起的编码帧缓冲 (Frame Buffer) 指针
    struct os_work    work;          ///< [调度] 工作项结构体，所有 Audio MSI 组件共用同一个全局 Audio Workqueue 进行异步任务调度
    struct os_mutex   lock;          ///< [同步] 用于多线程互斥访问msi组件

    struct fbqueue    outQ;          ///< [队列] 编码队列 (FIFO)，存放已编码完成、等待输出的帧缓冲
    txAudioInfo_t     audio_info;    ///< 音频编码信息，初始化时幅值
    uint16            interval_time; ///< [调度] 编码任务调度间隔 (ms)，用于限流，防止编码过快占用过多 CPU 导致其他任务饥饿
    uint8             id;            ///< [标识] 编码器实例 ID (0~N)，用于多实例场景下的日志区分和资源管理

    /* --- 状态标志位 (Bitfields, 共 1 字节) --- */
    uint8             start  : 1;    ///< 1: 已启动编码; 0: 未启动
    uint8             pause  : 1;    ///< 1: 处于暂停状态; 0: 正常运行
    uint8             update_timestamp  : 1;
    uint8             encode_delay      : 1;
    uint8             output_delay      : 1;
    uint8             rev    : 3;    ///< 保留位，用于未来功能扩展或内存对齐

    uint8             chans_cnt;     ///< 所支持的通道个数
    uint8             need_more;      ///< 需要取新数据输入到编码器
    struct audio_enc_mgr *mgr;       ///< 编码器组件的管理器
    struct acodec_encode_req req;
    uint32            timestamp;
    uint16            get_total_size;
    uint16            out_total_size;
};

struct audio_enc_mgr {
    struct msi                  *msi;           /**< 管理器MSI基类句柄 */
    struct audio_enc_msi        **chans;        /**< 编码器通道实例数组 */
    const char                  **chan_names;   /**< 通道名称数组（用于日志/调试） */
    const char                  **chan_default_names;   /**< 通道默认名称数组（用于日志/调试） */
    struct acodec_device        *dev;
    uint16                      msi_size;       /**< MSI组件实例大小（用于内存分配） */
    uint8                       chan_cnt;       /**< 最大支持通道数 */
    uint8                       outQ_size;      /**< 每个通道的输出队列深度 */
    uint16                      fb_type;
};

struct audio_enc_msi *aenc_msi_new(struct audio_enc_mgr *mgr, uint16 type, txAudioInfo_t *codec);

int32 aenc_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2);

int32 aenc_mgr_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2);

struct msi *aenc_get_msi(audio_codec_t type, const char *self_str, txAudioInfo_t *info);

#endif