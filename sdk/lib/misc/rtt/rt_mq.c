#include "rtthread.h"

rt_err_t rt_mq_init(rt_mq_t mq, const char *name, void *msgpool, rt_size_t msg_size, rt_size_t pool_size, rt_uint8_t flag)
{
    return os_msgq_init(mq, pool_size);
}

rt_err_t rt_mq_detach(rt_mq_t mq)
{
    return os_msgq_del(mq);
}

rt_mq_t rt_mq_create(const char *name, rt_size_t msg_size, rt_size_t max_msgs, rt_uint8_t flag)
{
    struct rt_messagequeue *msgq = os_zalloc(sizeof(struct rt_messagequeue));
    if (msgq) {
        os_msgq_init(msgq, max_msgs);
    }
    return msgq;
}

rt_err_t rt_mq_delete(rt_mq_t mq)
{
    os_msgq_del(mq);
    os_free(mq);
    return RT_EOK;
}

rt_err_t rt_mq_send_wait(rt_mq_t mq, const void *buffer, rt_size_t size, rt_int32_t timeout)
{
    void *ptr = os_malloc(size);
    if (ptr) {
        os_memcpy(ptr, buffer, size);
        if (os_msgq_put(mq, (uint32)ptr, timeout)) {
            os_free(ptr);
            return RT_ERROR;
        }
        return RT_EOK;
    }
    return RT_ENOMEM;
}

rt_err_t rt_mq_send(rt_mq_t mq, const void *buffer, rt_size_t size)
{
    return rt_mq_send_wait(mq, buffer, size, osWaitForever);
}

rt_err_t rt_mq_urgent(rt_mq_t mq, const void *buffer, rt_size_t size)
{
    void *ptr = os_malloc(size);
    if (ptr) {
        os_memcpy(ptr, buffer, size);
        if (os_msgq_put_head(mq, (uint32)ptr, osWaitForever)) {
            os_free(ptr);
            return RT_ERROR;
        }
        return RT_EOK;
    }
    return RT_ENOMEM;
}

rt_err_t rt_mq_recv(rt_mq_t mq, void *buffer, rt_size_t size, rt_int32_t timeout)
{
    void *ptr = (void *)os_msgq_get(mq, timeout);
    if (ptr) {
        os_memcpy(buffer, ptr, size);
        os_free(ptr);
    }
    return RT_EOK;
}

