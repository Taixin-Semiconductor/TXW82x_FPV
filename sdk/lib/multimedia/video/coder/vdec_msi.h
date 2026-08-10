#ifndef _TXSEMI_ADEC_MSI_H_
#define _TXSEMI_ADEC_MSI_H_
#include "hal/vcodec.h"
#include "hal/vdd.h"
/**
 * @file vdec_msi.h
 * @brief Video Decoder MSI 组件通用框架头文件
 *
 * @details
 * 本模块提供了视频解码器 MSI (Media Stream Interface) 组件的**通用实现框架**。
 */

/* -------------------------------------------------------------------------- */
/*                                日志宏定义                                   */
/* -------------------------------------------------------------------------- */
#define vdec_dbg(fmt, ...)    //os_printf(KERN_DEBUG"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define vdec_err(fmt, ...)    os_printf(KERN_ERR"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define vdec_warn(fmt, ...)   os_printf(KERN_WARNING"%s:%d::"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

/**
 * @brief Video解码 MSI 组件上下文结构 (Context Structure)
 * @details
 * 核心控制块，维护单个音频流运行时的所有状态。
 * **注意**：此结构设计为可被继承。具体解码器实现时，应将此结构体作为自定义结构体的**第一个成员**。
 *
 * 包含：接口指针、基类句柄、同步对象、解码内核、缓冲队列、统计信息等。
 */
struct video_dec_msi {
    struct msi            *msi;      ///< [基类] MSI 组件基类句柄，用于注册到系统调度器、定时器及生命周期管理
    struct msi            *avsync;   ///< [同步] 关联的 AV Sync 主组件句柄，用于获取全局时钟参考或上报 PTS 以维持音画同步
    void                  *chan;     ///< 解码器通道句柄

    struct os_work    work;          ///< [调度] 工作项结构体，所有 Video MSI 组件共用同一个全局 Video Workqueue 进行异步任务调度
    struct os_mutex   lock;          ///< [同步] 用于多线程互斥访问msi组件

    struct framebuff *ply_fb;        ///< 当前正在处理中或挂起的播放帧缓冲 (Frame Buffer) 指针
    struct fbqueue    plyQ;          ///< 播放队列 (FIFO)，存放已解码完成、等待输出显示的视频帧数据
    struct vcodec_decode_req req;    ///< 解码请求
    struct vdd_rect   vdd;

    uint8             chans_cnt;     ///< 所支持的通道个数
    uint8             interval_time; ///< [调度] 解码任务调度间隔 (ms)，用于限流，防止解码过快占用过多 CPU 导致其他任务饥饿
    uint8             id;            ///< [标识] 解码器实例 ID (0~N)，用于多实例场景下的日志区分和资源管理

    /* --- 状态标志位 (Bitfields, 共 1 字节) --- */
    uint8             start  : 1;    ///< 1: 已启动播放; 0: 未启动
    uint8             pause  : 1;    ///< 1: 处于暂停状态; 0: 正常运行
    uint8             eof    : 1;    ///< 1: 已收到文件结束 (EOF) 通知; 0: 流进行中 (通常由 Demuxer 通知)
    uint8             ply_end: 1;    ///< 1: 所有数据已播放完成; 0: 播放未结束
    uint8             decoding: 1;   ///< 1: 正在解码中
    uint8             rev    : 3;    ///< 保留位，用于未来功能扩展或内存对齐
    int32             decode_status; ///< 解码状态：0：成功，<0: 错误码, 1:已发起请求
    uint32            start_plytime; ///< 启动播放的时刻点
    uint32            play_time;     ///< 播放时间 - 无音频数据时使用video记录的播放时间

    struct video_dec_mgr *mgr;       ///< 解码器组件的管理器
};

/**
 * @struct video_dec_mgr
 * @brief 音频解码器管理器（多通道管理）
 *
 * @details
 * 统一管理多个音频解码器通道，提供全局 ops、通道分配、统一命令入口、资源管理。
 */
struct video_dec_mgr {
    struct msi                 *msi;          /**< 管理器MSI基类句柄 */
    struct video_dec_msi      **chans;        /**< 解码器通道实例数组 */
    const char                **chan_names;   /**< 通道名称数组（用于日志/调试） */
    struct vcodec_device       *dev;
    uint16                      msi_size;     /**< MSI组件实例大小（用于内存分配） */
    uint8                       chan_cnt;     /**< 最大支持通道数 */
    uint8                       plyQ_size;     /**< 每个通道的播放队列深度 */
};

/**
 * @brief 初始化音频解码 MSI 组件
 * @param mgr    [In] MSI组件的管理器
 * @param type   [In] 组件类型
 * @return 成功返回组件句柄指针 (struct video_dec_msi*)，失败返回 NULL
 * @note 返回的指针可以直接当作基类使用，也可通过 container_of 转回子类指针访问私有成员。
 */
struct video_dec_msi *vdec_msi_new(struct video_dec_mgr *mgr, uint16 type);

/**
 * @brief 通道对象的MSI命令执行函数
 * @param msi    [In] 组件句柄
 * @param cmd_id [In] 命令 ID 
 * @param param1 [In] 命令参数 1
 * @param param2 [In] 命令参数 2
 * @return 成功0，失败负值错误码
 */
int32 vdec_msi_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2);

/**
 * @brief 解码器管理器 MSI命令处理函数，主要是用于执行 MSI_CMD_NEW_CHANNEL
 *
 * @param msi     管理器MSI句柄
 * @param cmd_id  命令ID
 * @param param1  参数1
 * @param param2  参数2
 * @return 成功0，失败负值错误码
 */
int32 vdec_mgr_action(struct msi *msi, uint32 cmd_id, uint32 param1, uint32 param2);


#endif /* _TXSEMI_ADEC_MSI_H_ */
