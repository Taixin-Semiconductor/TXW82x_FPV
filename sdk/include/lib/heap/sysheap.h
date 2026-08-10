#ifndef _SYS_HEAP_H_
#define _SYS_HEAP_H_

#include "lib/heap/mmpool.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 系统堆管理器的配置标志位枚举
 * 
 * 这些标志用于在初始化堆时控制内存池的行为，如开启泄露检测、溢出检查或对齐方式。
 */
enum SYSHEAP_FLAGS {
    SYSHEAP_FLAGS_MEM_LEAK_TRACE     = (1u << 0),  ///< 开启内存泄漏追踪：记录分配调用栈，用于检测未释放的内存
    SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK = (1u << 1),  ///< 开启内存溢出检查：在分配块前后添加保护模式，检测越界写入
    SYSHEAP_FLAGS_MEM_ALIGN_16       = (1u << 2),  ///< 强制内存分配 16 字节对齐
    SYSHEAP_FLAGS_MEM_ALIGN_32       = (1u << 3),  ///< 强制内存分配 32 字节对齐
    SYSHEAP_FLAGS_MEM_TAIL_ALIGN_4   = (1u << 4),  ///< 强制内存块尾部 4 字节对齐 (可能用于特定 DMA 或硬件要求)
    SYSHEAP_FLAGS_MEM_TAIL_ALIGN_16  = (1u << 5),  ///< 强制内存块尾部 16 字节对齐
    SYSHEAP_FLAGS_MEM_TAIL_ALIGN_32  = (1u << 6),  ///< 强制内存块尾部 32 字节对齐
};

/**
 * @brief 通用系统堆结构体。
 *        __malloc/__free 通过sys_heap类型来访问具体的内存池，
 *        所以各个特定内存池的定义保持和 sys_heap 一致。
 */
struct sys_heap {
    const char *name;                ///< 堆的名称标识，用于调试打印
    const struct mmpool_ops *ops;    ///< 内存池操作函数表指针，定义具体的分配/释放算法
    struct mmpool_base pool;         ///< 内存池的基础实例数据
};

/**
 * @brief SRAM 内存池
 * 
 * SDK默认创建了 SRAM内存池。
 * 注意：当前使用 mmpool1 实现，如需切换算法，需修改此处类型及初始化函数中的 ops 赋值。
 */
struct sys_sramheap {
    const char *name;                ///< 堆名称
    const struct mmpool_ops *ops;    ///< 操作函数表
    struct mmpool1 pool;             ///< 具体的内存池实例 (类型可改为 mmpool3 以切换算法)
    // 提示: 若改为 mmpool3，需同步修改 malloc_init 函数: sram_heap.ops = &mmpool3_ops;
};

/**
 * @brief PSRAM 内存池
 * 
 * SDK默认创建了 PSRAM内存池。
 * 注意：当前使用 mmpool1 实现，支持切换到 mmpool3。
 */
struct sys_psramheap {
    const char *name;                ///< 堆名称
    const struct mmpool_ops *ops;    ///< 操作函数表
    struct mmpool1 pool;             ///< 具体的内存池实例
    // 提示: 若改为 mmpool3，需同步修改 malloc_psram_init 函数: psram_heap.ops = &mmpool3_ops;
};

/**
 * @brief AV (Audio/Video) 专用内存池
 *        如果需要使用AV heap，则需要确认SDK是否对AV Heap进行了初始化。
 * 注意：当前使用 mmpool1 实现，支持切换到 mmpool3。
 */
struct sys_av_heap {
    const char *name;                ///< 堆名称
    const struct mmpool_ops *ops;    ///< 操作函数表
    struct mmpool1 pool;             ///< 具体的内存池实例
    // 提示: 若改为 mmpool3，需同步修改对应的初始化函数中的 ops 赋值
};

/* ==========================================================================
 * 底层核心分配函数 (带调用者返回地址追踪)
 * 参数 'lr' 通常传入链接寄存器 (Link Register) 的值，用于定位调用者位置
 * ========================================================================== */
void *__malloc(struct sys_heap *heap, int size, void *lr);                  ///< 分配内存
void __free(struct sys_heap *heap, void *ptr, void *lr);                    ///< 释放内存
void *__zalloc(struct sys_heap *heap, int size, void *lr);                  ///< 分配并清零内存
void *__realloc(struct sys_heap *heap, void *ptr, int size, void *lr);   ///< 重新调整内存大小

/* ==========================================================================
 * 内部实现函数 (以下划线开头，建议通过下方宏调用)
 * ========================================================================== */

/**
 * @brief 初始化堆管理器
 * @param heap 堆结构体指针
 * @param heap_start 内存池起始物理地址
 * @param heap_size 内存池总大小
 * @param flags 配置标志位 (见 SYSHEAP_FLAGS)
 * @return 0 表示成功，负值表示失败
 */
int32 _sysheap_init(struct sys_heap *heap, void *heap_start, uint32 heap_size, uint32 flags);

/**
 * @brief 从堆中分配内存 (带调试信息)
 * @param heap 堆指针
 * @param size 请求大小
 * @param func 调用函数名 (用于调试追踪)
 * @param line 调用行号 (用于调试追踪)
 * @return 成功返回内存指针，失败返回 NULL
 */
void *_sysheap_alloc(struct sys_heap *heap, int size, void *lr, uint32 attr);

/**
 * @brief 释放堆内存
 * @param heap 堆指针
 * @param ptr 要释放的指针
 * @return 0 表示成功
 */
int32 _sysheap_free(struct sys_heap *heap, void *ptr);

