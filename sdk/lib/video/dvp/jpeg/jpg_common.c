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
#define STREAM_FREE   av_psram_free
#define STREAM_ZALLOC av_psram_zalloc

// 结构体申请空间函数
#define STREAM_LIBC_MALLOC av_malloc
#define STREAM_LIBC_FREE   av_free
#define STREAM_LIBC_ZALLOC av_zalloc

/*****************************************************************
 * 增加mjpg模块锁,主要用于复用的时候需要对mjeg模块锁
 ****************************************************************/
typedef struct
{
    os_event_t jpg_event;
    uint8_t    init;
    uint8_t    lock_value;
    uint8_t    last_lock_value;
} jpg_mutex;

enum
{
    JPG_LOCK = BIT(0),
};

static jpg_mutex *jpg_mutex_table[HARDWARE_JPG_NUM];

int32 jpg_mutex_init()
{
    int32_t    ret     = 0;
    int32_t    res     = 0;
    jpg_mutex *mutex   = (jpg_mutex *) STREAM_LIBC_ZALLOC(sizeof(jpg_mutex) * 2);
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
    os_printf("%s:%d\tret:%d\n", __FUNCTION__, __LINE__, ret);
    return ret;
}

/************************************************************************
 * jpgid: 硬件jpg模块ID,0:jpg0  1:jpg1
 * value: 设置lock的值,只有一致才能释放
 * last_value: 如果不为NULL,则返回上一次的锁值
 ***********************************************************************/
