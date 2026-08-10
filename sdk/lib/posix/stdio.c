#include "sys_config.h"
#include "typesdef.h"
#include "errno.h"
#include "osal/sleep.h"
#include "osal/string.h"
#include "lib/posix/stdio.h"
#include "lwip/sockets.h"

#ifdef TXWSDK_POSIX

#if FS_EN
#include "fs/vfs/vfs.h"
#endif


#define FILE void

#define FS_PSRAM_BUFF 0

/* sram slice buffer size, usr for psram slice write to sd */
#define STDIO_RWBUF_SIZE 4096

int read(int fd, void *buf, size_t nbytes);
int write(int fd, const void *buf, size_t nbytes);


#define ENV_MAX_CNT (8)
static const char *_env_list_[ENV_MAX_CNT][2];

char *getenv(const char *name)
{
    int32 i = 0;
    for (i = 0; i < ENV_MAX_CNT; i++) {
        if (_env_list_[i][0] == name) {
            return (char *)_env_list_[i][1];
        }
    }
    return NULL;
}

int setenv(const char *name, const char *value, int overwrite)
{
    int32 i = 0;
    int32 j = -1;

    for (i = 0; i < ENV_MAX_CNT; i++) {
        if (_env_list_[i][0] == name) {
            break;
        }
        if (j == -1 && _env_list_[i][0] == NULL) {
            j = i;
        }
    }

    if (i < ENV_MAX_CNT && (overwrite || _env_list_[i][1] == NULL)) {
        _env_list_[i][1] = value;
    } else if (i >= ENV_MAX_CNT && j != -1) {
        _env_list_[j][0] = name;
        _env_list_[j][1] = value;
    } else {
        return -1;
    }
    return 0;
}

int unsetenv(const char *name)
{
    int32 i = 0;
    for (i = 0; i < ENV_MAX_CNT; i++) {
        if (_env_list_[i][0] == name) {
            _env_list_[i][0] = NULL;
            _env_list_[i][1] = NULL;
            break;
        }
    }
    return 0;
}

int usleep(unsigned long usec)
{
    os_sleep_us(usec);
    return 0;
}

unsigned int sleep(unsigned int seconds)
{
    os_sleep(seconds);
    return 0;
}

int access(const char *pathname, int mode)
{
#if FS_EN
    struct stat st;
    (void)mode;
    return vfs_stat(pathname, &st);
#else
    return -1;
#endif
}

int fclose(FILE *stream)
{
#if FS_EN
    return vfs_close((struct vfs_file_desc *)stream);
#else
    return -1;
#endif
}

int feof(FILE *stream)
{
#if FS_EN
    return vfs_eof((struct vfs_file_desc *)stream);
#else
    return -1;
#endif
}

int ferror(FILE *stream)
{
    (void)stream;
    /* VFS 不提供文件错误状态，返回 0 */
    return 0;
}

int fflush(FILE *stream)
{
    (void)stream;
    /* VFS 不提供缓冲区刷新，直接返回成功 */
    return 0;
}

FILE *fopen(const char *filename, const char *mode)
{
#if FS_EN
    int flags = 0;

    /* 解析模式字符串 */
    if (mode[0] == 'r') {
        flags = VFS_O_RDONLY;
    } else if (mode[0] == 'w') {
        flags = VFS_O_WRONLY | VFS_O_CREAT | VFS_O_TRUNC;
    } else if (mode[0] == 'a') {
        flags = VFS_O_WRONLY | VFS_O_CREAT | VFS_O_APPEND;
    }

    /* 检查 '+' 表示读写 */
    if (mode[1] == '+' || (mode[1] && mode[2] == '+')) {
        flags &= ~VFS_O_RDONLY;
        flags &= ~VFS_O_WRONLY;
        flags |= VFS_O_RDWR;
    }

    return (FILE *)vfs_open(filename, flags, 0);
#else
    return 0;
#endif
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
#if FS_EN
    vfs_ssize_t ret = vfs_read((struct vfs_file_desc *)stream, ptr, size * nmemb);
    return ret > 0 ? (size_t)ret : 0;
#else
    return 0;
#endif
}

int fseek(FILE *stream, off_t offset, int whence)
{
#if FS_EN
    return vfs_lseek((struct vfs_file_desc *)stream, (vfs_off_t)offset, whence) < 0 ? -1 : 0;
#else
    return 0;
#endif
}

off_t ftell(FILE *stream)
{
#if FS_EN
    vfs_off_t pos = vfs_tell((struct vfs_file_desc *)stream);
    return (off_t)pos;
#else
    return 0;
#endif
}

off_t fsize(FILE *stream)
{
#if FS_EN
    vfs_off_t pos = vfs_size((struct vfs_file_desc *)stream);
    return (off_t)pos;
#else
    return 0;
#endif
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
#if FS_EN
    vfs_ssize_t ret = vfs_write((struct vfs_file_desc *)stream, ptr, size * nmemb);
    return ret > 0 ? (size_t)ret : 0;
#else
    return 0;
#endif
}

int fileno(FILE *stream)
{
    return (int)stream;
}

int ftruncate(int fd, off_t length)
{
#if FS_EN
    return vfs_truncate((struct vfs_file_desc *)fd, (vfs_off_t)length);
#else
    return -1;
#endif
}

int remove(char *filename)
{
#if FS_EN
    return vfs_unlink(filename);
#else
    return -1;
#endif
}

int rename(const char *old_filename, const char *new_filename)
{
#if FS_EN
    return vfs_rename(old_filename, new_filename);
#else
    return 0;
#endif
}

