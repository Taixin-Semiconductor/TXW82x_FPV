#ifndef __AVI_PLAYER_MSI_H
#define __AVI_PLAYER_MSI_H

#include "basic_include.h"
#include "utlist.h"
#include "avi.h"
#include "stream_define.h"
#include "osal_file.h"
#include "lib/heap/av_heap.h"
#include "lib/heap/av_psram_heap.h"
#include "lib/multimedia/msi.h"
#include "lib/multimedia/framebuff.h"

struct msi *avi_player_init(const char *stream_name, const char *filename);

#endif