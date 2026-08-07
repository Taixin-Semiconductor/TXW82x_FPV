#ifndef _AMR__DECODE_MSI_H_
#define _AMR__DECODE_MSI_H_

#include "basic_include.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

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

struct msi *amr_decode_init(uint8_t *filename, uint8_t direct_to_dac);
int32_t amr_decode_deinit(void);
void amr_decode_continue(void);
void amr_decode_pause(void);
void amr_decode_clear(void);
int32_t amr_decode_add_output(const char *msi_name);
int32_t amr_decode_del_output(const char *msi_name);
uint8_t get_amr_decode_status(void);

#endif