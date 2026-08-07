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
    MSI_AUDAC_SET_FILTER_TRACK,
    MSI_AUDAC_GET_FILTER_TRACK,
    MSI_AUDAC_SET_CALL_VOLUME,
    MSI_AUDAC_GET_CALL_VOLUME,
    MSI_AUDAC_SET_MEDIA_VOLUME,
    MSI_AUDAC_GET_MEDIA_VOLUME,
    MSI_AUDAC_SET_BELL_VOLUME,
    MSI_AUDAC_GET_BELL_VOLUME,
    MSI_AUDAC_CLEAR_STREAM,
    MSI_AUDAC_END_STREAM,
    MSI_AUDAC_GET_EMPTY,
    MSI_AUDAC_HOLD_EMPTY,
    MSI_AUDAC_TEST_MODE,
*/

#define AUDAC_SAMPLERATE      8000
#define AUDAC_QUEUE_NUM       3

#define AUDAC_TIME_INTERVAL   20
#define AUDAC_LEN             1920  //按48k、20ms为最大长度配置
#define MAX_AUDAC_RXBUF       24
#define AUDAC_TASK_PRIORITY   OS_TASK_PRIORITY_ABOVE_NORMAL

extern int32_t audio_dac_init(void);
extern int32_t audio_dac_deinit(void);
#endif