/**
 * @file url_file.h
 * @brief URL 文件流处理库头文件
 * 
 * 该模块提供了一个统一的接口来处理基于不同协议（如 HTTP, RTSP, FILE 等）的 URL 资源。
 * 它抽象了底层的网络套接字操作、数据缓冲、播放列表解析以及断点续传等功能，
 * 使得上层应用可以像操作普通文件一样操作网络流。
 */

#ifndef _URL_FILE_H_
#define _URL_FILE_H_

#include "basic_include.h"
#include "lib/common/rbuffer.h"
#include "lib/posix/stdio.h"
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/inet_chksum.h"
#include "netif/etharp.h"
#include "netif/ethernetif.h"
#include "lwip/ip.h"
#include "lwip/init.h"
#include "lib/net/skmonitor/skmonitor.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif

/** @brief 通用调试打印宏 */
#define UF_DEBUG(f,...)      os_printf("%s:%d:"f, __FUNCTION__, __LINE__, ##__VA_ARGS__)
/** @brief 错误日志打印宏 (带内核错误前缀) */
#define UF_ERR(f,...)        os_printf(KERN_ERR"%s:%d:"f, __FUNCTION__, __LINE__, ##__VA_ARGS__)


/** @brief 获取读锁 (阻塞直到获取成功) */
#define UF_LOCK_READ(uf)     os_mutex_lock(&uf->r_lock, osWaitForever)
/** @brief 释放读锁 */
#define UF_UNLOCK_READ(uf)   os_mutex_unlock(&uf->r_lock)

/** @brief 获取写锁 (阻塞直到获取成功) */
#define UF_LOCK_WRITE(uf)    os_mutex_lock(&uf->w_lock, osWaitForever)
/** @brief 释放写锁 */
#define UF_UNLOCK_WRITE(uf)  os_mutex_unlock(&uf->w_lock)

/** @brief 获取协议保护锁 (阻塞直到获取成功) */
#define UF_LOCK_PROTO(uf)    os_mutex_lock(&uf->lock, osWaitForever)
/** @brief 释放协议保护锁 */
#define UF_UNLOCK_PROTO(uf)  os_mutex_unlock(&uf->lock)


/** @brief 检查是否处于读模式 */
#define UF_READ(uf)          ((uf)->mode & UF_MODE_RD)
/** @brief 检查是否处于写模式 */
#define UF_WRITE(uf)         ((uf)->mode & UF_MODE_WR)
/** @brief 检查是否处于 AV (音视频) 模式 */
#define UF_AVMODE(uf)        ((uf)->mode & UF_MODE_AV)

/**
 * @brief URL 文件处理状态
 */
typedef enum {
    UF_CLOSE         = -2, /**< 需要关闭，此状态只能由 uf_close 设置 */
    UF_ERROR         = -1, /**< 发生错误 */
    UF_NONE          =  0, /**< 初始状态 */
    UF_ROPEN         =  1, /**< 重新打开中 */
    UF_OPEN          =  2, /**< 开始打开连接 */
    UF_RUN           =  3, /**< 数据传输中 */
    UF_DONE          =  4  /**< 数据传输完成，处于空闲状态 */
} UF_STATE;

/**
 * @brief URL 文件操作模式位掩码
 */
typedef enum {
    UF_MODE_AV       = BIT(0), /**< AV 模式 (音视频流) */
    UF_MODE_RD       = BIT(1), /**< 支持读取 */
    UF_MODE_WR       = BIT(2), /**< 支持写入 */
    UF_MODE_RECV     = BIT(3), /**< 客户自定义接收模式 (通过回调获取数据) */
    UF_MODE_MONITOR  = BIT(4), /**< 使用 Socket 监控模式 (不创建独立任务) */
    UF_MODE_AUTO_RUN = BIT(5), /**< 自动运行模式 (打开后立即启动传输) */
} UF_MODE;

/**
 * @brief 支持的播放列表格式类型
 */
typedef enum  {
    UF_PLAYLIST_NONE = 0,   /**< 无播放列表 */
    UF_PLAYLIST_CUST,       /**< 自定义格式 */
    UF_PLAYLIST_HLS,        /**< HLS (.m3u8) */
    UF_PLAYLIST_ASX,        /**< ASX (Windows Media) */
    UF_PLAYLIST_MMSREF,     /**< MMS Reference */
    UF_PLAYLIST_M3U,        /**< M3U */
    UF_PLAYLIST_PLS,        /**< PLS */
    UF_PLAYLIST_NSC,        /**< NSC (Netshow) */
    UF_PLAYLIST_SMIL,       /**< SMIL */
    UF_PLAYLIST_XSPF,       /**< XSPF */
    UF_PLAYLIST_MAX,        /**< 类型总数 */
} UF_PLAYLIST_TYPE;

