#include "vfs.h"
#include "vfs_fatfs.h"
#include "fatfs/ff.h"

#define FATFS_LSEEK_MINI_SIZE (16)

/*=============================================================================
 * FatFS 错误码转 VFS 错误码（使用 errno.h 定义）
 *============================================================================*/
static int fatfs_err_to_vfs(FRESULT fr)
{
    switch (fr) {
    case FR_OK:              return 0;
    case FR_DISK_ERR:        return -EIO;
    case FR_INT_ERR:         return -EIO;
    case FR_NOT_READY:       return -EIO;
    case FR_NO_FILE:         return -ENOENT;
    case FR_NO_PATH:         return -ENOENT;
    case FR_INVALID_NAME:    return -EINVAL;
    case FR_DENIED:          return -EACCES;
    case FR_EXIST:           return -EEXIST;
    case FR_INVALID_OBJECT:  return -EBADF;
    case FR_WRITE_PROTECTED: return -EROFS;
    case FR_INVALID_DRIVE:   return -ENODEV;
    case FR_NOT_ENABLED:     return -ENODEV;
    case FR_NO_FILESYSTEM:   return -ENODEV;
    case FR_MKFS_ABORTED:    return -EIO;
    case FR_TIMEOUT:         return -EBUSY;
    case FR_LOCKED:          return -EBUSY;
    case FR_NOT_ENOUGH_CORE: return -ENOMEM;
    case FR_TOO_MANY_OPEN_FILES: return -ENOSPC;
    case FR_INVALID_PARAMETER: return -EINVAL;
    default:                 return -EIO;
    }
}

/*=============================================================================
 * FatFS 私有数据结构
 *============================================================================*/

/* FatFS 卷上下文 */
struct fatfs_volume {
    FATFS fs;
    char path_prefix[VFS_PATH_MAX + 1];
    BYTE pdrv;
    atomic16_t users;
};

/* FatFS 文件包装 */
struct fatfs_file {
    FIL fil;
    struct fatfs_volume *vol;
};

/* FatFS 目录包装 */
struct fatfs_dir {
    DIR dir;
    FILINFO fno;
    struct fatfs_volume *vol;
    int valid;
};

/*=============================================================================
 * VFS 打开标志转 FatFS 模式
 *============================================================================*/

static BYTE vfs_flags_to_fatfs(int flags)
{
    BYTE mode = 0;

    if ((flags & VFS_O_RDWR) == VFS_O_RDWR) {
        mode |= FA_READ | FA_WRITE;
    } else if (flags & VFS_O_WRONLY) {
        mode |= FA_WRITE;
    } else {
        mode |= FA_READ;
    }

    if (flags & VFS_O_CREAT) {
        if (flags & VFS_O_TRUNC) {
            mode |= FA_CREATE_ALWAYS;
        } else {
            mode |= FA_OPEN_ALWAYS;
        }
    }

    if (flags & VFS_O_APPEND) {
        mode |= FA_OPEN_APPEND;
    }

    return mode;
}

//构造包含盘符的path
static char *fatfs_path(struct fatfs_volume *vol, const char *path)
{
    char *new_path = vfs_alloc(strlen(path) + 4);
    if (new_path) {
        sprintf(new_path, "%d:%s", vol->pdrv, path);
        return new_path;
    }
    return NULL;
}

/*=============================================================================
 * 文件操作实现
 *============================================================================*/

