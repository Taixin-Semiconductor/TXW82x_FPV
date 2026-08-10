#ifndef _TXSEMI_FRAMEBUFF_H_
#define _TXSEMI_FRAMEBUFF_H_
#include "media_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct framebuff;

enum FRAMEBUFF_MTYPE {
    F_NONE = 0, //无类型
    F_YUV,      //YUV数据
    F_JPG_NODE,      //jpeg
    F_JPG,      //jpeg
    F_JPG_DECODE_MSG,      //jpeg
    F_THUMB_JPG,      //jpeg
    F_H264,      //h264
    F_RGB,      //未压缩的rgb
    F_ERGB,     //压缩后的rgb
    F_FILE_T,     //文件类型(一般用于文件保存,仅仅支持普通模式:fb->data+fb->len)
};

//设定framebuff的子类型,用uint16_t去定义吧
enum FRAMEBUFF_STYPE {
    FSTYPE_NONE = 0,    //无类型
    FSTYPE_NORMAL_THUMB_JPG,
    FSTYPE_NORMAL_THUMB_JPG_USB,
    FSTYPE_OVER_DPI_THUMB_JPG,
    FSTYPE_OVER_DPI_JPG,
    FSTYPE_VIDEO_VPP_DATA0,
    FSTYPE_VIDEO_VPP_DATA1,
    FSTYPE_VIDEO_PRC_DATA,
    FSTYPE_VIDEO_SCALER_DATA,
    FSTYPE_VIDEO_GEN420_DATA,
    FSTYPE_VIDEO_SOFT_DATA,


    FSTYPE_YUV_P0,
    FSTYPE_YUV_P1,
    FSTYPE_YUV_P2,
    FSTYPE_YUV_SIM_P0,
    FSTYPE_YUV_SIM_P1,
    FSTYPE_YUV_OTHER,
    FSTYPE_YUV_TAKEPHOTO,
    FSTYPE_JPG_CAMERA0,
    FSTYPE_JPG_CAMERA1,
    FSTYPE_JPG_CAMERA2,
    FSTYPE_JPG_FILE,
    FSTYPE_JPG_GEN420_0,
    FSTYPE_JPG_GEN420_REJPG,
    FSTYPE_USB_CAM0,
    FSTYPE_USB_CAM1,

    //h264,硬件产生
    FSTYPE_H264_VPP_DATA0,
    FSTYPE_H264_VPP_DATA1,
    FSTYPE_H264_PRC_DATA,
    FSTYPE_H264_SCALER_DATA,
    FSTYPE_H264_GEN420_DATA,
    FSTYPE_H264_SOFT_DATA,

    FSTYPE_SCALE1_DATA,//scale1自行添加类型,这里添加一个demo去实现大分辨率拍照

    //h264软件(比如读取文件)
    FSTYPE_H264_FILE,

    FSYPTE_INVALID = 0x80, //无效的stype,用于特殊值,由应用去特殊使用
    FSTYPE_GEN420_720P,
};

enum FRAMEBUFF_SOURCE {
    FRAMEBUFF_SOURCE_NONE,
    FRAMEBUFF_SOURCE_JPG_GEN420,    //特殊,标记从gen420编码
    FRAMEBUFF_SOURCE_JPG_SCALER,    //特殊,标记从scaler编码


    FRAMEBUFF_SOURCE_FILE,
    FRAMEBUFF_SOURCE_USB,
    FRAMEBUFF_SOURCE_CAMERA0,
    FRAMEBUFF_SOURCE_CAMERA1,
    FRAMEBUFF_SOURCE_CAMERA2,

    FRAMEBUFF_SOURCE_OSD_ENC,
    FRAMEBUFF_SOURCE_CSC,

};

/** @brief 前向声明 MSI 结构体 */
struct msi;

/**
 * @defgroup Framebuff 媒体数据帧缓冲管理
 * @{
 */

/**
 * @brief 媒体数据帧结构体。       framebuff 只能由2种方式创建：
 *        1. 使用 msi_alloc_fb API
 *        2. 使用 fbpool 预分配：预分配每个fb所需内存空间
 */
struct framebuff {
    struct framebuff *next;      ///< 指向下一个片段节点。用于将多个 framebuff 链接成一个完整帧（分片传输）。

    uint8    *data;              ///< 数据缓冲区指针。
    uint32    len;               ///< 有效数据长度 (单位：字节)。
    uint32    time;              ///< 时间戳 (单位：ms)。
                                 ///< @note 存储的是相对于起始时间的差值 (Delta)，非绝对时间。
    void     *codec_info;        ///< 编码信息
                                 ///< audio 数据的 fb->codec_info 对应 @ref txAudioInfo_t 类型
                                 ///< video 数据的 fb->codec_info 对应 @ref txVideoInfo_t 类型
                                 ///< subtitle 数据的 fb->codec_info    对应 @ref txSubtitleInfo_t 类型

