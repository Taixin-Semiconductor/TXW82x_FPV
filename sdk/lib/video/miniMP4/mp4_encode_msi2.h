#ifndef __MP4_ENCODE_MSI2_H__
#define __MP4_ENCODE_MSI2_H__
void mp4_thumb_init();
struct msi *mp4_encode_msi2_init(const char *mp4_msi_name, uint8_t srcID, uint8_t filter_type, uint8_t rec_time, uint32_t audio_encode, struct file_process *file_process, uint8_t mode);
#endif