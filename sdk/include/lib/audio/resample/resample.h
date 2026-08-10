#ifndef _RESAMPLE_H_
#define _RESAMPLE_H_

#include "typesdef.h"
#include "osal/string.h"

#ifndef _AUDIO_LIB_
#include "hal/aures.h"
extern const struct aures_hal_ops aures_ops;

struct hgaures_v1 {
    struct aures_device   dev;
    const struct aures_hal_ops *ops;
};

int32 hgaures_v1_attach(uint32 dev_id, struct hgaures_v1 *aures);
#endif

void *aures_alloc(uint32 size);
void aures_free(void *ptr);

void *resampler_open(uint32 src_samplerate, uint32 dest_samplerate, uint32 channels);
int32 resampler_process(void *res, int16 *in_data, uint32 in_nsamples, int16 **out_data, uint32 *out_nsamples);
int32 resampler_close(void *res);
int32 resampler_set_src_samplerate(void *res, uint32 samplerate);
int32 resampler_get_src_samplerate(void *res, uint32 *samplerate);
int32 resampler_set_dest_samplerate(void *res, uint32 samplerate);
int32 resampler_get_dest_samplerate(void *res, uint32 *samplerate);
int32 resampler_set_channels(void *res, uint32 channels);
int32 resampler_get_channels(void *res, uint32 *channels);

#endif