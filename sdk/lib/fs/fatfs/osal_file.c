#include "typesdef.h"
#include "fatfs/ff.h"
// #include "osal.h"
#include <string.h>
#include "osal_file.h"
#include "osal/string.h"
#include "osal/task.h"
#include "lib/common/common.h"

// 结构体申请空间函数
#ifdef MORE_SRAM 
#define FILE_MALLOC os_malloc_psram
#define FILE_FREE os_free_psram
#define FILE_ZALLOC os_zalloc_psram
#else
#define FILE_MALLOC os_malloc
#define FILE_FREE os_free
#define FILE_ZALLOC os_zalloc
#endif

uint32_t file_mode(const char *mode)
{
    uint32_t flags = 0;
    if (!mode) {
        return 0;
    }

    switch (mode[0]) {
        case 'r':
            flags |= FA_READ;
            break;
        case 'w':
            flags |= FA_WRITE | FA_OPEN_ALWAYS;
            break;
        case 'a':
            flags |= FA_WRITE | FA_OPEN_APPEND;
            break;
        default:
            return 0;
    }

    for (const char *p = mode + 1; *p; p++) {
        switch (*p) {
            case '+':
                flags |= FA_READ | FA_WRITE;
                break;
            case 'x':
                flags |= FA_CREATE_NEW;
                break;
        }
    }

    return flags;
}

F_FILE *osal_open(const char *filename, int oflags, int mode)
{
    uint32_t res;
    FIL     *fp = FILE_MALLOC(sizeof(FIL));
    if (!fp)
    {
        return 0;
    }
    res = f_open(fp, filename, mode);
    if (res == FR_OK)
    {
        return fp;
    }
    else
    {
        os_printf("%s res:%d\tfilename:%s\n", __FUNCTION__, res, filename);
        set_errno(res);
        FILE_FREE(fp);
        return 0; // return NULL;
    }
}

F_FILE *osal_fopen(const char *filename, const char *mode)
{
    return osal_open(filename, 0, file_mode(mode));
}

// 返回值是读取到的字节数,与标准c返回值有一点点区别
uint32_t osal_fread(void *ptr, uint32_t size, uint32_t nmemb, F_FILE *fp)
{

#ifdef WIN32
    return fread(ptr, size, nmemb, fp);
#else
    uint32_t readLen;
    uint32_t res = f_read(fp, ptr, size * nmemb, &readLen);
    if (res == FR_OK)
    {
        return readLen;
    }
    else
    {
        os_printf("%s res:%d\n", __FUNCTION__, res);
        set_errno(res);
        return 0;
    }
#endif
}

uint32_t osal_fwrite(void *ptr, uint32_t size, uint32_t nmemb, F_FILE *fp)
{

#ifdef WIN32
    return fwrite(ptr, size, nmemb, fp);
#else
    uint32_t writeLen;
    uint32_t res = f_write(fp, ptr, size * nmemb, &writeLen);
    if (res == FR_OK)
    {
        return writeLen;
    }
    else
    {
        set_errno(res);
        return 0;
    }
#endif
    // return size*nmemb;
}

int osal_fclose(F_FILE *fp)
{
#ifdef WIN32
    return fclose(fp);
#else
    int res = f_close(fp);
    FILE_FREE(fp);
    if (!res)
    {
        set_errno(res);
    }
    return res;
#endif
}

uint32_t osal_ftell(F_FILE *fp)
{
#ifdef WIN32
    return ftell(fp);
#else
    return f_tell(fp);
#endif
}

uint32_t osal_fseek(F_FILE *fp, uint32_t offset)
{
#ifdef WIN32
    return fseek(fp, offset, SEEK_SET);
#else
    return f_lseek(fp, offset);
#endif
}

uint32_t osal_fsize(F_FILE *fp)
{
#ifdef WIN32
    uint32_t tmp;
    uint32_t size;
    tmp = osal_ftell(fp);
    fseek(fp, 0, SEEK_END);
    size = osal_ftell(fp);
    fseek(fp, tmp, SEEK_SET);
    return size;
#else
    return f_size(fp);
#endif
}

char *osal_getcwd(char *buf, uint32_t len)
{
    if (FR_OK != f_getcwd(buf, len))
    {
        return NULL;
    }
    return buf;
}

int osal_chdir(char *buf)
{
    int res;
    if (buf[1] == ':')
    {
        f_chdrive(buf);
        if (FR_OK != f_chdir(buf + 2))
        {
            return -1;
        }
    }
    else
    {
        res = f_chdir(buf);
        if (FR_OK != res)
        {
            os_printf("buf:%s\tres:%d\r\n", buf, res);
            return -1;
        }
    }
    return 0;
}

