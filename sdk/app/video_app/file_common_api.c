#include "file_common_api.h"
#include "lib/fs/fatfs/osal_file.h"
#include "lib/common/timezone.h"
#include "app/video_app/file_thumb.h"
#include "osal/string.h"
#include "lwip/sockets.h"

static uint8_t get_rtc_time_str(char *timestr, uint8_t timestr_len, struct timeval *t)
{
    struct tm time_info;
    struct timeval *time = t;
    struct timeval ptimeval;
    if(!time)
    {
        time = &ptimeval;
        gettimeofday(time, NULL);
    }

    time_t time_val = (time_t)time->tv_sec;
    int ms = time->tv_usec / 1000;
    
    localtime_tz(time_val, &time_info);
    os_snprintf(timestr, timestr_len, "%04d%02d%02d%02d%02d%02d%03d", 
               time_info.tm_year, time_info.tm_mon, time_info.tm_mday,
               time_info.tm_hour, time_info.tm_min, time_info.tm_sec, ms);
    return 0;
}

/* 
 * 获取JPG文件路径，不按天存储 
 * img_dir: 录像目录，0:/IMG
 * file_path: 文件名，格式: 0:/IMG/20250918010101000.JPG
 * filepath_size: 文件名大小
 */
uint8_t takephoto_name(const char *img_dir, char *file_path, int filepath_size)
{
    char timestr[FILE_NAME_LEN + 1];
    if(get_rtc_time_str(timestr, FILE_NAME_LEN + 1, NULL))
        return 1;

    os_snprintf(file_path, filepath_size, "%s/%.*s.JPG", img_dir, FILE_NAME_LEN, timestr);
    return 0;
}

/* 
 * 获取JPG文件路径，按天存储
 * img_dir: 录像目录，0:/IMG
 * file_path: 文件名，格式: 0:/IMG/20250918/20250918010101000.JPG
 * filepath_size: 文件名大小
 */
uint8_t takephoto_name_day(const char *img_dir, char *file_path, int filepath_size)
{
    char timestr[FILE_NAME_LEN + 1];
    if(get_rtc_time_str(timestr, FILE_NAME_LEN + 1, NULL))
        return 1;

    os_snprintf(file_path, filepath_size, "%s/%.*s/%.*s.JPG", img_dir, FILE_SUB_PATH_LEN, timestr, FILE_NAME_LEN, timestr);
    return 0;
}

/* 
 * 获取JPG文件名称
 * filename: 文件名，格式: 20250918010101000.JPG
 * filename_size: 文件名大小
 */
int32_t takephoto_name_no_dir(char *filename, int filename_size)
{
    char timestr[FILE_NAME_LEN + 1];
    if(get_rtc_time_str(timestr, FILE_NAME_LEN + 1, NULL))
        return 1;

    os_snprintf(filename, filename_size, "%.*s.JPG", FILE_NAME_LEN, timestr);
    return 0;
}

/* 
 * 获取JPG文件名称
 * filename: 文件名，格式: 20250918010101000.JPG
 * filename_size: 文件名大小
 * time: 时间
 */
int32_t takephoto_name_no_dir_time(char *filename, int filename_size, struct timeval *time)
{
    char timestr[FILE_NAME_LEN + 1];
    if(get_rtc_time_str(timestr, FILE_NAME_LEN + 1, time))
        return 1;
    os_snprintf(filename, filename_size, "%.*s.JPG", FILE_NAME_LEN, timestr);
    return 0;
}

/* 
 * 为JPG文件添加目录
 * path: 文件路径，格式: dir_name/filename
 * path_size: 文件路径大小
 */
int32_t takephoto_name_add_dir(char *filename, int filename_size, char *path, const char *dir_name)
{
    if(dir_name)
    {
        os_snprintf(filename, filename_size,"%s/%s", dir_name, path);
    }
    else
    {
        os_snprintf(filename, filename_size,"%s", path);
    }
	return 0;
}

/* 
 * 获取MP4文件名 
 * rec_dir: 录像目录，0:/REC
 * sub_path: 子目录，格式: 0:/REC/20250918
 * file_name: 文件名，格式: 20250918010101000.MP4
 */
uint8_t get_mp4_file_name(const char *rec_dir, char *sub_path, char *file_name)
{
    char timestr[FILE_NAME_LEN + 1];
    if(get_rtc_time_str(timestr, FILE_NAME_LEN + 1, NULL))
        return 1;
    
    os_sprintf(sub_path, "%s/%.*s", rec_dir, FILE_SUB_PATH_LEN, timestr);
    os_sprintf(file_name, "%.*s.MP4", FILE_NAME_LEN, timestr);

    return 0;
}

/* 
 * 获取文件名
 * rec_dir: 录像目录，0:/RECA
 * sub_path: 子目录，格式: 0:/RECA/20250918
 * file_name: 文件名，格式: 20250918010101000.MP4
 * extension_name: 文件扩展名，格式: .MP4/.AVI/...
 */