    void     *priv;              ///< 私有信息。通常是用来存储解码控制信息 txVideoDecInfo_t.
    uint8     mtype;             ///< 主数据类型 (Main Type)，取值范围 @ref media_data_category_t。
    uint8     stype;             ///< 子数据类型 (Sub Type)。
    uint8     srcID;             ///< 来源 ID (@ref FRAMEBUFF_SOURCE)，标识数据产生的源模块。
    uint8     datatag;           ///< 数据标签，供应用层自定义标记用途。

    atomic16_t  users;           ///< 引用计数。
                                 ///< - >0: 表示正在被使用
                                 ///< -  0: 触发资源释放

    uint8     index;             ///< 内存池索引。仅在从 @ref fbpool 分配时有效，用于快速归还。

    // 标志位域 (共 1 字节)
    uint8     used    : 1;       ///< [内部] 是否已被使用/占用。
    uint8     pool    : 1;       ///< [内部] 是否来源于内存池 (1: 是, 0: 动态分配)。
    uint8     clone   : 1;       ///< [内部] 是否为fb_clone产生的framebuff。
    uint8     keyfrm  : 1;       ///< [内部] 关键帧标志 (1: 是关键帧, 0: 非关键帧)。
    uint8     last    : 1;       ///< [内部] 是否为最后一帧数据。
    uint8     rev     : 4;       ///< 保留位。

    struct msi *msi;             ///< 产生此 framebuff 的 MSI 组件指针。

    /// 当引用计数归零时，若此函数指针非空，则优先调用此函数进行特殊清理。
    mfree_cb_t  free;
    void       *free_priv;       ///< 传递给 @ref free 回调函数的私有参数。
};

/**
 * @brief 多 Reader 支持的 Framebuff 队列
 * @details 支持多个消费者 (Reader) 同时从同一队列读取数据，适用于一对多分发场景。
 */
struct fbqueue {
    struct msi         *msi;
    struct os_semaphore sema;    ///< 信号量，用于队列阻塞/唤醒控制。
    uint16           reader_max; ///< 支持的最大 Reader 数量。

    // 标志位域
    uint8            init  : 1;  ///< 初始化标志。
    uint8            alloc : 1;  ///< 内存分配标志。
    uint8            rev   : 6;  ///< 保留位。

    uint8            full;       ///< [调试] 队列触发 full 的次数

    uint32          *readers;    ///< Reader 状态数组，记录每个 Reader 的读取位置。
    RBUFFER_DEF_R(rbQ, struct framebuff *); ///< 环形缓冲区定义，存储 framebuff 指针。
};


#define FBPOOL_SIZE(pool_size, data_size) ((pool_size) * (sizeof(struct framebuff) + (data_size)))

/**
 * @brief Framebuff 预分配内存池。
 * @warning **注意**: 仅在需要 预分配数据空间 时使用fbpool，如果仅需要限制framebuff的分配数量，使用msi->fb_limits即可。
 */
struct fbpool {
    uint8  size;               ///< 池容量 (framebuff 数量)。
    uint8  inited : 1;         ///< 初始化标志。
    uint8  rev    : 7;         ///< 保留位。
    uint16 used;               ///< 已被使用的fb数量
    struct framebuff *pool;    ///< 预分配的 framebuff 数组首指针。
};

/**
 * @brief framebuff默认分配函数，由应用代码提供实现。
 * @param size 需要分配的字节数。
 * @return 成功返回内存指针，失败返回 NULL。
 * @note 此内存通常需满足 DMA 对齐要求。
 */
extern void *fb_mem_alloc(uint32 size);

/**
 * @brief framebuff默认释放函数，由应用代码提供实现。
 * @param ptr 由 @ref fb_mem_alloc 分配的内存指针。
 */
extern void fb_mem_free(void *ptr);

/**
 * @brief 增加 framebuff 引用计数
 *
 * @param fb 目标 framebuff 指针。
 * @note MSI 组件接收到 framebuff 后，若需保存或异步处理，必须调用此函数防止被提前释放。
 */
extern void fb_get(struct framebuff *fb);

/**
 * @brief 建立 framebuff 引用链
 *
 * @details 将 fb_new 指向 fb_old，形成链表结构。fb_old 的引用计数会自动 +1。
 *          常用于将一个大数据帧拆分为多个小片段传输。
 *
 * @param fb_new 新的片段节点 (当前节点)。
 * @param fb_old 被引用的前驱节点。
 */
