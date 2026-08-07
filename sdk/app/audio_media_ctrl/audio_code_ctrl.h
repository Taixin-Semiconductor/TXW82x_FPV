#ifndef _AUDIO_CODER_CTRL_H_
#define _AUDIO_CODER_CTRL_H_

#include "basic_include.h"
#include "lib/audio/audio_code/audio_code.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"

#define PACKET_LOSS_CONCEALMENT      -1
#define OUTPUT_MUTE_DATA             -2
#define DECODE_ADD_FADE_IN           -3
#define DECODE_ADD_FADE_OUT          -4

typedef struct {
    int32_t decode_operation;
} AUDECODER_OPERATION;

typedef struct {
    uint16_t nsamples;
    uint16_t time_interval;
    uint32_t samplerate;
} AUDIO_INFO;

typedef struct {
    uint8_t track_type;
    uint8_t priority;
    uint32_t samplerate;
} AUDIO_TRACK;

typedef struct {
    struct msi *src_msi;
	uint8_t destroy_self;
} AUENC_INIT;

typedef struct {
    struct msi *src_msi;
    uint8_t track_type;
    uint8_t priority;
    uint8_t direct_to_dac;
    uint8_t use_tpc;
    uint8_t speed;
    uint8_t pitch;
	uint8_t destroy_self;
} AUDEC_INIT;

enum {
    CALL_TRACK = 1,
    MEDIA_TRACK,
    BELL_TRACK,
};

enum {
    AUCODEC_RUN,
    AUCODEC_PAUSE,
    AUCODEC_END,
    AUCODEC_EXIT,
};

enum {
    coder_clear_event = BIT(0),
    coder_clear_finish_event = BIT(1),
    coder_exit_event = BIT(2),
};

enum {
    play_disabled = 0x00,
    play_interruptible = 0x40,
    play_nonInterruptible = 0x80,
};

struct msi *audio_encode_init(uint32_t coder, uint32_t samplerate, AUENC_INIT *auenc_init);
struct msi *audio_decode_init(uint32_t coder, uint32_t samplerate, AUDEC_INIT *audec_init);
int32_t audio_code_set_src_msi(struct msi *msi, struct msi *src_msi);
int32_t audio_encode_deinit(struct msi *msi);
int32_t audio_decode_deinit(struct msi *msi);
int32_t audio_encode_set_bitrate(struct msi *msi, uint32_t bitrate);
int32_t audio_code_continue(struct msi *msi);
int32_t audio_code_pause(struct msi *msi);
int32_t audio_code_clear(struct msi *msi);
int32_t audio_code_add_output(struct msi *msi, const char *msi_name);
int32_t audio_code_del_output(struct msi *msi, const char *msi_name);
int32_t audio_code_direct_to_dac(struct msi *msi, uint32_t direct_to_dac);
int32_t audio_code_get_status(struct msi *msi);
 
#endif