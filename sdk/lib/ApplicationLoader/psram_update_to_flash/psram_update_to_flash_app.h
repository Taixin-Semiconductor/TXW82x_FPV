/**
  ******************************************************************************
  * @file    psram_update_to_flash_app.h
  * @brief   应用侧 PSRAM 固件暂存及二次 Loader 升级请求交接接口。
  * @note    应用侧通过本接口将 APP 固件按节点暂存到 PSRAM，完成后向二次
  *          Loader 发布升级请求，由二次 Loader 将固件写入 Flash。
  ******************************************************************************
  */
#ifndef __PSRAM_UPDATE_TO_FLASH_APP_H__
#define __PSRAM_UPDATE_TO_FLASH_APP_H__

#include "typesdef.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 单个 PSRAM 暂存节点的数据区容量，单位为字节。 */
#define PSRAM_UPDATE_TO_FLASH_APP_BLOCK_BYTES    (100 * 1024U)

#ifndef PSRAM_UPDATE_TO_FLASH_APP_DEBUG
/** @brief 控制本模块调试日志的输出。 */
#define PSRAM_UPDATE_TO_FLASH_APP_DEBUG          (1)
#endif

/**
 * @brief 初始化固件暂存所需的互斥锁和控制区。
 * @note 本接口可重复调用；调用其他暂存接口前应先完成初始化。
 * @return 成功返回 0，失败返回 -1。
 */
int psram_update_to_flash_app_init(void);

/**
 * @brief 开始一次 APP 固件暂存过程。
 * @param image_size 本次固件镜像的总字节数，必须大于 0。
 * @return 成功返回 0，失败返回 -1。
 * @note 开始成功后，必须按固件偏移顺序调用写入接口。
 * @warning 一次暂存过程结束或中止前，不得并发再次调用本接口。
 */
int psram_update_to_flash_app_begin(uint32 image_size);

/**
 * @brief 按顺序写入一段固件数据到 PSRAM 暂存节点。
 * @param data 待写入的固件数据，必须非空。
 * @param data_bytes 本次写入的字节数，必须非零。
 * @return 成功返回 0，失败返回 -1。
 * @note 数据必须按固件偏移顺序写入，累计写入量不得超过 begin 声明的镜像大小。
 */
int psram_update_to_flash_app_write(const uint8 *data, uint32 data_bytes);

/**
 * @brief 完成固件暂存并向二次 Loader 发布升级请求。
 * @param sequence 本次升级请求的序列号。
 * @return 成功返回 0，失败返回 -1。
 * @note 调用前累计写入量必须与 begin 声明的镜像大小精确匹配。
 * @note 成功路径会静默平台、发布升级请求并调用重启跳板，因此通常不会返回。
 */
int psram_update_to_flash_app_finish(uint32 sequence);

/**
 * @brief 中止当前暂存过程并释放相关资源。
 * @return 成功返回 0。
 * @note 本接口会清除控制区和所有已分配节点；未初始化时调用也视为成功。
 */
int psram_update_to_flash_app_abort(void);

/**
 * @brief 平台静默弱符号钩子，应用可按需覆盖实现。
 * @return 成功返回 0，失败返回 -1。
 * @note 用于停止网络、媒体、存储、CPU1 业务及其他 PSRAM 使用者。
 * @warning 静默失败会阻止升级请求发布。
 */
int psram_update_to_flash_app_platform_quiesce(void);

#ifdef __cplusplus
}
#endif

#endif /* __PSRAM_UPDATE_TO_FLASH_APP_H__ */