/**
 * @brief 获取堆当前剩余可用大小
 */
uint32 _sysheap_freesize(struct sys_heap *heap);

/**
 * @brief 获取堆总容量
 */
uint32 _sysheap_totalsize(struct sys_heap *heap);

/**
 * @brief 将 init 段占用的内存空间添加到指定的内存池，作为通用内存使用。
 */
void _sysheap_collect_init(struct sys_heap *heap);

/**
 * @brief 向内存池添加一块新的内存区域
 * @param start_addr 新区域起始地址
 * @param end_addr 新区域结束地址
 */
int32 _sysheap_add(struct sys_heap *heap, uint32 start_addr, uint32 end_addr);

/**
 * @brief 检查内存溢出 (Overflow Check)
 * @param ptr 检查的指针
 * @param size 原始分配大小
 * @return 0 表示正常，非 0 表示检测到溢出
 */
int32 _sysheap_of_check(struct sys_heap *heap, void *ptr, uint32 size);

/**
 * @brief 获取堆状态统计信息
 * @param status_buf 输出缓冲区
 * @param buf_size 缓冲区大小
 * @param mini_size 统计的最小块大小阈值
 */
void _sysheap_status(struct sys_heap *heap, uint32 *status_buf, int32 buf_size, uint32 mini_size);

/**
 * @brief 验证地址是否属于该堆且有效
 * @param ptr 待验证地址
 * @param first 是否只检查第一个块 (特定逻辑)
 * @return 1 表示有效，0 表示无效
 */
int32 _sysheap_valid_addr(struct sys_heap *heap, void *ptr, uint8 first);

/**
 * @brief 获取内存池统计的 alloc/free API执行时间
 */
uint32 _sysheap_time(struct sys_heap *heap);

/**
 * @brief 获取已分配内存块的列表 (用于泄露分析)
 * @param list_buf 输出列表缓冲区
 * @param buf_size 缓冲区大小
 * @return 填充的条目数量
 */
int32 _sysheap_used_list(struct sys_heap *heap, uint32 *list_buf, int32 buf_size);

/**
 * @brief 打印堆的详细调试信息到控制台/日志
 */
void _sysheap_dump(struct sys_heap *heap);

/* ==========================================================================
 * 公共宏接口
 * 自动进行类型转换并隐藏内部实现细节，推荐应用层使用这些宏
 * ========================================================================== */

/// 初始化堆
#define sysheap_init(heap, heap_start, heap_size, flags) \
    _sysheap_init((struct sys_heap *)(heap), heap_start, heap_size, flags)

/// 获取内存池统计的 alloc/free API执行时间
#define sysheap_time(heap)\
    _sysheap_time((struct sys_heap *)(heap))

/// 分配内存 (自动填入调用位置信息)
/// 用法: sysheap_alloc(&sram_heap, 1024, __func__, __LINE__);
#define sysheap_alloc(heap, size, lr, attr)\
    _sysheap_alloc((struct sys_heap *)(heap), size, lr, attr)

/// 释放内存
#define sysheap_free(heap, ptr)\
    _sysheap_free((struct sys_heap *)(heap), ptr)

/// 获取剩余空间
#define sysheap_freesize(heap)\
    _sysheap_freesize((struct sys_heap *)(heap))

/// 获取总空间
#define sysheap_totalsize(heap)\
    _sysheap_totalsize((struct sys_heap *)(heap))

/// 将 init 段占用的内存空间添加到指定的内存池，作为通用内存使用。
#define sysheap_collect_init(heap)\
    _sysheap_collect_init((struct sys_heap *)(heap))

/// 添加内存区域
#define sysheap_add(heap, start_addr, end_addr)\
    _sysheap_add((struct sys_heap *)(heap), start_addr, end_addr)

/// 执行溢出检查
#define sysheap_of_check(heap, ptr, size)\
    _sysheap_of_check((struct sys_heap *)(heap), ptr, size)

/// 获取状态统计
#define sysheap_status(heap, status_buf, buf_size, mini_size)\
    _sysheap_status((struct sys_heap *)(heap), status_buf, buf_size, mini_size)

/// 验证地址有效性
#define sysheap_valid_addr(heap, ptr, first)\
    _sysheap_valid_addr((struct sys_heap *)(heap), ptr, first)

/// 获取已用内存列表
#define sysheap_used_list(heap, list_buf, buf_size)\
    _sysheap_used_list((struct sys_heap *)(heap), list_buf, buf_size)

/// 打印堆信息
#define sysheap_dump(heap)\
    _sysheap_dump((struct sys_heap *)(heap))

/* ==========================================================================
 * SDK默认定义的 内存池
 * 如果需要创建自定义的内存池，请阅读 sdk/lib/heap/xxx_heap.c, sdk/lib/heap/xxx_heap.h 文件
 * ========================================================================== */
extern struct sys_sramheap  sram_heap;   ///< SDK 预定义的 SRAM 内存池
extern struct sys_psramheap psram_heap;  ///< SDK 预定义的 PSRAM 内存池

////////////////////////////////////////////////////////////////////////////////////////////
// 内存分配句柄，用于代码模块自定义内存分配方式
// void *priv 参数用于传递 自定义信息
typedef void *(*malloc_cb_t)(uint32 size, void *priv);
typedef void  (*mfree_cb_t)(void *ptr, void *priv);
typedef struct {
    malloc_cb_t alloc;
    mfree_cb_t  free;
} mem_allocer_t;

#endif /* _SYS_HEAP_H_ */

#ifdef __cplusplus
}
#endif
