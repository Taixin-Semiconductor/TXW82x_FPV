#ifndef _AAC_CODE_H_
#define _AAC_CODE_H_

#include "basic_include.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "audio_code_ctrl.h"

#ifdef PSRAM_HEAP
#define AAC_CODE_MALLOC    av_psram_malloc
#define AAC_CODE_ZALLOC    av_psram_zalloc
#define AAC_CODE_CALLOC    av_psram_calloc
#define AAC_CODE_FREE      av_psram_free
#else
#define AAC_CODE_MALLOC    av_malloc
#define AAC_CODE_ZALLOC    av_zalloc
#define AAC_CODE_CALLOC    av_calloc
#define AAC_CODE_FREE      av_free
#endif

#define AAC_DEBUG(fmt, args...)     		//os_printf(fmt, ##args)
#define AAC_INFO      					    os_printf

struct msi *aac_encode_init(char *filename, uint32_t samplerate, uint8_t direct_to_record, AUENC_INIT *auenc_init);
struct msi *aac_decode_init(char *filename, uint8_t loop_mode, AUDEC_INIT *audec_init);

#endif