#ifndef _HGIC_MMPOOL_H_
#define _HGIC_MMPOOL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "list.h"


#define MMPOOL_OBJ \
    const char *name;\
    uint32 size, free_size;\
    uint8 region_cnt, align, headsize, tailsize, trace_off, ofchk_off;

struct mmpool_base {
    MMPOOL_OBJ;
};

#define MMPOOL_REGION_MAX (4)
struct mmpool1 {
    MMPOOL_OBJ;
    uint32 regions[MMPOOL_REGION_MAX][2];
    struct list_head free_list;
    struct list_head used_list;
#ifdef MMPOOL_PERF_COUNT
    uint32 tot_alloc, tot_free;
    uint32 alloc_perf[MMPOOL_PERF_COUNT];
    uint32 free_perf[MMPOOL_PERF_COUNT];
#endif
};

#define MMPOOL2_REGION_MAX (4)
#define MMPOOL2_FRAG_LOG   (12)
struct mmpool2 {
    MMPOOL_OBJ;
    uint32 regions[MMPOOL2_REGION_MAX][2];
    struct list_head free_list[MMPOOL2_FRAG_LOG];
    struct list_head used_list;
#ifdef MMPOOL_PERF_COUNT
    uint32 tot_alloc, tot_free;
    uint32 alloc_perf[MMPOOL_PERF_COUNT];
    uint32 free_perf[MMPOOL_PERF_COUNT];
#endif
};

#define MMPOOL3_REGION_MAX (4)
#define MMPOOL3_FRAG_LOG   (12)
struct mmpool3_region {
    uint32 start, end;
    uint32 blk_size, blocks, blk_cnt;
    struct list_head block_list;
};
struct mmpool3 {
    MMPOOL_OBJ;
    uint8 min_size;
    struct mmpool3_region regions[MMPOOL3_REGION_MAX];
    struct list_head used_list;
    struct list_head frag_list[MMPOOL3_FRAG_LOG];
#ifdef MMPOOL_PERF_COUNT
    uint32 tot_alloc, tot_free;
    uint32 alloc_perf[MMPOOL_PERF_COUNT];
    uint32 free_perf[MMPOOL_PERF_COUNT];
#endif
};

struct mmpool_ops {
    void *(*alloc)(void *mp, uint32 size, const char *func, int32 line);
    int32(*free)(void *mp, void *ptr);
    int32(*init)(void *mp, uint32 addr, uint32 size, uint32 blk_size);
    int32(*free_state)(void *mp, uint32 *stat_buf, int32 size, uint32 *tot_size);
    int32(*used_state)(void *mp, uint32 *stat_buf, int32 size, uint32 *tot_size, uint32 mini_size);
    int32(*add_region)(void *mp, uint32 addr, uint32 size, uint32 blk_size);
    int32(*of_check)(void *mp, uint32 addr, uint32 size);
    int32(*valid_addr)(void *pool, uint32 addr, uint8 first);
    int32(*perfermance)(void *mp, uint8 alloc, uint32 *values, uint32 count);
    void (*dump)(void *mp);
    void (*time)(void *mp, uint32 *alloc_time, uint32 *free_time);
    int32 (*used_list)(void *pool, uint32_t *list_buf, int32 list_size);
};

extern const struct mmpool_ops mmpool1_ops;
extern const struct mmpool_ops mmpool2_ops;
extern const struct mmpool_ops mmpool3_ops;

#ifdef __cplusplus
}
#endif
#endif

