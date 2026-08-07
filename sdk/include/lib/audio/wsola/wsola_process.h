#ifndef _WSOLA_PROCESS_H_
#define _WSOLA_PROCESS_H_

#include "basic_include.h"
#include "lib/audio/ring_buffer/ring_buffer.h"

typedef void *(*WSOLA_MALLOC)(int size);
typedef void *(*WSOLA_ZALLOC)(int size);
typedef void *(*WSOLA_CALLOC)(int nmemb, int size);
typedef void *(*WSOLA_REALLOC)(void *ptr, int size);
typedef void (*WSOLA_FREE)(void *ptr);

extern WSOLA_MALLOC  wsola_malloc;
extern WSOLA_ZALLOC  wsola_zalloc;
extern WSOLA_CALLOC  wsola_calloc;
extern WSOLA_REALLOC wsola_realloc;
extern WSOLA_FREE    wsola_free;

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

void reg_wsola_alloc(WSOLA_MALLOC m, WSOLA_ZALLOC z, WSOLA_CALLOC c, WSOLA_REALLOC r, WSOLA_FREE f);
WsolaStream *wsolaStream_init(uint32_t samplerate, uint32_t channels, float speed, float pitch, 
                              uint32_t max_input_nsamples, uint32_t max_output_nsamples);
int32_t wsolaStream_input_data(WsolaStream* stream, int16_t *data, uint32_t nsamples);
int32_t wsolaStream_output_data(WsolaStream* stream, int16_t *data, uint32_t max_nsamples);
int32_t wsolaStream_input_available(WsolaStream* stream);
int32_t wsolaStream_output_available(WsolaStream* stream);
void wsolaStream_set_pitch(WsolaStream* stream, float pitch);
void wsolaStream_set_speed(WsolaStream* stream, float speed);
float wsolaStream_get_pitch(WsolaStream* stream);
float wsolaStream_get_speed(WsolaStream* stream);
uint32_t wsolaStream_get_sampleRate(WsolaStream *stream);
void wsolaStream_clean(WsolaStream* stream);
int32_t wsolaStream_flush(WsolaStream* stream);
void wsolaStream_deinit(WsolaStream *stream);

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