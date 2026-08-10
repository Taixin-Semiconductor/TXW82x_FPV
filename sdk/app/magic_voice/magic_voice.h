#ifndef _MAGIC_VOICE_H_
#define _MAGIC_VOICE_H_

#include "hal/auchange.h"

typedef enum {
    original_voice,
    alien_voice,
    robot_voice,
    hight_voice,
    deep_voice,
    etourdi_voice
}magic_voice_type;

typedef struct {
    uint16 type;
    uint16 frame_size;
    int16 *buf;
    int16 *delay_buf;
    uint32 table_index;
    void *auchange_chan;
    struct auchange_device *auchange_dev;
}magic_voice_struct;

int32 magic_voice_write(magic_voice_struct *magic_voice_s, int16 *data, uint32 *nsamples);
int32 magic_voice_read(magic_voice_struct *magic_voice_s, int16 *data, uint32 *nsamples);
int32 magic_voice_read_available(magic_voice_struct *magic_voice_s);
int32 magic_voice_set_type(uint8_t type);
int32 magic_voice_deinit(magic_voice_struct *magic_voice_s);
magic_voice_struct *magic_voice_init(uint32_t samplerate, uint32_t size);

#endif