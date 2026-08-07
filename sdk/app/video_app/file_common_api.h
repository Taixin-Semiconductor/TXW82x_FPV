#ifndef _FILE_COMMON_API_H_
#define _FILE_COMMON_API_H_

#include <stdint.h>
#include "typesdef.h"

// 文件名长度
#define FILE_NAME_LEN       17
#define FILE_SUB_PATH_LEN   8

struct file_info
{
    char *file_name;
    uint32_t file_size;
    uint32_t file_date;
    uint32_t file_time;
    uint32_t file_count;
};

uint8_t takephoto_name(const char *img_dir, char *file_path, int filepath_size);
uint8_t takephoto_name_day(const char *img_dir, char *file_path, int filepath_size);
int32_t takephoto_name_no_dir(char *filename, int filename_size);
int32_t takephoto_name_no_dir_time(char *filename, int filename_size, struct timeval *time);
int32_t takephoto_name_add_dir(char *filename, int filename_size,char *path, const char *dir_name);
uint8_t get_mp4_file_name(const char *rec_dir, char *sub_path, char *file_name);
uint8_t get_extension_file_name(const char *rec_dir, char *sub_path, char *file_name, const char *extension_name);
uint8_t get_extension_file_name_time(const char *rec_dir, char *sub_path, char *file_name, const char *extension_name, struct timeval *time);
void gen_photo_path(const char *filename, char *path, uint32_t pathsize);
void gen_video_path(const char *filename, char *path, uint32_t pathsize);
void gen_photo_video_path(const char *filename, char *path, uint32_t pathsize);

#endif
