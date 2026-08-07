#include "lib/audio/wsola/wsola_process.h"

WSOLA_MALLOC 	wsola_malloc  = NULL;
WSOLA_ZALLOC 	wsola_zalloc  = NULL;
WSOLA_CALLOC 	wsola_calloc  = NULL;
WSOLA_REALLOC   wsola_realloc = NULL;
WSOLA_FREE      wsola_free    = NULL;

void reg_wsola_alloc(WSOLA_MALLOC m, WSOLA_ZALLOC z, WSOLA_CALLOC c, WSOLA_REALLOC r, WSOLA_FREE f)
{
	wsola_malloc  = m;
	wsola_zalloc  = z;
	wsola_calloc  = c;
	wsola_realloc = r;
	wsola_free    = f;
}

WsolaStream *wsolaStream_init(uint32_t samplerate, uint32_t channels, float speed, float pitch, 
                              uint32_t max_input_nsamples, uint32_t max_output_nsamples)
{
    WsolaStream *stream = wsola_stream_init(samplerate, channels, max_input_nsamples, max_output_nsamples);
    if(stream==NULL) {
        return NULL;
    }
    wsola_stream_set_speed(stream, speed);
    wsola_stream_set_pitch(stream, pitch);
    return stream;
}

int32_t wsolaStream_input_data(WsolaStream* stream, int16_t *data, uint32_t nsamples)
{
    return wsola_stream_write_data(stream, data, nsamples);
}

int32_t wsolaStream_output_data(WsolaStream* stream, int16_t *data, uint32_t max_nsamples)
{
    return wsola_stream_read_data(stream, data, max_nsamples);
}

int32_t wsolaStream_input_available(WsolaStream* stream)
{
    return wsola_stream_write_available(stream);
}

int32_t wsolaStream_output_available(WsolaStream* stream)
{
    return wsola_stream_read_available(stream);
}

void wsolaStream_set_pitch(WsolaStream* stream, float pitch)
{
    wsola_stream_set_pitch(stream, pitch);
}

void wsolaStream_set_speed(WsolaStream* stream, float speed)
{
    wsola_stream_set_speed(stream, speed);
}

float wsolaStream_get_pitch(WsolaStream* stream) 
{
    return wsola_stream_get_pitch(stream);
}

float wsolaStream_get_speed(WsolaStream* stream) 
{
    return wsola_stream_get_speed(stream);
}

uint32_t wsolaStream_get_sampleRate(WsolaStream *stream)
{
    return wsola_stream_get_samplerate(stream);
}

void wsolaStream_clean(WsolaStream* stream)
{
    wsola_stream_clean(stream);
}

int32_t wsolaStream_flush(WsolaStream* stream)
{
    return wsola_stream_flush(stream);
}

void wsolaStream_deinit(WsolaStream *stream)
{
    wsola_stream_deinit(stream);
}