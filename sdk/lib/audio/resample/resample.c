#include "basic_include.h"
#include "lib/audio/resample/resample.h"
#include "heap/aurpc_heap.h"

extern void *audio_mem_alloc(uint32_t size, void *priv);
extern void audio_mem_free(void *ptr, void *priv);

typedef struct {
    malloc_cb_t mem_alloc;         
    mfree_cb_t  mem_free;           
    void *priv; 
    void *rpc_priv; 
} AURES_MEM_STRUCT;

AURES_MEM_STRUCT g_aures_mem_s = {
    .mem_alloc = audio_mem_alloc,
    .mem_free = audio_mem_free,
    .priv = &psram_heap,
    .rpc_priv = &aurpc_psram_heap,
};

void *aures_alloc(uint32 size)
{
    void *ptr = NULL;
    if(sysctrl_get_cpu_id() == 0) {
        ptr = g_aures_mem_s.mem_alloc(size, g_aures_mem_s.priv);
    }
    else {
        ptr = g_aures_mem_s.mem_alloc(size, g_aures_mem_s.rpc_priv);
    }
    ASSERT(ptr != NULL);
    return ptr;
}

void aures_free(void *ptr)
{
    if(sysctrl_get_cpu_id() == 0) {
        g_aures_mem_s.mem_free(ptr, g_aures_mem_s.priv);
    }
    else {
        g_aures_mem_s.mem_free(ptr, g_aures_mem_s.rpc_priv);
    }
}

void *audio_resampler_open(struct aures_device *aures, uint32 src_sample_rate, uint32 dest_sample_rate, uint32 channels)
{
	return resampler_open(src_sample_rate, dest_sample_rate, channels);
}

int32 audio_resampler_close(struct aures_device *aures, void *chan)
{
	return resampler_close(chan);
}

int32 audio_resample_frame(struct aures_device *aures, struct aures_req *req)
{
	return resampler_process(req->chan, req->in_data, req->in_nsamples, &(req->outbuf), &(req->out_nsamples));
}

int32 audio_resampler_ctrl(struct aures_device *aures, void *chan, enum aures_ioctl_cmd cmd, uint32 param)
{
	int32 ret = RET_ERR;
	switch(cmd) {
		case AURES_IOCTL_SET_SRC_SAMPLERATE:
			ret = resampler_set_src_samplerate(chan, param);
			break;
		case AURES_IOCTL_GET_SRC_SAMPLERATE:
			ret = resampler_get_src_samplerate(chan, (uint32*)param);
			break;
		case AURES_IOCTL_SET_DEST_SAMPLERATE:
			ret = resampler_set_dest_samplerate(chan, param);
			break;
		case AURES_IOCTL_GET_DEST_SAMPLERATE:
			ret = resampler_get_dest_samplerate(chan, (uint32*)param);
			break;
		case AURES_IOCTL_SET_CHANNELS:
			ret = resampler_set_channels(chan, param);
			break;
		case AURES_IOCTL_GET_CHANNELS:
			ret = resampler_get_channels(chan, (uint32*)param);
			break;
		default:
			break;
	}
	return ret;
}

const struct aures_hal_ops aures_ops = {
    .open     = audio_resampler_open,
    .close    = audio_resampler_close,
    .resample = audio_resample_frame,
	.ioctl    = audio_resampler_ctrl,
};

int32 hgaures_v1_attach(uint32 dev_id, struct hgaures_v1 *aures)
{
    aures->dev.dev.ops        = (const struct devobj_ops *)aures->ops;
    dev_register(dev_id, (struct dev_obj *)aures);
    
    return RET_OK;
}