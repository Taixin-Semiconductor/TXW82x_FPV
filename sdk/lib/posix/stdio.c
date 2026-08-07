#include "sys_config.h"
#include "typesdef.h"
#include "errno.h"
#include "osal/sleep.h"
#include "osal/string.h"
#include "lib/posix/stdio.h"
#include "lwip/sockets.h"

#ifdef TXWSDK_POSIX

#if FS_EN
#include "fatfs/osal_file.h"
#include "fatfs/ff.h"
#endif


#define FILE void

#define FS_PSRAM_BUFF 1

/* sram slice buffer size, usr for psram slice write to sd */
#define STDIO_RWBUF_SIZE 10000

extern int osal_open(const char *filename, int oflags, int mode);
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
    return osal_fexist(pathname);
#else
    return -1;
#endif
}

int fclose(FILE *stream)
{
#if FS_EN
    return osal_fclose((F_FILE *)stream);
#else
    return -1;
#endif
}

int feof(FILE *stream)
{
#if FS_EN
    return f_eof((F_FILE *)stream);
#else
    return -1;
#endif
}

int ferror(FILE *stream)
{
#if FS_EN
    return f_error((F_FILE *)stream);
#else
    return -1;
#endif
}

int fflush(FILE *stream)
{
#if FS_EN
    return fsync((int)stream);
#else
    return -1;
#endif
}

FILE *fopen(const char *filename, const char *mode)
{
#if FS_EN
    return (FILE *)osal_fopen(filename, mode);
#else
    return 0;
#endif
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
#if FS_EN
    return read((int)stream, ptr, size * nmemb);
#else
    return 0;
#endif
}

int fseek(FILE *stream, long int offset, int whence)
{
    // TODO: not support whence now
#if FS_EN
    return osal_fseek((F_FILE *)stream, offset);
#else
    return 0;
#endif
}

// FIXME: 仅支持获取文件大小, 不支持从指针位置开始
long ftell(FILE *stream)
{
#if FS_EN
    return f_size((FIL *)stream);
#else
    return 0;
#endif
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
#if FS_EN
    return write((int)stream, ptr, size *nmemb);
#else
    return 0;
#endif
}

// ADD truncate
int fileno(FILE *stream)
{
    return (int)stream;
}

int ftruncate(int fd, off_t length)
{
#if FS_EN
    return f_truncate((F_FILE *)fd);
#else
    return -1;
#endif
}

int remove(char *filename)
{
#if FS_EN
    return osal_unlink(filename);
#else
    return -1;
#endif
}

int rename(const char *old_filename, const char *new_filename)
{
#if FS_EN
    return osal_rename(old_filename, new_filename);
#else
    return 0;
#endif
}

void rewind(FILE *stream)
{
#if FS_EN
    f_rewind((FIL*)stream);
#else
    return;
#endif
}



int vfprintf(FILE *stream, const char *format, va_list arg)
{
#if FS_EN
	int f_vprintf (FIL* fp,const TCHAR* fmt,va_list arp);
    return f_vprintf((FIL*)stream, format, arg);
#else
    return -1;
#endif
}

/*
int fgetc(FILE *stream)
{
    uint8 val = 0;
    osal_fread(val, 1, 1, stream);
    return val;
}
*/

char *fgets(char *str, int n, FILE *stream)
{
#if FS_EN
    return f_gets(str, n, (FIL*)stream);
#else
    return NULL;
#endif
}

/*
int fputc(int c, FILE *stream)
{
    return f_putc(c, stream);
}

int fputs(const char *str, FILE *stream)
{
    return f_puts(str, stream);
}
*/

int fsync(int fd)
{
#if FS_EN
    return f_sync((F_FILE *)fd) == FR_OK ? 0 : -1;
#else
    return -1;
#endif
}

int getc(FILE *stream)
{
    uint8 val = 0;
#if FS_EN    
    osal_fread(&val, 1, 1, (F_FILE *)stream);
#endif
    return val;
}

/*
int putc(int c, FILE *stream)
{
    return f_putc(c, stream);
}
*/

int stat(const char *path, struct stat *buf)
{
#if FS_EN
    FILINFO *file_info = (FILINFO *)os_malloc(sizeof(FILINFO));
    f_stat(path, file_info);
    buf->st_size = file_info->fsize;
    buf->st_mtime = file_info->ftime;
    buf->fattrib = file_info->fattrib;
    buf->fname = NULL;
    os_free(file_info);
#endif
    return 0;
}

// TODO: just support file size for now
int fstat(int fd, struct stat *statbuf)
{
#if FS_EN
    F_FILE *file = (F_FILE *)fd;
    statbuf->st_size = f_size(file);
#endif
    return 0;
}

