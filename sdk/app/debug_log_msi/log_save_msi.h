#ifndef __LOG_SAVE_STREAM_H
#define __LOG_SAVE_STREAM_H

#include "basic_include.h"
#include "lib/multimedia/msi.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "stream_frame.h"
#include "osal_file.h"

#define CACHE_SIZE  (4096)

struct log_save_s
{
    struct os_work work;
    struct msi *s;
    uint32_t save_len;
    uint8_t save_buf[CACHE_SIZE]; //设置一个缓冲区,暂时是4K
    uint16_t offset;        //这个只会小于等于CACHE_SIZE
    uint16_t start_offset;  //因为写入sd存在可能不对齐的问题,所以这里会有一个基础偏移来作为对齐
    uint32_t last_syn_time;        //上一次同步的时间
    uint32_t lastupdate_file_time;  //上一次更换文件的时间       
    uint32_t syn_interval;          //同步间隔
    uint32_t update_file_interval;  //定时切换log文件,尽量保证旧文件是完整的
    uint32_t filesize;
    void *fp;
};

void *creat_log_save_stream(const char *name,uint32_t syn_interval,uint32_t update_file_interval);

#endif

