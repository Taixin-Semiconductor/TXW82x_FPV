/**
  ******************************************************************************
  * @file    psram_update_to_flash_app_ota.h
  * @brief   TCP OTA 服务公共接口。
  *
  * TCP OTA 服务通过网络接收固件，将固件暂存到 PSRAM，并交由
  * ApplicationLoader 完成后续更新流程。
  ******************************************************************************
  */
#ifndef __PSRAM_UPDATE_TO_FLASH_APP_OTA_H__
#define __PSRAM_UPDATE_TO_FLASH_APP_OTA_H__

#ifdef __cplusplus
extern "C" {
#endif

/** TCP OTA 升级请求命令。 */
#define PSRAM_UPDATE_TO_FLASH_APP_REQ_NET_UPGRADE     (0xA0U)
/** TCP OTA 升级请求的 ACK 命令。 */
#define PSRAM_UPDATE_TO_FLASH_APP_ACK_NET_UPGRADE     (0xA1U)
/** TCP OTA 服务的默认监听端口。 */
#define PSRAM_UPDATE_TO_FLASH_APP_OTA_PORT            (20202)

#ifndef PSRAM_UPDATE_TO_FLASH_APP_OTA_DEBUG
/** TCP OTA 日志开关，非 0 表示启用日志。 */
#define PSRAM_UPDATE_TO_FLASH_APP_OTA_DEBUG           (1)
#endif

/**
 * @brief 启动 TCP OTA 服务。
 *
 * @pre eloop 已完成初始化并处于运行状态。
 * @note 重复启动视为成功。
 *
 * @retval 0 启动成功或服务已经启动。
 * @retval -1 启动失败。
 */
int psram_update_to_flash_app_ota_server_start(void);

/**
 * @brief 获取 TCP OTA 服务的处理状态。
 *
 * @retval 0 当前空闲。
 * @retval 非0 正在处理升级连接。
 */
int psram_update_to_flash_app_ota_get_status(void);

/**
 * @brief 通过已连接的 TCP Socket 接收 OTA 固件。
 *
 * 函数依次发送升级 ACK，接收固件信息和固件正文，并回复处理结果，
 * 但不负责关闭传入的 Socket。
 *
 * @param[in] tcp_connect_fd 已建立连接的 TCP Socket 描述符。
 *
 * @retval 0 接收及处理成功。
 * @retval -1 接收或处理失败。
 *
 * @note 成功时通常会触发底层 finish 流程并重启系统。
 */
int psram_update_to_flash_app_ota_receive(int tcp_connect_fd);

#ifdef __cplusplus
}
#endif

#endif /* __PSRAM_UPDATE_TO_FLASH_APP_OTA_H__ */
