#ifndef __DEBUG_LOG_STREAM_H
#define __DEBUG_LOG_STREAM_H
#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "stream_frame.h"

struct dbg_log_s
{
    struct os_work work;
    struct msi *s;
    struct framebuff *fb;  //当前的data_s
    struct fbpool tx_pool;
    uint32_t offset;             //当前数据的偏移,最大值是cache_data_size
};

void hgprntf_debug_log_msi(void *priv, char *str, int32 len);
void *dbg_log_msi(const char *host_name);
#endif