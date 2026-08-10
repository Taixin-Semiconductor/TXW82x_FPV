#ifndef _AUDIO_PROCESS_CTRL_H_
#define _AUDIO_PROCESS_CTRL_H_

#include "typesdef.h"
#include "osal/string.h"

#ifndef _AUDIO_LIB_
#include "hal/auproc.h"
extern const struct auproc_hal_ops auproc_ops;

struct hgauproc_v1 {
    struct auproc_device   dev;
    const struct auproc_hal_ops *ops;   ///< 硬件抽象层操作接口表.
    void *auproc_hdl;
};

int32 hgauproc_v1_attach(uint32 dev_id, struct hgauproc_v1 *auproc);
#endif

#ifndef CSI_RFFT_FAST_F32_MAX_LEN
#define CSI_RFFT_FAST_F32_MAX_LEN 1024
#endif

#define ANF_PROCESSING   0
#define AEC_PROCESSING   1
#define AHS_PROCESSING   1
#define ANS_PROCESSING   1   
#define VAD_PROCESSING   0
#define AGC_PROCESSING   0

void *auproc_alloc(uint32 size);
void auproc_free(void *ptr);

enum {
    TYPE_VOICE_ONLY,
    TYPE_MUSIC_ONLY,
    TYPE_HYBRID,
};

enum {
    DETECT_ENERGY,
    DETECT_SPEECH,
};

enum {
    SUPPRESSION_LEVEL_0,
    SUPPRESSION_LEVEL_1,
    SUPPRESSION_LEVEL_2,
    SUPPRESSION_LEVEL_3,
};

typedef struct {
    /********** AEC **********/
    float magnSum_MaxMin_ratio;
    float farend_vad_threshold_ratio;
    float max_filter_coef;
    float far_magnSum_min;
    float far_magn_valid;
    float nearend_min_snr;
    float update_filter_snr;
    float delta1_threshold;
    float delta2_threshold;

    /********** AHS **********/
    float hs_min_ratio;
    float hs_threshold;
    float decrease_speed;
    float increase_speed;

    /********** ANS **********/
    float quantile;
    uint32 suppression_level;

    /********** VAD **********/
    float vad_min_snr;
    float vad_min_pr;
    uint16 min_band_valid_cnt;
    uint16 detect_mode;

    /********** AGC **********/
    int8 target_db;
    int8 max_increase_db;
    int8 max_output_db;
    int8 sub_frame_time_ms;
    float gain_mute_decay;
    float env_decay_voiced;
    float env_decay_mute;
    float update_step;
    float min_env_value;
} AUPROC_CFG;

typedef struct {
    uint32_t samplerate;
    uint32_t channels;

    uint32_t near_discard_ms;
    uint32_t far_discard_ms;

    uint32_t rand_seed;

    float cosTable[256];
    float sinTable[256];

    float *time_inputbuf;
    float *overlap_buf;;
    float *filter_w;
    struct domainTrans_struct *domainTrans_s;

    void *anf;
    void *aec;
    void *ahs;
    void *ans;
    void *vad;
    void *agc;

    float *noiseEsit;
    float *noiseFilter;

    AUPROC_CFG *cfg;
} AUPROC_HDL;

extern AUPROC_CFG auproc_cfg;

AUPROC_HDL *audio_process_init(uint32 sample_rate, uint32 channels, AUPROC_CFG *cfg);
int32 audio_process_prepare(AUPROC_HDL *auproc_hdl);
int32 auproc_put_neardata(AUPROC_HDL *auproc_hdl, int16 *near_data, int16 *out_data, uint32 nsamples);
int32 auproc_put_fardata(AUPROC_HDL *auproc_hdl, int16 *far_data, uint32 nsamples);
int32 audio_process_deinit(AUPROC_HDL *auproc_hdl);

int32 auproc_attach_anf(AUPROC_HDL *auproc_hdl);
int32 auproc_attach_aec(AUPROC_HDL *auproc_hdl);
int32 auproc_attach_ahs(AUPROC_HDL *auproc_hdl);
int32 auproc_attach_ans(AUPROC_HDL *auproc_hdl);
int32 auproc_attach_vad(AUPROC_HDL *auproc_hdl);
int32 auproc_attach_agc(AUPROC_HDL *auproc_hdl);

#endif