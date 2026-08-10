#ifndef _AUADC_PROC_H_
#define _AUADC_PROC_H_

#ifndef AUDIO_PROCESS
#define AUDIO_PROCESS     0
#endif

#ifndef MAGIC_VOICE_EN
#define MAGIC_VOICE_EN    0
#endif

struct msi *auadc_proc_init(uint32 samplerate, uint32 channels, uint32 frame_size, uint32 soft_gain);
int32 auadc_proc_deinit(struct msi *msi);

#endif