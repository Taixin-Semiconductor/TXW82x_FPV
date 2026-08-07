#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "stream_define.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "dev/jpg/hgjpg.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
/***********************************************************
 * 可以存放mjpg的公用函数组件
 **********************************************************/

#define HARDWARE_JPG_NUM 2

// data申请空间函数
#define STREAM_MALLOC av_psram_malloc
#define STREAM_FREE av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE av_free
#define STREAM_LIBC_ZALLOC av_zalloc



/*****************************************************************
 * 增加mjpg模块锁,主要用于复用的时候需要对mjeg模块锁
 ****************************************************************/
typedef struct 
{
    os_event_t      jpg_event;
    uint8_t         init;
    uint8_t         lock_value;
    uint8_t         last_lock_value;
}jpg_mutex;

enum
{
    JPG_LOCK = BIT(0),
};

static jpg_mutex *jpg_mutex_table[HARDWARE_JPG_NUM];

int32 jpg_mutex_init()
{
    int32_t ret = 0;
    int32_t res = 0;
    jpg_mutex *mutex = (jpg_mutex*)STREAM_LIBC_ZALLOC(sizeof(jpg_mutex)*2);
    jpg_mutex_table[0] = &mutex[0];
    jpg_mutex_table[1] = &mutex[1];

    res = os_event_init(&jpg_mutex_table[0]->jpg_event);
    ret |= res;
    if (!res)
    {
        jpg_mutex_table[0]->init = 1;
        os_event_set(&jpg_mutex_table[0]->jpg_event, JPG_LOCK, NULL);
    }
    
    res = os_event_init(&jpg_mutex_table[1]->jpg_event);
    ret |= res;
    if (!res)
    {
        jpg_mutex_table[1]->init = 1;
        os_event_set(&jpg_mutex_table[1]->jpg_event, JPG_LOCK, NULL);
    }
    os_printf("%s:%d\tret:%d\n",__FUNCTION__,__LINE__,ret);
    return ret;
}

/************************************************************************
 * jpgid: 硬件jpg模块ID,0:jpg0  1:jpg1
 * value: 设置lock的值,只有一致才能释放
 * last_value: 如果不为NULL,则返回上一次的锁值
 ***********************************************************************/
int32_t jpg_mutex_lock(uint32_t jpgid,uint8_t value,uint8_t *last_value)
{
    ASSERT(jpgid<HARDWARE_JPG_NUM);
    jpg_mutex *mutex = jpg_mutex_table[jpgid];
    ASSERT(mutex && mutex->init==1);
    uint32_t rflags;
    if (mutex->lock_value || value == 0)
    {
        if(last_value)
        {
            *last_value = mutex->last_lock_value;
        }
        return 1;
    }
    // 如果获取到锁,就将lock_value设置value
    int32_t ret = os_event_wait(&mutex->jpg_event, JPG_LOCK, &rflags, OS_EVENT_WMODE_CLEAR, 0);
    if (ret == 0)
    {
        if(last_value)
        {
            *last_value = mutex->last_lock_value;
        }
        mutex->lock_value = value;
    }
    return ret;
}


/********************************************************
 * jpgid: 硬件jpg模块ID,0:jpg0  1:jpg1
 * value: 设置lock的值,只有一致才能释放
 ******************************************************/
int32_t jpg_mutex_unlock(uint32_t jpgid,int32_t value)
{
    ASSERT(jpgid<HARDWARE_JPG_NUM);
    jpg_mutex *mutex = jpg_mutex_table[jpgid];
    ASSERT(mutex && mutex->init==1);
    int32_t ret = 1;
    // 只有相同的值才支持解锁
    if (mutex->lock_value == value)
    {
        ret = os_event_set(&mutex->jpg_event, JPG_LOCK, NULL);
        if (ret == 0)
        {
            mutex->last_lock_value = mutex->lock_value;
            mutex->lock_value      = 0;
        }
    }
    return ret;
}

int32_t jpg_mutex_unlock_check(uint32_t jpgid,int32_t value)
{
    ASSERT(jpgid<HARDWARE_JPG_NUM);
    jpg_mutex *mutex = jpg_mutex_table[jpgid];
    ASSERT(mutex && mutex->init==1);
    int32_t ret = 1;
    // 只有相同的值才支持解锁
    if (mutex->lock_value == value)
    {
        ret = os_event_set(&mutex->jpg_event, JPG_LOCK, NULL);
        if (ret == 0)
        {
            mutex->lock_value      = 0;
        }
    }
    return ret;
}

