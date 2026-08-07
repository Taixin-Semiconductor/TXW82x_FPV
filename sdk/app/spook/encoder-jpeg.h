#ifndef __ENCODER_JPEG_H
#define __ENCODER_JPEG_H
#include "stream.h"
struct jpeg_encoder {
	struct stream *output;
	int format;
	struct frame_exchanger *ex;
	volatile int running;
};

struct h264_encoder {
	struct stream *output;
	int format;
	struct frame_exchanger *ex;
	volatile int running;
};
void * file_jpeg_init_ret(void);

void * jpeg_init_ret(void);
void *get_video_ex(struct jpeg_encoder *en);
void * jpeg_init(const rtp_name *jpg_name);


void * file_h264_init_ret(void);
void * h264_init_ret(void);
void *get_video_ex_h264(struct h264_encoder *en);
void * h264_spook_init(const rtp_name *h264_name);

#endif
