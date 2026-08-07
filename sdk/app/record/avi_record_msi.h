#ifndef __AVI_RECORD_MSI_H_
#define __AVI_RECORD_MSI_H_

#include "lib/multimedia/msi.h"
#include "app/recorder/file_process.h"

struct msi *avi_record_msi_init(const char *avi_msi_name, uint8_t srcID, uint8_t filter_type, uint8_t rec_time,
                                uint32_t audio_encode, struct file_process *file_process, uint8_t mode);

#endif
