#ifndef _AUDIO_ADC_H_
#define _AUDIO_ADC_H_

#include "basic_include.h"
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

#define AUADC_SAMPLERATE      8000
#define AUADC_QUEUE_NUM       3

#define AUADC_TIME_INTERVAL   20
#define AUPROC_FRAME_MS       10
#define AUADC_LEN             (AUADC_SAMPLERATE/1000*2*AUADC_TIME_INTERVAL)
#define MAX_AUADC_TXBUF       4
#define AUADC_SOFT_GAIN       (4)
#define AUADC_TASK_PRIORITY   OS_TASK_PRIORITY_ABOVE_NORMAL

#define AUADC_OUTPUT_SIN      0
#define AUDIO_PROCESS         0

extern int32_t audio_adc_init(void);
extern int32_t audio_adc_deinit(void);
extern int32 auadc_msi_add_output(const char *msi_name);
extern int32 auadc_msi_del_output(const char *msi_name);

#endif