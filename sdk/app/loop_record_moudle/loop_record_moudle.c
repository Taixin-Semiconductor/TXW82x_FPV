#include "fatfs/osal_file.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "typesdef.h"
#include "utlist.h"
#include "loop_record_moudle/loop_record_moudle.h"

// 结构体申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

typedef struct el
{
    struct el *next, *prev;
    uint32_t   filesize;
    char       filename[32];
} el;

typedef struct
{
    el *head;
    char dir_path[32];
} loop_file;

// 默认当作后缀名是4位,这个应该是用户去考虑的
// 主要用于排序
static uint8_t file_cmp(char *dir1, char *dir2)
{
    int compare = 0;
    compare = os_strcmp(dir1, dir2);
    if(compare < 0)
        return 0;
    else
        return 1;
}

static int namecmp(el *a, el *b)
{
    return file_cmp(a->filename, b->filename);
}

static loop_file *loop_get_file_init()
{
    loop_file *loop = (loop_file *) STREAM_ZALLOC(sizeof(loop_file));
    return loop;
}

static void loop_get_file_deinit(void *loop_f)
{
    loop_file *loop = (loop_file *) loop_f;
    if (loop)
    {
        el *el, *tmp;
        LL_FOREACH_SAFE(loop->head, el, tmp)
        {
            DL_DELETE(loop->head, el);
            STREAM_FREE(el);
        }
        STREAM_FREE(loop);
    }
}

static int8_t loop_add_file(void *loop_f, char *file_name, int filesize)
{
    uint8_t    ret  = 0;
    loop_file *loop = (loop_file *) loop_f;
    if (loop)
    {
        el *node = (el *) STREAM_MALLOC(sizeof(el));
        if (node)
        {
            node->filesize = filesize;
            strcpy(node->filename, file_name);
            DL_APPEND(loop->head, node);
        }
        else
        {
            ret = RET_ERR;
        }
    }
    else
    {
        ret = RET_ERR;
    }
    return ret;
}


static int8_t loop_add_file_sort(void *loop_f)
{
    loop_file *loop = (loop_file *) loop_f;
    if (loop)
    {
        DL_SORT(loop->head, namecmp);
    }
    return 0;
}

static uint8_t find_dir_with_min_number(const char *path, char* result_name)
{
    void *dir;
    FILINFO *fil;
    
    unsigned int min_number = 0xFFFFFFFF;
    char min_dir_name[32];
	
    dir = osal_opendir(path);
    if(dir == NULL) {
        os_printf("open dir failed, dir: %s\r\n", path);
        return 1;
    }

    while (1) {
        fil = osal_readdir(dir);
        
        if(fil == NULL)
            break;

        if (fil->fname[0] == '.') {
            continue;
        }
        
        if (osal_dirent_isdir(fil))
        {
            char *dir_name = fil->fname;
            unsigned int current_number = 0;
            int found_digit = 0;
            
            for (int i = 0; dir_name[i] != '\0'; i++) {
                if (dir_name[i] >= '0' && dir_name[i] <= '9') {
                    current_number = current_number * 10 + (dir_name[i] - '0');
                } else {
                    found_digit = 0;
                    break;
                }
                found_digit = 1;
            }

            if (found_digit) {
                if (current_number < min_number) {
                    min_number = current_number;
                    os_strcpy(min_dir_name, dir_name);
                }
            }
        }
    }

    osal_closedir(dir);

    if (min_number != 0xFFFFFFFF) {
        strcpy(result_name, min_dir_name);
        return 0;
    } else {
        return 1;
    }
}

static void get_file(void *loop_f, const char *path, const char *extension_name)
{
    loop_file *loop = (loop_file *) loop_f;
    void      *dir;
    FILINFO   *fil;
    FRESULT   res = 0;

    dir = osal_opendir(path);
    if (!dir)
    {
        os_printf("get_dir res: %d\n", res);
        return;
    }
    do
    {
        // dent = VFS_readdir(dir);
        fil = osal_readdir(dir);
        if (fil)
        {
            if (osal_dirent_isdir(fil))
            {
                continue;
            }
            // 检查后缀名是否匹配
            uint8_t extension_filename[16]; // 文件后缀名转换,统一大写
            uint8_t extension_name_len = strlen(extension_name);

            char    *fname        = osal_dirent_name(fil);
            uint8_t  filename_len = strlen(fname);
            uint32_t filesize     = osal_dirent_size(fil);
            // 进行全部转换成大写

            if (filename_len - extension_name_len > 0)
            {
                for (uint8_t i = 0; i < strlen(extension_name); i++)
                {
                    extension_filename[i] = toupper(fname[filename_len - extension_name_len + i]);
                }
                // 后缀名匹配
                if (memcmp(extension_name, extension_filename, extension_name_len) == 0)
                {
                    // _os_printf("fname:%s\tsize:%d\n", fname,filesize);
                    el *name       = (el *) STREAM_MALLOC(sizeof(el));
                    name->filesize = filesize;
                    os_strncpy(name->filename, fname, sizeof(name->filename));
                    DL_APPEND(loop->head, name);
                }
            }
        }
    } while (fil);
    osal_closedir(dir);
}

// 读取文件夹列表
void *get_file_list(const char *rec_path, const char *extension_name)
{
    char dir_path[32];
    char min_dir_name[32];
    
    if(find_dir_with_min_number(rec_path, min_dir_name))
    {
        os_printf("fine dir error, path: %s\r\n", rec_path);
        return NULL;
    }

    os_sprintf(dir_path, "%s/%s", rec_path, min_dir_name);
    loop_file *loop = loop_get_file_init();
    if (loop)
    {
        os_strcpy(loop->dir_path, dir_path);
        get_file(loop, dir_path, extension_name);
        // 排序
        loop_add_file_sort(loop);
    }
    
    return loop;
}

void free_file_list(void *loop_f)
{
    loop_get_file_deinit(loop_f);
}

void *get_file_node(void *loop_f)
{
    el        *el   = NULL, *tmp;
    loop_file *loop = (loop_file *) loop_f;
    if (loop)
    {
        LL_FOREACH_SAFE(loop->head, el, tmp)
        {
            DL_DELETE(loop->head, el);
            break;
        }
    }
    return el;
}

void free_file_node(void *node)
{
    el *file_node = (el *) node;
    if (file_node)
    {
        STREAM_FREE(file_node);
    }
}

char *get_file_name(void *node)
{
    el *file_node = (el *) node;
    if (file_node)
    {
        return file_node->filename;
    }
    return NULL;
}

uint32_t get_file_size(void *node)
{
    el *file_node = (el *) node;
    if (file_node)
    {
        return file_node->filesize;
    }
    return 0;
}

void *get_file_dir(void *loop_f)
{
    loop_file *loop = (loop_file *) loop_f;
    if (loop)
    {
        if(loop->dir_path)
        {
            return loop->dir_path;
        }
    }
    return NULL;
}

void *get_min_file(void *loop_f)
{
    el        *el   = NULL, *tmp;
    loop_file *loop = (loop_file *) loop_f;
    if (loop)
    {
        LL_FOREACH_SAFE(loop->head, el, tmp)
        {
            return el->filename;
        }
    }
    return NULL;
}
