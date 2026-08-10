#ifndef _WSOLA_PROCESS_H_
#define _WSOLA_PROCESS_H_

#include "typesdef.h"
#include "osal/string.h"
#include "lib/audio/ring_buffer/ring_buffer.h"

#ifndef _AUDIO_LIB_
#include "hal/auchange.h"
extern const struct auchange_hal_ops auchange_ops;

struct hgauchange_v1 {
    struct auchange_device   dev;
    const struct auchange_hal_ops *ops;
};

int32 hgauchange_v1_attach(uint32 dev_id, struct hgauchange_v1 *aures);
#endif

typedef struct {
    RINGBUF *input_ringbuf;
    RINGBUF *output_ringbuf;
    int16_t *input_buf;
    int16_t *output_buf;
    int16_t *downsample_buf;
    int16_t *pitch_buf;

    int32_t max_required;
    int32_t max_period_nsamples;
    int32_t min_period_nsamples;
    int32_t prev_period_nsamples;
    int32_t prev_min_diff;

    float pitch;
    float speed;

    float persample_time;
    float input_playtime;
    float time_err;

    int32_t input_nsamples;
    int32_t output_nsamples;
    int32_t pitch_nsamples;
    int32_t reserve_nsamples;

    int32_t old_rate_position;
    int32_t new_rate_position;

    uint32_t input_buf_size;
    uint32_t output_buf_size;
    uint32_t downsample_buf_size;
    uint32_t pitch_buf_size;

    uint32_t samplerate;
    uint32_t channels;
}WsolaStream;

void *wsola_alloc(uint32 size);
void wsola_free(void *ptr);

WsolaStream *wsola_stream_init(uint32_t samplerate, uint32_t channels, 
                               uint32_t max_input_nsamples, uint32_t max_output_nsamples);
int32_t wsola_stream_write_data(WsolaStream* stream, int16_t *data, uint32_t nsamples);
int32_t wsola_stream_read_data(WsolaStream* stream, int16_t *data, uint32_t max_nsamples);
int32_t wsola_stream_write_available(WsolaStream* stream);
int32_t wsola_stream_read_available(WsolaStream* stream);
void wsola_stream_set_pitch(WsolaStream* stream, float pitch);
void wsola_stream_set_speed(WsolaStream* stream, float speed);
float wsola_stream_get_pitch(WsolaStream* stream);
float wsola_stream_get_speed(WsolaStream* stream);
uint32_t wsola_stream_get_samplerate(WsolaStream *stream);
void wsola_stream_clean(WsolaStream* stream);
int32_t wsola_stream_flush(WsolaStream *stream);
void wsola_stream_deinit(WsolaStream *stream);

#endif