#ifndef SCREEN_MEMORY_H
#define SCREEN_MEMORY_H

#include "lib/heap/av_psram_heap.h"
#include "osal/string.h"

#ifndef SCREEN_MALLOC
#define SCREEN_MALLOC(size) av_psram_malloc(size)
#endif

#ifndef SCREEN_FREE
#define SCREEN_FREE(ptr) av_psram_free(ptr)
#endif

#endif