uint8_t get_extension_file_name(const char *rec_dir, char *sub_path, char *file_name, const char *extension_name)
{
    char timestr[FILE_NAME_LEN + 1];
    if(get_rtc_time_str(timestr, FILE_NAME_LEN + 1, NULL))
        return 1;
    
    os_sprintf(sub_path, "%s/%.*s", rec_dir, FILE_SUB_PATH_LEN, timestr);
    os_sprintf(file_name, "%.*s%s", FILE_NAME_LEN, timestr, extension_name);

    return 0;
}

/* 
 * 获取文件名
 * rec_dir: 录像目录，0:/RECA
 * sub_path: 子目录，格式: 0:/RECA/20250918
 * file_name: 文件名，格式: 20250918010101000.MP4
 * extension_name: 文件扩展名，格式: .MP4/.AVI/...
 * time: 时间
 */
uint8_t get_extension_file_name_time(const char *rec_dir, char *sub_path, char *file_name, const char *extension_name, struct timeval *time)
{
    char timestr[FILE_NAME_LEN + 1];
    if(get_rtc_time_str(timestr, FILE_NAME_LEN + 1, time))
        return 1;
    
    os_sprintf(sub_path, "%s/%.*s", rec_dir, FILE_SUB_PATH_LEN, timestr);
    os_sprintf(file_name, "%.*s%s", FILE_NAME_LEN, timestr, extension_name);

    return 0;
}

/* 
 * 根据文件类型获取照片文件路径
 * filename: 文件名
 * path: 文件路径
 * pathsize: 文件路径大小
 */
void gen_photo_path(const char *filename, char *path, uint32_t pathsize)
{
    const char *rec_path = NULL;

    const char* file_dot = strrchr(filename, '.');
    if(file_dot == NULL) {
        os_printf("%s file_dot is NULL\n", __FUNCTION__);
        return;
    }

    if(os_strstr(filename, FRONT_SUFFIX)) {
        rec_path = IMGA_PATH;
    } else if(os_strstr(filename, INTER_SUFFIX)) {
        rec_path = IMGB_PATH;
    } else if(os_strstr(filename, BACK_SUFFIX)) {
        rec_path = IMGC_PATH;
    } else {
        rec_path = IMG_PATH;
    }

    os_snprintf(path, pathsize, "%s/%.*s%s", rec_path, FILE_NAME_LEN, filename, file_dot);

    os_printf("%s path: %s\n", __FUNCTION__, path);
}

/* 
 * 根据文件类型获取视频文件路径
 * filename: 文件名
 * path: 文件路径
 * pathsize: 文件路径大小
 */
void gen_video_path(const char *filename, char *path, uint32_t pathsize)
{
    uint8_t prefix_len = 0;
    const char *rec_path = NULL;

    const char *file_name = os_strrchr(filename, '/');
    if(file_name == NULL) {
        file_name = filename;
    }

    const char* file_dot = strrchr(filename, '.');
    if(file_dot == NULL) {
        os_printf("%s file_dot is NULL\n", __FUNCTION__);
        return;
    }

    if(os_strstr(file_name, SOS_PREFIX)) {
        prefix_len = strlen(SOS_PREFIX);
    } else if(os_strstr(file_name, PARKF_PREFIX)) {
        prefix_len = strlen(PARKF_PREFIX);
    }

    if(os_strstr(file_name, FRONT_SUFFIX)) {
        rec_path = RECA_PATH;
    } else if(os_strstr(file_name, INTER_SUFFIX)) {
        rec_path = RECB_PATH;
    } else if(os_strstr(file_name, BACK_SUFFIX)) {
        rec_path = RECC_PATH;
    } else {
        rec_path = REC_PATH;
    }

    os_snprintf(path, pathsize, "%s/%.*s/%.*s%s", rec_path, FILE_SUB_PATH_LEN, file_name + prefix_len, FILE_NAME_LEN + prefix_len, file_name, file_dot);

    os_printf("%s path: %s\n", __FUNCTION__, path);
}

void gen_photo_video_path(const char *filename, char *path, uint32_t pathsize)
{
    if(os_strstr(filename, JPG_EXTENSION_NAME)) 
    {
        gen_photo_path(filename, path, pathsize);
    }
    else if(os_strstr(filename, MP4_EXTENSION_NAME) || os_strstr(filename, AVI_EXTENSION_NAME))
    {
        gen_video_path(filename, path, pathsize);
    }
    else
    {
        _os_printf("filename %s format err!\n", filename);
    }
}


/***************************************************************
 *                   获取文件列表接口                           *
 *                                                             *
****************************************************************/

typedef void (* conver_send_fn)(int fd, struct file_info *file_info);

