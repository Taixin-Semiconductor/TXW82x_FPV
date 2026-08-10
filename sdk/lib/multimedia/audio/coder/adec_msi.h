#ifndef _TXSEMI_ADEC_MSI_H_
#define _TXSEMI_ADEC_MSI_H_

/**
 * @file adec_msi.h
 * @brief Audio Decoder MSI 组件通用框架头文件
 *
 * @details
 * 本模块提供了音频解码器 MSI (Media Stream Interface) 组件的**通用实现框架**。
 *
 * 设计背景：
 * - 不同的音频解码器 (MP3, AAC, PCM, OGG 等) 在 MSI 框架中的行为模式高度一致。
 *   (如：初始化 -> 取流 -> 解码 -> 入队 -> 同步 -> 播放)。
 * - 为避免重复开发，本模块提取了共通逻辑 (状态机、队列管理、调度机制、AV 同步接口)。
 * - 所有基于此框架开发的解码器将自动具备调速、多轨道切换、防欠载等通用功能。
 *
 * **扩展机制 (重要)**：
 * - 各具体的 Audio Decoder MSI 组件若需维护**私有信息** (如特定格式的上下文、私有缓冲区等)，
 *   应在定义自身结构体时**继承 (包含)** `struct audio_dec_msi` 作为第一个成员。
 * - 调用 `adec_msi_new` 时，`size` 参数应传入**子类结构体的真实大小** (sizeof(SubStruct))。
 * - 框架内部会按传入的 size 分配内存，并返回指向基类 `struct audio_dec_msi` 的指针。
 * - 使用者可通过 `container_of` 宏或强制类型转换，从基类指针还原出包含私有数据的完整结构体。
 *
 * 使用示例：
 *   struct my_mp3_dec {
 *       struct audio_dec_msi base;  // 必须放在首位
 *       int specific_mp3_var;       // 私有扩展数据
 *   };
 *   // 初始化时：adec_msi_init(..., sizeof(struct my_mp3_dec));
 */

/* -------------------------------------------------------------------------- */
/*                                日志宏定义                                   */
/* -------------------------------------------------------------------------- */
#define adec_dbg(fmt, ...)    //os_printf(KERN_DEBUG"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define adec_err(fmt, ...)    os_printf(KERN_ERR"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define adec_warn(fmt, ...)   os_printf(KERN_WARNING"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

/**
 * @brief 音频解码 MSI 组件上下文结构 (Context Structure)
 * @details
 * 核心控制块，维护单个音频流运行时的所有状态。
 * **注意**：此结构设计为可被继承。具体解码器实现时，应将此结构体作为自定义结构体的**第一个成员**。
 *
 * 包含：接口指针、基类句柄、同步对象、解码内核、缓冲队列、统计信息等。
 */
struct audio_dec_msi {
    struct msi       *msi;           ///< [基类] MSI 组件基类句柄，用于注册到系统调度器、定时器及生命周期管理
    struct msi       *avsync;        ///< [同步] 关联的 AV Sync 主组件句柄，用于获取全局时钟参考或上报 PTS 以维持音画同步
    struct msi       *follow;        ///< [同步] 跟随输出的msi组件，用于音轨切换时等待旧的音轨数据输出完成
    void             *dec;           ///< [内核] 具体解码器实例的私有句柄 (由 ops->open 创建)

    struct framebuff *ply_fb;        ///< [状态] 当前正在处理中或挂起的播放帧缓冲 (Frame Buffer) 指针
    struct os_work    work;          ///< [调度] 工作项结构体，所有 Audio MSI 组件共用同一个全局 Audio Workqueue 进行异步任务调度
    struct os_mutex   lock;          ///< [同步] 用于多线程互斥访问msi组件

    struct fbqueue    plyQ;          ///< [队列] 播放队列 (FIFO)，存放已解码完成、等待送入 DAC 的 PCM 帧缓冲
    AUDIO_SPEED_PITCH speed;         ///< [控制] 调速/变调控制参数 (e.g., 1.0x=正常, 1.5x=快放, 0.5x=慢放)
    txAudioInfo_t     audio_info;    ///< 音频编码信息，复制于解码前的fb，给解码后的fb->priv赋值。
    uint32            start_plytime; ///< 启动播放的时刻点

    uint16            interval_time; ///< [调度] 解码任务调度间隔 (ms)，用于限流，防止解码过快占用过多 CPU 导致其他任务饥饿
    uint8             id;            ///< [标识] 解码器实例 ID (0~N)，用于多实例场景下的日志区分和资源管理

    /* --- 状态标志位 (Bitfields, 共 1 字节) --- */
    uint8             start  : 1;    ///< 1: 已启动播放; 0: 未启动
    uint8             pause  : 1;    ///< 1: 处于暂停状态; 0: 正常运行
    uint8             eof    : 1;    ///< 1: 已收到文件结束 (EOF) 通知; 0: 流进行中 (通常由 Demuxer 通知)
    uint8             ply_end: 1;    ///< 1: 所有数据已播放完成; 0: 播放未结束
    uint8             rev    : 4;    ///< 保留位，用于未来功能扩展或内存对齐

    uint8             dac_buftime;   ///< [硬件] 反馈值：DAC (数模转换器) 硬件 FIFO 中当前的剩余数据时长 (ms)，用于判断是否即将欠载
    uint8             chans_cnt;     ///< 所支持的通道个数
    struct audio_dec_mgr *mgr;       ///< 解码器组件的管理器
    struct acodec_decode_req req;
};

/**
 * @struct audio_dec_mgr
 * @brief 音频解码器管理器（多通道管理）
 *
 * @details
 * 统一管理多个音频解码器通道，提供全局 ops、通道分配、统一命令入口、资源管理。
 */
struct audio_dec_mgr {
    struct msi                  *msi;          /**< 管理器MSI基类句柄 */
    struct audio_dec_msi        **chans;        /**< 解码器通道实例数组 */
    const char                  **chan_names;   /**< 通道名称数组（用于日志/调试） */
    struct acodec_device        *dev;
    struct auchange_device      *ac_dev;
    uint16                      msi_size;     /**< MSI组件实例大小（用于内存分配） */
    uint8                       chan_cnt;     /**< 最大支持通道数 */
    uint8                       plyQ_size;     /**< 每个通道的播放队列深度 */
};

/**
 * @brief 初始化音频解码 MSI 组件
 * @param mgr    [In] MSI组件的管理器
 * @param type   [In] 组件类型
 * @param codec  [In] 初始化参数
 * @return 成功返回组件句柄指针 (struct audio_dec_msi*)，失败返回 NULL
 * @note 返回的指针可以直接当作基类使用，也可通过 container_of 转回子类指针访问私有成员。
 */
struct audio_dec_msi *adec_msi_new(struct audio_dec_mgr *mgr, uint16 type, txAudioInfo_t *codec);

/**
 * @brief 通道对象的MSI命令执行函数
 * @param msi    [In] 组件句柄
 * @param cmd_id [In] 命令 ID 
 * @param param1 [In] 命令参数 1
 * @param param2 [In] 命令参数 2
 * @return 成功0，失败负值错误码
 */
int32 adec_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2);

/**
 * @brief 解码器管理器 MSI命令处理函数，主要是用于执行 MSI_CMD_NEW_CHANNEL
 *
 * @param msi     管理器MSI句柄
 * @param cmd_id  命令ID
 * @param param1  参数1
 * @param param2  参数2
 * @return 成功0，失败负值错误码
 */
int32 adec_mgr_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2);


#endif /* _TXSEMI_ADEC_MSI_H_ */