int32_t jpg_mutex_lock(uint32_t jpgid, uint8_t value, uint8_t *last_value)
{
    ASSERT(jpgid < HARDWARE_JPG_NUM);
    jpg_mutex *mutex = jpg_mutex_table[jpgid];
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
    int32_t ret = os_event_wait(&mutex->jpg_event, JPG_LOCK, &rflags, OS_EVENT_WMODE_CLEAR, 0);
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

/********************************************************
 * jpgid: 硬件jpg模块ID,0:jpg0  1:jpg1
 * value: 设置lock的值,只有一致才能释放
 ******************************************************/
int32_t jpg_mutex_unlock(uint32_t jpgid, int32_t value)
{
    ASSERT(jpgid < HARDWARE_JPG_NUM);
    jpg_mutex *mutex = jpg_mutex_table[jpgid];
    ASSERT(mutex && mutex->init == 1);
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

int32_t jpg_mutex_unlock_check(uint32_t jpgid, int32_t value)
{
    ASSERT(jpgid < HARDWARE_JPG_NUM);
    jpg_mutex *mutex = jpg_mutex_table[jpgid];
    ASSERT(mutex && mutex->init == 1);
    int32_t ret = 1;
    // 只有相同的值才支持解锁
    if (mutex->lock_value == value)
    {
        ret = os_event_set(&mutex->jpg_event, JPG_LOCK, NULL);
        if (ret == 0)
        {
            mutex->lock_value = 0;
        }
    }
    return ret;
}

struct jpg_mem_list_s;
#define JPG_MEM_LIST_COUNT (4)

struct jpg_mem_chunk
{
    struct jpg_mem_chunk  *next;
    uint8_t                del : 1, jpg_mem_map : 5, rev : 2;
    uint8_t                count;
    uint16_t               len;
    struct jpg_mem_list_s *jpg_mem_list;
    uint32_t               t_bitmap; // 当前chunk的内存位图
    uint32_t               bitmap;   // 内存位图
    struct jpg_mem        *mem[32];
};

// 内存池列表,最多N个内存池
struct jpg_mem_list_s
{
    uint32_t              bitmap;      // 1代表有空间,0代表没有空间
    uint32_t              total_count; // 记录当前内存池最大的空间数量,动态增加以及动态释放
    // 剩余总共内存块数量,如果有,就判断bitmap,从最低位开始寻找,释放空间优先释放高位的内存块,低位内存块尽量在完全没用的时候才释放(防止频繁申请),可以通过一个超时机制去释放(影响立刻申请空间的模块,也可以实现一个立刻释放的命令)
    uint32_t              remain_block_count;
    struct jpg_mem_chunk *last_chunk; // 记录上一次申请空间的chunk,如果释放,还需要检查是否是最低位的,尽量从最低位的chunk申请(利于释放空间)
    struct jpg_mem_chunk *chunk[JPG_MEM_LIST_COUNT];
};

// 32byte对齐,所以前面留32byte作为其他数据保存作用
struct jpg_mem
{
    struct jpg_mem_chunk *chunk; // chunk的地址,相当于申请空间的chunk
    struct jpg_mem       *next;
    uint32_t              bitmap; // 固定只有一个bit被置位(代表内存块的位置)
    uint32_t              rev[5];
};

static struct jpg_mem_list_s jpg_mem_list;

// mjpg的内存管理,暂时不支持动态扩展,相当于预先分配
// 正常应该是可以后面动态添加以及删除
struct jpg_mem_chunk *jpg_mem_manage_init(uint8_t chunk_block)
{
    if (chunk_block > 32)
    {
        chunk_block = 32;
    }
    if (!chunk_block)
    {
        return NULL;
    }
    uint8_t               ret          = 1;
    // 先去申请空间,清除cache问题,然后再挂接到链表中
    uint32_t              malloc_count = 0;
    struct jpg_mem_chunk *chunk        = NULL;

    chunk = (struct jpg_mem_chunk *) STREAM_ZALLOC(sizeof(struct jpg_mem_chunk));
    if (!chunk)
    {
        goto jpg_mem_manage_init_end;
    }
    chunk->len = 16 * 1024;
    for (malloc_count = 0; malloc_count < chunk_block; malloc_count++)
    {
        chunk->mem[malloc_count] = (struct jpg_mem *) STREAM_MALLOC(16 * 1024 + sizeof(struct jpg_mem));
        if (chunk->mem[malloc_count])
        {
            sys_dcache_invalid_range((uint32_t *) chunk->mem[malloc_count], 16 * 1024 + sizeof(struct jpg_mem));
            chunk->t_bitmap |= BIT(malloc_count);
            chunk->mem[malloc_count]->bitmap = BIT(malloc_count);
            chunk->mem[malloc_count]->chunk  = chunk;
        }
    }

    chunk->bitmap = chunk->t_bitmap;
    chunk->count  = malloc_count + 1;
    // 没有申请到空间,返回错误
    if (!chunk->bitmap)
    {
        goto jpg_mem_manage_init_end;
    }
    ret = 0;
jpg_mem_manage_init_end:
    if (ret)
    {
        if (chunk)
        {
            STREAM_FREE(chunk);
            chunk = NULL;
        }
    }
    return chunk;
}

// 释放一个chunk
static void free_jpg_chunk(struct jpg_mem_chunk *chunk)
{
    if (chunk)
    {
        // 不一致,不能释放,报错
        ASSERT(chunk->t_bitmap == chunk->bitmap);
        for (int i = 0; i < chunk->count; i++)
        {
            ASSERT(chunk->mem[i]);
            STREAM_FREE(chunk->mem[i]);
            chunk->mem[i] = NULL;
        }
        STREAM_FREE(chunk);
    }
}

// 增加一个内存块到列表
uint8_t add_jpg_block(uint8_t chunk_block)
{
    uint8_t               ret   = 1;
    uint8_t               map   = 0;
    struct jpg_mem_chunk *chunk = jpg_mem_manage_init(chunk_block);
    if (chunk)
    {
        // 检查最低位是否有0,代表空闲
        if (jpg_mem_list.bitmap)
        {
            map = __builtin_ctz(~jpg_mem_list.bitmap);
        }
        os_printf("jpg_block map:%d\n", map);
        // list没有满,则可以添加
        if (map < JPG_MEM_LIST_COUNT)
        {
            uint32_t flags = disable_irq();

            jpg_mem_list.chunk[map] = chunk;
            jpg_mem_list.bitmap |= BIT(map);
            jpg_mem_list.total_count = chunk->count;
            jpg_mem_list.remain_block_count += chunk->count;
            chunk->jpg_mem_list = &jpg_mem_list;
            chunk->jpg_mem_map  = map;
            enable_irq(flags);
            ret = 0;
        }
        // 释放chunk
        else
        {
            free_jpg_chunk(chunk);
        }
    }
    return ret;
}

// 获取一个有内存块的chunk
static struct jpg_mem_chunk *get_free_chunk()
{
    struct jpg_mem_chunk *chunk;
    uint8_t               map;
    // os_printf("jpg_mem_list.last_chunk:%X\n", jpg_mem_list.last_chunk);
    if (jpg_mem_list.last_chunk)
    {
        chunk = jpg_mem_list.last_chunk;
    }
    else
    {
        if (jpg_mem_list.bitmap)
        {
            map   = __builtin_ctz(jpg_mem_list.bitmap);
            chunk = jpg_mem_list.chunk[map];
        }
        else
        {
            chunk = NULL;
        }
    }
    return chunk;
}

void *get_jpg_node(uint16_t *len)
{
    struct jpg_mem       *buf        = NULL;
    uint32_t              flags      = disable_irq();
    struct jpg_mem_chunk *next_chunk = get_free_chunk();
    uint8_t               map;
    uint8_t               change = 1;

    if (next_chunk && next_chunk->bitmap)
    {
        map       = __builtin_ctz(next_chunk->bitmap);
        buf       = next_chunk->mem[map];
        buf->next = NULL;
        buf += 1;
        next_chunk->bitmap &= (~BIT(map));
        // os_printf("next_chunk->bitmap:%X\n", next_chunk->bitmap);
        if (!next_chunk->bitmap)
        {
            next_chunk->jpg_mem_list->bitmap &= (~BIT(next_chunk->jpg_mem_map));
            next_chunk->jpg_mem_list->last_chunk = NULL;
        }
        else
        {
            if (next_chunk->jpg_mem_list->last_chunk)
            {
                if (next_chunk->jpg_mem_list->last_chunk->jpg_mem_map < next_chunk->jpg_mem_map)
                {
                    change = 0;
                }
            }
            if (change)
            {
                next_chunk->jpg_mem_list->last_chunk = next_chunk;
            }
        }
        if (len)
        {
            *len = next_chunk->len;
        }
    }
    else
    {
        if (len)
        {
            *len = 0;
        }
    }
    enable_irq(flags);
    if (buf)
    {
        // os_printf("malloc map:%X\tbuf:%X\taddr:%X\n",BIT(map),buf,RETURN_ADDR());
    }
    else
    {
        os_printf("%s:%d err\n", __FUNCTION__, __LINE__);
    }
    return (void *) buf;
}

uint16_t get_node_len(void *buf)
{
    struct jpg_mem *node = (struct jpg_mem *) buf;
    if (node)
    {
        node -= 1;
        return node->chunk->len;
    }
    else
    {
        return 0;
    }
}

// 获取node是否有足够的保留空间来填充额外的数据
void *get_node_rev(void *buf, uint32_t rev_len)
{
    struct jpg_mem *node = (struct jpg_mem *) buf;
    if (node)
    {
        node -= 1;
        if (sizeof(node->rev) > rev_len)
        {
            return node->rev;
        }
    }
    return NULL;
}

void free_jpg_node(void *buf)
{
    // os_printf("buf:%X\n",buf);
    struct jpg_mem       *free_buf = (struct jpg_mem *) buf;
    struct jpg_mem_chunk *chunk;
    free_buf -= 1;
    chunk           = free_buf->chunk;
    uint8_t  change = 1;
    // 正常应该要检查是否符合,这里默认传入的地址是正确的
    uint32_t flags  = disable_irq();
    // uint32_t map   = free_buf->bitmap;
    chunk->bitmap |= free_buf->bitmap;
    chunk->jpg_mem_list->bitmap |= BIT(chunk->jpg_mem_map);
    // os_printf("chunk->bitmap:%X\n", chunk->bitmap);
    if (chunk->jpg_mem_list->last_chunk)
    {
        if (chunk->jpg_mem_list->last_chunk->jpg_mem_map < chunk->jpg_mem_map)
        {
            change = 0;
        }
    }
    if (change)
    {
        chunk->jpg_mem_list->last_chunk = chunk;
    }
    enable_irq(flags);
    // os_printf("free map:%X\tbuf:%X\n",map,buf);
    //  正常应该要检查当前chunk是否要被释放,如果需要被释放,是要考虑释放空间
}

// 将一个节点放到另一个节点
void *push_node(void *head, void *next)
{
    struct jpg_mem *h = (struct jpg_mem *) head;
    struct jpg_mem *n = (struct jpg_mem *) next;
    if (head)
    {
        h -= 1;
        n -= 1;
        h->next = n;
    }
    // os_printf("head:%X\tnext:%X\n",head,next);
    return next;
}

// pop一个节点头出去
void *pop_node(void *head)
{
    if (!head)
    {
        return NULL;
    }
    struct jpg_mem *h = (struct jpg_mem *) head;
    h -= 1;
    if (!h->next)
    {
        return NULL;
    }
    return h->next + 1;
}