#include "cjson/cjson.h"
#include "lib/fs/fatfs/osal_file.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "vfs.h"
#include "file_thumb.h"
#include "file_common_api.h"

/*******************************************************************************
 * 这个文件主要是为了读取图片和缩略图用的,所以文件路径是固定
 *
 ******************************************************************************/
// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

#define HASH_MOD (9)

uint32_t calc_hash(const char *filename)
{
    uint32_t    hash   = 0;
    const char *h_name = (const char *) filename;
    uint8_t     t;
    // 遇到'.'代表后缀名,则不需要计算了
    while (*h_name && (*h_name) != '.')
    {
        t    = tolower(*h_name);
        hash = (hash + (hash << 2)) ^ t;
        h_name++;
    }
    hash = hash % HASH_MOD;
    return hash;
}

uint32_t calc_hash2(const char *filename, uint32_t *pre_name_len)
{
    uint32_t    hash   = 0;
    const char *h_name = (const char *) filename;
    uint8_t     t;
    // 遇到'.'代表后缀名,则不需要计算了
    while (*h_name && (*h_name) != '.')
    {
        t    = tolower(*h_name);
        hash = (hash + (hash << 2)) ^ t;
        h_name++;
    }
    hash = hash % HASH_MOD;
    if (pre_name_len)
    {
        *pre_name_len = h_name - filename;
    }
    return hash;
}

// pathsize后续可以用来判断是否越界的,现在没有用上
// path保存路径的buf
uint8_t gen_thumb_path(const char *filename, char *path, uint32_t pathsize)
{
    uint32_t name_len;
    char name[32];
    const char *p;
    p = strrchr(filename, ':');
    if(!p)
    {
        p = filename;
    }
    else
    {
        p++;
    }
    filename = p;
    p = strrchr(p, '/');
    if(p)
    {
        p++;
    }
    else
    {
        p = filename;
    }
    filename = p;
    os_snprintf(name, sizeof(name), "%.17s", filename);

    uint32_t hash = calc_hash2(name, &name_len);
    os_sprintf((char *) path, THUMB_PATH "/%d/%s", hash, name);
    // 将后缀名修改成jpg
    uint32_t offset = os_strlen(path);
    path[offset++]  = '.';
    path[offset++]  = 'J';
    path[offset++]  = 'P';
    path[offset++]  = 'G';
    path[offset]    = '\0';

    os_printf("%s:%d\tpath:%s\n", __FUNCTION__, __LINE__, path);
    return 0;
}

uint8_t gen_normal_jpg_path2(const char *filename, char *path, uint32_t pathsize)
{
    uint32_t name_len;
    calc_hash2(filename, &name_len);
    os_sprintf((char *) path, IMG_PATH "/%s", filename);
    // 将后缀名修改成jpg
    uint32_t offset = os_strlen(path) - os_strlen(filename) + name_len;
    path[offset++]  = '.';
    path[offset++]  = 'J';
    path[offset++]  = 'P';
    path[offset++]  = 'G';

    os_printf("%s:%d\tpath:%s\n", __FUNCTION__, __LINE__, path);
    return 0;
}

void *jpg_file_read(const char *filename, uint8_t thumb, int32_t *filesize)
{
    char     path[128];
    uint8_t *file_buf = NULL;
    // 寻找缩略图
    if (thumb)
    {
        // 得到缩略图的路径
        gen_thumb_path(filename, path, sizeof(path));
    }
    else
    {
        gen_photo_path(filename, path, sizeof(path));
    }

    // 读取文件
    // int fd = VFS_open((const char *)path,O_RDONLY);
    F_FILE *fd = osal_fopen((const char *) path, "rb");
    // if(fd >= 0)
    if (fd)
    {
        //*filesize = VFS_fsize(fd);
        *filesize = osal_fsize(fd);
        os_printf("filesize:%d\tpath:%s\n", *filesize, path);
        if (*filesize <= 0)
        {
            goto jpg_file_read_end;
        }
        file_buf = STREAM_MALLOC((*filesize));
        os_printf("file_buf:%X\n", file_buf);
        if (!file_buf)
        {
            goto jpg_file_read_end;
        }
        osal_fread(file_buf, 1, *filesize, fd);
        osal_fclose(fd);
        // VFS_read(fd,file_buf,*filesize);
        // VFS_close(fd);
        // fd = -1;
        fd = NULL;
    }
jpg_file_read_end:
    // if(fd >= 0)
    if (fd)
    {
        // VFS_close(fd);
        osal_fclose(fd);
    }
    return file_buf;
}

