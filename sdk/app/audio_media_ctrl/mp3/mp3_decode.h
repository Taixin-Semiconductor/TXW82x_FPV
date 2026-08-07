#ifndef _MP3_DECODE_MSI_H_
#define _MP3_DECODE_MSI_H_

#include "basic_include.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

#ifdef PSRAM_HEAP
#define MP3_DECODE_MALLOC    av_psram_malloc
#define MP3_DECODE_ZALLOC    av_psram_zalloc
#define MP3_DECODE_CALLOC    av_psram_calloc
#define MP3_DECODE_FREE      av_psram_free
#else
#define MP3_DECODE_MALLOC    av_malloc
#define MP3_DECODE_ZALLOC    av_zalloc
#define MP3_DECODE_CALLOC    av_calloc
#define MP3_DECODE_FREE      av_free
#endif

#define MP3_DEBUG(fmt, args...)         //os_printf(fmt, ##args)
#define MP3_INFO          				os_printf

struct msi *mp3_decode_init(uint8_t *filename, uint8_t direct_to_dac);
int32_t mp3_decode_deinit(void);
void mp3_decode_continue(void);
void mp3_decode_pause(void);
void mp3_decode_clear(void);
int32_t mp3_decode_add_output(const char *msi_name);
int32_t mp3_decode_del_output(const char *msi_name);
uint8_t get_mp3_decode_status(void);

#endif