#ifndef __AVIMUXER_H__
#define __AVIMUXER_H__
#include "basic_include.h"
#include <stdio.h>

void* avimuxer_init (char *file, int duration, int w, int h, int frate, int gop, int h265, int sampnum);
void  avimuxer_exit (void *ctx);
unsigned int  avimuxer_video(void *ctx, unsigned char *buf, int len, int key, unsigned pts);
void  avimuxer_audio(void *ctx, unsigned char *buf, int len, int key, unsigned pts);

void *avimuxer_init2(void *fp,  uint32_t max_size, int w, int h, int frate, int gop, int h265, int sampnum);
uint32_t avimuxer_video2(void *ctx, unsigned char *buf, int len, int key, unsigned pts,uint8_t insert);
void avimuxer_exit2(void *ctx);
uint32_t avimuxer_audio2(void *ctx, unsigned char *buf, int len, int key, unsigned pts);

#endif



