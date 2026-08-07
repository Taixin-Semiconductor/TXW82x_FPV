#ifndef _PROMPT_TONE_H_
#define _PROMPT_TONE_H_

#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"

#ifdef PSRAM_HEAP
#define PROMPTTONE_MALLOC av_psram_malloc
#define PROMPTTONE_ZALLOC av_psram_zalloc
#define PROMPTTONE_FREE   av_psram_free
#else
#define PROMPTTONE_MALLOC av_malloc
#define PROMPTTONE_ZALLOC av_zalloc
#define PROMPTTONE_FREE   av_free
#endif

typedef struct {
    const uint8_t *tone_buf;
    void *task_hdl;
    uint32_t tone_buf_size;
    uint32_t tone_buf_offset;
    struct fbpool tx_pool;
    struct msi *msi;
} PROMPT_TONE_STRUCT;

extern const uint8_t connect_mp3[];
extern const uint8_t disconnect_mp3[];

#endif