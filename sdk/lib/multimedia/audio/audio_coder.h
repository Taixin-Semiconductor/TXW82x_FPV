#ifndef _AUDIO_CODER_H_
#define _AUDIO_CODER_H_

#include "basic_include.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/audio/wsola/wsola_process.h"
#include "hal/acodec.h"
#include "lib/audio/audio_code/audio_code.h"

#ifdef PSRAM_HEAP
#define aucoder_msi_malloc    os_malloc_psram
#define aucoder_msi_zalloc    os_zalloc_psram
#define aucoder_msi_free      os_free_psram
#else
#define aucoder_msi_malloc    os_malloc
#define aucoder_msi_zalloc    os_zalloc
#define aucoder_msi_free      os_free
#endif

typedef struct {
    WsolaStream  *stream;       
    uint16         speed;        
    uint16         pitch;        
    uint8         enabled:1,    
                  inited:1,     
                  reserved:6;
} AUDIO_SPEED_PITCH;

extern int16 *g_audio_decbuff;
int32 audio_coder_msi_init(uint32 priority, void *stack, uint16 stack_size);
int32 audio_coder_msi_deinit(void);
int32 audio_coder_msi_run(struct os_work *work);
int32 audio_coder_msi_delay_run(struct os_work *work, uint32 delay_ms);
int32 audio_coder_msi_cancle(struct os_work *work);

/**
 * @brief 音频混音器初始化
 * @details 根据采样率、延迟、位深、声道数配置混音器参数，并计算帧大小、初始化缓冲区等
 * 
 * @param sample_rate   采样率 (Hz)。填0则使用默认值16000
 * @param latency_ms    音频延迟 / 帧时长 (ms)。填0则使用默认值20ms
 * @param bit_depth     采样位深 (bit)。填0则使用默认值16bit
 * @param channels      声道数，1=单声道，2=双声道。填0则使用默认值：单声道
 * 
 * @return 初始化结果
 *         - 0：成功
 *         - 负数：失败（参数错误、内存分配失败等）
 */
int32 audio_mixer_init(uint32 sample_rate, uint32 latency_ms, uint32 bit_depth, uint32 channels);

int32 dac_msg_init(uint32 msg, uint32 sample_rate, uint32 channels);
void dac_msg_task_suspend();
void dac_msg_task_resume();
int32 aac_dec_msi_init(void);
int32 aac_enc_msi_init(void);
int32 alaw_dec_msi_init(void);
int32 alaw_enc_msi_init(void);
int32 ulaw_dec_msi_init(void);
int32 ulaw_enc_msi_init(void);
int32 mp3_dec_msi_init(void);
int32 pcm_dec_msi_init(void);
int32 opus_dec_msi_init(void);
int32 opus_enc_msi_init(void);
int32 amrwb_dec_msi_init(void);
int32 amrnb_dec_msi_init(void);

#endif
