#ifndef __APP_MEM_H
#define __APP_MEM_H

int32_t video_sram_init(uint32_t flags, uint32_t size);
int32_t video_psram_init(uint32_t flags, uint32_t size);
void user_heap_init();

#endif