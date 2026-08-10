#ifndef _TXSEMI_AVCONTAINER_H_
#define _TXSEMI_AVCONTAINER_H_
#include "framebuff.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AVContainer 音视频容器解复用接口
 * @brief 定义了解复用器 (Demuxer) 的标准接口、操作回调及控制命令。
 * @{
 */

/**
 * @brief 链接段属性宏：用于将解复用器实例注册到特定内存段
 * @details 编译器将通过这些宏将实现了该接口的结构体自动放置在 .avmuxer 或 .avdemuxer 段中，
 *          以便系统在启动时自动扫描并注册这些组件。
 */
#define __avmuxer   __at_section(".avmuxer")
#define __avdemuxer __at_section(".avdemuxer")


/**
 * @brief 输入到解复用器的数据流类型。Container可以根据数据流类型进行相应的逻辑优化。
 */
enum AVDEMUXER_STREAM_TYPE {
    AVDEMUXER_STREAM_FILE,         ///< 本地文件流
    AVDEMUXER_STREAM_URLFILE,      ///< 网络文件流
    AVDEMUXER_STREAM_LIVE_STREAM,  ///< 直播流
};

/**
 * @brief 解复用器 ioctl 控制命令枚举
 */
enum AVDEMUXER_IOCTL {
    AVDEMUXER_SET_PLAY_SPEED,      ///< [由Owner执行] 设置播放倍速 (param1: 速度值，例如 100 表示 1.0x)。
    AVDEMUXER_SET_TRACK,           ///< [由Owner执行] 切换 音频/字幕 track (param1:  track id)。
    AVDEMUXER_GET_TOTAL_DURATION,  ///< [由Owner执行] 获取媒体数据总时长
    AVDEMUXER_GET_FILE_SIZE,       ///< [由Demuxer执行] 获取文件Size. param1: int64 *, param2: 0
    AVDEMUXER_GET_STREAM_TYPE,     ///< [由Demuxer执行] 获取数据流类型：本地文件，网络文件，直播流. param1:0, param2:0
    AVDEMUXER_GET_BUF_SIZE,        ///< [由Demuxer执行] 获取数据缓冲区大小
    AVDEMUXER_SET_BUFFERING,       ///< [由Owner执行] 通知Demuxer开始缓存数据，不要执行demux
};

/**
 * @brief 解复用器数据访问操作集 (I/O Callbacks)
 * @details 由外部调用者（如播放器核心）实现并传入，供 Container 内部调用以读取媒体流数据。
 *          接口行为类比标准 C 库文件操作 (fread, fseek, feof)。
 */
struct AVDemuxerOps {
    /**
     * @brief 读取数据回调
     * @details 功能类比 fread。Container 通过此接口从源端读取原始字节流。
     *
     * @param ptr   目标缓冲区指针。
     * @param size  单个元素的大小 (字节)。
     * @param nmemb 元素个数。
     * @param hdl   数据源句柄 (由外部传入)。
     * @return size_t 成功读取的元素个数。若小于 nmemb 可能表示遇到 EOF 或错误。
     */
    size_t (*read)(void *ptr, size_t size, size_t nmemb, void *hdl);

    /**
     * @brief  seek 定位回调
     * @details 功能类比 fseek。Container 计算出目标偏移量后调用此接口进行跳转。
     *
     * @param hdl    数据源句柄。
     * @param offset 偏移量。
     * @param whence 基准位置 (SEEK_SET, SEEK_CUR, SEEK_END)。
     * @return int 成功返回 0，失败返回 -1。
     */
    int (*seek)(void *hdl, off_t offset, int whence);

    /**
     * @brief 文件结束判断回调
     * @details 功能类比 feof。用于判断数据源是否已读取完毕。
     *
     * @param hdl 数据源句柄。
     * @return int 若到达文件末尾返回非零值，否则返回 0。
     */
    int (*eof)(void *hdl);

    /**
     * @brief 查询可读取的数据长度
     *
     * @param hdl 数据源句柄。
     * @return int 返回可读取的数据长度。
     */
    int (*avail)(void *hdl);

    /**
     * @brief 关闭数据源回调
     * @details 用于关闭底层句柄。
     * @note Container 内部通常**不直接调用**此接口。句柄的生命周期管理由外部代码负责，
     *       外部代码在销毁 Container 后自行关闭句柄。
     *
     * @param hdl 数据源句柄。
     */
    void (*close)(void *hdl);

    /**
     * @brief 输出解析后的数据帧回调
     * @details Container 解析完一帧数据并构建好 framebuff 后，调用此接口将数据交出。
     *
     * @param owner Container的创建者。
     * @param fb    构建好的 framebuff 指针。
     *
     * @note **所有权转移**: 一旦调用 outFB，framebuff 的所有权即转移给外部代码。
     *       Container 不再负责该 fb 的释放，外部代码需负责后续的引用计数管理 (fb_put)。
     */
    void (*outFB)(struct msi *owner, struct framebuff *fb);


    /**
     * @brief 通用控制接口 (IOCTL)
     * @details 用于Container向Owner执行特定命令 (如查询文件Size，数据流类型)。
     *
     * @param owner  Container的创建者。
     * @param cmd    命令字 (@ref AVDEMUXER_IOCTL)。
     * @param param1 命令参数 1。
     * @param param2 命令参数 2。
     *
     * @return int 成功返回 0，失败返回负错误码。
     */
    int (*ioctl)(struct msi *owner, uint32 cmd, uint32 param1, uint32 param2);
};

/**
 * @brief 音视频解复用器 (Demuxer) 接口定义
 * @details 定义了特定格式容器（如 MP4, TS, MKV）的解复用逻辑。
 *          每种格式需实现此结构体中的所有函数指针。
 */
struct AVDemuxer {
    uint32      type;       ///< 容器类型标识。外部代码根据文件头识别格式后，据此查找对应的 Demuxer。
    const char *name;       ///< 解复用器名称字符串。

    /**
     * @brief 初始化解复用器实例
     * @details 创建并初始化特定格式的 Container 对象。
     *
     * @param hdl    数据源句柄 (文件句柄、网络 Socket 等)。
     * @param ops    I/O 操作接口集。外部已初始化好，Container 仅需调用。
     * @param hdr    预读数据缓冲区指针。
     *               - 背景：外部在识别格式时可能已读取了文件头部分数据。
     *               - 作用：由于网络流等不可回溯源无法重新 seek 读取，Container 必须解析并保存这部分数据。
     *               - 生命周期：init 返回后，外部将释放 hdr 指向的内存，Container 如需使用必须自行拷贝。
     * @param len    预读数据的长度。
     * @param owner  demuxer的创建者。
     *
     * @return void* 成功返回初始化的 Container 实例指针，失败返回 NULL。
     */
    void *(*init)(void *hdl, const struct AVDemuxerOps *ops, void *hdr, uint32 len, struct msi *owner);

    /**
     * @brief 释放解复用器资源
     * @details 关闭 Container，释放内部申请的私有内存。
     *
     * @param c Container 实例指针。
     * @return int32 成功返回 0，失败返回负错误码。
     */
    int32(*release)(void *c);

    /**
     * @brief 执行播放跳转 (Seek)
     * @details 外部请求跳转到指定时间点。Container 需计算对应的时间戳偏移量，
     *          调用 ops->seek 进行物理跳转，并清空内部缓存的旧数据。
     *
     * @param c    Container 实例指针。
     * @param time 目标播放时间 (单位：ms)。
     *
     * @return int32 成功返回 0。
     *         - 若不支持 seek 操作，返回 -ENOTSUPP。
     *         - 其他失败情况返回相应负错误码。
     */
    int32(*do_seek)(void *c, uint32 time);

    /**
     * @brief 执行解复用操作 (Demux)
     * @details 核心解析函数。外部循环调用此接口，每次调用尝试解析**一帧**完整数据。
     *
     * @param c    Container 实例指针。
     * @param priv 外部上下文指针 (透传给 ops->outFB)。
     *
     * @return int32 
     *         >0：成功读取的数据长度，返回后马上再次执行do_demux
     *         =0: 未读取到数据. 检查是否eof，delay后再执行do_demux
     *         <0: demux失败，返回错误码：
     *             -EAGAIN: 返回后马上再次执行do_demux
     *             -ENOMEM: 没有buffer，可以 delay 后再执行
     *
     * @note **关键行为约束**:
     *       1. **非阻塞**: 此函数严禁阻塞或 sleep。
     *       2. **缓存机制**: 若当前读取的数据不足以构成一帧，应将数据缓存在内部，立即返回 0 (或特定状态)
     *       3. **输出**: 解析成功后，构建 framebuff (设置 mtype/stype/time)，并立即调用 ops->outFB 输出。
     *       4. 注意 返回值>0 和 -EAGAIN 会导致马上再次执行do_demux，需要避免因此死产生循环。
     */
    int32(*do_demux)(void *c);

    /**
     * @brief 通用控制接口 (IOCTL)
     * @details 用于Owner向Container执行特定命令 (如切换音轨、变速)。
     *
     * @param c      Container 实例指针。
     * @param cmd    命令字 (@ref AVDEMUXER_IOCTL)。
     * @param param1 命令参数 1。
     * @param param2 命令参数 2。
     *
     * @return int 成功返回 0，失败返回负错误码。
     */
    int (*ioctl)(void *c, uint32 cmd, uint32 param1, uint32 param2);
};

/**
 * @brief 根据类型获取解复用器实例
 *
 * @param type 容器类型标识。
 * @return const struct AVDemuxer* 找到则返回对应的接口结构体指针，未找到返回 NULL。
 */
const struct AVDemuxer *AVDemuxer_Get(uint32 type);

/** @} */ // End of AVContainer group

#ifdef __cplusplus
}
#endif

#endif /* _TXSEMI_AVCONTAINER_H_ */

