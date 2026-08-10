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

typedef struct Node {
    int value;
    struct Node *next;
} Node;

typedef struct {
    Node *head;
    Node *tail;
    size_t size;
} LinkedList;

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
    LinkedList *list;   // 存放异常文件夹
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

void init_list(LinkedList *list)
{
    list->head = list->tail = NULL;
    list->size = 0;
}

void push_back(LinkedList *list, uint32_t value)
{
    Node *node = STREAM_MALLOC(sizeof(Node));
    if (!node) return;
    node->value = value;
    node->next = NULL;

    if (list->tail) {
        list->tail->next = node;
    } else {
        list->head = node;
    }
    list->tail = node;
    list->size++;
}

void free_list(void *loop_f)
{
    loop_file *loop = (loop_file *) loop_f;
    LinkedList *list = (LinkedList *) loop->list;
    if(list)
    {
        Node *cur = list->head;
        while (cur) {
            Node *next = cur->next;
            STREAM_FREE(cur);
            cur = next;
        }
        list->head = list->tail = NULL;
        list->size = 0;
        STREAM_FREE(list);
        loop->list = NULL;
    }
}

void* get_err_dir_list(void *loop_f)
{
    loop_file *loop = (loop_file *) loop_f;
    return loop->list;
}

void err_dir_add_list(void *loop_f, const char *dir_path)
{
    loop_file *loop = (loop_file *) loop_f;
    LinkedList *list = NULL;

    if(loop->list)
    {
        list = (LinkedList *) loop->list;
    }
    else
    {
        list = (LinkedList *)STREAM_MALLOC(sizeof(LinkedList));
        if(list)
        {
            loop->list = list;
            init_list(list);
        }
        else
        {
            _os_printf("malloc err list failed\r\n");
            return;
        }
    }

    const char *last_slash = strrchr(dir_path, '/');
    const char *num_ptr = (last_slash != NULL) ? (last_slash + 1) : dir_path;
    char *endptr;
    uint32_t value = strtoul(num_ptr, &endptr, 10);

    if (endptr == num_ptr || (*endptr != '\0')) {
        _os_printf("Error: '%s' is not a valid number directory\r\n", num_ptr);
        return;
    }

    if(value > 0)
    {
        Node *cur = list->head;
        while (cur) {
            if (cur->value == value) {
                _os_printf("dir %s is exist\r\n");
                return;
            }
            cur = cur->next;
        }

        push_back(list, value);
        _os_printf("add %s to err dir list\r\n", dir_path);
    }
}

static uint8_t find_dir_with_min_number(const char *path, char* result_name, LinkedList *list)
{
    void *dir;
    FILINFO *fil;
    uint32_t min_number = 0xFFFFFFFF;
    char min_dir_name[32];
    uint8_t err_dir = 0;
	
    dir = osal_opendir(path);
    if(dir == NULL) {
        os_printf("open dir failed, dir: %s\r\n", path);
        return 1;
    }

    while (1) {
        fil = osal_readdir(dir);
        
        if(fil == NULL)
        {
            break;
        }

        if(fil->fname[0] == '.')
        {
            continue;
        }
        
        if (osal_dirent_isdir(fil))
        {
            char *dir_name = fil->fname;
            uint32_t current_number = 0;
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
                if(list)
                {
                    Node *cur = list->head;
                    while (cur) {
                        if (cur->value == current_number) {
                            err_dir = 1;
                            break;
                        }
                        cur = cur->next;
                    }
                }
                if (current_number < min_number && !err_dir) {
                    min_number = current_number;
                    os_strcpy(min_dir_name, dir_name);
                }
                err_dir = 0;
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
    uint8_t extension_name_len = strlen(extension_name);

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

            if (fil->fattrib & AM_RDO) {
                os_printf("File is read-only\n");
                continue; 
            }

            // 检查后缀名是否匹配
            uint8_t extension_filename[16]; // 文件后缀名转换,统一大写

            char    *fname        = osal_dirent_name(fil);
            uint8_t  filename_len = strlen(fname);
            uint32_t filesize     = osal_dirent_size(fil);

            if (filename_len - extension_name_len > 0)
            {
                // 转换成大写
                for (uint8_t i = 0; i < extension_name_len; i++)
                {
                    extension_filename[i] = toupper(fname[filename_len - extension_name_len + i]);
                }
                // 后缀名匹配
                if (memcmp(extension_name, extension_filename, extension_name_len) == 0)
                {
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
    
    if(find_dir_with_min_number(rec_path, min_dir_name, NULL))
    {
        os_printf("fine dir error, path: %s\r\n", rec_path);
        return NULL;
    }

    os_sprintf(dir_path, "%s/%s", rec_path, min_dir_name);
    loop_file *loop = loop_get_file_init();
    if (loop)
    {
        loop->list = NULL;
        os_strcpy(loop->dir_path, dir_path);
        get_file(loop, dir_path, extension_name);
        // 排序
        loop_add_file_sort(loop);
    }
    
    return loop;
}

void *get_file_list2(const char *rec_path, const char *extension_name, void *list)
{
    char dir_path[32];
    char min_dir_name[32];
    
    if(find_dir_with_min_number(rec_path, min_dir_name, list))
    {
        os_printf("fine dir error, path: %s\r\n", rec_path);
        return NULL;
    }

    os_sprintf(dir_path, "%s/%s", rec_path, min_dir_name);
    loop_file *loop = loop_get_file_init();
    if (loop)
    {
        loop->list = list;
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
