#include "rtthread.h"

rt_err_t rt_event_init(rt_event_t event, const char *name, rt_uint8_t flag)
{
    return os_event_init(event);
}

rt_err_t rt_event_detach(rt_event_t event)
{
    return os_event_del(event);
}

rt_event_t rt_event_create(const char *name, rt_uint8_t flag)
{
    struct rt_event *event = os_malloc(sizeof(struct rt_event));
    if (event) {
        os_event_init(event);
    }
    return event;
}

rt_err_t rt_event_delete(rt_event_t event)
{
    os_event_del(event);
    os_free(event);
	return RT_EOK;
}

rt_err_t rt_event_send(rt_event_t event, rt_uint32_t set)
{
    return os_event_set(event, set, NULL);
}

rt_err_t rt_event_recv(rt_event_t   event, rt_uint32_t  set, rt_uint8_t opt, rt_int32_t timeout, rt_uint32_t *recved)
{
    return os_event_wait(event, set, recved, opt, timeout);
}