#if FS_EN && FS_PSRAM_BUFF
int write_from_psram(int fd, const char *psram_buf, size_t nbytes)
{
    int left_len = nbytes;
    int write_len = 0;
    int wlen = 0;
    int len = -1;        
    char *sram_buf = os_malloc(STDIO_RWBUF_SIZE);
    if(sram_buf == NULL){
        return -ENOMEM;
    }

    while (left_len) {
        wlen = left_len < STDIO_RWBUF_SIZE ? left_len : STDIO_RWBUF_SIZE;
        hw_memcpy(sram_buf, psram_buf + write_len, wlen);
        len = (int)osal_fwrite(sram_buf, wlen, 1, (F_FILE *)fd);
        if (len < wlen) {
            os_free(sram_buf);
            return write_len + len;
        }
        left_len -= wlen;
        write_len += wlen;
    }

    os_free(sram_buf);
    return write_len;
}
#endif

int write(int fd, const void *buf, size_t nbytes)
{
    if (fd > 0 && fd < 256) {
        return lwip_write(fd, buf, nbytes);
    } else {
#if FS_EN
    #if FS_PSRAM_BUFF
        return write_from_psram(fd, buf, nbytes);
    #else
        return osal_fwrite(buf, nbytes, 1, (F_FILE *)fd);
    #endif
#else
        return -1;
#endif
    }
}

#if FS_EN && FS_PSRAM_BUFF
int read_to_psram(int fd, char *psram_buf, size_t nbytes)
{
    int left_len = nbytes;
    int read_len = 0;
    int rlen = 0;
    int len = -1;
    char *sram_buff = os_malloc(STDIO_RWBUF_SIZE);
    if(sram_buff == NULL){
        return -ENOMEM;
    }

    while (left_len) {
        rlen = left_len < STDIO_RWBUF_SIZE ? left_len : STDIO_RWBUF_SIZE;
        len = (int)osal_fread(sram_buff, rlen, 1, (F_FILE *)fd);
        hw_memcpy(psram_buf + read_len, sram_buff, len);

        if (len < rlen) {
            os_free(sram_buff);
            return read_len + len;
        }

        left_len -= rlen;
        read_len += rlen;
    }

    os_free(sram_buff);
    return read_len;
}
#endif

int read(int fd, void *buf, size_t nbytes)
{
    if (fd > 0 && fd < 256) {
        return lwip_read(fd, buf, nbytes);
    } else {
#if FS_EN
    #if FS_PSRAM_BUFF
        return read_to_psram(fd, buf, nbytes);
    #else
        return osal_fread(buf, nbytes, 1, (F_FILE *)fd);
    #endif
#else
        return -1;
#endif
    }
}

off_t lseek(int fd, off_t offset, int whence)
{
    // TODO: whence not support now
#if FS_EN
    return osal_fseek((F_FILE *)fd, offset) == FR_OK ? offset : -1;
#else
    return 0;
#endif
}

int open(const char *path, int oflags, ...)
{
    int handler = 0;
#if FS_EN
    int flags = 0;

#if 0
    /* mode is not use */
    int mode = 0;
    va_list arg;

    va_start(arg, oflags);
    mode = va_arg(arg, int);
    va_end(arg);
#endif

    if (ACC_MODE(oflags) == O_RDWR) {
        flags = (FA_READ | FA_WRITE);
    } else if (ACC_MODE(oflags) ==  O_RDONLY) {
        flags = FA_READ;
    } else {
        flags = FA_WRITE;
    }

    if (oflags & O_CREAT) {
        flags |= FA_CREATE_ALWAYS;
    }

    handler = (int)osal_open(path, 0, flags);
#endif
    return handler == 0 ? -1 : handler;
}

int close(int fd)
{
    if (fd > 0 && fd < 256) {
        return lwip_close(fd);
    } else {
#if FS_EN
        return osal_fclose((F_FILE *)fd);
#else
        return -1;
#endif
    }
}

int unlink(const char *path)
{
#if FS_EN
    return osal_unlink(path);
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
        return f_cntl((FIL *)fd, cmd, arg);
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
        return f_ioctl((FIL *)fd, cmd, argp);
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

int _fclose_r (void *rptr, FILE * fp)
{
    return fclose(fp);
}
int	_fflush_r (void *rptr, FILE *fp)
{
    return fflush(fp);
}
#else
int _fclose_r (void *rptr, void * fp)
{
    return 0;
}
int	_fflush_r (void *rptr, void *fp)
{
    return 0;
}
#endif

