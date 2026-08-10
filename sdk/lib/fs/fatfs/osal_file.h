#ifndef __OSAL_FILE_H
#define __OSAL_FILE_H
#include "typesdef.h"

#ifdef WIN32
	#include <stdio.h>
	#include <stdlib.h>
	typedef FILE	F_FILE;
	
//不是win32,是mcu平台则要声明文件类型,以及一些宏实现
#else
	#include "fatfs/ff.h"
	typedef FIL	F_FILE;
#endif

int osal_fopen_no_malloc(F_FILE *fp,const char *filename,const char *mode);
F_FILE *osal_fopen(const char *filename,const char *mode);
uint32_t osal_fread(void *ptr,uint32_t size,uint32_t nmemb,F_FILE *fp);
uint32_t osal_fwrite(void *ptr,uint32_t size,uint32_t nmemb,F_FILE *fp);
int osal_fclose(F_FILE *fp);
uint32_t osal_ftell(F_FILE *fp);
uint32_t osal_fseek(F_FILE *fp,uint32_t offset);
uint32_t osal_fsize(F_FILE *fp);


FRESULT osal_fexist (const char *name);
FRESULT osal_fstat(const TCHAR* path,FILINFO* fno);
FRESULT osal_fchmod(const TCHAR* path,uint8 attr,uint8 mask);
FRESULT osal_rename(const TCHAR* oldpath,const TCHAR* newpath);
FRESULT osal_unlink(const TCHAR* path);
FRESULT osal_unlink_dir(const TCHAR* path, uint8_t force);
FRESULT osal_stat(const TCHAR* path,FILINFO* fno);
uint32_t osal_fmkdir(const char *dir);
uint32_t osal_dirent_size ( void *HFIL );
uint32_t osal_dirent_date ( void *HFIL );
uint32_t osal_dirent_time ( void *HFIL);
int osal_dirent_isdir ( void *HFIL );
char *osal_dirent_name ( void *HFIL);
void *osal_readdir( void *HDIR );
void osal_closedir (void *HDIR);
void *osal_opendir(const char *path);
int osal_chdir (char *buf);
char *osal_getcwd (char *buf, uint32_t len);

uint32_t osal_fwrite2 (const char *buf, uint32_t is, uint32_t s, void *fp);
uint32_t osal_fread2 (char *buf, uint32_t is, uint32_t s, void *fp);
FRESULT osal_unlink(const TCHAR* path);
uint32_t file_mode (const char *mode);
FRESULT osal_fsync (void *fp);
FRESULT osal_fatfsfree(const char *path, uint32_t *totalsize, uint32_t *freesize);
FRESULT osal_ftruncate (F_FILE *fp);
FRESULT osal_auto_create_dirs(const char* filepath,char *path,uint8_t ishid);
F_FILE *osal_fopen_auto(const char *filename,const char *mode,uint8_t ishid);

#endif
