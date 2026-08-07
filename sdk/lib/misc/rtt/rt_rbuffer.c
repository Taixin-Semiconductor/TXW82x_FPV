#include "rtthread.h"

void rt_ringbuffer_init(struct rt_ringbuffer *rb, rt_uint8_t *pool, rt_int16_t size)
{
    rbuffer_init(rb, size, pool);
}

void rt_ringbuffer_reset(struct rt_ringbuffer *rb)
{
    rbuffer_reset(rb);
}

void rt_ringbuffer_destroy(struct rt_ringbuffer *rb)
{
    rbuffer_destroy(rb);
}

rt_size_t rt_ringbuffer_put(struct rt_ringbuffer *rb, const rt_uint8_t *ptr, rt_uint16_t length)
{
    return rbuffer_set(rb, (void *)ptr, length);
}

rt_size_t rt_ringbuffer_put_force(struct rt_ringbuffer *rb, const rt_uint8_t *ptr, rt_uint16_t length)
{
    return rbuffer_set_force(rb, (void *)ptr, length);
}

rt_size_t rt_ringbuffer_putchar(struct rt_ringbuffer *rb, const rt_uint8_t ch)
{
    return rbuffer_set(rb, (void *)&ch, 1);
}

rt_size_t rt_ringbuffer_putchar_force(struct rt_ringbuffer *rb, const rt_uint8_t ch)
{
    return rbuffer_set_force(rb, (void *)&ch, 1);
}

rt_size_t rt_ringbuffer_get(struct rt_ringbuffer *rb, rt_uint8_t *ptr, rt_uint16_t length)
{
    return rbuffer_get(rb, ptr, length);
}

rt_size_t rt_ringbuffer_getchar(struct rt_ringbuffer *rb, rt_uint8_t *ch)
{
    return rbuffer_get(rb, ch, 1);
}

rt_size_t rt_ringbuffer_data_len(struct rt_ringbuffer *rb)
{
    return RB_COUNT(rb);
}

