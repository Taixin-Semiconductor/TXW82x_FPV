#ifndef _AAC_CODE_H_
#define _AAC_CODE_H_

#include "basic_include.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

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

struct msi *aac_encode_init(uint8_t *filename, uint32_t samplerate, uint8_t direct_to_record);
int32_t aac_encode_deinit(uint8_t stop_record);
void aac_encode_continue(void);
void aac_encode_pause(void);
void aac_encode_clear(void);
int32_t aac_encode_add_output(const char *msi_name);
int32_t aac_encode_del_output(const char *msi_name);  
uint8_t get_aac_encode_status(void);

struct msi *aac_decode_init(uint8_t *filename, uint8_t direct_to_dac);
int32_t aac_decode_deinit(void);
void aac_decode_continue(void);
void aac_decode_pause(void);
void aac_decode_clear(void);
int32_t aac_decode_add_output(const char *msi_name);
int32_t aac_decode_del_output(const char *msi_name);
uint8_t get_aac_decode_status(void);

#endif