struct diriter
{
    DIR     dir;
    FILINFO fil;
};

void *osal_opendir(const char *path)
{
    struct diriter *dir;
    dir = FILE_MALLOC(sizeof(struct diriter));
    if (!dir)
    {
        return NULL;
    }

    if (f_opendir(&dir->dir, path) == FR_OK)
    {
        return dir;
    }

    FILE_FREE(dir);
    return NULL;
}

void osal_closedir(void *HDIR)
{
    struct diriter *dir = (struct diriter *) HDIR;
    if (!dir)
    {
        return;
    }

    FILE_FREE(dir);
}

void *osal_readdir(void *HDIR)
{
    struct diriter *dir = (struct diriter *) HDIR;
    if (!dir)
    {
        return NULL;
    }

    if (f_readdir(&dir->dir, &dir->fil) != FR_OK)
    {
        return NULL;
    }

    if (dir->dir.sect == 0)
    {
        return NULL;
    }
    return &dir->fil;
}

char *osal_dirent_name(void *HFIL)
{
    FILINFO *fil = (FILINFO *) HFIL;
    if (!fil)
    {
        return NULL;
    }
    return fil->fname;
}

int osal_dirent_isdir(void *HFIL)
{
    FILINFO *fil = (FILINFO *) HFIL;
    if (!fil)
    {
        return 0;
    }
    return fil->fattrib & AM_DIR;
}

uint32_t osal_dirent_date(void *HFIL)
{
    FILINFO *fil = (FILINFO *) HFIL;
    return fil->fdate;
}

uint32_t osal_dirent_time(void *HFIL)
{
    FILINFO *fil = (FILINFO *) HFIL;
    if (!fil)
    {
        return 0;
    }
    return fil->ftime;
}

uint32_t osal_dirent_size(void *HFIL)
{
    FILINFO *fil = (FILINFO *) HFIL;
    return fil->fsize;
}

uint32_t osal_fmkdir(const char *dir)
{
    return f_mkdir(dir);
}

FRESULT osal_stat(const TCHAR *path, FILINFO *fno)
{
    return f_stat(path, fno);
}

FRESULT osal_unlink(const TCHAR *path)
{
    return f_unlink(path);
}

FRESULT osal_rename(const TCHAR *oldpath, const TCHAR *newpath)
{
    return f_rename(oldpath, newpath);
}

FRESULT osal_fchmod(const TCHAR *path, uint8 attr, uint8 mask)
{
    return f_chmod(path, attr, mask);
}

FRESULT osal_fstat(const TCHAR *path, FILINFO *fno)
{
    return f_stat(path, fno);
}

// 判断是否存在某个文件
FRESULT osal_fexist(const char *name)
{
    FIL *fp;
    fp = osal_fopen(name, "rb");
    if (fp)
    {
        osal_fclose(fp);
        return 0;
    }

    return 1;
}

FRESULT osal_ftruncate(F_FILE *fp)
{
    return f_truncate(fp);
}

FRESULT osal_fsync(void *fp)
{
    return f_sync((FIL *) fp);
}

/******************************************************************************
 * 非标准接口
 *****************************************************************************/

FRESULT osal_fatfsfree(const char *path, uint32_t *totalsize, uint32_t *freesize)
{
    FATFS  *fs;
    DWORD   fre_clust, fre_sect, tot_sect;
    FRESULT res = f_getfree(path, &fre_clust, &fs);
    if (!res)
    {
        tot_sect  = (fs->n_fatent - 2) * fs->csize;
        fre_sect  = fre_clust * fs->csize;
        if(totalsize)
            *totalsize = tot_sect / 2 / 1024;
        if(freesize)
            *freesize = fre_sect / 2 / 1024;
    }
    else
    {
        os_printf("%s:%d\tres:%d\n",__FUNCTION__,__LINE__,res);
    }
    return res;
}

