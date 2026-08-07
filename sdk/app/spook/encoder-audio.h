#ifndef __ENCODER_AUDIO_H
#define __ENCODER_AUDIO_H

struct audio_encoder {
	struct stream *output;
	int format;
	struct frame_exchanger *ex;
	volatile int running;

};

void * file_audio_init_ret(void);
void * rtp_audio_init_ret(void);
void *get_audio_ex(struct audio_encoder *en);
int rtp_audio_init(void);




#endif
