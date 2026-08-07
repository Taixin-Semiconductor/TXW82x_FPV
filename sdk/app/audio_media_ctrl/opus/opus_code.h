#ifndef _OPUS_CODE_H_
#define _OPUS_CODE_H_

#include "basic_include.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "audio_code_ctrl.h"

#ifdef PSRAM_HEAP
#define OPUS_CODE_MALLOC    av_psram_malloc
#define OPUS_CODE_ZALLOC    av_psram_zalloc
#define OPUS_CODE_CALLOC    av_psram_calloc
#define OPUS_CODE_FREE      av_psram_free
#else
#define OPUS_CODE_MALLOC    av_malloc
#define OPUS_CODE_ZALLOC    av_zalloc
#define OPUS_CODE_CALLOC    av_calloc
#define OPUS_CODE_FREE      av_free
#endif

#define OPUS_DEBUG(fmt, args...)     		//os_printf(fmt, ##args)
#define OPUS_INFO      					    os_printf

struct msi *opus_encode_init(uint32_t samplerate, AUENC_INIT *auenc_init);
struct msi *opus_decode_init(uint32_t samplerate, AUDEC_INIT *audec_init);

#endif