#ifndef _AUDIO_ADC_H_
#define _AUDIO_ADC_H_

#include "basic_include.h"
#include "dev/audio/ausys.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

#ifdef PSRAM_HEAP
#define AUADC_MALLOC    av_psram_malloc
#define AUADC_ZALLOC    av_psram_zalloc
#define AUADC_FREE      av_psram_free
#else
#define AUADC_MALLOC    av_malloc
#define AUADC_ZALLOC    av_zalloc
#define AUADC_FREE      av_free
#endif

#define AUADC_DEBUG(fmt, args...)     		//os_printf(fmt, ##args)
#define AUADC_INFO      					os_printf

#define AUADC_QUEUE_NUM       3

#ifndef MAX_AUADC_TXBUF
#define MAX_AUADC_TXBUF       4
#endif
#ifndef AUADC_TIME_INTERVAL
#define AUADC_TIME_INTERVAL   20
#endif
#ifndef AUADC_TASK_PRIORITY
#define AUADC_TASK_PRIORITY   OS_TASK_PRIORITY_ABOVE_NORMAL
#endif
#ifndef AUDIO_PROCESS
#define AUDIO_PROCESS         0
#endif

#define AUADC_OUTPUT_SIN      0

int32_t audio_adc_init(enum ausys_ad_platform platform, uint32_t sampleRate, uint32_t channels, uint32_t soft_gain, uint32_t auproc_enable);
int32_t audio_adc_deinit(enum ausys_ad_platform platform);
int32_t audio_adc_get_samplerate(enum ausys_ad_platform platform);
struct msi *get_auadc_msi(enum ausys_ad_platform platform);
int32_t auadc_msi_add_output(enum ausys_ad_platform platform, const char *msi_name);
int32_t auadc_msi_del_output(enum ausys_ad_platform platform, const char *msi_name);
int32_t audio_adc_set_soft_gain(enum ausys_ad_platform platform, uint32_t soft_gain);
uint32_t audio_adc_get_energy(enum ausys_ad_platform platform);
#endif