// 需要保证path有足够的空间(比filepath大)
FRESULT osal_auto_create_dirs(const char *filepath, char *path,uint8_t ishid)
{
    FRESULT fr;
    DIR     dir;
    char   *p; // 用于遍历路径的指针

    // 1. 从文件路径中提取目录路径
    strcpy(path, filepath);

    // 找到最后一个路径分隔符，将其后的内容（文件名）截断
    p = strrchr(path, '/');
    if (p == NULL)
    {
        // 如果路径中没有目录，只有文件名，则无需创建目录
        return FR_OK;
    }
    *p = '\0'; // 现在path里就是纯目录路径了

    // 2. 尝试打开该目录，如果成功说明目录已存在
    fr = f_opendir(&dir, path);
    if (fr == FR_OK)
    {
        f_closedir(&dir);
        return FR_OK; // 目录已存在，直接返回成功
    }
    // 3. 如果目录不存在，则开始逐级创建
    // 从根目录开始遍历路径
    p = path;
    if (*p == '/')
    {
        p++; // 如果以'/'开头，跳过第一个（根据你的实际路径格式调整）
    }

    while ((p = strchr(p, '/')) != NULL)
    {
        *p = '\0'; // 在分隔符处临时截断，得到当前要检查的子路径
        // 尝试打开当前层级的目录
        fr = f_opendir(&dir, path);
        if (fr == FR_OK)
        {
            f_closedir(&dir); // 当前层级目录存在，继续下一级
        }
        else if (fr == FR_NO_PATH)
        {
            // 当前层级目录不存在，创建它
            fr = f_mkdir(path);
            if (fr != FR_OK && fr != FR_EXIST)
            {              // 忽略FR_EXIST错误，可能其他线程已创建
                return fr; // 创建失败，返回错误
            }
            if(ishid)
            {
                f_chmod(path,AM_HID,AM_HID);
            }
        }
        else
        {
            return fr; // 其他错误，返回
        }
        *p = '/'; // 恢复分隔符
        p++;      // 移动到下一级
    }

    // 4. 最后再尝试一次打开最终目录，确保创建成功
    fr = f_opendir(&dir, path);
    if (fr == FR_OK)
    {
        f_closedir(&dir);
        return FR_OK;
    }
    else
    {
        // 最终目录创建仍未成功
        fr = f_mkdir(path);
        if (fr != FR_OK && fr != FR_EXIST)
        {
        }
        if(ishid)
        {
            f_chmod(path,AM_HID,AM_HID);
        }
        return fr;
    }
}

FRESULT delete_directory_recursive(const TCHAR *path)
{
    FRESULT res;
    DIR     dir;
    FILINFO fno;

    res = f_opendir(&dir, path);
    if (res != FR_OK)
    {
        return res;
    }

    while (1)
    {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0)
        {
            break;
        }

        if (fno.fname[0] == '.')
        {
            continue;
        }

        TCHAR full_path[256];
        sprintf(full_path, "%s/%s", path, fno.fname);

        if (fno.fattrib & AM_DIR)
        {
            res = delete_directory_recursive(full_path);
            if (res != FR_OK)
            {
                f_closedir(&dir);
                return res;
            }
        }
        else
        {
            res = f_unlink(full_path);
            os_printf("delete file %s res = %d\r\n", full_path, res);
            if (res != FR_OK)
            {
                f_closedir(&dir);
                return res;
            }
        }
    }

    f_closedir(&dir);

    return f_unlink(path);
}

FRESULT osal_unlink_dir(const TCHAR *path, uint8_t force)
{
    // 使用普通删除方式（只能删除空目录）
    if (force == 0)
    {
        return f_unlink(path);
    }
    // 强制删除目录及其所有内容
    else if (force == 1)
    {
        return delete_directory_recursive(path);
    }
    return FR_INVALID_PARAMETER;
}

FRESULT rename_dir_in_directory(const TCHAR *path, const TCHAR *base_path)
{
    FRESULT    res;
    DIR        dir;
    static int file_count = 1;

    // 打开目录
    res = f_opendir(&dir, path);
    if (res != FR_OK)
    {
        return res;
    }

    TCHAR new_path[256];
    os_sprintf(new_path, "%s/ERR%d", base_path, file_count++);

    res = f_rename(path, new_path);
    while (res != FR_OK)
    {
        f_closedir(&dir);
        os_printf("rename file error\r\n");
        res = rename_dir_in_directory(path, base_path);
        if (res == FR_OK)
        {
            break;
        }
    }
    _os_printf("dir rename success, new path: %s\r\n", new_path);

    f_closedir(&dir);

    return FR_OK;
}

F_FILE *osal_fopen_auto(const char *filename, const char *mode,uint8_t ishid)
{
    F_FILE *fp = osal_open(filename, 0, file_mode(mode));
    FRESULT res;
    if (!fp)
    {
        if (get_errno() == FR_NO_PATH)
        {
            char path[128]; // 路径缓冲区，根据需求调整大小,用的是任务栈,注意太大的话有可能任务栈也需要加大
            res = osal_auto_create_dirs(filename, path,ishid);
            // 如果创建目录成功,重新创建文件
            if (!res)
            {
                fp = osal_open(filename, 0, file_mode(mode));
            }
        }
    }
    return fp;
}
