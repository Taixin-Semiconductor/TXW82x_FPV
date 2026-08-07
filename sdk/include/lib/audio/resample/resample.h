#ifndef _RESAMPLE_H_
#define _RESAMPLE_H_

#include "typesdef.h"
#include "osal/string.h"

#define aures_memcpy                  os_memcpy
#define aures_memset                  os_memset
#define aures_memmove                 os_memmove

#define AURES_OK                      0
#define AURES_ERR                     -1

typedef void *(*AURES_MALLOC)(int size);
typedef void *(*AURES_ZALLOC)(int size);
typedef void *(*AURES_CALLOC)(int nmemb, int size);
typedef void *(*AURES_REALLOC)(void *ptr, int size);
typedef void (*AURES_FREE)(void *ptr);

extern AURES_MALLOC  aures_malloc;
extern AURES_ZALLOC  aures_zalloc;
extern AURES_CALLOC  aures_calloc;
extern AURES_REALLOC aures_realloc;
extern AURES_FREE    aures_free;

void reg_aures_alloc(AURES_MALLOC m, AURES_ZALLOC z, AURES_CALLOC c, AURES_REALLOC r, AURES_FREE f);

void *resampler_open(uint32_t src_samplerate, uint32_t dest_samplerate, uint32_t channels);
int32_t resampler_reconfig(void *res, uint32_t src_samplerate, uint32_t dest_samplerate);
int32_t resampler_process(void *res, int16_t *in_data, uint32_t in_nsamples, int16_t *out_data, uint32_t *out_nsamples);
void resampler_close(void *res);

#endif