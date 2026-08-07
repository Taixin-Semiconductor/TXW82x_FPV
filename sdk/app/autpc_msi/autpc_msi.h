#ifndef _AUTPC_STREAM_H_
#define _AUTPC_STREAM_H_

#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

#ifdef PSRAM_HEAP
#define AUTPC_MALLOC av_psram_malloc
#define AUTPC_ZALLOC av_psram_zalloc
#define AUTPC_FREE   av_psram_free
#else
#define AUTPC_MALLOC av_malloc
#define AUTPC_ZALLOC av_zalloc
#define AUTPC_FREE   av_free
#endif

struct msi *autpc_msi_init(uint32_t samplerate, uint32_t speed, uint32_t pitch, uint32_t max_inputSamples);

#endif