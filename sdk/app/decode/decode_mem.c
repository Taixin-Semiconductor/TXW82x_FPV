#include "basic_include.h"
#include "decode_mem.h"
typedef void *(decode_malloc) (int size);
typedef void(decode_free)(void *mem);
// 检查是否有足够空间的内存,如果没有就申请
// size:需要申请的空间大小
uint8_t *decode_mem_malloc(struct mem_info **mem_info, uint32_t max_mem_num, uint32_t size, decode_malloc c_malloc)
{
    int8_t free_mem_index   = -1;
    int8_t malloc_mem_index = -1;
    // 先去搜索是否有足够空间的内存,如果没有就申请
    for (int i = 0; i < max_mem_num; i++)
    {
        if (!mem_info[i])
        {
            if (free_mem_index == -1)
            {
                free_mem_index = i;
            }
        }
        else
        {
            if (!mem_info[i]->used && mem_info[i]->size >= size)
            {
                if (malloc_mem_index == -1)
                {
                    malloc_mem_index = i;
                }
                // 使用最优的内存块
                else
                {
                    // 内存块的size有区别,使用适配的内存块
                    if (mem_info[i]->size < mem_info[malloc_mem_index]->size)
                    {
                        malloc_mem_index = i;
                    }
                    // 内存块size不一致,则使用最近使用的内存块?
                    else if (mem_info[i]->size == mem_info[malloc_mem_index]->size)
                    {
                        // 时间越大,越接近最近使用
                        if (mem_info[i]->last_time > mem_info[malloc_mem_index]->last_time)
                        {
                            malloc_mem_index = i;
                        }
                    }
                }
            }
        }
    }

    if (malloc_mem_index != -1)
    {
        // 更新时间
        mem_info[malloc_mem_index]->used      = 1;
        mem_info[malloc_mem_index]->last_time = os_jiffies();
    }
    // 没有找到,重新申请一下空间块
    else if (free_mem_index != -1)
    {
        if (c_malloc)
        {
            mem_info[free_mem_index] = c_malloc(sizeof(struct mem_info) + size);
        }

        if (mem_info[free_mem_index])
        {
            sys_dcache_invalid_range((uint32_t *) mem_info[free_mem_index], sizeof(struct mem_info) + size);
            mem_info[free_mem_index]->size      = size;
            mem_info[free_mem_index]->addr      = (uint32_t) (mem_info[free_mem_index] + 1);
            mem_info[free_mem_index]->used      = 1;
            mem_info[free_mem_index]->last_time = os_jiffies();
            malloc_mem_index                    = free_mem_index;
        }
    }
    if (malloc_mem_index != -1)
    {
        return (uint8_t *) mem_info[malloc_mem_index]->addr;
    }
    return NULL;
}

int8_t decode_mem_free(uint8_t *addr)
{
    int8_t ret = -1;
    if (!addr)
    {
        return ret;
    }
    struct mem_info *mem_info = (struct mem_info *) addr;
    if (mem_info)
    {
        mem_info       = mem_info - 1;
        mem_info->used = 0;
        ret            = 0;
    }

    return ret;
}

void decode_mem_free_all(struct mem_info **mem_info, uint32_t max_mem_num, decode_free c_free)
{
    for (int i = 0; i < max_mem_num; i++)
    {
        if (mem_info[i])
        {
            if (c_free)
            {
                c_free(mem_info[i]);
            }
        }
    }
}

void decode_mem_check(struct mem_info **mem_info, uint32_t max_mem_num, uint32_t max_ttl, decode_free c_free)
{
    for (int i = 0; i < max_mem_num; i++)
    {
        // 如果没有被使用,并且超时,则去释放
        if (mem_info[i] && !mem_info[i]->used && os_jiffies() - mem_info[i]->last_time > max_ttl)
        {
            if (c_free)
            {
                c_free(mem_info[i]);
                mem_info[i] = NULL;
            }
        }
    }
}