static int fatfs_vfs_open(void *fs_ctx, const char *path, int flags, int mode, void **file_priv)
{
    (void)mode;
    struct fatfs_volume *vol = (struct fatfs_volume *)fs_ctx;

    struct fatfs_file *file = vfs_alloc(sizeof(struct fatfs_file));
    if (!file) {
        return -ENOMEM;
    }

    BYTE fmode = vfs_flags_to_fatfs(flags);
    char *new_path = fatfs_path(vol, path);
    if (!new_path) {
        vfs_free(file);
        return -ENOMEM;
    }

    FRESULT fr = f_open(&file->fil, new_path, fmode);
    vfs_free(new_path);
    if (fr != FR_OK) {
        vfs_free(file);
        return fatfs_err_to_vfs(fr);
    }

#if FF_USE_FASTSEEK
    file->fil.cltbl = NULL;
    if (fmode == FA_READ) {
        QWORD    lseek_size = FATFS_LSEEK_MINI_SIZE;
        uint32_t seek_res;
        uint8_t  exit_flag = 0;
        while (1) {
            file->fil.cltbl = vfs_alloc(lseek_size * sizeof(QWORD));
            if (!file->fil.cltbl) {
                break;
            }
            *file->fil.cltbl = lseek_size;
            seek_res = f_lseek(&file->fil, CREATE_LINKMAP);
            if (seek_res == FR_NOT_ENOUGH_CORE) {
                lseek_size = *file->fil.cltbl + 1;
                vfs_free(file->fil.cltbl);
                file->fil.cltbl = NULL;
                // 如果已经赋值为1,则不尝试了,可能异常了
                if (exit_flag) {
                    os_printf("%s:%d\tfastseek err lseek_size:%X\r\n", __FUNCTION__, __LINE__, lseek_size);
                    break;
                }
                exit_flag = 1;
            } else {
                break;
            }
        }
    }
#endif

    atomic_inc(&vol->users);
    file->vol = vol;
    *file_priv = file;
    return 0;
}

static int fatfs_vfs_close(void *file_priv)
{
    struct fatfs_file *file = (struct fatfs_file *)file_priv;
    struct fatfs_volume *vol = file->vol;

#if FF_USE_FASTSEEK
    if (file->fil.cltbl) {
        vfs_free(file->fil.cltbl);
        file->fil.cltbl = NULL;
    }
#endif

    FRESULT fr = f_close(&file->fil);
    vfs_free(file);
    if (atomic_dec_and_test(&vol->users)) {
        vfs_free(vol);
    }
    return fatfs_err_to_vfs(fr);
}

static vfs_ssize_t fatfs_vfs_read(void *file_priv, void *buf, size_t size)
{
    struct fatfs_file *file = (struct fatfs_file *)file_priv;
    UINT br;

    if(atomic_read(&file->vol->users) <= 1) return -ENOENT;

    FRESULT fr = f_read(&file->fil, buf, (UINT)size, &br);
    if (fr != FR_OK) {
        return fatfs_err_to_vfs(fr);
    }
    return (vfs_ssize_t)br;
}

static vfs_ssize_t fatfs_vfs_write(void *file_priv, const void *buf, size_t size)
{
    struct fatfs_file *file = (struct fatfs_file *)file_priv;
    UINT bw;

    if(atomic_read(&file->vol->users) <= 1) return -ENOENT;

    FRESULT fr = f_write(&file->fil, buf, (UINT)size, &bw);
    if (fr != FR_OK) {
        return fatfs_err_to_vfs(fr);
    }
    return (vfs_ssize_t)bw;
}

static int fatfs_vfs_eof(void *file_priv)
{
    struct fatfs_file *file = (struct fatfs_file *)file_priv;
    if(atomic_read(&file->vol->users) <= 1) return -ENOENT;
    return f_eof(&file->fil);
}

static int fatfs_vfs_sync(void *file_priv)
{
    struct fatfs_file *file = (struct fatfs_file *)file_priv;
    if(atomic_read(&file->vol->users) <= 1) return -ENOENT;
    return f_sync(&file->fil);
}

static int fatfs_vfs_truncate(void *file_priv, vfs_off_t length)
{
    struct fatfs_file *file = (struct fatfs_file *)file_priv;
    if(atomic_read(&file->vol->users) <= 1) return -ENOENT;
    f_lseek(&file->fil, (FSIZE_t)length);
    return f_truncate(&file->fil);
}

