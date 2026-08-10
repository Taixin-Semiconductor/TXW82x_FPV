#include "vfs.h"

/* 挂载点结构 */
struct vfs_mount_point {
    char path[VFS_PATH_MAX];
    void *fs_ctx;
    const struct vfs_file_ops *ops;
    const struct vfs_mount_ops *mops;
#if VFS_MUTEX_ENABLED
    os_mutex_t mutex;
#endif
};

/* 文件描述符结构 */
struct vfs_file_desc {
    struct vfs_mount_point *mp;
    void *priv;
    int flags;
};

/* 目录描述符结构 */
struct vfs_dir_desc {
    struct vfs_mount_point *mp;
    void *priv;
};

/* 全局变量 */
static struct vfs_mount_point g_mounts[VFS_MAX_MOUNTS];
static int g_mount_count = 0;

static const struct vfs_filesystem *g_fs_table[VFS_MAX_FILESYSTEMS];
static int g_fs_count = 0;

#if VFS_MUTEX_ENABLED
static os_mutex_t       g_vfs_global_mutex;
#define VFS_LOCK()      os_mutex_lock(&g_vfs_global_mutex, osWaitForever)
#define VFS_UNLOCK()    os_mutex_unlock(&g_vfs_global_mutex)
#define MP_LOCK(mp)     os_mutex_lock(&(mp->mutex), osWaitForever)
#define MP_UNLOCK(mp)   os_mutex_unlock(&(mp->mutex))
#else
#define VFS_LOCK()
#define VFS_UNLOCK()
#define MP_LOCK(mp)
#define MP_UNLOCK(mp)
#endif

/*=============================================================================
 * 内部辅助函数
 *============================================================================*/

/* 根据文件系统名称查找注册表 */
static const struct vfs_filesystem *find_filesystem(const char *name)
{
    for (int i = 0; i < g_fs_count; i++) {
        if (strcmp(g_fs_table[i]->name, name) == 0) {
            return g_fs_table[i];
        }
    }
    return NULL;
}

/* 最长前缀匹配挂载点 */
static struct vfs_mount_point *find_mount_point(const char *path)
{
    struct vfs_mount_point *best = NULL;
    size_t best_len = 0;

    for (int i = 0; i < g_mount_count; i++) {
        const char *mp_path = g_mounts[i].path;
        size_t len = strlen(mp_path);
        if (strncmp(path, mp_path, len) == 0 &&
            (path[len] == '/' || path[len] == '\0') &&
            len > best_len) {
            best = &g_mounts[i];
            best_len = len;
        }
    }
    return best;
}

/* 提取相对路径（跳过挂载点前缀） */
static const char *get_relative_path(const struct vfs_mount_point *mp, const char *full_path)
{
    size_t prefix_len = strlen(mp->path);
    const char *rel = full_path + prefix_len;
    if (*rel == '/') rel++;
    return rel;
}

/*=============================================================================
 * VFS核心API
 *============================================================================*/

int vfs_init(void)
{
    g_mount_count = 0;
    g_fs_count = 0;
    memset(g_mounts, 0, sizeof(g_mounts));
    memset(g_fs_table, 0, sizeof(g_fs_table));

#if VFS_MUTEX_ENABLED
    if (os_mutex_init(&g_vfs_global_mutex)) {
        return -ENOMEM;
    }
#endif
    return 0;
}

int vfs_register_fs(const struct vfs_filesystem *fs)
{
    if (!fs || !fs->name) return -EINVAL;
    if (g_fs_count >= VFS_MAX_FILESYSTEMS) return -ENOSPC;

    if (find_filesystem(fs->name) != NULL) return -EEXIST;

    g_fs_table[g_fs_count++] = fs;
    return 0;
}

int vfs_mkfs(const char *devpath, const char *fsname, void *data)
{
    if (!devpath || !fsname) return -EINVAL;

    const struct vfs_filesystem *fs = find_filesystem(fsname);
    if (!fs || !fs->mkfs) return -ENODEV;

    int ret = fs->mkfs(devpath, data);
    if (ret != 0) set_errno(-ret);
    return ret;
}

