#ifndef __DECODE_MEM_H__
#define __DECODE_MEM_H__
// 统计mem空间使用的时间情况,超时1s没有使用,则移除
// 优先使用最近使用的内存池空间
// 32对齐
struct mem_info
{
    uint32_t used : 1, rev : 31;
    uint32_t size;      // 内存块大小
    uint32_t addr;      // 内存块地址
    uint32_t last_time; // 记录上一次使用的时间,预防有异常的时候可以计算超时;
    uint32_t reserve[4];
};
typedef void *(decode_malloc) (int size);
typedef void(decode_free)(void *mem);
uint8_t *decode_mem_malloc(struct mem_info **mem_info, uint32_t max_mem_num, uint32_t size, decode_malloc c_malloc);
int8_t   decode_mem_free(uint8_t *addr);
void     decode_mem_free_all(struct mem_info **mem_info, uint32_t max_mem_num, decode_free c_free);
void     decode_mem_check(struct mem_info **mem_info, uint32_t max_mem_num, uint32_t max_ttl, decode_free c_free);
#endif