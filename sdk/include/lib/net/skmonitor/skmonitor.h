/**
 * @file skmonitor.h
 * @brief Socket 集中监控器 (Socket Monitor)
 * 
 * @par 模块概述
 * 本模块旨在为**小数据量通信**或**低使用频率**的 Socket 提供一种轻量级的集中监测机制。
 * SDK的dhcpd，uhttpd模块都是基于skmonitor而运行的。
 * 
 * @par 设计优势
 * - **节省内存开销**：无需为每个此类 Socket 单独创建监测任务 (Task)，所有 Socket 共享一个监控中心。
 * - **事件驱动**：当监测到 Socket 有数据可读、可写或发生错误时，自动通过回调函数 (Callback) 触发用户注册的处理逻辑。
 * 
 * @par 核心约束 (重要)
 * - **独占读取权**：一旦 Socket 被注册到 skmonitor，**严禁**在所属的业务任务 (Task) 中主动调用 read/recv 等函数进行读取。
 * - **回调执行**：所有的数据读取和处理操作，必须且只能在 skmonitor 触发的回调函数中执行。
 *   违反此规则可能导致数据竞争、事件丢失或程序崩溃。
 */

#ifndef _SDK_SKMONITOR_H_
#define _SDK_SKMONITOR_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Socket 监控事件标志位
 * 
 * 用于指定需要监控的 Socket 事件类型，支持按位或 (|) 组合。
 */
typedef enum {
    SOCK_MONITOR_READ   = BIT(0),  /**< 监控可读事件 (接收到数据、连接关闭等) */
    SOCK_MONITOR_WRITE  = BIT(1),  /**< 监控可写事件 (发送缓冲区空闲) */
    SOCK_MONITOR_ERROR  = BIT(2),  /**< 监控异常事件 (连接错误、复位等) */
} skmonitor_flags;

/**
 * @brief Socket 事件回调函数类型定义
 * 
 * 当注册的 Socket 发生指定事件时，系统将调用此函数。
 * 
 * @param sock 触发事件的 Socket 句柄
 * @param flags 当前发生的事件标志 (@ref skmonitor_flags)
 * @param priv 用户私有数据，在注册时传入，用于传递上下文信息
 */
typedef void (*skmonitor_cb)(uint16 sock, skmonitor_flags flags, uint32 priv);

/**
 * @brief 初始化 Socket 监控器
 * 
 * 系统启动时需调用一次，初始化内部数据结构及监控任务。
 * 
 * @return int32 0: 成功; 负值: 失败
 */
int32 sock_monitor_init(void);

/**
 * @brief 打印监控状态信息
 * 
 * 用于调试，输出当前所有已注册 Socket 的状态列表。
 */
void sock_monitor_dump(void);

/**
 * @brief 添加 Socket 到监控列表
 * 
 * 注册一个 Socket 并设定关注的事件。注册成功后，该 Socket 将由 skmonitor 统一管理。
 * 
 * @param sock 待监控的 Socket 句柄
 * @param flags 关注的事件类型 (如 @ref SOCK_MONITOR_READ)
 * @param cb 事件触发时的回调函数
 * @param priv 用户私有数据指针
 * 
 * @return int32 0: 成功; 负值: 失败
 * 
 * @warning 
 * **关键限制**：注册后，业务任务**不得**再主动读取该 Socket。
 * 所有读取操作必须由 skmonitor 检测到数据后，通过回调函数 @p cb 触发执行。
 */
int32 sock_monitor_add(uint16 sock, skmonitor_flags flags, skmonitor_cb cb, uint32 priv);

/**
 * @brief 从监控列表中移除 Socket
 * 
 * 停止对指定 Socket 的监控，释放相关资源。移除后，业务任务可恢复对该 Socket 的自主控制。
 * 
 * @param sock 待移除的 Socket 句柄
 */
void sock_monitor_del(uint16 sock);

/**
 * @brief 禁用或启用 Socket 监控
 * 
 * 临时挂起或恢复对特定 Socket 的事件响应，而不必将其从列表中移除。
 * 
 * @param sock 目标 Socket 句柄
 * @param disable 
 *        - 1: 禁用监控 (暂停回调)
 *        - 0: 启用监控 (恢复回调)
 */
void sock_monitor_disable(uint16 sock, uint8 disable);

#ifdef __cplusplus
}
#endif

#endif /* _SDK_SKMONITOR_H_ */
