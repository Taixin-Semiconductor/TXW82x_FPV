#ifndef __VIDEO_APP_H264_MSI_H
#define __VIDEO_APP_H264_MSI_H
#include "lib/multimedia/msi.h"
struct msi *h264_msi_init();
struct msi *h264_msi_init_with_mode(uint32_t drv1_from, uint32_t drv1_w, uint32_t drv1_h, uint32_t drv2_from, uint32_t drv2_w, uint32_t drv2_h);
struct msi *h264_msi_init_with_mode_for_264wq(uint32_t drv1_from, uint16_t drv1_w, uint16_t drv1_h);
#endif