/**
 * @brief IO 控制命令
 */
typedef enum {
    UF_IOCTL_CMD_IDLE,                      /**< 空闲命令 */
    UF_IOCTL_CMD_SOCK_ERROR,                /**< 获取/设置 Socket 错误 */
    UF_IOCTL_CMD_GET_FILESIZE,              /**< 获取文件大小 */
    UF_IOCTL_CMD_GET_TOTALTIME,             /**< 获取总时长 (媒体文件) */
    UF_IOCTL_CMD_SET_EVTCB,                 /**< 设置事件回调函数 */
    UF_IOCTL_CMD_SET_BREAKPOINT_CONTINUALLY,/**< 设置断点续传参数 */
    UF_IOCTL_CMD_SET_HTTP_HEADER,           /**< 设置自定义 HTTP 头 */
    UF_IOCTL_CMD_SET_HTTP_UPSIZE,           /**< 设置 HTTP 上传大小 */
} UF_IOCTL_CMD;

/**
 * @brief URL 文件事件类型
 */
typedef enum {
    UF_EVENT_RECVD_DATA,      /**< 接收到数据事件 (用于 UF_MODE_RECV 模式) */
    UF_EVENT_TRANS_COMPLETE,  /**< 传输完成事件 */
} UF_EVENT;


struct urlfile;

/**
 * @brief 协议层操作函数集
 * 
 * 每个支持的协议 (如 http, rtsp, file) 都需要实现此结构体中的函数指针。
 */
struct ufprotocol_ops {
    /** @brief 初始化协议上下文 */
    int (*do_init)(struct urlfile *file);
    /** @brief 释放协议资源 */
    int (*do_release)(struct urlfile *file);
    /** @brief 执行主传输循环 */
    int (*do_run)(struct urlfile *file);
    /** @brief  seek 操作 (跳转) */
    int (*do_seek)(struct urlfile *file, long offset, int whence);
    /** @brief 发送数据 */
    int (*do_send)(struct urlfile *file, void *data, size_t size);
    /** @brief 协议特定的 IO 控制 */
    int (*do_ioctl)(struct urlfile *file, uint32 cmd, uint32 param1, uint32 param2);
};

/**
 * @brief 协议描述符
 */
struct ufprotocol {
    const char *name;                 /**< 协议名称 (如 "http", "rtsp") */
    const struct ufprotocol_ops *ops; /**< 操作函数集指针 */
};

/**
 * @brief 事件回调函数类型
 * 
 * @param file URL 文件句柄
 * @param evet 事件类型 (@ref UF_EVENT)
 * @param param1 参数 1 (取决于事件类型)
 * @param param2 参数 2 (取决于事件类型)
 * @return int32 处理结果
 */
typedef int32(*uf_evt_cb)(struct urlfile *file, uint32 evet, uint32 param1, uint32 param2);

/**
 * @brief 播放列表条目
 */
struct uf_playlist_entry {
    struct list_head list; /**< 链表节点 */
    char  *title;          /**< 媒体标题 */
    char  *url;            /**< 媒体 URL */
};

/**
 * @brief 播放列表容器
 */
struct uf_playlist {
    uint8 *buff_addr;      /**< 缓冲区地址 (用于存储原始播放列表数据) */
    uint32 buff_size;      /**< 缓冲区总大小 */
    uint32 buff_len;       /**< 当前已填充长度 */
    int16 type;            /**< 播放列表类型 (@ref UF_PLAYLIST_TYPE) */
    uint16 count;          /**< 条目数量 */
    struct list_head  list;/**< 条目链表头 */
};

/**
 * @brief URL 文件核心控制结构
 * 
 * 该结构体维护了一个 URL 资源的所有状态，包括网络连接、缓冲、状态机、播放列表等。
 */
