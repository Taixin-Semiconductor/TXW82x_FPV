#ifndef _OS_MSG_QUEUE_H_
#define _OS_MSG_QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 消息队列结构体 (os_msgqueue)
 * 
 * 消息队列是一种用于线程间通信 (IPC) 的机制，允许一个或多个线程发送固定大小的消息，
 * 并由一个或多个线程接收。
 * 
 * 【核心特性】:
 * 1. 基于指针或整数值 (uint32) 的传递，通常用于传递数据值或指向数据块的指针。
 * 2. 支持阻塞发送/接收 (带超时) 和非阻塞操作。
 * 3. 遵循 FIFO (先进先出) 原则
 */
struct os_msgqueue {
    uint32 magic;       /**< [内部] 魔数，用于校验结构体是否已正确初始化，防止操作未初始化的对象 */
    void  *hdl;         /**< [内部] 底层消息队列句柄 (如 RTOS 的 Queue 指针或内部实现结构) */
};

/**
 * @brief 消息队列类型定义
 */
typedef struct os_msgqueue os_msgqueue_t;

/**
 * @brief 初始化消息队列
 * @param msgq [Out] 指向消息队列结构体的指针
 * @param size [In]  队列容量 (能容纳的最大消息数量，单位：个)
 * @return int32 状态码 (0: 成功, 非0: 失败)
 * 
 * @note:
 * - 分配内部缓冲区以存储 size 个 uint32 类型的消息。
 * - 设置 magic 值。
 * - 必须在首次使用前调用。
 * - 线程安全：应在没有线程访问该队列时调用。
 */
int32 os_msgq_init(os_msgqueue_t *msgq, int32 size);

/**
 * @brief 从队列接收消息 (简化版)
 * @param msgq   [In] 指向消息队列结构体的指针
 * @param tmo_ms [In] 超时时间 (毫秒)
 *                 - 0: 非阻塞模式。若队列空，立即返回错误。
 *                 - >0: 阻塞等待指定时间，直到有消息或超时。
 *                 - -1 / osWaitForever: 永久阻塞直到有消息。
 * @return uint32 接收到的消息数据。
 * 
 * @note:
 * - 【局限性】：此接口无法区分“接收到的消息值为0”和“接收失败”。
 *   如果业务逻辑中合法的消息值包含 0，请使用 os_msgq_get2。
 * - 若失败 (如超时)，通常返回 0，但调用者无法确知是超时还是真的收到了 0。
 */
uint32 os_msgq_get(os_msgqueue_t *msgq, int32 tmo_ms);

/**
 * @brief 从队列接收消息 (增强版，带错误输出)
 * @param msgq   [In] 指向消息队列结构体的指针
 * @param tmo_ms [In] 超时时间 (毫秒，同 os_msgq_get)
 * @param err    [Out] 错误码输出指针。
 *                     - 若成功，*err = 0。
 *                     - 若超时，*err = 特定超时错误码 (如 -ETIMEDOUT)。
 *                     - 若队列无效，*err = 特定错误码。
 * @return uint32 接收到的消息数据。
 * 
 * @note:
 * - 推荐使用的标准接口。通过检查 *err 可以准确判断操作结果。
 * - 即使返回值为 0，只要 *err == 0，就说明成功接收到了值为 0 的消息。
 */
uint32 os_msgq_get2(struct os_msgqueue *msgq, int32 tmo_ms, int32 *err);

/**
 * @brief 向队列末尾发送消息 (FIFO 模式)
 * @param msgq   [In] 指向消息队列结构体的指针
 * @param data   [In] 要发送的消息数据 (uint32)
 * @param tmo_ms [In] 超时时间 (毫秒)
 *                 - 0: 非阻塞。若队列满，立即返回错误。
 *                 - >0: 阻塞等待指定时间，直到有空位或超时。
 *                 - -1 / osWaitForever: 永久阻塞直到有空位。
 * @return int32 状态码 (0: 成功, 非0: 失败/超时)
 * 
 * @note:
 * - 消息被添加到队列的**尾部**。
 * - 遵循先进先出 (FIFO) 原则，最早进入的消息最早被取出。
 */
int32 os_msgq_put(os_msgqueue_t *msgq, uint32 data, int32 tmo_ms);

/**
 * @brief 向队列头部发送消息 (紧急消息/LIFO 模式)
 * @param msgq   [In] 指向消息队列结构体的指针
 * @param data   [In] 要发送的消息数据 (uint32)
 * @param tmo_ms [In] 超时时间 (毫秒，同 os_msgq_put)
 * @return int32 状态码 (0: 成功, 非0: 失败/超时)
 * 
 * @note:
 * - 消息被插入到队列的**头部**。
 * - 该消息将成为下一个被接收者取出的消息 (插队)。
 * - 适用场景：高优先级事件、紧急控制命令等需要优先处理的场景。
 */
int32 os_msgq_put_head(os_msgqueue_t *msgq, uint32 data, int32 tmo_ms);

/**
 * @brief 销毁/删除消息队列
 * @param msgq [In] 指向消息队列结构体的指针
 * @return int32 状态码
 * 
 * @note:
 * - 释放内部缓冲区和底层资源 (hdl)。
 * - 【警告】：调用前必须确保没有线程正在该队列上阻塞等待 (get 或 put)，
 *   否则可能导致这些线程唤醒失败或访问非法内存。
 * - 通常在模块卸载或对象析构时调用。
 */
int32 os_msgq_del(os_msgqueue_t *msgq);

/**
 * @brief 获取队列当前消息数量
 * @param msgq [In] 指向消息队列结构体的指针
 * @return int32 当前队列中挂起的消息数量。
 * 
 * @note:
 * - 返回值为 0 表示队列空。
 * - 返回值为 size (初始化时设定的容量) 表示队列满。
 * - 该操作是原子的，可用于流量监控或调试。
 */
int32 os_msgq_cnt(os_msgqueue_t *msgq);

#ifdef __cplusplus
}
#endif
#endif /* _OS_MSG_QUEUE_H_ */