static vfs_off_t fatfs_vfs_lseek(void *file_priv, vfs_off_t offset, int whence)
{
    struct fatfs_file *file = (struct fatfs_file *)file_priv;
    FRESULT fr;
    vfs_off_t new_pos;

    if(atomic_read(&file->vol->users) <= 1) return -ENOENT;

    switch (whence) {
        case VFS_SEEK_SET:
            new_pos = offset;
            break;
        case VFS_SEEK_CUR:
            new_pos = (vfs_off_t)f_tell(&file->fil) + offset;
            break;
        case VFS_SEEK_END:
            new_pos = (vfs_off_t)f_size(&file->fil) + offset;
            break;
        default:
            return -EINVAL;
    }

    fr = f_lseek(&file->fil, (FSIZE_t)new_pos);
    if (f_size(&file->fil) < new_pos) {
        f_truncate(&file->fil);
    }

    if (fr != FR_OK) {
        return fatfs_err_to_vfs(fr);
    }
    return new_pos;
}

static vfs_off_t fatfs_vfs_ftell(void *file_priv)
{
    struct fatfs_file *file = (struct fatfs_file *)file_priv;
    if(atomic_read(&file->vol->users) <= 1) return -ENOENT;
    return f_tell(&file->fil);
}

static vfs_off_t fatfs_vfs_fsize(void *file_priv)
{
    struct fatfs_file *file = (struct fatfs_file *)file_priv;
    if(atomic_read(&file->vol->users) <= 1) return -ENOENT;
    return f_size(&file->fil);
}

static int fatfs_vfs_stat(void *fs_ctx, const char *path, struct stat *st)
{
    FILINFO fno;
    struct fatfs_volume *vol = (struct fatfs_volume *)fs_ctx;

    char *new_path = fatfs_path(vol, path);
    if (!new_path) {
        return -ENOMEM;
    }

    FRESULT fr = f_stat(new_path, &fno);
    vfs_free(new_path);
    if (fr != FR_OK) {
        return fatfs_err_to_vfs(fr);
    }

    st->st_size = (uint32_t)fno.fsize;
    st->st_mode = (fno.fattrib & AM_DIR) ? S_IFDIR : S_IFREG;
    st->st_mtime = ((uint32_t)fno.fdate << 16) | fno.ftime;

    return 0;
}

static int fatfs_vfs_unlink(void *fs_ctx, const char *path)
{
    struct fatfs_volume *vol = (struct fatfs_volume *)fs_ctx;

    char *new_path = fatfs_path(vol, path);
    if (!new_path) {
        return -ENOMEM;
    }

    FRESULT fr = f_unlink(new_path);
    vfs_free(new_path);
    return fatfs_err_to_vfs(fr);
}

static int fatfs_vfs_rename(void *fs_ctx, const char *old, const char *new)
{
    (void)fs_ctx;
    FRESULT fr = f_rename(old, new);
    return fatfs_err_to_vfs(fr);
}

/*=============================================================================
 * 目录操作实现
 *============================================================================*/

static void *fatfs_vfs_opendir(void *fs_ctx, const char *path)
{
    struct fatfs_volume *vol = (struct fatfs_volume *)fs_ctx;

    struct fatfs_dir *dir = vfs_alloc(sizeof(struct fatfs_dir));
    if (!dir) {
        return NULL;
    }

    char *new_path = fatfs_path(vol, path);
    if (!new_path) {
        vfs_free(dir);
        return NULL;
    }

    FRESULT fr = f_opendir(&dir->dir, new_path);
    vfs_free(new_path);
    if (fr != FR_OK) {
        vfs_free(dir);
        return NULL;
    }

    atomic_inc(&vol->users);
    dir->vol = vol;
    dir->valid = 1;
    return dir;
}

