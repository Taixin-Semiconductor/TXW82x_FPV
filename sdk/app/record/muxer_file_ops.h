#ifndef __MUXER_FILE_OPS_H__
#define __MUXER_FILE_OPS_H__

#include "basic_include.h"

#define MUXER_FILE_ALIGN_SIZE (32U * 1024U)

typedef struct
{
    void *file;

    int (*write)(void *file, const void *buf, uint32_t len);
    int (*skip)(void *file, uint32_t len);
    uint32_t (*tell)(void *file);

    int (*write_at)(void *file, uint32_t offset, const void *buf, uint32_t len);
    int (*flush)(void *file);
    int (*finish)(void *file);
    int (*tail)(void *file, const void *tail, uint32_t tail_len,
                uint32_t logical_end, uint32_t reserved_end);
} file_ops_t;

static inline uint32_t muxer_file_align_up(uint32_t value)
{
    return (value + MUXER_FILE_ALIGN_SIZE - 1U) & ~(MUXER_FILE_ALIGN_SIZE - 1U);
}

#endif
