#ifndef _MEM_POOL_H_
#define _MEM_POOL_H_

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_MAX (12)

/*free frag list*/
struct frag_list {
    struct frag_list *next;
    struct frag_list *prev;
};

struct mem_pool {
    unsigned int block_size: 24, init:1, rev: 7;
    unsigned int block_count;
    unsigned int heap_start;
    unsigned int heap_end;
    unsigned int heap_ptr;
    unsigned int heap_base;
    unsigned int blocks;
    struct frag_list  frags[LOG_MAX];
#ifdef MMPOOL_PERF_COUNT
    uint32 alloc_perf[MMPOOL_PERF_COUNT];
    uint32 free_perf[MMPOOL_PERF_COUNT];
#endif
};

#define mpool_free_size(pool) ((pool)->heap_end-(pool)->heap_ptr)
int32 mpool_init(struct mem_pool *pool, uint32 block_size, uint32 mem_addr, uint32 mem_size);
void mpool_free(struct mem_pool *pool, void *ptr);
void *mpool_alloc(struct mem_pool *pool, uint32 size);
void mpool_state(struct mem_pool *pool, char *state_buf, int32 buf_size);

#ifdef __cplusplus
}
#endif
#endif
