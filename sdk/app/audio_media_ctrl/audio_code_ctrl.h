#ifndef _AUDIO_CODER_CTRL_H_
#define _AUDIO_CODER_CTRL_H_

#include "basic_include.h"
#include "lib/audio/audio_code/audio_code.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"
#include "aac_code.h"
#include "alaw_code.h"
#include "amr_decode.h"
#include "mp3_decode.h"
#include "opus_code.h"
#include "wave_code.h"

#define PACKET_LOSS_CONCEALMENT      -1
#define OUTPUT_MUTE_DATA             -2
#define DECODE_ADD_FADE_IN           -3
#define DECODE_ADD_FADE_OUT          -4

typedef struct {
    int32_t decode_operation;
} AUDECODER_OPERATION;

enum {
    AUDIO_RUN,
    AUDIO_PAUSE,
    AUDIO_STOP,
};

enum {
    clear_event = BIT(0),
    clear_finish_event = BIT(1),
    exit_event = BIT(2),
};

const char *audio_code_msi_name(uint32_t coder);
struct msi *audio_encode_init(uint32_t coder, uint32_t samplerate);
void audio_encode_set_bitrate(uint32_t coder, uint32_t bitrate);
struct msi *audio_decode_init(uint32_t coder, uint32_t samplerate, uint8_t direct_to_dac);
int32_t audio_encode_deinit(uint32_t coder);
int32_t audio_decode_deinit(uint32_t coder);
void audio_code_continue(uint32_t coder);
void audio_code_pause(uint32_t coder);
void audio_code_clear(uint32_t coder);
int32_t audio_code_add_output(uint32_t coder, const char *msi_name);
int32_t audio_code_del_output(uint32_t coder, const char *msi_name);
int32_t get_audio_code_status(uint32_t coder);

#endif