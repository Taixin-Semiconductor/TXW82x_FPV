/*
 * @Author: jiejie
 * @Github: https://github.com/jiejieTop
 * @Date: 2019-12-14 22:02:07
 * @LastEditTime: 2020-04-27 16:32:58
 * @Description: the code belongs to jiejie, please keep the author information and source code according to the license.
 */
#include "platform_memory.h"

void *platform_memory_alloc(size_t size)
{
    return os_zalloc(size);
}

void *platform_memory_calloc(size_t num, size_t size)
{
    return os_calloc(num, size);
}

void platform_memory_free(void *ptr)
{
    os_free(ptr);
}