void send_chunk(int fd, const char* data, uint16_t data_len) 
{
    char chunk_buffer[256]; // 或者使用动态分配
	int chunk_len = os_snprintf(chunk_buffer, sizeof(chunk_buffer), "%x\r\n%s\r\n", data_len, data);
	send(fd, chunk_buffer, chunk_len, 0);
}

void conver_send(int fd, struct file_info *file_info)
{
    char send_buffer[256];
    char timestr[15];

    os_snprintf(timestr, sizeof(timestr), "%04d%02d%02d%02d%02d%02d",
                ((file_info->file_date & 0xFE00)>>9)+1980,
                (file_info->file_date & 0x1E0)>>5,
                (file_info->file_date & 0x1F),
                (file_info->file_time & 0xF800)>>11,
                (file_info->file_time & 0x7E0)>>5,
                (file_info->file_time & 0x1F)*2);

    os_snprintf(send_buffer, sizeof(send_buffer), "%s{\"name\":\"%s\",\"size\":%d,\"createtimestr\":\"%s\"}",
               (file_info->file_count++ > 0) ? "," : "",
                file_info->file_name,
                file_info->file_size / 1024,
                timestr);

    // 发送数据
    send_chunk(fd, send_buffer, strlen(send_buffer));
}

int get_fileinfo_send(int fd, const char *search_dir, const char *ext_name, conver_send_fn cs_fn)
{
    struct file_info file_info;
    FILINFO *fil = NULL;

    void *dir = osal_opendir(search_dir);
    if (dir)
    {
        do
        {
            fil = osal_readdir(dir);
            if(!fil) break;
            if(fil->fname[0] == 0) break;
            if (fil->fname[0] == '.') continue;

            if(!osal_dirent_isdir(fil))
            {
                char    *filename     = osal_dirent_name(fil);
                uint32_t filesize     = osal_dirent_size(fil);
                uint32_t filedate     = osal_dirent_date(fil);
                uint32_t filetime     = osal_dirent_time(fil);
                uint8_t  filename_len = os_strlen(filename);

                // 检查后缀名是否匹配
                if(ext_name)
                {
                    uint8_t ext_filename[16];
                    uint8_t extname_len = os_strlen(ext_name);
                    // 进行全部转换成大写
                    if (filename_len - extname_len > 0)
                    {
                        for (uint8_t i = 0; i < os_strlen(ext_name); i++)
                        {
                            ext_filename[i] = toupper(filename[filename_len - extname_len + i]);
                        }
                        // 后缀名匹配
                        if (memcmp(ext_name, ext_filename, extname_len) != 0)
                        {
                            continue;
                        }
                    }
                }
                file_info.file_name = filename;
                file_info.file_size = filesize;
                file_info.file_date = filedate;
                file_info.file_time = filetime;
                file_info.file_count++;
                cs_fn(fd, &file_info);
            }
        } while (fil);
        osal_closedir(dir);
    }
    return file_info.file_count;
}

// 发送demo，采用分块发送的方式 
void send_stream_filelist(int fd)
{
    int     video_count = 0;
    int     photo_count = 0;
    char    send_buffer[256];

    // 创建HTTP响应头
    snprintf(send_buffer, sizeof(send_buffer), 
        "HTTP/1.0 200 OK\r\n"
        "Connection: close\r\n"
        "Content-Type: application/json\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n");
    send(fd, send_buffer, strlen(send_buffer), 0);
    
    // 获取视频
    os_snprintf(send_buffer, sizeof(send_buffer), "{\"result\":0,\"info\":[{\"folder\":\"%s\",\"files\":[", LOOP_PREFIX);
    send_chunk(fd, send_buffer, strlen(send_buffer));

    void *dir = osal_opendir(REC_PATH);
    FILINFO *fil = NULL;
    char path[64];
    if(dir)
    {
        do
        {
            fil = osal_readdir(dir);
            if(!fil) break;
            if(fil->fname[0] == 0) break;
            if (fil->fname[0] == '.') continue;
            if(osal_dirent_isdir(fil))
            {
                os_snprintf(path, sizeof(path), "%s/%s", REC_PATH, fil->fname);
                video_count += get_fileinfo_send(fd, path, MP4_EXTENSION_NAME, conver_send);
                send_chunk(fd, ",", strlen(","));
            }
        } while(fil);
    }

    // 获取照片
    os_snprintf(send_buffer, sizeof(send_buffer), "],\"count\":%d},{\"folder\":\"%s\",\"files\":[", video_count, EVENT_PREFIX);
    send_chunk(fd, send_buffer, strlen(send_buffer));
    photo_count = get_fileinfo_send(fd, IMG_PATH, JPG_EXTENSION_NAME, conver_send);

    os_snprintf(send_buffer, sizeof(send_buffer), "],\"count\":%d}]}", photo_count);
    send_chunk(fd, send_buffer, strlen(send_buffer));
    
    // 发送分块结束标记
    send_chunk(fd, "", 0);  // 0长度块表示结束
}
