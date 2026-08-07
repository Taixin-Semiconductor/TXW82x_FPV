#ifndef __JPG_COMMON_H
#define __JPG_COMMON_H
#include "lib/video/dvp/jpeg/jpg.h"
int32 jpg_mutex_init();
int32_t jpg_mutex_lock(uint32_t jpgid,uint8_t value,uint8_t *last_value);
int32_t jpg_mutex_unlock(uint32_t jpgid,int32_t value);
int32_t jpg_mutex_unlock_check(uint32_t jpgid,int32_t value);
#endif
