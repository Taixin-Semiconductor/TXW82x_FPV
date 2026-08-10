#include "basic_include.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

/*****************************************************************
 * 增加scale模块锁,主要用于复用的时候需要对scale模块锁
 ****************************************************************/
typedef struct
{
    os_event_t event;
    uint8_t    init;
    uint8_t    lock_value;
    uint8_t    last_lock_value;
} scale_mutex;

enum
{
    SCALE_LOCK = BIT(0),
};

static scale_mutex g_scale_mutext;

// scale的锁初始化
int32 scale_mutex_init()
{
    if(!g_scale_mutext.init)
    {
        os_event_init(&g_scale_mutext.event);
        os_event_set(&g_scale_mutext.event, SCALE_LOCK, NULL);
        g_scale_mutext.init = 1;
    }
    return 0;
}

int32_t scale_mutex_lock(uint32_t scale_id, uint8_t value, uint8_t *last_value)
{
    scale_mutex *mutex = &g_scale_mutext;
    ASSERT(mutex && mutex->init == 1);
    uint32_t rflags;
    if (mutex->lock_value || value == 0)
    {
        if (last_value)
        {
            *last_value = mutex->last_lock_value;
        }
        return 1;
    }
    // 如果获取到锁,就将lock_value设置value
    int32_t ret = os_event_wait(&mutex->event, SCALE_LOCK, &rflags, OS_EVENT_WMODE_CLEAR, 0);
    if (ret == 0)
    {
        if (last_value)
        {
            *last_value = mutex->last_lock_value;
        }
        mutex->lock_value = value;
    }
    return ret;
}

int32_t scale_mutex_unlock(uint32_t scale_id, int32_t value)
{
    scale_mutex *mutex = &g_scale_mutext;
    ASSERT(mutex && mutex->init == 1);
    int32_t ret = 1;
    // 只有相同的值才支持解锁
    if (mutex->lock_value == value)
    {
        ret = os_event_set(&mutex->event, SCALE_LOCK, NULL);
        if (ret == 0)
        {
            mutex->last_lock_value = mutex->lock_value;
            mutex->lock_value      = 0;
        }
    }
    return ret;
}

int32_t scale_mutex_unlock_check(uint32_t scale_id, int32_t value)
{
    scale_mutex *mutex = &g_scale_mutext;
    ASSERT(mutex && mutex->init == 1);
    int32_t ret = 1;
    // 只有相同的值才支持解锁
    if (mutex->lock_value == value)
    {
        ret = os_event_set(&mutex->event, SCALE_LOCK, NULL);
        if (ret == 0)
        {
            mutex->lock_value = 0;
        }
    }
    return ret;
}