/**
 ******************************************************************************
 * @file    ApplicationLoader_ota.h
 * @brief   ApplicationLoader TCP OTA 服务公共接口。
 * @details 定义 TCP 20203 Loader OTA 服务使用的协议命令、超时参数以及
 *          服务启动、状态查询和单连接固件接收接口。
 ******************************************************************************
 */
#ifndef APPLICATION_LOADER_OTA_H
#define APPLICATION_LOADER_OTA_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 客户端发起 Loader 网络升级时使用的请求命令。 */
#define APPLICATION_LOADER_OTA_REQ_NET_UPGRADE   (0xA0U)
/** @brief 设备确认 Loader 网络升级请求时使用的应答命令。 */
#define APPLICATION_LOADER_OTA_ACK_NET_UPGRADE   (0xA1U)
/** @brief Loader TCP OTA 服务固定监听端口。 */
#define APPLICATION_LOADER_OTA_PORT              (20203U)
/** @brief 已连接 Socket 的接收和发送超时时间，单位为毫秒。 */
#define APPLICATION_LOADER_OTA_SOCKET_TIMEOUT_MS (30000)

#ifndef APPLICATION_LOADER_OTA_TCP_DEBUG
/** @brief TCP OTA 调试日志开关，非零表示启用日志。 */
#define APPLICATION_LOADER_OTA_TCP_DEBUG         (1)
#endif

/**
 * @brief 启动 ApplicationLoader TCP OTA 服务。
 * @pre eloop 已完成初始化并允许注册文件描述符事件。
 * @retval 0 服务启动成功，或服务此前已经启动。
 * @retval -1 Socket 创建、绑定、监听或事件注册失败。
 * @note 本函数可重复调用；重复启动不会创建新的监听 Socket。
 */
int ApplicationLoader_ota_server_start(void);

/**
 * @brief 查询 Loader OTA 单连接处理状态。
 * @retval 0 当前没有 Loader OTA 连接正在处理。
 * @retval 非0 当前已有 Loader OTA 连接正在处理。
 */
int ApplicationLoader_ota_get_status(void);

/**
 * @brief 从已连接 TCP Socket 接收并写入 Loader 固件。
 * @param[in] tcp_connect_fd 已建立连接的 TCP Socket 描述符。
 * @retval 0 ACK、固件信息、固件正文和成功结果均处理完成。
 * @retval -1 任一网络、参数、内存或底层 Loader OTA 操作失败。
 * @note 本函数不关闭传入的 Socket；成功后不触发系统重启。
 */
int ApplicationLoader_ota_receive(int tcp_connect_fd);

#ifdef __cplusplus
}
#endif

#endif /* APPLICATION_LOADER_OTA_H */