extern void fb_ref(struct framebuff *fb_new, struct framebuff *fb_old);

/**
 * @brief 在 framebuff 链表中查找指定类型的第一个节点
 *
 * @param fb    链表头指针。
 * @param mtype 主类型匹配值。
 * @param stype 子类型匹配值 (若为 0 则仅匹配主类型)。
 *
 * @return struct framebuff* 找到则返回节点指针，否则返回 NULL。
 */
extern struct framebuff *fb_find(struct framebuff *fb, uint8 mtype, uint8 stype);

/**
 * @brief 计算 framebuff 链表中指定类型数据的总长度
 *
 * @param fb    链表头指针。
 * @param mtype 主类型匹配值。
 * @param stype 子类型匹配值。
 *
 * @return uint32 累计长度 (字节)。
 */
extern uint32 fb_len(struct framebuff *fb, uint8 mtype, uint8 stype);

/**
 * @brief 减少 framebuff 引用计数并可能释放资源
 *
 * @param fb 目标 framebuff 指针。
 * @details 每调用一次，引用计数减 1。当计数归零时：
 *          产生 MSI_CMD_FREE_FB 命令，通知fb->msi组件，fb即将被释放。
 *          如果设置了fb->free，则调用 fb->free 执行释放资源
 */
extern void fb_put(struct framebuff *fb);

/**
 * @brief 克隆一个 framebuff
 *
 * @details 创建一个新的 framebuff，复制原帧的大部分元数据 (如时间戳、类型等)，
 *          但会重新设置 type，并重新管理引用计数。数据指针被共享。
 *
 * @param fb   原始帧指针。
 * @param type 新帧的类型。
 * @param msi  新帧所属的 MSI 组件。
 *
 * @return struct framebuff* 成功返回克隆体，失败返回 NULL。
 */
extern struct framebuff *fb_clone(struct framebuff *fb, uint16 type, struct msi *msi, void *priv);

/**
 * @brief 初始化 framebuff 队列
 *
 * @param q     队列结构体指针。
 * @param qbuff 队列内部缓冲区地址
 * @details 如果qbuff为NULL，则自动申请qsize大小的空间
 * @param qsize 队列缓冲区大小 (元素个数)。
 *
 * @return int32 成功返回 0，失败返回负错误码。
 */
extern int32 fbq_init(struct fbqueue *q, uint8 *qbuff, int32 qsize, struct msi *msi);

/**
 * @brief 销毁 framebuff 队列
 *
 * @param q 队列结构体指针。
 * @details 会遍历队列并调用 @ref fb_put 释放所有残留的 framebuff。
 *
 * @return int32 成功返回 0，失败返回负错误码。
 */
extern int32 fbq_destroy(struct fbqueue *q);

/**
 * @brief 向队列存入一个 framebuff。
 *
 * @param q   队列指针。
 * @param fb  待入队的 framebuff 指针。
 * @param ref 存入队列时是否增加对 framebuff 的引用计数。
 *            默认应该增加引用计数，以下2种情况可以不增加引用计数：
 *            1. fb创建者 将fb存入到自己的队列。
 *            2. msi从队列中取出fb，又存入（另一个）队列。
 *
 * @return int32 成功返回 0，队列满或其他错误返回负值。
 */
extern int32 fbq_enqueue(struct fbqueue *q, struct framebuff *fb, uint8 ref);

/**
 * @brief 清空队列中的所有 framebuff
 *
 * @param q 队列指针。
 * @details 会对每个出队的 framebuff 调用 @ref fb_put。
 *
 */
extern void fbq_clear(struct fbqueue *q);

/**
 * @brief 获取队列中当前的 framebuff 数量
 *
 * @param q 队列指针。
 * @return int32 元素个数。
 */
extern int32 fbq_count(struct fbqueue *q);

/**
 * @brief 追踪或丢弃队列中的指定 framebuff
 *
 * @param q       队列指针。
 * @param fb      目标 framebuff 指针。
 * @param discard 操作标志。
 *                - 0: 仅查找/追踪。
 *                - 1: 查找并从队列中移除 (丢弃)，同时调用 fb_put。
 *
 */
extern void fbq_trace(struct fbqueue *q, struct framebuff *fb, struct msi *owner, int8 discard, int8 dump);

/**
 * @brief 从队列取出一个 framebuff (阻塞/超时模式)
 *
 * @param q      队列指针。
 * @param tmo_ms 超时时间 (毫秒)。
 *               - 0: 立即返回 (非阻塞)。
 *               - >0: 等待指定时间。
 *               - 特定值 (如 0xFFFFFFFF): 永久等待。
 *
 * @return struct framebuff* 成功返回帧指针，超时或错误返回 NULL。
 */
