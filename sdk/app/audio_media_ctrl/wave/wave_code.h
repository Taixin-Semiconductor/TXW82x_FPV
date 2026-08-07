#ifndef _WAVE_CODE_H_
#define _WAVE_CODE_H_

#include "basic_include.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "audio_code_ctrl.h"

#ifdef PSRAM_HEAP
#define WAVE_CODE_MALLOC    av_psram_malloc
#define WAVE_CODE_ZALLOC    av_psram_zalloc
#define WAVE_CODE_FREE      av_psram_free
#else
#define WAVE_CODE_MALLOC    av_malloc
#define WAVE_CODE_ZALLOC    av_zalloc
#define WAVE_CODE_FREE      av_free
#endif

#define WAVE_DEBUG(fmt, args...)         //os_printf(fmt, ##args)
#define WAVE_INFO          				 os_printf

typedef struct _riff_chunk {
	uint8_t  ChunkID[4];
	uint32_t ChunkSize;
	uint8_t  Format[4];
} TYPE_RIFF_CHUNK;
typedef struct _fmt_chunk {
	uint8_t  FmtID[4];
	uint32_t FmtSize;
	uint16_t FmtTag;
	uint16_t FmtChannels;
	uint32_t SampleRate;
	uint32_t ByteRate;
	uint16_t BlockAlign;
	uint16_t BitsPerSample;
} TYPE_FMT_CHUNK;
typedef struct _data_chunk {
	uint8_t  DataID[4];
	uint32_t DataSize;
} TYPE_DATA_CHUNK;
typedef struct _wave_head {
	TYPE_RIFF_CHUNK  riff_chunk;
	TYPE_FMT_CHUNK   fmt_chunk;
	TYPE_DATA_CHUNK  data_chunk;
} TYPE_WAVE_HEAD;

struct msi *wave_encode_init(char *filename, uint32_t samplerate, AUENC_INIT *auenc_init);
struct msi *wave_decode_init(char *filename, uint8_t loop_mode, AUDEC_INIT *audec_init);

#endif