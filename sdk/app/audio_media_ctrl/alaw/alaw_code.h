#ifndef _ALAW_CODE_H_
#define _ALAW_CODE_H_

#include "basic_include.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

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

struct msi *alaw_encode_init(uint32_t samplerate);
int32_t alaw_encode_deinit(void);
void alaw_encode_continue(void);
void alaw_encode_pause(void);
void alaw_encode_clear(void);
int32_t alaw_encode_add_output(const char *msi_name);
int32_t alaw_encode_del_output(const char *msi_name);  
uint8_t get_alaw_encode_status(void);

struct msi *alaw_decode_init(uint8_t direct_to_dac);
int32_t alaw_decode_deinit(void);
void alaw_decode_continue(void);
void alaw_decode_pause(void);
void alaw_decode_clear(void);
int32_t alaw_decode_add_output(const char *msi_name);
int32_t alaw_decode_del_output(const char *msi_name);
uint8_t get_alaw_decode_status(void);

#endif