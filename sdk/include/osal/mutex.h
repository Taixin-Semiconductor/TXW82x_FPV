#ifndef _OS_MUTEX_H_
#define _OS_MUTEX_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 互斥锁结构体 (os_mutex)
 * 
 * 互斥锁 (Mutex, Mutual Exclusion) 是一种用于保护共享资源的同步原语。
 * 它确保在任何时刻，只有一个线程可以持有该锁并访问临界区代码。
 * 
 * 【核心特性】:
 * 1. **独占性**: 一旦某个线程锁住了 mutex，其他试图锁住它的线程将被阻塞。
 * 2. **所有权**: 锁与持有它的线程绑定。只有持有锁的线程才能解锁它（防止误解锁）。
 */
struct os_mutex {
    uint32 magic;       /**< [内部] 魔数，用于校验结构体是否已正确初始化，防止操作未初始化的对象 */
    void  *hdl;         /**< [内部] 底层互斥锁句柄 (如 RTOS 的 Mutex 指针或内部实现结构) */
};

/**
 * @brief 互斥锁类型定义
 */
typedef struct os_mutex os_mutex_t;

/**
 * @brief 初始化互斥锁
 * @param mutex [Out] 指向互斥锁结构体的指针
 * @return int32 状态码 (0: 成功, 非0: 失败)
 * 
 * @note:
 * - 创建底层锁对象，初始状态为“未锁定” (Unlocked)。
 * - 设置 magic 值。
 * - 必须在首次使用前调用。
 * - 线程安全：应在没有线程访问该锁时调用。
 */
int32 os_mutex_init(os_mutex_t *mutex);

/**
 * @brief 获取互斥锁 (加锁)
 * @param mutex [In] 指向互斥锁结构体的指针
 * @param tmo   [In] 超时时间 (毫秒或系统 Tick，视具体实现定义)
 *              - 0: 非阻塞模式。若锁已被占用，立即返回错误 (如 -EBUSY)。
 *              - >0: 阻塞等待指定时间。若在此期间锁被释放，则获取成功；否则超时返回。
 *              - -1 / OS_WAIT_FOREVER: 永久阻塞，直到成功获取锁。
 * @return int32 状态码
 *         - 0: 成功获取锁。当前线程成为锁的持有者 (Owner)。
 *         - 非0: 失败 (超时、无效句柄等)。
 * 
 * @note:
 *   支持重入：同一个task多次调用lock时，内部会维护计数，需调用相同次数的 unlock 才能真正释放。
 * - **临界区**: 成功返回后，线程进入临界区，应尽快完成共享资源操作并解锁。
 */
int32 os_mutex_lock(os_mutex_t *mutex, int32 tmo);

/**
 * @brief 释放互斥锁 (解锁)
 * @param mutex [In] 指向互斥锁结构体的指针
 * @return int32 状态码
 *         - 0: 成功释放。
 *         - 非0: 失败 (例如：当前线程并非锁的持有者，或锁未初始化)。
 * 
 * @note:
 * - **所有权检查**: 只有当前持有锁的线程才能调用此函数。
 *   若由其他线程调用，通常返回错误且锁状态不变，以防止逻辑混乱。
 * - **唤醒**: 解锁后，若有其他线程在等待该锁，系统会根据调度策略唤醒其中一个。
 * - **配对使用**: 必须与 os_mutex_lock 成对出现，避免死锁或资源泄露。
 */
int32 os_mutex_unlock(os_mutex_t *mutex);

/**
 * @brief 销毁/删除互斥锁
 * @param mutex [In] 指向互斥锁结构体的指针
 * @return int32 状态码
 * 
 * @note:
 * - 释放底层资源 (hdl)。
 * - 【严重警告】:
 *   1. 调用前必须确保 **没有** 任何线程正在持有该锁。
 *   2. 必须确保 **没有** 任何线程正在等待 (阻塞在 lock 调用中) 该锁。
 *   违反上述任一条件将导致未定义行为 (如持有者访问非法内存、等待者永久挂起)。
 * - 通常在模块卸载或对象析构时调用。
 */
int32 os_mutex_del(os_mutex_t *mutex);

/**
 * @brief 获取当前锁的持有者 (Owner)
 * @param mutex [In] 指向互斥锁结构体的指针
 * @return void* 当前持有该锁的线程handler
 * 
 * @note:
 * - 主要用于调试、死锁检测或监控模块。
 * - 返回值的具体类型取决于底层 OS 实现 (可能是 os_task_t* 或 uint32 task_id)。
 * - 此操作通常是只读的，不会改变锁的状态。
 */
void *os_mutex_owner(os_mutex_t *mutex);

#ifdef __cplusplus
}
#endif

#endif /* _OS_MUTEX_H_ */
