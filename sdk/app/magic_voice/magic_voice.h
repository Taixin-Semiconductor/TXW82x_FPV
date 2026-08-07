#ifndef _MAGIC_VOICE_H_
#define _MAGIC_VOICE_H_

#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"

#ifdef PSRAM_HEAP
#define MAGIC_VOICE_MALLOC av_psram_malloc
#define MAGIC_VOICE_ZALLOC av_psram_zalloc
#define MAGIC_VOICE_FREE   av_psram_free
#else
#define MAGIC_VOICE_MALLOC av_malloc
#define MAGIC_VOICE_ZALLOC av_zalloc
#define MAGIC_VOICE_FREE   av_free
#endif

typedef enum {
    original_voice,
    alien_voice,
    robot_voice,
    hight_voice,
    deep_voice,
    etourdi_voice
}magic_voice_type;

int32_t magic_voice_set_type(uint8_t type);
int32_t magic_voice_msi_add_output(const char *msi_name);
int32_t magic_voice_msi_del_output(const char *msi_name);
struct msi *magic_voice_init(uint32_t samplerate, uint32_t size);
int32_t magic_voice_deinit(void);

#endif