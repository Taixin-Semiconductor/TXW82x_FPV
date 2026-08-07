#include "file_common_api.h"
#include "lib/fs/fatfs/osal_file.h"
#include "app/video_app/file_thumb.h"
#include "osal/string.h"

static uint8_t get_rtc_time_str(char *timestr)
{
    struct tm *time_info;
    struct timeval ptimeval;
    gettimeofday(&ptimeval, NULL);

    time_t time_val = (time_t)ptimeval.tv_sec;
    int ms = ptimeval.tv_usec / 1000;
    
    time_info = gmtime(&time_val);
    if (time_info == NULL) {
        _os_printf("gmtime error\r\n");
        return 1;
    }
    os_sprintf(timestr, "%04d%02d%02d%02d%02d%02d%03d", 
               time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday,
               time_info->tm_hour, time_info->tm_min, time_info->tm_sec, ms);
    return 0;
}

/* 
 * 获取JPG文件路径，不按天存储 
 * img_dir: 录像目录，0:/IMG
 * file_path: 文件名，格式: 0:/IMG/20250918010101000.jpg
 * filepath_size: 文件名大小
 */
uint8_t takephoto_name(const char *img_dir, char *file_path, int filepath_size)
{
    char timestr[18];
    if(get_rtc_time_str(timestr))
        return 1;

    os_snprintf(file_path, filepath_size, "%s/%.17s.jpg", img_dir, timestr);
    return 0;
}

/* 
 * 获取JPG文件路径，按天存储
 * img_dir: 录像目录，0:/IMG
 * file_path: 文件名，格式: 0:/IMG/20250918/20250918010101000.jpg
 * filepath_size: 文件名大小
 */
uint8_t takephoto_name_day(const char *img_dir, char *file_path, int filepath_size)
{
    char timestr[18];
    if(get_rtc_time_str(timestr))
        return 1;

    os_snprintf(file_path, filepath_size, "%s/%.8s/%.17s.jpg", img_dir, timestr, timestr);
    return 0;
}

/* 
 * 获取JPG文件名称
 * filename: 文件名，格式: 20250918010101000.jpg
 * filename_size: 文件名大小
 */
int32_t takephoto_name_no_dir(char *filename, int filename_size)
{
    char        timestr[18];
    if(get_rtc_time_str(timestr))
        return 1;

    os_snprintf(filename, filename_size, "%.17s.jpg", timestr);
    return 0;
}

/* 
 * 为JPG文件添加目录
 * path: 文件路径，格式: dir_name/filename
 * path_size: 文件路径大小
 */
int32_t takephoto_name_add_dir(char *filename, int filename_size,char *path, const char *dir_name)
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
 * file_name: 文件名，格式: 20250918010101000.mp4
 */
uint8_t get_mp4_file_name(const char *rec_dir, char *sub_path, char *file_name)
{
    char        timestr[18];
    if(get_rtc_time_str(timestr))
        return 1;
    
    os_sprintf(sub_path, "%s/%.8s", rec_dir, timestr);
    os_sprintf(file_name, "%.17s.mp4", timestr);

    return 0;
}

/* 
 * 根据文件类型获取文件路径
 * filename: 文件名
 * path: 文件路径
 * pathsize: 文件路径大小
 * type: 1:JPG 2:MP4
 */
uint8_t gen_file_path(const char *filename, char *path, uint32_t pathsize, uint8_t type)
{
    char *dirname = (type == 1) ? IMG_PATH : ((type == 2) ? REC_PATH : (type == 3) ? EMR_PATH : PARK_PATH);
    if(type == 1 || type == 3)
    {
        os_snprintf((char *) path, pathsize, "%s/%s", dirname, filename);
    }
    else if(type == 2 || type == 4)
    {
        char datefile[9];
        os_sprintf(datefile, "%.8s", filename);
        os_snprintf((char *) path, pathsize, "%s/%s/%s", dirname, datefile, filename);
    }
    os_printf("%s path:%s\n", __FUNCTION__, path);
    return 0;
}
