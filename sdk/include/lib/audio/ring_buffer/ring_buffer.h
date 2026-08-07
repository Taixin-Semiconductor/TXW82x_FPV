#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

typedef struct RINGBUF {
    uint8_t *data;
    uint32_t front;
    uint32_t rear;
    uint32_t elementsize;
    uint32_t elementcount;
} RINGBUF;

RINGBUF *ringbuf_Init(uint8_t elementsize, uint32_t elementcount);
int32_t ringbuf_read_available(RINGBUF *ringbuf);
int32_t ringbuf_write_available(RINGBUF *ringbuf);
int32_t ringbuf_read(RINGBUF *ringbuf, void *data, uint32_t elementcount);
int32_t ringbuf_write(RINGBUF *ringbuf, void *data, uint32_t elementcount);
int32_t ringbuf_move_readptr(RINGBUF *ringbuf, int32_t elementcount);
int32_t ringbuf_read_nomove(RINGBUF *ringbuf, void *data, uint32_t elementcount);
void ringbuf_clean(RINGBUF *ringbuf);
void ringbuf_del(RINGBUF *ringbuf);

#endif