#ifndef _SCALE_DEV_H_
#define _SCALE_DEV_H_
#include "hal/scale.h"




void scale_from_vpp(struct scale_device *scale_dev,uint32 yuvbuf_addr,uint32 s_w,uint32 s_h,uint32 d_w,uint32 d_h);
void scale3_all_frame(struct scale_device *scale_dev,uint32_t in_w,uint32_t in_h,uint32_t out_w,uint32_t out_h,uint8_t input_format,uint8_t output_format,uint32_t src,uint32_t dst);
void scale_from_jpeg_config(struct scale_device *scale_dev,uint8_t dirtolcd,uint32 in_w,uint32 in_h,uint32 out_w,uint32 out_h,uint8_t larger);
void scale2_from_jpeg_config_for_msi(struct scale_device *scale_dev,uint32_t yinsram,uint32_t uinsram,uint32_t vinsram,uint32_t yuvoutbuf,uint32 in_w,uint32 in_h,uint32 out_w,uint32 out_h,uint8_t larger);
void scale_from_h264_config(struct scale_device *scale_dev,uint32 in_w,uint32 in_h,uint32 out_w,uint32 out_h,uint8_t larger);
void scale2_all_frame(struct scale_device *scale_dev,uint8_t type,uint32 in_w,uint32 in_h,uint32 out_w,uint32 out_h,uint32 src_addr,uint32 des_addr);
void scale_to_lcd_config(uint32_t iw,uint32_t ih);
void scale2_mutex_init();
void scale3_to_memory_for_thumb(uint16_t iw,uint16_t ih,uint16_t ow,uint16_t oh,uint32_t outadr,uint32_t yuvsram,uint16_t lanenum);
void scale_soft_from_psram_to_enc(struct scale_device *scale_dev,uint8_t * psram_data,uint32_t w,uint32 h,uint32_t ow,uint32_t oh);

#endif
