/*
 * Copyright 2024, Hiroyuki OYAMA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// #include <stdarg.h>
#include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
//#include <errno.h>
//#include <time.h>
// #include <unistd.h>
#include "blockdevice.h"
#include "fat.h"
#include "ff.h"
#include "diskio.h"
#include "VFS.h"

#ifndef ETIMEDOUT
#define ETIMEDOUT 116
#endif

#undef PATH_MAX
#define PATH_MAX 256

static WORD disk_get_sector_size(blockdevice_t *block_dev)
{
    size_t sector_size = block_dev->erase_size;

    WORD ssize = sector_size;
    if (ssize < 512)
    {
        ssize = 512;
    }
    return ssize;
}

static DWORD disk_get_sector_count(blockdevice_t *block_dev)
{
    DWORD scount = (block_dev->size(block_dev)) / disk_get_sector_size(block_dev);
    return scount;
}

static DSTATUS fatfs_status(void *status)
{
    return RES_OK;
}

static DSTATUS fatfs_init(void *init_dev)
{
    blockdevice_t *block_dev = (blockdevice_t *)init_dev;
    return block_dev->init(block_dev);
}

static DRESULT fatfs_read(void *dev, BYTE *buff, DWORD sector, UINT count)
{
    blockdevice_t *block_dev = (blockdevice_t *)dev;
    DWORD ssize = disk_get_sector_size(block_dev);
    bd_size_t addr = (bd_size_t)sector * ssize;
    bd_size_t size = count * ssize;
    int err = block_dev->read(block_dev, (const void *)buff, addr, size);
    return err ? RES_PARERR : RES_OK;
}

static DRESULT fatfs_write(void *dev, BYTE *buff, DWORD sector, UINT count)
{
    blockdevice_t *block_dev = (blockdevice_t *)dev;
    DWORD ssize = disk_get_sector_size(block_dev);
    bd_size_t addr = (bd_size_t)sector * ssize;
    bd_size_t size = count * ssize;

    int err = block_dev->erase(block_dev, addr, size);
    if (err)
    {
        return RES_PARERR;
    }
    err = block_dev->program(block_dev, buff, addr, size);
    if (err)
    {
        return RES_PARERR;
    }

    return RES_OK;
}

static DRESULT fatfs_ioctl(void *dev, BYTE cmd, void *buff)
{
    blockdevice_t *block_dev = (blockdevice_t *)dev;
    switch (cmd)
    {
    case CTRL_SYNC:
        if (block_dev == NULL)
        {
            return RES_NOTRDY;
        }
        else
        {
            return RES_OK;
        }
    case GET_SECTOR_COUNT:
        if (block_dev == NULL)
        {
            return RES_NOTRDY;
        }
        else
        {
            *((DWORD *)buff) = disk_get_sector_count(block_dev);
            return RES_OK;
        }
    case GET_SECTOR_SIZE:
        if (block_dev == NULL)
        {
            return RES_NOTRDY;
        }
        else
        {
            *((WORD *)buff) = disk_get_sector_size(block_dev);
            return RES_OK;
        }
    case GET_BLOCK_SIZE:
        *((DWORD *)buff) = 1; // default when not known
        return RES_OK;
    case CTRL_TRIM:
        if (block_dev == NULL)
        {
            return RES_NOTRDY;
        }
        else
        {
            DWORD *sectors = (DWORD *)buff;
            DWORD ssize = disk_get_sector_size(block_dev);
            bd_size_t addr = (bd_size_t)sectors[0] * ssize;
            bd_size_t size = (bd_size_t)(sectors[1] - sectors[0] + 1) * ssize;
            int err = block_dev->trim(block_dev, addr, size);
            return err ? RES_PARERR : RES_OK;
        }
    }
    return RES_PARERR;
}

static struct fatfs_diskio sdcdisk_driver = {
    .status = fatfs_status,
    .init = fatfs_init,
    .read = fatfs_read,
    .write = fatfs_write,
    .ioctl = fatfs_ioctl};

typedef struct
{
    FIL file;
} fat_file_t;

typedef struct
{
    FATFS fatfs;
    int id;
} filesystem_fat_context_t;

static const char FILESYSTEM_NAME[] = "FAT";
static blockdevice_t *_ffs[FF_VOLUMES] = {0};

static int fat_error_remap(FRESULT res)
{
    switch (res)
    {
    case FR_OK: // (0) Succeeded
        return 0;
    case FR_DISK_ERR: // (1) A hard error occurred in the low level disk I/O layer
        return -EIO;
    case FR_INT_ERR: // (2) Assertion failed
        return -1;
    case FR_NOT_READY: // (3) The physical drive cannot work
        return -EIO;
    case FR_NO_FILE: // (4) Could not find the file
        return -ENOENT;
    case FR_NO_PATH: // (5) Could not find the path
        return -ENOTDIR;
    case FR_INVALID_NAME: // (6) The path name format is invalid
        return -EINVAL;
    case FR_DENIED: // (7) Access denied due to prohibited access or directory full
        return -EACCES;
    case FR_EXIST: // (8) Access denied due to prohibited access
        return -EEXIST;
    case FR_INVALID_OBJECT: // (9) The file/directory object is invalid
        return -EBADF;
    case FR_WRITE_PROTECTED: // (10) The physical drive is write protected
        return -EACCES;
    case FR_INVALID_DRIVE: // (11) The logical drive number is invalid
        return -ENODEV;
    case FR_NOT_ENABLED: // (12) The volume has no work area
        return -ENODEV;
    case FR_NO_FILESYSTEM: // (13) There is no valid FAT volume
        return -EINVAL;
    case FR_MKFS_ABORTED: // (14) The f_mkfs() aborted due to any problem
        return -EIO;
    case FR_TIMEOUT: // (15) Could not get a grant to access the volume within defined period
        return -ETIMEDOUT;
    case FR_LOCKED: // (16) The operation is rejected according to the file sharing policy
        return -EBUSY;
    case FR_NOT_ENOUGH_CORE: // (17) LFN working buffer could not be allocated
        return -ENOMEM;
    case FR_TOO_MANY_OPEN_FILES: // (18) Number of open files > FF_FS_LOCK
        return -ENFILE;
    case FR_INVALID_PARAMETER: // (19) Given parameter is invalid
        return -EINVAL;
    default:
        return -res;
    }
}

static inline void debug_if(int condition, const char *format, ...)
{
    if (condition)
    {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}


static int mount(filesystem_t *fs, blockdevice_t *device, bool pending)
{
    filesystem_fat_context_t *context = fs->context;

    char _fsid[3] = {0};
    for (size_t i = 0; i < FF_VOLUMES; i++)
    {
        if (_ffs[i] == NULL)
        {
            context->id = i;
            _ffs[i] = device;
            _fsid[0] = '0' + i;
            _fsid[1] = ':';
            _fsid[2] = '\0';
            //这里再去注册
            fatfs_register_drive(i,&sdcdisk_driver,device);
            FRESULT res = f_mount(&context->fatfs, _fsid, !pending);
            if (res != FR_OK)
            {
                _ffs[i] = NULL;
            }
            return fat_error_remap(res);
        }
    }
    return -ENOMEM;
}

static int unmount(filesystem_t *fs)
{
    filesystem_fat_context_t *context = fs->context;

    char _fsid[3] = "0:";
    _fsid[0] = '0' + context->id;

    FRESULT res = f_mount(NULL, _fsid, 0);
    _ffs[context->id] = NULL;
    return fat_error_remap(res);
}

static int format(filesystem_t *fs, blockdevice_t *device)
{
    filesystem_fat_context_t *context = fs->context;

    if (!device->is_initialized)
    {
        int err = device->init(device);
        if (err)
        {
            return err;
        }
    }
    // erase first handful of blocks
    bd_size_t header = 2 * device->erase_size;
    int err = device->erase(device, 0, header);
    if (err)
    {
        return err;
    }
#if 0
    size_t program_size = device->program_size;


    void *buffer = VFS_malloc(program_size);
    if (!buffer) {
        return -ENOMEM;
    }
    VFS_memset(buffer, 0xFF, program_size);
    for (size_t i = 0; i < header; i += program_size) {
        err = device->program(device, buffer, i, program_size);
        if (err) {
            VFS_free(buffer);
            return err;
        }
    }
    VFS_free(buffer);
#endif

    // trim entire device to indicate it is unneeded
    err = device->trim(device, 0, device->size(device));
    if (err)
    {
        return err;
    }

    err = fs->mount(fs, device, true);
    if (err)
    {
        return err;
    }

    uint8_t work[512];

    char id[3] = "0:";
    id[0] = '0' + context->id;

    FRESULT res = f_mkfs((const TCHAR *)id, FM_ANY | FM_SFD, 0, work, 512);
    if (res != FR_OK)
    {
        fs->unmount(fs);
        return fat_error_remap(res);
    }

    err = fs->unmount(fs);
    if (err)
    {
        return res;
    }
    return 0;
}

static const char *fat_path_prefix(char *dist, int id, const char *path)
{
    if (id == 0)
    {
        VFS_strcpy(dist, path);
        return (const char *)dist;
    }

    dist[0] = '0' + id;
    dist[1] = ':';
    VFS_strcpy(dist + strlen("0:"), path);
    return (const char *)dist;
}

static int file_remove(filesystem_t *fs, const char *path)
{
    filesystem_fat_context_t *context = fs->context;

    char fpath[PATH_MAX];
    fat_path_prefix(fpath, context->id, path);
    FRESULT res = f_unlink(fpath);

    if (res != FR_OK)
    {
        debug_if(FFS_DBG, "f_unlink() failed: %d\n", res);
        if (res == FR_DENIED)
            return -ENOTEMPTY;
    }
    return fat_error_remap(res);
}

static int file_rename(filesystem_t *fs, const char *oldpath, const char *newpath)
{
    filesystem_fat_context_t *context = fs->context;
    char oldfpath[PATH_MAX];
    char newfpath[PATH_MAX];
    fat_path_prefix(oldfpath, context->id, oldpath);
    fat_path_prefix(newfpath, context->id, newpath);

    FRESULT res = f_rename(oldfpath, newfpath);

    if (res != FR_OK)
    {
        debug_if(FFS_DBG, "f_rename() failed: %d\n", res);
    }
    return fat_error_remap(res);
}

static int file_mkdir(filesystem_t *fs, const char *path, mode_t mode)
{
    (void)mode;
    filesystem_fat_context_t *context = fs->context;
    char fpath[PATH_MAX];
    fat_path_prefix(fpath, context->id, path);
    FRESULT res = f_mkdir(fpath);

    if (res != FR_OK)
    {
        debug_if(FFS_DBG, "f_mkdir() failed: %d\n", res);
    }
    return fat_error_remap(res);
}

static int file_rmdir(filesystem_t *fs, const char *path)
{
    filesystem_fat_context_t *context = fs->context;
    char fpath[PATH_MAX];
    fat_path_prefix(fpath, context->id, path);
    FRESULT res = f_unlink(fpath);
    if (res != FR_OK)
    {
        debug_if(FFS_DBG, "f_unlink() failed: %d\n", res);
    }
    return fat_error_remap(res);
}

static int file_stat(filesystem_t *fs, const char *path, struct VFS_stat *st)
{
    filesystem_fat_context_t *context = fs->context;
    char fpath[PATH_MAX1];
    fat_path_prefix(fpath, context->id, path);
    FILINFO f = {0};

    FRESULT res = f_stat(fpath, &f);

    if (res != FR_OK)
    {
        return fat_error_remap(res);
    }
    st->st_size = f.fsize;
    st->st_mode = 0;
    st->st_mode |= (f.fattrib & AM_DIR) ? S_IFDIR : S_IFREG;
    st->st_mode |= (f.fattrib & AM_RDO) ? (S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) : (S_IRWXU | S_IRWXG | S_IRWXO);
    return 0;
}

static int file_open(filesystem_t *fs, fs_file_t *file, const char *path, int flags)
{
    BYTE open_mode;
    if (flags & O_RDWR)
        open_mode = FA_READ | FA_WRITE;
    else if (flags & O_WRONLY)
        open_mode = FA_WRITE;
    else
        open_mode = FA_READ;
    if (flags & O_CREAT)
    {
        if (flags & O_TRUNC)
            open_mode |= FA_CREATE_ALWAYS;
        else
            open_mode |= FA_OPEN_ALWAYS;
    }
    if (flags & O_APPEND)
        open_mode |= FA_OPEN_APPEND;

    char fpath[PATH_MAX];
    filesystem_fat_context_t *context = fs->context;
    fat_path_prefix(fpath, context->id, path);
    fat_file_t *fat_file = file->context = VFS_calloc(1, sizeof(fat_file_t));
    if (fat_file == NULL)
    {
        os_printf("file_open: Out of memory\n");
        return -ENOMEM;
    }
    FRESULT res = f_open(&fat_file->file, fpath, open_mode);

    if (res != FR_OK)
    {
        VFS_free(fat_file);
        debug_if(FFS_DBG, "f_open('w') failed: %d\n", res);
        return fat_error_remap(res);
    }
    return 0;
}

static int file_close(filesystem_t *fs, fs_file_t *file)
{
    (void)fs;
    // filesystem_fat_context_t *context = fs->context;
    fat_file_t *fat_file = file->context;
    FRESULT res = f_close(&fat_file->file);
    VFS_free(file->context);
    file->context = NULL;
    return fat_error_remap(res);
}

static ssize_t file_write(filesystem_t *fs, fs_file_t *file, const void *buffer, size_t size)
{
    (void)fs;
    // filesystem_fat_context_t *context = fs->context;
    fat_file_t *fat_file = file->context;

    UINT n;

    FRESULT res = f_write(&(fat_file->file), buffer, size, &n);
    if (res != FR_OK)
    {
        debug_if(FFS_DBG, "f_write() failed: %d", res);
        return fat_error_remap(res);
    }
    res = f_sync(&fat_file->file);

    if (res != FR_OK)
    {
        debug_if(FFS_DBG, "f_write() failed: %d", res);
        return fat_error_remap(res);
    }
    return n;
}

static ssize_t file_read(filesystem_t *fs, fs_file_t *file, void *buffer, size_t size)
{
    (void)fs;
    // filesystem_fat_context_t *context = fs->context;
    fat_file_t *fat_file = file->context;

    UINT n;

    FRESULT res = f_read(&fat_file->file, buffer, size, &n);

    if (res != FR_OK)
    {
        debug_if(FFS_DBG, "f_read() failed: %d\n", res);
        return fat_error_remap(res);
    }
    return n;
}

static int file_sync(filesystem_t *fs, fs_file_t *file)
{
    (void)fs;
    // filesystem_fat_context_t *context = fs->context;
    fat_file_t *fat_file = file->context;

    FRESULT res = f_sync(&fat_file->file);

    if (res != FR_OK)
    {
        debug_if(FFS_DBG, "f_sync() failed: %d\n", res);
    }
    return fat_error_remap(res);
}

static int32_t file_seek(filesystem_t *fs, fs_file_t *file, int32_t offset, int whence)
{
    (void)fs;
    // filesystem_fat_context_t *context = fs->context;
    fat_file_t *fat_file = file->context;
    if (whence == SEEK_END)
        offset += f_size(&fat_file->file);
    else if (whence == SEEK_CUR)
        offset += f_tell(&fat_file->file);

    FRESULT res = f_lseek(&fat_file->file, offset);

    if (res != FR_OK)
    {
        debug_if(FFS_DBG, "lseek failed: %d\n", res);
        return fat_error_remap(res);
    }
    return (int32_t)fat_file->file.fptr;
}

static int32_t file_tell(filesystem_t *fs, fs_file_t *file)
{
    (void)fs;
    // filesystem_fat_context_t *context = fs->context;
    fat_file_t *fat_file = file->context;
    int32_t res = f_tell(&fat_file->file);

    return res;
}

static int32_t file_size(filesystem_t *fs, fs_file_t *file)
{
    (void)fs;
    // filesystem_fat_context_t *context = fs->context;
    fat_file_t *fat_file = file->context;
    int32_t res = f_size(&fat_file->file);

    return res;
}

static int file_truncate(filesystem_t *fs, fs_file_t *file, int32_t length)
{
    (void)fs;
    // filesystem_fat_context_t *context = fs->context;
    fat_file_t *fat_file = file->context;
    FSIZE_t old_offset = f_tell(&fat_file->file);
    FRESULT res = f_lseek(&fat_file->file, length);
    if (res)
    {
        return fat_error_remap(res);
    }
    res = f_truncate(&fat_file->file);
    if (res)
    {
        return fat_error_remap(res);
    }
    res = f_lseek(&fat_file->file, old_offset);

    if (res)
    {
        return fat_error_remap(res);
    }
    return 0;
}

static int dir_open(filesystem_t *fs, fs_dir_t *dir, const char *path)
{
    filesystem_fat_context_t *context = fs->context;
    char fpath[PATH_MAX];
    fat_path_prefix(fpath, context->id, path);

    DIR *dh = VFS_calloc(1, sizeof(DIR));
    if (dh == NULL)
    {
        os_printf("dir_open: Out of memory\n");
        return -ENOMEM;
    }
    FRESULT res = f_opendir(dh, fpath);

    if (res != FR_OK)
    {
        debug_if(FFS_DBG, "f_opendir() failed: %d\n", res);
        VFS_free(dh);
        return fat_error_remap(res);
    }
    dir->context = dh;
    dir->fd = -1;
    return 0;
}

static int dir_close(filesystem_t *fs, fs_dir_t *dir)
{
    (void)fs;
    // filesystem_fat_context_t *context = fs->context;

    DIR *dh = (DIR *)dir->context;

    FRESULT res = f_closedir(dh);

    VFS_free(dh);
    return fat_error_remap(res);
}

static int dir_read(filesystem_t *fs, fs_dir_t *dir, struct dirent *ent)
{
    (void)fs;
    // filesystem_fat_context_t *context = fs->context;

    DIR *dh = (DIR *)dir->context;
    FILINFO finfo = {0};
    FRESULT res = f_readdir(dh, &finfo);

    if (res != FR_OK)
    {
        return fat_error_remap(res);
    }
    else if (finfo.fname[0] == 0)
    {
        return -ENOENT;
    }
    ent->d_type = (finfo.fattrib & AM_DIR) ? DT_DIR : DT_REG;
    ent->fsize = finfo.fsize;
    VFS_strncpy(ent->d_name, finfo.fname, FF_MAX_LFN);
    ent->d_name[FF_MAX_LFN] = '\0';
    return 0;
}

filesystem_t *filesystem_fat_create()
{
    
    
    filesystem_t *fs = VFS_calloc(1, sizeof(filesystem_t));
    if (fs == NULL)
    {
        os_printf("filesystem_fat_create: Out of memory\n");
        return NULL;
    }
    fs->type = FILESYSTEM_TYPE_FAT;
    fs->name = FILESYSTEM_NAME;
    fs->mount = mount;
    fs->unmount = unmount;
    fs->format = format;
    fs->remove = file_remove;
    fs->rename = file_rename;
    fs->mkdir = file_mkdir;
    fs->rmdir = file_rmdir;
    fs->stat = file_stat;
    fs->file_open = file_open;
    fs->file_close = file_close;
    fs->file_write = file_write;
    fs->file_read = file_read;
    fs->file_sync = file_sync;
    fs->file_seek = file_seek;
    fs->file_tell = file_tell;
    fs->file_size = file_size;
    fs->file_truncate = file_truncate;
    fs->dir_open = dir_open;
    fs->dir_close = dir_close;
    fs->dir_read = dir_read;
    filesystem_fat_context_t *context = VFS_calloc(1, sizeof(filesystem_fat_context_t));
    if (context == NULL)
    {
        os_printf("filesystem_fat_create: Out of memory\n");
        VFS_free(fs);
        return NULL;
    }
    context->id = -1;

    fs->context = context;
    return fs;
}

void filesystem_fat_free(filesystem_t *fs)
{
    VFS_free(fs->context);
    fs->context = NULL;
    VFS_free(fs);
    fs = NULL;
}