int vfs_mount(const char *source, const char *target, const char *fsname, int flags, void *data)
{
    if (!source || !target || !fsname) return -EINVAL;
    if (strlen(target) >= VFS_PATH_MAX) return -ENAMETOOLONG;

    VFS_LOCK();

    for (int i = 0; i < g_mount_count; i++) {
        if (strcmp(g_mounts[i].path, target) == 0) {
            VFS_UNLOCK();
            return -EBUSY;
        }
    }

    const struct vfs_filesystem *fs = find_filesystem(fsname);
    if (!fs || !fs->mount_ops || !fs->file_ops) {
        VFS_UNLOCK();
        os_printf(KERN_ERR"vfs: %s unsupport!\r\n", fsname);
        return -ENODEV;
    }

    void *fs_ctx = NULL;
    int ret = fs->mount_ops->mount(&fs_ctx, source, target, flags, data);
    if (ret != 0) {
        VFS_UNLOCK();
        return ret;
    }

    if (g_mount_count >= VFS_MAX_MOUNTS) {
        fs->mount_ops->unmount(fs_ctx);
        VFS_UNLOCK();
        return -ENOSPC;
    }

    struct vfs_mount_point *mp = &g_mounts[g_mount_count++];
    strncpy(mp->path, target, VFS_PATH_MAX - 1);
    mp->path[VFS_PATH_MAX - 1] = '\0';
    mp->fs_ctx = fs_ctx;
    mp->ops = fs->file_ops;
    mp->mops = fs->mount_ops;
#if VFS_MUTEX_ENABLED
    if (os_mutex_init(&mp->mutex)) {
        fs->mount_ops->unmount(fs_ctx);
        g_mount_count--;
        VFS_UNLOCK();
        return -ENOMEM;
    }
#endif

    VFS_UNLOCK();
    return 0;
}

int vfs_umount(const char *target)
{
    if (!target) return -EINVAL;

    VFS_LOCK();

    int index = -1;
    for (int i = 0; i < g_mount_count; i++) {
        if (strcmp(g_mounts[i].path, target) == 0) {
            index = i;
            break;
        }
    }
    if (index < 0) {
        VFS_UNLOCK();
        return -ENOENT;
    }

    struct vfs_mount_point *mp = &g_mounts[index];

    int ret = 0;
    if (mp->mops && mp->mops->unmount) {
        ret = mp->mops->unmount(mp->fs_ctx);
    }

#if VFS_MUTEX_ENABLED
    os_mutex_del(&mp->mutex);
#endif

    for (int i = index; i < g_mount_count - 1; i++) {
        g_mounts[i] = g_mounts[i + 1];
    }
    g_mount_count--;
    memset(&g_mounts[g_mount_count], 0, sizeof(struct vfs_mount_point));

    VFS_UNLOCK();
    if (ret != 0) set_errno(-ret);
    return ret;
}

/*=============================================================================
 * 文件操作API实现
 *============================================================================*/

struct vfs_file_desc *vfs_open(const char *path, int flags, int mode)
{
    if (!path) {
        set_errno(EINVAL);
        return NULL;
    }

    VFS_LOCK();

    struct vfs_mount_point *mp = find_mount_point(path);
    if (!mp) {
        set_errno(ENOENT);
        VFS_UNLOCK();
        return NULL;
    }

    const char *rel_path = get_relative_path(mp, path);
    void *file_priv = NULL;

    MP_LOCK(mp);
    int ret = mp->ops->open(mp->fs_ctx, rel_path, flags, mode, &file_priv);
    MP_UNLOCK(mp);

    if (ret != 0) {
        set_errno(-ret);
        VFS_UNLOCK();
        return NULL;
    }

    struct vfs_file_desc *fd = (struct vfs_file_desc *)vfs_alloc(sizeof(struct vfs_file_desc));
    if (!fd) {
        mp->ops->close(file_priv);
        set_errno(ENOMEM);
        VFS_UNLOCK();
        return NULL;
    }

    fd->mp = mp;
    fd->priv = file_priv;
    fd->flags = flags;
    VFS_UNLOCK();
    return fd;
}

vfs_ssize_t vfs_read(struct vfs_file_desc *fd, void *buf, size_t size)
{
    if (!fd || !buf) {
        set_errno(EBADF);
        return -1;
    }

    MP_LOCK(fd->mp);
    vfs_ssize_t ret = fd->mp->ops->read(fd->priv, buf, size);
    MP_UNLOCK(fd->mp);

    if (ret < 0) set_errno(-ret);
    return ret;
}

