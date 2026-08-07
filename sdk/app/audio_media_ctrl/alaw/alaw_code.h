#ifndef _ALAW_CODE_H_
#define _ALAW_CODE_H_

#include "basic_include.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "audio_code_ctrl.h"

#ifdef PSRAM_HEAP
#define ALAW_CODE_MALLOC    av_psram_malloc
#define ALAW_CODE_ZALLOC    av_psram_zalloc
#define ALAW_CODE_CALLOC    av_psram_calloc
#define ALAW_CODE_FREE      av_psram_free
#else
#define ALAW_CODE_MALLOC    av_malloc
#define ALAW_CODE_ZALLOC    av_zalloc
#define ALAW_CODE_CALLOC    av_calloc
#define ALAW_CODE_FREE      av_free
#endif

#define ALAW_DEBUG(fmt, args...)     		//os_printf(fmt, ##args)
#define ALAW_INFO      					    os_printf

struct msi *alaw_encode_init(uint32_t samplerate, AUENC_INIT *auenc_init);
struct msi *alaw_decode_init(AUDEC_INIT *audec_init);

#endif