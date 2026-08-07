#ifndef _AUDIO_DAC_H_
#define _AUDIO_DAC_H_

#include "basic_include.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

#ifdef PSRAM_HEAP
#define AUDAC_MALLOC    av_psram_malloc
#define AUDAC_ZALLOC    av_psram_zalloc
#define AUDAC_FREE      av_psram_free
#else
#define AUDAC_MALLOC    av_malloc
#define AUDAC_ZALLOC    av_zalloc
#define AUDAC_FREE      av_free
#endif

#define AUDAC_DEBUG(fmt, args...)     	//os_printf(fmt, ##args)
#define AUDAC_INFO      				os_printf
/*support cmd*/
/*
    MSI_AUDAC_SET_SAMPLING_RATE,
    MSI_AUDAC_GET_SAMPLING_RATE,
    MSI_AUDAC_SET_FILTER_TYPE,
    MSI_AUDAC_GET_FILTER_TYPE,
    MSI_AUDAC_SET_VOLUME,
    MSI_AUDAC_GET_VOLUME,
    MSI_AUDAC_SET_SPEED,
    MSI_AUDAC_GET_SPEED,	
    MSI_AUDAC_SET_PITCH,
    MSI_AUDAC_GET_PITCH,
    MSI_AUDAC_CLEAR_STREAM,
    MSI_AUDAC_END_STREAM,
    MSI_AUDAC_GET_EMPTY,
*/

#define AUDAC_SAMPLERATE      8000
#define AUDAC_QUEUE_NUM       3

#define AUDAC_TIME_INTERVAL   20
#define AUDAC_LEN             1920  //按48k、20ms为最大长度配置
#define MAX_AUDAC_RXBUF       4
#define AUDAC_TASK_PRIORITY   OS_TASK_PRIORITY_ABOVE_NORMAL

#define AUDAC_TPC_PROCESS     0

extern int32_t audio_dac_init(void);
extern int32_t audio_dac_deinit(void);
#endif