void rewind(FILE *stream)
{
#if FS_EN
    vfs_lseek((struct vfs_file_desc *)stream, 0, SEEK_SET);
#else
    return;
#endif
}

int vfprintf(FILE *stream, const char *format, va_list arg)
{
    (void)stream;
    (void)format;
    (void)arg;
    /* VFS 不提供格式化输出，返回错误 */
    return -1;
}

char *fgets(char *str, int n, FILE *stream)
{
#if FS_EN
    /* 逐字节读取直到换行或达到数量限制 */
    int i = 0;
    char c;
    while (i < n - 1) {
        vfs_ssize_t ret = vfs_read((struct vfs_file_desc *)stream, &c, 1);
        if (ret <= 0) {
            break;
        }
        str[i++] = c;
        if (c == '\n') {
            break;
        }
    }
    if (i > 0) {
        str[i] = '\0';
        return str;
    }
#else
    (void)str;
    (void)n;
    (void)stream;
#endif
    return NULL;
}

int fsync(int fd)
{
    int ret = 0;
    #if FS_EN
        ret = vfs_sync((struct vfs_file_desc *)fd);
    #endif
    return ret;
}

int getc(FILE *stream)
{
    uint8_t val = 0;
#if FS_EN
    vfs_read((struct vfs_file_desc *)stream, &val, 1);
#endif
    return val;
}

int stat(const char *path, struct stat *buf)
{
#if FS_EN
    int ret = vfs_stat(path, buf);
    (void)ret;
#endif
    return 0;
}

int fstat(int fd, struct stat *statbuf)
{
#if FS_EN
    /* VFS 不直接提供 fstat，返回 0 */
    (void)fd;
    (void)statbuf;
#endif
    return 0;
}

int write(int fd, const void *buf, size_t nbytes)
{
    if (fd > 0 && fd < 256) {
        return lwip_write(fd, buf, nbytes);
    } else {
#if FS_EN
        vfs_ssize_t ret = vfs_write((struct vfs_file_desc *)fd, buf, nbytes);
        return ret > 0 ? (int)ret : -1;
#else
        return -1;
#endif
    }
}

int read(int fd, void *buf, size_t nbytes)
{
    if (fd > 0 && fd < 256) {
        return lwip_read(fd, buf, nbytes);
    } else {
#if FS_EN
        vfs_ssize_t ret = vfs_read((struct vfs_file_desc *)fd, buf, nbytes);
        return ret > 0 ? (int)ret : -1;
#else
        return -1;
#endif
    }
}

off_t lseek(int fd, off_t offset, int whence)
{
#if FS_EN
    vfs_off_t ret = vfs_lseek((struct vfs_file_desc *)fd, (vfs_off_t)offset, whence);
    return ret < 0 ? (off_t) - 1 : (off_t)ret;
#else
    return 0;
#endif
}

int open(const char *path, int oflags, ...)
{
    int handler = 0;
#if FS_EN
    int flags = 0;

    /* 将 POSIX open 标志转换为 VFS 标志 */
    if (oflags & O_RDWR) {
        flags = VFS_O_RDWR;
    } else if (oflags & O_WRONLY) {
        flags = VFS_O_WRONLY;
    } else {
        flags = VFS_O_RDONLY;
    }

    if (oflags & O_CREAT) {
        flags |= VFS_O_CREAT;
    }
    if (oflags & O_TRUNC) {
        flags |= VFS_O_TRUNC;
    }
    if (oflags & O_APPEND) {
        flags |= VFS_O_APPEND;
    }

    handler = (int)vfs_open(path, flags, 0);
#endif
    return handler == 0 ? -1 : handler;
}

int close(int fd)
{
    if (fd > 0 && fd < 256) {
        return lwip_close(fd);
    } else {
#if FS_EN
        return vfs_close((struct vfs_file_desc *)fd);
#else
        return -1;
#endif
    }
}

int unlink(const char *path)
{
#if FS_EN
    return vfs_unlink(path);
#else
    return -1;
#endif
}

int fcntl(int fd, int cmd, ...)
{
    int arg = 0;
    va_list ap;

    va_start(ap, cmd);
    arg = va_arg(ap, int);
    va_end(ap);

    if (fd > 0 && fd < 256) {
        return lwip_fcntl(fd, cmd, arg);
    } else {
#if FS_EN
        /* VFS 不提供 fcntl，返回错误 */
        (void)fd;
        (void)cmd;
        (void)arg;
        return -1;
#else
        return -1;
#endif
    }
}

int ioctl(int fd, long cmd, void *argp)
{
    if (fd > 0 && fd < 256) {
        return lwip_ioctl(fd, cmd, argp);
    } else {
#if FS_EN
        /* VFS 不提供 ioctl，返回错误 */
        (void)fd;
        (void)cmd;
        (void)argp;
        return -1;
#else
        return -1;
#endif
    }
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
    if (fd > 0 && fd < 256) {
        return lwip_readv(fd, iov, iovcnt);
    } else {
        return -1;
    }
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
    if (fd > 0 && fd < 256) {
        return lwip_writev(fd, iov, iovcnt);
    } else {
        return -1;
    }
}

int fputs(const char *str, FILE *stream)
{
#if FS_EN
    return fwrite(str, 1, os_strlen(str), stream);
#else
    return 0;
#endif
}

int _fclose_r(void *rptr, FILE *fp)
{
    return fclose(fp);
}
int _fflush_r(void *rptr, FILE *fp)
{
    return fflush(fp);
}
#else
int _fclose_r(void *rptr, void *fp)
{
    return 0;
}
int _fflush_r(void *rptr, void *fp)
{
    return 0;
}
#endif

