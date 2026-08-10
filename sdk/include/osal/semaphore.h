#ifndef __OS_SEMAPHORE_H
#define __OS_SEMAPHORE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 信号量结构体 (os_semaphore)
 * 
 * 信号量是一种用于控制对共享资源访问的同步原语，也可以用于线程间通信。
 * 它维护一个非负整数计数器：
 * - **计数器 > 0**: 表示有可用资源，线程获取信号量时计数器减 1，继续执行。
 * - **计数器 == 0**: 表示无可用资源，线程获取信号量时将被阻塞，直到计数器变为正数。
 * 
 * 【常见用途】:
 * 1. **二值信号量 (Binary Semaphore)**: 初始值为 1。用于互斥锁（虽不如 Mutex 安全）或任务同步（如中断通知任务）。
 * 2. **计数信号量 (Counting Semaphore)**: 初始值 > 1。用于控制同时访问某类资源的最大线程数（如线程池、缓冲区槽位）。
 */
struct os_semaphore {
    uint32 magic;       /**< [内部] 魔数，用于校验结构体是否已正确初始化，防止操作未初始化的对象 */
    void  *hdl;         /**< [内部] 底层信号量句柄 (如 RTOS 的 Semaphore 指针或内部实现结构) */
};

/**
 * @brief 信号量类型定义
 */
typedef struct os_semaphore os_semaphore_t;

/**
 * @brief 初始化信号量
 * @param sem [Out] 指向信号量结构体的指针
 * @param val [In]  信号量的初始计数值
 *                 - 1: 创建二值信号量 (常用于同步或简单互斥)。
 *                 - N: 创建计数信号量 (允许最多 N 个线程同时通过)。
 *                 - 0: 创建初始不可用的信号量 (通常用于等待某个事件发生后由其他线程 Up)。
 * @return int32 状态码 (0: 成功, 非0: 失败)
 * 
 * @note:
 * - 创建底层信号量对象并设置初始计数。
 * - 设置 magic 值。
 * - 必须在首次使用前调用。
 */
int32 os_sema_init(os_semaphore_t *sem, int32 val);

/**
 * @brief 销毁/删除信号量
 * @param sem [In] 指向信号量结构体的指针
 * @return int32 状态码
 * 
 * @note:
 * - 释放底层资源 (hdl)。
 * - 【警告】：调用前必须确保没有线程正在该信号量上等待 (Down)，
 *   否则可能导致这些线程唤醒失败或访问非法内存。
 * - 通常在模块卸载或对象析构时调用。
 */
int32 os_sema_del(os_semaphore_t *sem);

/**
 * @brief 获取信号量 (P 操作 / Wait / Down)
 * 
 * 尝试减少信号量的计数值。
 * 
 * @param sem    [In] 指向信号量结构体的指针
 * @param tmo_ms [In] 超时时间 (毫秒)
 *                 - 0: 非阻塞模式。若计数为 0，立即返回错误。
 *                 - >0: 阻塞等待指定时间。若在此期间计数变为正数，则获取成功；否则超时返回。
 *                 - -1 / osWaitForever: 永久阻塞，直到获取成功。
 * @return int32 状态码
 *         - 1: 成功获取 (计数值已减 1)。
 *         - 0: 失败 (超时、无效句柄等)。
 * 
 * @note:
 * - 原子操作：检查计数值和减 1 是原子进行的。
 * - 若计数值 > 0：计数值减 1，函数立即返回成功。
 * - 若计数值 == 0：当前线程进入阻塞状态，挂入等待队列。
 */
int32 os_sema_down(os_semaphore_t *sem, int32 tmo_ms);

/**
 * @brief 释放信号量 (V 操作 / Signal / Up)
 * 
 * 增加信号量的计数值，可能唤醒等待的线程。
 * 
 * @param sem [In] 指向信号量结构体的指针
 * @return int32 状态码 (0: 成功, 非0: 失败)
 * 
 * @note:
 * - 原子操作：增加计数值和唤醒线程是原子进行的。
 * - 若有线程在等待队列中：唤醒优先级最高的一个线程（该线程会将计数值减 1，因此计数值可能保持为 0）。
 * - 若无线程等待：计数值加 1。
 * - **无所有权限制**：任何线程都可以调用 Up，这与 Mutex 不同（Mutex 只能由持有者解锁）。
 * - 适用于中断上下文。
 */
int32 os_sema_up(os_semaphore_t *sem);

/**
 * @brief 获取当前信号量计数值
 * @param sem [In] 指向信号量结构体的指针
 * @return int32 当前的计数值。
 * 
 * @note:
 * - 返回值为 0 表示资源不可用（或有线程在等待且无剩余配额）。
 * - 该操作通常是原子的，可用于调试或流量监控。
 * - 注意：返回值仅代表调用瞬间的状态，下一秒可能就会变化。
 */
int32 os_sema_count(os_semaphore_t *sem);

/**
 * @brief 非阻塞消耗信号量直到信号量为0 (Try Down / Eat)
 * 
 * @param sem [In] 指向信号量结构体的指针
 * 
 * @note:
 * - 该API通常是在需要放弃并清理信号时使用
 */
void os_sema_eat(os_semaphore_t *sem);

#ifdef __cplusplus
}
#endif
#endif /* __OS_SEMAPHORE_H */
