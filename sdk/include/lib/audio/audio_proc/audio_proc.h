#ifndef _AUDIO_PROCESS_CTRL_H_
#define _AUDIO_PROCESS_CTRL_H_

#include "basic_include.h"  

#define AUPROC_STACK_SIZE     5120

#define ANF_PROCESSING   0
#define AEC_PROCESSING   1
#define AHS_PROCESSING   1
#define ANS_PROCESSING   1   
#define RES_PROCESSING   1

typedef void *(*AUPROC_MALLOC)(int size);
typedef void *(*AUPROC_ZALLOC)(int size);
typedef void *(*AUPROC_CALLOC)(int nmemb, int size);
typedef void *(*AUPROC_REALLOC)(void *ptr, int size);
typedef void (*AUPROC_FREE)(void *ptr);

extern volatile AUPROC_MALLOC  auproc_malloc;
extern volatile AUPROC_ZALLOC  auproc_zalloc;
extern volatile AUPROC_CALLOC  auproc_calloc;
extern volatile AUPROC_REALLOC auproc_realloc;
extern volatile AUPROC_FREE    auproc_free;

enum {
    TYPE_VOICE_ONLY,
    TYPE_MUSIC_ONLY,
    TYPE_HYBRID,
};

typedef struct {
    int8_t *stack_priv;
    uint32_t used_size;
    uint32_t total_size;    
} AUPROC_STACK;

typedef struct {
    uint32_t samplerate;
    uint32_t channels;

    uint32_t near_discard_ms;
    uint32_t far_discard_ms;

    float *time_inputbuf;
    float *freq_buf;
    float *time_outputbuf;
    int16_t *overlap_buf;
    struct domainTrans_struct *domainTrans_s;

    void *anf;
    void *aec;
    void *ahs;
    void *ans;
    void *vad;
    void *agc;
    void *res;

    float *noiseEsit;
} AUPROC_HDL;

AUPROC_HDL *audio_process_init(uint32_t samplerate, uint32_t channels, uint32_t frame_size);
int32_t audio_process_data(AUPROC_HDL *auproc_hdl, int16_t *data, uint32_t nsamples);
int32_t audio_process_fardata(AUPROC_HDL *auproc_hdl, int16_t *data, uint32_t nsamples);
int32_t audio_resample_config(AUPROC_HDL *auproc_hdl, uint32_t src_samplerate, uint32_t dest_samplerate);
int32_t audio_resample_data(AUPROC_HDL *auproc_hdl, int16_t *in_data, uint32_t in_size, int16_t *out_data, uint32_t *out_size);
int32_t audio_process_deinit(AUPROC_HDL *auproc_hdl);

void reg_auproc_alloc(AUPROC_MALLOC m, AUPROC_ZALLOC z, AUPROC_CALLOC c, AUPROC_REALLOC r, AUPROC_FREE f);
void *auproc_stack_alloc(uint32_t size);
void auproc_stack_free(void *ptr);
AUPROC_HDL *audio_process_open(uint32_t samplerate, uint32_t channels, uint32_t frame_size);
int32_t audio_process_prepare(AUPROC_HDL *auproc_hdl);
int32_t audio_process(AUPROC_HDL *auproc_hdl, int16_t *data, uint32_t size);
int32_t audio_process_put_fardata(AUPROC_HDL *auproc_hdl, int16_t *data, uint32_t size);
int32_t audio_resample_reconfig(AUPROC_HDL *auproc_hdl, uint32_t src_samplerate, uint32_t dest_samplerate);
int32_t audio_resample(AUPROC_HDL *auproc_hdl, int16_t *in_data, uint32_t in_size, int16_t *out_data, uint32_t *out_size);
int32_t audio_process_close(AUPROC_HDL *auproc_hdl);

int32_t auproc_attach_anf(AUPROC_HDL * auproc_hdl);
int32_t auproc_attach_aec(AUPROC_HDL * auproc_hdl);
int32_t auproc_attach_ahs(AUPROC_HDL * auproc_hdl, uint8_t mode);
int32_t auproc_attach_ans(AUPROC_HDL * auproc_hdl, uint8_t mode);
int32_t auproc_attach_res(AUPROC_HDL * auproc_hdl);
#endif