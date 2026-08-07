/*
 * Copyright 2024, Hiroyuki OYAMA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef FILESYSTEM_H
#define FILESYSTEM_H
/** \file filesystem.h
 * \defgroup filesystem filesystem
 * \brief File system abstraction layer
 */
#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <fcntl.h>
//#include <unistd.h>
#include "blockdevice.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif


struct VFS_stat {
    int st_size;
    int st_mode;
 };

#define PATH_MAX1   256
#ifndef mode_t
#define mode_t uint32_t
#endif

enum {
    FILESYSTEM_TYPE_FAT,
    FILESYSTEM_TYPE_LITTLEFS,
};

enum {
    DT_UNKNOWN = 0,
    DT_DIR     = 4,
    DT_REG     = 8,
};

struct dirent {
    uint64_t fsize;
    uint8_t d_type;
    char d_name[255 + 1];
};

/*! \brief file object
 * \ingroup filesystem
 *
 * Files operated on by file system objects are represented by fs_file_t objects
 */
typedef struct {
    int fd;
    void *context;
} fs_file_t;

/*! \brief directory object
 * \ingroup filesystem
 *
 * Directories operated on by file system objects are represented by fs_dir_t objects
 */
typedef struct {
    int fd;
    void *context;
    struct dirent current;
} fs_dir_t;

/*! \brief file system abstract object
 *  \ingroup filesystem
 *
 *  All file system objects implement filesystem_t
 */
typedef struct filesystem {
    uint8_t type;
    const char *name;
    void *context;

    int (*mount)(struct filesystem *fs, blockdevice_t *device, bool pending);
    int (*unmount)(struct filesystem *fs);
    int (*format)(struct filesystem *fs, blockdevice_t *device);

    int (*remove)(struct filesystem *fs, const char *path);
    int (*rename)(struct filesystem *fs, const char *oldpath, const char *newpath);
    int (*mkdir)(struct filesystem *fs, const char *path, mode_t mode);
    int (*rmdir)(struct filesystem *fs, const char *path);
    int (*stat)(struct filesystem *fs, const char *path, struct VFS_stat *st);

    int (*file_open)(struct filesystem *fs, fs_file_t *file, const char *path, int flags);
    int (*file_close)(struct filesystem *fs, fs_file_t *file);
    int32_t (*file_write)(struct filesystem *fs, fs_file_t *file, const void *buffer, size_t size);
    int32_t (*file_read)(struct filesystem *fs, fs_file_t *file, void *buffer, size_t size);
    int (*file_sync)(struct filesystem *fs, fs_file_t *file);
    int32_t (*file_seek)(struct filesystem *fs, fs_file_t *file, int32_t offset, int whence);
    int32_t (*file_tell)(struct filesystem *fs, fs_file_t *file);
    int32_t (*file_size)(struct filesystem *fs, fs_file_t *file);
    int (*file_truncate)(struct filesystem *fs, fs_file_t *file, int32_t length);

    int (*dir_open)(struct filesystem *fs, fs_dir_t *dir, const char *path);
    int (*dir_close)(struct filesystem *fs, fs_dir_t *dir);
    int (*dir_read)(struct filesystem *fs, fs_dir_t *dir, struct dirent *ent);
} filesystem_t;

#ifdef __cplusplus
}
#endif
#endif