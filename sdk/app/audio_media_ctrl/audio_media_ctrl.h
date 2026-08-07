#ifndef _AUDIO_MEDIA_CTRL_H_
#define _AUDIO_MEDIA_CTRL_H_

#include "basic_include.h"
#include "audio_code_ctrl.h"

enum {
    WAV = 1,
    MP3,
    AMR,
    AAC,
};

enum {
    NORMAL_PLAY = 1,
    LOOP_PLAY,
};

#define RECORD_WAV     1
#define RECORD_AAC     2

#define DEFAULT_RECORD_FORMAT     RECORD_AAC

typedef struct {
    uint8_t filename[30];
    uint8_t record_format;
    uint8_t is_running;
    int32_t record_time;  //seconds 
    uint32_t sampleRate;  
} audio_record_struct;

int32_t audio_file_record_pause(void);
int32_t audio_file_record_continue(void);
int32_t audio_file_record_stop(void);
void audio_file_record_init(char *filename, uint32_t sampleRate, int32_t record_time);
int32_t audio_file_play_pause(struct msi *msi);
int32_t audio_file_play_continue(struct msi *msi);
int32_t audio_file_play_stop(struct msi *msi);
struct msi *audio_file_play_init(char *filename, uint8_t play_mode, AUDEC_INIT *audec_init);

#endif
