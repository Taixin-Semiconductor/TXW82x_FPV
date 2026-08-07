#ifndef _HGIC_POSIX_FS_H_
#define _HGIC_POSIX_FS_H_
#include "typesdef.h"
#include <stdarg.h>

#define _SYS_STAT_H

#ifdef __cplusplus
extern "C" {
#endif


#ifndef O_ACCMODE
#define O_ACCMODE       00000003
#endif

#ifndef O_CREAT
#define O_CREAT       0100
#endif

#define ACC_MODE(x) ((x)&O_ACCMODE)

#ifndef _OFF_T_DECLARED
typedef	unsigned long off_t;		/* file offset */
#define	_OFF_T_DECLARED
#endif

#ifndef _MODE_T_DECLARED
typedef	unsigned int mode_t;		/* permissions */
#define	_MODE_T_DECLARED
#endif

#ifndef F_DUPFD
#define	F_DUPFD		0	/* Duplicate fildes */
#endif
#ifndef F_GETFD
#define	F_GETFD		1	/* Get fildes flags (close on exec) */
#endif
#ifndef F_SETFD
#define	F_SETFD		2	/* Set fildes flags (close on exec) */
#endif
#ifndef F_GETFL
#define	F_GETFL		3	/* Get file flags */
#endif
#ifndef F_SETFL
#define	F_SETFL		4	/* Set file flags */
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK      04000
#endif
#ifndef O_NDELAY
#define O_NDELAY        O_NONBLOCK
#endif
#ifndef O_RDONLY
#define	O_RDONLY	0		/* +1 == FREAD */
#endif
#ifndef O_WRONLY
#define	O_WRONLY	1		/* +1 == FWRITE */
#endif
#ifndef O_RDWR
#define	O_RDWR		2		/* +1 == FREAD|FWRITE */
#endif

off_t lseek(int fd, off_t offset, int whence);
int fsync(int fd);

struct __stdio_file{
    int fd;
};

struct stat {
    unsigned int    st_size;        /* File size */
    unsigned int    st_mtime;       /* Modified date */
    unsigned char   fattrib;        /* File attribute */
    char            *fname;         /* TODO: long file name File name */
};

extern int stat(const char *__file, struct stat *__buf);
extern int fstat(int fd, struct stat *statbuf);
extern int access(const char *pathname, int mode);
extern int fcntl (int __fd, int __cmd, ...);

#define F_OK 0
#define R_OK 1
#define W_OK 2
#define X_OK 3

#ifdef __cplusplus
}
#endif

#endif

