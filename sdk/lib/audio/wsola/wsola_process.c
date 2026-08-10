#include "basic_include.h"
#include "lib/audio/wsola/wsola_process.h"
#include "heap/aurpc_heap.h"

extern void *audio_mem_alloc(uint32_t size, void *priv);
extern void audio_mem_free(void *ptr, void *priv);

typedef struct {
    malloc_cb_t mem_alloc;         
    mfree_cb_t  mem_free;           
    void *priv; 
    void *rpc_priv; 
} WSOLA_MEM_STRUCT;

WSOLA_MEM_STRUCT g_wsola_mem_s = {
    .mem_alloc = audio_mem_alloc,
    .mem_free = audio_mem_free,
    .priv = &psram_heap,
    .rpc_priv = &aurpc_psram_heap,
};

void *wsola_alloc(uint32 size)
{
    void *ptr = NULL;
    if(sysctrl_get_cpu_id() == 0) {
        ptr = g_wsola_mem_s.mem_alloc(size, g_wsola_mem_s.priv);
    }
    else {
        ptr = g_wsola_mem_s.mem_alloc(size, g_wsola_mem_s.rpc_priv);
    }
    ASSERT(ptr != NULL);
    return ptr;
}

void wsola_free(void *ptr)
{
    if(sysctrl_get_cpu_id() == 0) {
        g_wsola_mem_s.mem_free(ptr, g_wsola_mem_s.priv);
    }
    else {
        g_wsola_mem_s.mem_free(ptr, g_wsola_mem_s.rpc_priv);
    }
}

void *wsolaStream_open(struct auchange_device *auchange, uint32 sample_rate, uint32 channels, uint32 max_nsamples)
{
    WsolaStream *stream = wsola_stream_init(sample_rate, channels, max_nsamples*3, max_nsamples*6);
    if(stream==NULL) {
        return NULL;
    }
    wsola_stream_set_speed(stream, 1.0f);
    wsola_stream_set_pitch(stream, 1.0f);
    return stream;
}

int32 wsolaStream_close(struct auchange_device *auchange, void *chan)
{
    wsola_stream_deinit((WsolaStream*)chan);
    return RET_OK;
}

int32 wsolaStream_write(struct auchange_device *auchange, struct auchange_req *req)
{
    int32 nsamples = wsola_stream_write_data((WsolaStream*)(req->chan), req->data, req->nsamples);
    if(req->nsamples == nsamples) {
        return RET_OK;
    }
    req->nsamples = nsamples;
    return RET_ERR;
}

int32 wsolaStream_read(struct auchange_device *auchange, struct auchange_req *req)
{
    int32 nsamples = wsola_stream_read_data((WsolaStream*)(req->chan), req->data, req->nsamples);
    if(req->nsamples == nsamples) {
        return RET_OK;
    }
    req->nsamples = nsamples;
    return RET_ERR;
}

int32 wsolaStream_ctrl(struct auchange_device *auchange, void *chan, enum auchange_ioctl_cmd cmd, uint32 param)
{
    int32 ret = RET_ERR;
    switch(cmd) {
        case AUCHANGE_IOCTL_SET_SPEED:
            wsola_stream_set_speed(chan, (float)param / 100.0f);
            ret = RET_OK;
            break;
        case AUCHANGE_IOCTL_SET_PITCH:
            wsola_stream_set_pitch(chan, (float)param / 100.0f);
            ret = RET_OK;
            break;
        case AUCHANGE_IOCTL_WRITE_AVAILABLE:
            *((int32*)param) = wsola_stream_write_available((WsolaStream*)chan);
            ret = RET_OK;
            break;
        case AUCHANGE_IOCTL_READ_AVAILABLE:
            *((int32*)param) = wsola_stream_read_available((WsolaStream*)chan);
            ret = RET_OK;
            break;
        case AUCHANGE_IOCTL_FLUSH_STREAM:
            wsola_stream_flush((WsolaStream*)chan);
            ret = RET_OK;
            break;  
        case AUCHANGE_IOCTL_CLEAN_STREAM:
            wsola_stream_clean((WsolaStream*)chan);
            ret = RET_OK;
            break;           
        default:break;
    }
    return ret;
}

const struct auchange_hal_ops auchange_ops = {
    .open     = wsolaStream_open,
    .close    = wsolaStream_close,
    .write    = wsolaStream_write,
    .read     = wsolaStream_read,
	.ioctl    = wsolaStream_ctrl,
};

int32 hgauchange_v1_attach(uint32 dev_id, struct hgauchange_v1 *auchange)
{
    auchange->dev.dev.ops        = (const struct devobj_ops *)auchange->ops;
    dev_register(dev_id, (struct dev_obj *)auchange);
    
    return RET_OK;
}