vfs_ssize_t vfs_write(struct vfs_file_desc *fd, const void *buf, size_t size)
{
    if (!fd || !buf) {
        set_errno(EBADF);
        return -1;
    }

    MP_LOCK(fd->mp);
    vfs_ssize_t ret = fd->mp->ops->write(fd->priv, buf, size);
    MP_UNLOCK(fd->mp);

    if (ret < 0) set_errno(-ret);
    return ret;
}

vfs_off_t vfs_lseek(struct vfs_file_desc *fd, vfs_off_t offset, int whence)
{
    if (!fd) {
        set_errno(EBADF);
        return -1;
    }

    MP_LOCK(fd->mp);
    vfs_off_t ret = fd->mp->ops->lseek(fd->priv, offset, whence);
    MP_UNLOCK(fd->mp);

    if (ret < 0) set_errno(-ret);
    return ret;
}

vfs_off_t vfs_tell(struct vfs_file_desc *fd)
{
    if (!fd) {
        set_errno(EBADF);
        return 0;
    }

    MP_LOCK(fd->mp);
    vfs_off_t ret = fd->mp->ops->ftell(fd->priv);
    MP_UNLOCK(fd->mp);

    if (ret < 0) set_errno(-ret);
    return ret;
}

vfs_off_t vfs_size(struct vfs_file_desc *fd)
{
    vfs_off_t size = 0;

    if (!fd) {
        set_errno(EBADF);
        return 0;
    }

    MP_LOCK(fd->mp);
    if(fd->mp->ops->fsize){
        size = fd->mp->ops->fsize(fd->priv);
    }else{
        vfs_off_t off = fd->mp->ops->ftell(fd->priv);
        fd->mp->ops->lseek(fd->priv, 0, SEEK_END);
        size = fd->mp->ops->ftell(fd->priv);
        fd->mp->ops->lseek(fd->priv, off, SEEK_SET);
    }
    MP_UNLOCK(fd->mp);

    return size;

}

int vfs_close(struct vfs_file_desc *fd)
{
    if (!fd) {
        set_errno(EBADF);
        return -1;
    }

    MP_LOCK(fd->mp);
    int ret = fd->mp->ops->close(fd->priv);
    MP_UNLOCK(fd->mp);

    vfs_free(fd);
    if (ret != 0) set_errno(-ret);
    return ret;
}

int vfs_stat(const char *path, struct stat *st)
{
    if (!path || !st) return -EINVAL;

    VFS_LOCK();
    struct vfs_mount_point *mp = find_mount_point(path);
    if (!mp) {
        VFS_UNLOCK();
        return -ENOENT;
    }

    const char *rel_path = get_relative_path(mp, path);
    MP_LOCK(mp);
    int ret = mp->ops->stat(mp->fs_ctx, rel_path, st);
    MP_UNLOCK(mp);

    VFS_UNLOCK();
    if (ret != 0) set_errno(-ret);
    return ret;
}

int vfs_sync(struct vfs_file_desc *fd)
{
    if (!fd) {
        set_errno(EBADF);
        return -1;
    }

    MP_LOCK(fd->mp);
    int ret = fd->mp->ops->sync(fd->priv);
    MP_UNLOCK(fd->mp);

    if (ret != 0) set_errno(-ret);
    return ret;
}

int vfs_truncate(struct vfs_file_desc *fd, off_t length)
{
    if (!fd) {
        set_errno(EBADF);
        return -1;
    }

    MP_LOCK(fd->mp);
    int ret = fd->mp->ops->truncate(fd->priv, length);
    MP_UNLOCK(fd->mp);

    if (ret != 0) set_errno(-ret);
    return ret;
}

int vfs_eof(struct vfs_file_desc *fd)
{
    if (!fd) {
        set_errno(EBADF);
        return -1;
    }

    MP_LOCK(fd->mp);
    int ret = fd->mp->ops->eof(fd->priv);
    MP_UNLOCK(fd->mp);
    return ret;
}