// 可以释放在buf增加容错
void jpg_file_free(void *buf)
{
    if (buf)
    {
        STREAM_FREE(buf);
    }
}

/***********************************************
返回图片的json结构,返回某给后缀名字的文件
参数:search_dir:搜索的目录
       extension_name:后缀名
       gen_prefix:生成特定前缀
       *count:返回array数量,因为这个返回是文件列表,所以有数量,
***********************************************/

cJSON *get_extension_json(const char *search_dir, const char *extension_name, const char *gen_prefix, int type, int *count)
{
    // fs_dir_t *dir;
    // struct dirent *dent;
    cJSON   *array_json  = NULL;
    cJSON   *path_array  = NULL;
    int      event_count = 0;
    char     path[128];
    FILINFO *fil;
    // DIR dh;
    // dir = VFS_opendir(search_dir);
    void    *dir = osal_opendir((char *) search_dir);
    if (dir)
    {
        path_array = cJSON_CreateArray();
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
                        array_json = cJSON_CreateObject();
                        os_sprintf((char *) path, "%s%s", gen_prefix, fname);
                        cJSON_AddItemToObject(array_json, "name", cJSON_CreateString(path));
                        cJSON_AddItemToObject(array_json, "size", cJSON_CreateNumber(filesize / 1024));
                        cJSON_AddItemToObject(array_json, "createtimestr", cJSON_CreateString("20250318103006"));
                        cJSON_AddItemToObject(array_json, "type", cJSON_CreateNumber(type));
                        // 加入到json
                        cJSON_AddItemToArray(path_array, array_json);
                        event_count++;
                    }
                }
            }
        } while (fil);
        // VFS_closedir(dir);
        osal_closedir(dir);
    }
    *count = event_count;
    return path_array;
}

char *photo_json_path()
{
    cJSON *root        = NULL;
    cJSON *loop        = NULL;
    cJSON *event       = NULL;
    cJSON *path_array  = NULL;
    cJSON *info_array  = cJSON_CreateArray();
    cJSON *video_array = NULL;
    char  *json_str    = NULL;
    int    event_count = 0;
    // 视频这里默认先返回空数据
    loop               = cJSON_CreateObject();
    cJSON_AddItemToObject(loop, "folder", cJSON_CreateString("loop"));
    video_array = get_extension_json("0:" MAIN_PATH "/" NORMAL_PATH, MP4_EXTENSION_NAME, "loop/RECA/", 2, &event_count);

    if (video_array)
    {
        cJSON_AddItemToObject(loop, "files", video_array);
        cJSON_AddItemToObject(loop, "count", cJSON_CreateNumber(event_count));
    }

    event = cJSON_CreateObject();
    cJSON_AddItemToObject(event, "folder", cJSON_CreateString("event"));
    path_array = get_extension_json("0:" MAIN_PATH "/" NORMAL_PATH, JPG_EXTENSION_NAME, "event/IMGA/", 1, &event_count);
    if (path_array)
    {
        cJSON_AddItemToObject(event, "files", path_array);
        cJSON_AddItemToObject(event, "count", cJSON_CreateNumber(event_count));
    }

    cJSON_AddItemToArray(info_array, loop);
    cJSON_AddItemToArray(info_array, event);
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "result", cJSON_CreateNumber(0));
    cJSON_AddItemToObject(root, "info", info_array);

    // 生成json数据
    json_str = cJSON_PrintUnformatted(root);

    cJSON_Delete(root);
    return json_str;
}

void path_json_free(void *str)
{
    if (str)
    {
        cJSON_free(str);
    }
}
