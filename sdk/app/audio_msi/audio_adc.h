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

#define AUADC_TIME_INTERVAL   20
#define AUPROC_FRAME_MS       10
#define MAX_AUADC_TXBUF       4
#define AUADC_TASK_PRIORITY   OS_TASK_PRIORITY_ABOVE_NORMAL

#define AUADC_OUTPUT_SIN      0
#define AUDIO_PROCESS         1

extern int32_t audio_adc_init(enum ausys_ad_platform platform, uint32_t sampleRate, uint32_t channels, uint32_t soft_gain, uint32_t auproc_enable);
extern int32_t audio_adc_deinit(enum ausys_ad_platform platform);
extern int32_t audio_adc_get_samplerate(enum ausys_ad_platform platform);
extern struct msi *get_auadc_msi(enum ausys_ad_platform platform);
extern int32_t auadc_msi_add_output(enum ausys_ad_platform platform, const char *msi_name);
extern int32_t auadc_msi_del_output(enum ausys_ad_platform platform, const char *msi_name);

#endif