struct urlfile {
    uint8              mode;      /**< 打开模式: 0:AVMODE, BIT0:read; BIT1:write; others:customize */
    int8               state;     /**< URL 文件当前状态 (@ref UF_STATE) */
    uint16             sock;      /**< 关联的 Socket 描述符 */
    char              *url;       /**< 当前打开的 URL 字符串 */
    off_t              size;      /**< 文件总大小。初始为 0，连接成功后设置。若未知 (如直播流) 则设为 -1 */
    off_t              cur_pos;   /**< 当前读取位置 */
    off_t              doffset;   /**< 当前下载偏移量 (用于断点续传) */
    void              *priv;      /**< 协议层私有数据指针 */
    uf_evt_cb          evt_cb;    /**< 事件回调函数指针 */
    struct os_mutex    lock;      /**< 全局互斥锁 (保护主要状态) */
    struct os_mutex    r_lock;    /**< 读操作互斥锁 */
    struct os_mutex    w_lock;    /**< 写操作互斥锁 */
    uint64             open_tick; /**< 打开时的系统 tick 计数 ，用于统计网络打开延时 */

    const struct ufprotocol *proto;   /**< 匹配的协议描述符指针 */
    struct rbuffer      buffer;       /**< 接收数据环形缓冲区 */
    void               *task;         /**< 处理任务句柄 (若非监控模式) */
    struct uf_playlist  playlist;     /**< 播放列表信息 */
    uint16              playindex;    /**< 当前播放的索引 */
    uint8               seek_type;    /**< Seek 类型标志 */
    uint8               rev;          /**< 保留字段 */
};

/* ================= 外部API声明 ================= */
/**
 * @brief 打开 URL 资源
 * 
 * 根据 URL 协议类型创建相应的处理实例，并初始化资源。
 * 
 * @param url 资源 URL 字符串 (如 "http://...", "rtsp://...")
 * @param mode 模式字符串:
 *             - 'v': AV 模式 (音视频流)
 *             - 'r': 支持读取
 *             - 'w': 支持写入
 *             - 'c': 客户自定义接收模式 (不需要调用 uf_read，数据通过 UF_EVENT_RECVD_DATA 回调输出，需配合 UF_IOCTL_CMD_SET_EVTCB)
 *             - 'm': 使用 Socket 监控模式 (不为此 urlfile 创建独立任务)
 *             - 'a': 自动运行模式 (打开后自动开始传输)
 * @param flags 额外标志位
 * @return void* 成功返回 urlfile 句柄，失败返回 NULL
 */
void *uf_open(char *url, char *mode, uint32 flags);

/**
 * @brief 关闭 URL 资源
 * 
 * 释放所有关联资源，断开连接，停止任务。
 * 
 * @param file urlfile 句柄
 */
void uf_close(void *file);

/**
 * @brief 从 URL 资源读取数据
 * 
 * 类似于标准 fread，从内部缓冲区拷贝数据到用户缓冲区。
 * 
 * @param ptr 用户缓冲区指针
 * @param size 单个元素大小
 * @param nmemb 元素个数
 * @param file urlfile 句柄
 * @return size_t 实际读取的元素个数
 */
size_t uf_read(void *ptr, size_t size, size_t nmemb, void *file);

/**
 * @brief 向 URL 资源写入数据
 * 
 * 类似于标准 fwrite，将用户数据发送到远程端。
 * 
 * @param ptr 用户数据指针
 * @param size 单个元素大小
 * @param nmemb 元素个数
 * @param file urlfile 句柄
 * @return size_t 实际写入的元素个数
 */
size_t uf_write(void *ptr, size_t size, size_t nmemb, void *file);

/**
 * @brief 移动读写指针
 * 
 * @param file urlfile 句柄
 * @param offset 偏移量
 * @param whence 起始位置 (SEEK_SET, SEEK_CUR, SEEK_END)
 * @return int 0: 成功; -1: 失败
 */
int uf_seek(void *file, long offset, int whence);

/**
 * @brief 获取当前读写指针位置
 * 
 * @param file urlfile 句柄
 * @return off_t 当前位置
 */
off_t uf_tell(void *file);

/**
 * @brief 检查是否到达文件末尾
 * 
 * @param file urlfile 句柄
 * @return int 1: 到达末尾; 0: 未到达
 */
int uf_eof(void *file);

/**
 * @brief IO 控制接口
 * 
 * 用于设置或获取特定参数 (如回调、HTTP 头、文件大小等)。
 * 
 * @param file urlfile 句柄
 * @param cmd 控制命令 (@ref UF_IOCTL_CMD)
 * @param param1 参数 1
 * @param param2 参数 2
 * @return int 操作结果
 */
int uf_ioctl(void *file, uint32 cmd, uint32 param1, uint32 param2);

