#ifndef __MUX_FILE_H__
#define __MUX_FILE_H__

#include "basic_include.h"
#include "fatfs/osal_file.h"
#include "lib/video/muxer/muxer_file_ops.h"

#define MUX_FILE_ALIGN_EN   0
#define MUX_FILE_ALIGN_SIZE MUXER_FILE_ALIGN_SIZE

void *mux_file_open(F_FILE *fp, uint32_t prealloc_size, uint8_t align_en);
int mux_file_close(void *file);
int mux_file_write(void *file, const void *buf, uint32_t len);
int mux_file_skip(void *file, uint32_t len);
uint32_t mux_file_tell(void *file);
int mux_file_write_at(void *file, uint32_t offset, const void *buf, uint32_t len);
int mux_file_flush(void *file);
int mux_file_finish(void *file);
int mux_file_tail(void *file, const void *tail, uint32_t tail_len, uint32_t logical_end, uint32_t reserved_end);
void mux_file_get_ops(void *file, file_ops_t *ops);

#endif
