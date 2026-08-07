#ifndef __MEDIA_H
#define __MEDIA_H
#include "diskio.h"
#include "integer.h"

bool create_jpg_file(char *dir_name);
void *create_video_file(char *dir_name);

char *get_min_video_file(char *dir_name);

void fatfs_create_jpg();
void fatfs_write_data(uint8_t *buf,uint32_t len);
void fatfs_close_jpg();



#endif