static int fatfs_vfs_readdir(void *dir_priv, struct vfs_dirent *ent)
{
    struct fatfs_dir *dir = (struct fatfs_dir *)dir_priv;
    struct fatfs_volume *vol = (struct fatfs_volume *)dir->vol;

    if(atomic_read(&vol->users) <= 1) return -ENOENT;

    FRESULT fr = f_readdir(&dir->dir, &dir->fno);
    if (fr != FR_OK) {
        return fatfs_err_to_vfs(fr);
    }

    if (dir->fno.fname[0] == '\0') {
        return -ENOENT;
    }

    strncpy(ent->d_name, dir->fno.fname, VFS_NAME_MAX - 1);
    ent->d_name[VFS_NAME_MAX - 1] = '\0';
    ent->d_type = (dir->fno.fattrib & AM_DIR) ? VFS_DT_DIR : VFS_DT_REG;

    return 0;
}

static int fatfs_vfs_closedir(void *dir_priv)
{
    struct fatfs_dir *dir = (struct fatfs_dir *)dir_priv;
    struct fatfs_volume *vol = dir->vol;
    FRESULT fr = f_closedir(&dir->dir);
    vfs_free(dir);

    if (atomic_dec_and_test(&vol->users)) {
        vfs_free(vol);
    }
    return fatfs_err_to_vfs(fr);
}

static int fatfs_vfs_mkdir(void *fs_ctx, const char *path, int mode)
{
    (void)mode;

    struct fatfs_volume *vol = (struct fatfs_volume *)fs_ctx;
    char *new_path = fatfs_path(vol, path);
    if (!new_path) {
        return -ENOMEM;
    }
    FRESULT fr = f_mkdir(new_path);
    vfs_free(new_path);
    return fatfs_err_to_vfs(fr);
}

static int fatfs_vfs_rmdir(void *fs_ctx, const char *path)
{
    struct fatfs_volume *vol = (struct fatfs_volume *)fs_ctx;
    char *new_path = fatfs_path(vol, path);
    if (!new_path) {
        return -ENOMEM;
    }

    FRESULT fr = f_unlink(new_path);
    vfs_free(new_path);
    return fatfs_err_to_vfs(fr);
}

/*=============================================================================
 * 文件操作表
 *============================================================================*/

static const struct vfs_file_ops fatfs_vfs_file_ops = {
    .open       = fatfs_vfs_open,
    .close      = fatfs_vfs_close,
    .read       = fatfs_vfs_read,
    .write      = fatfs_vfs_write,
    .lseek      = fatfs_vfs_lseek,
    .ftell      = fatfs_vfs_ftell,
    .fsize      = fatfs_vfs_fsize,
    .eof        = fatfs_vfs_eof,
    .sync       = fatfs_vfs_sync,
    .stat       = fatfs_vfs_stat,
    .truncate   = fatfs_vfs_truncate,
    .unlink     = fatfs_vfs_unlink,
    .rename     = fatfs_vfs_rename,
    .opendir    = fatfs_vfs_opendir,
    .readdir    = fatfs_vfs_readdir,
    .closedir   = fatfs_vfs_closedir,
    .mkdir      = fatfs_vfs_mkdir,
    .rmdir      = fatfs_vfs_rmdir,
};

/*=============================================================================
 * 挂载/卸载操作
 *============================================================================*/

static int fatfs_vfs_mount(void **fs_ctx, const char *source, const char *target, int flags, void *data)
{
    (void)flags;
    (void)data;

    struct fatfs_volume *vol = vfs_alloc(sizeof(struct fatfs_volume));
    if (!vol) {
        return -ENOMEM;
    }

    atomic_set(&vol->users, 1);
    strncpy(vol->path_prefix, target, VFS_PATH_MAX - 1);
    vol->path_prefix[VFS_PATH_MAX - 1] = '\0';

    /* source 格式: "0:", "1:" 等，对应 FatFS 的驱动器号 */
    if (source && source[1] == ':') {
        vol->pdrv = (BYTE)(source[0] - '0');
    } else {
        vol->pdrv = 0;
    }

    FRESULT fr = f_mount(&vol->fs, source, 1);
    if (fr != FR_OK) {
        f_mount(NULL, source, 0);
        vfs_free(vol);
        os_printf(KERN_ERR"fatfs: mount fail, fr=%d. source:%s, target:%s\r\n", fr, source, target);
        return fatfs_err_to_vfs(fr);
    }

    *fs_ctx = vol;
    return 0;
}

