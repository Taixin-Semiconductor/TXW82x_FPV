#ifndef __AVIMUXER_H__
#define __AVIMUXER_H__
#include "basic_include.h"
#include "lib/video/muxer/muxer_file_ops.h"
#include <stdio.h>

enum
{
    AVIMUXER_OK,
    AVIMUXER_FULL,
    AVIMUXER_ERR,
};

void avimuxer_sync(void *ctx);
void avimuxer_sync_time(void *ctx, uint32_t time_ms);
void *avimuxer_init(void *fp,  uint32_t max_size, int w, int h, int frate, int h265, int audio_enable);
void *avimuxer_init_with_file(void *fp, const file_ops_t *ops, uint32_t max_size, int w, int h, int frate, int h265, int audio_enable);
void avimuxer_set_file(void *ctx, const file_ops_t *ops);
uint32_t avimuxer_video(void *ctx, unsigned char *buf, int len, int key, uint8_t insert);
uint32_t avimuxer_audio(void *ctx, unsigned char *buf, int len);
void avimuxer_exit(void *ctx);

#endif



