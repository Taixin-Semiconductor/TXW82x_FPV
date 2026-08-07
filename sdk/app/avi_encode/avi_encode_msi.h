#ifndef __AVI_ENCODE_MSI_H__
#define __AVI_ENCODE_MSI_H__
#include "file_process.h"
struct msi *avi_encode_msi_init(const char *avi_msi_name, uint16_t filter_type, uint8_t rec_time);
struct msi *avi_encode_msi2_init(const char *avi_msi_name, uint8_t srcID, uint8_t filter_type, uint8_t rec_time, 
                                uint32_t audio_encode, struct file_process *file_process, uint8_t mode);


#endif
