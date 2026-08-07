#ifndef __CPU1_MEM_H
#define __CPU1_MEM_H
void cpu1_info_free();
void *cpu1_RXBUF_heap_get(uint32_t *size);
void *cpu1_heap_get(uint32_t *size);
void *cpu1_skb_heap_get(uint32_t *size);
#endif