/**
 * @brief 获取文件大小
 * 
 * 尝试获取远程文件的总大小。如果当前接收的数据量不足以判断大小，可能返回 0。
 * 
 * @param fp urlfile 句柄
 * @param size 触发检查的最小数据阈值
 * @return off_t 文件总大小；若未知或条件不满足返回 0
 */
off_t uf_filesize(void *fp, uint32 size);

/**
 * @brief 获取 Seek 类型
 * 
 * @param fp urlfile 句柄
 * @return int32 Seek 类型标识
 */
int32 uf_seektype(void *fp);

/**
 * @brief 获取有效数据大小及缓冲水位
 * 
 * @param fp urlfile 句柄
 * @param buffer_level 输出参数：当前缓冲区水位 (百分比)
 * @return off_t 有效数据总大小
 */
off_t uf_datasize(void *fp, uint8 *buffer_level);

/**
 * @brief 分配内存   -- 具体实现在 app_mem.c
 * @param size 请求大小
 * @return void* 内存指针
 */
void *uf_alloc(int32 size);

/**
 * @brief 释放内存   -- 具体实现在 app_mem.c
 * @param ptr 内存指针
 */
void uf_free(void *ptr);

/**
 * @brief 复制字符串
 * @param s 源字符串
 * @return char* 新分配的字符串副本
 */
char *uf_strdup(char *s);

/**
 * @brief urlfile状态切换（内部使用）
 * @param file urlfile 句柄
 * @param state 新状态
 */
void uf_state(struct urlfile *file, UF_STATE state);

/**
 * @brief 初始化 URL 文件系统
 * 
 * 注册支持的协议列表。
 * 
 * @param protos 协议描述符数组
 * @param count 数组元素个数
 */
void urlfile_init(const struct ufprotocol *protos, int count);

/* ================= 协议内部使用的API ================= */
/**
 * @brief 将数据存入内部缓冲区
 * 
 * @param file urlfile 句柄
 * @param data 数据指针
 * @param len 数据长度
 * @return size_t 实际存入的长度
 */
size_t uf_store_data(struct urlfile *file, char *data, size_t len);

/**
 * @brief 解析播放列表数据
 * 
 * @param list 播放列表结构
 * @param data 原始数据
 * @param len 数据长度
 * @param tot_size 总大小
 * @return int 解析到的条目数
 */
int uf_playlist_recieve(struct uf_playlist *list, char *data, int len, uint32 tot_size);

/**
 * @brief 检测播放列表类型
 * 
 * @param list 播放列表结构
 * @param data 数据样本
 * @param size 样本大小
 */
void uf_playlist_detect(struct uf_playlist *list, void *data, size_t size);

/**
 * @brief 构建播放列表
 * 
 * @param list 播放列表结构
 * @param urls 包含 URL 的字符串
 * @return int32 0: 成功; 负值: 失败
 */
int32 uf_playlist_build(struct uf_playlist *list, char *urls);

/**
 * @brief 释放播放列表资源
 * 
 * @param list 播放列表结构
 * @return int32 0: 成功
 */
int32 uf_playlist_free(struct uf_playlist *list);

/**
 * @brief 向播放列表添加新 URL
 * 
 * @param list 播放列表结构
 * @param title 标题
 * @param url URL 字符串
 * @return int32 0: 成功; 负值: 失败
 */
int32 uf_playlist_new_url(struct uf_playlist *list, char *title, char *url);

/**
 * @brief 获取播放列表中指定索引的 URL
 * 
 * @param list 播放列表结构
 * @param index 索引
 * @return char* URL 字符串
 */
char *uf_playlist_get_url(struct uf_playlist *list, uint16 index);

/**
 * @brief 从缓冲区获取下一行文本
 * 
 * @param buf 缓冲区
 * @param line 输出：行起始指针
 * @return char* 下一行的起始位置或 NULL
 */
char *uf_playlist_get_line(char *buf, char **line);


/**
 * @brief 根据 URL 查找匹配的协议处理者
 * 
 * @param url URL 字符串
 * @return const struct ufprotocol* 匹配的协议描述符，未找到返回 NULL
 */
const struct ufprotocol *uf_findproto(char *url);

/* ================= 外部声明 ================= */
/** @brief 外部声明：CURL HTTP 协议操作集 */
extern const struct ufprotocol_ops uf_curl_http;

#ifdef __cplusplus
}
#endif
#endif