int vfs_unlink(const char *path)
{
    if (!path) return -EINVAL;

    VFS_LOCK();
    struct vfs_mount_point *mp = find_mount_point(path);
    if (!mp) {
        VFS_UNLOCK();
        return -ENOENT;
    }

    const char *rel_path = get_relative_path(mp, path);
    MP_LOCK(mp);
    int ret = mp->ops->unlink(mp->fs_ctx, rel_path);
    MP_UNLOCK(mp);

    VFS_UNLOCK();
    if (ret != 0) set_errno(-ret);
    return ret;
}

int vfs_rename(const char *old, const char *new)
{
    if (!old || !new) return -EINVAL;

    VFS_LOCK();
    struct vfs_mount_point *mp_old = find_mount_point(old);
    struct vfs_mount_point *mp_new = find_mount_point(new);

    if (!mp_old || mp_old != mp_new) {
        VFS_UNLOCK();
        return -EXDEV;
    }

    const char *rel_old = get_relative_path(mp_old, old);
    const char *rel_new = get_relative_path(mp_new, new);
    MP_LOCK(mp_old);
    int ret = mp_old->ops->rename(mp_old->fs_ctx, rel_old, rel_new);
    MP_UNLOCK(mp_old);

    VFS_UNLOCK();
    if (ret != 0) set_errno(-ret);
    return ret;
}

/*=============================================================================
 * 目录操作
 *=============================================================================*/

void *vfs_opendir(const char *path)
{
    if (!path) {
        set_errno(EINVAL);
        return NULL;
    }

    VFS_LOCK();
    struct vfs_mount_point *mp = find_mount_point(path);
    if (!mp) {
        set_errno(ENOENT);
        VFS_UNLOCK();
        return NULL;
    }

    const char *rel_path = get_relative_path(mp, path);
    void *dir_priv = NULL;
    MP_LOCK(mp);
    dir_priv = mp->ops->opendir(mp->fs_ctx, rel_path);
    MP_UNLOCK(mp);

    if (!dir_priv) {
        set_errno(EIO);
        VFS_UNLOCK();
        return NULL;
    }

    struct vfs_dir_desc *dd = (struct vfs_dir_desc *)vfs_alloc(sizeof(struct vfs_dir_desc));
    if (!dd) {
        mp->ops->closedir(dir_priv);
        set_errno(ENOMEM);
        VFS_UNLOCK();
        return NULL;
    }

    dd->mp = mp;
    dd->priv = dir_priv;
    VFS_UNLOCK();
    return dd;
}

int vfs_readdir(void *dir_priv, struct vfs_dirent *ent)
{
    if (!dir_priv || !ent) return -EINVAL;
    struct vfs_dir_desc *dd = (struct vfs_dir_desc *)dir_priv;

    MP_LOCK(dd->mp);
    int ret = dd->mp->ops->readdir(dd->priv, ent);
    MP_UNLOCK(dd->mp);

    if (ret != 0) set_errno(-ret);
    return ret;
}

int vfs_closedir(void *dir_priv)
{
    if (!dir_priv) return -EINVAL;
    struct vfs_dir_desc *dd = (struct vfs_dir_desc *)dir_priv;

    MP_LOCK(dd->mp);
    int ret = dd->mp->ops->closedir(dd->priv);
    MP_UNLOCK(dd->mp);

    vfs_free(dd);
    if (ret != 0) set_errno(-ret);
    return ret;
}

int vfs_mkdir(const char *path, int mode)
{
    if (!path) return -EINVAL;

    VFS_LOCK();
    struct vfs_mount_point *mp = find_mount_point(path);
    if (!mp) {
        VFS_UNLOCK();
        return -ENOENT;
    }

    const char *rel_path = get_relative_path(mp, path);
    MP_LOCK(mp);
    int ret = mp->ops->mkdir(mp->fs_ctx, rel_path, mode);
    MP_UNLOCK(mp);

    VFS_UNLOCK();
    if (ret != 0) set_errno(-ret);
    return ret;
}

int vfs_rmdir(const char *path)
{
    if (!path) return -EINVAL;

    VFS_LOCK();
    struct vfs_mount_point *mp = find_mount_point(path);
    if (!mp) {
        VFS_UNLOCK();
        return -ENOENT;
    }

    const char *rel_path = get_relative_path(mp, path);
    MP_LOCK(mp);
    int ret = mp->ops->rmdir(mp->fs_ctx, rel_path);
    MP_UNLOCK(mp);

    VFS_UNLOCK();
    if (ret != 0) set_errno(-ret);
    return ret;
}

