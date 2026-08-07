#ifndef _OPUS_CODE_H_
#define _OPUS_CODE_H_

#include "basic_include.h"
#include "audio_code_ctrl.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

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

struct msi *opus_encode_init(uint32_t samplerate);
int32_t opus_encode_set_bitrate(uint32_t bitrate);
int32_t opus_encode_deinit(void);
void opus_encode_continue(void);
void opus_encode_pause(void);
void opus_encode_clear(void);
int32_t opus_encode_add_output(const char *msi_name);
int32_t opus_encode_del_output(const char *msi_name);  
uint8_t get_opus_encode_status(void);

struct msi *opus_decode_init(uint32_t samplerate, uint8_t direct_to_dac);
int32_t opus_decode_deinit(void);
void opus_decode_continue(void);
void opus_decode_pause(void);
void opus_decode_clear(void);
int32_t opus_decode_add_output(const char *msi_name);
int32_t opus_decode_del_output(const char *msi_name);  
uint8_t get_opus_decode_status(void);

#endif