extern struct framebuff *fbq_dequeue(struct fbqueue *q, uint32 tmo_ms);

/**
 * @brief 从队列取出一个 framebuff (指定 Reader ID)
 *
 * @details 用于多消费者场景，每个 Reader 拥有独立的读取游标。
 *
 * @param q      队列指针。
 * @param reader Reader ID (0 ~ reader_max-1)。
 *
 * @return struct framebuff* 成功返回帧指针，无数据返回 NULL。
 */
extern struct framebuff *fbq_dequeue_r(struct fbqueue *q, uint8 reader);

/**
 * @brief 初始化 framebuff 预分配池
 *
 * @param pool      池结构体指针。
 * @param size      池中预分配的 framebuff 数量。
 * @param free_cb   自定义的framebuff 释放函数
 * @param free_priv 自定义的framebuff 释放函数的私有数据。
 *
 * @return int32 成功返回 0，失败返回负错误码。
 */
extern int32 fbpool_init(struct fbpool *pool, uint8 size, mfree_cb_t free_cb, void *free_priv);

/**
 * @brief 从预分配池获取一个 framebuff
 *
 * @param pool 池指针。
 * @param type 数据类型。
 * @param msi  所属 MSI 组件。
 *
 * @return struct framebuff* 成功返回帧指针，池空返回 NULL。
 */
extern struct framebuff *fbpool_get(struct fbpool *pool, uint16 type, struct msi *msi);

/**
 * @brief 向预分配池归还一个 framebuff
 *
 * @param pool 池指针。
 * @param fb   待归还的帧指针 (必须源自该池)。
 *
 * @return int32 成功返回 0，失败返回负错误码。
 */
extern int32 fbpool_put(struct fbpool *pool, struct framebuff *fb);

/**
 * @brief 销毁预分配池
 *
 * @param pool 池指针。
 * @details 通知所有相关模块丢弃源自此池的 framebuff，并释放池内存。
 *
 * @return int32 成功返回 0。
 */
extern int32 fbpool_destroy(struct fbpool *pool);

/**
 * @brief 初始化预分配 framebuff 的静态信息
 *
 * @details 仅在池初始化阶段调用，用于绑定预分配内存地址和大小。
 *
 * @param fbpool    预分配池指针 (struct fbpool*)。
 * @param index     帧索引 (0 <= index < size)。
 * @param buff_addr 预分配的数据缓冲区地址。
 * @param buff_size 预分配的数据缓冲区大小。
 * @param priv_data 私有数据指针。
 *
 * @note 这是一个宏，直接操作结构体成员。
 */
#define FBPOOL_SET_INFO(fbpool, index, buff_addr, buff_size, priv_data) do { \
        (fbpool)->pool[index].data = (uint8 *)(void *)(buff_addr); \
        (fbpool)->pool[index].len  = (buff_size); \
        (fbpool)->pool[index].priv = (priv_data); \
    } while(0)

/**
 * @brief 释放 fbpool 的预分配内存空间
 *
 * @details 仅用于在 fbpool_destroy 之前释放预分配的内存
 *
 * @param p         预分配池指针 (struct fbpool*)。
 * @param free_data fb->data的内存释放句柄
 * @param free_priv fb->priv的内存释放句柄
 *
 * @note 这是一个宏，直接操作结构体成员。
 */
#define FBPOOL_FREE(p, free_data, free_priv) do{ \
    for(int i=0; i<(p)->size; i++){ \
        if(((int)(free_data) != 0) && (p)->pool[i].data) \
            ((void (*)(void*))(free_data))((void *)(p)->pool[i].data); \
        if(((int)(free_priv) != 0) && (p)->pool[i].priv) \
            ((void (*)(void*))(free_priv))((void *)(p)->pool[i].priv); \
    } \
}while(0);

///////////////////////////////////////////////////////////////////////
extern void *decoder_mem_alloc(uint32 size);
extern void decoder_mem_free(void *ptr);
extern void *decoder_mem_zalloc(size_t size);
extern void *decoder_mem_realloc(void *ptr, size_t size);
extern void *decoder_mem_calloc(size_t nitems, size_t size);

extern void *encoder_mem_alloc(uint32 size);
extern void encoder_mem_free(void *ptr);
extern void *encoder_mem_zalloc(size_t size);
extern void *encoder_mem_realloc(void *ptr, size_t size);
extern void *encoder_mem_calloc(size_t nitems, size_t size);


/** @} */ // End of Framebuff group

#ifdef __cplusplus
}
#endif

#endif

