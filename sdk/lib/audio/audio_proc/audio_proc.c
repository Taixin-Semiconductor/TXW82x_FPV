#include "basic_include.h"
#include "lib/audio/audio_proc/audio_proc.h"

AUPROC_STACK *g_auproc_stack = NULL;

volatile AUPROC_MALLOC 	 auproc_malloc  = NULL;
volatile AUPROC_ZALLOC 	 auproc_zalloc  = NULL;
volatile AUPROC_CALLOC 	 auproc_calloc  = NULL;
volatile AUPROC_REALLOC  auproc_realloc = NULL;
volatile AUPROC_FREE     auproc_free    = NULL;

void reg_auproc_alloc(AUPROC_MALLOC m, AUPROC_ZALLOC z, AUPROC_CALLOC c, AUPROC_REALLOC r, AUPROC_FREE f)
{
	auproc_malloc  = m;
	auproc_zalloc  = z;
	auproc_calloc  = c;
	auproc_realloc = r;
	auproc_free    = f;
}

void *auproc_stack_alloc(uint32_t size)
{
    void *return_addr = NULL;
	if((size%32)!=0) {
		size = (size+32)/32*32;
	}
	// _os_printf("%s %d %d\n",__FUNCTION__,g_auproc_stack->used_size,size);
    if((g_auproc_stack->used_size+sizeof(uint32_t)+size)>g_auproc_stack->total_size) {
        while(1) {
            _os_printf("auproc_stack_alloc fail\n");
            os_sleep_ms(1000);
        }
    }
    *((uint32_t*)(g_auproc_stack->stack_priv+g_auproc_stack->used_size+size)) = size;
    return_addr = g_auproc_stack->stack_priv+g_auproc_stack->used_size;
    // _os_printf("%s %p %p\n",__FUNCTION__,return_addr,(g_auproc_stack->stack_priv+g_auproc_stack->used_size+size));
    g_auproc_stack->used_size += (sizeof(uint32_t)+size);
    return return_addr;
}

void auproc_stack_free(void *ptr)
{
    uint32_t last_size = *((uint32_t*)(g_auproc_stack->stack_priv+g_auproc_stack->used_size-4));
    // _os_printf("%s %p %p %d\n",__FUNCTION__,ptr,(g_auproc_stack->stack_priv+g_auproc_stack->used_size-4),last_size);
    if(ptr != (g_auproc_stack->stack_priv+g_auproc_stack->used_size-last_size-4)) {
        while(1) {
            _os_printf("auproc_stack_free fail\n");
            os_sleep_ms(1000);
        }
    }
    g_auproc_stack->used_size -= (sizeof(uint32_t)+last_size);
}

AUPROC_HDL *audio_process_init(uint32_t samplerate, uint32_t channels, uint32_t frame_size)
{
    int32_t ret = 0;
	AUPROC_HDL *auproc_hdl = NULL;
    g_auproc_stack = (AUPROC_STACK*)auproc_zalloc(sizeof(AUPROC_STACK));
    if(g_auproc_stack == NULL) {
        os_printf("alloc auproc stack fail\n");
        return NULL;
    }
    g_auproc_stack->stack_priv = (int8_t*)auproc_malloc(sizeof(int8_t)*AUPROC_STACK_SIZE);
    if(g_auproc_stack->stack_priv == NULL) {
        os_printf("alloc g_auproc_stack->stack_priv fail\n");
        goto audio_process_init_err;
    }
    g_auproc_stack->total_size = AUPROC_STACK_SIZE;
    auproc_hdl = audio_process_open(samplerate, channels, frame_size);
    if(!auproc_hdl) {
        os_printf("audio_process_open fail!\n");
        goto audio_process_init_err;
    }
#if ANF_PROCESSING
    ret = auproc_attach_anf(auproc_hdl);
    if(ret != 0) {
        os_printf("auproc_attach_anf fail!\n");
        goto audio_process_init_err;        
    }
#endif
#if AEC_PROCESSING
    ret = auproc_attach_aec(auproc_hdl);
    if(ret != 0) {
        os_printf("auproc_attach_aec fail!\n");
        goto audio_process_init_err;        
    }
#endif
#if AHS_PROCESSING
    ret = auproc_attach_ahs(auproc_hdl, 3);
    if(ret != 0) {
        os_printf("auproc_attach_ahs fail!\n");
        goto audio_process_init_err;        
    }
#endif
#if ANS_PROCESSING
    ret = auproc_attach_ans(auproc_hdl, 3);
    if(ret != 0) {
        os_printf("auproc_attach_ans fail!\n");
        goto audio_process_init_err;        
    }
#endif
#if RES_PROCESSING
    ret = auproc_attach_res(auproc_hdl);
    if(ret != 0) {
        os_printf("auproc_attach_res fail!\n");
        goto audio_process_init_err;        
    }
#endif
    ret = audio_process_prepare(auproc_hdl);
    if(ret != RET_OK) {
        os_printf("audio_process_prepare fail!\n");
        goto audio_process_init_err;        
    }
    return auproc_hdl;
audio_process_init_err:
    if(auproc_hdl) {
        audio_process_close(auproc_hdl);
    }
    if(g_auproc_stack->stack_priv) {
        auproc_free(g_auproc_stack->stack_priv);
    }
    if(g_auproc_stack) {
        auproc_free(g_auproc_stack);
        g_auproc_stack = NULL;
    }
    return NULL;
}

int32_t audio_process_data(AUPROC_HDL *auproc_hdl, int16_t *data, uint32_t nsamples)
{
    return audio_process(auproc_hdl, data, nsamples);
}

int32_t audio_process_fardata(AUPROC_HDL *auproc_hdl, int16_t *data, uint32_t nsamples)
{
    return audio_process_put_fardata(auproc_hdl, data, nsamples);
}

int32_t audio_resample_config(AUPROC_HDL *auproc_hdl, uint32_t src_samplerate, uint32_t dest_samplerate)
{
    return audio_resample_reconfig(auproc_hdl, src_samplerate, dest_samplerate);
}

int32_t audio_resample_data(AUPROC_HDL *auproc_hdl, int16_t *in_data, uint32_t in_size, int16_t *out_data, uint32_t *out_size)
{
    return audio_resample(auproc_hdl, in_data, in_size, out_data, out_size);
}

int32_t audio_process_deinit(AUPROC_HDL *auproc_hdl)
{
    if(g_auproc_stack->stack_priv) {
        auproc_free(g_auproc_stack->stack_priv);
    }
    if(g_auproc_stack) {
        auproc_free(g_auproc_stack);
        g_auproc_stack = NULL;
    }
    return audio_process_close(auproc_hdl);
}