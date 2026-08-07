#ifndef __FILE_THUMB_H
#define __FILE_THUMB_H
#include "sys_config.h"
#include "typesdef.h"

#define MAIN_PATH          ""
#define THUMB_PATH         "0:/thumb"
#define NORMAL_PATH        "DCIM"
#define IMG_PATH           "0:/IMG"
#define REC_PATH           "0:/REC"
#define EMR_PATH           "0:/EMR"
#define PARK_PATH          "0:/PARK"
#define JPG_EXTENSION_NAME ".JPG" // 根据文件系统是否区分大小写,可以在代码上进行不区分大小写,这里用大写,代码将获取的后缀名进行转换成大写
#define MP4_EXTENSION_NAME ".MP4"
#define LOOP_PREFIX        "LOOP"
#define EVENT_PREFIX       "EVENT"
#define EMR_PREFIX         "EMR"
#define PARK_PREFIX        "PARK"


uint8_t gen_thumb_path(const char *filename, char *path, uint32_t pathsize);
void *jpg_file_read(const char *filename,uint8_t thumb,int32_t *filesize);
void jpg_file_free(void *buf);
uint32_t calc_hash(const char *filename);
char *photo_json_path();
void path_json_free(void *str);
uint8_t file_delete(const char *filename, uint8_t type);

#endif