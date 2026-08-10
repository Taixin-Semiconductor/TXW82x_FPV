#ifndef __SCALE_COMMON_H
#define __SCALE_COMMON_H

// 通过定义宏去设置jpg的lock的值,只要有复用,就需要添加一个,防止与其他冲突
enum SCALE_LOCK_VALUE
{
    SCALE_LOCK_NONE,
    SCALE_DECODE_LOCK,
    SCALE_LOCK_MAX = 0xff,
};

int32   scale_mutex_init();
int32_t scale_mutex_lock(uint32_t scale_id, uint8_t value, uint8_t *last_value);
int32_t scale_mutex_unlock(uint32_t scale_id, int32_t value);
int32_t scale_mutex_unlock_check(uint32_t scale_id, int32_t value);

#endif