static int fatfs_vfs_unmount(void *fs_ctx)
{
    struct fatfs_volume *vol = (struct fatfs_volume *)fs_ctx;
    char drv[3] = { (char)('0' + vol->pdrv), ':', '\0' };

    FRESULT fr = f_mount(NULL, drv, 0);
    if (atomic_dec_and_test(&vol->users)) {
        vfs_free(vol);
    }
    return fatfs_err_to_vfs(fr);
}

static int fatfs_vfs_statvfs(void *fs_ctx, const char *path, struct vfs_statvfs *buf)
{
    FATFS *fat = (FATFS *)fs_ctx;       /* 挂载时保存的文件系统上下文 */
    DWORD free_clusters;
    FRESULT res;

    /* 如果传入的路径为空，则使用根目录 "/" */
    const char *fs_path = (path && path[0] != '\0') ? path : "/";

    /* 调用 FatFS 获取空闲簇数，同时获得有效的 FATFS 指针 */
    res = f_getfree(fs_path, &free_clusters, &fat);
    if (res != FR_OK) {
        /* 将 FatFS 错误码映射为负 errno（这里简单映射为 -EIO） */
        return -EIO;
    }

    /* 计算总簇数：n_fatent 包括保留簇（通常 2 个），需减去 */
    DWORD total_clusters = fat->n_fatent - 2;
    UINT sector_size = fat->ssize;       /* 扇区大小（字节） */
    UINT sectors_per_cluster = fat->csize; /* 每簇扇区数 */

    /* 每簇大小（字节） */
    unsigned long cluster_size = sector_size * sectors_per_cluster;

    /* 填充统计结构体（遵循 POSIX statvfs 语义） */
    buf->f_bsize   = sector_size;            /* 文件系统块大小（通常指扇区） */
    buf->f_frsize  = cluster_size;           /* 片段大小（即簇大小） */
    buf->f_blocks  = total_clusters;         /* 总块数（按 f_frsize 计） */
    buf->f_bfree   = free_clusters;          /* 空闲块数 */
    buf->f_bavail  = free_clusters;          /* 非特权用户可用块数（无区别） */
    buf->f_files   = 0;                      /* FatFS 无 inode 统计，填 0 */
    buf->f_ffree   = 0;
    buf->f_favail  = 0;
    buf->f_fsid    = 0;                      /* 文件系统 ID（可忽略） */
    buf->f_flag    = 0;                      /* 挂载标志（可忽略） */
    buf->f_namemax = 255;                    /* 最大文件名长度（长文件名支持） */

    return 0;  /* 成功 */
}

static const struct vfs_mount_ops fatfs_vfs_mount_ops = {
    .mount      = fatfs_vfs_mount,
    .unmount    = fatfs_vfs_unmount,
    .statvfs    = fatfs_vfs_statvfs,
};

/*=============================================================================
 * mkfs 操作
 *============================================================================*/

static int fatfs_vfs_mkfs(const char *devpath, void *data)
{
    (void)data;
    MKFS_PARM opt = { 0 };
    opt.fmt = FM_ANY|FM_SFD;
    opt.au_size = 0;

    FRESULT fr = f_mkfs(devpath, &opt, NULL, 4096);
    return fatfs_err_to_vfs(fr);
}

/*=============================================================================
 * 文件系统注册
 *============================================================================*/

static const struct vfs_filesystem fatfs_vfs_fs = {
    .name       = "fatfs",
    .mkfs       = fatfs_vfs_mkfs,
    .mount_ops  = &fatfs_vfs_mount_ops,
    .file_ops   = &fatfs_vfs_file_ops,
};

/* 注册 FatFS 到 VFS */
int vfs_fatfs_register(void)
{
    return vfs_register_fs(&fatfs_vfs_fs);
}
