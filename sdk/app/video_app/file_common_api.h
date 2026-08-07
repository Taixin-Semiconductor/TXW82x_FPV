#ifndef _FILE_COMMON_API_H_
#define _FILE_COMMON_API_H_

#include <stdint.h>

uint8_t takephoto_name(const char *img_dir, char *file_path, int filepath_size);
uint8_t takephoto_name_day(const char *img_dir, char *file_path, int filepath_size);
int32_t takephoto_name_no_dir(char *filename, int filename_size);
int32_t takephoto_name_add_dir(char *filename, int filename_size,char *path, const char *dir_name);
uint8_t get_mp4_file_name(const char *rec_dir, char *sub_path, char *file_name);
uint8_t gen_file_path(const char *filename, char *path, uint32_t pathsize, uint8_t type);

#endif
