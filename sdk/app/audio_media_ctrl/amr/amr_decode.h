#ifndef _AMR__DECODE_MSI_H_
#define _AMR__DECODE_MSI_H_

#include "basic_include.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "audio_code_ctrl.h"

#ifdef PSRAM_HEAP
#define AMR_DECODE_MALLOC    av_psram_malloc
#define AMR_DECODE_ZALLOC    av_psram_zalloc
#define AMR_DECODE_FREE      av_psram_free
#else
#define AMR_DECODE_MALLOC    av_malloc
#define AMR_DECODE_ZALLOC    av_zalloc
#define AMR_DECODE_FREE      av_free
#endif

#define AMR_DEBUG(fmt, args...)     		//os_printf(fmt, ##args)
#define AMR_INFO      					    os_printf

struct msi *amr_decode_init(char *filename, uint8_t loop_mode, AUDEC_INIT *audec_init);

#endif