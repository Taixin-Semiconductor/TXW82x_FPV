#ifndef _TXSDK_VFS_H_
#define _TXSDK_VFS_H_

#include "basic_include.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * 配置宏（用户可预先定义）
 *============================================================================*/
#ifndef VFS_MAX_MOUNTS
#define VFS_MAX_MOUNTS     4
#endif

#ifndef VFS_MAX_FILESYSTEMS
#define VFS_MAX_FILESYSTEMS 4
#endif

#ifndef VFS_PATH_MAX
#define VFS_PATH_MAX        128
#endif

#ifndef VFS_NAME_MAX
#define VFS_NAME_MAX        64
#endif

#ifndef VFS_MUTEX_ENABLED
/* 是否启用线程安全（全局锁 + per-mount锁） */
#define VFS_MUTEX_ENABLED 1
#endif

/*=============================================================================
 * VFS 错误码（直接使用 errno.h 定义）
 * 返回值约定：0 表示成功，负 errno 表示错误
 *============================================================================*/
#define VFS_OK              0

/*=============================================================================
 * 文件打开标志（与POSIX兼容子集）
 *============================================================================*/
#define VFS_O_RDONLY        0x0000
#define VFS_O_WRONLY        0x0001
#define VFS_O_RDWR          0x0002
#define VFS_O_CREAT         0x0100
#define VFS_O_APPEND        0x0008
#define VFS_O_TRUNC         0x0200

/* 文件寻址位置 */
#define VFS_SEEK_SET        0
#define VFS_SEEK_CUR        1
#define VFS_SEEK_END        2

/*=============================================================================
 * 类型定义
 *============================================================================*/

typedef long vfs_off_t;
typedef long vfs_ssize_t;

/* 使用系统 struct stat */
#include "lib/posix/stdio.h"

/* 文件模式位（兼容定义） */
#define VFS_S_IFDIR         S_IFDIR
#define VFS_S_IFREG         S_IFREG
#define VFS_S_IFMT          (S_IFDIR | S_IFREG)

/* 目录项结构（类似dirent） */
struct vfs_dirent {
    char d_name[VFS_NAME_MAX];
    uint8_t d_type;
};

#define VFS_DT_UNKNOWN      0
#define VFS_DT_REG          1
#define VFS_DT_DIR          2

/* 前向声明 */
struct vfs_file_desc;

/*=============================================================================
 * 文件系统操作接口（需由具体FS实现）
 *============================================================================*/

/* 文件操作函数集 */
struct vfs_file_ops {
    int     (*open)(void *fs_ctx, const char *path, int flags, int mode, void **file_priv);
    int     (*close)(void *file_priv);
    vfs_ssize_t (*read)(void *file_priv, void *buf, size_t size);
    vfs_ssize_t (*write)(void *file_priv, const void *buf, size_t size);
    vfs_off_t   (*lseek)(void *file_priv, vfs_off_t offset, int whence);
    vfs_ssize_t (*fsize)(void *file_priv);
    vfs_ssize_t (*ftell)(void *file_priv);
    int     (*sync)(void *file_priv);
    int     (*eof)(void *file_priv);
    int     (*stat)(void *fs_ctx, const char *path, struct stat *st);
    int     (*unlink)(void *fs_ctx, const char *path);
    int     (*rename)(void *fs_ctx, const char *old, const char *new);
    int     (*truncate)(void *file_priv, vfs_off_t length);

    /* 目录操作 */
    void*   (*opendir)(void *fs_ctx, const char *path);
    int     (*readdir)(void *dir_priv, struct vfs_dirent *ent);
    int     (*closedir)(void *dir_priv);
    int     (*mkdir)(void *fs_ctx, const char *path, int mode);
    int     (*rmdir)(void *fs_ctx, const char *path);
};

/* 挂载/卸载操作 */
struct vfs_mount_ops {
    int (*mount)(void **fs_ctx, const char *source, const char *target, int flags, void *data);
    int (*unmount)(void *fs_ctx);
};

/* 文件系统类型描述符（由具体FS注册） */
struct vfs_filesystem {
    const char *name;
    int (*mkfs)(const char *devpath, void *data);
    const struct vfs_mount_ops *mount_ops;
    const struct vfs_file_ops *file_ops;
};

void *vfs_alloc(uint32_t size);
void vfs_free(void *ptr);

/*=============================================================================
 * VFS核心API
 *============================================================================*/

/* 初始化VFS（必须在任何VFS调用前执行） */
int vfs_init(void);

/* 注册文件系统类型 */
int vfs_register_fs(const struct vfs_filesystem *fs);

/* 格式化设备（创建文件系统） */
int vfs_mkfs(const char *devpath, const char *fsname, void *data);

/* 挂载文件系统 */
int vfs_mount(const char *source, const char *target, const char *fsname, int flags, void *data);

/* 卸载文件系统 */
int vfs_umount(const char *target);

/*=============================================================================
 * 文件操作API
 *============================================================================*/

struct vfs_file_desc *vfs_open(const char *path, int flags, int mode);
vfs_ssize_t vfs_read(struct vfs_file_desc *fd, void *buf, size_t size);
vfs_ssize_t vfs_write(struct vfs_file_desc *fd, const void *buf, size_t size);
vfs_off_t   vfs_lseek(struct vfs_file_desc *fd, vfs_off_t offset, int whence);
vfs_off_t   vfs_tell(struct vfs_file_desc *fd);
vfs_off_t   vfs_size(struct vfs_file_desc *fd);

int     vfs_close(struct vfs_file_desc *fd);
int     vfs_stat(const char *path, struct stat *st);
int     vfs_unlink(const char *path);
int     vfs_rename(const char *old, const char *new);
int     vfs_sync(struct vfs_file_desc *fd);
int     vfs_truncate(struct vfs_file_desc *fd, off_t length);
int     vfs_eof(struct vfs_file_desc *fd);

/* 目录操作 */
void   *vfs_opendir(const char *path);
int     vfs_readdir(void *dir_priv, struct vfs_dirent *ent);
int     vfs_closedir(void *dir_priv);
int     vfs_mkdir(const char *path, int mode);
int     vfs_rmdir(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* VFS_H */
