/**
 * @file vfs_fatfs.h
 * @brief FatFS VFS 适配层头文件
 */

#ifndef VFS_FATFS_H
#define VFS_FATFS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 注册 FatFS 到 VFS
 *
 * 在调用 vfs_init() 之后、vfs_mount() 之前调用此函数
 * 注册 "fatfs" 文件系统类型到 VFS 框架。
 *
 * @return VFS_OK 成功，负数为错误码
 */
int vfs_fatfs_register(void);

#ifdef __cplusplus
}
#endif

#endif /* VFS_FATFS_H */
