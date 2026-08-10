#ifndef __OS_CONDV_H
#define __OS_CONDV_H

#include "osal/atomic.h"
#include "osal/mutex.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 条件变量结构体 (os_condv)
 * 
 * 条件变量是一种同步机制，允许线程在某个条件不满足时挂起（等待），
 * 直到另一个线程改变了该条件并发出信号唤醒它。
 * 
 * 【核心特性】：
 * 1. 必须与互斥锁 (Mutex) 配合使用，以保护共享数据和避免竞态条件。
 * 2. 内部维护一个等待计数器 (waitings)，用于追踪当前有多少线程在等待。
 * 3. 底层通常基于信号量 (sema) 实现阻塞唤醒。
 */
struct os_condv {
    uint32 magic;         /**< [内部] 魔数，用于校验结构体是否已正确初始化，防止操作未初始化的对象 */
    atomic_t waitings;    /**< [内部] 原子计数器，记录当前正在调用 os_condv_wait 且处于阻塞状态的线程数量 */
    void    *sema;        /**< [内部] 底层信号量句柄。当有线程等待时，信号量计数为0；signal/broadcast 时增加计数以唤醒线程 */
};

/**
 * @brief 条件变量类型定义
 */
typedef struct os_condv os_condv_t;

/**
 * @brief 初始化条件变量
 * @param cond [Out] 指向条件变量结构体的指针
 * @return int32 状态码 (0: 成功, 非0: 失败)
 * 
 * @note:
 * - 必须在首次使用前调用。
 * - 会设置 magic 值，初始化 waitings 为 0，并创建内部的信号量。
 * - 线程安全：应在没有线程等待该条件变量时调用。
 */
int32 os_condv_init(os_condv_t *cond);

/**
 * @brief 广播唤醒所有等待线程
 * @param cond [In] 指向已初始化的条件变量指针
 * @return int32 状态码
 * 
 * @note:
 * - 唤醒 **所有** 当前正在该条件变量上等待的线程。
 * - 被唤醒的线程将尝试重新获取关联的互斥锁，获取成功后从 os_condv_wait 返回。
 * - 适用场景：当共享资源的状态发生全局性变化（如缓冲区从空变为非空，或多个读者可同时读取）时使用。
 * - 性能提示：比 signal 开销大，因为要唤醒多个线程，但能避免“惊群效应”之外的漏唤醒问题。
 */
int32 os_condv_broadcast(os_condv_t *cond);

/**
 * @brief 唤醒单个等待线程
 * @param cond [In] 指向已初始化的条件变量指针
 * @return int32 状态码
 * 
 * @note:
 * - 唤醒 **至少一个** 当前正在该条件变量上等待的线程（如果有线程在等待）。
 * - 如果有多个线程在等待，具体唤醒哪一个取决于调度策略（通常是优先级最高或等待最久的）。
 * - 适用场景：当资源一次只能被一个消费者处理时（如单生产者-单消费者模型）。
 * - 若无线程等待，此调用通常无效（信号丢失），但这在逻辑上是安全的。
 */
int32 os_condv_signal(os_condv_t *cond);

/**
 * @brief 销毁条件变量
 * @param cond [In] 指向条件变量结构体的指针
 * @return int32 状态码
 * 
 * @note:
 * - 释放内部资源（如销毁信号量）。
 * - 【警告】：调用前必须确保 **没有** 任何线程正在该条件变量上等待 (waitings == 0)。
 *   否则可能导致未定义行为或资源泄漏。
 * - 通常在模块卸载或对象析构时调用。
 */
int32 os_condv_del(os_condv_t *cond);

/**
 * @brief 等待条件变量成立
 * @param cond     [In] 指向已初始化的条件变量指针
 * @param mutex    [In] 与条件变量配合使用的互斥锁指针
 * @param tmo_ms   [In] 超时时间 (毫秒)
 *                   - 0: 立即返回（非阻塞检查，视具体实现而定，通常建议设为 osWaitForever 表示无限等待）
 *                   - osWaitForever (或特定宏): 永久等待直到被唤醒
 *                   - >0: 等待指定毫秒数，若超时仍未被唤醒则返回超时错误
 * @return int32 状态码
 *         - 0: 成功被唤醒 (收到 signal 或 broadcast)
 *         - 非0: 超时或其他错误
 * 
 * @note 【标准使用范式】:
 * 此函数是原子操作，执行流程如下：
 * 1. 自动释放传入的 mutex (让其他线程有机会修改共享数据并发出信号)。
 * 2. 将当前线程挂起，加入 cond 的等待队列，waitings 计数加 1。
 * 3. 等待直到：
 *    a) 被 os_condv_signal 或 os_condv_broadcast 唤醒。
 *    b) 等待超时 (tmo_ms 到期)。
 * 4. 被唤醒或超时后，在返回前 **自动重新获取** mutex。
 * 
 */
int32 os_condv_wait(os_condv_t *cond, os_mutex_t *mutex, uint32 tmo_ms);

#ifdef __cplusplus
}
#endif
#endif /* __OS_CONDV_H */
