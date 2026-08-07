#ifndef __AVI_RECORD_MSI_H_
#define __AVI_RECORD_MSI_H_

typedef void (*user_callback)(void *user_priv);

uint32_t* avi_record_msi_init(uint32_t video_width, uint32_t video_height, uint8_t video_fps, uint32_t audio_frq, uint32_t record_time, user_callback cb, void *user_priv);
int avi_record_msi_deinit();

#endif